#include "pulse_counter.h"
#ifndef RS41_RSM4x4
    #include <stm32f1xx_hal.h>
#else
    #error "TODO!"
#endif

#include "config.h"

EXTI_HandleTypeDef hexti;

// The pulse count will wrap to zero at 65535 as it is stored as a 16-bit unsigned integer value
uint16_t pulse_count = 0;

void pulse_counter_init(int pin_mode, int edge)
{
    // Initialize pin PB11 with optional internal pull-up resistor
    GPIO_InitTypeDef gpio_init;
    gpio_init.Pin = GPIO_PIN_11;
    gpio_init.Mode = GPIO_MODE_INPUT;
    gpio_init.Pull = (pin_mode == PULSE_COUNTER_PIN_MODE_INTERNAL_PULL_UP)
                          ? GPIO_PULLUP :
                          ((pin_mode == PULSE_COUNTER_PIN_MODE_INTERNAL_PULL_DOWN)
                           ? GPIO_PULLDOWN
                           : GPIO_NOPULL);
    gpio_init.Speed = GPIO_SPEED_FREQ_MEDIUM;
    HAL_GPIO_Init(GPIOB, &gpio_init);

    // PB11 is connected to interrupt line 11, set trigger on the configured edge and enable the interrupt
    EXTI_ConfigTypeDef exti_config;
    exti_config.Line = EXTI_LINE_11;
    exti_config.Mode = EXTI_MODE_INTERRUPT;
    exti_config.Trigger = (edge == PULSE_COUNTER_INTERRUPT_EDGE_FALLING)
                             ? EXTI_TRIGGER_FALLING
                             : EXTI_TRIGGER_RISING;
    exti_config.GPIOSel = EXTI_GPIOB;
    HAL_EXTI_SetConfigLine(&hexti, &exti_config);

    // PB11 is connected to EXTI_Line11, which has EXTI15_10_IRQn vector. Use priority 0 for now.
    HAL_NVIC_SetPriority(EXTI15_10_IRQn, 2, 0);
    HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);
}

uint16_t pulse_counter_get_count()
{
    return pulse_count;
}

void HAL_EXTI_Callback(void)
{
    if (hexti.Line == EXTI_LINE_11)
    {
        __HAL_GPIO_EXTI_CLEAR_IT(GPIO_PIN_11);
        pulse_count = pulse_count + 1;
    }
}