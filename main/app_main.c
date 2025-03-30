/*******************************************************************************/
/*                                  INCLUDES                                   */
/*******************************************************************************/

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2c.h"

#include "gui/gui.h"
#include "sht3x.h"

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

/*******************************************************************************/
/*                          STATIC DATA & CONSTANTS                            */
/*******************************************************************************/

static const i2c_config_t _i2c_config = {
    .mode             = I2C_MODE_MASTER,
    .scl_io_num       = GPIO_NUM_21,
    .sda_io_num       = GPIO_NUM_22,
    .sda_pullup_en    = GPIO_PULLUP_ENABLE,
    .scl_pullup_en    = GPIO_PULLUP_ENABLE,
    .master.clk_speed = I2C_MASTER_FREQ_HZ, // 10 KHz, SHT can go up to 1 MHZ.
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
}

/*******************************************************************************/
/*                             PRIVATE FUNCTIONS                               */
/*******************************************************************************/

static void temp_sens_task(void *args) {
    sht3x_sensors_values_t sensor_values = { 0 };
    TickType_t last_wake_time            = xTaskGetTickCount();

    for(;;) {
        esp_err_t ret = sht3x_read_measurement(&sensor_values);

        if(ret == ESP_OK) {
            ESP_LOGI(TAG, "Temperature: %.1f°C - Humidity: %.1f%%", sensor_values.temperature, sensor_values.humidity);

        } else {
            ESP_LOGE(TAG, "Failed to read sensor measurement! Error: %d", ret);
        }

        vTaskDelayUntil(&last_wake_time, pdMS_TO_TICKS(TEMP_READ_DELAY_MS));
    }

    vTaskDelete(NULL);
}
/*******************************************************************************/
/*                             INTERRUPT HANDLERS                              */
/*******************************************************************************/
