/**
 * @file acc_data_provider.h
 * @brief Centralized accelerometer data provider to reduce SPI bus contention
 */
#ifndef ACC_DATA_PROVIDER_H
#define ACC_DATA_PROVIDER_H

#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "LIS2DH12TR.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Update rate for accelerometer reading in milliseconds */
#define ACC_UPDATE_RATE_MS 200 // Higher frequency than any consumer needs

/**
 * @brief Shared accelerometer data structure
 */
typedef struct {
    LIS2DH12TR_accelerations raw_acc; // Raw accelerometer values
    float filtered_acc_x;             // Filtered X acceleration
    float filtered_acc_y;             // Filtered Y acceleration
    float filtered_acc_z;             // Filtered Z acceleration
    float magnitude;                  // Total acceleration magnitude
    float magnitude_horizontal;       // Horizontal plane magnitude (X-Y)
    uint32_t timestamp;               // Timestamp in milliseconds
    bool is_valid;                    // Whether the data is valid
    uint32_t sample_count;            // Running count of samples taken
} acc_data_t;

/**
 * @brief Initialize the accelerometer data provider
 * 
 * @return esp_err_t ESP_OK on success
 */
esp_err_t acc_data_provider_init(void);

/**
 * @brief Get the latest accelerometer data
 * 
 * @param data Pointer to store the acceleration data
 * @return esp_err_t ESP_OK on success
 */
esp_err_t acc_data_get(acc_data_t *data);

/**
 * @brief Start the accelerometer data provider task
 * 
 * @return esp_err_t ESP_OK on success
 */
esp_err_t acc_data_provider_start(void);

/**
 * @brief Accelerometer data provider task
 * 
 * @param pvParameters Task parameters (not used)
 */
void acc_data_provider_task(void *pvParameters);

#ifdef __cplusplus
}
#endif

#endif /* ACC_DATA_PROVIDER_H */