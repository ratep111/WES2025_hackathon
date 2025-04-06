/**
 * @file acc_data_provider.c
 * @brief Centralized accelerometer data provider to reduce SPI bus contention
 */
#include "acc_data_provider.h"
#include "LIS2DH12TR.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <math.h>
#include <string.h>

#define TAG                 "ACC_DATA_PROVIDER"
#define ACC_TASK_STACK_SIZE 2048
#define ACC_TASK_PRIORITY   10 // Higher priority than consumers

// The shared data structure
static acc_data_t shared_acc_data = { 0 };

// Mutex to protect shared data access
static SemaphoreHandle_t data_mutex = NULL;

// Filter coefficient
#define FILTER_ALPHA 0.8f

esp_err_t acc_data_provider_init(void) {
    // Create the mutex
    data_mutex = xSemaphoreCreateMutex();
    if(!data_mutex) {
        ESP_LOGE(TAG, "Failed to create data mutex");
        return ESP_FAIL;
    }

    // Initialize the accelerometer
    LIS2DH12TR_init_status status = LIS2DH12TR_init();
    if(status != LIS2DH12TR_OK) {
        ESP_LOGE(TAG, "Failed to initialize LIS2DH12TR: status=%d", status);
        return ESP_FAIL;
    }

    // Initialize shared data
    memset(&shared_acc_data, 0, sizeof(acc_data_t));
    shared_acc_data.is_valid = false;

    ESP_LOGI(TAG, "Accelerometer data provider initialized");
    return ESP_OK;
}

esp_err_t acc_data_get(acc_data_t *data) {
    if(data == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    if(xSemaphoreTake(data_mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
        memcpy(data, &shared_acc_data, sizeof(acc_data_t));
        xSemaphoreGive(data_mutex);
        return ESP_OK;
    } else {
        ESP_LOGW(TAG, "Timeout getting accelerometer data");
        return ESP_ERR_TIMEOUT;
    }
}

void acc_data_provider_task(void *pvParameters) {
    // Wait for a short time to ensure system initialization is complete
    vTaskDelay(pdMS_TO_TICKS(500));

    ESP_LOGI(TAG, "Accelerometer data provider task started");

    LIS2DH12TR_accelerations raw_acc = { 0 };
    acc_data_t local_data            = { 0 };
    TickType_t last_wake_time        = xTaskGetTickCount();

    while(1) {
        // Read accelerometer data
        LIS2DH12TR_reading_status read_status = LIS2DH12TR_read_acc(&raw_acc);

        if(read_status == LIS2DH12TR_READING_OK) {
            // Copy raw values
            memcpy(&local_data.raw_acc, &raw_acc, sizeof(LIS2DH12TR_accelerations));

            // Apply low-pass filter to each axis
            local_data.filtered_acc_x = FILTER_ALPHA * local_data.filtered_acc_x + (1 - FILTER_ALPHA) * raw_acc.x_acc;
            local_data.filtered_acc_y = FILTER_ALPHA * local_data.filtered_acc_y + (1 - FILTER_ALPHA) * raw_acc.y_acc;
            local_data.filtered_acc_z = FILTER_ALPHA * local_data.filtered_acc_z + (1 - FILTER_ALPHA) * raw_acc.z_acc;

            // Calculate magnitudes
            local_data.magnitude = sqrtf(
                    local_data.filtered_acc_x * local_data.filtered_acc_x + local_data.filtered_acc_y * local_data.filtered_acc_y
                    + local_data.filtered_acc_z * local_data.filtered_acc_z);

            local_data.magnitude_horizontal = sqrtf(
                    local_data.filtered_acc_x * local_data.filtered_acc_x
                    + local_data.filtered_acc_y * local_data.filtered_acc_y);

            // Update timestamp and validity
            local_data.timestamp = xTaskGetTickCount() * portTICK_PERIOD_MS;
            local_data.is_valid  = true;
            local_data.sample_count++;

            // Copy to shared data structure with mutex protection
            if(xSemaphoreTake(data_mutex, pdMS_TO_TICKS(5)) == pdTRUE) {
                memcpy(&shared_acc_data, &local_data, sizeof(acc_data_t));
                xSemaphoreGive(data_mutex);
            } else {
                ESP_LOGW(TAG, "Mutex timeout when updating shared data");
            }

            // Debug log (reduced frequency to avoid console flooding)
            if(local_data.sample_count % 50 == 0) {
                ESP_LOGD(TAG,
                        "ACC data: X=%.2f Y=%.2f Z=%.2f Mag=%.2f",
                        local_data.filtered_acc_x,
                        local_data.filtered_acc_y,
                        local_data.filtered_acc_z,
                        local_data.magnitude);
            }
        } else if(read_status == LIS2DH12TR_READING_ERROR) {
            ESP_LOGW(TAG, "Error reading accelerometer data");
        }

        // Run at a fixed frequency
        vTaskDelayUntil(&last_wake_time, pdMS_TO_TICKS(ACC_UPDATE_RATE_MS));
    }
}

esp_err_t acc_data_provider_start(void) {
    // Create the accelerometer task
    BaseType_t result =
            xTaskCreate(acc_data_provider_task, "acc_provider", ACC_TASK_STACK_SIZE, NULL, ACC_TASK_PRIORITY, NULL);

    if(result != pdPASS) {
        ESP_LOGE(TAG, "Failed to create accelerometer data provider task");
        return ESP_FAIL;
    }

    return ESP_OK;
}