#include "pulse_counter.h"
#include "stm32f10x_exti.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x.h"
#include "misc.h"

uint16_t counts=1666;

bool pulse_counter_init()
{
	//TODO: start counter.
	GPIO_Init();
    return true;
}

uint16_t pulse_counter_get_counts(){
	//TODO: Retrieve total counts and return it.
	return counts;
}

void GPIO_Init(void) {
	
	GPIO_InitTypeDef GPIO_InitStructure; 
	//Set pin 22 as input
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_22;
	//Enable internal pullup
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
	GPIO_Init(GPIOB, &GPIO_InitStructure);
	
	
	
	EXTI_InitTypeDef EXTI_InitStruct;
	/* PD0 is connected to EXTI_Line0 */
	EXTI_InitStruct.EXTI_Line = EXTI_Line11;
	/* Interrupt mode */
	EXTI_InitStruct.EXTI_Mode = EXTI_Mode_Interrupt;
	/* Triggers on rising and falling edge */
	EXTI_InitStruct.EXTI_Trigger = EXTI_Trigger_Falling;
	/* Enable interrupt */
	EXTI_InitStruct.EXTI_LineCmd = ENABLE;
	/* Add to EXTI */
	EXTI_Init(&EXTI_InitStruct);
	
	
	
	/* Add IRQ vector to NVIC */
	/* PB11 is connected to EXTI_Line11, which has EXTI11_IRQn vector */
	NVIC_InitStruct.NVIC_IRQChannel = EXTI11_IRQn;
	/* Set priority */
	NVIC_InitStruct.NVIC_IRQChannelPreemptionPriority = 0x00;
	/* Set sub priority */
	NVIC_InitStruct.NVIC_IRQChannelSubPriority = 0x00;
	/* Enable interrupt */
	NVIC_InitStruct.NVIC_IRQChannelCmd = ENABLE;
	/* Add to NVIC */
	NVIC_Init(&NVIC_InitStruct);
}

void EXTI11_IRQHandler(void)
{
	counts = counts+1;
	EXTI_ClearITPendingBit(EXTI_Line11); //Clear the interrupt flag bit on LINE11
}