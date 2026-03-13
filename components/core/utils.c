#include "utils.h"

#include <stdlib.h>

#include "esp_log.h"

#include "analysis.h"
#include "esp_camera.h"   // fmt2jpg


#define SET_PIXEL_BGR(buf, idx, b, g, r) \
    do { buf[idx+0] = (b); buf[idx+1] = (g); buf[idx+2] = (r); } while(0)


static const char *TAG = "utils";


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