/**
 * @file my_mqtt_client.c
 * @brief MQTT client module template for publishing sensor data.
 */

//--------------------------------- INCLUDES ----------------------------------
#include "mqtt_client.h"
#include "my_mqtt.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"

#include "esp_wifi.h"

#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "esp_event.h"

#include "protocol_examples_common.h"
#include "esp_wifi.h"
#include "../json/cJSON/cJSON.h"
#include "my_sntp.h"
#include "../eeprom/at24cx_i2c.h"
#include "mqtt_client.h"

//---------------------------------- MACROS -----------------------------------
#define TAG        "MQTT"
#define MQTT_URI   "mqtt://192.168.160.50:1883"
#define MQTT_TOPIC "gps/directions"

#define WIFI_SSID   "myssid"
#define WIFI_PASS   "mypassword"
#define MAX_RETRIES 5

//-------------------------------- DATA TYPES ---------------------------------

//---------------------- PRIVATE FUNCTION PROTOTYPES --------------------------
static char *_mqtt_client_create_json_payload(float temperature, float humidity);
static void _mqtt_client_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data);
// static void _mqtt_client_temp_task(void *args);
static esp_err_t wifi_init_sta(void);
static bool wifi_is_connected(void);
static void wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);

//------------------------- STATIC DATA & CONSTANTS ---------------------------
static esp_mqtt_client_handle_t s_client;
static bool s_mqtt_connected = false;

//------------------------------- GLOBAL DATA ---------------------------------
QueueHandle_t temperature_change_queue;

//------------------------------ PUBLIC FUNCTIONS -----------------------------

static esp_err_t wifi_init_sta(void) {

    esp_err_t ret = nvs_flash_init();
    if(ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    /* This helper function configures Wi-Fi or Ethernet, as selected in menuconfig.
     * Read "Establishing Wi-Fi or Ethernet Connection" section in
     * examples/protocols/README.md for more information about this function.
     */
    ESP_ERROR_CHECK(example_connect());
    ESP_LOGI(TAG, "Connected to AP, begin http example");

    return ESP_OK;
}

static bool wifi_is_connected(void) {
    wifi_ap_record_t ap_info;
    return (esp_wifi_sta_get_ap_info(&ap_info) == ESP_OK);
}

static void wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {
    if(event_base == WIFI_EVENT) {
        switch(event_id) {
            case WIFI_EVENT_STA_START:
                ESP_LOGI(TAG, "WiFi station started");
                break;
            case WIFI_EVENT_STA_CONNECTED:
                ESP_LOGI(TAG, "Connected to WiFi AP");
                break;
            case WIFI_EVENT_STA_DISCONNECTED:
                ESP_LOGI(TAG, "Disconnected from WiFi AP");
                // Attempt to reconnect
                esp_wifi_connect();
                break;
        }
    } else if(event_base == IP_EVENT) {
        switch(event_id) {
            case IP_EVENT_STA_GOT_IP:
                ip_event_got_ip_t *event = (ip_event_got_ip_t *) event_data;
                ESP_LOGI(TAG, "Got IP: " IPSTR, IP2STR(&event->ip_info.ip));
                break;
        }
    }
}

esp_err_t mqtt_client_init(void) {
    // Initialize WiFi first
    esp_err_t ret = wifi_init_sta();
    if(ret != ESP_OK) {
        ESP_LOGE(TAG, "WiFi initialization failed");
        return ret;
    }

    // Wait for WiFi connection
    int retries = 0;
    while(retries < MAX_RETRIES) {
        if(wifi_is_connected()) {
            break;
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
        retries++;
        ESP_LOGI(TAG, "Retrying WiFi connection (%d/%d)", retries, MAX_RETRIES);
    }

    if(retries >= MAX_RETRIES) {
        ESP_LOGE(TAG, "Failed to connect to WiFi");
        return ESP_FAIL;
    }

    // Initialize MQTT client
    esp_mqtt_client_config_t mqtt_cfg = {
        .broker = {
            .address.uri = MQTT_URI,
        },
    };

    s_client = esp_mqtt_client_init(&mqtt_cfg);
    if(s_client == NULL) {
        ESP_LOGE(TAG, "Failed to initialize MQTT client");
        return ESP_FAIL;
    }

    // Register event handler
    ESP_ERROR_CHECK(esp_mqtt_client_register_event(s_client, ESP_EVENT_ANY_ID, _mqtt_client_event_handler, NULL));
    ESP_ERROR_CHECK(esp_mqtt_client_start(s_client));

    // Create task to handle publishing temperature data
    // BaseType_t task_created = xTaskCreate(_mqtt_client_temp_task, "mqtt_temp_task", 4096, NULL, 5, NULL);
    // if(task_created != pdPASS) {
    //     ESP_LOGE(TAG, "Failed to create MQTT temperature task");
    //     return ESP_FAIL;
    // }

    sntp_app_main();

    // esp_wifi_stop();


    return ESP_OK;
}

bool mqtt_client_is_connected(void) {
    return s_mqtt_connected;
}

//---------------------------- PRIVATE FUNCTIONS ------------------------------

static void _mqtt_client_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data) {
    esp_mqtt_event_handle_t event = (esp_mqtt_event_handle_t) event_data;

    switch((esp_mqtt_event_id_t) event_id) {
        case MQTT_EVENT_CONNECTED:
            s_mqtt_connected = true;
            ESP_LOGI(TAG, "Connected to MQTT broker");
            esp_mqtt_client_subscribe(s_client, "gps/directions", 1);
            break;
        case MQTT_EVENT_DISCONNECTED:
            s_mqtt_connected = false;
            ESP_LOGW(TAG, "Disconnected from MQTT broker");
            break;
        case MQTT_EVENT_DATA:
            if(event->topic_len > 0 && event->data_len > 0) {
                char *topic = malloc(event->topic_len + 1);
                char *data  = malloc(event->data_len + 1);
                if(topic && data) {
                    memcpy(topic, event->topic, event->topic_len);
                    memcpy(data, event->data, event->data_len);
                    topic[event->topic_len] = '\0';
                    data[event->data_len]   = '\0';
                    ESP_LOGI(TAG, "Data received: Topic=%s, Data=%s", topic, data);
                    free(topic);
                    free(data);
                }
            }
            break;
        case MQTT_EVENT_ERROR:
            ESP_LOGE(TAG, "MQTT error occurred");
            break;
        default:
            break;
    }
}

// static void _mqtt_client_temp_task(void *args) {
//     TempHumData sensor_data;

//     for(;;) {
//         if(xQueueReceive(temperature_change_queue, &sensor_data, portMAX_DELAY) == pdPASS) {
//             // Wait for both WiFi and MQTT connection
//             while(!s_mqtt_connected || !wifi_is_connected()) {
//                 vTaskDelay(pdMS_TO_TICKS(1000));
//             }

//             char *payload = _mqtt_client_create_json_payload(sensor_data.temperature, sensor_data.humidity);
//             if(payload) {
//                 int msg_id = esp_mqtt_client_publish(s_client, MQTT_TOPIC, payload, strlen(payload), 1, 0);
//                 if(msg_id < 0) {
//                     ESP_LOGE(TAG, "Failed to publish message");
//                 } else {
//                     ESP_LOGI(TAG, "Message published successfully, ID: %d", msg_id);
//                 }
//                 free(payload);
//             } else {
//                 ESP_LOGE(TAG, "Failed to create JSON payload");
//             }
//         }
//     }
// }