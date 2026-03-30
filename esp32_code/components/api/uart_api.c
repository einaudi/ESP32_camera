#include "uart_api.h"

#include "esp_err.h"
#include "esp_log.h"

#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>

#include "driver/uart.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "api.h"
#include "utils.h"


static const char *TAG = "uart_api";

// Commands handling
// General
static esp_err_t general_idn(char *out, size_t maxlen) {

    char buf[32];
    size_t len;

    if(api_idn(buf, &len) != API_OK) return ESP_FAIL;

    snprintf(out, maxlen, ":%s", buf);
    return ESP_OK;
}


// Camera
static esp_err_t camera_set_exposure(const char *arg) {
    int value = atoi(arg);
    if(api_set_exposure(value) == API_OK) return ESP_OK;
    else return ESP_FAIL;
}
static esp_err_t camera_set_gain(const char *arg) {
    int value = atoi(arg);
    if(api_set_gain(value) == API_OK) return ESP_OK;
    else return ESP_FAIL;
}
static esp_err_t camera_set_brightness(const char *arg) {
    int value = atoi(arg);
    if(api_set_brightness(value) == API_OK) return ESP_OK;
    else return ESP_FAIL;
}
static esp_err_t camera_set_contrast(const char *arg) {
    int value = atoi(arg);
    if(api_set_contrast(value) == API_OK) return ESP_OK;
    else return ESP_FAIL;
}

static esp_err_t camera_get_exposure(char *out, size_t maxlen) {
    int value;
    if(api_get_exposure(&value) != API_OK) return ESP_FAIL;
    snprintf(out, maxlen, ":%d", value);
    return ESP_OK;
}
static esp_err_t camera_get_gain(char *out, size_t maxlen) {
    int value;
    if(api_get_gain(&value) != API_OK) return ESP_FAIL;
    snprintf(out, maxlen, ":%d", value);
    return ESP_OK;
}
static esp_err_t camera_get_brightness(char *out, size_t maxlen) {
    int value;
    if(api_get_brightness(&value) != API_OK) return ESP_FAIL;
    snprintf(out, maxlen, ":%d", value);
    return ESP_OK;
}
static esp_err_t camera_get_contrast(char *out, size_t maxlen) {
    int value;
    if(api_get_contrast(&value) != API_OK) return ESP_FAIL;
    snprintf(out, maxlen, ":%d", value);
    return ESP_OK;
}

// ROI
static esp_err_t ROI_set_x0(const char *arg) {
    uint16_t value = (uint16_t) atoi(arg);
    if(api_set_ROI_x0(value) == API_OK) return ESP_OK;
    else return ESP_FAIL;
}
static esp_err_t ROI_set_y0(const char *arg) {
    uint16_t value = (uint16_t) atoi(arg);
    if(api_set_ROI_y0(value) == API_OK) return ESP_OK;
    else return ESP_FAIL;
}
static esp_err_t ROI_set_dx(const char *arg) {
    uint16_t value = (uint16_t) atoi(arg);
    if(api_set_ROI_dx(value) == API_OK) return ESP_OK;
    else return ESP_FAIL;
}
static esp_err_t ROI_set_dy(const char *arg) {
    uint16_t value = (uint16_t) atoi(arg);
    if(api_set_ROI_dy(value) == API_OK) return ESP_OK;
    else return ESP_FAIL;
}
static esp_err_t ROI_set_enable(const char *arg) {
    uint8_t value = (uint8_t) atoi(arg);
    if(api_set_ROI_enable(value) == API_OK) return ESP_OK;
    else return ESP_FAIL;
}

