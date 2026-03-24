#include "http_api.h"

#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "lwip/inet.h"

#include "esp_http_server.h"
#include "esp_log.h"
#include "esp_spiffs.h"

#include <string.h>
#include <stdlib.h>

#include "app_config.h"
#include "camera_worker.h"
#include "analysis.h"
#include "utils.h"
#include "api.h"


static const char *TAG = "http_api";

static httpd_handle_t g_api_server = NULL;

uint16_t roi_x0, roi_y0, roi_dx, roi_dy;

// Init
void http_api_init(void) {

    wifi_init_sta();

    ESP_ERROR_CHECK(http_api_start());
}

/* =========================
   WiFi event handling
   ========================= */
static EventGroupHandle_t s_wifi_event_group;
#define WIFI_CONNECTED_BIT BIT0

static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data) {
    (void)arg;

    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        ESP_LOGI(TAG, "WiFi started, connecting...");
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        ESP_LOGW(TAG, "WiFi disconnected, retrying...");
        esp_wifi_connect();
        xEventGroupClearBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        ESP_LOGI(TAG, "Got IP: " IPSTR, IP2STR(&event->ip_info.ip));
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

void wifi_init_sta(void) {
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    s_wifi_event_group = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL));

    wifi_config_t wifi_config = {0};
    strncpy((char *)wifi_config.sta.ssid, WIFI_SSID, sizeof(wifi_config.sta.ssid));
    strncpy((char *)wifi_config.sta.password, WIFI_PASS, sizeof(wifi_config.sta.password));

    wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
    wifi_config.sta.pmf_cfg.capable = true;
    wifi_config.sta.pmf_cfg.required = false;

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "wifi_init_sta finished");
}

// ========================
// SPIFFS mount
// ========================
static esp_err_t spiffs_mount(void) {
    esp_vfs_spiffs_conf_t conf = {
        .base_path = "/spiffs",
        .partition_label = "spiffs",
        .max_files = 8,
        .format_if_mount_failed = false,
    };
    esp_err_t ret = esp_vfs_spiffs_register(&conf);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "SPIFFS mount failed: %s", esp_err_to_name(ret));
        return ret;
    }
    size_t total = 0, used = 0;
    ret = esp_spiffs_info("spiffs", &total, &used);
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "SPIFFS mounted: total=%u, used=%u", (unsigned)total, (unsigned)used);
    } else {
        ESP_LOGW(TAG, "esp_spiffs_info failed: %s", esp_err_to_name(ret));
    }
    return ESP_OK;
}

// ========================
// Helpers
// ========================
static const char *get_mime_type(const char *path) {
    if (strstr(path, ".html")) return "text/html";
    if (strstr(path, ".js")) return "application/javascript";
    if (strstr(path, ".css")) return "text/css";
    if (strstr(path, ".png")) return "image/png";
    if (strstr(path, ".jpg")) return "image/jpeg";
    if (strstr(path, ".ico")) return "image/x-icon";
    return "text/plain";
}

static esp_err_t send_file(httpd_req_t *req, const char *filepath) {
    FILE *f = fopen(filepath, "rb");
    if (!f) {
        ESP_LOGW(TAG, "File not found: %s", filepath);
        httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "File not found");
        return ESP_OK;
    }
    httpd_resp_set_type(req, get_mime_type(filepath));
    httpd_resp_set_hdr(req, "Cache-Control", "no-store");
    char buf[1024];
    size_t read_bytes;
    while ((read_bytes = fread(buf, 1, sizeof(buf), f)) > 0) {
        esp_err_t res = httpd_resp_send_chunk(req, buf, read_bytes);
        if (res != ESP_OK) {
            ESP_LOGW(TAG, "send_chunk failed (%s)", esp_err_to_name(res));
            fclose(f);
            httpd_resp_send_chunk(req, NULL, 0);
            return res;
        }
    }
    fclose(f);
    httpd_resp_send_chunk(req, NULL, 0);
    return ESP_OK;
}


// ========================
// HTTP handlers
// ========================
static esp_err_t index_handler(httpd_req_t *req) {
    // / -> /spiffs/index.html
    return send_file(req, "/spiffs/index.html");
}

static esp_err_t appjs_handler(httpd_req_t *req) {
    return send_file(req, "/spiffs/app.js");
}

static esp_err_t style_handler(httpd_req_t *req) {
    return send_file(req, "/spiffs/style.css");
}

