#include "at24cx_i2c.h"
#include "at24cx_i2c_hal.h"
#include "esp_log.h"

#include "stdio.h"

#define TAG "eeprom"

void at24cx_i2c_device_register(at24cx_dev_t *dev, uint16_t _dev_chip, uint8_t _i2c_addres) {
    dev->dev_chip   = _dev_chip;
    dev->byte_size  = (128 * _dev_chip) - 1;
    dev->i2c_addres = _i2c_addres;
    dev->status     = 0;

    dev->page_write_size = 16;

    if(at24cx_i2c_hal_test(dev->i2c_addres) == AT24CX_OK) {
        dev->status = 1;
    }
    ESP_LOGI(TAG,
            "Device registered. Status: %s, Chip: AT24C%d, Address: 0x%02X, Size: %ld\n",
            dev->status ? "Active" : "Inactive",
            dev->dev_chip,
            dev->i2c_addres,
            dev->byte_size);
}

static at24cx_err_t at24cx_i2c_error_check(at24cx_dev_t *dev, at24cx_writedata_t *dt) {
    if(!dev->status)
        return AT24CX_NOT_DETECTED;
    else if(dt->address > dev->byte_size)
        return AT24CX_INVALID_ADDRESS;
    else
        return AT24CX_OK;
}

at24cx_err_t at24cx_i2c_byte_write(at24cx_dev_t dev, at24cx_writedata_t dt) {
    at24cx_err_t err;
    uint8_t data[3];
    data[0] = 0b10100000;
    data[1] = dt.address & 0xFF;
    data[2] = dt.data;

    err = at24cx_i2c_error_check(&dev, &dt);
    if(err != AT24CX_OK)
        return err;


    err = at24cx_i2c_hal_write(dev.i2c_addres, data, 3);
    at24cx_i2c_hal_ms_delay(AT24CX_WRITE_CYCLE_DELAY);

    return err;
}

at24cx_err_t at24cx_i2c_page_write(at24cx_dev_t dev, at24cx_writedata_t dt) {
    at24cx_err_t err;
    uint8_t data[dev.page_write_size + 2];
    data[0] = dt.address >> 8;
    data[1] = dt.address & 0xFF;

    for(int i = 0; i < dev.page_write_size; i++)
        data[2 + i] = dt.data_multi[i];

    err = at24cx_i2c_error_check(&dev, &dt);
    if(err != AT24CX_OK)
        return err;

    if(((dev.byte_size - dt.address) < dev.page_write_size) || (dt.address) % 128)
        return AT24CX_INVALID_PAGEWRITE_ADDRESS;

    err = at24cx_i2c_hal_write(dev.i2c_addres, data, dev.page_write_size + 2);
    at24cx_i2c_hal_ms_delay(AT24CX_WRITE_CYCLE_DELAY);

    return err;
}

at24cx_err_t at24cx_i2c_byte_read(at24cx_dev_t dev, at24cx_writedata_t *dt) {
    at24cx_err_t err;
    uint8_t reg[2];
    uint8_t data;
    reg[0] = 0b10100000;
    reg[1] = dt->address;

    err = at24cx_i2c_error_check(&dev, dt);
    if(err != AT24CX_OK)
        return err;

    err      = at24cx_i2c_hal_read(dev.i2c_addres, reg, 2, &data, 1);
    dt->data = data;

    return err;
}

at24cx_err_t at24cx_i2c_current_address_read(at24cx_dev_t dev, at24cx_writedata_t *dt) {
    at24cx_err_t err;
    uint8_t data;
    err      = at24cx_i2c_hal_read(dev.i2c_addres, 0, 0, &data, 1);
    dt->data = data;

    return err;
}
