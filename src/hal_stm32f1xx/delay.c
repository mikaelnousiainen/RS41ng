#include <stdbool.h>

#include <stm32f1xx_hal.h>

#include "delay.h"
#include "timers.h"

volatile bool done;

void delay_init()
{
    //HAL_TIM_Base_DeInit(&htim3);

    // __HAL_RCC_TIM3_CLK_ENABLE();

    // The data timer assumes a 24 MHz clock source
    htim2.Init.Prescaler = 24 - 1; // tick every 1/1000000 s
    htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim2.Init.Period = 0;
    htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim2.Init.RepetitionCounter = 0;

    HAL_TIM_Base_Init(&htim3);

    // No interrupts necessary for data timer, as it is only used for triggering DMA transfers
    __HAL_TIM_CLEAR_IT(&htim3, TIM_IT_UPDATE);
    __HAL_TIM_ENABLE_IT(&htim3, TIM_IT_UPDATE);

    HAL_NVIC_SetPriority(TIM3_IRQn, 1, 1);
    HAL_NVIC_EnableIRQ(TIM3_IRQn);

    HAL_TIM_Base_Stop_IT(&htim3);
}

void delay_us(uint16_t us)
{
    HAL_TIM_Base_Stop_IT(&htim3);
    __HAL_TIM_SET_AUTORELOAD(&htim3, us);
    __HAL_TIM_SET_COUNTER(&htim3, 0);
    HAL_TIM_Base_Start_IT(&htim3);

    done = false;
    while (!done) {}

    HAL_TIM_Base_Stop_IT(&htim3);
}

inline void delay_ms(uint32_t ms)
{
    while (ms-- > 0) {
        delay_us(1000);
    }
}

void TIM3_IRQHandler(void)
{
    if (__HAL_TIM_GET_IT_SOURCE(&htim3, TIM_IT_UPDATE) != RESET)
    {
        done = true;
        __HAL_TIM_CLEAR_IT(&htim3, TIM_IT_UPDATE);
    }
}
