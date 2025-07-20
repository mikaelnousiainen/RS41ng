#ifndef __MILLIS_H
#define __MILLIS_H

#include <stdint.h>

extern void millis_timer_init(void);
extern void millis_timer_uninit();

extern uint32_t millis();

#endif

