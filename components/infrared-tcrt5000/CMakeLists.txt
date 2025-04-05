idf_component_register(
    SRCS "tcrt5000.c"
    INCLUDE_DIRS "."
    REQUIRES driver esp_adc io-expander-pcf8574
)