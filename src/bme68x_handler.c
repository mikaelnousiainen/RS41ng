#include "drivers/bme68x/bme68x.h"
#include <stdint.h>

#include "telemetry.h"
#include "log.h"
#include "hal/i2c.h"
#include "hal/delay.h"

/**
 * Copyright (C) 2024 Bosch Sensortec GmbH. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 * 
 * Modifed by KE5GDB for RS41ng use -- 3/25
 * 
 */

#define BME68X_VALID_DATA  UINT8_C(0xB0)
#define BME68X_DO_NOT_USE_FPU

static uint8_t dev_addr;
static struct bme68x_dev bme;
static struct bme68x_conf conf;
static struct bme68x_heatr_conf heatr_conf;
static bool bme680_initialization_required = true;

BME68X_INTF_RET_TYPE bme68x_i2c_read(uint8_t reg_addr, uint8_t *reg_data, uint32_t len, void *intf_ptr)
{
    if (i2c_read_bytes(&DEFAULT_I2C_PORT, SENSOR_BME68X_I2C_ADDRESS, reg_addr, (uint8_t)len, reg_data) != HAL_OK) {
        return BME68X_E_COM_FAIL;
    }

    return BME68X_INTF_RET_SUCCESS;
}
 
BME68X_INTF_RET_TYPE bme68x_i2c_write(uint8_t reg_addr, const uint8_t *reg_data, uint32_t len, void *intf_ptr)
{
    if (i2c_write_bytes(&DEFAULT_I2C_PORT, SENSOR_BME68X_I2C_ADDRESS, reg_addr, len, (uint8_t *)reg_data) != HAL_OK) {
        return BME68X_E_COM_FAIL;
    }

    return BME68X_INTF_RET_SUCCESS;
}
 
void bme68x_delay_us(uint32_t period, void *intf_ptr)
{
    delay_us(period);
}
 
void bme68x_check_rslt(const char api_name[], int8_t rslt)
{
    switch (rslt)
    {
        case BME68X_OK:
            log_error("API name [%s]  BME68X_OK [%d]\r\n", api_name, rslt);
            /* Do nothing */
            break;
        case BME68X_E_NULL_PTR:
            log_error("API name [%s]  Error [%d] : Null pointer\r\n", api_name, rslt);
            break;
        case BME68X_E_COM_FAIL:
            log_error("API name [%s]  Error [%d] : Communication failure\r\n", api_name, rslt);
            break;
        case BME68X_E_INVALID_LENGTH:
            log_error("API name [%s]  Error [%d] : Incorrect length parameter\r\n", api_name, rslt);
            break;
        case BME68X_E_DEV_NOT_FOUND:
            log_error("API name [%s]  Error [%d] : Device not found\r\n", api_name, rslt);
            break;
        case BME68X_E_SELF_TEST:
            log_error("API name [%s]  Error [%d] : Self test error\r\n", api_name, rslt);
            break;
        case BME68X_W_NO_NEW_DATA:
            log_error("API name [%s]  Warning [%d] : No new data found\r\n", api_name, rslt);
            break;
        default:
            log_error("API name [%s]  Error [%d] : Unknown error code\r\n", api_name, rslt);
            break;
    }
}
 
int8_t bme68x_interface_init(struct bme68x_dev *bme, uint8_t intf)
{
    int8_t rslt = BME68X_OK;

    if (bme != NULL)
    {
        dev_addr = SENSOR_BME68X_I2C_ADDRESS;
        bme->read = bme68x_i2c_read;
        bme->write = bme68x_i2c_write;
        bme->intf = BME68X_I2C_INTF;

        bme->delay_us = bme68x_delay_us;
        bme->intf_ptr = &dev_addr;
        bme->amb_temp = 25; /* The ambient temperature in deg C is used for defining the heater temperature */
    }
    else
    {
        rslt = BME68X_E_NULL_PTR;
    }

    return rslt;
}