static esp_err_t camera_handler(httpd_req_t *req) {
    ESP_LOGI(TAG, "Camera request: %s", req->uri);

    const char *uri = req->uri;
    const char *prefix = "/camera/";
    size_t prefix_len = strlen(prefix);

    /* Check prefix */
    if (strncmp(uri, prefix, prefix_len) != 0) {
        httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "Invalid camera path");
        return ESP_FAIL;
    }

    /* Find query mark */
    const char *query_mark = strchr(uri, '?');
    size_t path_len = query_mark ? (size_t)(query_mark - uri)
                                 : strlen(uri);

    /* Extract subpath */
    const char *subpath = uri + prefix_len;
    size_t subpath_len = path_len - prefix_len;

    /* Extract query (if exists) */
    char query[128];
    bool has_query = false;

    esp_err_t qerr = httpd_req_get_url_query_str(req, query, sizeof(query));
    if (qerr == ESP_OK) {
        has_query = true;
    } else if (qerr == ESP_ERR_NOT_FOUND) {
        has_query = false;
    } else {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Query error");
        return ESP_FAIL;
    }

    /* ---- SET VALUE ---- */
    if (has_query) {
        char val_str[32];

        // Get value
        if (httpd_query_key_value(query, "val", val_str, sizeof(val_str)) != ESP_OK) {
            httpd_resp_send_err(req,
                                HTTPD_400_BAD_REQUEST,
                                "Missing val parameter");
            return ESP_OK;
        }

        /* ---- EXPOSURE ---- */
        if (subpath_len == strlen("exposure") &&
            strncmp(subpath, "exposure", subpath_len) == 0)
        {
            if (api_set_exposure(atoi(val_str)) == API_OK) {
                httpd_resp_sendstr(req, "OK");
            } else {
                httpd_resp_send_err(req,
                                    HTTPD_500_INTERNAL_SERVER_ERROR,
                                    "Set failed");
            }

            return ESP_OK;
        }
        /* ---- GAIN ---- */
        else if (subpath_len == strlen("gain") &&
            strncmp(subpath, "gain", subpath_len) == 0)
        {
            if (api_set_gain(atoi(val_str)) == API_OK) {
                httpd_resp_sendstr(req, "OK");
            } else {
                httpd_resp_send_err(req,
                                    HTTPD_500_INTERNAL_SERVER_ERROR,
                                    "Set failed");
            }

            return ESP_OK;
        }
        /* ---- BRIGHTNESS ---- */
        else if (subpath_len == strlen("brightness") &&
            strncmp(subpath, "brightness", subpath_len) == 0)
        {
            if (api_set_brightness(atoi(val_str)) == API_OK) {
                httpd_resp_sendstr(req, "OK");
            } else {
                httpd_resp_send_err(req,
                                    HTTPD_500_INTERNAL_SERVER_ERROR,
                                    "Set failed");
            }

            return ESP_OK;
        }
        /* ---- CONTRAST ---- */
        else if (subpath_len == strlen("contrast") &&
            strncmp(subpath, "contrast", subpath_len) == 0)
        {
            if (api_set_contrast(atoi(val_str)) == API_OK) {
                httpd_resp_sendstr(req, "OK");
            } else {
                httpd_resp_send_err(req,
                                    HTTPD_500_INTERNAL_SERVER_ERROR,
                                    "Set failed");
            }

            return ESP_OK;
        }
        /* ---- UNKNOWN COMMAND ---- */
        else {
            httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "Unknown camera command");
            return ESP_OK;
        }
    }

    /* ---- GET VALUE ---- */
    else {
        int value;

        /* ---- EXPOSURE ---- */
        if (subpath_len == strlen("exposure") &&
            strncmp(subpath, "exposure", subpath_len) == 0) {

            if (!(api_get_exposure(&value) == API_OK)) {
                httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Get failed");
                return ESP_OK;
            }

        }
        /* ---- GAIN ---- */
        else if (subpath_len == strlen("gain") &&
            strncmp(subpath, "gain", subpath_len) == 0) {

            if (!(api_get_gain(&value) == API_OK)) {
                httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Get failed");
                return ESP_OK;
            }

        }
        /* ---- BRIGHTNESS ---- */
        else if (subpath_len == strlen("brightness") &&
            strncmp(subpath, "brightness", subpath_len) == 0) {

            if (!(api_get_brightness(&value) == API_OK)) {
                httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Get failed");
                return ESP_OK;
            }

        }
        /* ---- CONTRAST ---- */
        else if (subpath_len == strlen("contrast") &&
            strncmp(subpath, "contrast", subpath_len) == 0) {

            if (!(api_get_contrast(&value) == API_OK)) {
                httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Get failed");
                return ESP_OK;
            }

        }
        /* ---- UNKNOWN COMMAND ---- */
        else {
            httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "Unknown camera command");
            return ESP_OK;
        }

        char resp[32];
        snprintf(resp, sizeof(resp), "%d", value);
        httpd_resp_sendstr(req, resp);
        return ESP_OK;
    }

}

