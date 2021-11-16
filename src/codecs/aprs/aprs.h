#ifndef __APRS_H
#define __APRS_H

#include <stddef.h>
#include <stdint.h>

#include "gps.h"
#include "telemetry.h"

void convert_degrees_to_dmh(long x, int8_t *degrees, uint8_t *minutes, uint8_t *h_minutes);
void aprs_generate_timestamp(char *timestamp, size_t length, telemetry_data *data);

extern volatile uint16_t aprs_packet_counter;

#endif
