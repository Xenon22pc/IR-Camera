#include <stdint.h>
#include <stdbool.h>
#include "driver/i2c.h"

#define HTPA_DEV_ADR       0x1A
#define HTPA_EEPROM_ADR    0x50
#define I2C_TIMOUT_MS      1000

static int i2c_port = I2C_NUM_0;

int HTPA_I2C_Init(int i2c_num, int sda_pin, int scl_pin, uint32_t clk_speed) {
    int ret = 0;
    i2c_port = i2c_num;
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = sda_pin,
        .sda_pullup_en = GPIO_PULLUP_DISABLE,
        .scl_io_num = scl_pin,
        .scl_pullup_en = GPIO_PULLUP_DISABLE,
        .master.clk_speed = clk_speed,
        conf.clk_flags = I2C_SCLK_SRC_FLAG_FOR_NOMAL
    };

    ret = i2c_driver_install(i2c_port, I2C_MODE_MASTER, 0, 0, 0);
    if (ret != ESP_OK) return ret;
    vTaskDelay(50 / portTICK_PERIOD_MS);

    ret = i2c_param_config(i2c_port, &conf);
    if (ret != ESP_OK) return ret;
    vTaskDelay(50 / portTICK_PERIOD_MS);

    return ret;
}

int HTPA_I2C_DeInit(i2c_port_t i2c_num) {
    return i2c_driver_delete(i2c_num);
}

int HTPA_I2C_Read(uint8_t reg, uint8_t *data, uint16_t len) {
    int ret = 0;
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (HTPA_DEV_ADR << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, reg, true);
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (HTPA_DEV_ADR << 1) | I2C_MASTER_READ, true);
    i2c_master_read(cmd, data, len, I2C_MASTER_LAST_NACK);  
    i2c_master_stop(cmd);

    ret = i2c_master_cmd_begin(i2c_port, cmd, 1000 / portTICK_RATE_MS);
    if (ret != ESP_OK) return ret;

    i2c_cmd_link_delete(cmd);
    return ret;
}

int HTPA_I2C_Write(uint8_t reg, uint8_t *data, uint16_t len) {
    int ret = 0;
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (HTPA_DEV_ADR << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, reg, true);
    i2c_master_write(cmd, data, len, true);
    i2c_master_stop(cmd);

    ret = i2c_master_cmd_begin(i2c_port, cmd, I2C_TIMOUT_MS / portTICK_RATE_MS);
    if (ret != ESP_OK) return ret;

    i2c_cmd_link_delete(cmd);
    vTaskDelay(5 / portTICK_PERIOD_MS);
    return ret;
}

int HTPA_EEPROM_Read(uint16_t addr, uint8_t *data, uint16_t len) {
    int ret = 0;
    uint8_t hi_lo[] = { (uint8_t)(addr >> 8), (uint8_t)addr };
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (HTPA_EEPROM_ADR << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write(cmd, hi_lo, 2, true);
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (HTPA_EEPROM_ADR << 1) | I2C_MASTER_READ, true);
    i2c_master_read(cmd, data, len, I2C_MASTER_LAST_NACK);  
    i2c_master_stop(cmd);

    ret = i2c_master_cmd_begin(i2c_port, cmd, 1000 / portTICK_RATE_MS);
    if (ret != ESP_OK) return ret;

    i2c_cmd_link_delete(cmd);
    return ret;
}
