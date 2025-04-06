idf_component_register(
    SRCS "acc_data_provider.c"
    INCLUDE_DIRS "."
    REQUIRES driver freertos acc-LIS2DH12TR
)
