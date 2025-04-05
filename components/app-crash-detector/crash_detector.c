/**
 * @file crash_detector.c
 * @brief Impact/crash detection using accelerometer
 */
#include "crash_detector.h"
#include "LIS2DH12TR.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "driver/uart.h"
#include <string.h>
#include <time.h>
#include <math.h>

#define TAG "CRASH_DETECTOR"

// UART configuration for sending notifications to ESP32-CAM
#define UART_NUM       UART_NUM_1
#define UART_TX_PIN    1 // TXD0 pin (GPIO1)
#define UART_RX_PIN    3 // RXD0 pin (GPIO3)
#define UART_BAUD_RATE 115200

// Sampling interval (ms)
#define SAMPLE_INTERVAL_MS 50 // 50Hz sampling rate for fast detection

// Configuration and state variables
static float crash_threshold                        = CRASH_ACCEL_THRESHOLD;
static bool crash_detected                          = false;
static crash_event_t last_crash_event               = { 0 };
static TimerHandle_t reset_timer                    = NULL;
static void (*crash_callback)(crash_event_t *event) = NULL;

// Mock timestamp counter (instead of SNTP)
static time_t mock_timestamp = 1712342400; // April 5, 2024 12:00:00 UTC

/**
 * @brief Generate a mock timestamp
 * 
 * @return time_t Current mock time
 */
static time_t get_mock_time(void) {
    // Increment mock time by sampling interval
    mock_timestamp += (SAMPLE_INTERVAL_MS / 1000);
    return mock_timestamp;
}

/**
 * @brief Format timestamp to human-readable string with underscores instead of colons
 * 
 * @param ts Unix timestamp
 * @param buf Output buffer
 * @param size Buffer size
 */
static void format_timestamp(time_t ts, char *buf, size_t size) {
    struct tm timeinfo;
    localtime_r(&ts, &timeinfo);

    // Format: YYYY-MM-DD HH_MM_SS (using underscores instead of colons)
    snprintf(buf,
            size,
            "%04d-%02d-%02d %02d_%02d_%02d",
            timeinfo.tm_year + 1900,
            timeinfo.tm_mon + 1,
            timeinfo.tm_mday,
            timeinfo.tm_hour,
            timeinfo.tm_min,
            timeinfo.tm_sec);
}

/**
 * @brief Send crash notification via UART
 *
 * @param event Crash event data
 */
static void send_crash_notification(crash_event_t *event) {
    char notification[64];

    // Send only the timestamp to UART for simpler parsing
    snprintf(notification, sizeof(notification), "%s\n", event->timestamp_str);

    uart_write_bytes(UART_NUM, notification, strlen(notification));

    // Keep the full log message for console debugging
    ESP_LOGI(TAG, "CRASH DETECTED! Force: %.2fg, Time: %s", event->impact_force, event->timestamp_str);
}

/**
 * @brief Timer callback to auto-reset crash state
 */
static void reset_timer_callback(TimerHandle_t xTimer) {
    crash_detected = false;
    ESP_LOGI(TAG, "Crash state auto-reset after timeout");
}

esp_err_t crash_detector_init(void) {
    // Configure UART for notification
    uart_config_t uart_config = {
        .baud_rate           = UART_BAUD_RATE,
        .data_bits           = UART_DATA_8_BITS,
        .parity              = UART_PARITY_DISABLE,
        .stop_bits           = UART_STOP_BITS_1,
        .flow_ctrl           = UART_HW_FLOWCTRL_DISABLE,
        .rx_flow_ctrl_thresh = 0,
        .source_clk          = UART_SCLK_DEFAULT,
    };

    // Install UART driver
    ESP_LOGI(TAG, "Initializing UART for crash notifications");
    esp_err_t ret = uart_driver_install(UART_NUM, 256, 0, 0, NULL, 0);
    if(ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to install UART driver: %s", esp_err_to_name(ret));
        return ret;
    }

    ret = uart_param_config(UART_NUM, &uart_config);
    if(ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure UART parameters: %s", esp_err_to_name(ret));
        return ret;
    }

    ret = uart_set_pin(UART_NUM, UART_TX_PIN, UART_RX_PIN, -1, -1);
    if(ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set UART pins: %s", esp_err_to_name(ret));
        return ret;
    }

    // Create auto-reset timer
    reset_timer = xTimerCreate("crash_reset_timer",
            pdMS_TO_TICKS(CRASH_RESET_TIMEOUT_MS),
            pdFALSE, // One-shot timer
            0,
            reset_timer_callback);

    if(reset_timer == NULL) {
        ESP_LOGE(TAG, "Failed to create crash reset timer");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Crash detector initialized with threshold: %.2fg", crash_threshold);
    return ESP_OK;
}

void crash_detector_task(void *pvParameters) {
    // Ensure LIS2DH12TR accelerometer is initialized
    // Note: We assume the accelerometer is already initialized by the
    // speed estimator component, so we don't initialize it again here

    LIS2DH12TR_accelerations acc = { 0 };
    float impact_magnitude;

    ESP_LOGI(TAG, "Crash detector task started");

    while(1) {
        if(LIS2DH12TR_read_acc(&acc) == LIS2DH12TR_READING_OK) {
            // Calculate total impact force (vector magnitude of acceleration)
            impact_magnitude = sqrtf(acc.x_acc * acc.x_acc + acc.y_acc * acc.y_acc + acc.z_acc * acc.z_acc);

            // Check for crash (subtract 1g for earth's gravity)
            float adjusted_magnitude = impact_magnitude > 1.0f ? impact_magnitude - 1.0f : 0.0f;

            // Crash detection logic
            if(adjusted_magnitude > crash_threshold && !crash_detected) {
                // Create crash event
                crash_detected                = true;
                last_crash_event.impact_force = adjusted_magnitude;
                last_crash_event.timestamp    = get_mock_time();
                format_timestamp(last_crash_event.timestamp,
                        last_crash_event.timestamp_str,
                        sizeof(last_crash_event.timestamp_str));

                // Send notification
                send_crash_notification(&last_crash_event);

                // Notify callback if registered
                if(crash_callback != NULL) {
                    crash_callback(&last_crash_event);
                }

                // Start auto-reset timer
                xTimerStart(reset_timer, 0);

                ESP_LOGI(TAG, "Crash detected! Force: %.2fg", adjusted_magnitude);
            }

            // Optional: Log high impacts even if not a crash
            if(adjusted_magnitude > crash_threshold / 2) {
                ESP_LOGD(TAG, "High impact detected: %.2fg", adjusted_magnitude);
            }
        }

        vTaskDelay(pdMS_TO_TICKS(SAMPLE_INTERVAL_MS));
    }
}

bool crash_detector_is_crashed(void) {
    return crash_detected;
}

bool crash_detector_get_last_event(crash_event_t *event) {
    if(!crash_detected || event == NULL) {
        return false;
    }

    memcpy(event, &last_crash_event, sizeof(crash_event_t));
    return true;
}

void crash_detector_reset(void) {
    crash_detected = false;
    ESP_LOGI(TAG, "Crash state manually reset");
}

void crash_detector_set_threshold(float threshold) {
    if(threshold > 0) {
        crash_threshold = threshold;
        ESP_LOGI(TAG, "Crash threshold updated to %.2fg", threshold);
    }
}

void crash_detector_register_callback(void (*callback)(crash_event_t *event)) {
    crash_callback = callback;
}