#include <stddef.h>

#include <stm32f10x_rcc.h>
#include <stm32f10x_gpio.h>
#include <stm32f10x_usart.h>
#include <misc.h>

#include "usart_gps.h"

void (*usart_gps_handle_incoming_byte)(uint8_t data) = NULL;

void usart_gps_init(uint32_t baud_rate, bool enable_irq)
{
    GPIO_InitTypeDef gpio_init;

    // USART1 TX
    gpio_init.GPIO_Pin = GPIO_Pin_9;
    gpio_init.GPIO_Mode = GPIO_Mode_AF_PP;
    gpio_init.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &gpio_init);

    // USART1 RX
    gpio_init.GPIO_Pin = GPIO_Pin_10;
    gpio_init.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOA, &gpio_init);

    NVIC_DisableIRQ(USART1_IRQn);
    USART_ITConfig(USART1, USART_IT_RXNE, DISABLE);
    USART_ClearITPendingBit(USART1, USART_IT_RXNE);
    USART_ClearITPendingBit(USART1, USART_IT_ORE);

    USART_Cmd(USART1, DISABLE);

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);

    USART_InitTypeDef usart_init;
    usart_init.USART_BaudRate = baud_rate;
    usart_init.USART_WordLength = USART_WordLength_8b;
    usart_init.USART_StopBits = USART_StopBits_1;
    usart_init.USART_Parity = USART_Parity_No;
    usart_init.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    usart_init.USART_Mode = USART_Mode_Tx | USART_Mode_Rx;
    USART_Init(USART1, &usart_init);

    NVIC_InitTypeDef nvic_init;
    nvic_init.NVIC_IRQChannel = USART1_IRQn;
    nvic_init.NVIC_IRQChannelPreemptionPriority = 15;
    nvic_init.NVIC_IRQChannelSubPriority = 2;
    nvic_init.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&nvic_init);

    USART_Cmd(USART1, ENABLE);
    if (enable_irq) {
        USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);
        NVIC_EnableIRQ(USART1_IRQn);
    }
}

static void usart_gps_enable_irq(bool enabled) {
    if (enabled) {
        USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);
        NVIC_EnableIRQ(USART1_IRQn);
    } else {
        NVIC_DisableIRQ(USART1_IRQn);
        USART_ITConfig(USART1, USART_IT_RXNE, DISABLE);
    }
    USART_ClearITPendingBit(USART1, USART_IT_RXNE);
    USART_ClearITPendingBit(USART1, USART_IT_ORE);
}

void usart_gps_uninit()
{
    usart_gps_enable_irq(false);
    USART_Cmd(USART1, DISABLE);
    USART_DeInit(USART1);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, DISABLE);
}

void usart_gps_enable(bool enabled)
{
    usart_gps_enable_irq(enabled);
    USART_Cmd(USART1, enabled ? ENABLE : DISABLE);
}

void usart_gps_send_byte(uint8_t data)
{
    while (USART_GetFlagStatus(USART1, USART_FLAG_TC) == RESET) {}
    USART_SendData(USART1, data);
    while (USART_GetFlagStatus(USART1, USART_FLAG_TC) == RESET) {}
}

void USART1_IRQHandler(void)
{
    if (USART_GetITStatus(USART1, USART_IT_RXNE) != RESET) {
        USART_ClearITPendingBit(USART1, USART_IT_RXNE);
        uint8_t data = (uint8_t) USART_ReceiveData(USART1);
        usart_gps_handle_incoming_byte(data);
    } else if (USART_GetITStatus(USART1, USART_IT_ORE) != RESET) {
        USART_ClearITPendingBit(USART1, USART_IT_ORE);
        USART_ReceiveData(USART1);
    } else {
        USART_ReceiveData(USART1);
    }
}
