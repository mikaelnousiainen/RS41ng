#ifndef __GPS_H
#define __GPS_H

#include <stdint.h>

typedef struct _gps_data {
    uint32_t time_of_week_millis;
    int16_t week;
    uint16_t year;
    uint8_t month;
    uint8_t day;
    uint8_t seconds;
    uint8_t minutes;
    uint8_t hours;

    int32_t lat_raw;
    int32_t lon_raw;
    int32_t alt_raw;
    int32_t speed_raw;
    int32_t heading_raw;
    int32_t climb_raw;
    uint8_t sats_raw;
    uint8_t fix;
    uint16_t ok_packets;
    uint16_t bad_packets;
} gps_data;

#endif
