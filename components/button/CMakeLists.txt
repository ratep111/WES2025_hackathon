idf_component_register(
    SRCS "button.c" "platform/src/button_gpio.c"
    INCLUDE_DIRS "platform/inc" "."
    REQUIRES driver
)
