#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "aprs.h"
#include "aprs_weather.h"

size_t aprs_generate_weather_report(uint8_t *payload, size_t length, telemetry_data *data,
        bool include_timestamp, char *comment)
{
    char timestamp[12];

    int8_t la_degrees, lo_degrees;
    uint8_t la_minutes, la_h_minutes, lo_minutes, lo_h_minutes;

    convert_degrees_to_dmh(data->gps.latitude_degrees_1000000 / 10, &la_degrees, &la_minutes, &la_h_minutes);
    convert_degrees_to_dmh(data->gps.longitude_degrees_1000000 / 10, &lo_degrees, &lo_minutes, &lo_h_minutes);

    if (include_timestamp) {
        aprs_generate_timestamp(timestamp, sizeof(timestamp), data);
    } else {
        strncpy(timestamp, "!", sizeof(timestamp));
    }

    int32_t temperature_fahrenheits = (data->temperature_celsius_100 * 9 / 5) / 100 + 32;
    uint32_t pressure_mbar_10 = data->pressure_mbar_100 / 10;
    uint16_t humidity_percentage = data->humidity_percentage_100 / 100;
    if (humidity_percentage > 99) {
        humidity_percentage = 0; // Zero represents 100%
    } else if (humidity_percentage < 1) {
        humidity_percentage = 1;
    }

    aprs_packet_counter++;

    return snprintf((char *) payload,
            length,
            ("%s%02d%02d.%02u%c%c%03d%02u.%02u%c%c.../...g...t%03dh%02ub%05u%s"),
            timestamp,
            abs(la_degrees), la_minutes, la_h_minutes,
            la_degrees > 0 ? 'N' : 'S',
            '/', // Primary symbol table
            abs(lo_degrees), lo_minutes, lo_h_minutes,
            lo_degrees > 0 ? 'E' : 'W',
            '_', // Weather station symbol
            (int) temperature_fahrenheits,
            (unsigned int) humidity_percentage,
            (unsigned int) pressure_mbar_10,
            comment
    );
}
