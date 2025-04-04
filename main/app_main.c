/*******************************************************************************/
/*                                  INCLUDES                                   */
/*******************************************************************************/

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/event_groups.h"
#include "driver/i2c.h"

#include "gui/gui.h"
#include "LIS2DH12TR.h"
#include "sht3x.h"
#include "pcf8523.h"
#include "button.h"
#include "at24cx_i2c.h"
#include "joystick.h"
#include "led.h"
#include "buzzer.h"
#include "esp32_perfmon.h"
#include "wifi_prov.h"
#include "my_mqtt_client.h"

/*******************************************************************************/
/*                                   MACROS                                     */
/*******************************************************************************/

#define TAG "MAIN"

// Stack sizes for different tasks
#define MINIMAL_STACK_SIZE 2048
#define NORMAL_STACK_SIZE  3072
#define LARGE_STACK_SIZE   4096

// Task priorities (higher number = higher priority)
// Use a range of 1-10 where:
// - 1-3: Background, non-time-critical tasks
// - 4-7: Normal operational tasks
// - 8-10: High-priority, time-sensitive tasks
#define PRIORITY_LOW      3
#define PRIORITY_MEDIUM   5
#define PRIORITY_HIGH     7
#define PRIORITY_CRITICAL 9

// Task-specific priorities based on their requirements
#define ACCEL_TASK_PRIORITY    PRIORITY_MEDIUM // Medium rate sensor sampling
#define TEMP_TASK_PRIORITY     PRIORITY_MEDIUM // Medium rate sensor sampling
#define RTC_TASK_PRIORITY      PRIORITY_LOW    // Time display not time-critical
#define BUTTON_TASK_PRIORITY   PRIORITY_HIGH   // User input should be responsive
#define EEPROM_TASK_PRIORITY   PRIORITY_LOW    // Storage operations can be background
#define JOYSTICK_TASK_PRIORITY PRIORITY_HIGH   // User input should be responsive
#define LED_TASK_PRIORITY      PRIORITY_LOW    // Visual feedback not time-critical
#define WIFI_TASK_PRIORITY     PRIORITY_MEDIUM // Network operations are important but not critical
#define MQTT_TASK_PRIORITY     PRIORITY_MEDIUM // Data publishing is important but not critical
#define BUZZER_TASK_PRIORITY   PRIORITY_MEDIUM // Audio feedback is important but not critical

// Task delays (in milliseconds)
#define TEMP_READ_DELAY_MS      2000
#define RTC_READ_DELAY_MS       2000
#define ACCEL_READ_DELAY_MS     1000
#define MQTT_CHECK_DELAY_MS     5000
#define JOYSTICK_CHECK_DELAY_MS 200 // More responsive for better UX
#define LED_CYCLE_DELAY_MS      1000

// Event group bits for peripheral initialization
#define INIT_I2C_DONE_BIT       (1 << 0)
#define INIT_GUI_DONE_BIT       (1 << 1)
#define INIT_ACCEL_DONE_BIT     (1 << 2)
#define INIT_TEMP_SENS_DONE_BIT (1 << 3)
#define INIT_RTC_DONE_BIT       (1 << 4)
#define INIT_BUTTON_DONE_BIT    (1 << 5)
#define INIT_EEPROM_DONE_BIT    (1 << 6)
#define INIT_JOYSTICK_DONE_BIT  (1 << 7)
#define INIT_BUZZER_DONE_BIT    (1 << 8)
#define INIT_LED_DONE_BIT       (1 << 9)
#define INIT_WIFI_DONE_BIT      (1 << 10)
#define INIT_MQTT_DONE_BIT      (1 << 11)

#define ALL_PERIPHERALS_INIT                                                                                   \
    (INIT_I2C_DONE_BIT | INIT_GUI_DONE_BIT | INIT_ACCEL_DONE_BIT | INIT_TEMP_SENS_DONE_BIT | INIT_RTC_DONE_BIT \
            | INIT_BUTTON_DONE_BIT | INIT_EEPROM_DONE_BIT | INIT_JOYSTICK_DONE_BIT | INIT_BUZZER_DONE_BIT | INIT_LED_DONE_BIT)

