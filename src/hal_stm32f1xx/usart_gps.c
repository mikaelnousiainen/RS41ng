#include <stddef.h>

#include <stm32f1xx_hal.h>

#include "usart_gps.h"
#include "gpio.h"

void (*usart_gps_handle_incoming_byte)(uint8_t data) = NULL;

USART_HandleTypeDef usart1;

void usart_gps_init(uint32_t baud_rate, bool enable_irq)
{
    GPIO_InitTypeDef gpio_init;

    // USART TX
    gpio_init.Pin = PIN_USART_TX;
    gpio_init.Mode = GPIO_MODE_AF_PP;
    gpio_init.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(BANK_USART_TX, &gpio_init);

    // USART3 RX
    gpio_init.Pin = PIN_USART_RX;
    gpio_init.Mode = GPIO_MODE_INPUT;
    gpio_init.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(BANK_USART_RX, &gpio_init);

    __HAL_RCC_USART1_CLK_ENABLE();

    usart1.Instance = USART1;

    HAL_NVIC_DisableIRQ(USART1_IRQn);
    __HAL_USART_DISABLE_IT(&usart1, USART_IT_RXNE);
    __HAL_USART_CLEAR_FLAG(&usart1, USART_FLAG_RXNE); 
    __HAL_USART_CLEAR_OREFLAG(&usart1);

    __HAL_USART_DISABLE(&usart1);

    usart1.Init.BaudRate = baud_rate;
    usart1.Init.WordLength = USART_WORDLENGTH_8B;
    usart1.Init.StopBits = USART_STOPBITS_1;
    usart1.Init.Parity = USART_PARITY_NONE;
    usart1.Init.Mode = USART_MODE_TX_RX; // Enable only transmit for now | USART_Mode_Rx;
    HAL_USART_Init(&usart1);

    HAL_NVIC_SetPriority(USART1_IRQn, 3, 2);

    if (enable_irq) {
        HAL_NVIC_EnableIRQ(USART1_IRQn);
        __HAL_USART_ENABLE_IT(&usart1, USART_IT_RXNE);
    }
}

static void usart_gps_enable_irq(bool enabled) {
    if (enabled) {
        HAL_NVIC_EnableIRQ(USART1_IRQn);
        __HAL_USART_ENABLE_IT(&usart1, USART_IT_RXNE);
    } else {
        HAL_NVIC_DisableIRQ(USART1_IRQn);
        __HAL_USART_DISABLE_IT(&usart1, USART_IT_RXNE);
    }
    __HAL_USART_CLEAR_FLAG(&usart1, USART_FLAG_RXNE); 
    __HAL_USART_CLEAR_OREFLAG(&usart1);
}

void usart_gps_uninit()
{
    usart_gps_enable_irq(false);
    __HAL_USART_DISABLE(&usart1);
    HAL_USART_DeInit(&usart1);
    __HAL_RCC_USART1_CLK_DISABLE();
}

void usart_gps_enable(bool enabled)
{
    usart_gps_enable_irq(enabled);
    if(enabled) {
        __HAL_USART_ENABLE(&usart1);
    } else {
        __HAL_USART_DISABLE(&usart1);
    }
}

void usart_gps_send_byte(uint8_t data)
{
    HAL_USART_Transmit(&usart1, &data, 1, 10);
}

void USART1_IRQHandler(void)
{
    uint32_t sr = READ_REG(usart1.Instance->SR);

    /* RX not empty */
    if (sr & USART_SR_RXNE) 
    {
        uint8_t byte = (uint8_t)(READ_REG(usart1.Instance->DR) & 0xFF); // reading DR clears RXNE
        if(usart_gps_handle_incoming_byte) 
        {
            usart_gps_handle_incoming_byte(byte);
        }
        return;
    }

    /* Overrun error: clear by reading SR then DR (we already read SR) */
    if (sr & USART_SR_ORE) 
    {
        volatile uint32_t tmp = usart1.Instance->DR; (void)tmp; // read DR to clear ORE
        __HAL_USART_CLEAR_OREFLAG(&usart1); // optional helper
        return;
    }

    /* For other events (TXE, TC, ERR), either call HAL IRQ handler or handle explicitly */
    // HAL_USART_IRQHandler(&usart1); // safe for other flags (non-blocking)

    // uint8_t data;
    // if (__HAL_USART_GET_FLAG(&usart1, USART_FLAG_RXNE) != RESET) {
    //     __HAL_USART_CLEAR_FLAG(&usart1, USART_FLAG_RXNE); 
    //     HAL_USART_Receive(&usart1, &data, 1, 10);
    //     usart_gps_handle_incoming_byte(data);
    // } else if (__HAL_USART_GET_FLAG(&usart1, USART_FLAG_ORE) != RESET) {
    //     __HAL_USART_CLEAR_OREFLAG(&usart1);
    //     HAL_USART_Receive(&usart1, &data, 1, 10);
    // } else {
    //     HAL_USART_Receive(&usart1, &data, 1, 10);
    // }
}