static esp_err_t ROI_get_x0(char *out, size_t maxlen) {
    int value;
    if(api_get_ROI_x0(&value) != API_OK) return ESP_FAIL;
    snprintf(out, maxlen, ":%d", value);
    return ESP_OK;
}
static esp_err_t ROI_get_y0(char *out, size_t maxlen) {
    int value;
    if(api_get_ROI_y0(&value) != API_OK) return ESP_FAIL;
    snprintf(out, maxlen, ":%d", value);
    return ESP_OK;
}
static esp_err_t ROI_get_dx(char *out, size_t maxlen) {
    int value;
    if(api_get_ROI_dx(&value) != API_OK) return ESP_FAIL;
    snprintf(out, maxlen, ":%d", value);
    return ESP_OK;
}
static esp_err_t ROI_get_dy(char *out, size_t maxlen) {
    int value;
    if(api_get_ROI_dy(&value) != API_OK) return ESP_FAIL;
    snprintf(out, maxlen, ":%d", value);
    return ESP_OK;
}
static esp_err_t ROI_get_enable(char *out, size_t maxlen) {
    int value;
    if(api_get_ROI_enable(&value) != API_OK) return ESP_FAIL;
    snprintf(out, maxlen, ":%d", value);
    return ESP_OK;
}

// Target
static esp_err_t target_set_x0(const char *arg) {
    float value =  atof(arg);
    if(api_set_target_x0(value) == API_OK) return ESP_OK;
    else return ESP_FAIL;
}
static esp_err_t target_set_y0(const char *arg) {
    float value =  atof(arg);
    if(api_set_target_y0(value) == API_OK) return ESP_OK;
    else return ESP_FAIL;
}
static esp_err_t target_set_x0_relative(const char *arg) {
    float value =  atof(arg);
    if(api_set_target_x0_relative(value) == API_OK) return ESP_OK;
    else return ESP_FAIL;
}
static esp_err_t target_set_y0_relative(const char *arg) {
    float value =  atof(arg);
    if(api_set_target_y0_relative(value) == API_OK) return ESP_OK;
    else return ESP_FAIL;
}
static esp_err_t target_set_tol(const char *arg) {
    float value =  atof(arg);
    if(api_set_target_tol(value) == API_OK) return ESP_OK;
    else return ESP_FAIL;
}
static esp_err_t target_set_threshold(const char *arg) {
    float value =  atof(arg);
    if(api_set_target_threshold(value) == API_OK) return ESP_OK;
    else return ESP_FAIL;
}
static esp_err_t target_set_ROI_bound(const char *arg) {
    uint8_t value = (uint8_t) atoi(arg);
    if(api_set_target_ROI_bound(value) == API_OK) return ESP_OK;
    else return ESP_FAIL;
}

static esp_err_t target_get_x0(char *out, size_t maxlen) {
    float value;
    if(api_get_target_x0(&value) != API_OK) return ESP_FAIL;
    snprintf(out, maxlen, ":%f", value);
    return ESP_OK;
}
static esp_err_t target_get_y0(char *out, size_t maxlen) {
    float value;
    if(api_get_target_y0(&value) != API_OK) return ESP_FAIL;
    snprintf(out, maxlen, ":%f", value);
    return ESP_OK;
}
static esp_err_t target_get_x0_relative(char *out, size_t maxlen) {
    float value;
    if(api_get_target_x0_relative(&value) != API_OK) return ESP_FAIL;
    snprintf(out, maxlen, ":%f", value);
    return ESP_OK;
}
static esp_err_t target_get_y0_relative(char *out, size_t maxlen) {
    float value;
    if(api_get_target_y0_relative(&value) != API_OK) return ESP_FAIL;
    snprintf(out, maxlen, ":%f", value);
    return ESP_OK;
}
static esp_err_t target_get_tol(char *out, size_t maxlen) {
    float value;
    if(api_get_target_tol(&value) != API_OK) return ESP_FAIL;
    snprintf(out, maxlen, ":%f", value);
    return ESP_OK;
}
static esp_err_t target_get_threshold(char *out, size_t maxlen) {
    float value;
    if(api_get_target_threshold(&value) != API_OK) return ESP_FAIL;
    snprintf(out, maxlen, ":%f", value);
    return ESP_OK;
}
static esp_err_t target_get_ROI_bound(char *out, size_t maxlen) {
    int value;
    if(api_get_target_ROI_bound(&value) != API_OK) return ESP_FAIL;
    snprintf(out, maxlen, ":%d", value);
    return ESP_OK;
}
static esp_err_t target_get_error_x(char *out, size_t maxlen) {
    float value;
    if(api_get_target_error_x(&value) != API_OK) return ESP_FAIL;
    snprintf(out, maxlen, ":%f", value);
    return ESP_OK;
}
static esp_err_t target_get_error_y(char *out, size_t maxlen) {
    float value;
    if(api_get_target_error_y(&value) != API_OK) return ESP_FAIL;
    snprintf(out, maxlen, ":%f", value);
    return ESP_OK;
}
static esp_err_t target_get_is_hit(char *out, size_t maxlen) {
    uint8_t value;
    if(api_get_target_is_hit(&value) != API_OK) return ESP_FAIL;
    snprintf(out, maxlen, ":%d", value);
    return ESP_OK;
}