/*******************************************************************************/
/*                                 DATA TYPES                                  */
/*******************************************************************************/

/*******************************************************************************/
/*                         PRIVATE FUNCTION PROTOTYPES                         */
/*******************************************************************************/

// Create-all tasks function
static void create_system_tasks(void);

// Initialization functions
static esp_err_t init_i2c(void);
static esp_err_t init_gui(void);
static esp_err_t init_accelerometer(void);
static esp_err_t init_temp_sensor(void);
static esp_err_t init_rtc(void);
static esp_err_t init_buttons(void);
static esp_err_t init_eeprom(void);
static esp_err_t init_joystick(void);
static esp_err_t init_buzzer(void);
static esp_err_t init_leds(void);
static esp_err_t init_wifi(void);
static esp_err_t init_mqtt(void);

// Task functions
static void accelerometer_task(void *args);
static void temp_sens_task(void *args);
static void rtc_task(void *args);
static void eeprom_task(void *args);
static void joystick_task(void *args);
static void buzzer_task(void *args);
static void led_task(void *args);
static void mqtt_task(void *args);

// Button callbacks
static void button1_pressed(void *args);
static void button2_pressed(void *args);
static void button3_pressed(void *args);
static void button4_pressed(void *args);

/*******************************************************************************/
/*                          STATIC DATA & CONSTANTS                            */
/*******************************************************************************/

static const i2c_config_t _i2c_config = {
    .mode             = I2C_MODE_MASTER,
    .scl_io_num       = GPIO_NUM_21,
    .sda_io_num       = GPIO_NUM_22,
    .sda_pullup_en    = GPIO_PULLUP_ENABLE,
    .scl_pullup_en    = GPIO_PULLUP_ENABLE,
    .master.clk_speed = 200000,
    .clk_flags        = 0,
};

// Event group for tracking initialization
static EventGroupHandle_t init_event_group;

/*******************************************************************************/
/*                                 GLOBAL DATA                                 */
/*******************************************************************************/

// Queue for sending sensor data to MQTT
QueueHandle_t temperature_change_queue;

