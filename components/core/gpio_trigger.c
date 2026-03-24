#include "gpio_trigger.h"
#include "camera_worker.h"

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "app_config.h"
#include "utils.h"


static const char *TAG = "gpio_trigger";


/* optional: debounce */
#define TRIGGER_DEBOUNCE_MS 0

/* --- ISR --- */

static void IRAM_ATTR gpio_trigger_isr(void *arg)
{
#if TRIGGER_DEBOUNCE_MS > 0
    static uint32_t last_tick = 0;
    uint32_t now = xTaskGetTickCountFromISR();

    if ((now - last_tick) < pdMS_TO_TICKS(TRIGGER_DEBOUNCE_MS)) {
        return;
    }
    last_tick = now;
#endif

#if LATENCY_DEBUG
    latency_measure_isr_event();
#endif

    /* ISR-safe camera task notification */
    camera_capture_notify_from_isr();

    // ESP_LOGI(TAG, "GPIO trigger fired!");
}


/* --- API --- */

esp_err_t gpio_trigger_init()
{
    gpio_config_t io_conf = {
        .intr_type = GPIO_INTR_POSEDGE,
        .mode = GPIO_MODE_INPUT,
        .pin_bit_mask = (1ULL << CAM_TRIGGER_GPIO),
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE
    };

    ESP_ERROR_CHECK(gpio_config(&io_conf));

    /* ISR installation */
    esp_err_t err = gpio_install_isr_service(0);
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
        return err;
    }

    ESP_ERROR_CHECK(
        gpio_isr_handler_add(CAM_TRIGGER_GPIO, gpio_trigger_isr, NULL)
    );

    ESP_LOGI(TAG, "GPIO trigger initialized on pin %d", CAM_TRIGGER_GPIO);

    return ESP_OK;
}


esp_err_t gpio_trigger_deinit()
{
    ESP_ERROR_CHECK(gpio_isr_handler_remove(CAM_TRIGGER_GPIO));

    ESP_LOGI(TAG, "GPIO trigger removed from pin %d", CAM_TRIGGER_GPIO);

    return ESP_OK;
}