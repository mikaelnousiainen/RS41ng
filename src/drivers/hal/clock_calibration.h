#ifndef __CLOCK_CALIBRATION_H
#define __CLOCK_CALIBRATION_H

#include "config.h"

#ifdef DFM17

extern void timepulse_init();
extern uint8_t clock_calibration_get_trim();
extern uint16_t clock_calibration_get_change_count();
extern uint32_t clock_calibration_get_millis_delta();
extern void clock_calibration_adjust();

// Returns the GPS-disciplined Si4063 XO_TUNE offset (signed, ±CAP_TRIM_OFFSET_MAX steps).
// Add this to the temperature LUT value before calling si4063_set_crystal_capacitance().
extern int clock_calibration_get_cap_trim_offset();

// Returns the last measured timepulse error in µs (delta_ticks - 1,000,000).
// Nominally 0 when locked; sign shows frequency direction.
extern int32_t clock_calibration_get_us_error();

#endif

#endif
