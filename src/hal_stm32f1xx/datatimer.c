#include <stm32f1xx_hal.h>

#include "datatimer.h"
#include "timers.h"

void (*system_handle_data_timer_tick)() = NULL;

void data_timer_init(uint32_t baud_rate)
{
    // Timer frequency = TIM_CLK/(TIM_PSC+1)/(TIM_ARR + 1)
    // TIM_CLK =
    // TIM_PSC = Prescaler
    // TIM_ARR = Period

    HAL_TIM_Base_DeInit(&htim2);

    __HAL_RCC_TIM2_CLK_ENABLE();

    // The data timer assumes a 24 MHz clock source
    htim2.Init.Prescaler = 24 - 1; // tick every 1/1000000 s
    htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim2.Init.Period = (uint16_t) ((1000000 / baud_rate) - 1);
    htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim2.Init.RepetitionCounter = 0;

    HAL_TIM_Base_Init(&htim2);

    // No interrupts necessary for data timer, as it is only used for triggering DMA transfers
    __HAL_TIM_CLEAR_IT(&htim2, TIM_IT_UPDATE);
    __HAL_TIM_ENABLE_IT(&htim2, TIM_IT_UPDATE);

    HAL_NVIC_SetPriority(TIM2_IRQn, 2, 2);
    HAL_NVIC_EnableIRQ(TIM2_IRQn);

    HAL_TIM_Base_Start_IT(&htim2);
}

void data_timer_uninit()
{
    HAL_TIM_Base_Stop(&htim2);

    HAL_NVIC_DisableIRQ(TIM2_IRQn);

    __HAL_TIM_DISABLE_IT(&htim2, TIM_IT_UPDATE);
    __HAL_TIM_CLEAR_IT(&htim2, TIM_IT_UPDATE);
}

void TIM2_IRQHandler(void)
{
    if (__HAL_TIM_GET_IT_SOURCE(&htim2, TIM_IT_UPDATE) != RESET)
    {
        system_handle_data_timer_tick();
        __HAL_TIM_CLEAR_IT(&htim2, TIM_IT_UPDATE);
    }
    // HAL_TIM_IRQHandler(&htim2);
}
