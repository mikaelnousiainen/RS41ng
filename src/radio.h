#ifndef __RADIO_H
#define __RADIO_H

#include <stdbool.h>

#include "gps.h"

void radio_init();
void radio_handle_timer_tick();
void radio_handle_data_timer_tick();
void radio_handle_main_loop();
bool radio_gps_get_position(gps_data *out);

#endif
