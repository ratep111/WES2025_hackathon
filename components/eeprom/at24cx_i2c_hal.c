#include "at24cx_i2c_hal.h"

//Hardware Specific Components
#include "driver/i2c.h"

//I2C User Defines
#define I2C_MASTER_SCL_IO GPIO_NUM_21 /*!< GPIO number used for I2C master clock */
#define I2C_MASTER_SDA_IO GPIO_NUM_22 /*!< GPIO number used for I2C master data  */
#define I2C_MASTER_NUM \
    0 /*!< I2C master i2c port number, the number of i2c peripheral interfaces available will depend on the chip */
#define I2C_MASTER_FREQ_HZ        200000 /*!< I2C master clock frequency */
#define I2C_MASTER_TX_BUF_DISABLE 0      /*!< I2C master doesn't need buffer */
#define I2C_MASTER_RX_BUF_DISABLE 0      /*!< I2C master doesn't need buffer */
#define I2C_MASTER_TIMEOUT_MS     1000

at24cx_err_t at24cx_i2c_hal_init() {
    // at24cx_err_t err = AT24CX_OK;

    // int i2c_master_port = I2C_MASTER_NUM;

    // i2c_config_t conf = {
    //     .mode = I2C_MODE_MASTER,
    //     .sda_io_num = I2C_MASTER_SDA_IO,
    //     .scl_io_num = I2C_MASTER_SCL_IO,
    //     .sda_pullup_en = GPIO_PULLUP_ENABLE,    //Disable this if I2C lines have pull up resistor in place
    //     .scl_pullup_en = GPIO_PULLUP_ENABLE,    //Disable this if I2C lines have pull up resistor in place
    //     .master.clk_speed = I2C_MASTER_FREQ_HZ,
    // };

    // i2c_param_config(i2c_master_port, &conf);
    // ESP_ERROR_CHECK(i2c_driver_install(i2c_master_port, I2C_MODE_MASTER, I2C_MASTER_RX_BUF_DISABLE, I2C_MASTER_TX_BUF_DISABLE, 0) == ESP_FAIL);

    return AT24CX_OK;
}

at24cx_err_t at24cx_i2c_hal_read(uint8_t address, uint8_t *reg, uint16_t reg_count, uint8_t *data, uint16_t data_count) {
    at24cx_err_t err = AT24CX_OK;

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    uint8_t read_addr    = 0b10100001;

    if(reg_count) {
        i2c_master_start(cmd);
        i2c_master_write(cmd, reg, 1, 1);
        i2c_master_write(cmd, reg + 1, 1, 1);
    }

    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, read_addr, 1);
    i2c_master_read(cmd, data, 1, I2C_MASTER_LAST_NACK);
    i2c_master_stop(cmd);

    ESP_ERROR_CHECK(i2c_master_cmd_begin(I2C_NUM_0, cmd, I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS));

    i2c_cmd_link_delete(cmd);


    return err;
}

at24cx_err_t at24cx_i2c_hal_write(uint8_t address, uint8_t *data, uint16_t count) {
    at24cx_err_t err = AT24CX_OK;

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    // i2c_master_write_byte(cmd, (address << 1) | I2C_MASTER_WRITE, 1);
    i2c_master_write(cmd, data, 1, 1);
    i2c_master_write(cmd, data + 1, 1, 1);
    i2c_master_write(cmd, data + 2, 1, 1);
    i2c_master_stop(cmd);
    if(i2c_master_cmd_begin(I2C_NUM_0, cmd, I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS) == ESP_FAIL) {
        err = AT24CX_ERR;
    }
    i2c_cmd_link_delete(cmd);


    return err;
}

at24cx_err_t at24cx_i2c_hal_test(uint8_t address) {
    at24cx_err_t err = AT24CX_OK;

    uint8_t test_data = 1;

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (address << 1) | I2C_MASTER_WRITE, 1);
    i2c_master_write_byte(cmd, test_data, 1);
    i2c_master_stop(cmd);
    err = i2c_master_cmd_begin(I2C_NUM_0, cmd, I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);

    return err;
}

void at24cx_i2c_hal_ms_delay(uint32_t ms) {

    vTaskDelay(pdMS_TO_TICKS(ms));
}