// Centroid
static esp_err_t centroid_get_x0(char *out, size_t maxlen) {
    float value;
    if(api_get_centroid_x0(&value) != API_OK) return ESP_FAIL;
    snprintf(out, maxlen, ":%f", value);
    return ESP_OK;
}
static esp_err_t centroid_get_y0(char *out, size_t maxlen) {
    float value;
    if(api_get_centroid_y0(&value) != API_OK) return ESP_FAIL;
    snprintf(out, maxlen, ":%f", value);
    return ESP_OK;
}
static esp_err_t centroid_get_x0_relative(char *out, size_t maxlen) {
    float value;
    if(api_get_centroid_x0_relative(&value) != API_OK) return ESP_FAIL;
    snprintf(out, maxlen, ":%f", value);
    return ESP_OK;
}
static esp_err_t centroid_get_y0_relative(char *out, size_t maxlen) {
    float value;
    if(api_get_centroid_y0_relative(&value) != API_OK) return ESP_FAIL;
    snprintf(out, maxlen, ":%f", value);
    return ESP_OK;
}

// Image
static esp_err_t image_set_capture(const char *arg) {
    if(api_snapshot() == API_OK) return ESP_OK;
    else return ESP_FAIL;
}

static void send_image_chunked(uint8_t *data, uint32_t len) {
    char header[32];
    int header_len = snprintf(header, sizeof(header), ":IMG %u\n", (unsigned)len);

    write(STDOUT_FILENO, header, header_len);

    size_t sent = 0;

    while (sent < len) {

        size_t chunk = len - sent;
        if (chunk > CHUNK_SIZE)
            chunk = CHUNK_SIZE;

        size_t n = write(STDOUT_FILENO,
                          data + sent,
                          chunk);

        // if (n <= 0) {
        //     ESP_LOGE(TAG, "Write error");
        //     return;
        // }

        sent += n;

        // vTaskDelay(pdMS_TO_TICKS(10));
    }

    write(STDOUT_FILENO, "END OF IMAGE\n", 13);
}

static void send_image_chunked_sync(uint8_t *data, uint32_t len) {
    // Header
    char header[32];
    int header_len = snprintf(header, sizeof(header), ":IMG %u CHUNK_SIZE %u\n", (unsigned)len, (unsigned)CHUNK_SIZE);
    write(STDOUT_FILENO, header, header_len);

    // Data chunks
    size_t sent = 0;
    uint16_t chunk_id = 0;
    while (sent < len) {

        size_t chunk = len - sent;
        if (chunk > CHUNK_SIZE)
            chunk = CHUNK_SIZE;

        // Chunk header
        header_len = snprintf(header, sizeof(header), ":CHUNK_ID %u\n", (unsigned)chunk_id);
        write(STDOUT_FILENO, header, header_len);
        // Chunk 
        size_t n = write(STDOUT_FILENO,
                          data + sent,
                          chunk);

        if (n <= 0) {
            ESP_LOGE(TAG, "Write error");
            return;
        }

        sent += n;
        chunk_id++;
    }

    // End
    header_len = snprintf(header, sizeof(header), ":END CHUNKS_TOTAL %u\n", (unsigned)chunk_id);
    write(STDOUT_FILENO, header, header_len);

}

