idf_component_register(
    SRCS 
        "main.c"
    INCLUDE_DIRS "."
    REQUIRES
        api
        core
        camera_ctrl
        esp_psram
        esp32-camera
)
