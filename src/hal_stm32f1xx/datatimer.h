#ifndef __DATATIMER_H
#define __DATATIMER_H

#include <stddef.h>
#include <stdbool.h>
#include <stm32f1xx_hal.h>

void data_timer_init(uint32_t baud_rate);
void data_timer_uninit();

extern void (*system_handle_data_timer_tick)();

void User_TIM2_IRQHandler(TIM_HandleTypeDef *htim);

#endif
