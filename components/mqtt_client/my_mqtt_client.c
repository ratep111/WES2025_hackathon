/**
 * @file my_mqtt_client.c
 * @brief MQTT client module template for publishing sensor data.
 */

//--------------------------------- INCLUDES ----------------------------------
#include "mqtt_client.h"
#include "my_mqtt_client.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"
#include <cJSON.h>
#include "led.h"

//---------------------------------- MACROS -----------------------------------
#define TAG        "MQTT"
#define MQTT_URI   "mqtt://your.broker.address"
#define MQTT_TOPIC "your/topic"

//-------------------------------- DATA TYPES ---------------------------------

//---------------------- PRIVATE FUNCTION PROTOTYPES --------------------------
/**
 * @brief Create a JSON payload from sensor data.
 */
static char *_mqtt_client_create_json_payload(float temperature, float humidity);

/**
 * @brief MQTT event handler.
 */
static void _mqtt_client_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data);

/**
 * @brief Publish temperature and humidity data from the queue.
 */
static void _mqtt_client_temp_task(void *args);

//------------------------- STATIC DATA & CONSTANTS ---------------------------
static esp_mqtt_client_handle_t s_client;
static bool s_mqtt_connected = false;

//------------------------------- GLOBAL DATA ---------------------------------
extern QueueHandle_t temperature_change_queue;

//------------------------------ PUBLIC FUNCTIONS -----------------------------
esp_err_t mqtt_client_init(void) {
    esp_err_t ret = nvs_flash_init();
    if(ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        // NVS partition was truncated and needs to be erased
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    // Initialize with proper MQTT client configuration
    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = MQTT_URI,
    };

    s_client = esp_mqtt_client_init(&mqtt_cfg);
    if(s_client == NULL) {
        ESP_LOGE(TAG, "Failed to initialize MQTT client");
        return ESP_FAIL;
    }

    // Register event handler with correct parameters
    ESP_ERROR_CHECK(esp_mqtt_client_register_event(s_client, ESP_EVENT_ANY_ID, _mqtt_client_event_handler, NULL));
    ESP_ERROR_CHECK(esp_mqtt_client_start(s_client));

    // Create task to handle publishing temperature data
    BaseType_t task_created = xTaskCreate(_mqtt_client_temp_task, "mqtt_temp_task", 4096, NULL, 5, NULL);
    if(task_created != pdPASS) {
        ESP_LOGE(TAG, "Failed to create MQTT temperature task");
        return ESP_FAIL;
    }

    return ESP_OK;
}

bool mqtt_client_is_connected(void) {
    return s_mqtt_connected;
}


//---------------------------- PRIVATE FUNCTIONS ------------------------------
static char *_mqtt_client_create_json_payload(float temperature, float humidity) {
    cJSON *root = cJSON_CreateObject();
    cJSON_AddNumberToObject(root, "temp", temperature);
    cJSON_AddNumberToObject(root, "hum", humidity);
    char *json = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    return json;
}

static void _mqtt_client_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data) {
    esp_mqtt_event_handle_t event = (esp_mqtt_event_handle_t) event_data;

    // Only set the client handle when it's null to avoid overwriting
    if(s_client == NULL) {
        s_client = event->client;
    }

    switch((esp_mqtt_event_id_t) event_id) {
        case MQTT_EVENT_CONNECTED:
            s_mqtt_connected = true;
            ESP_LOGI(TAG, "Connected to MQTT broker");
            break;
        case MQTT_EVENT_DISCONNECTED:
            s_mqtt_connected = false;
            ESP_LOGW(TAG, "Disconnected from MQTT broker");
            break;
        case MQTT_EVENT_DATA:
            // Create null-terminated strings for safer handling
            char *topic = malloc(event->topic_len + 1);
            char *data  = malloc(event->data_len + 1);

            if(topic && data) {
                memcpy(topic, event->topic, event->topic_len);
                memcpy(data, event->data, event->data_len);
                topic[event->topic_len] = '\0';
                data[event->data_len]   = '\0';

                ESP_LOGI(TAG, "Data received: Topic=%s, Data=%s", topic, data);

                // Process the received data here if needed

                free(topic);
                free(data);
            } else {
                ESP_LOGE(TAG, "Failed to allocate memory for MQTT data");
                // Free anything that was successfully allocated
                if(topic)
                    free(topic);
                if(data)
                    free(data);
            }
            break;
        case MQTT_EVENT_ERROR:
            ESP_LOGE(TAG, "MQTT error occurred");
            break;
        default:
            break;
    }
}

static void _mqtt_client_temp_task(void *args) {
    TempHumData sensor_data;

    for(;;) {
        if(xQueueReceive(temperature_change_queue, &sensor_data, portMAX_DELAY) == pdPASS) {
            if(!s_mqtt_connected) {
                ESP_LOGW(TAG, "Cannot publish - not connected to broker");
                continue;
            }

            char *payload = _mqtt_client_create_json_payload(sensor_data.temperature, sensor_data.humidity);
            if(payload) {
                int msg_id = esp_mqtt_client_publish(s_client, MQTT_TOPIC, payload, 0, 1, 0);
                if(msg_id < 0) {
                    ESP_LOGE(TAG, "Failed to publish message");
                } else {
                    ESP_LOGI(TAG, "Message published successfully, ID: %d", msg_id);
                }
                free(payload);
            } else {
                ESP_LOGE(TAG, "Failed to create JSON payload");
            }
        }
    }
}

//---------------------------- INTERRUPT HANDLERS -----------------------------