#ifndef __TIMEPULSE_H
#define __TIMEPULSE_H

extern uint8_t get_clock_calibration(void);
extern uint16_t get_calib_change_count(void);
extern void timepulse_init(void);
extern void adjust_clock_calibration(void);

extern volatile int timepulsed;

#endif // __TIMEPULSE_H
