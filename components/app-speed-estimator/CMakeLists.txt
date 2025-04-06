idf_component_register(
    SRCS "speed_estimator.c"
    INCLUDE_DIRS "."
    REQUIRES driver freertos app-acc-data-provider
)