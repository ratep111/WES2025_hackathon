/**
 * @file door_detector.c
 * @brief Door open/closed detection using TCRT5000 IR sensor
 */
#include "door_detector.h"
#include "../infrared-tcrt5000/tcrt5000.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "freertos/queue.h"
#include "driver/gpio.h"

#define TAG "DOOR_DETECTOR"

// Event group bits
#define DOOR_OPEN_BIT   BIT0
#define DOOR_CLOSED_BIT BIT1

#define TCRT5000_DIGITAL_PIN GPIO_NUM_14 // LED_B 13

// Queue size for events
#define DOOR_EVENT_QUEUE_SIZE 10

// Sensor configuration
static tcrt5000_handle_t sensor;
static const tcrt5000_config_t config = { .use_digital = true, .digital_pin = TCRT5000_DIGITAL_PIN, .invert_output = false };

// Internal state
static EventGroupHandle_t door_event_group       = NULL;
static QueueHandle_t door_event_queue            = NULL;
static door_state_t current_door_state           = DOOR_STATE_UNKNOWN;
static void (*door_state_callback)(door_state_t) = NULL;

esp_err_t door_detector_init(void) {
    esp_err_t ret = tcrt5000_init(&config, &sensor);
    if(ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize TCRT5000 sensor: %s", esp_err_to_name(ret));
        return ret;
    }

    if(!door_event_group) {
        door_event_group = xEventGroupCreate();
        if(!door_event_group) {
            ESP_LOGE(TAG, "Failed to create event group");
            return ESP_FAIL;
        }
    }

    if(!door_event_queue) {
        door_event_queue = xQueueCreate(DOOR_EVENT_QUEUE_SIZE, sizeof(door_event_t));
        if(!door_event_queue) {
            ESP_LOGE(TAG, "Failed to create door event queue");
            return ESP_FAIL;
        }
    }

    ESP_LOGI(TAG, "Door detector initialized");
    return ESP_OK;
}

bool is_door_open(void) {
    if(!door_event_group)
        return false;
    return (xEventGroupGetBits(door_event_group) & DOOR_OPEN_BIT);
}

bool is_door_closed(void) {
    if(!door_event_group)
        return false;
    return (xEventGroupGetBits(door_event_group) & DOOR_CLOSED_BIT);
}

door_state_t get_door_state(void) {
    return current_door_state;
}

bool get_door_event(door_event_t *event, TickType_t wait_ticks) {
    if(!door_event_queue || !event)
        return false;
    return xQueueReceive(door_event_queue, event, wait_ticks) == pdTRUE;
}

void door_register_callback(void (*callback)(door_state_t state)) {
    door_state_callback = callback;
}

static void update_door_state(door_state_t new_state) {
    if(new_state == current_door_state)
        return;

    current_door_state = new_state;

    // Event group
    if(new_state == DOOR_STATE_CLOSED) {
        xEventGroupSetBits(door_event_group, DOOR_CLOSED_BIT);
        xEventGroupClearBits(door_event_group, DOOR_OPEN_BIT);
        ESP_LOGI(TAG, "Door CLOSED");
    } else {
        xEventGroupSetBits(door_event_group, DOOR_OPEN_BIT);
        xEventGroupClearBits(door_event_group, DOOR_CLOSED_BIT);
        ESP_LOGI(TAG, "Door OPEN");
    }

    // Queue event
    door_event_t event = { .state = new_state, .timestamp = (uint32_t) (xTaskGetTickCount() * portTICK_PERIOD_MS) };
    if(xQueueSend(door_event_queue, &event, 0) != pdTRUE) {
        ESP_LOGW(TAG, "Door event queue full");
    }

    // Optional user callback
    if(door_state_callback) {
        door_state_callback(new_state);
    }
}

void door_detector_task(void *pvParameters) {
    if(door_detector_init() != ESP_OK) {
        vTaskDelete(NULL);
        return;
    }

    bool detected = false, prev_detected = false;
    int stable_count         = 0;
    const int DEBOUNCE_COUNT = 3;

    while(1) {
        esp_err_t ret = tcrt5000_read_digital(&sensor, &detected);
        if(ret != ESP_OK) {
            ESP_LOGE(TAG, "TCRT5000 read failed: %s", esp_err_to_name(ret));
            vTaskDelay(pdMS_TO_TICKS(100));
            continue;
        }

        if(detected == prev_detected) {
            stable_count++;
        } else {
            stable_count = 0;
        }

        prev_detected = detected;

        if(stable_count >= DEBOUNCE_COUNT) {
            door_state_t new_state = detected ? DOOR_STATE_CLOSED : DOOR_STATE_OPEN;
            update_door_state(new_state);
            stable_count = 0;
        }

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}
