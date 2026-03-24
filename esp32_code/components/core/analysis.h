#pragma once
#include "esp_err.h"
#include <stdint.h>
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"


/* =========================
   IMAGE STRUCT
   ========================= */
typedef struct {
    uint8_t  *buf;
    size_t    len;
    uint32_t  width;
    uint32_t  height;
    bool      valid;
} image_snapshot_t;

extern image_snapshot_t g_image_snapshot;
extern SemaphoreHandle_t g_image_mutex;

esp_err_t image_snapshot_init(size_t max_size);

/* =========================
   ROI STRUCT
   ========================= */
typedef struct {
    uint16_t x0;
    uint16_t y0;
    uint16_t dx;
    uint16_t dy;
    bool     enabled;
} roi_t;

extern roi_t g_roi;

bool roi_validate_and_set(uint16_t x0, uint16_t y0,
                          uint16_t dx, uint16_t dy);


/* =========================
   CENTROID STRUCT
   ========================= */
typedef struct {
    bool     valid;
    float    x;
    float    y;
    uint32_t sum;
    uint32_t area;
} centroid_t;

extern centroid_t g_centroid;
extern centroid_t g_target;

void compute_centroid(const uint8_t *img,
                      int img_w,
                      int img_h,
                      const roi_t *roi,
                      uint8_t threshold);


/* =========================
   ERROR STRUCT
   ========================= */
typedef struct {
    float err_x;
    float err_y;
    float err2;
    float tol2;
    bool hit;
    uint8_t hit_counter;
} error_target_t;

extern error_target_t g_error;

void compute_error();