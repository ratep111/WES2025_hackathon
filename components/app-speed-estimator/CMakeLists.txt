idf_component_register(
    SRCS "speed_estimator.c"
    INCLUDE_DIRS "."
    REQUIRES driver freertos acc-LIS2DH12TR
)