#include "utils.h"

#include <stdlib.h>

#include "esp_log.h"

#include "analysis.h"
#include "esp_camera.h"   // fmt2jpg


#define SET_PIXEL_BGR(buf, idx, b, g, r) \
    do { buf[idx+0] = (b); buf[idx+1] = (g); buf[idx+2] = (r); } while(0)


static const char *TAG = "utils";


void grayscale_to_buff(uint8_t *gray, uint8_t *img, int width, int height) {
    for (int i = 0; i < width * height; i++) {
        uint8_t v = gray[i];
        img[i + 0] = v;
    }
}

void grayscale_to_rgb888(uint8_t *gray, uint8_t *rgb, int width, int height) {
    for (int i = 0; i < width * height; i++) {
        uint8_t v = gray[i];
        rgb[3*i + 0] = v; // R
        rgb[3*i + 1] = v; // G
        rgb[3*i + 2] = v; // B
    }
}

void draw_roi_rect_rgb(uint8_t *rgb, int img_w, int img_h, const roi_t *roi) {
    int x0 = roi->x0;
    int y0 = roi->y0;
    int x1 = x0 + roi->dx - 1;
    int y1 = y0 + roi->dy - 1;

    // clamp (na wszelki wypadek)
    if (x1 >= img_w) x1 = img_w - 1;
    if (y1 >= img_h) y1 = img_h - 1;

    // górna i dolna krawędź
    for (int x = x0; x <= x1; x++) {
        int top = (y0 * img_w + x) * 3;
        int bot = (y1 * img_w + x) * 3;

        if (roi->enabled) {
            SET_PIXEL_BGR(rgb, top, 0, 0, 255);
            SET_PIXEL_BGR(rgb, bot, 0, 0, 255);
        }
        else {
            SET_PIXEL_BGR(rgb, top, 255, 0, 0);
            SET_PIXEL_BGR(rgb, bot, 255, 0, 0);
        }
    }

    // lewa i prawa krawędź
    for (int y = y0; y <= y1; y++) {
        int left  = (y * img_w + x0) * 3;
        int right = (y * img_w + x1) * 3;

        if (roi->enabled) {
            SET_PIXEL_BGR(rgb, left, 0, 0, 255);
            SET_PIXEL_BGR(rgb, right, 0, 0, 255);
        }
        else {
            SET_PIXEL_BGR(rgb, left, 255, 0, 0);
            SET_PIXEL_BGR(rgb, right, 255, 0, 0);
        }
    }
}

void draw_centroid_rgb(uint8_t *rgb, int img_w, int img_h, const centroid_t *c, int color) {
    if (!c->valid)
        return;

    int cx = (int)(c->x + 0.5f);
    int cy = (int)(c->y + 0.5f);

    const int size = 5;

    for (int dx = -size; dx <= size; dx++) {
        int x = cx + dx;
        int y = cy;
        if (x >= 0 && x < img_w && y >= 0 && y < img_h) {
            int p = (y * img_w + x) * 3;
            if(color == 0) SET_PIXEL_BGR(rgb, p, 255, 0, 0);
            else if(color == 1)SET_PIXEL_BGR(rgb, p, 0, 255, 0);
            else SET_PIXEL_BGR(rgb, p, 0, 0, 255);
        }
    }

    for (int dy = -size; dy <= size; dy++) {
        int x = cx;
        int y = cy + dy;
        if (x >= 0 && x < img_w && y >= 0 && y < img_h) {
            int p = (y * img_w + x) * 3;
            if(color == 0) SET_PIXEL_BGR(rgb, p, 255, 0, 0);
            else if(color == 1)SET_PIXEL_BGR(rgb, p, 0, 255, 0);
            else SET_PIXEL_BGR(rgb, p, 0, 0, 255);
        }
    }
}

bool get_image_large(uint8_t** img_buf, size_t* img_size) {
    ESP_LOGI(TAG, "Image requested!");

    if (!g_image_snapshot.valid) {
        return false;
    }

    if (xSemaphoreTake(g_image_mutex, pdTICKS_TO_MS(1000)) != pdTRUE) {
        return false;
    }

    int w = g_image_snapshot.width;
    int h = g_image_snapshot.height;

    *img_buf = malloc(w * h);
    *img_size = w * h;

    grayscale_to_buff(g_image_snapshot.buf, *img_buf, w, h);

    xSemaphoreGive(g_image_mutex);

    ESP_LOGI(TAG, "Image prepared!");
    return true;
}

