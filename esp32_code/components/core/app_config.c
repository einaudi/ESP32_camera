#include "app_config.h"
#include "nvs.h"
#include "esp_log.h"

#include "analysis.h"
#include "camera_worker.h"

static const char *TAG = "config";


config_t g_config = {0};
// Factory defaults
const config_t f_config = {
    .cam_exposure = 10,
    .cam_gain = 0,
    .cam_brightness = 0,
    .cam_contrast = 0,

    .target_x = 25,
    .target_y = 25,
    .target_tolerance = 5.,
    .brightness_threshold = 10,
    .ROI_bound = 0,

    .roi_x0 = 0,
    .roi_y0 = 0,
    .roi_dx = 50,
    .roi_dy = 50,
    .roi_enabled = 0,

    .version = 2
};

bool config_load(config_t *cfg) {
    ESP_LOGI(TAG, "Loading config...");
    nvs_handle_t handle;

    if (nvs_open("config", NVS_READONLY, &handle) != ESP_OK) {
        ESP_LOGI(TAG, "Could not load config from memory! Setting factory defaults");
        g_config = f_config;
        return false;
    }

    size_t size = sizeof(config_t);
    esp_err_t err = nvs_get_blob(handle, "blob", cfg, &size);

    nvs_close(handle);

    if(g_config.version < f_config.version) {
        ESP_LOGI(TAG, "Old config version!! Setting factory defaults");
        g_config = f_config;
    }

    ESP_LOGI(TAG, "Config loaded!");
    return (err == ESP_OK);
}

bool config_save(const config_t *cfg) {
    // ESP_LOGI(TAG, "Saving config...");
    nvs_handle_t handle;

    if (nvs_open("config", NVS_READWRITE, &handle) != ESP_OK)
        return false;

    esp_err_t err = nvs_set_blob(handle, "blob",
                                 cfg, sizeof(config_t));

    if (err == ESP_OK)
        err = nvs_commit(handle);

    nvs_close(handle);

    // ESP_LOGI(TAG, "Config saved!");
    return (err == ESP_OK);
}

void config_apply() {
    ESP_LOGI(TAG, "Applying config...");
    // Camera
    queue_cam_cmd(CAM_CMD_SET_EXPOSURE, g_config.cam_exposure);
    queue_cam_cmd(CAM_CMD_SET_GAIN, g_config.cam_gain);
    queue_cam_cmd(CAM_CMD_SET_BRIGHTNESS, g_config.cam_brightness);
    queue_cam_cmd(CAM_CMD_SET_CONTRAST, g_config.cam_contrast);

    // ROI
    g_roi.x0 = g_config.roi_x0;
    g_roi.y0 = g_config.roi_y0;
    g_roi.dx = g_config.roi_dx;
    g_roi.dy = g_config.roi_dy;
    g_roi.enabled = g_config.roi_enabled ? true : false;

    // Target centroid
    g_target.x = g_config.target_x;
    g_target.y = g_config.target_y;
    g_target.valid = true;
    g_target.sum = 0;

    // Error
    g_error.tol2 = g_config.target_tolerance*g_config.target_tolerance;

    ESP_LOGI(TAG, "Config applied!");
}