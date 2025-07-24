#include <stm32f1xx_hal.h>

#include "usart_ext.h"

// USART3 is the external serial port, which shares TX/RX pins with the external I²C bus.
USART_HandleTypeDef usart3;

void usart_ext_init(uint32_t baud_rate)
{
    GPIO_InitTypeDef gpio_init;

    // USART3 TX
    gpio_init.Pin = GPIO_PIN_10;
    gpio_init.Mode = GPIO_MODE_AF_PP;
    gpio_init.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOB, &gpio_init);

    // USART3 RX
    gpio_init.Pin = GPIO_PIN_11;
    gpio_init.Mode = GPIO_MODE_INPUT;
    gpio_init.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOB, &gpio_init);

    HAL_NVIC_DisableIRQ(USART3_IRQn);
    __HAL_UART_DISABLE(&usart3);

    __HAL_RCC_USART3_CLK_ENABLE();

    usart3.Init.BaudRate = baud_rate;
    usart3.Init.WordLength = USART_WORDLENGTH_8B;
    usart3.Init.StopBits = USART_STOPBITS_1;
    usart3.Init.Parity = USART_PARITY_NONE;
    usart3.Init.Mode = USART_MODE_TX; // Enable only transmit for now | USART_Mode_Rx;
    HAL_USART_Init(&usart3);
}

void usart_ext_uninit()
{
    __HAL_UART_DISABLE(&usart3);
    HAL_USART_DeInit(&usart3);
    __HAL_RCC_USART3_CLK_DISABLE();
}

void usart_ext_enable(bool enabled)
{
    if(enabled)
    {
        __HAL_UART_ENABLE(&usart3);
    } else {
        __HAL_UART_DISABLE(&usart3);
    }
}

void usart_ext_send_byte(uint8_t data)
{
    while (__HAL_UART_GET_FLAG(&usart3, UART_FLAG_TC) == RESET) {}
    HAL_USART_Transmit(&usart3, &data, 1, 10);
    while (__HAL_UART_GET_FLAG(&usart3, UART_FLAG_TC) == RESET) {}
}
