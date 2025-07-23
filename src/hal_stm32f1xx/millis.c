#include <stm32f1xx_hal.h>

#include "src/hal_stm32f1xx/millis.h"
#include "src/hal_stm32f1xx/timers.h"

static uint32_t	millis_counter;

void millis_timer_init(void)
{
    HAL_TIM_Base_DeInit(&htim7);

    __HAL_RCC_TIM7_CLK_ENABLE();

    // The data timer assumes a 24 MHz clock source
    htim2.Init.Prescaler = 24 - 1; // tick every 1/1000000 s
    htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim2.Init.Period = (uint16_t) (1000 - 1); // set up period of 1 millisecond
    htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim2.Init.RepetitionCounter = 0;

    HAL_TIM_Base_Init(&htim7);

    // No interrupts necessary for data timer, as it is only used for triggering DMA transfers
    __HAL_TIM_CLEAR_IT(&htim7, TIM_IT_UPDATE);
    __HAL_TIM_ENABLE_IT(&htim7, TIM_IT_UPDATE);

    HAL_NVIC_SetPriority(TIM7_IRQn, 0, 1);
    HAL_NVIC_EnableIRQ(TIM7_IRQn);
}

void millis_timer_uninit()
{
    HAL_TIM_Base_Stop_IT(&htim7);
}

void TIM7_IRQHandler(void)
{
    if (__HAL_TIM_GET_IT_SOURCE(&htim7, TIM_IT_UPDATE) != RESET)
    {
        __HAL_TIM_CLEAR_IT(&htim7, TIM_IT_UPDATE);
        millis_counter++;
    }
}

uint32_t millis(void)
{
    return millis_counter;
}