static esp_err_t image_get_image(char *out, size_t maxlen) {
    (void)out;
    (void)maxlen;

    uint8_t *img_buf = NULL;
    size_t img_len = 0;

    if (!get_image_large(&img_buf, &img_len)) {
        write(STDOUT_FILENO, ":ERR\n", 5);
        return ESP_FAIL;
    }

    esp_log_level_set("*", ESP_LOG_NONE);

    send_image_chunked(img_buf, img_len);

    esp_log_level_set("*", ESP_LOG_INFO);

    free(img_buf);

    ESP_LOGI(TAG, "Image sent (%u bytes)", (unsigned)img_len);

    return ESP_OK;
}

static esp_err_t image_get_preview(char *out, size_t maxlen) {
    (void)out;
    (void)maxlen;

    uint8_t *rgb_buf = NULL;
    size_t rgb_len = 0;

    if (!get_preview_large(&rgb_buf, &rgb_len)) {
        write(STDOUT_FILENO, ":ERR\n", 5);
        return ESP_FAIL;
    }

    esp_log_level_set("*", ESP_LOG_NONE);

    send_image_chunked(rgb_buf, rgb_len);

    esp_log_level_set("*", ESP_LOG_INFO);

    free(rgb_buf);

    ESP_LOGI(TAG, "Preview sent (%u bytes)", (unsigned)rgb_len);

    return ESP_OK;
}


// Commands table
static const command_entry_t command_table[] =
{
    {"*IDN", NULL, general_idn},
    {":camera:exposure", camera_set_exposure, camera_get_exposure},
    {":camera:gain", camera_set_gain, camera_get_gain},
    {":camera:brightness", camera_set_brightness, camera_get_brightness},
    {":camera:contrast", camera_set_contrast, camera_get_contrast},
    {":roi:x0", ROI_set_x0, ROI_get_x0},
    {":roi:y0", ROI_set_y0, ROI_get_y0},
    {":roi:dx", ROI_set_dx, ROI_get_dx},
    {":roi:dy", ROI_set_dy, ROI_get_dy},
    {":roi:enable", ROI_set_enable, ROI_get_enable},
    {":target:x0", target_set_x0, target_get_x0},
    {":target:y0", target_set_y0, target_get_y0},
    {":target:relative:x0", target_set_x0_relative, target_get_x0_relative},
    {":target:relative:y0", target_set_y0_relative, target_get_y0_relative},
    {":target:tolerance", target_set_tol, target_get_tol},
    {":target:threshold", target_set_threshold, target_get_threshold},
    {":target:error:x", NULL, target_get_error_x},
    {":target:error:y", NULL, target_get_error_y},
    {":target:hit", NULL, target_get_is_hit},
    {":target:bindroi", target_set_ROI_bound, target_get_ROI_bound},
    {":centroid:x0", NULL, centroid_get_x0},
    {":centroid:relative:x0", NULL, centroid_get_x0_relative},
    {":centroid:y0", NULL, centroid_get_y0},
    {":centroid:relative:y0", NULL, centroid_get_y0_relative},
    {":image:capture", image_set_capture, NULL},
    {":image:image", NULL, image_get_image},
    {":image:preview", NULL, image_get_preview},
};

#define COMMAND_COUNT (sizeof(command_table)/sizeof(command_table[0]))

