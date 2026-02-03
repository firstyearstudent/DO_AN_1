idf_component_register(
    SRCS "AD8232.c"
    INCLUDE_DIRS "."
    REQUIRES "driver" "esp_timer" "ADS1115"
)