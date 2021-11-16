#include <stdio.h>
#include <string.h>
#include "aprs.h"

volatile uint16_t aprs_packet_counter = 0;

void convert_degrees_to_dmh(long x, int8_t *degrees, uint8_t *minutes, uint8_t *h_minutes)
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

void aprs_generate_timestamp(char *timestamp, size_t length, telemetry_data *data)
{
    snprintf(timestamp, length, "/%02d%02d%02dz", data->gps.hours, data->gps.minutes, data->gps.seconds);
}