bool get_preview(uint8_t** jpg_buf, size_t* jpg_len) {
    ESP_LOGI(TAG, "Preview requested!");

    if (!g_image_snapshot.valid) {
        return false;
    }

    if (xSemaphoreTake(g_image_mutex, pdTICKS_TO_MS(1000)) != pdTRUE) {
        return false;
    }

    int w = g_image_snapshot.width;
    int h = g_image_snapshot.height;

    static uint8_t *rgb_buf = NULL;
    static size_t rgb_size = 0;

    if (rgb_size < w * h * 3) {
        free(rgb_buf);
        rgb_buf = malloc(w * h * 3);
        rgb_size = w * h * 3;
    }

    grayscale_to_rgb888(g_image_snapshot.buf, rgb_buf, w, h);
    draw_roi_rect_rgb(rgb_buf, w, h, &g_roi);
    draw_centroid_rgb(rgb_buf, w, h, &g_target, 1);
    draw_centroid_rgb(rgb_buf, w, h, &g_centroid, 2);

    xSemaphoreGive(g_image_mutex);

    bool ok = fmt2jpg(
        rgb_buf,
        w * h * 3,
        w,
        h,
        PIXFORMAT_RGB888,
        80,
        jpg_buf,
        jpg_len
    );

    if (!ok) {
        ESP_LOGI(TAG, "Preview failed!");
        return false;
    }

    ESP_LOGI(TAG, "Preview prepared!");
    return true;
}

bool get_preview_large(uint8_t** rgb_buf, size_t* rgb_size) {
    ESP_LOGI(TAG, "Preview requested!");

    if (!g_image_snapshot.valid) {
        return false;
    }

    if (xSemaphoreTake(g_image_mutex, pdTICKS_TO_MS(1000)) != pdTRUE) {
        return false;
    }

    int w = g_image_snapshot.width;
    int h = g_image_snapshot.height;

    *rgb_buf = malloc(w * h * 3);
    *rgb_size = w * h * 3;

    grayscale_to_rgb888(g_image_snapshot.buf, *rgb_buf, w, h);
    draw_roi_rect_rgb(*rgb_buf, w, h, &g_roi);
    draw_centroid_rgb(*rgb_buf, w, h, &g_target, 1);
    draw_centroid_rgb(*rgb_buf, w, h, &g_centroid, 2);

    xSemaphoreGive(g_image_mutex);

    ESP_LOGI(TAG, "Preview prepared!");
    return true;
}

#if LATENCY_DEBUG

#include "esp_timer.h"
#include <math.h>
#include <stdint.h>

#define LATENCY_BUF_SIZE 100

static volatile uint32_t t_start_buf[LATENCY_BUF_SIZE];
static volatile uint32_t t_idx = 0;

static uint32_t results[LATENCY_BUF_SIZE];
static uint32_t results_count = 0;


// --- ISR START ---
void IRAM_ATTR latency_measure_isr_event(void)
{
    t_start_buf[t_idx] = (uint32_t)esp_timer_get_time();
}


// --- TASK STOP ---
void latency_measure_capture_event(void)
{
    if (t_idx >= LATENCY_BUF_SIZE) return;

    uint32_t t_stop = (uint32_t)esp_timer_get_time();
    uint32_t t_start = t_start_buf[t_idx];

    uint32_t dt = t_stop - t_start;

    results[t_idx] = dt;

    t_idx++;
    results_count = t_idx;
}

// --- STATS ---
void latency_measure_print_stats(void)
{
    if (t_idx >= LATENCY_BUF_SIZE) latency_measure_reset();

    double sum = 0.0;
    double sum_sq = 0.0;

    for (uint32_t i = 0; i < results_count; i++) {
        double x = (double)results[i];
        sum += x;
        sum_sq += x * x;
    }

    double mean = sum / results_count;
    double var = (sum_sq / results_count) - (mean * mean);
    double std = sqrt(var);

    uint32_t t1 = esp_timer_get_time();
    uint32_t t2 = esp_timer_get_time();

    ESP_LOGI(TAG, "Latency stats: N=%lu latency=%lu us, mean=%.2f us, std=%.2f us, overhead=%lu us",
             t_idx-1, results[t_idx-1], mean, std, t2-t1);
}

void latency_measure_reset(void) {
    ESP_LOGI(TAG, "Latency stats reset!");

    memset((void*)t_start_buf, 0, sizeof(t_start_buf));
    t_idx = 0;

    memset((void*)results, 0, sizeof(results));
    results_count = 0;
}

#endif // LATENCY_DEBUG
