#pragma once

#include <stdio.h>

// API status
typedef enum {
    API_OK,
    API_ERR_INVALID_ARG,
    API_ERR_BUSY,
    API_ERR_INTERNAL,
} api_status_t;

// API responses
typedef struct {
    api_status_t    status;
    int             value;
} api_response_int_t;

typedef struct {
    api_status_t    status;
    float           value;
} api_response_float_t;

typedef struct {
    api_status_t    status;
} api_response_image_t;


void api_init(void);

// General
api_status_t api_idn(char* buf, size_t* len);

// Camera control
api_status_t api_set_exposure(int val);
api_status_t api_set_gain(int val);
api_status_t api_set_brightness(int val);
api_status_t api_set_contrast(int val);

api_status_t api_get_exposure(int* val);
api_status_t api_get_gain(int* val);
api_status_t api_get_brightness(int* val);
api_status_t api_get_contrast(int* val);

// ROI control
api_status_t api_set_ROI_x0(uint16_t val);
api_status_t api_set_ROI_y0(uint16_t val);
api_status_t api_set_ROI_dx(uint16_t val);
api_status_t api_set_ROI_dy(uint16_t val);
api_status_t api_set_ROI_enable(uint8_t val);

api_status_t api_get_ROI_x0(int* val);
api_status_t api_get_ROI_y0(int* val);
api_status_t api_get_ROI_dx(int* val);
api_status_t api_get_ROI_dy(int* val);
api_status_t api_get_ROI_enable(int* val);

// Target control
api_status_t api_set_target_x0(float val);
api_status_t api_set_target_y0(float val);
api_status_t api_set_target_x0_relative(float val);
api_status_t api_set_target_y0_relative(float val);
api_status_t api_set_target_tol(float val);
api_status_t api_set_target_threshold(float val);
api_status_t api_set_target_ROI_bound(uint8_t val);

api_status_t api_get_target_x0(float* val);
api_status_t api_get_target_y0(float* val);
api_status_t api_get_target_x0_relative(float* val);
api_status_t api_get_target_y0_relative(float* val);
api_status_t api_get_target_tol(float* val);
api_status_t api_get_target_threshold(float* val);
api_status_t api_get_target_ROI_bound(int* val);
// Target error
api_status_t api_get_target_error_x(float* val);
api_status_t api_get_target_error_y(float* val);
api_status_t api_get_target_is_hit(uint8_t* val);

// Centroid data
api_status_t api_get_centroid_x0(float* val);
api_status_t api_get_centroid_y0(float* val);
api_status_t api_get_centroid_x0_relative(float* val);
api_status_t api_get_centroid_y0_relative(float* val);

// Image
api_status_t api_snapshot();