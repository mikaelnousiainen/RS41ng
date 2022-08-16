#include "pulse_counter.h"
#include "stm32f10x_exti.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x.h"
#include "misc.h"

uint16_t pulse_count = 0;

bool pulse_counter_init()
{
    // Set pin PB11, with internal pullup, 10MHz speed
    GPIO_InitTypeDef gpio_init;
    gpio_init.GPIO_Pin = GPIO_Pin_11;
    gpio_init.GPIO_Mode = GPIO_Mode_IPU;
    gpio_init.GPIO_Speed = GPIO_Speed_10MHz;
    GPIO_Init(GPIOB, &gpio_init);

    // PB11 is connected to interrupt line 11, trigger interrupt on a falling edge and enable it.
    EXTI_InitTypeDef exti_init;
    exti_init.EXTI_Line = EXTI_Line11;
    exti_init.EXTI_Mode = EXTI_Mode_Interrupt;
    exti_init.EXTI_Trigger = EXTI_Trigger_Falling;
    exti_init.EXTI_LineCmd = ENABLE;
    EXTI_Init(&exti_init);

    // Attach interrupt line to port B
    GPIO_EXTILineConfig(GPIO_PortSourceGPIOB, GPIO_PinSource11);

    // PB11 is connected to EXTI_Line11, which has EXTI15_10_IRQn vector. Use priority 0 for now.
    NVIC_InitTypeDef NVIC_InitStruct;
    NVIC_InitStruct.NVIC_IRQChannel = EXTI15_10_IRQn;
    NVIC_InitStruct.NVIC_IRQChannelPreemptionPriority = 0x00;
    NVIC_InitStruct.NVIC_IRQChannelSubPriority = 0x00;
    NVIC_InitStruct.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStruct);

    return true;
}

uint16_t pulse_counter_get_count()
{
    return pulse_count;
}

void EXTI15_10_IRQHandler(void)
{
    pulse_count = pulse_count + 1;
    EXTI_ClearITPendingBit(EXTI_Line11);
}