static esp_err_t roi_handler(httpd_req_t *req) {
    ESP_LOGI(TAG, "ROI request: %s", req->uri);

    const char *uri = req->uri;
    const char *prefix = "/roi/";
    size_t prefix_len = strlen(prefix);

    /* Check prefix */
    if (strncmp(uri, prefix, prefix_len) != 0) {
        httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "Invalid ROI path");
        return ESP_FAIL;
    }

    /* Find query mark */
    const char *query_mark = strchr(uri, '?');
    size_t path_len = query_mark ? (size_t)(query_mark - uri)
                                 : strlen(uri);

    /* Extract subpath */
    const char *subpath = uri + prefix_len;
    size_t subpath_len = path_len - prefix_len;

    /* Extract query (if exists) */
    char query[128];
    bool has_query = false;

    esp_err_t qerr = httpd_req_get_url_query_str(req, query, sizeof(query));
    if (qerr == ESP_OK) {
        has_query = true;
    } else if (qerr == ESP_ERR_NOT_FOUND) {
        has_query = false;
    } else {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Query error");
        return ESP_FAIL;
    }

    /* ---- SET VALUE ---- */
    if(has_query) {
        char val_str[32];

        // Get value
        if (httpd_query_key_value(query, "val", val_str, sizeof(val_str)) != ESP_OK) {
            httpd_resp_send_err(req,
                                HTTPD_400_BAD_REQUEST,
                                "Missing val parameter");
            return ESP_OK;
        }

        /* ---- ROI x0 ---- */
        if (subpath_len == strlen("x0") &&
            strncmp(subpath, "x0", subpath_len) == 0)
        {
            if (api_set_ROI_x0(atoi(val_str)) == API_OK) {
                httpd_resp_sendstr(req, "OK");
            } else {
                httpd_resp_send_err(req,
                                    HTTPD_500_INTERNAL_SERVER_ERROR,
                                    "Set failed");
            }

            return ESP_OK;
        }
        /* ---- ROI y0 ---- */
        else if (subpath_len == strlen("y0") &&
            strncmp(subpath, "y0", subpath_len) == 0)
        {
            if (api_set_ROI_y0(atoi(val_str)) == API_OK) {
                httpd_resp_sendstr(req, "OK");
            } else {
                httpd_resp_send_err(req,
                                    HTTPD_500_INTERNAL_SERVER_ERROR,
                                    "Set failed");
            }

            return ESP_OK;
        }
        /* ---- ROI dx ---- */
        else if (subpath_len == strlen("dx") &&
            strncmp(subpath, "dx", subpath_len) == 0)
        {
            if (api_set_ROI_dx(atoi(val_str)) == API_OK) {
                httpd_resp_sendstr(req, "OK");
            } else {
                httpd_resp_send_err(req,
                                    HTTPD_500_INTERNAL_SERVER_ERROR,
                                    "Set failed");
            }

            return ESP_OK;
        }
        /* ---- ROI dy ---- */
        else if (subpath_len == strlen("dy") &&
            strncmp(subpath, "dy", subpath_len) == 0)
        {
            if (api_set_ROI_dy(atoi(val_str)) == API_OK) {
                httpd_resp_sendstr(req, "OK");
            } else {
                httpd_resp_send_err(req,
                                    HTTPD_500_INTERNAL_SERVER_ERROR,
                                    "Set failed");
            }

            return ESP_OK;
        }
        /* ---- ROI enable ---- */
        else if (subpath_len == strlen("enable") &&
            strncmp(subpath, "enable", subpath_len) == 0)
        {
            if (api_set_ROI_enable(atoi(val_str)) == API_OK) {
                httpd_resp_sendstr(req, "OK");
            } else {
                httpd_resp_send_err(req,
                                    HTTPD_500_INTERNAL_SERVER_ERROR,
                                    "Set failed");
            }

            return ESP_OK;
        }
        /* ---- UNKNOWN COMMAND ---- */
        else {
            httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "Unknown ROI command");
            return ESP_OK;
        }

    }
    /* ---- GET VALUE ---- */
    else {
        int value;

        /* ---- ROI x0 ---- */
        if (subpath_len == strlen("x0") &&
            strncmp(subpath, "x0", subpath_len) == 0) {

            if (!(api_get_ROI_x0(&value) == API_OK)) {
                httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Get failed");
                return ESP_OK;
            }

        }
        /* ---- ROI y0 ---- */
        else if (subpath_len == strlen("y0") &&
            strncmp(subpath, "y0", subpath_len) == 0) {

            if (!(api_get_ROI_y0(&value) == API_OK)) {
                httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Get failed");
                return ESP_OK;
            }

        }
        /* ---- ROI dx ---- */
        else if (subpath_len == strlen("dx") &&
            strncmp(subpath, "dx", subpath_len) == 0) {

            if (!(api_get_ROI_dx(&value) == API_OK)) {
                httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Get failed");
                return ESP_OK;
            }

        }
        /* ---- ROI dy ---- */
        else if (subpath_len == strlen("dy") &&
            strncmp(subpath, "dy", subpath_len) == 0) {

            if (!(api_get_ROI_dy(&value) == API_OK)) {
                httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Get failed");
                return ESP_OK;
            }

        }
        /* ---- ROI enabled ---- */
        else if (subpath_len == strlen("enable") &&
            strncmp(subpath, "enable", subpath_len) == 0) {

            if (!(api_get_ROI_enable(&value) == API_OK)) {
                httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Get failed");
                return ESP_OK;
            }

        }
        /* ---- UNKNOWN COMMAND ---- */
        else {
            httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "Unknown ROI command");
            return ESP_OK;
        }

        char resp[32];
        snprintf(resp, sizeof(resp), "%d", value);
        httpd_resp_sendstr(req, resp);
        return ESP_OK;
    }
}

