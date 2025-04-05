idf_component_register(
    SRCS "hc-sr04.c"
    INCLUDE_DIRS "."
    REQUIRES driver freertos esp_idf_lib_helpers esp_timer
)