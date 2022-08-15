#include "pulse_counter.h"
#include "stm32f10x_exti.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x.h"
#include "misc.h"

uint16_t counts=1666;

bool pulse_counter_init()
{
	//TODO: start counter.
	CounterPin_Init();
    return true;
}

uint16_t pulse_counter_get_counts(){
	//TODO: Retrieve total counts and return it.
	return counts;
}

void CounterPin_Init(void) {
	
	//Initialize GPIO typedef
	GPIO_InitTypeDef GPIO_InitStructure; 
	//Set pin 11 as input
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11;
	//Enable internal pullup
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
	GPIO_Init(GPIOB, &GPIO_InitStructure);
	
	
	//Initialize EXTI typedef
	EXTI_InitTypeDef EXTI_InitStruct;
	//PB11 is connected to interrupt line 11
	EXTI_InitStruct.EXTI_Line = EXTI_Line11;
	//Interrupt mode
	EXTI_InitStruct.EXTI_Mode = EXTI_Mode_Interrupt;
	//Triggers on falling edge
	EXTI_InitStruct.EXTI_Trigger = EXTI_Trigger_Falling;
	//Enable interrupt
	EXTI_InitStruct.EXTI_LineCmd = ENABLE;
	//Add to EXTI
	EXTI_Init(&EXTI_InitStruct);
	
	//Initialize NVIC typedef
	NVIC_InitTypeDef NVIC_InitStruct;
	//PB11 is connected to EXTI_Line11, which has EXTI4_IRQn vector */
	NVIC_InitStruct.NVIC_IRQChannel = EXTI4_IRQn;
	//Priority
	NVIC_InitStruct.NVIC_IRQChannelPreemptionPriority = 0x00;
	//Sub-priority
	NVIC_InitStruct.NVIC_IRQChannelSubPriority = 0x00;
	//Enable interrupt
	NVIC_InitStruct.NVIC_IRQChannelCmd = ENABLE;
	//Add to NVIC
	NVIC_Init(&NVIC_InitStruct);
}

void EXTI4_IRQHandler(void){
	counts = counts+1;
	EXTI_ClearITPendingBit(EXTI_Line11); //Clear the interrupt flag bit on LINE11
}