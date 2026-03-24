#include "api.h"

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

#include "esp_log.h"

#include "camera_worker.h"
#include "analysis.h"
#include "app_config.h"
#include "utils.h"
#include "uart_api.h"
#include "http_api.h"


static const char *TAG = "api";

static SemaphoreHandle_t api_mutex;
#define API_LOCK() if (xSemaphoreTake(api_mutex, pdMS_TO_TICKS(100)) != pdTRUE) return API_ERR_BUSY;
#define API_UNLOCK() xSemaphoreGive(api_mutex);


void api_init(void) {
    api_mutex = xSemaphoreCreateMutex();

    #if ENABLE_HTTP_API
        http_api_init();
    #endif

    #if ENABLE_UART_API
        uart_api_init();
    #endif

    #if ENABLE_CONSOLE_API
        console_api_init();
    #endif
}

// General
api_status_t api_idn(char* buf, size_t* len) {
    API_LOCK();

    *len = sprintf(buf, "%s", DEV_NAME);

    API_UNLOCK();
    return API_OK;
}

/* =========================
   API camera
   ========================= */
api_status_t api_set_exposure(int val) {
    ESP_LOGI(TAG, "Camera exposure: %d", val);

    API_LOCK();

    if (!queue_cam_cmd(CAM_CMD_SET_EXPOSURE, val)) {
        API_UNLOCK();
        return API_ERR_INTERNAL;
    }

    API_UNLOCK();
    return API_OK;
}
api_status_t api_set_gain(int val) {
    ESP_LOGI(TAG, "Camera gain: %d", val);
    
    API_LOCK();

    if (!queue_cam_cmd(CAM_CMD_SET_GAIN, val)) {
        API_UNLOCK();
        return API_ERR_INTERNAL;
    }

    API_UNLOCK();
    return API_OK;
}
api_status_t api_set_brightness(int val) {
    ESP_LOGI(TAG, "Camera brightness: %d", val);
    
    API_LOCK();

    if (!queue_cam_cmd(CAM_CMD_SET_BRIGHTNESS, val)) {
        API_UNLOCK();
        return API_ERR_INTERNAL;
    }

    API_UNLOCK();
    return API_OK;
}
api_status_t api_set_contrast(int val) {
    ESP_LOGI(TAG, "Camera contrast: %d", val);
    
    API_LOCK();

    if (!queue_cam_cmd(CAM_CMD_SET_CONTRAST, val)) {
        API_UNLOCK();
        return API_ERR_INTERNAL;
    }

    API_UNLOCK();
    return API_OK;
}

api_status_t api_get_exposure(int* val) {
    API_LOCK();
    *val = g_config.cam_exposure;
    API_UNLOCK();
    return API_OK; 
}
api_status_t api_get_gain(int* val) { 
    API_LOCK();
    *val = g_config.cam_gain;
    API_UNLOCK();
    return API_OK; 
}
api_status_t api_get_brightness(int* val) {
    API_LOCK();
    *val = g_config.cam_brightness;
    API_UNLOCK();
    return API_OK; 
}
api_status_t api_get_contrast(int* val) {
    API_LOCK();
    *val = g_config.cam_contrast;
    API_UNLOCK();
    return API_OK; 
}

/* =========================
   API ROI
   ========================= */
api_status_t api_set_ROI_x0(uint16_t val) {
    ESP_LOGI(TAG, "ROI x0: %d", val);
    API_LOCK();

    if(!roi_validate_and_set(val, g_roi.y0, g_roi.dx, g_roi.dy)) {
        API_UNLOCK();
        return API_ERR_INVALID_ARG;
    }
    else {
        API_UNLOCK();
        return API_OK;
    }
}
api_status_t api_set_ROI_y0(uint16_t val) {
    ESP_LOGI(TAG, "ROI y0: %d", val);
    API_LOCK();

    if(!roi_validate_and_set(g_roi.x0, val, g_roi.dx, g_roi.dy)) {
        API_UNLOCK();
        return API_ERR_INVALID_ARG;
    }
    else {
        API_UNLOCK();
        return API_OK;
    }
}
api_status_t api_set_ROI_dx(uint16_t val) {
    ESP_LOGI(TAG, "ROI dx: %d", val);
    API_LOCK();

    if(!roi_validate_and_set(g_roi.x0, g_roi.y0, val, g_roi.dy)) {
        API_UNLOCK();
        return API_ERR_INVALID_ARG;
    }
    else {
        API_UNLOCK();
        return API_OK;
    }
}
api_status_t api_set_ROI_dy(uint16_t val) {
    ESP_LOGI(TAG, "ROI dy: %d", val);
    API_LOCK();

    if(!roi_validate_and_set(g_roi.x0, g_roi.y0, g_roi.dx, val)) {
        API_UNLOCK();
        return API_ERR_INVALID_ARG;
    }
    else {
        API_UNLOCK();
        return API_OK;
    }
}
api_status_t api_set_ROI_enable(uint8_t val) {
    ESP_LOGI(TAG, "ROI enable: %d", val);
    API_LOCK();

    g_roi.enabled = val ? 1 : 0;
    
    API_UNLOCK();
    return API_OK;
}

