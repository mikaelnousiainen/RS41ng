#ifndef __APRS_POSITION_H
#define __APRS_POSITION_H

#include <stddef.h>
#include <stdint.h>

#include "gps.h"
#include "telemetry.h"

size_t aprs_generate_position(uint8_t *payload, size_t length, telemetry_data *data,
        char symbol_table, char symbol, bool include_timestamp, char *comment);

#endif
