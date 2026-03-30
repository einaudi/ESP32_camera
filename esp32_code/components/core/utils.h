#pragma once

#include "analysis.h"
#include "app_config.h"


void grayscale_to_buff(uint8_t *gray, uint8_t *img, int width, int height);
void grayscale_to_rgb888(uint8_t *gray, uint8_t *rgb, int width, int height);
void draw_roi_rect_rgb(uint8_t *rgb, int img_w, int img_h, const roi_t *roi);
void draw_centroid_rgb(uint8_t *rgb, int img_w, int img_h, const centroid_t *c,  int color);
bool get_image_large(uint8_t** img_buf, size_t* img_size);
bool get_preview(uint8_t** jpg_buf, size_t* jpg_len);
bool get_preview_large(uint8_t** rgb_buf, size_t* rgb_size);

#if LATENCY_DEBUG

void latency_measure_isr_event(void);
void latency_measure_capture_event(void);
void latency_measure_print_stats(void);
void latency_measure_reset(void);

#endif