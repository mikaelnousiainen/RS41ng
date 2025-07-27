#ifndef __DELAY_H
#define __DELAY_H

#include <stdint.h>
#include <stm32f1xx_hal.h>

#ifdef __cplusplus
extern "C" {
#endif

void delay_init();

void delay_us(uint16_t us);

void delay_ms(uint32_t ms);

void User_TIM3_IRQHandler(TIM_HandleTypeDef *htim);

#ifdef __cplusplus
}
#endif

#endif
