idf_component_register(
    SRCS "crash_detector.c"
    INCLUDE_DIRS "."
    REQUIRES driver freertos acc-LIS2DH12TR io-expander-pcf8574
)