static esp_err_t target_handler(httpd_req_t *req) {
    ESP_LOGI(TAG, "Target request: %s", req->uri);

    const char *uri = req->uri;
    const char *prefix = "/target/";
    size_t prefix_len = strlen(prefix);

    /* Check prefix */
    if (strncmp(uri, prefix, prefix_len) != 0) {
        httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "Invalid target path");
        return ESP_FAIL;
    }

    /* Find query mark */
    const char *query_mark = strchr(uri, '?');
    size_t path_len = query_mark ? (size_t)(query_mark - uri)
                                 : strlen(uri);

    /* Extract subpath */
    const char *subpath = uri + prefix_len;
    size_t subpath_len = path_len - prefix_len;

    /* Extract query (if exists) */
    char query[128];
    bool has_query = false;

    esp_err_t qerr = httpd_req_get_url_query_str(req, query, sizeof(query));
    if (qerr == ESP_OK) {
        has_query = true;
    } else if (qerr == ESP_ERR_NOT_FOUND) {
        has_query = false;
    } else {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Query error");
        return ESP_FAIL;
    }

    /* ---- SET VALUE ---- */
    if (has_query) {
        char val_str[32];

        // Get value
        if (httpd_query_key_value(query, "val", val_str, sizeof(val_str)) != ESP_OK) {
            httpd_resp_send_err(req,
                                HTTPD_400_BAD_REQUEST,
                                "Missing val parameter");
            return ESP_OK;
        }

        /* ---- TARGET x0 ---- */
        if (subpath_len == strlen("x0") &&
            strncmp(subpath, "x0", subpath_len) == 0)
        {
            if (api_set_target_x0(atof(val_str)) == API_OK) {
                httpd_resp_sendstr(req, "OK");
            } else {
                httpd_resp_send_err(req,
                                    HTTPD_500_INTERNAL_SERVER_ERROR,
                                    "Set failed");
            }

            return ESP_OK;
        }
        /* ---- TARGET y0 ---- */
        else if (subpath_len == strlen("y0") &&
            strncmp(subpath, "y0", subpath_len) == 0)
        {
            if (api_set_target_y0(atof(val_str)) == API_OK) {
                httpd_resp_sendstr(req, "OK");
            } else {
                httpd_resp_send_err(req,
                                    HTTPD_500_INTERNAL_SERVER_ERROR,
                                    "Set failed");
            }

            return ESP_OK;
        }
        /* ---- TARGET TOLERANCE ---- */
        else if (subpath_len == strlen("tolerance") &&
            strncmp(subpath, "tolerance", subpath_len) == 0)
        {
            if (api_set_target_tol(atof(val_str)) == API_OK) {
                httpd_resp_sendstr(req, "OK");
            } else {
                httpd_resp_send_err(req,
                                    HTTPD_500_INTERNAL_SERVER_ERROR,
                                    "Set failed");
            }

            return ESP_OK;
        }
        /* ---- UNKNOWN COMMAND ---- */
        else {
            httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "Unknown target command");
            return ESP_OK;
        }
    }
    /* ---- GET VALUE ---- */
    else {
        float value;

        /* ---- TARGET x0 ---- */
        if (subpath_len == strlen("x0") &&
            strncmp(subpath, "x0", subpath_len) == 0) {

            if (!(api_get_target_x0(&value) == API_OK)) {
                httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Get failed");
                return ESP_OK;
            }

        }
        /* ---- TARGET y0 ---- */
        else if (subpath_len == strlen("y0") &&
            strncmp(subpath, "y0", subpath_len) == 0) {

            if (!(api_get_target_y0(&value) == API_OK)) {
                httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Get failed");
                return ESP_OK;
            }

        }
        /* ---- TARGET TOLERANCE ---- */
        else if (subpath_len == strlen("tolerance") &&
            strncmp(subpath, "tolerance", subpath_len) == 0) {

            if (!(api_get_target_tol(&value) == API_OK)) {
                httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Get failed");
                return ESP_OK;
            }

        }
        /* ---- TARGET ERROR X ---- */
        else if (subpath_len == strlen("error_x") &&
            strncmp(subpath, "error_x", subpath_len) == 0) {

            if (!(api_get_target_error_x(&value) == API_OK)) {
                httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Get failed");
                return ESP_OK;
            }

        }
        /* ---- TARGET ERROR Y ---- */
        else if (subpath_len == strlen("error_y") &&
            strncmp(subpath, "error_y", subpath_len) == 0) {

            if (!(api_get_target_error_y(&value) == API_OK)) {
                httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Get failed");
                return ESP_OK;
            }

        }

        /* ---- TARGET IS HIT ---- */
        else if (subpath_len == strlen("hit") &&
            strncmp(subpath, "hit", subpath_len) == 0) {

            uint8_t val_hit;
            if (!(api_get_target_is_hit(&val_hit) == API_OK)) {
                httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Get failed");
                return ESP_OK;
            }

        }
        /* ---- UNKNOWN COMMAND ---- */
        else {
            httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "Unknown target command");
            return ESP_OK;
        }

        char resp[32];
        snprintf(resp, sizeof(resp), "%f", value);
        httpd_resp_sendstr(req, resp);
        return ESP_OK;
    }
}