/*******************************************************************************/
/*                              PUBLIC FUNCTIONS                               */
/*******************************************************************************/
void app_main() {
    esp_err_t ret;

    // Create initialization event group
    init_event_group = xEventGroupCreate();
    if(init_event_group == NULL) {
        ESP_LOGE(TAG, "Failed to create init event group!");
        return;
    }

    // Create temperature queue
    temperature_change_queue = xQueueCreate(10, sizeof(TempHumData));
    if(temperature_change_queue == NULL) {
        ESP_LOGE(TAG, "Failed to create temperature queue");
        return;
    }

    // Initialize all peripherals in the correct order
    ESP_LOGI(TAG, "Starting peripheral initialization");

    // 1. Initialize I2C bus (required by many peripherals)
    ret = init_i2c();
    if(ret != ESP_OK) {
        ESP_LOGE(TAG, "I2C initialization failed: %s", esp_err_to_name(ret));
        return;
    }

    // 2. Initialize GUI (required by accelerometer due to SPI)
    ret = init_gui();
    if(ret != ESP_OK) {
        ESP_LOGE(TAG, "GUI initialization failed: %s", esp_err_to_name(ret));
        return;
    }

    // 3. Initialize all other peripherals
    // We can initialize these in parallel since their dependencies are satisfied

    // Sensor initializations
    ret = init_accelerometer();
    if(ret != ESP_OK) {
        ESP_LOGW(TAG, "Accelerometer initialization failed: %s", esp_err_to_name(ret));
        // Continue anyway - non-critical component
    }

    ret = init_temp_sensor();
    if(ret != ESP_OK) {
        ESP_LOGW(TAG, "Temperature sensor initialization failed: %s", esp_err_to_name(ret));
    }

    ret = init_rtc();
    if(ret != ESP_OK) {
        ESP_LOGW(TAG, "RTC initialization failed: %s", esp_err_to_name(ret));
    }

    // Initialize input devices
    ret = init_buttons();
    if(ret != ESP_OK) {
        ESP_LOGW(TAG, "Button initialization failed: %s", esp_err_to_name(ret));
    }

    ret = init_joystick();
    if(ret != ESP_OK) {
        ESP_LOGW(TAG, "Joystick initialization failed: %s", esp_err_to_name(ret));
    }

    // Initialize output devices
    ret = init_buzzer();
    if(ret != ESP_OK) {
        ESP_LOGW(TAG, "Buzzer initialization failed: %s", esp_err_to_name(ret));
    }

    ret = init_leds();
    if(ret != ESP_OK) {
        ESP_LOGW(TAG, "LED initialization failed: %s", esp_err_to_name(ret));
    }

    // Initialize storage
    ret = init_eeprom();
    if(ret != ESP_OK) {
        ESP_LOGW(TAG, "EEPROM initialization failed: %s", esp_err_to_name(ret));
    }

    // Performance monitoring
    perfmon_start();

    // Start Wi-Fi
    ret = init_wifi();
    if(ret != ESP_OK) {
        ESP_LOGE(TAG, "Wi-Fi initialization failed: %s", esp_err_to_name(ret));
        // Wi-Fi is required for MQTT, but continue anyway to allow local functionality
    }

    // Initialize MQTT
    ret = init_mqtt();
    if(ret != ESP_OK) {
        ESP_LOGW(TAG, "MQTT initialization failed: %s", esp_err_to_name(ret));
    }

    // Wait for all critical peripherals
    EventBits_t bits = xEventGroupWaitBits(init_event_group,
            INIT_I2C_DONE_BIT | INIT_GUI_DONE_BIT, // Only wait for critical components
            pdFALSE,                               // Don't clear bits
            pdTRUE,                                // Wait for all bits
            portMAX_DELAY                          // Wait indefinitely
    );

    ESP_LOGI(TAG, "Critical peripherals initialized. Starting tasks...");

    // Create all tasks
    create_system_tasks();
    ESP_LOGI(TAG, "System initialization complete!");
}

// Create all tasks with appropriate priorities and stack sizes
static void create_system_tasks(void) {
    // User input tasks (higher priority)
    //xTaskCreate(button_handler_task, "button_task", MINIMAL_STACK_SIZE, NULL, BUTTON_TASK_PRIORITY, NULL);
    xTaskCreate(joystick_task, "joystick_task", MINIMAL_STACK_SIZE, NULL, JOYSTICK_TASK_PRIORITY, NULL);

    // Sensor tasks (medium priority)
    xTaskCreate(accelerometer_task, "accel_task", NORMAL_STACK_SIZE, NULL, ACCEL_TASK_PRIORITY, NULL);
    xTaskCreate(temp_sens_task, "temp_task", NORMAL_STACK_SIZE, NULL, TEMP_TASK_PRIORITY, NULL);

    // Time-keeping tasks (low priority)
    xTaskCreate(rtc_task, "rtc_task", MINIMAL_STACK_SIZE, NULL, RTC_TASK_PRIORITY, NULL);

    // Output tasks (variable priority)
    xTaskCreate(led_task, "led_task", MINIMAL_STACK_SIZE, NULL, LED_TASK_PRIORITY, NULL);
    xTaskCreate(buzzer_task, "buzzer_task", MINIMAL_STACK_SIZE, NULL, BUZZER_TASK_PRIORITY, NULL);

    // Storage tasks (low priority, run once)
    xTaskCreate(eeprom_task, "eeprom_task", NORMAL_STACK_SIZE, NULL, EEPROM_TASK_PRIORITY, NULL);

    // Communication tasks (medium priority)
    xTaskCreate(mqtt_task, "mqtt_task", LARGE_STACK_SIZE, NULL, MQTT_TASK_PRIORITY, NULL);

    ESP_LOGI(TAG, "All tasks created with appropriate priorities");
}

