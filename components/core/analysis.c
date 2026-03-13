#include "analysis.h"

#include <stdlib.h>
#include <string.h>

#include "esp_log.h"
#include "esp_camera.h"

#include "app_config.h"

static const char *TAG = "analysis";


/* =========================
   IMAGE STRUCT
   ========================= */
image_snapshot_t g_image_snapshot = {0};
SemaphoreHandle_t g_image_mutex = NULL;

esp_err_t image_snapshot_init(size_t max_size)
{
    g_image_snapshot.buf = malloc(max_size);
    if (!g_image_snapshot.buf) {
        ESP_LOGE(TAG, "Failed to allocate snapshot buffer");
        abort();
    }

    g_image_snapshot.len = 0;
    g_image_snapshot.valid = false;

    g_image_mutex = xSemaphoreCreateMutex();
    if (!g_image_mutex) {
        ESP_LOGE(TAG, "Failed to create snapshot mutex");
        abort();
    }

    ESP_LOGI(TAG, "Snapshot initialized (%d bytes)", max_size);

    return ESP_OK;
}

/* =========================
   ROI STRUCT
   ========================= */
roi_t g_roi = {0};

bool roi_validate_and_set(uint16_t x0, uint16_t y0,
                          uint16_t dx, uint16_t dy) {
    uint16_t img_width = 0;
    uint16_t img_height = 0;
    if (CAM_FRAMESIZE == FRAMESIZE_QVGA) {
        img_width = 320;
        img_height = 240;
    }
    else if (CAM_FRAMESIZE == FRAMESIZE_QQVGA) {
        img_width = 160;
        img_height = 120;
    }
    else return false;

    // dx, dy muszą być > 0
    if (dx == 0 || dy == 0)
        return false;

    // początek musi być w obrazie
    if (x0 >= img_width || y0 >= img_height)
        return false;

    // ROI nie może wyjść poza obraz
    if ((x0 + dx) > img_width)
        return false;

    if ((y0 + dy) > img_height)
        return false;

    g_roi.x0 = x0;
    g_roi.y0 = y0;
    g_roi.dx = dx;
    g_roi.dy = dy;
    // g_roi.enabled = true;

    g_config.roi_x0 = x0;
    g_config.roi_y0 = y0;
    g_config.roi_dx = dx;
    g_config.roi_dy = dy;
    config_save(&g_config);

    return true;
}

/* =========================
   CENTROID STRUCT
   ========================= */
centroid_t g_centroid = {0};
centroid_t g_target = {0};

void compute_centroid(const uint8_t *img,
                      int img_w,
                      int img_h,
                      const roi_t *roi,
                      uint8_t threshold)
{
    uint64_t sum_i  = 0;
    uint64_t sum_xi = 0;
    uint64_t sum_yi = 0;
    uint32_t area_i = 0;

    int x0 = roi->enabled ? roi->x0 : 0;
    int y0 = roi->enabled ? roi->y0 : 0;
    int x1 = roi->enabled ? (roi->x0 + roi->dx) : img_w;
    int y1 = roi->enabled ? (roi->y0 + roi->dy) : img_h;

    if (x1 > img_w) x1 = img_w;
    if (y1 > img_h) y1 = img_h;

    for (int y = y0; y < y1; y++) {
        int row = y * img_w;
        for (int x = x0; x < x1; x++) {
            uint8_t v = img[row + x];
            if (v > threshold) {
                sum_i  += v;
                sum_xi += (uint64_t)x * v;
                sum_yi += (uint64_t)y * v;
                area_i++;
            }
        }
    }

    if (sum_i == 0) {
        g_centroid.valid = false;
        return;
    }

    g_centroid.x = (float)sum_xi / (float)sum_i;
    g_centroid.y = (float)sum_yi / (float)sum_i;
    g_centroid.sum = sum_i;
    g_centroid.area = area_i;
    g_centroid.valid = true;
}

/* =========================
   ERROR STRUCT
   ========================= */
error_target_t g_error = {0};

void compute_error() {
    if(!g_centroid.valid) {
        g_error.hit = false;
        g_error.hit_counter = 0;
        return;
    }

    g_error.err_x = g_target.x - g_centroid.x;
    g_error.err_y = g_target.y - g_centroid.y;
    g_error.err2 = g_error.err_x*g_error.err_x + g_error.err_y*g_error.err_y;

    if(g_error.err2 <= g_error.tol2) g_error.hit_counter++;
    else {
        g_error.hit_counter = 0;
        g_error.hit = false;
    }

    if(g_error.hit_counter > TARGET_MAX_HIT_COUNT) {
        g_error.hit_counter = TARGET_MAX_HIT_COUNT;
        g_error.hit = true;
    }
}