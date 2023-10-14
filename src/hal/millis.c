#include <stm32f10x_rcc.h>
#include <stm32f10x_tim.h>
#include <misc.h>

#include "src/hal/millis.h"

static uint32_t	millis_counter;

void millis_timer_init(void)
{
    TIM_DeInit(TIM7);

    TIM_TimeBaseInitTypeDef tim_init;
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM7, ENABLE);
    RCC_APB2PeriphResetCmd(RCC_APB1Periph_TIM7, DISABLE);

    // The data timer assumes a 24 MHz clock source
    tim_init.TIM_Prescaler = 24 - 1; // tick every 1/1000000 s
    tim_init.TIM_CounterMode = TIM_CounterMode_Up;
    tim_init.TIM_Period = (uint16_t) (1000 - 1); // set up period of 1 millisecond
    tim_init.TIM_ClockDivision = TIM_CKD_DIV1;
    tim_init.TIM_RepetitionCounter = 0;

    TIM_TimeBaseInit(TIM7, &tim_init);

    TIM_ClearITPendingBit(TIM7, TIM_IT_Update);
    TIM_ITConfig(TIM7, TIM_IT_Update, ENABLE);

    NVIC_InitTypeDef nvic_init;
    nvic_init.NVIC_IRQChannel = TIM7_IRQn;
    nvic_init.NVIC_IRQChannelPreemptionPriority = 0;
    nvic_init.NVIC_IRQChannelSubPriority = 1;
    nvic_init.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&nvic_init);

    TIM_Cmd(TIM7, ENABLE);
}

void millis_timer_uninit()
{
    TIM_Cmd(TIM7, DISABLE);

    NVIC_InitTypeDef nvic_init;
    nvic_init.NVIC_IRQChannel = TIM7_IRQn;
    nvic_init.NVIC_IRQChannelPreemptionPriority = 0;
    nvic_init.NVIC_IRQChannelSubPriority = 1;
    nvic_init.NVIC_IRQChannelCmd = DISABLE;
    NVIC_Init(&nvic_init);

    TIM_ITConfig(TIM7, TIM_IT_Update, DISABLE);
    TIM_ClearITPendingBit(TIM7, TIM_IT_Update);
}

void TIM7_IRQHandler(void)
{
    if (TIM_GetITStatus(TIM7, TIM_IT_Update) != RESET) {
        TIM_ClearITPendingBit(TIM7, TIM_IT_Update);
        millis_counter++;
    }
}

uint32_t millis(void)
{
    return millis_counter;
}