/*******************************************************************************/
/*                         INITIALIZATION FUNCTIONS                            */
/*******************************************************************************/

static esp_err_t init_i2c(void) {
    ESP_LOGI(TAG, "Initializing I2C master...");
    esp_err_t ret = i2c_param_config(I2C_MASTER_NUM, &_i2c_config);
    if(ret != ESP_OK)
        return ret;

    ret = i2c_driver_install(I2C_MASTER_NUM, _i2c_config.mode, I2C_MASTER_RX_BUF_DISABLE, I2C_MASTER_TX_BUF_DISABLE, 0);
    if(ret != ESP_OK)
        return ret;

    xEventGroupSetBits(init_event_group, INIT_I2C_DONE_BIT);
    return ESP_OK;
}

static esp_err_t init_gui(void) {
    ESP_LOGI(TAG, "Initializing GUI...");
    gui_init();
    // Assuming gui_init doesn't return errors
    xEventGroupSetBits(init_event_group, INIT_GUI_DONE_BIT);
    return ESP_OK;
}

static esp_err_t init_accelerometer(void) {
    ESP_LOGI(TAG, "Initializing accelerometer...");
    // Check if GUI is initialized (required for SPI)
    EventBits_t bits = xEventGroupGetBits(init_event_group);
    if(!(bits & INIT_GUI_DONE_BIT)) {
        ESP_LOGE(TAG, "Cannot initialize accelerometer: GUI not initialized");
        return ESP_ERR_INVALID_STATE;
    }

    // Initialize accelerometer
    LIS2DH12TR_init();
    // Assuming initialization doesn't return errors
    xEventGroupSetBits(init_event_group, INIT_ACCEL_DONE_BIT);
    return ESP_OK;
}

static esp_err_t init_temp_sensor(void) {
    ESP_LOGI(TAG, "Initializing temperature sensor...");
    esp_err_t ret = sht3x_start_periodic_measurement();
    if(ret != ESP_OK)
        return ret;

    xEventGroupSetBits(init_event_group, INIT_TEMP_SENS_DONE_BIT);
    return ESP_OK;
}

static esp_err_t init_rtc(void) {
    ESP_LOGI(TAG, "Initializing RTC...");
    // Note: We use I2C_NUM_0 here which should be I2C_MASTER_NUM for consistency
    esp_err_t ret = pcf8523_init(I2C_NUM_0, GPIO_NUM_22, GPIO_NUM_21);
    if(ret != ESP_OK)
        return ret;

    struct tm time = { 0 }; // Default time
    ret            = pcf8523_set_time(&time);
    if(ret != ESP_OK)
        return ret;

    xEventGroupSetBits(init_event_group, INIT_RTC_DONE_BIT);
    return ESP_OK;
}

static esp_err_t init_buttons(void) {
    ESP_LOGI(TAG, "Initializing buttons...");
    // Create buttons with callbacks
    button_create(BUTTON_1, button1_pressed);
    button_create(BUTTON_2, button2_pressed);
    button_create(BUTTON_3, button3_pressed);
    button_create(BUTTON_4, button4_pressed);

    xEventGroupSetBits(init_event_group, INIT_BUTTON_DONE_BIT);
    return ESP_OK;
}

static esp_err_t init_eeprom(void) {
#define AT24CX_SIZE 256
    ESP_LOGI(TAG, "Initializing EEPROM...");

    at24cx_i2c_hal_init();

    // Note: actual device registration and testing will be done in the eeprom_task
    xEventGroupSetBits(init_event_group, INIT_EEPROM_DONE_BIT);
    return ESP_OK;
}

static esp_err_t init_joystick(void) {
    ESP_LOGI(TAG, "Initializing joystick...");
    esp_err_t ret = joystick_init();
    if(ret != ESP_OK)
        return ret;

    xEventGroupSetBits(init_event_group, INIT_JOYSTICK_DONE_BIT);
    return ESP_OK;
}

