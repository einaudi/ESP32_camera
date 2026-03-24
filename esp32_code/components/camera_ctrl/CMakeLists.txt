idf_component_register(
    SRCS
        "camera_worker.c"

    INCLUDE_DIRS
        "."

    REQUIRES
        esp32-camera
        core
)