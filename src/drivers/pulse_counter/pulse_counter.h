#ifndef __PULSE_COUNTER_H
#define __PULSE_COUNTER_H

#include <stdint.h>
#include <stdbool.h>

void pulse_counter_init(int pin_mode, int edge);
uint16_t pulse_counter_get_count();

#endif