static esp_err_t init_buzzer(void) {
    ESP_LOGI(TAG, "Initializing buzzer...");
    buzzer_init();

    // Add a short delay to prevent double trigger (as noted in your code)
    vTaskDelay(500 / portTICK_PERIOD_MS);

    xEventGroupSetBits(init_event_group, INIT_BUZZER_DONE_BIT);
    return ESP_OK;
}

static esp_err_t init_leds(void) {
    ESP_LOGI(TAG, "Initializing LEDs...");
    led_t leds[] = { LED_RED, LED_GREEN, LED_BLUE };

    for(int i = 0; i < LED_COUNT; i++) {
        esp_err_t ret = led_init(leds[i]);
        if(ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to initialize LED %d", i);
            return ret;
        }
    }

    xEventGroupSetBits(init_event_group, INIT_LED_DONE_BIT);
    return ESP_OK;
}

static esp_err_t init_wifi(void) {
    ESP_LOGI(TAG, "Starting Wi-Fi provisioning...");
    esp_err_t ret = wifi_provisioning_start();
    if(ret != ESP_OK)
        return ret;

    xEventGroupSetBits(init_event_group, INIT_WIFI_DONE_BIT);
    return ESP_OK;
}

static esp_err_t init_mqtt(void) {
    ESP_LOGI(TAG, "Initializing MQTT client...");
    esp_err_t ret = mqtt_client_init();
    if(ret != ESP_OK)
        return ret;

    xEventGroupSetBits(init_event_group, INIT_MQTT_DONE_BIT);
    return ESP_OK;
}

/*******************************************************************************/
/*                             TASK FUNCTIONS                                  */
/*******************************************************************************/

static void accelerometer_task(void *args) {
    LIS2DH12TR_accelerations acc = { 0 };

    ESP_LOGI(TAG, "Accelerometer task started");

    while(1) {
        LIS2DH12TR_read_acc(&acc);
        ESP_LOGI(TAG, "x: %f, y: %f, z: %f", acc.x_acc, acc.y_acc, acc.z_acc);
        vTaskDelay(ACCEL_READ_DELAY_MS / portTICK_PERIOD_MS);
    }
}

static void temp_sens_task(void *args) {
    sht3x_sensors_values_t sensor_values = { 0 };
    TempHumData mqtt_data                = { 0 };
    TickType_t last_wake_time            = xTaskGetTickCount();

    ESP_LOGI(TAG, "Temperature sensor task started");

    for(;;) {
        esp_err_t ret = sht3x_read_measurement(&sensor_values);

        if(ret == ESP_OK) {
            ESP_LOGI(TAG, "Temperature: %.1f°C - Humidity: %.1f%%", sensor_values.temperature, sensor_values.humidity);

            // Send data to MQTT queue
            mqtt_data.temperature = sensor_values.temperature;
            mqtt_data.humidity    = sensor_values.humidity;

            if(xQueueSend(temperature_change_queue, &mqtt_data, 0) != pdPASS) {
                ESP_LOGW(TAG, "Temperature queue full, discarding data");
            }
        } else {
            ESP_LOGE(TAG, "Failed to read sensor measurement! Error: %d", ret);
        }

        vTaskDelayUntil(&last_wake_time, TEMP_READ_DELAY_MS / portTICK_PERIOD_MS);
    }
}

