#include "speed_estimator.h"
#include "LIS2DH12TR.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include <math.h>

#define TAG "SPEED_ESTIMATOR"

// Sampling interval (seconds)
#define SAMPLE_INTERVAL_MS  100
#define SAMPLE_INTERVAL_SEC (SAMPLE_INTERVAL_MS / 1000.0f)

// Flag to track if accelerometer is initialized
static bool accel_initialized                 = false;
static float current_speed                    = 0.0f;
static movement_direction_t current_direction = DIRECTION_UNKNOWN;

esp_err_t speed_estimator_init(void) {
    if(accel_initialized) {
        ESP_LOGI(TAG, "Accelerometer already initialized");
        return ESP_OK;
    }

    ESP_LOGI(TAG, "Initializing accelerometer for speed estimation...");

    // Initialize the accelerometer
    LIS2DH12TR_init_status status = LIS2DH12TR_init();
    if(status != LIS2DH12TR_OK) {
        ESP_LOGE(TAG, "Failed to initialize LIS2DH12TR: status=%d", status);
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Accelerometer initialized successfully");
    accel_initialized = true;
    return ESP_OK;
}

float speed_estimator_get_speed_mps(void) {
    return current_speed;
}

float speed_estimator_get_speed_kmh(void) {
    return current_speed * 3.6;
}

movement_direction_t speed_estimator_get_direction(void) {
    return current_direction;
}

bool speed_estimator_is_moving_forward(void) {
    return current_direction == DIRECTION_FORWARD;
}

bool speed_estimator_is_moving_backward(void) {
    return current_direction == DIRECTION_BACKWARD;
}

const char *speed_estimator_get_direction_string(void) {
    switch(current_direction) {
        case DIRECTION_FORWARD:
            return "Forward";
        case DIRECTION_BACKWARD:
            return "Backward";
        case DIRECTION_LEFT:
            return "Left";
        case DIRECTION_RIGHT:
            return "Right";
        default:
            return "Unknown";
    }
}

void speed_estimator_task(void *args) {
    // Wait for LVGL to initialize the SPI bus
    vTaskDelay(2000 / portTICK_PERIOD_MS);

    // Now initialize the accelerometer
    esp_err_t ret = speed_estimator_init();
    if(ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize speed estimator");
        vTaskDelete(NULL);
    }

    // Delay to ensure everything is properly set up
    vTaskDelay(500 / portTICK_PERIOD_MS);

    ESP_LOGI(TAG, "Speed estimator task started");

    LIS2DH12TR_accelerations acc = { 0 };

    // For filtering
    float filtered_acc_x = 0, filtered_acc_y = 0;
    const float ALPHA = 0.8f;

    // For stationary detection
    const float STATIONARY_THRESHOLD     = 0.05f;
    int stationary_count                 = 0;
    const int STATIONARY_COUNT_THRESHOLD = 10;

    // For direction detection
    const float DOMINANT_AXIS_THRESHOLD = 0.1f;

    while(1) {
        LIS2DH12TR_reading_status read_status = LIS2DH12TR_read_acc(&acc);

        if(read_status == LIS2DH12TR_READING_OK) {
            // Apply low-pass filter to smooth acceleration readings
            filtered_acc_x = ALPHA * filtered_acc_x + (1 - ALPHA) * acc.x_acc;
            filtered_acc_y = ALPHA * filtered_acc_y + (1 - ALPHA) * acc.y_acc;

            // Calculate magnitude (horizontal plane only)
            float acc_magnitude = sqrtf(filtered_acc_x * filtered_acc_x + filtered_acc_y * filtered_acc_y);

            // Zero velocity update - detect when device is stationary
            if(acc_magnitude < STATIONARY_THRESHOLD) {
                stationary_count++;
                if(stationary_count >= STATIONARY_COUNT_THRESHOLD) {
                    // Device is likely stationary, reset speed to avoid drift
                    current_speed     = 0;
                    current_direction = DIRECTION_UNKNOWN;
                    ESP_LOGD(TAG, "Zero velocity update applied");
                }
            } else {
                stationary_count = 0;

                // Integrate acceleration to get speed: v = v0 + a * t
                current_speed += acc_magnitude * SAMPLE_INTERVAL_SEC;

                // Apply damping to prevent drift
                current_speed *= 0.98f;

                // Determine direction of movement
                float abs_x = fabsf(filtered_acc_x);
                float abs_y = fabsf(filtered_acc_y);

                if(abs_x > DOMINANT_AXIS_THRESHOLD || abs_y > DOMINANT_AXIS_THRESHOLD) {
                    if(abs_x > abs_y) {
                        current_direction = (filtered_acc_x > 0) ? DIRECTION_LEFT : DIRECTION_RIGHT;
                    } else {
                        current_direction = (filtered_acc_y > 0) ? DIRECTION_FORWARD : DIRECTION_BACKWARD;
                    }
                }
            }

            ESP_LOGI(TAG,
                    "Speed: %.2f m/s (%.2f km/h), Direction: %s",
                    current_speed,
                    current_speed * 3.6,
                    speed_estimator_get_direction_string());
        } else if(read_status == LIS2DH12TR_READING_ERROR) {
            ESP_LOGE(TAG, "Error reading accelerometer data");
        } else if(read_status == LIS2DH12TR_READING_EMPTY) {
            ESP_LOGW(TAG, "No new accelerometer data available");
        }

        vTaskDelay(pdMS_TO_TICKS(SAMPLE_INTERVAL_MS));
    }
}