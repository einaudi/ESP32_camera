#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"

#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_camera.h"

#include "app_config.h"
#include "camera_worker.h"
#include "analysis.h"
#include "api.h"
#include "gpio_trigger.h"
#include "utils.h"

static const char *TAG = "main";

/* =========================
   MAIN
   ========================= */
void app_main(void)
{
    ESP_LOGI(TAG, "Boot OK");

    // NVS init - flash storage
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ESP_ERROR_CHECK(nvs_flash_init());
    } else {
        ESP_ERROR_CHECK(ret);
    }

    size_t image_size;
    if(CAM_FRAMESIZE == FRAMESIZE_VGA) image_size = 640*480;
    else if(CAM_FRAMESIZE == FRAMESIZE_QVGA) image_size = 320*240;
    else image_size = 160*120; // QQVGA

    ESP_ERROR_CHECK(image_snapshot_init(image_size));
    ESP_ERROR_CHECK(camera_worker_start());

    config_load(&g_config);
    config_apply();

    // for(uint8_t i=0; i<10; i++) {
    //     camera_capture();
    //     vTaskDelay(pdMS_TO_TICKS(100));
    // }

#if LATENCY_DEBUG
    latency_measure_reset();
#endif // LATENCY_DEBUG

    gpio_trigger_init();

    api_init();

    ESP_LOGI(TAG, "Ready!");
}
