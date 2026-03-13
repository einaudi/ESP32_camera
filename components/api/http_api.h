#pragma once

#include "esp_err.h"

void http_api_init(void);

void wifi_init_sta(void);
esp_err_t http_api_start(void);