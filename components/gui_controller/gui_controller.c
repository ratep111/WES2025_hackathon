/**
 * @file gui_controller.c
 * @brief Connects sensor modules to the GUI frontend
 * 
 * This module handles the connection between sensor data and GUI display,
 * registering callbacks for sensor events and updating the UI accordingly.
 */

#include "gui_controller.h"
#include "../gui/gui.h" // Make sure this path matches your actual GUI header location

// Include all sensor modules
#include "crash_detector.h"
#include "day_night_detector.h"
#include "door_detector.h"
#include "parking_sensor.h"
#include "speed_estimator.h"

// For temperature sensing
#include "sht3x.h"

#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include <time.h>
#include <string.h>

#define TAG                             "GUI_CTRL"
#define GUI_CONTROLLER_UPDATE_PERIOD_MS 100
#define GUI_CONTROLLER_STACK_SIZE       4096
#define GUI_CONTROLLER_PRIORITY         5

static TaskHandle_t gui_controller_task_handle = NULL;
static EventGroupHandle_t gui_event_group      = NULL;

// Bit definitions for event group
#define GUI_EVT_SPEED_UPDATE     BIT0
#define GUI_EVT_PROXIMITY_UPDATE BIT1
#define GUI_EVT_DOOR_UPDATE      BIT2
#define GUI_EVT_TIME_UPDATE      BIT3
#define GUI_EVT_TEMP_UPDATE      BIT4
#define GUI_EVT_LIGHT_UPDATE     BIT5
#define GUI_EVT_CRASH_UPDATE     BIT6
#define GUI_EVT_FUEL_UPDATE      BIT7

// Current state storage
static int current_speed                            = 0;
static gui_proximity_t current_proximity            = GUI_PROX_NUM; // Default to invalid value, will be set correctly
static sht3x_sensors_values_t current_temp_humidity = { 0 };
static light_state_t current_light_state            = LIGHT_STATE_UNKNOWN;
static door_state_t door_states[gui_num_of_doors]   = { 0 }; // Match enum in gui.h
static bool crash_detected                          = false;
static int fuel_percentage                          = 100; // Mock value

// Forward declarations for callbacks
static void crash_event_callback(crash_event_t *event);
static void door_state_callback(door_state_t state);
static void light_state_callback(light_state_t state);

/**
   * @brief Main task for the GUI controller
   * 
   * This task periodically checks for events and updates the GUI accordingly
   */
static void gui_controller_task(void *pvParameters) {
    char time_str[16] = { 0 };
    char date_str[16] = { 0 };
    char temp_str[16] = { 0 };
    char hum_str[16]  = { 0 };

    ESP_LOGI(TAG, "GUI controller task started");

    TickType_t last_wake_time = xTaskGetTickCount();

    while(1) {
        EventBits_t events = xEventGroupGetBits(gui_event_group);

        // Handle speed updates
        if(events & GUI_EVT_SPEED_UPDATE) {
            gui_speed_bar_set(current_speed);
            xEventGroupClearBits(gui_event_group, GUI_EVT_SPEED_UPDATE);
            ESP_LOGD(TAG, "Updated speed: %d", current_speed);
        }

        // Handle proximity updates
        if(events & GUI_EVT_PROXIMITY_UPDATE) {
            // Ensure the proximity value is valid before updating
            if(current_proximity >= 0 && current_proximity < GUI_PROX_NUM) {
                ESP_LOGD(TAG, "Setting proximity to: %d", current_proximity);
                gui_proximity_set(current_proximity);
            } else {
                ESP_LOGW(TAG, "Skipping proximity update - invalid value: %d", current_proximity);
            }
            xEventGroupClearBits(gui_event_group, GUI_EVT_PROXIMITY_UPDATE);
        }

        // Handle door updates
        if(events & GUI_EVT_DOOR_UPDATE) {
            for(int i = 0; i < gui_num_of_doors; i++) {
                if(door_states[i] == DOOR_STATE_OPEN) {
                    gui_set_door_open((gui_doors_t) i);
                } else if(door_states[i] == DOOR_STATE_CLOSED) {
                    gui_set_door_closed((gui_doors_t) i);
                }
            }
            xEventGroupClearBits(gui_event_group, GUI_EVT_DOOR_UPDATE);
        }

        // Handle time updates
        if(events & GUI_EVT_TIME_UPDATE) {
            // Get current time
            time_t now;
            struct tm timeinfo;
            time(&now);
            localtime_r(&now, &timeinfo);

            // Format time and date strings
            strftime(time_str, sizeof(time_str), "%H:%M", &timeinfo);
            strftime(date_str, sizeof(date_str), "%d/%m/%Y", &timeinfo);

            // Update GUI
            gui_time_set(time_str);
            gui_date_set(date_str);

            xEventGroupClearBits(gui_event_group, GUI_EVT_TIME_UPDATE);
        }

        // Handle temperature updates
        if(events & GUI_EVT_TEMP_UPDATE) {
            // Format temperature string
            sprintf(temp_str, "%.1f°C", current_temp_humidity.temperature);

            // Format humidity string
            sprintf(hum_str, "%.1f%%", current_temp_humidity.humidity);

            // Update GUI with temperature and humidity
            gui_local_temp_set(temp_str);
            gui_sntp_temp_set(temp_str); // Also update the SNTP temp for consistency
            gui_hum_temp_set(hum_str);

            // Update weather info based on temperature and light
            char weather_info[64];
            if(current_light_state == LIGHT_STATE_DAY) {
                if(current_temp_humidity.temperature > 25) {
                    sprintf(weather_info, "Sunny, %.1f°C", current_temp_humidity.temperature);
                } else {
                    sprintf(weather_info, "Cloudy, %.1f°C", current_temp_humidity.temperature);
                }
            } else {
                sprintf(weather_info, "Night, %.1f°C", current_temp_humidity.temperature);
            }
            gui_weather_set(weather_info);

            xEventGroupClearBits(gui_event_group, GUI_EVT_TEMP_UPDATE);
        }

        // Handle fuel updates
        if(events & GUI_EVT_FUEL_UPDATE) {
            gui_fuel_percentage_set(fuel_percentage);
            xEventGroupClearBits(gui_event_group, GUI_EVT_FUEL_UPDATE);
        }

        // Periodically update time
        static int counter = 0;
        if(++counter >= 10) { // Update time every second
            counter = 0;
            xEventGroupSetBits(gui_event_group, GUI_EVT_TIME_UPDATE);
        }

        // Wait for the next cycle
        vTaskDelayUntil(&last_wake_time, pdMS_TO_TICKS(GUI_CONTROLLER_UPDATE_PERIOD_MS));
    }
}

