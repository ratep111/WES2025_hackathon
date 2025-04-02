set(COMPONENT_SRCS 
                    "esp32-sht3x/sht3x.c")
set(COMPONENT_ADD_INCLUDEDIRS 
                    "." "esp32-sht3x")
set(COMPONENT_REQUIRES driver)

register_component()