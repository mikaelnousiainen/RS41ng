#include <stm32f1xx_hal.h>

#include "datatimer.h"

TIM_HandleTypeDef htim2; // Data timer
TIM_HandleTypeDef htim3; // Delay() timer
TIM_HandleTypeDef htim4; // System scheduler timer
TIM_HandleTypeDef htim7; // Millisecond counter timer
TIM_HandleTypeDef htim15; // PWM timer

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM2)
    {
        system_handle_data_timer_tick();
    }
}