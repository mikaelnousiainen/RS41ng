#ifndef __GPS_H
#define __GPS_H

#include <stdint.h>
#include <stdbool.h>

typedef struct _gps_data {
    bool updated;

    uint32_t time_of_week_millis;
    int16_t week;
    uint16_t year;
    uint8_t month;
    uint8_t day;
    uint8_t seconds;
    uint8_t minutes;
    uint8_t hours;

    int32_t latitude_degrees_1000000;
    int32_t longitude_degrees_1000000;
    int32_t altitude_mm;
    uint32_t ground_speed_cm_per_second;
    int32_t heading_degrees_100000;
    int32_t climb_cm_per_second;
    uint8_t satellites_visible;
    uint8_t fix;
    uint16_t ok_packets;
    uint16_t bad_packets;
} gps_data;

#endif
