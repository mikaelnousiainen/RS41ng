#ifndef __GPS_H
#define __GPS_H

#include <stdint.h>
#include <stdbool.h>
#include "config.h"

#define POWER_SAFE_MODE_STATE_ACQUISITION 0
#define POWER_SAFE_MODE_STATE_TRACKING 1
#define POWER_SAFE_MODE_STATE_POWER_OPTIMIZED_TRACKING 2
#define POWER_SAFE_MODE_STATE_INACTIVE 3

#define GPS_FIX_NO_FIX 0
#define GPS_FIX_DEAD_RECKONING_ONLY 1
#define GPS_FIX_2D 2
#define GPS_FIX_3D 3
#define GPS_FIX_GPS_AND_DEAD_RECKONING 4
#define GPS_FIX_TIME_ONLY 5

#if GPS_REQUIRE_3D_FIX
#define GPS_HAS_FIX(gps_data) (gps_data.fix_ok && (gps_data.fix == GPS_FIX_3D))
#else
#define GPS_HAS_FIX(gps_data) (gps_data.fix_ok && (gps_data.fix == GPS_FIX_2D || gps_data.fix == GPS_FIX_3D))
#endif

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
    bool fix_ok;
    uint16_t ok_packets;
    uint16_t bad_packets;

    uint8_t power_safe_mode_state;
    uint16_t position_dilution_of_precision; // pDOP
} gps_data;

#endif
