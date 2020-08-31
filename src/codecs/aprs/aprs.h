#ifndef __APRS_H
#define __APRS_H

#include <stddef.h>
#include <stdint.h>

#include "gps.h"
#include "telemetry.h"

size_t aprs_generate_position_without_timestamp(uint8_t *payload, size_t length, telemetry_data *data,
        char symbol_table, char symbol, char *comment);

#endif