api_status_t api_get_ROI_x0(int* val) {
    API_LOCK();
    *val = (int) g_roi.x0;
    API_UNLOCK();
    return API_OK;
}
api_status_t api_get_ROI_y0(int* val) {
    API_LOCK();
    *val = (int) g_roi.y0;
    API_UNLOCK();
    return API_OK;
}
api_status_t api_get_ROI_dx(int* val) {
    API_LOCK();
    *val = (int) g_roi.dx;
    API_UNLOCK();
    return API_OK;
}
api_status_t api_get_ROI_dy(int* val) {
    API_LOCK();
    *val = (int) g_roi.dy;
    API_UNLOCK();
    return API_OK;
}
api_status_t api_get_ROI_enable(int* val) {
    API_LOCK();
    *val = (int) g_roi.enabled;
    API_UNLOCK();
    return API_OK;
}

/* =========================
   API target
   ========================= */
api_status_t api_set_target_x0(float val) {
    ESP_LOGI(TAG, "Target x0: %f", val);
    API_LOCK();

    g_target.x = val;
    g_config.target_x = val;
    config_save(&g_config);

    API_UNLOCK();
    return API_OK;
}
api_status_t api_set_target_y0(float val) {
    ESP_LOGI(TAG, "Target y0: %f", val);
    API_LOCK();

    g_target.y = val;
    g_config.target_y = val;
    config_save(&g_config);

    API_UNLOCK();
    return API_OK;
}
api_status_t api_set_target_tol(float val) {
    ESP_LOGI(TAG, "Target tolerance: %f", val);
    API_LOCK();

    g_config.target_tolerance = val;
    config_save(&g_config);

    g_error.tol2 = g_config.target_tolerance*g_config.target_tolerance;

    API_UNLOCK();
    return API_OK;
}
api_status_t api_set_target_threshold(float val) {
    ESP_LOGI(TAG, "Target brightness threshold: %f", val);
    API_LOCK();

    g_config.brightness_threshold = (uint8_t) val;
    config_save(&g_config);

    API_UNLOCK();
    return API_OK;
}

api_status_t api_get_target_x0(float* val) {
    API_LOCK();
    *val = g_target.x;

    API_UNLOCK();
    return API_OK;
}
api_status_t api_get_target_y0(float* val) {
    API_LOCK();
    *val = g_target.y;

    API_UNLOCK();
    return API_OK;
}
api_status_t api_get_target_tol(float* val) {
    API_LOCK();
    *val = g_config.target_tolerance;

    API_UNLOCK();
    return API_OK;
}
api_status_t api_get_target_threshold(float* val) {
    API_LOCK();
    *val = (float) g_config.brightness_threshold;

    API_UNLOCK();
    return API_OK;
}
// Target error
api_status_t api_get_target_error_x(float* val) {
    API_LOCK();
    *val = g_error.err_x;

    API_UNLOCK();
    return API_OK;
}
api_status_t api_get_target_error_y(float* val) {
    API_LOCK();
    *val = g_error.err_y;

    API_UNLOCK();
    return API_OK;
}
api_status_t api_get_target_is_hit(uint8_t* val) {
    API_LOCK();
    *val = g_error.hit ? 1 : 0;

    API_UNLOCK();
    return API_OK;
}

/* =========================
   API centroid
   ========================= */
api_status_t api_get_centroid_x0(float* val) {
    API_LOCK();
    if(g_centroid.valid) *val = g_centroid.x;
    else *val = -1;

    API_UNLOCK();
    return API_OK;
}
api_status_t api_get_centroid_y0(float* val) {
    API_LOCK();
    if(g_centroid.valid) *val = g_centroid.y;
    else *val = -1;

    API_UNLOCK();
    return API_OK;
}

/* =========================
   Image
   ========================= */
api_status_t api_snapshot() {
    ESP_LOGI(TAG, "Snapshot requested!");
    API_LOCK()
    if(camera_capture() != ESP_OK) {
        ESP_LOGI(TAG, "Snapshot failed!");
        API_UNLOCK();
        return API_ERR_INTERNAL;
    }
    else {
        API_UNLOCK();
        return API_OK;
    }
}
