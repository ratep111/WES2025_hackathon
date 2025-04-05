idf_component_register(
    SRCS "day_night_detector.c"
    INCLUDE_DIRS "."
    REQUIRES driver freertos als-veml7700
    )