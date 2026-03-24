#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"
#include "esp_err.h"
#include "esp_camera.h"

// ====== Camera commands ======
typedef enum {
    CAM_CMD_SET_EXPOSURE,
    CAM_CMD_SET_GAIN,
    CAM_CMD_SET_BRIGHTNESS,
    CAM_CMD_SET_CONTRAST,
} cam_cmd_type_t;

typedef struct {
    cam_cmd_type_t type;
    int value;
} cam_cmd_t;

// ====== Camera module API ======
esp_err_t camera_worker_start();
bool queue_cam_cmd(cam_cmd_type_t type, int value);

esp_err_t camera_capture();
void camera_capture_notify_from_isr(void);