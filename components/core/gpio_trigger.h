#pragma once

#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"


esp_err_t gpio_trigger_init();
esp_err_t gpio_trigger_deinit();
