#include "pulse_counter.h"
#include "stm32f10x_exti.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x.h"
#include "misc.h"
#include "config.h"

// The pulse count will wrap to zero at 65535 as it is stored as a 16-bit unsigned integer value
uint16_t pulse_count = 0;

void pulse_counter_init(int pin_mode, int edge)
{
    // Initialize pin PB11 with optional internal pull-up resistor
    GPIO_InitTypeDef gpio_init;
    gpio_init.GPIO_Pin = GPIO_Pin_11;
    gpio_init.GPIO_Mode = (pin_mode == PULSE_COUNTER_PIN_MODE_INTERNAL_PULL_UP)
                          ? GPIO_Mode_IPU :
                          ((pin_mode == PULSE_COUNTER_PIN_MODE_INTERNAL_PULL_DOWN)
                           ? GPIO_Mode_IPD
                           : GPIO_Mode_IN_FLOATING);
    gpio_init.GPIO_Speed = GPIO_Speed_10MHz;
    GPIO_Init(GPIOB, &gpio_init);

    // PB11 is connected to interrupt line 11, set trigger on the configured edge and enable the interrupt
    EXTI_InitTypeDef exti_init;
    exti_init.EXTI_Line = EXTI_Line11;
    exti_init.EXTI_Mode = EXTI_Mode_Interrupt;
    exti_init.EXTI_Trigger = (edge == PULSE_COUNTER_INTERRUPT_EDGE_FALLING)
                             ? EXTI_Trigger_Falling
                             : EXTI_Trigger_Rising;
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
