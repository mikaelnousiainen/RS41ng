#ifndef __CLOCK_CALIBRATION_H
#define __CLOCK_CALIBRATION_H

#include "config.h"

#ifdef DFM17

extern void timepulse_init();
extern uint8_t clock_calibration_get_trim();
extern uint16_t clock_calibration_get_change_count();
uint32_t clock_calibration_get_millis_delta();
extern void clock_calibration_adjust();

#endif

#endif
