#include <stdio.h>
#include <stdlib.h>
#include "aprs.h"

/*
 * Examples:
2019-11-27 04:37:21 EET: OH2NJR>APRS,WIDE1-1,qAR,OH2HCY-2:;434.700-C*111111z6030.65N/02444.84Er434.700MHz T118 -200 R50k OH2RUC Nurmijarvi
2019-11-27 04:52:08 EET: OH2NJR>APX210,TCPIP*,qAC,APRSFI-I3:=6030.35N/02443.91E_154/001g002t034r000P000p000b10029

  TODO: speed and bearing before altitude!
2019-11-27 13:39:45 EET: OH3EUJ>APRARX,SONDEGATE,TCPIP,qAR,OH3EUJ:;R1920638 *113959h6051.11N/02331.80EO063/021/A=011240 Clb=6.6m/s t=-13.2C 402.970 MHz Type=RS41-SG Radiosonde http://bit.ly/2Bj4Sfk !woM!
2019-11-27 13:40:00 EET: OH3EUJ>APRARX,SONDEGATE,TCPIP,qAR,OH3EUJ:;R1920638 *114015h6051.17N/02331.98EO056/026/A=011536 Clb=5.0m/s t=-13.8C 402.970 MHz Type=RS41-SG Radiosonde http://bit.ly/2Bj4Sfk !wja!
 */

// TODO: add function to generate weather report

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

#include <stdio.h>

size_t aprs_generate_position_without_timestamp(uint8_t *payload, size_t length, telemetry_data *data, char symbol, char *comment)
{
    int8_t la_degrees, lo_degrees;
    uint8_t la_minutes, la_h_minutes, lo_minutes, lo_h_minutes;

    convert_degrees_to_dmh(data->gps.lat_raw / 10, &la_degrees, &la_minutes, &la_h_minutes);
    convert_degrees_to_dmh(data->gps.lon_raw / 10, &lo_degrees, &lo_minutes, &lo_h_minutes);

    aprs_packet_counter++;

    // TODO: speed + bearing

    return snprintf((char *) payload,
            length,
            ("!%02d%02d.%02u%c/%03d%02u.%02u%c%c/A=%06ld/P%dS%dT%dV%d%s"),
            abs(la_degrees), la_minutes, la_h_minutes,
            la_degrees > 0 ? 'N' : 'S',
            abs(lo_degrees), lo_minutes, lo_h_minutes,
            lo_degrees > 0 ? 'E' : 'W',
            symbol,
            (data->gps.alt_raw / 1000) * 3280 / 1000,
            aprs_packet_counter,
            data->gps.sats_raw,
            (int16_t) data->temperature_celsius_100,
            data->battery_voltage_millivolts, // TODO: unit?
            comment
    );
}
