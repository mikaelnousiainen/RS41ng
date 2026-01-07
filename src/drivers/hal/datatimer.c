#include "datatimer.h"
#include "timers.h"
#include "log.h"
#include "config.h"

#ifdef RS41_RSM4x4
#include <stm32l4xx_hal.h>
#else
#include <stm32f1xx_hal.h>
#endif


void (*system_handle_data_timer_tick)() = NULL;

void data_timer_init(uint32_t baud_rate)
{
    // Timer frequency = TIM_CLK/(TIM_PSC+1)/(TIM_ARR + 1)
    // TIM_CLK =
    // TIM_PSC = Prescaler
    // TIM_ARR = Period

    //__HAL_RCC_TIM2_CLK_ENABLE();
    __TIM2_CLK_ENABLE();

    htim2.Instance = TIM2;

    HAL_TIM_Base_DeInit(&htim2);

    // The data timer assumes a 24 MHz clock source
    htim2.Instance = TIM2;
    htim2.Init.Prescaler = 24 - 1; // tick every 1/1000000 s
    htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim2.Init.Period = (uint16_t) ((1000000 / baud_rate) - 1);
    htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim2.Init.RepetitionCounter = 0;
    htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;

    hang_if_bad("HAL_TIM_Base_Init",
                HAL_TIM_Base_Init(&htim2)
               );

    // No interrupts necessary for data timer, as it is only used for triggering DMA transfers
    __HAL_TIM_CLEAR_IT(&htim2, TIM_IT_UPDATE);
    __HAL_TIM_ENABLE_IT(&htim2, TIM_IT_UPDATE);

    HAL_TIM_RegisterCallback(&htim2, HAL_TIM_PERIOD_ELAPSED_CB_ID, User_TIM2_IRQHandler);

    hang_if_bad("HAL_TIM_Base_Start_IT",
                HAL_TIM_Base_Start_IT(&htim2)
               );

    HAL_NVIC_SetPriority(TIM2_IRQn, 2, 2);
    HAL_NVIC_EnableIRQ(TIM2_IRQn);

}

void data_timer_uninit()
{
    HAL_TIM_Base_Stop_IT(&htim2);

    __HAL_TIM_DISABLE_IT(&htim2, TIM_IT_UPDATE);
    __HAL_TIM_CLEAR_IT(&htim2, TIM_IT_UPDATE);

    HAL_NVIC_DisableIRQ(TIM2_IRQn);
}

// Provide our own stub that just calls the standard HAL IRQ Handler.  
// It will call our PeriodElapsedCallback routine  - User_TIM2_IRQHandler

extern void TIM2_IRQHandler()
{   
    HAL_TIM_IRQHandler(&htim2); 
}   

void User_TIM2_IRQHandler(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM2)
    {
        system_handle_data_timer_tick();
    }
}
