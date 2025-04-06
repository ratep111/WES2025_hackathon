//--------------------------------- INCLUDES ----------------------------------
#include "my_sntp.h"
#include "esp_log.h"
#include "esp_sntp.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "../eeprom/at24cx_i2c.h"
#include "../rtc-pcf8523t/pcf8523.h"

//---------------------------------- MACROS -----------------------------------
static const char *TAG = "sntp";

#ifndef INET6_ADDRSTRLEN
#define INET6_ADDRSTRLEN 48
#endif

//---------------------- PRIVATE FUNCTION PROTOTYPES --------------------------

static void initialize_time_sync(void);
static void update_time_and_timezone(void);
static void log_current_time(void);
static void setup_sntp(void);

//------------------------- STATIC DATA & CONSTANTS ---------------------------
RTC_DATA_ATTR static int boot_count = 0;
static time_t now;
static struct tm timeinfo;
static char strftime_buf[64];

//------------------------------ PUBLIC FUNCTIONS -----------------------------
void updateTimeTask(void *params) {
    while(1) {

        // Update the time
        time(&now);
        localtime_r(&now, &timeinfo);

        // Adjust for timezone if necessary
        setenv("TZ", "CET-1CEST,M3.5.0,M10.5.0/3", 1);
        tzset();
        localtime_r(&now, &timeinfo);

        timeinfo.tm_hour = timeinfo.tm_hour;
        // Prepare time string

        ESP_LOGI(TAG, "Initializing AT24CX. . .");

        at24cx_i2c_device_register(32, 0x50); // AT24C32 at address 0x50

        struct tm time;
        time.tm_min   = timeinfo.tm_min;
        time.tm_hour  = timeinfo.tm_hour;
        time.tm_mday  = timeinfo.tm_mday;
        time.tm_mon   = timeinfo.tm_mon;
        time.tm_year  = timeinfo.tm_year;
        time.tm_wday  = timeinfo.tm_wday;
        time.tm_yday  = timeinfo.tm_yday;
        time.tm_isdst = timeinfo.tm_isdst;

        write_to_eeprom(&time, sizeof(time));
        pcf8523_set_time(&time);

        struct tm time2;
        pcf8523_get_time(&time2);
        strftime(strftime_buf, sizeof(strftime_buf), "%c", &time2);
        ESP_LOGI(TAG, "Current date/time: %s", strftime_buf);


        // Wait for one minute before updating again
        vTaskDelete(NULL);
    }
}

void time_sync_notification_cb(struct timeval *tv) {
    ESP_LOGI(TAG, "Notification of a time synchronization event");
}

void sntp_app_main(void) {
    ESP_LOGI(TAG, "Boot count: %d", ++boot_count);
    initialize_time_sync();
    update_time_and_timezone();
    log_current_time();
    xTaskCreate(updateTimeTask, "UpdateTimeTask", 4096, NULL, 5, NULL);
}

//---------------------------- PRIVATE FUNCTIONS ------------------------------
static void initialize_time_sync(void) {
    ESP_LOGI(TAG, "Starting time synchronization");
    nvs_flash_init();
    esp_netif_init();
    setup_sntp();

    // Wait for time to be set
    const int retry_count = 15;
    for(int retry = 0; retry < retry_count && sntp_get_sync_status() == SNTP_SYNC_STATUS_RESET; ++retry) {
        ESP_LOGI(TAG, "Waiting for system time to be set... (%d/%d)", retry + 1, retry_count);
        vTaskDelay(2000 / portTICK_PERIOD_MS);
    }
    time(&now);
    localtime_r(&now, &timeinfo);
}

static void update_time_and_timezone(void) {
    ESP_LOGI(TAG, "Updating time and timezone settings");

    // Set timezone to Central European Time with proper format
    // CET-1CEST,M3.5.0,M10.5.0/3 means:
    // - CET with 1 hour offset from UTC
    // - CEST (summer time) starts last Sunday of March
    // - CEST ends last Sunday of October at 3:00
    setenv("TZ", "CET-1CEST,M3.5.0,M10.5.0/3", 1);
    tzset();

    // Get the current time with the timezone applied
    time(&now);
    localtime_r(&now, &timeinfo);
}

static void log_current_time(void) {
    timeinfo.tm_hour = timeinfo.tm_hour;
    strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeinfo);
    ESP_LOGI(TAG, "Current date/time: %s", strftime_buf);
}

static void setup_sntp(void) {
    ESP_LOGI(TAG, "Setting up SNTP");
    esp_sntp_setoperatingmode(ESP_SNTP_OPMODE_POLL);
    esp_sntp_setservername(0, "time.windows.com");
    esp_sntp_setservername(1, "pool.ntp.org");

#if LWIP_DHCP_GET_NTP_SRV && SNTP_MAX_SERVERS > 1
    esp_sntp_servermode_dhcp(1); // Accept NTP offers from DHCP server
#endif

    sntp_set_time_sync_notification_cb(time_sync_notification_cb);
    esp_sntp_init();

    // Log configured NTP servers
    for(uint8_t i = 0; i < SNTP_MAX_SERVERS; ++i) {
        const char *server_name = esp_sntp_getservername(i);
        if(server_name) {
            ESP_LOGI(TAG, "NTP Server %d: %s", i, server_name);
        } else {
            ip_addr_t *ip = (ip_addr_t *) esp_sntp_getserver(i);
            char address_str[INET6_ADDRSTRLEN];
            ipaddr_ntoa_r(ip, address_str, INET6_ADDRSTRLEN);
            ESP_LOGI(TAG, "NTP Server %d: %s", i, address_str);
        }
    }
}