bool bme68x_handler_init(void)
{
    int8_t rslt;

    /* Heater temperature in degree Celsius */
    // uint16_t temp_prof[10] = { 320, 100, 100, 100, 200, 200, 200, 320, 320, 320 };
    // uint16_t temp_prof[10] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

    /* Multiplier to the shared heater duration */
    // uint16_t mul_prof[10] = { 5, 2, 10, 30, 5, 5, 5, 5, 5, 5 };

    rslt = bme68x_interface_init(&bme, BME68X_I2C_INTF);
    // bme68x_check_rslt("bme68x_interface_init", rslt);

    rslt = bme68x_init(&bme);
    // bme68x_check_rslt("bme68x_init", rslt);

    /* Check if rslt == BME68X_OK, report or handle if otherwise */
    rslt = bme68x_get_conf(&conf, &bme);
    // bme68x_check_rslt("bme68x_get_conf", rslt);

    /* Check if rslt == BME68X_OK, report or handle if otherwise */
    conf.filter = BME68X_FILTER_OFF;
    conf.odr = BME68X_ODR_NONE;
    conf.os_hum = BME68X_OS_1X;
    conf.os_pres = BME68X_OS_16X;
    conf.os_temp = BME68X_OS_2X;
    rslt = bme68x_set_conf(&conf, &bme);
    // bme68x_check_rslt("bme68x_set_conf", rslt);

    /* Check if rslt == BME68X_OK, report or handle if otherwise */
    // heatr_conf.enable = BME68X_ENABLE;
    heatr_conf.enable = BME68X_DISABLE;
    heatr_conf.heatr_dur = 100;
    heatr_conf.heatr_temp = 300;
    // heatr_conf.heatr_temp_prof = temp_prof;
    // heatr_conf.heatr_dur_prof = mul_prof;

    /* Shared heating duration in milliseconds */
    // heatr_conf.shared_heatr_dur = (uint16_t)(140 - (bme68x_get_meas_dur(BME68X_FORCED_MODE, &conf, &bme) / 1000));

    // heatr_conf.profile_len = 10;
    rslt = bme68x_set_heatr_conf(BME68X_FORCED_MODE, &heatr_conf, &bme);
    // bme68x_check_rslt("bme68x_set_heatr_conf", rslt);

    /* Check if rslt == BME68X_OK, report or handle if otherwise */
    rslt = bme68x_set_op_mode(BME68X_FORCED_MODE, &bme);
    // bme68x_check_rslt("bme68x_set_op_mode", rslt);

    if(rslt == BME68X_OK) {
        bme680_initialization_required = false;
        return true;
    }
    return false;
}

bool bme68x_read(int32_t *temperature_celsius_100, uint32_t *pressure_mbar_100, uint32_t *humidity_percentage_100, uint32_t *bme680_gas_r) {
    uint32_t del_period;
    struct bme68x_data data;
    uint8_t n_fields;

    /* Calculate delay period in microseconds */
    del_period = bme68x_get_meas_dur(BME68X_FORCED_MODE, &conf, &bme) + (heatr_conf.heatr_dur * 1000);
    bme.delay_us(del_period, bme.intf_ptr);

    bme68x_get_data(BME68X_FORCED_MODE, &data, &n_fields, &bme);
    // bme68x_check_rslt("bme68x_get_data", rslt);

    // if(n_fields) {
        log_info("+++++ %d %lu %lu\n", data.temperature, data.pressure, data.humidity / 10);
        *temperature_celsius_100 = data.temperature;
        *pressure_mbar_100 = data.pressure;
        *humidity_percentage_100 = data.humidity / 10;
        *bme680_gas_r = data.gas_resistance;
    // }
    
    // *air_quality_index = data.gas_index;
    
    return true;
}

bool bme68x_read_telemetry(telemetry_data *data)
{
    bool success;

    if (bme680_initialization_required) {
        log_info("BME re-init\n");
        success = bme68x_handler_init();
        log_info("BME re-init: %d\n", success);
        if (!success) {
            data->temperature_celsius_100 = 0;
            data->pressure_mbar_100 = 0;
            data->humidity_percentage_100 = 0;
            return false;
        }
    }

    success = bme68x_read(&data->temperature_celsius_100, &data->pressure_mbar_100, &data->humidity_percentage_100, &data->bme6xx_gas_r);

    if (!success) {
        log_info("BME re-init\n");
        success = bme68x_handler_init();
        log_info("BME re-init: %d\n", success);

        if (success) {
            success = bme68x_read(&data->temperature_celsius_100, &data->pressure_mbar_100, &data->humidity_percentage_100, &data->bme6xx_gas_r);
        }

        if (!success) {
            data->temperature_celsius_100 = 0;
            data->pressure_mbar_100 = 0;
            data->humidity_percentage_100 = 0;
        }
    }

    bme680_initialization_required = !success;

    return success;
}