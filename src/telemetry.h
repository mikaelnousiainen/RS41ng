#ifndef __TELEMETRY_H
#define __TELEMETRY_H

#include <stdint.h>
#include <stdbool.h>

#include "gps.h"

typedef struct _telemetry_data {
    uint16_t battery_voltage_millivolts;
    int32_t internal_temperature_celsius_100;

    int32_t temperature_celsius_100;
    uint32_t pressure_mbar_100;
    uint32_t humidity_percentage_100;

    gps_data gps;
} telemetry_data;

void telemetry_collect();

#endif
