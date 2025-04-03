/**
 * @file wifi_prov.c
 *
 * @brief BLE-based WiFi provisioning module using ESP-IDF's provisioning manager.
 */

//--------------------------------- INCLUDES ----------------------------------
#include <string.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/event_groups.h>

#include <esp_system.h>
#include <esp_log.h>
#include <esp_types.h>
#include <esp_event.h>
#include <nvs_flash.h>
#include <esp_wifi.h>

#include <wifi_provisioning/manager.h>
#include <wifi_provisioning/scheme_ble.h>

#include "wifi_prov.h"

#include "qrcode.h"

//---------------------------------- MACROS -----------------------------------
#define TAG                  "wifi_prov"
#define WIFI_CONNECTED_EVENT BIT0

//--------------------------------- TYPEDEFS ----------------------------------

//------------------------- STATIC DATA & CONSTANTS ---------------------------
static EventGroupHandle_t s_wifi_event_group;

//---------------------- PRIVATE FUNCTION PROTOTYPES --------------------------
/**
 * @brief Internal event handler for WiFi and provisioning events.
 *
 * @param[in] arg         Unused.
 * @param[in] event_base  The event base ID.
 * @param[in] event_id    The specific event ID.
 * @param[in] event_data  Event-specific data.
 */
static void _wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);

//------------------------------ PUBLIC FUNCTIONS -----------------------------
esp_err_t wifi_provisioning_init(void) {
    s_wifi_event_group = xEventGroupCreate();
    if(s_wifi_event_group == NULL) {
        ESP_LOGE(TAG, "Failed to create event group");
        return ESP_FAIL;
    }

    esp_err_t ret = nvs_flash_init();
    if(ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &_wifi_event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &_wifi_event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_PROV_EVENT, ESP_EVENT_ANY_ID, &_wifi_event_handler, NULL));

    return ESP_OK;
}

esp_err_t wifi_provisioning_start(void) {
    bool provisioned = false;

    wifi_prov_mgr_config_t config = {
        .scheme               = wifi_prov_scheme_ble,
        .scheme_event_handler = WIFI_PROV_SCHEME_BLE_EVENT_HANDLER_FREE_BTDM,
    };

    ESP_ERROR_CHECK(wifi_prov_mgr_init(config));
    ESP_ERROR_CHECK(wifi_prov_mgr_is_provisioned(&provisioned));

    if(!provisioned) {
        ESP_LOGI(TAG, "Starting provisioning process");

        char service_name[12];
        uint8_t eth_mac[6];
        esp_err_t err = esp_wifi_get_mac(WIFI_IF_STA, eth_mac);
        if(err != ESP_OK) {
            ESP_LOGE(TAG, "Failed to get MAC address: %s", esp_err_to_name(err));
            return err;
        }

        snprintf(service_name, sizeof(service_name), "PROV_%02X%02X%02X", eth_mac[3], eth_mac[4], eth_mac[5]);

        wifi_prov_security_t security     = WIFI_PROV_SECURITY_1;
        const char *pop                   = "abcd1234";
        wifi_prov_security1_params_t sec1 = WIFI_PROV_SECURITY_0;

        ESP_ERROR_CHECK(wifi_prov_mgr_start_provisioning(security, &sec1, service_name, NULL));

        ESP_LOGI(TAG, "Scan QR with ESP BLE Provisioning app");
        char payload[150];
        snprintf(payload, sizeof(payload), "{\"ver\":\"v1\",\"name\":\"%s\",\"pop\":\"%s\",\"transport\":\"ble\"}", service_name, pop);

        esp_qrcode_config_t cfg = ESP_QRCODE_CONFIG_DEFAULT();
        esp_qrcode_generate(&cfg, payload);
    } else {
        ESP_LOGI(TAG, "Device already provisioned, starting WiFi connection");
        wifi_prov_mgr_deinit();
        ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
        ESP_ERROR_CHECK(esp_wifi_start());
    }

    return ESP_OK;
}

void wifi_provisioning_wait(void) {
    if(s_wifi_event_group == NULL) {
        ESP_LOGE(TAG, "Event group not initialized, call wifi_provisioning_init first");
        return;
    }

    ESP_LOGI(TAG, "Waiting for WiFi connection...");
    xEventGroupWaitBits(s_wifi_event_group, WIFI_CONNECTED_EVENT, pdFALSE, pdTRUE, portMAX_DELAY);
    ESP_LOGI(TAG, "WiFi connection established");
}

bool wifi_provisioning_is_connected(void) {
    if(s_wifi_event_group == NULL) {
        ESP_LOGE(TAG, "Event group not initialized, call wifi_provisioning_init first");
        return false;
    }

    EventBits_t bits = xEventGroupGetBits(s_wifi_event_group);
    return (bits & WIFI_CONNECTED_EVENT) != 0;
}

//---------------------------- PRIVATE FUNCTIONS ------------------------------
static void _wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {
    if(event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if(event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        ESP_LOGI(TAG, "WiFi disconnected, attempting to reconnect...");
        esp_wifi_connect();
    } else if(event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *) event_data;
        ESP_LOGI(TAG, "Connected with IP Address: " IPSTR, IP2STR(&event->ip_info.ip));
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_EVENT);
    } else if(event_base == WIFI_PROV_EVENT) {
        switch(event_id) {
            case WIFI_PROV_CRED_RECV:
                ESP_LOGI(TAG, "Credentials received");
                break;
            case WIFI_PROV_CRED_FAIL:
                ESP_LOGE(TAG, "Credentials failed");
                break;
            case WIFI_PROV_CRED_SUCCESS:
                ESP_LOGI(TAG, "Credentials successfully applied");
                break;
            case WIFI_PROV_END:
                ESP_LOGI(TAG, "Provisioning complete, deinitializing manager");
                wifi_prov_mgr_deinit();
                break;
            default:
                break;
        }
    }
}

//---------------------------- INTERRUPT HANDLERS -----------------------------