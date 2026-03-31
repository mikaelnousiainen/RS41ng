#ifndef __DELAY_H
#define __DELAY_H

#include <stdint.h>
#include "config.h"

#ifdef RS41_RSM4x4
#include <stm32l4xx_hal.h>
#else
#include <stm32f1xx_hal.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

void delay_init();

void delay_us(uint16_t us);

void delay_us_loop(uint16_t us);

void delay_ms(uint32_t ms);

void User_TIM1_IRQHandler(TIM_HandleTypeDef *htim);

#ifdef __cplusplus
}
#endif

#endif
