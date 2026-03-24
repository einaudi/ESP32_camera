#include "camera_worker.h"
#include "esp_log.h"
#include "freertos/task.h"
#include <stdlib.h>
#include <string.h>

#include "app_config.h"
#include "analysis.h"
#include "utils.h"

static const char *TAG = "cam_worker";


static TaskHandle_t camera_task_handle = NULL;
static TaskHandle_t capture_waiter = NULL;
static volatile bool capture_requested = false;
static volatile bool capture_in_progress = false;

static QueueHandle_t g_cmd_queue = NULL;


static void camera_sensor_stop(sensor_t* s)
{
    s->set_reg(s, 0x3008, 0xFF, 0x42);
}

static void camera_sensor_start(sensor_t* s)
{
    s->set_reg(s, 0x3008, 0xFF, 0x02);
}

static void apply_cmd_to_sensor(sensor_t *s, const cam_cmd_t *cmd) {
    if (!s || !cmd) return;
    switch (cmd->type) {
        case CAM_CMD_SET_EXPOSURE:
            ESP_LOGI(TAG, "CMD exposure=%d", cmd->value);
            s->set_aec_value(s, cmd->value);
            g_config.cam_exposure = cmd->value;
            config_save(&g_config);
            break;
        case CAM_CMD_SET_GAIN:
            ESP_LOGI(TAG, "CMD gain=%d", cmd->value);
            s->set_agc_gain(s, cmd->value);
            g_config.cam_gain = cmd->value;
            config_save(&g_config);
            break;
        case CAM_CMD_SET_BRIGHTNESS:
            ESP_LOGI(TAG, "CMD brightness=%d", cmd->value);
            s->set_brightness(s, cmd->value);
            g_config.cam_brightness = cmd->value;
            config_save(&g_config);
            break;
        case CAM_CMD_SET_CONTRAST:
            ESP_LOGI(TAG, "CMD contrast=%d", cmd->value);
            s->set_contrast(s, cmd->value);
            g_config.cam_contrast = cmd->value;
            config_save(&g_config);
            break;
        default:
            break;
    }
}

static void camera_task(void *arg) {
    (void)arg;

    sensor_t *s = esp_camera_sensor_get();

    cam_cmd_t cmd;

    ESP_LOGI(TAG, "Camera ready");

    while (1) {
        // Wake up task
        xTaskNotifyWait(0, 0, NULL, portMAX_DELAY);

        // 1) Handle all pending commands
        while (xQueueReceive(g_cmd_queue, &cmd, 0) == pdTRUE) {
            apply_cmd_to_sensor(s, &cmd);
        }

        // 2) Take snapshot
        if (capture_requested)
        {
            capture_in_progress = true;
            capture_requested = false;

            ESP_LOGI(TAG, "Camera snapshot initiated");

            camera_fb_t* fb;

            camera_sensor_start(s);
#if LATENCY_DEBUG
            latency_measure_capture_event();
            latency_measure_print_stats();
#endif
            // flush frame to ensure latest
            fb = esp_camera_fb_get();
            esp_camera_fb_return(fb);
            // capture
            fb = esp_camera_fb_get();
            camera_sensor_stop(s);


            // Check if camera returned frame buffer
            if (!fb) {
                ESP_LOGW(TAG, "esp_camera_fb_get returned NULL");
                esp_camera_fb_return(fb);
                vTaskDelay(pdMS_TO_TICKS(10));
                continue;
            }
            // Check picture format
            if (fb->format != PIXFORMAT_GRAYSCALE) {
                ESP_LOGW(TAG, "Frame not GRAYSCALE (format=%d)", fb->format);
                esp_camera_fb_return(fb);
                vTaskDelay(pdMS_TO_TICKS(10));
                continue;
            }

            // Copy snapshot to buffer and compute centroid and error
            if (xSemaphoreTake(g_image_mutex, pdMS_TO_TICKS(50)) == pdTRUE) {
                size_t copy_len = fb->len;
                if (copy_len > g_image_snapshot.len) {}

                memcpy(g_image_snapshot.buf, fb->buf, copy_len);
                g_image_snapshot.len    = copy_len;
                g_image_snapshot.width  = fb->width;
                g_image_snapshot.height = fb->height;
                g_image_snapshot.valid  = true;

                compute_centroid(
                    g_image_snapshot.buf,
                    g_image_snapshot.width,
                    g_image_snapshot.height,
                    &g_roi,
                    g_config.brightness_threshold  // threshold
                );
                compute_error();
                
                xSemaphoreGive(g_image_mutex);
            }

            esp_camera_fb_return(fb);

            // flush queue
            // fb = esp_camera_fb_get();
            // if(fb) esp_camera_fb_return(fb);

            capture_in_progress = false;

            if (capture_waiter)
            {
                xTaskNotify(capture_waiter, 1, eSetValueWithOverwrite);
                capture_waiter = NULL;
            }
            ESP_LOGI(TAG, "Camera snapshot taken!");
        }

        // vTaskDelay(pdMS_TO_TICKS(10));
    }
}

