#include <stdbool.h>

#include <stm32f10x.h>
#include <stm32f10x_rcc.h>
#include <stm32f10x_tim.h>
#include <misc.h>

#include "delay.h"

volatile bool done;

void delay_init()
{
    TIM_DeInit(TIM3);

    TIM_TimeBaseInitTypeDef tim_init;

    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);
    RCC_APB1PeriphResetCmd(RCC_APB1Periph_TIM3, DISABLE);

    tim_init.TIM_Prescaler = 24 - 1;
    tim_init.TIM_CounterMode = TIM_CounterMode_Up;
    tim_init.TIM_Period = 0;
    tim_init.TIM_ClockDivision = TIM_CKD_DIV1;
    tim_init.TIM_RepetitionCounter = 0;
    TIM_TimeBaseInit(TIM3, &tim_init);

    TIM_ClearITPendingBit(TIM3, TIM_IT_Update);
    TIM_ITConfig(TIM3, TIM_IT_Update, ENABLE);

    NVIC_InitTypeDef nvic_init;
    nvic_init.NVIC_IRQChannel = TIM3_IRQn;
    nvic_init.NVIC_IRQChannelPreemptionPriority = 0;
    nvic_init.NVIC_IRQChannelSubPriority = 1;
    nvic_init.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&nvic_init);

    TIM_Cmd(TIM3, DISABLE);
}

void delay_us(uint16_t us)
{
    TIM_Cmd(TIM3, DISABLE);
    TIM_SetAutoreload(TIM3, us);
    TIM_SetCounter(TIM3, 0);
    TIM_Cmd(TIM3, ENABLE);
    done = false;
    while (!done) {}

    TIM_Cmd(TIM3, DISABLE);
}

inline void delay_ms(uint32_t ms)
{
    while (ms-- > 0) {
        delay_us(1000);
    }
}

void TIM3_IRQHandler(void)
{
    if (TIM_GetITStatus(TIM3, TIM_IT_Update) != RESET) {
        TIM_ClearITPendingBit(TIM3, TIM_IT_Update);
        done = true;
    }
}
