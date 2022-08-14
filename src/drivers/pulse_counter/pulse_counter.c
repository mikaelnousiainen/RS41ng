#include "pulse_counter.h"
#include "stm32f10x_tim.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_rcc.h"
#include "stm32f10x.h"
#include "misc.h"


bool pulse_counter_init()
{
	//TODO: start counter.
	GPIOBinit_TIM2(void);
	TIM2init_counter(void);

    return true;
}

uint16_t pulse_counter_get_counts(){
	//TODO: Retrieve total counts and return it.
	return 1666;
}

void GPIOBinit_TIM2(void) {
	GPIO_InitTypeDef GPIO_InitStructure; // 2
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE); // 4
	GPIO_PinAFConfig(GPIOB, GPIO_PinSource11, GPIO_AF_TIM2); // PB11:TIM2_Ch4/ETR // 6
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11; // 8
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF; // 9
	GPIO_Init(GPIOA, &GPIO_InitStructure); // 10
}


void TIM2init_counter(void) { // counting from ETR
	TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStructure; // 2
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE); // 4
	TIM_TimeBaseInitStructure.TIM_Prescaler = 0; // 6
	TIM_TimeBaseInitStructure.TIM_CounterMode = TIM_CounterMode_Up; // 7
	TIM_TimeBaseInitStructure.TIM_Period = 100000; // 8
	TIM_TimeBaseInitStructure.TIM_ClockDivision = TIM_CKD_DIV1; // 9
	TIM_TimeBaseInit(TIM2, &TIM_TimeBaseInitStructure); // 10
	TIM_ETRClockMode2Config(TIM2, TIM_ExtTRGPSC_OFF, TIM_ExtTRGPolarity_NonInverted, 0x00); // 12
	TIM_Cmd(TIM2, ENABLE); // 14
}