esp_err_t camera_worker_start() {

    camera_config_t config = {0};

    config.ledc_channel = LEDC_CHANNEL_0;
    config.ledc_timer   = LEDC_TIMER_0;

    config.pin_d0 = CAM_PIN_D0;
    config.pin_d1 = CAM_PIN_D1;
    config.pin_d2 = CAM_PIN_D2;
    config.pin_d3 = CAM_PIN_D3;
    config.pin_d4 = CAM_PIN_D4;
    config.pin_d5 = CAM_PIN_D5;
    config.pin_d6 = CAM_PIN_D6;
    config.pin_d7 = CAM_PIN_D7;

    config.pin_xclk  = CAM_PIN_XCLK;
    config.pin_pclk  = CAM_PIN_PCLK;
    config.pin_vsync = CAM_PIN_VSYNC;
    config.pin_href  = CAM_PIN_HREF;

    config.pin_sccb_sda = CAM_PIN_SIOD;
    config.pin_sccb_scl = CAM_PIN_SIOC;

    config.pin_pwdn  = CAM_PIN_PWDN;
    config.pin_reset = CAM_PIN_RESET;

    config.xclk_freq_hz = 10000000;

    config.pixel_format = PIXFORMAT_GRAYSCALE;
    config.frame_size = CAM_FRAMESIZE;
    config.jpeg_quality = 12;
    config.fb_count = 1;

    config.fb_location = CAMERA_FB_IN_PSRAM;
    config.grab_mode = CAMERA_GRAB_LATEST; // CAMERA_GRAB_WHEN_EMPTY;

    g_image_mutex = xSemaphoreCreateMutex();
    if (!g_image_mutex) return ESP_ERR_NO_MEM;

    g_cmd_queue = xQueueCreate(16, sizeof(cam_cmd_t));
    if (!g_cmd_queue) return ESP_ERR_NO_MEM;

    ESP_LOGI(TAG, "Initializing camera...");
    esp_err_t err = esp_camera_init(&config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_camera_init() failed: 0x%x", err);
        return err;
    }

    sensor_t *s = esp_camera_sensor_get();
    if (s) {
        ESP_LOGI(TAG, "Camera sensor PID=0x%04x", s->id.PID);
    }
    s->set_exposure_ctrl(s, 0); // manual exposure
    s->set_aec_value(s, 10);   // mid exposure
    s->set_gain_ctrl(s, 0);     // manual gain
    s->set_agc_gain(s, 0);      // no gain
    s->set_contrast(s, 0);      // neutral contrast
    s->set_brightness(s, 0);    // neutral brightness
    s->set_whitebal(s, 0);      // disable auto WB

    // flush queue
    camera_fb_t* fb = esp_camera_fb_get();
    esp_camera_fb_return(fb);

    camera_sensor_stop(s);

    ESP_LOGI(TAG, "Camera init OK");

    xTaskCreate(camera_task, "camera_task", 4096, NULL, 5, &camera_task_handle);

    return ESP_OK;
}

bool queue_cam_cmd(cam_cmd_type_t type, int value) {
    if (!g_cmd_queue) return false;
    cam_cmd_t cmd = {.type = type, .value = value};
    
    if(xQueueSend(g_cmd_queue, &cmd, 0) != pdTRUE) return false;

    if (camera_task_handle) {
        xTaskNotify(camera_task_handle, 0, eNoAction);
    }

    return true;
}

esp_err_t camera_capture() {
    capture_waiter = xTaskGetCurrentTaskHandle();

    if(!capture_in_progress) capture_requested = true;

    xTaskNotify(camera_task_handle, 0, eNoAction);

    uint32_t val;

    if (xTaskNotifyWait(0, 0, &val, pdMS_TO_TICKS(1000)) != pdTRUE)
    {
        ESP_LOGI(TAG, "Camera capture timeout!");
        capture_waiter = NULL;
        return ESP_ERR_TIMEOUT;
    }

    return ESP_OK;
}

void IRAM_ATTR camera_capture_notify_from_isr(void)
{
    if (camera_task_handle == NULL) {
        return;
    }

    if(!capture_in_progress) capture_requested = true;

    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    vTaskNotifyGiveFromISR(camera_task_handle, &xHigherPriorityTaskWoken);

    if (xHigherPriorityTaskWoken) {
        portYIELD_FROM_ISR();
    }
}