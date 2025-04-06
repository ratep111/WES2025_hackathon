#include "speed_estimator.h"
#include "acc_data_provider.h" // New accelerometer data provider
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include <math.h>

#define TAG "SPEED_ESTIMATOR"

// Sampling interval (seconds)
#define SAMPLE_INTERVAL_MS  100
#define SAMPLE_INTERVAL_SEC (SAMPLE_INTERVAL_MS / 1000.0f)

// State variables
static float current_speed                    = 0.0f;
static movement_direction_t current_direction = DIRECTION_UNKNOWN;

// For stationary detection
static const float STATIONARY_THRESHOLD     = 0.05f;
static int stationary_count                 = 0;
static const int STATIONARY_COUNT_THRESHOLD = 10;

// For direction detection
static const float DOMINANT_AXIS_THRESHOLD = 0.1f;

esp_err_t speed_estimator_init(void) {
    ESP_LOGI(TAG, "Speed estimator initialized");
    return ESP_OK;
}

float speed_estimator_get_speed_mps(void) {
    return current_speed;
}

float speed_estimator_get_speed_kmh(void) {
    return current_speed * 3.6f;
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
    // Wait for a bit to ensure the acc data provider is running
    vTaskDelay(pdMS_TO_TICKS(1000));

    ESP_LOGI(TAG, "Speed estimator task started");

    TickType_t last_wake_time = xTaskGetTickCount();
    acc_data_t acc_data       = { 0 };

    while(1) {
        // Get latest accelerometer data from the provider
        if(acc_data_get(&acc_data) == ESP_OK && acc_data.is_valid) {
            // Use the horizontal plane magnitude for speed estimation
            float acc_magnitude = acc_data.magnitude_horizontal;

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
                float abs_x = fabsf(acc_data.filtered_acc_x);
                float abs_y = fabsf(acc_data.filtered_acc_y);

                if(abs_x > DOMINANT_AXIS_THRESHOLD || abs_y > DOMINANT_AXIS_THRESHOLD) {
                    if(abs_x > abs_y) {
                        current_direction = (acc_data.filtered_acc_x > 0) ? DIRECTION_LEFT : DIRECTION_RIGHT;
                    } else {
                        current_direction = (acc_data.filtered_acc_y > 0) ? DIRECTION_FORWARD : DIRECTION_BACKWARD;
                    }
                }
            }

            ESP_LOGI(TAG,
                    "Speed: %.2f m/s (%.2f km/h), Direction: %s",
                    current_speed,
                    current_speed * 3.6f,
                    speed_estimator_get_direction_string());
        } else {
            ESP_LOGW(TAG, "Failed to get valid accelerometer data");
        }

        // Run at a fixed interval
        vTaskDelayUntil(&last_wake_time, pdMS_TO_TICKS(SAMPLE_INTERVAL_MS));
    }
}
