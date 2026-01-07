#ifndef __MILLIS_H
#define __MILLIS_H

#include <stdint.h>
#include "config.h"

#ifdef RS41_RSM4x4
#include <stm32l4xx_hal.h>
#else
#include <stm32f1xx_hal.h>
#endif


extern void millis_timer_init(void);
extern void millis_timer_uninit();

extern uint32_t millis();

void User_TIM7_IRQHandler(TIM_HandleTypeDef *htim);

#endif

