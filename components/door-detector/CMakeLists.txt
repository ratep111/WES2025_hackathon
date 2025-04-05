idf_component_register(
    SRCS "door_detector.c"
    INCLUDE_DIRS "."
    REQUIRES driver freertos infrared-tcrt5000
)