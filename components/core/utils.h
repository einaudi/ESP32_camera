#pragma once

#include "analysis.h"


void grayscale_to_rgb888(uint8_t *gray, uint8_t *rgb, int width, int height);
void draw_roi_rect_rgb(uint8_t *rgb, int img_w, int img_h, const roi_t *roi);
void draw_centroid_rgb(uint8_t *rgb, int img_w, int img_h, const centroid_t *c,  int color);
bool get_preview(uint8_t** jpg_buf, size_t* jpg_len);
bool get_preview_large(uint8_t** rgb_buf, size_t* rgb_size);