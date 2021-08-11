#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "aprs.h"

/**
 * TODO: add function to generate weather report
 *
 * Examples:
 * 2019-11-27 04:37:21 EET: OH2NJR>APRS,WIDE1-1,qAR,OH2HCY-2:;434.700-C*111111z6030.65N/02444.84Er434.700MHz T118 -200 R50k OH2RUC Nurmijarvi
 * 2019-11-27 04:52:08 EET: OH2NJR>APX210,TCPIP*,qAC,APRSFI-I3:=6030.35N/02443.91E_154/001g002t034r000P000p000b10029
 */

volatile uint16_t aprs_packet_counter = 0;

static void convert_degrees_to_dmh(long x, int8_t *degrees, uint8_t *minutes, uint8_t *h_minutes)
{
    uint8_t sign = (uint8_t) (x > 0 ? 1 : 0);
    if (!sign) {
        x = -(x);
    }
    *degrees = (int8_t) (x / 1000000);
    x = x - (*degrees * 1000000);
    x = (x) * 60 / 10000;
    *minutes = (uint8_t) (x / 100);
    *h_minutes = (uint8_t) (x - (*minutes * 100));
    if (!sign) {
        *degrees = -*degrees;
    }
}

size_t aprs_generate_position(uint8_t *payload, size_t length, telemetry_data *data,
        char symbol_table, char symbol, bool include_timestamp, char *comment)
{
    char timestamp[12];

    int8_t la_degrees, lo_degrees;
    uint8_t la_minutes, la_h_minutes, lo_minutes, lo_h_minutes;

    convert_degrees_to_dmh(data->gps.latitude_degrees_1000000 / 10, &la_degrees, &la_minutes, &la_h_minutes);
    convert_degrees_to_dmh(data->gps.longitude_degrees_1000000 / 10, &lo_degrees, &lo_minutes, &lo_h_minutes);

    int16_t heading_degrees = (int16_t) ((float) data->gps.heading_degrees_100000 / 100000.0f);
    int16_t ground_speed_knots = (int16_t) (((float) data->gps.ground_speed_cm_per_second / 100.0f) * 3.6f / 1.852f);
    int32_t altitude_feet = (data->gps.altitude_mm / 1000) * 3280 / 1000;

    if (include_timestamp) {
        snprintf(timestamp, sizeof(timestamp), "/%02d%02d%02dz", data->gps.hours, data->gps.minutes, data->gps.seconds);
    } else {
        strncpy(timestamp, "!", sizeof(timestamp));
    }

    aprs_packet_counter++;

    return snprintf((char *) payload,
            length,
            ("%s%02d%02d.%02u%c%c%03d%02u.%02u%c%c%03d/%03d/A=%06d/P%dS%dT%02dV%04dC%02d%s"),
            timestamp,
            abs(la_degrees), la_minutes, la_h_minutes,
            la_degrees > 0 ? 'N' : 'S',
            symbol_table,
            abs(lo_degrees), lo_minutes, lo_h_minutes,
            lo_degrees > 0 ? 'E' : 'W',
            symbol,
            heading_degrees,
            ground_speed_knots,
            (int) altitude_feet,
            aprs_packet_counter,
            data->gps.satellites_visible,
            (int) data->internal_temperature_celsius_100 / 100,
            data->battery_voltage_millivolts,
            (int16_t) ((float) data->gps.climb_cm_per_second / 100.0f),
            comment
    );
}
