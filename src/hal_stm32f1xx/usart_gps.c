#include <stddef.h>

#include <stm32f1xx_hal.h>

#include "usart_gps.h"
#include "gpio.h"
#include "log.h"

void (*usart_gps_handle_incoming_byte)(uint8_t data) = NULL;

UART_HandleTypeDef usart1;

void usart_gps_init(uint32_t baud_rate, bool enable_irq)
{
    GPIO_InitTypeDef gpio_init;

    usart1.Instance = USART1;

    __HAL_RCC_USART1_CLK_ENABLE();

    // Deinit sequence for purpose of baudrate change
    HAL_NVIC_DisableIRQ(USART1_IRQn);
    NVIC_ClearPendingIRQ(USART1_IRQn);
    __HAL_UART_DISABLE_IT(&usart1, USART_IT_TC | USART_IT_TXE | USART_IT_RXNE | USART_IT_ERR);
    if ((usart1.Instance->CR1 & USART_CR1_UE) != 0U)
    {
        while((usart1.Instance->SR & USART_SR_TC) == 0) { __NOP(); }
    }
    __HAL_UART_DISABLE(&usart1);
    HAL_UART_DeInit(&usart1);

    // USART TX
    gpio_init.Pin = PIN_USART_TX;
    gpio_init.Mode = GPIO_MODE_AF_PP;
    gpio_init.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(BANK_USART_TX, &gpio_init);

    // USART RX
    gpio_init.Pin = PIN_USART_RX;
    gpio_init.Mode = GPIO_MODE_INPUT;
    gpio_init.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(BANK_USART_RX, &gpio_init);

    HAL_NVIC_DisableIRQ(USART1_IRQn);
    NVIC_ClearPendingIRQ(USART1_IRQn);
    __HAL_UART_DISABLE_IT(&usart1, USART_IT_RXNE);
    __HAL_UART_CLEAR_OREFLAG(&usart1);

    __HAL_UART_DISABLE(&usart1);

    usart1.Init.BaudRate = baud_rate;
    usart1.Init.WordLength = UART_WORDLENGTH_8B;
    usart1.Init.StopBits = UART_STOPBITS_1;
    usart1.Init.Parity = UART_PARITY_NONE;
    usart1.Init.Mode = USART_MODE_TX_RX; // Enable only transmit for now | USART_Mode_Rx;
    HAL_UART_Init(&usart1);

    HAL_NVIC_SetPriority(USART1_IRQn, 1, 2);

    HAL_UART_RegisterCallback(&usart1, HAL_UART_RX_COMPLETE_CB_ID, USART1_IRQHandler);

    if (enable_irq) {
        HAL_NVIC_EnableIRQ(USART1_IRQn);
        __HAL_UART_ENABLE_IT(&usart1, UART_IT_RXNE);
    }

}

void usart_gps_set_baud_rate(uint32_t baud_rate) {
    __HAL_UART_DISABLE(&usart1);
    usart1.Instance->BRR = UART_BRR_SAMPLING16(HAL_RCC_GetPCLK2Freq(), baud_rate);
    __HAL_UART_ENABLE(&usart1);
}

static void usart_gps_enable_irq(bool enabled) {
    if (enabled) {
        HAL_NVIC_EnableIRQ(USART1_IRQn);
        __HAL_UART_ENABLE_IT(&usart1, USART_IT_RXNE);
    } else {
        HAL_NVIC_DisableIRQ(USART1_IRQn);
        NVIC_ClearPendingIRQ(USART1_IRQn);
        __HAL_UART_DISABLE_IT(&usart1, USART_IT_RXNE);
    }
    __HAL_UART_CLEAR_OREFLAG(&usart1);
}

void usart_gps_uninit()
{
    usart_gps_enable_irq(false);
    __HAL_UART_DISABLE(&usart1);
    HAL_UART_DeInit(&usart1);
    NVIC_ClearPendingIRQ(USART1_IRQn);
    __HAL_RCC_USART1_CLK_DISABLE();
}

void usart_gps_enable(bool enabled)
{
    usart_gps_enable_irq(enabled);
    if(enabled) {
        __HAL_UART_ENABLE(&usart1);
    } else {
        __HAL_UART_DISABLE(&usart1);
    }
}

void usart_gps_send_byte(uint8_t data)
{
    while ((__HAL_UART_GET_FLAG(&usart1, USART_FLAG_TXE)) == RESET) {}
    usart1.Instance->DR = data;
    // optional: wait for TC if absolutely needed
    while ((__HAL_UART_GET_FLAG(&usart1, USART_FLAG_TC)) == RESET) {}
}

void USART1_IRQHandler(UART_HandleTypeDef *huart)
{
    uint32_t sr = READ_REG(usart1.Instance->SR);

    /* Overrun error: clear by reading SR then DR (we already read SR) */
    if (sr & USART_SR_ORE) 
    {
        volatile uint32_t tmp = usart1.Instance->DR; (void)tmp; // read DR to clear ORE
        // log_info("ORE\n");
        return;
    }

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
}