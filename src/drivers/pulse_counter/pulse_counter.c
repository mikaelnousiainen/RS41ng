#include "pulse_counter.h"
#include "stm32f10x_exti.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x.h"
#include "misc.h"

uint16_t count=0;

bool pulse_counter_init() {
    counter_pin_init();
    return true;
}

uint16_t pulse_counter_get_count() {
    return count;
}

void counter_pin_init(void) {
    //Initialize GPIO typedef
    GPIO_InitTypeDef gpio_init;
    //Set pin PB11, with internal pullup, 10MHz speed
    gpio_init.GPIO_Pin = GPIO_Pin_11;
    gpio_init.GPIO_Mode = GPIO_Mode_IPU;
    gpio_init.GPIO_Speed = GPIO_Speed_10MHz;
    GPIO_Init(GPIOB, &gpio_init);
    
    //Initialize EXTI typedef
    EXTI_InitTypeDef exti_init;
    //PB11 is connected to interrupt line 11, interrupt mode, we trigger on a falling edge and enable it.
    exti_init.EXTI_Line = EXTI_Line11;
    exti_init.EXTI_Mode = EXTI_Mode_Interrupt; 
    exti_init.EXTI_Trigger = EXTI_Trigger_Falling;
    exti_init.EXTI_LineCmd = ENABLE;
    EXTI_Init(&exti_init);
    
    //Attach interrupt line to port B
    GPIO_EXTILineConfig(GPIO_PortSourceGPIOB, GPIO_PinSource11);
    
    //Initialize NVIC typedef
    NVIC_InitTypeDef NVIC_InitStruct;
    //PB11 is connected to EXTI_Line11, which has EXTI15_10_IRQn vector, we leave 00 priority for now.
    NVIC_InitStruct.NVIC_IRQChannel = EXTI15_10_IRQn;
    NVIC_InitStruct.NVIC_IRQChannelPreemptionPriority = 0x00;
    NVIC_InitStruct.NVIC_IRQChannelSubPriority = 0x00;
    NVIC_InitStruct.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStruct);
}

void EXTI15_10_IRQHandler(void) {
    count = count+1;
    EXTI_ClearITPendingBit(EXTI_Line11); //Clear the interrupt flag bit on LINE11
}