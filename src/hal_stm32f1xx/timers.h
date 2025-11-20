#ifndef __TIMERS_H
#define __TIMERS_H

#include <stm32f1xx_hal.h>

extern TIM_HandleTypeDef htim2; // Data timer
extern TIM_HandleTypeDef htim3; // Delay() timer
extern TIM_HandleTypeDef htim4; // System scheduler timer
extern TIM_HandleTypeDef htim7; // Millisecond counter timer
extern TIM_HandleTypeDef htim15; // PWM timer

#endif