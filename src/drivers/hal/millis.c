#include "config.h"

#ifdef RS41_RSM4x4
#include <stm32l4xx_hal.h>
#else
#include <stm32f1xx_hal.h>
#endif


#include "src/drivers/hal/millis.h"
#include "src/drivers/hal/timers.h"

static uint32_t	millis_counter;

void millis_timer_init(void)
{
    __HAL_RCC_TIM6_CLK_ENABLE();

    htim6.Instance = TIM6;

    // The millis timer assumes a 24 MHz clock source
    htim6.Init.Prescaler = 24 - 1; // tick every 1/1000000 s
    htim6.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim6.Init.Period = (uint16_t) (1000 - 1); // set up period of 1 millisecond
    htim6.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim6.Init.RepetitionCounter = 0;

    HAL_TIM_Base_Init(&htim6);

    // No interrupts necessary for data timer, as it is only used for triggering DMA transfers
    __HAL_TIM_CLEAR_IT(&htim6, TIM_IT_UPDATE);
    __HAL_TIM_ENABLE_IT(&htim6, TIM_IT_UPDATE);

    HAL_NVIC_SetPriority(TIM6_IRQn, 0, 1);
    HAL_NVIC_EnableIRQ(TIM6_IRQn);

    HAL_TIM_RegisterCallback(&htim6, HAL_TIM_PERIOD_ELAPSED_CB_ID, User_TIM6_IRQHandler);
}

void millis_timer_uninit()
{
    HAL_TIM_Base_Stop_IT(&htim6);
}

void User_TIM6_IRQHandler(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM6)
    {
        millis_counter++;
    }
}

uint32_t millis(void)
{
    return millis_counter;
}
