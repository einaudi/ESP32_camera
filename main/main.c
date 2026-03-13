#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"

#include "esp_log.h"
#include "nvs_flash.h"

#include "app_config.h"
#include "camera_worker.h"
#include "analysis.h"
#include "api.h"

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

    ESP_ERROR_CHECK(image_snapshot_init(320 * 240)); // QVGA grayscale
    ESP_ERROR_CHECK(camera_worker_start());

    config_load(&g_config);
    config_apply();

    api_init();

    ESP_LOGI(TAG, "Ready!");
}
