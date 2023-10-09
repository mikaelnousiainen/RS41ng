#ifndef __TIMEPULSE_H
#define __TIMEPULSE_H

extern void timepulse_init(void);
extern int calib_suggestion;
extern volatile int timepulsed;
extern volatile uint32_t d_millis;

#endif // __TIMEPULSE_H
