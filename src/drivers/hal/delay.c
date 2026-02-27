#include <stdbool.h>

#include "config.h"

#ifdef RS41_RSM4x4
#include <stm32l4xx_hal.h>
#else
#include <stm32f1xx_hal.h>
#endif

#include "delay.h"
#include "timers.h"
#include "log.h"

volatile bool done;

void delay_init()
{
    //HAL_TIM_Base_DeInit(&htim1);

    __HAL_RCC_TIM1_CLK_ENABLE();

    // The data timer assumes a 24 MHz clock source
    htim1.Instance = TIM1;
    htim1.Init.Prescaler = 24 - 1; // tick every 1/1000000 s
    htim1.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim1.Init.Period = 0xFFFF;
    htim1.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim1.Init.RepetitionCounter = 0;

    hang_if_bad("HAL_TIM_Base_Init",
               HAL_TIM_Base_Init(&htim1)
               );

    HAL_TIM_Base_Stop_IT(&htim1);
}

void delay_us(uint16_t us)
{
    __HAL_TIM_SET_COUNTER(&htim1, 0);
    __HAL_TIM_SET_AUTORELOAD(&htim1, us);
    __HAL_TIM_CLEAR_FLAG(&htim1, TIM_FLAG_UPDATE);
    HAL_TIM_Base_Start_IT(&htim1);

    // Loop, in case a different interrupt triggers
    while (!__HAL_TIM_GET_FLAG(&htim1, TIM_FLAG_UPDATE)) {
        __WFI();
    }

    HAL_TIM_Base_Stop_IT(&htim1);
}

inline void delay_ms(uint32_t ms)
{
    while (ms >= 65) {
        delay_us(65000);
        ms -= 65;
    }
    if (ms > 0) {
        delay_us((uint16_t)(ms * 1000));
    }
}
