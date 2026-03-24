#pragma once

#include "esp_camera.h"

// Device name and version
#define DEV_NAME "ESP32-S3 CAMERA, v2.0"

// Snapshot JPEG limit (ochrona heap)
// #define MAX_JPEG_SIZE (300 * 1024)

// Trigger interrupt
#define CAM_TRIGGER_GPIO GPIO_NUM_2

// DEBUG
#define LATENCY_DEBUG 0

/* =========================
   ACTIVE API
   ========================= */
#define ENABLE_HTTP_API    0
#define ENABLE_UART_API    0
#define ENABLE_CONSOLE_API 1

/* =========================
   WIFI CONFIG
   ========================= */
#define WIFI_SSID ""
#define WIFI_PASS ""

/* =========================
   CONFIG
   ========================= */
typedef struct {
   uint16_t cam_exposure;
   uint8_t cam_gain;
   int8_t cam_brightness;
   int8_t cam_contrast;

   float target_x;
   float target_y;
   float target_tolerance;
   uint8_t brightness_threshold;

   uint16_t roi_x0;
   uint16_t roi_y0;
   uint16_t roi_dx;
   uint16_t roi_dy;
   uint8_t  roi_enabled;

   uint32_t version;
} config_t;

extern config_t g_config;

bool config_load(config_t *cfg);
bool config_save(const config_t *cfg);
void config_apply();

// Other constants
#define TARGET_MAX_HIT_COUNT 1

/* =========================
   CAMERA PINOUT (XIAO S3 Sense)
   ========================= */
#define CAM_PIN_PWDN   -1
#define CAM_PIN_RESET  -1

#define CAM_PIN_XCLK   10
#define CAM_PIN_SIOD   40
#define CAM_PIN_SIOC   39

#define CAM_PIN_D7     48
#define CAM_PIN_D6     11
#define CAM_PIN_D5     12
#define CAM_PIN_D4     14
#define CAM_PIN_D3     16
#define CAM_PIN_D2     18
#define CAM_PIN_D1     17
#define CAM_PIN_D0     15

#define CAM_PIN_VSYNC  38
#define CAM_PIN_HREF   47
#define CAM_PIN_PCLK   13

#define CAM_FRAMESIZE FRAMESIZE_VGA // 640x480
// #define CAM_FRAMESIZE FRAMESIZE_QVGA // 320x240
// #define CAM_FRAMESIZE FRAMESIZE_QQVGA // 160x120