/**
   * @brief Updates proximity based on parking sensor data
   */
static void update_proximity_from_parking_sensor(void) {
    uint32_t distance;
    esp_err_t err = parking_sensor_get_distance(&distance);

    if(err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get parking sensor distance");
        return;
    }

    movement_direction_t direction = speed_estimator_get_direction();
    bool is_forward                = (direction == DIRECTION_FORWARD);

    // Check if current distance is valid for updating proximity
    if(distance > MAX_DISTANCE) {
        ESP_LOGW(TAG, "Invalid distance reading: %lu", distance);
        return;
    }

    // Determine the appropriate proximity value based on distance and direction
    if(distance < DISTANCE_DANGER) {
        current_proximity = is_forward ? GUI_PROX_FRONT_CLOSE : GUI_PROX_BACK_CLOSE;
    } else if(distance < DISTANCE_WARNING) {
        current_proximity = is_forward ? GUI_PROX_FRONT_MID : GUI_PROX_BACK_MID;
    } else if(distance < DISTANCE_SAFE) {
        current_proximity = is_forward ? GUI_PROX_FRONT_FAR : GUI_PROX_BACK_FAR;
    } else {
        // No proximity detected - this needs to be handled differently
        // since GUI_PROX_NONE is not a valid value in the enum
        current_proximity = GUI_PROX_NUM; // Setting to invalid value that will be caught by validation
        return;                           // Skip setting the event bit for this case
    }

    // Only update the GUI if we have a valid proximity value
    if(current_proximity >= 0 && current_proximity < GUI_PROX_NUM) {
        ESP_LOGI(TAG,
                "Updating proximity: %d (distance: %lu cm, direction: %s)",
                current_proximity,
                distance,
                is_forward ? "forward" : "backward");
        xEventGroupSetBits(gui_event_group, GUI_EVT_PROXIMITY_UPDATE);
    } else {
        ESP_LOGW(TAG, "Invalid proximity value: %d", current_proximity);
    }
}

/**
    * @brief Crash event callback
    */
static void crash_event_callback(crash_event_t *event) {
    crash_detected = true;
    ESP_LOGI(TAG, "Crash detected! Impact force: %.2f g", event->impact_force);

    // You could add warning indicators to the GUI for crashes
    // For example, make speed indicator flash red

    // Auto-reset after a while
    vTaskDelay(pdMS_TO_TICKS(CRASH_RESET_TIMEOUT_MS));
    crash_detector_reset();
    crash_detected = false;
}

/**
    * @brief Door state change callback
    */
static void door_state_callback(door_state_t state) {
    // Mock: Update the driver's door for demonstration
    door_states[front_left] = state; // Use enum from gui.h

    xEventGroupSetBits(gui_event_group, GUI_EVT_DOOR_UPDATE);
}

/**
    * @brief Light state change callback
    */
static void light_state_callback(light_state_t state) {
    current_light_state = state;
    xEventGroupSetBits(gui_event_group, GUI_EVT_LIGHT_UPDATE);

    // Update temperature display which includes light state
    xEventGroupSetBits(gui_event_group, GUI_EVT_TEMP_UPDATE);
}

/**
    * @brief Periodic temperature reading task
    */
