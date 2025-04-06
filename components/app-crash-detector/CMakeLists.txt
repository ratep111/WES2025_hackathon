idf_component_register(
    SRCS "crash_detector.c"
    INCLUDE_DIRS "."
    REQUIRES driver freertos io-expander-pcf8574 app-acc-data-provider
)
