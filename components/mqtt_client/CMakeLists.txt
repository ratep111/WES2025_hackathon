idf_component_register(
    SRCS "my_mqtt_client.c"
    INCLUDE_DIRS "."
    REQUIRES driver mqtt nvs_flash esp_netif json led
)