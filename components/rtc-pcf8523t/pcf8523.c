#include "pcf8523.h"
#include <string.h>

#define PCF8523_REG_CONTROL_1 0x00
#define PCF8523_REG_CONTROL_2 0x01
#define PCF8523_REG_CONTROL_3 0x02
#define PCF8523_REG_SECONDS   0x03
#define PCF8523_REG_MINUTES   0x04
#define PCF8523_REG_HOURS     0x05
#define PCF8523_REG_DAYS      0x06
#define PCF8523_REG_WEEKDAYS  0x07
#define PCF8523_REG_MONTHS    0x08
#define PCF8523_REG_YEARS     0x09
#define PCF8523_SECONDS_OS    (1 << 7)

static i2c_port_t g_port;

static uint8_t bcd2dec(uint8_t val) {
    return (val >> 4) * 10 + (val & 0x0F);
}

static uint8_t dec2bcd(uint8_t val) {
    return ((val / 10) << 4) + (val % 10);
}

esp_err_t pcf8523_init(i2c_port_t port, gpio_num_t sda_gpio, gpio_num_t scl_gpio) {
    g_port = port;

    i2c_config_t conf = { .mode = I2C_MODE_MASTER,
        .sda_io_num             = sda_gpio,
        .sda_pullup_en          = GPIO_PULLUP_ENABLE,
        .scl_io_num             = scl_gpio,
        .scl_pullup_en          = GPIO_PULLUP_ENABLE,
        .master.clk_speed       = I2C_FREQ_HZ,
        .clk_flags              = 0 };
    ESP_ERROR_CHECK(i2c_param_config(port, &conf));
    return i2c_driver_install(port, I2C_MODE_MASTER, 0, 0, 0);
}

esp_err_t pcf8523_set_time(const struct tm *time) {
    if(!time)
        return ESP_ERR_INVALID_ARG;
    uint8_t data[8] = { PCF8523_REG_SECONDS,
        dec2bcd(time->tm_sec),
        dec2bcd(time->tm_min),
        dec2bcd(time->tm_hour),
        dec2bcd(time->tm_mday),
        time->tm_wday,
        dec2bcd(time->tm_mon + 1),
        dec2bcd(time->tm_year - 100) };
    return i2c_master_write_to_device(g_port, PCF8523_I2C_ADDR, data, sizeof(data), pdMS_TO_TICKS(1000));
}

esp_err_t pcf8523_get_time(struct tm *time) {
    if(!time)
        return ESP_ERR_INVALID_ARG;
    uint8_t reg = PCF8523_REG_SECONDS;
    uint8_t data[7];
    esp_err_t res = i2c_master_write_read_device(g_port, PCF8523_I2C_ADDR, &reg, 1, data, 7, pdMS_TO_TICKS(1000));
    if(res != ESP_OK)
        return res;
    if(data[0] & PCF8523_SECONDS_OS)
        return ESP_ERR_INVALID_STATE;
    time->tm_sec   = bcd2dec(data[0] & 0x7F);
    time->tm_min   = bcd2dec(data[1] & 0x7F);
    time->tm_hour  = bcd2dec(data[2] & 0x3F);
    time->tm_mday  = bcd2dec(data[3] & 0x3F);
    time->tm_wday  = data[4] & 0x07;
    time->tm_mon   = bcd2dec(data[5] & 0x1F) - 1;
    time->tm_year  = bcd2dec(data[6]) + 100;
    time->tm_isdst = 0;
    return ESP_OK;
}

esp_err_t pcf8523_check_status(pcf8523_status_t *status) {
    if(!status)
        return ESP_ERR_INVALID_ARG;
    uint8_t reg = PCF8523_REG_SECONDS;
    uint8_t data;
    esp_err_t ret = i2c_master_write_read_device(g_port, PCF8523_I2C_ADDR, &reg, 1, &data, 1, pdMS_TO_TICKS(1000));
    if(ret != ESP_OK) {
        *status = PCF8523_ERROR;
        return ret;
    }
    *status = (data & PCF8523_SECONDS_OS) ? PCF8523_OSCILLATOR_STOPPED : PCF8523_OK;
    return ESP_OK;
}
