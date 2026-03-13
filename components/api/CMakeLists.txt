idf_component_register(
    SRCS
        "api.c"
        "http_api.c"
        "uart_api.c"

    INCLUDE_DIRS
        "."

    REQUIRES
        core
        camera_ctrl
        esp_wifi
        esp_http_server
        spiffs
)