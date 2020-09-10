#include "telemetry.h"
#include "hal/system.h"
#include "drivers/si4032/si4032.h"
#include "drivers/ubxg6010/ubxg6010.h"
#include "bmp280_handler.h"
#include "locator.h"
#include "config.h"

void telemetry_collect(telemetry_data *data)
{
    data->button_adc_value = system_get_button_adc_value();
    data->battery_voltage_millivolts = system_get_battery_voltage_millivolts();
    data->internal_temperature_celsius_100 = si4032_read_temperature_celsius_100();

    if (bmp280_enabled) {
        bmp280_read_telemetry(data);
    }

    ubxg6010_get_current_gps_data(&data->gps);
    locator_from_lonlat(data->gps.longitude_degrees_1000000, data->gps.latitude_degrees_1000000,
            LOCATOR_PAIR_COUNT_FULL, data->locator);
}
