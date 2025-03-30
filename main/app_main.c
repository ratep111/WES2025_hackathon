/*******************************************************************************/
/*                                  INCLUDES                                   */
/*******************************************************************************/

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2c.h"

#include "gui/gui.h"
#include "sht3x.h"
#include "pcf8523.h"

/*******************************************************************************/
/*                                   MACROS                                     */
/*******************************************************************************/

#define TAG                  "MAIN"
#define TEMP_TASK_STACK_SIZE 2048
#define TEMP_TASK_PRIORITY   5
#define TEMP_READ_DELAY_MS   2000

/*******************************************************************************/
/*                                 DATA TYPES                                  */
/*******************************************************************************/

/*******************************************************************************/
/*                         PRIVATE FUNCTION PROTOTYPES                         */
/*******************************************************************************/

static void temp_sens_task(void *args);
static void rtc_task(void *args);

/*******************************************************************************/
/*                          STATIC DATA & CONSTANTS                            */
/*******************************************************************************/

static const i2c_config_t _i2c_config = {
    .mode             = I2C_MODE_MASTER,
    .scl_io_num       = GPIO_NUM_21,
    .sda_io_num       = GPIO_NUM_22,
    .sda_pullup_en    = GPIO_PULLUP_ENABLE,
    .scl_pullup_en    = GPIO_PULLUP_ENABLE,
    .master.clk_speed = 400000, // 10 KHz, SHT can go up to 1 MHZ.
    .clk_flags        = 0,
};

/*******************************************************************************/
/*                                 GLOBAL DATA                                 */
/*******************************************************************************/

/*******************************************************************************/
/*                              PUBLIC FUNCTIONS                               */
/*******************************************************************************/
void app_main() {
    ESP_LOGI(TAG, "Initializing I2C master...");
    ESP_ERROR_CHECK(i2c_param_config(I2C_MASTER_NUM, &_i2c_config));
    ESP_ERROR_CHECK(
            i2c_driver_install(I2C_MASTER_NUM, _i2c_config.mode, I2C_MASTER_RX_BUF_DISABLE, I2C_MASTER_TX_BUF_DISABLE, 0));

    if(sht3x_start_periodic_measurement() != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start SHT3x periodic measurement!");
        return;
    }

    gui_init();

    ESP_LOGI(TAG, "Creating temperature sensor task...");
    xTaskCreate(temp_sens_task, "temp_sens_task", TEMP_TASK_STACK_SIZE, NULL, TEMP_TASK_PRIORITY, NULL);
    xTaskCreate(rtc_task, "rtc_task", TEMP_TASK_STACK_SIZE, NULL, TEMP_TASK_PRIORITY, NULL);
}

/*******************************************************************************/
/*                             PRIVATE FUNCTIONS                               */
/*******************************************************************************/

static void temp_sens_task(void *args) {
    sht3x_sensors_values_t sensor_values = { 0 };
    TickType_t last_wake_time            = xTaskGetTickCount();

    ESP_LOGI(TAG, "Start temp task.");
    for(;;) {
        esp_err_t ret = sht3x_read_measurement(&sensor_values);

        if(ret == ESP_OK) {
            ESP_LOGI(TAG, "Temperature: %.1f°C - Humidity: %.1f%%", sensor_values.temperature, sensor_values.humidity);

        } else {
            ESP_LOGE(TAG, "Failed to read sensor measurement! Error: %d", ret);
        }

        vTaskDelayUntil(&last_wake_time, TEMP_READ_DELAY_MS / portTICK_PERIOD_MS);
    }

    vTaskDelete(NULL);
}

static void rtc_task(void *args) {
    struct tm time            = { 0 };
    TickType_t last_wake_time = xTaskGetTickCount();

    esp_err_t ret = pcf8523_init(I2C_NUM_0, GPIO_NUM_22, GPIO_NUM_21);
    if(ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize PCF8523 RTC: %s", esp_err_to_name(ret));
        vTaskDelete(NULL);
    }

    pcf8523_set_time(&time);

    ESP_LOGI(TAG, "RTC task started successfully");
    for(;;) {
        ret = pcf8523_get_time(&time);
        if(ret == ESP_OK) {
            ESP_LOGI(TAG,
                    "Current time: %02d:%02d:%02d %02d-%02d-%04d",
                    time.tm_hour,
                    time.tm_min,
                    time.tm_sec,
                    time.tm_mday,
                    time.tm_mon + 1,
                    time.tm_year + 1900);
        } else {
            ESP_LOGE(TAG, "Failed to get RTC time: %s", esp_err_to_name(ret));
        }

        vTaskDelayUntil(&last_wake_time, 2000 / portTICK_PERIOD_MS);
    }
}

/*******************************************************************************/
/*                             INTERRUPT HANDLERS                              */
/*******************************************************************************/
