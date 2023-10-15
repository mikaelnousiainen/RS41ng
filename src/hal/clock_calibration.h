#ifndef __CLOCK_CALIBRATION_H
#define __CLOCK_CALIBRATION_H

#include "config.h"

#ifdef DFM17

extern void timepulse_init();
extern uint8_t clock_calibration_get_trim();
extern uint16_t clock_calibration_get_change_count();
extern void clock_calibration_adjust();

#endif

#endif
