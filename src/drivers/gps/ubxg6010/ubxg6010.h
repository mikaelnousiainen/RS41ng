#ifndef __UBXG6010_H
#define __UBXG6010_H

#include <stdint.h>

#include "src/gps.h"

bool ubxg6010_init();

void ubxg6010_init_and_sleep(void);

bool ubxg6010_enable_power_save_mode();

void ubxg6010_sleep(void);

void ubxg6010_request_gpstime();

bool ubxg6010_get_current_gps_data(gps_data *data);

bool ubxg6010_peek_current_gps_data(gps_data *data);

void ubxg6010_handle_incoming_byte(uint8_t data, uint8_t reset);

void ubxg6010_reset_parser();

void ubxg6010_clear_data();

#endif
