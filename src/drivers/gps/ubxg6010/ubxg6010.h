#ifndef __UBXG6010_H
#define __UBXG6010_H

#include <stdint.h>

#include "src/gps.h"

bool ubxg6010_init();

bool ubxg6010_enable_power_save_mode();

void ubxg6010_request_gpstime();

bool ubxg6010_get_current_gps_data(gps_data *data);

void ubxg6010_handle_incoming_byte(uint8_t data);

void ubxg6010_reset_parser();

#endif
