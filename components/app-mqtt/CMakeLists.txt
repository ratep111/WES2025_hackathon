set(COMPONENT_SRCS "my_mqtt.c" "my_sntp.c")
set(COMPONENT_ADD_INCLUDEDIRS ".")
set(COMPONENT_REQUIRES driver mqtt nvs_flash esp_netif esp_wifi protocol_examples_common) 

register_component()