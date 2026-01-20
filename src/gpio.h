#ifndef __GPIO_H
#define __GPIO_H

#ifndef RS41_RSM4x4
    #include <stm32f1xx_hal.h>
#else
    #include <stm32l4xx_hal.h>
#endif

#include "config.h"


// GPIO definitions for devices we use

#if defined (RS41)
#if defined (RS41_RSM4x4)
// RSM4x4 pins
#define BANK_SHUTDOWN	GPIOA
#define PIN_SHUTDOWN	GPIO_PIN_9

#define BANK_VOLTAGE	GPIOA
#define PIN_VOLTAGE	GPIO_PIN_5
#define ADC_VOLTAGE	ADC1
#define CHANNEL_VOLTAGE	ADC_CHANNEL_10

#define	BANK_BUTTON	GPIOA
#define PIN_BUTTON	GPIO_PIN_6
#define ADC_BUTTON	ADC1
#define CHANNEL_BUTTON  ADC_CHANNEL_11

#define	BANK_RED_LED	GPIOC
#define PIN_RED_LED	GPIO_PIN_8

#define	BANK_GREEN_LED	GPIOC
#define PIN_GREEN_LED	GPIO_PIN_7

#define BANK_SDN	GPIOC
#define PIN_SDN		GPIO_PIN_3
#define BANK_MOSI	GPIOB
#define PIN_MOSI	GPIO_PIN_15
#define	BANK_SCK	GPIOB
#define PIN_SCK		GPIO_PIN_13
#define BANK_MISO	GPIOB
#define PIN_MISO	GPIO_PIN_14
#define BANK_NSEL	GPIOC
#define PIN_NSEL	GPIO_PIN_13
#define	APBPERIPHERAL_SPI	RCC_APB1Periph_SPI2
#define	PERIPHERAL_SPI	SPI2
#define RCC_SPIPeriphClockCmd RCC_APB1PeriphClockCmd
#define BANK_SI4063_GPIO2	GPIOD
#define PIN_SI4063_GPIO2	GPIO_PIN_0
#define BANK_SI4063_GPIO3	GPIOA
#define PIN_SI4063_GPIO3	GPIO_PIN_4

#define BANK_USART_TX	GPIOB
#define	PIN_USART_TX	GPIO_PIN_6
#define BANK_USART_RX	GPIOB
#define	PIN_USART_RX	GPIO_PIN_7
#define BANK_USART_PWR	GPIOB
#define	PIN_USART_PWR	GPIO_PIN_9
#define USART_IRQ	USART1_IRQn
#define USART_IT	USART1
#define APBPERIPHERAL_USART	RCC_APB2Periph_USART1
#define USART_IRQ_HANDLER	USART1_IRQHandler

#define BANK_PULSE	GPIOB
#define PIN_PULSE	GPIO_PIN_11

#define BANK_PULSE	GPIOB
#define PIN_PULSE	GPIO_PIN_11
#define EXTI_LINE_PULSE	EXTI_LINE_11
#define EXTI_BANK_PULSE	EXTI_GPIOB
#define EXTI_VECTOR_PULSE 	EXTI15_10_IRQn


#else ///////////////////////////////////  It's a classic RSM4x2 ///////////////////////////////////
#define BANK_SHUTDOWN	GPIOA
#define PIN_SHUTDOWN	GPIO_PIN_12

#define BANK_VOLTAGE	GPIOA
#define PIN_VOLTAGE	GPIO_PIN_5
#define ADC_VOLTAGE	ADC1
#define CHANNEL_VOLTAGE	ADC_CHANNEL_5

#define	BANK_BUTTON	GPIOA
#define PIN_BUTTON	GPIO_PIN_6
#define ADC_BUTTON	ADC1
#define CHANNEL_BUTTON  ADC_CHANNEL_6

#define	BANK_RED_LED	GPIOB
#define PIN_RED_LED	GPIO_PIN_8

#define	BANK_GREEN_LED	GPIOB
#define PIN_GREEN_LED	GPIO_PIN_7