static esp_err_t centroid_handler(httpd_req_t *req) {
    ESP_LOGI(TAG, "Centroid request: %s", req->uri);

    const char *uri = req->uri;
    const char *prefix = "/centroid/";
    size_t prefix_len = strlen(prefix);

    /* Check prefix */
    if (strncmp(uri, prefix, prefix_len) != 0) {
        httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "Invalid centroid path");
        return ESP_FAIL;
    }

    /* Find query mark */
    const char *query_mark = strchr(uri, '?');
    size_t path_len = query_mark ? (size_t)(query_mark - uri)
                                 : strlen(uri);

    /* Extract subpath */
    const char *subpath = uri + prefix_len;
    size_t subpath_len = path_len - prefix_len;

    /* Extract query (if exists) */
    char query[128];
    bool has_query = false;

    esp_err_t qerr = httpd_req_get_url_query_str(req, query, sizeof(query));
    if (qerr == ESP_OK) {
        has_query = true;
    } else if (qerr == ESP_ERR_NOT_FOUND) {
        has_query = false;
    } else {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Query error");
        return ESP_FAIL;
    }

    /* ---- SET VALUE ---- */
    if (has_query) {
        char val_str[32];

        // Get value
        if (httpd_query_key_value(query, "val", val_str, sizeof(val_str)) != ESP_OK) {
            httpd_resp_send_err(req,
                                HTTPD_400_BAD_REQUEST,
                                "Missing val parameter");
            return ESP_OK;
        }

        httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "Unknown ROI command");
        return ESP_OK;
    }
    /* ---- GET VALUE ---- */
    else {
        float value;

        /* ---- CENTROID x0 ---- */
        if (subpath_len == strlen("x0") &&
            strncmp(subpath, "x0", subpath_len) == 0) {

            if (!(api_get_centroid_x0(&value) == API_OK)) {
                httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Get failed");
                return ESP_OK;
            }

        }
        /* ---- CENTROID y0 ---- */
        else if (subpath_len == strlen("y0") &&
            strncmp(subpath, "y0", subpath_len) == 0) {

            if (!(api_get_centroid_y0(&value) == API_OK)) {
                httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Get failed");
                return ESP_OK;
            }

        }
        /* ---- UNKNOWN COMMAND ---- */
        else {
            httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "Unknown centroid command");
            return ESP_OK;
        }

        char resp[32];
        snprintf(resp, sizeof(resp), "%f", value);
        httpd_resp_sendstr(req, resp);
        return ESP_OK;
    }
}

