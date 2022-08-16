#include "telemetry.h"
#include "hal/system.h"
#include "drivers/si4032/si4032.h"
#include "drivers/ubxg6010/ubxg6010.h"
#include "drivers/pulse_counter/pulse_counter.h"
#include "bmp280_handler.h"
#include "locator.h"
#include "config.h"
#include "log.h"

void telemetry_collect(telemetry_data *data)
{
    log_info("Collecting telemetry...\n");

    data->button_adc_value = system_get_button_adc_value();
    data->battery_voltage_millivolts = system_get_battery_voltage_millivolts();
    data->internal_temperature_celsius_100 = si4032_read_temperature_celsius_100();

    if (bmp280_enabled) {
        bmp280_read_telemetry(data);
    }

    if (pulse_counter_enabled) {
        data->pulse_count = pulse_counter_get_count();
    }

    ubxg6010_get_current_gps_data(&data->gps);

    // Zero out position data if we don't have a valid GPS fix.
    // This is done to avoid transmitting invalid position information.
    if (!GPS_HAS_FIX(data->gps)) {
        data->gps.latitude_degrees_1000000 = 0;
        data->gps.longitude_degrees_1000000 = 0;
        data->gps.altitude_mm = 0;
        data->gps.ground_speed_cm_per_second = 0;
        data->gps.heading_degrees_100000 = 0;
        data->gps.climb_cm_per_second = 0;
    }

    locator_from_lonlat(data->gps.longitude_degrees_1000000, data->gps.latitude_degrees_1000000,
            LOCATOR_PAIR_COUNT_FULL, data->locator);

    log_info("Telemetry collected!\n");
}
