#include "drivers/bmp280/bmp280.h"
#include "bmp280_handler.h"
#include "log.h"

bmp280 bmp280_dev;

static bool bmp280_initialization_required = true;

bool bmp280_handler_init()
{
    bmp280_dev.port = &DEFAULT_I2C_PORT;
    bmp280_dev.addr = SENSOR_BMP280_I2C_ADDRESS;

    bmp280_params_t bmp280_params = {
            .mode = BMP280_MODE_NORMAL,
            .filter = BMP280_FILTER_16,
            .oversampling_pressure = BMP280_ULTRA_HIGH_RES,
            .oversampling_temperature = BMP280_ULTRA_HIGH_RES,
            .oversampling_humidity = BMP280_ULTRA_HIGH_RES,
            .standby = BMP280_STANDBY_250,
    };

    bool success = bmp280_init(&bmp280_dev, &bmp280_params);
    bmp280_initialization_required = !success;
    return success;
}

bool bmp280_read(int32_t *temperature_celsius_100, uint32_t *pressure_mbar_100, uint32_t *humidity_percentage_100)
{
    int32_t temperature_raw;
    uint32_t pressure_raw;
    uint32_t humidity_raw;

    bool success = bmp280_read_fixed(&bmp280_dev, &temperature_raw, &pressure_raw, &humidity_raw);
    if (!success) {
        log_error("BMP280 read failed\n");
        return false;
    }

    if (temperature_celsius_100) {
        *temperature_celsius_100 = temperature_raw;
    }
    if (pressure_mbar_100) {
        // Pressure unit is Pascal (= mbar * 100) * 256
        *pressure_mbar_100 = (uint32_t) (((float) pressure_raw) / 256.0f);
    }
    // NOTE: Only BME280 sensors will report humidity. For BMP280 humidity readings will be zero.
    if (humidity_percentage_100) {
        *humidity_percentage_100 = (uint32_t) (((float) humidity_raw) * 100.0f / 1024.0f);
    }

    return true;
}

bool bmp280_read_telemetry(telemetry_data *data)
{
    bool success;

    if (bmp280_initialization_required) {
        log_info("BMP280 re-init\n");
        success = bmp280_handler_init();
        log_info("BMP280 re-init: %d\n", success);
        if (!success) {
            data->temperature_celsius_100 = 0;
            data->pressure_mbar_100 = 0;
            data->humidity_percentage_100 = 0;
            return false;
        }
    }

    success = bmp280_read(&data->temperature_celsius_100, &data->pressure_mbar_100, &data->humidity_percentage_100);

    if (!success) {
        log_info("BMP280 re-init\n");
        success = bmp280_handler_init();
        log_info("BMP280 re-init: %d\n", success);

        if (success) {
            success = bmp280_read(&data->temperature_celsius_100, &data->pressure_mbar_100, &data->humidity_percentage_100);
        }

        if (!success) {
            data->temperature_celsius_100 = 0;
            data->pressure_mbar_100 = 0;
            data->humidity_percentage_100 = 0;
        }
    }

    bmp280_initialization_required = !success;

    return success;
}