static esp_err_t preview_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG, "Preview request: %s", req->uri);
    uint8_t* jpg_buf = NULL;
    size_t jpg_len = 0;

    if(api_snapshot() != API_OK) {
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }

    if(!get_preview(&jpg_buf, &jpg_len)) {
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }

    httpd_resp_set_type(req, "image/jpeg");
    // disable cache
    httpd_resp_set_hdr(req, "Cache-Control", "no-store, no-cache, must-revalidate, max-age=0");
    httpd_resp_set_hdr(req, "Pragma", "no-cache");
    httpd_resp_set_hdr(req, "Expires", "0");
    httpd_resp_send(req, (const char *)jpg_buf, jpg_len);

    ESP_LOGI(TAG, "Preview sent!");
    free(jpg_buf);
    return ESP_OK;

}

// ========================
// HTTP SERVER
// ========================
esp_err_t http_api_start(void) {

    ESP_ERROR_CHECK(spiffs_mount());

    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.lru_purge_enable = true;
    config.max_open_sockets = 6;
    config.stack_size = 8192;
    config.uri_match_fn = httpd_uri_match_wildcard;
    config.task_priority = 2;

    ESP_LOGI(TAG, "Starting API server on port %d", config.server_port);
    if (httpd_start(&g_api_server, &config) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start API server");
        return ESP_FAIL;
    }

    httpd_uri_t uri_index = {
        .uri="/",
        .method=HTTP_GET,
        .handler=index_handler
    };
    httpd_uri_t uri_appjs = {
        .uri="/app.js",
        .method=HTTP_GET,
        .handler=appjs_handler
    };
    httpd_uri_t uri_style = {
        .uri="/style.css",
        .method=HTTP_GET,
        .handler=style_handler
    };
    httpd_uri_t uri_camera = {
        .uri = "/camera/*",
        .method = HTTP_GET,
        .handler = camera_handler
    };
    httpd_uri_t uri_preview = {
        .uri = "/preview",
        .method = HTTP_GET,
        .handler = preview_handler
    };
    httpd_uri_t uri_roi = {
        .uri = "/roi/*",
        .method = HTTP_GET,
        .handler = roi_handler
    };
    httpd_uri_t uri_target = {
        .uri = "/target/*",
        .method = HTTP_GET,
        .handler = target_handler
    };
    httpd_uri_t uri_centroid = {
        .uri = "/centroid/*",
        .method = HTTP_GET,
        .handler = centroid_handler
    };

    httpd_register_uri_handler(g_api_server, &uri_index);
    httpd_register_uri_handler(g_api_server, &uri_appjs);
    httpd_register_uri_handler(g_api_server, &uri_style);
    httpd_register_uri_handler(g_api_server, &uri_camera);
    httpd_register_uri_handler(g_api_server, &uri_preview);
    httpd_register_uri_handler(g_api_server, &uri_roi);
    httpd_register_uri_handler(g_api_server, &uri_target);
    httpd_register_uri_handler(g_api_server, &uri_centroid);

    return ESP_OK;
}