#define BANK_SDN	GPIOC
#define PIN_SDN		GPIO_PIN_3
#define BANK_MOSI	GPIOB
#define PIN_MOSI	GPIO_PIN_15
#define	BANK_SCK	GPIOB
#define PIN_SCK		GPIO_PIN_13
#define BANK_MISO	GPIOB
#define PIN_MISO	GPIO_PIN_14
#define BANK_NSEL	GPIOC
#define PIN_NSEL	GPIO_PIN_13
#define	APBPERIPHERAL_SPI	RCC_APB1Periph_SPI2
#define	PERIPHERAL_SPI	SPI2
#define RCC_SPIPeriphClockCmd RCC_APB1PeriphClockCmd
#define BANK_SI4063_GPIO2	GPIOD
#define PIN_SI4063_GPIO2	GPIO_PIN_0
#define BANK_SI4063_GPIO3	GPIOA
#define PIN_SI4063_GPIO3	GPIO_PIN_4

#define	PIN_USART_TX	GPIO_PIN_9
#define BANK_USART_TX	GPIOA
#define	PIN_USART_RX	GPIO_PIN_10
#define BANK_USART_RX	GPIOA
#define BANK_USART_PWR	GPIOA
#define	PIN_USART_PWR	GPIO_PIN_15
#define USART_IRQ	USART1_IRQn
#define USART_IT	USART1
#define APBPERIPHERAL_USART	RCC_APB2Periph_USART1
#define USART_IRQ_HANDLER	USART1_IRQHandler

#define BANK_PULSE	GPIOB
#define PIN_PULSE	GPIO_PIN_11
#define EXTI_LINE_PULSE	EXTI_LINE_11
#define EXTI_BANK_PULSE	EXTI_GPIOB
#define EXTI_VECTOR_PULSE 	EXTI15_10_IRQn

#endif // Classic RS41

///////////////////////////////////  It's a DFM17 ///////////////////////////////////
#elif defined (DFM17)

#define BANK_SHUTDOWN	GPIOC			
#define PIN_SHUTDOWN	GPIO_PIN_0

#define BANK_VOLTAGE	GPIOA			// Needs confirmation
#define PIN_VOLTAGE	GPIO_PIN_0		// Needs confirmation
#define ADC_VOLTAGE	ADC1			// Needs confirmation
#define CHANNEL_VOLTAGE	ADC_CHANNEL_0		// Needs confirmation

#define	BANK_BUTTON	GPIOC			
#define PIN_BUTTON	GPIO_PIN_8
// No ADC available on the GPIOC, so we have to use digital reads/writes for the button

#define	BANK_RED_LED	GPIOB
#define PIN_RED_LED	GPIO_PIN_12

#define	BANK_GREEN_LED	GPIOC
#define PIN_GREEN_LED	GPIO_PIN_6

#define	BANK_YELLOW_LED	GPIOC
#define PIN_YELLOW_LED	GPIO_PIN_7

#define BANK_SDN	GPIOC
#define PIN_SDN		GPIO_PIN_3
#define BANK_MOSI	GPIOA
#define PIN_MOSI	GPIO_PIN_7
#define	BANK_SCK	GPIOA
#define PIN_SCK		GPIO_PIN_5
#define BANK_MISO	GPIOA
#define PIN_MISO	GPIO_PIN_6
#define BANK_NSEL	GPIOB
#define PIN_NSEL	GPIO_PIN_2
#define BANK_SI4063_GPIO2	GPIOD
#define PIN_SI4063_GPIO2	GPIO_PIN_0
#define BANK_SI4063_GPIO3	GPIOA
#define PIN_SI4063_GPIO3	GPIO_PIN_4
#define	APBPERIPHERAL_SPI	RCC_APB2Periph_SPI1
#define	PERIPHERAL_SPI	SPI1
#define RCC_SPIPeriphClockCmd RCC_APB2PeriphClockCmd

#define	PIN_USART_TX	GPIO_PIN_2
#define BANK_USART_TX	GPIOA
#define	PIN_USART_RX	GPIO_PIN_3
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
