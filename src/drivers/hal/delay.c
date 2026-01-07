#include <stdbool.h>

#include "config.h"
#include "delay.h"
#include "timers.h"
#include "log.h"

#ifdef RS41_RSM4x4
#include <stm32l4xx_hal.h>
#else
#include <stm32f1xx_hal.h>
#endif

volatile bool done;

void delay_init()
{
    //HAL_TIM_Base_DeInit(&htim3);

    __HAL_RCC_TIM3_CLK_ENABLE();

    // The data timer assumes a 24 MHz clock source
    htim3.Instance = TIM3;
    htim3.Init.Prescaler = 24 - 1; // tick every 1/1000000 s
    htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim3.Init.Period = 0xFFFF;
    htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim3.Init.RepetitionCounter = 0;

    hang_if_bad("HAL_TIM_Base_Init",
               HAL_TIM_Base_Init(&htim3)
               );

    HAL_TIM_Base_Stop_IT(&htim3);
}

void delay_us(uint16_t us)
{
    HAL_TIM_Base_Start_IT(&htim3);
    __HAL_TIM_SET_COUNTER(&htim3, 0);

    while ((uint16_t)(__HAL_TIM_GET_COUNTER(&htim3)) < us);
    HAL_TIM_Base_Stop_IT(&htim3);
}

inline void delay_ms(uint32_t ms)
{
    while (ms-- > 0) {
        delay_us(1000);
    }
}