static void rtc_task(void *args) {
    struct tm time            = { 0 };
    TickType_t last_wake_time = xTaskGetTickCount();

    ESP_LOGI(TAG, "RTC task started");

    for(;;) {
        esp_err_t ret = pcf8523_get_time(&time);
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

static void eeprom_task(void *args) {
#define AT24CX_SIZE 256

    at24cx_dev_t eeprom_1;

    ESP_LOGI(TAG, "EEPROM task started");

    //Register device
    at24cx_i2c_device_register(&eeprom_1, AT24CX_SIZE, I2C_ADDRESS_AT24CX);

    //Check if eeprom_1 is active
    ESP_LOGI(TAG, "eeprom_1 is %s", eeprom_1.status ? "detected" : "not detected");

    ESP_LOGI(TAG, "Write byte demo: This will write value 0 at address 0x00, 1 at address 0x01 and so on");
    for(int i = 0; i < 10; i++) {
        at24cx_writedata_t dt = { .address = i, .data = i };

        if(at24cx_i2c_byte_write(eeprom_1, dt) == AT24CX_OK)
            ESP_LOGI(TAG, "Writing at address 0x%02X: %d", dt.address, dt.data);
        else
            ESP_LOGE(TAG, "Device write error!");
    }

    ESP_LOGI(TAG, "Read byte demo: Obtain values from addresses 0x00 to 0x09, values should be from 0 to 9 respectively");
    for(int i = 0; i < 10; i++) {
        at24cx_writedata_t dt = {
            .address = i,
        };
        if(at24cx_i2c_byte_read(eeprom_1, &dt) == AT24CX_OK)
            ESP_LOGI(TAG, "Reading at address 0x%02X: %d", dt.address, dt.data);
        else
            ESP_LOGE(TAG, "Device read error!");
    }

    vTaskDelete(NULL);
}

static void joystick_task(void *args) {
    ESP_LOGI(TAG, "Joystick task started");

    while(1) {
        enum joystick_pos_t pos = joystick_get_position();
        (void) pos;
        // Position is already logged by the driver if logging is enabled
        vTaskDelay(JOYSTICK_CHECK_DELAY_MS / portTICK_PERIOD_MS);
    }
}

static void buzzer_task(void *args) {
    buzzer_set_duty(1000);

    vTaskDelay(500 / portTICK_PERIOD_MS);

    buzzer_set_duty(0);

    vTaskDelete(NULL);
}

static void led_task(void *args) {
    ESP_LOGI(TAG, "LED task started");

    while(1) {
        led_on(LED_RED);
        vTaskDelay(LED_CYCLE_DELAY_MS / portTICK_PERIOD_MS);
        led_off(LED_RED);

        led_on(LED_GREEN);
        vTaskDelay(LED_CYCLE_DELAY_MS / portTICK_PERIOD_MS);
        led_off(LED_GREEN);

        led_on(LED_BLUE);
        vTaskDelay(LED_CYCLE_DELAY_MS / portTICK_PERIOD_MS);
        led_off(LED_BLUE);
    }
}

static void mqtt_task(void *args) {
    ESP_LOGI(TAG, "MQTT task started");

    // Visual indicator that MQTT is active
    led_t status_led = LED_GREEN;

    // Check MQTT connection status
    while(1) {
        if(mqtt_client_is_connected()) {
            // Briefly flash the LED to show connected status
            led_on(status_led);
            vTaskDelay(50 / portTICK_PERIOD_MS);
            led_off(status_led);
        } else {
            ESP_LOGW(TAG, "MQTT client not connected");

            // Double flash to indicate disconnected status
            for(int i = 0; i < 2; i++) {
                led_on(status_led);
                vTaskDelay(100 / portTICK_PERIOD_MS);
                led_off(status_led);
                vTaskDelay(100 / portTICK_PERIOD_MS);
            }
        }

        // Check status every 5 seconds
        vTaskDelay(MQTT_CHECK_DELAY_MS / portTICK_PERIOD_MS);
    }
}

/*******************************************************************************/
/*                             CALLBACK FUNCTIONS                               */
/*******************************************************************************/

static void button1_pressed(void *args) {
    ESP_LOGI(TAG, "Button 1 pressed");
}

static void button2_pressed(void *args) {
    ESP_LOGI(TAG, "Button 2 pressed");
}

static void button3_pressed(void *args) {
    ESP_LOGI(TAG, "Button 3 pressed");
}

static void button4_pressed(void *args) {
    ESP_LOGI(TAG, "Button 4 pressed");
}

/*******************************************************************************/
/*                             INTERRUPT HANDLERS                              */
/*******************************************************************************/