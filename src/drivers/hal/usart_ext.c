#include "config.h"

#if GPS_NMEA_OUTPUT_VIA_SERIAL_PORT_ENABLE

#ifdef RS41_RSM4x4
#include <stm32l4xx_hal.h>
#else
#include <stm32f1xx_hal.h>
#endif

#include "usart_ext.h"
#include "gpio.h"

USART_HandleTypeDef usart_ext_handle;

void usart_ext_init(uint32_t baud_rate)
{
    GPIO_InitTypeDef gpio_init;

    // TX
    gpio_init.Pin = PIN_EXT_USART_TX;
    gpio_init.Mode = GPIO_MODE_AF_PP;
    gpio_init.Speed = GPIO_SPEED_FREQ_HIGH;
#ifdef RS41_RSM4x4
    gpio_init.Alternate = GPIO_AF7_USART3;
#endif
    HAL_GPIO_Init(BANK_EXT_USART_TX, &gpio_init);

    // RX
    gpio_init.Pin = PIN_EXT_USART_RX;
    gpio_init.Mode = GPIO_MODE_INPUT;
    gpio_init.Pull = GPIO_NOPULL;
#ifdef RS41_RSM4x4
    gpio_init.Alternate = GPIO_AF7_USART3;
#endif
    HAL_GPIO_Init(BANK_EXT_USART_RX, &gpio_init);

    HAL_NVIC_DisableIRQ(EXT_USART_IRQn);
    __HAL_UART_DISABLE(&usart_ext_handle);

    EXT_USART_CLK_ENABLE();

    usart_ext_handle.Instance = EXT_USART;
    usart_ext_handle.Init.BaudRate = baud_rate;
    usart_ext_handle.Init.WordLength = USART_WORDLENGTH_8B;
    usart_ext_handle.Init.StopBits = USART_STOPBITS_1;
    usart_ext_handle.Init.Parity = USART_PARITY_NONE;
    usart_ext_handle.Init.Mode = USART_MODE_TX;
    HAL_USART_Init(&usart_ext_handle);
}

void usart_ext_uninit()
{
    __HAL_UART_DISABLE(&usart_ext_handle);
    HAL_USART_DeInit(&usart_ext_handle);
    EXT_USART_CLK_DISABLE();
}

void usart_ext_enable(bool enabled)
{
    if (enabled) {
        __HAL_UART_ENABLE(&usart_ext_handle);
    } else {
        __HAL_UART_DISABLE(&usart_ext_handle);
    }
}

void usart_ext_send_byte(uint8_t data)
{
    while (__HAL_UART_GET_FLAG(&usart_ext_handle, UART_FLAG_TXE) == RESET) {}
#ifdef RS41_RSM4x4
    usart_ext_handle.Instance->TDR = data;
#else
    usart_ext_handle.Instance->DR = data;
#endif
}

#endif /* GPS_NMEA_OUTPUT_VIA_SERIAL_PORT_ENABLE */
