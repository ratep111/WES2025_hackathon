set(COMPONENT_SRCS "button.c" "platform/src/button_gpio.c")
set(COMPONENT_ADD_INCLUDEDIRS "platform/inc" ".")
set(COMPONENT_REQUIRES driver)

register_component()
