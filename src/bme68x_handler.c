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
 * Modifed by KE5GDB for RS41ng use -- 7/25
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

    bme68x_interface_init(&bme, BME68X_I2C_INTF);

    bme68x_init(&bme);

    bme68x_get_conf(&conf, &bme);

    conf.filter = BME68X_FILTER_OFF;
    conf.odr = BME68X_ODR_NONE;
    conf.os_hum = BME68X_OS_16X;
    conf.os_pres = BME68X_OS_16X;
    conf.os_temp = BME68X_OS_16X;
    bme68x_set_conf(&conf, &bme);

#if SENSOR_BME_6XX_GAS_MEASUREMENT
    heatr_conf.enable = BME68X_ENABLE;
#else
    heatr_conf.enable = BME68X_DISABLE;
#endif
    heatr_conf.heatr_dur = SENSOR_BME_6XX_GAS_HEATER_DURATION;
    heatr_conf.heatr_temp = SENSOR_BME_6XX_GAS_HEATER_TEMP;
    // heatr_conf.heatr_temp_prof = temp_prof;
    // heatr_conf.heatr_dur_prof = mul_prof;

    /* Shared heating duration in milliseconds */
    // heatr_conf.shared_heatr_dur = (uint16_t)(140 - (bme68x_get_meas_dur(BME68X_FORCED_MODE, &conf, &bme) / 1000));

    // heatr_conf.profile_len = 10;
    bme68x_set_heatr_conf(BME68X_FORCED_MODE, &heatr_conf, &bme);

    rslt = bme68x_set_op_mode(BME68X_FORCED_MODE, &bme);

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

    bme68x_set_op_mode(BME68X_FORCED_MODE, &bme);

    /* Calculate delay period in microseconds */
    del_period = bme68x_get_meas_dur(BME68X_FORCED_MODE, &conf, &bme) + (heatr_conf.heatr_dur * 1000);
    bme.delay_us(del_period, bme.intf_ptr);

    bme68x_get_data(BME68X_FORCED_MODE, &data, &n_fields, &bme);

    log_info("BME68X: Temperature %d, Pressure %lu, Humidity %lu, Gas R %lu, Status 0x%x\n",
            (data.temperature / 100),
            (long unsigned int)data.pressure,
            (long unsigned int)(data.humidity / 1000),
            (long unsigned int)data.gas_resistance,
            data.status);

    *temperature_celsius_100 = data.temperature;
    *pressure_mbar_100 = data.pressure;
    *humidity_percentage_100 = data.humidity / 10;
    *bme680_gas_r = data.gas_resistance;
    
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
            data->bme6xx_gas_r = 0;
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
            data->bme6xx_gas_r = 0;
        }
    }

    bme680_initialization_required = !success;

    return success;
}