idf_component_register(
    SRCS "parking_sensor.c"
    INCLUDE_DIRS "."
    REQUIRES driver freertos ultrasonic-hc-sr04
)