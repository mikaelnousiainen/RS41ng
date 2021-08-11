#include <stm32f10x_rcc.h>
#include <stm32f10x_tim.h>
#include <misc.h>

#include "datatimer.h"

void (*system_handle_data_timer_tick)() = NULL;

void data_timer_init(uint32_t baud_rate)
{
    // Timer frequency = TIM_CLK/(TIM_PSC+1)/(TIM_ARR + 1)
    // TIM_CLK =
    // TIM_PSC = Prescaler
    // TIM_ARR = Period

    TIM_DeInit(TIM2);

    TIM_TimeBaseInitTypeDef tim_init;
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);
    RCC_APB2PeriphResetCmd(RCC_APB1Periph_TIM2, DISABLE);

    tim_init.TIM_Prescaler = 24 - 1; // tick every 1/1000000 s
    tim_init.TIM_CounterMode = TIM_CounterMode_Up;
    tim_init.TIM_Period = (uint16_t) ((1000000 / baud_rate) - 1);
    tim_init.TIM_ClockDivision = TIM_CKD_DIV1;
    tim_init.TIM_RepetitionCounter = 0;

    TIM_TimeBaseInit(TIM2, &tim_init);

    // No interrupts necessary for data timer, as it is only used for triggering DMA transfers
    TIM_ClearITPendingBit(TIM2, TIM_IT_Update);
    TIM_ITConfig(TIM2, TIM_IT_Update, ENABLE);

    NVIC_InitTypeDef nvic_init;
    nvic_init.NVIC_IRQChannel = TIM2_IRQn;
    nvic_init.NVIC_IRQChannelPreemptionPriority = 0;
    nvic_init.NVIC_IRQChannelSubPriority = 1;
    nvic_init.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&nvic_init);

    TIM_Cmd(TIM2, ENABLE);
}

void data_timer_uninit()
{
    TIM_Cmd(TIM2, DISABLE);

    NVIC_InitTypeDef nvic_init;
    nvic_init.NVIC_IRQChannel = TIM2_IRQn;
    nvic_init.NVIC_IRQChannelPreemptionPriority = 0;
    nvic_init.NVIC_IRQChannelSubPriority = 1;
    nvic_init.NVIC_IRQChannelCmd = DISABLE;
    NVIC_Init(&nvic_init);

    TIM_ITConfig(TIM2, TIM_IT_Update, DISABLE);
    TIM_ClearITPendingBit(TIM2, TIM_IT_Update);
}

void TIM2_IRQHandler(void)
{
    if (TIM_GetITStatus(TIM2, TIM_IT_Update) != RESET) {
        TIM_ClearITPendingBit(TIM2, TIM_IT_Update);

        system_handle_data_timer_tick();

    }
}
