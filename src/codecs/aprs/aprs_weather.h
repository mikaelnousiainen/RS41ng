#ifndef __APRS_WEATHER_H
#define __APRS_WEATHER_H

#include <stddef.h>
#include <stdint.h>

#include "gps.h"
#include "telemetry.h"

size_t aprs_generate_weather_report(uint8_t *payload, size_t length, telemetry_data *data,
        bool include_timestamp, char *comment);

#endif