static void temp_sensor_task(void *pvParameters) {
    TickType_t last_wake_time = xTaskGetTickCount();

    while(1) {
        // Read temperature and humidity
        esp_err_t err = sht3x_read_measurement(&current_temp_humidity);

        if(err == ESP_OK) {
            ESP_LOGI(TAG,
                    "Temperature: %.2f°C, Humidity: %.2f%%",
                    current_temp_humidity.temperature,
                    current_temp_humidity.humidity);

            xEventGroupSetBits(gui_event_group, GUI_EVT_TEMP_UPDATE);
        } else {
            ESP_LOGE(TAG, "Failed to read SHT3x sensor");
        }

        // Update every 30 seconds
        vTaskDelayUntil(&last_wake_time, pdMS_TO_TICKS(30000));
    }
}

/**
    * @brief Sensor reading function for the speed estimator
    */
static void speed_sensor_task(void *pvParameters) {
    TickType_t last_wake_time = xTaskGetTickCount();

    while(1) {
        // Get speed from the speed estimator
        float speed_kmh = speed_estimator_get_speed_kmh();
        current_speed   = (int) speed_kmh;

        // Set the event bit to update the GUI
        xEventGroupSetBits(gui_event_group, GUI_EVT_SPEED_UPDATE);

        // Also update proximity since direction might have changed
        update_proximity_from_parking_sensor();

        // Update every 200ms
        vTaskDelayUntil(&last_wake_time, pdMS_TO_TICKS(200));
    }
}

/**
   * @brief Task to periodically update proximity readings
   */
static void proximity_sensor_task(void *pvParameters) {
    TickType_t last_wake_time = xTaskGetTickCount();

    while(1) {
        // Update proximity values
        update_proximity_from_parking_sensor();

        // Check proximity every 200ms
        vTaskDelayUntil(&last_wake_time, pdMS_TO_TICKS(200));
    }
}

/**
   * @brief Initialize the GUI controller
   * 
   * This function initializes all sensor modules and registers callbacks
   * for sensor events to update the GUI
   */
esp_err_t gui_controller_init(void) {
    esp_err_t ret = ESP_OK;

    // Initialize event group
    gui_event_group = xEventGroupCreate();
    if(gui_event_group == NULL) {
        ESP_LOGE(TAG, "Failed to create event group");
        return ESP_FAIL;
    }

    // Register callbacks for existing modules
    crash_detector_register_callback(crash_event_callback);
    door_register_callback(door_state_callback);
    light_register_callback(light_state_callback);

    // Create GUI controller task
    BaseType_t task_created = xTaskCreatePinnedToCore(gui_controller_task,
            "gui_controller",
            GUI_CONTROLLER_STACK_SIZE,
            NULL,
            GUI_CONTROLLER_PRIORITY,
            &gui_controller_task_handle,
            0 // Run on Core 0 (GUI runs on Core 1)
    );

    if(task_created != pdPASS) {
        ESP_LOGE(TAG, "Failed to create GUI controller task");
        return ESP_FAIL;
    }

    // Create temperature sensor task
    task_created = xTaskCreate(temp_sensor_task, "temp_sensor", 2048, NULL, 3, NULL);

    if(task_created != pdPASS) {
        ESP_LOGE(TAG, "Failed to create temperature sensor task");
        return ESP_FAIL;
    }

    // Create speed sensor task
    task_created = xTaskCreate(speed_sensor_task, "speed_sensor", 2048, NULL, 4, NULL);

    if(task_created != pdPASS) {
        ESP_LOGE(TAG, "Failed to create speed sensor task");
        return ESP_FAIL;
    }

    // Create proximity sensor task
    task_created = xTaskCreate(proximity_sensor_task, "proximity_sensor", 2048, NULL, 4, NULL);

    if(task_created != pdPASS) {
        ESP_LOGE(TAG, "Failed to create proximity sensor task");
        return ESP_FAIL;
    }

    // Trigger initial updates
    xEventGroupSetBits(gui_event_group, GUI_EVT_SPEED_UPDATE | GUI_EVT_TIME_UPDATE | GUI_EVT_FUEL_UPDATE);

    ESP_LOGI(TAG, "GUI controller initialized successfully");
    return ESP_OK;
}
/**
    * @brief Deinitialize the GUI controller
    */
esp_err_t gui_controller_deinit(void) {
    if(gui_controller_task_handle != NULL) {
        vTaskDelete(gui_controller_task_handle);
        gui_controller_task_handle = NULL;
    }

    if(gui_event_group != NULL) {
        vEventGroupDelete(gui_event_group);
        gui_event_group = NULL;
    }

    return ESP_OK;
}

/**
    * @brief Simulate a fuel level change (for demo purposes)
    * 
    * @param percentage New fuel level (0-100)
    */
void gui_controller_set_fuel(int percentage) {
    if(percentage < 0)
        percentage = 0;
    if(percentage > 100)
        percentage = 100;

    fuel_percentage = percentage;
    xEventGroupSetBits(gui_event_group, GUI_EVT_FUEL_UPDATE);
}