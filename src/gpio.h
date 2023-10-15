#ifndef __GPIO_H
#define __GPIO_H

#include <system_stm32f10x.h>
#include <stm32f10x_gpio.h>

#include "config.h"


// GPIO definitions for devices we use

#if defined (RS41)

#define BANK_SHUTDOWN	GPIOA
#define PIN_SHUTDOWN	GPIO_Pin_12

#define BANK_VOLTAGE	GPIOA
#define PIN_VOLTAGE	GPIO_Pin_5
#define ADC_VOLTAGE	ADC1
#define CHANNEL_VOLTAGE	ADC_Channel_5

#define	BANK_BUTTON	GPIOA
#define PIN_BUTTON	GPIO_Pin_6
#define ADC_BUTTON	ADC1
#define CHANNEL_BUTTON  ADC_Channel_6

#define	BANK_RED_LED	GPIOB
#define PIN_RED_LED	GPIO_Pin_8

#define	BANK_GREEN_LED	GPIOB
#define PIN_GREEN_LED	GPIO_Pin_7

#define BANK_MOSI	GPIOB
#define PIN_MOSI	GPIO_Pin_15
#define	BANK_SCK	GPIOB
#define PIN_SCK		GPIO_Pin_13
#define BANK_MISO	GPIOB
#define PIN_MISO	GPIO_Pin_14
#define	APBPERIPHERAL_SPI	RCC_APB1Periph_SPI2
#define	PERIPHERAL_SPI	SPI2
#define RCC_SPIPeriphClockCmd RCC_APB1PeriphClockCmd

#define	PIN_USART_TX	GPIO_Pin_9
#define BANK_USART_TX	GPIOA
#define	PIN_USART_RX	GPIO_Pin_10
#define BANK_USART_RX	GPIOA
#define USART_IRQ	USART1_IRQn
#define USART_IT	USART1
#define APBPERIPHERAL_USART	RCC_APB2Periph_USART1
#define USART_IRQ_HANDLER	USART1_IRQHandler

#elif defined (DFM17)

#define BANK_SHUTDOWN	GPIOC			
#define PIN_SHUTDOWN	GPIO_Pin_0

#define BANK_VOLTAGE	GPIOA			// Needs confirmation
#define PIN_VOLTAGE	GPIO_Pin_0		// Needs confirmation
#define ADC_VOLTAGE	ADC1			// Needs confirmation
#define CHANNEL_VOLTAGE	ADC_Channel_0		// Needs confirmation

#define	BANK_BUTTON	GPIOC			
#define PIN_BUTTON	GPIO_Pin_8
// No ADC available on the GPIOC, so we have to use digital reads/writes for the button

#define	BANK_RED_LED	GPIOB
#define PIN_RED_LED	GPIO_Pin_12

#define	BANK_GREEN_LED	GPIOC
#define PIN_GREEN_LED	GPIO_Pin_6

#define	BANK_YELLOW_LED	GPIOC
#define PIN_YELLOW_LED	GPIO_Pin_7

#define BANK_MOSI	GPIOA
#define PIN_MOSI	GPIO_Pin_7
#define	BANK_SCK	GPIOA
#define PIN_SCK		GPIO_Pin_5
#define BANK_MISO	GPIOA
#define PIN_MISO	GPIO_Pin_6
#define	APBPERIPHERAL_SPI	RCC_APB2Periph_SPI1
#define	PERIPHERAL_SPI	SPI1
#define RCC_SPIPeriphClockCmd RCC_APB2PeriphClockCmd

#define	PIN_USART_TX	GPIO_Pin_2
#define BANK_USART_TX	GPIOA
#define	PIN_USART_RX	GPIO_Pin_3
#define BANK_USART_RX	GPIOA
#define USART_IRQ	USART2_IRQn
#define USART_IT	USART2
#define APBPERIPHERAL_USART	RCC_APB1Periph_USART2
#define USART_IRQ_HANDLER	USART2_IRQHandler

#else
Compiler error.  You must define RS41 or DFM17.
#endif  // RS41 or DFM17

// Hardware Sanity Check

#if defined (RS41) && defined (DFM17)
Compiler error.  You must define RS41 or DFM17 but not both.
#endif

#endif // __GPIO_H