// Command parsing
static void scpi_parse(char *line, char **path, char **arg, bool *is_query) {
    *path = line;
    *arg = NULL;
    *is_query = false;

    char *p = line;

    while (*p)
    {
        if (*p == '?')
        {
            *is_query = true;
            *p = 0;
            return;
        }

        if (*p == ' ')
        {
            *p = 0;
            *arg = p + 1;
            return;
        }

        p++;
    }
}

static const command_entry_t* scpi_find(const char *path) {
    for (int i = 0; i < COMMAND_COUNT; i++)
    {
        if (strcmp(path, command_table[i].path) == 0)
            return &command_table[i];
    }

    return NULL;
}

static void scpi_process_line(char *line, char *resp, size_t maxlen) {
    char *path;
    char *arg;
    bool query;

    scpi_parse(line, &path, &arg, &query);

    const command_entry_t *cmd = scpi_find(path);

    if (!cmd)
    {
        snprintf(resp, maxlen, ":ERR:UNKNOWN");
        return;
    }

    if (query)
    {
        if (!cmd->get)
        {
            snprintf(resp, maxlen, ":ERR:NOTQUERY");
            return;
        }

        cmd->get(resp, maxlen);
    }
    else
    {
        if (!cmd->set)
        {
            snprintf(resp, maxlen, ":ERR:NOTSET");
            return;
        }

        if (cmd->set(arg) == ESP_OK)
            snprintf(resp, maxlen, ":OK");
        else
            snprintf(resp, maxlen, ":ERR");
    }
}

// UART worker
static void uart_worker(void *arg) {
    uint8_t rx;

    char cmd_buf[CMD_BUF_SIZE];
    char resp[RESP_BUF_SIZE];

    int pos = 0;

    while (1)
    {
        int len = uart_read_bytes(UART_PORT, &rx, 1, portMAX_DELAY);

        if (len > 0)
        {   
            if (rx == '\n' || rx == '\r')
            {
                if (pos > 0)
                {
                    cmd_buf[pos] = 0;
                    ESP_LOGI(TAG, "Received command: %s\r\n", cmd_buf);

                    scpi_process_line(cmd_buf, resp, sizeof(resp));

                    uart_write_bytes(UART_PORT, resp, strlen(resp));
                    uart_write_bytes(UART_PORT, "\r\n", 2);

                    pos = 0;
                }
            }
            else
            {
                if (pos < CMD_BUF_SIZE - 1)
                    cmd_buf[pos++] = rx;
                else
                    pos = 0;
            }
        }
    }
}

static void console_worker(void *arg) {
    char cmd_buf[256];
    char resp[128];

    int pos = 0;

    while (1)
    {
        int c = getchar();

        if (c < 0)
        {
            vTaskDelay(pdMS_TO_TICKS(10));
            continue;
        }

        char ch = (char)c;

        if (ch == '\n' || ch == '\r')
        {
            if (pos > 0)
            {
                cmd_buf[pos] = 0;

                ESP_LOGI(TAG, "Received command: %s", cmd_buf);

                scpi_process_line(cmd_buf, resp, sizeof(resp));

                printf("%s\r\n", resp);
                fflush(stdout);

                pos = 0;
            }
        }
        else
        {
            if (pos < sizeof(cmd_buf) - 1)
                cmd_buf[pos++] = ch;
        }
    }
}

// Init
void uart_api_init(void) {
    const uart_config_t uart_config = {
        .baud_rate = UART_BAUD,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
    };

    uart_driver_install(UART_PORT, RX_BUF_SIZE, 0, 0, NULL, 0);
    uart_param_config(UART_PORT, &uart_config);

    xTaskCreate(
        uart_worker,
        "uart_transport",
        4096,
        NULL,
        5,
        NULL
    );

    ESP_LOGI(TAG, "UART API ready!");
}

void console_api_init(void) {

    xTaskCreate(
        console_worker,
        "console_transport",
        4096,
        NULL,
        3,
        NULL
    );

    ESP_LOGI(TAG, "Console API ready!");
}
