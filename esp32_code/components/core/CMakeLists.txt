idf_component_register(
    SRCS
        "analysis.c"
        "app_config.c"
        "utils.c"
        "gpio_trigger.c"

    INCLUDE_DIRS
        "."

    REQUIRES
        nvs_flash
        camera_ctrl
        esp_timer
        esp32-camera
)