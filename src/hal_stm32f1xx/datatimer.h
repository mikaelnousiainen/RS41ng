#ifndef __DATATIMER_H
#define __DATATIMER_H

#include <stddef.h>
#include <stdbool.h>

void data_timer_init(uint32_t baud_rate);
void data_timer_uninit();

extern void (*system_handle_data_timer_tick)();

#endif
