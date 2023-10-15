#include <stddef.h>

#include <stm32f10x_rcc.h>
#include <stm32f10x_gpio.h>
#include <stm32f10x_usart.h>
#include <misc.h>

#include "usart_gps.h"
#include "gpio.h"

void (*usart_gps_handle_incoming_byte)(uint8_t data) = NULL;

void usart_gps_init(uint32_t baud_rate, bool enable_irq)
{
    GPIO_InitTypeDef gpio_init;

    // USART TX
    gpio_init.GPIO_Pin = PIN_USART_TX;
    gpio_init.GPIO_Mode = GPIO_Mode_AF_PP;
    gpio_init.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(BANK_USART_TX, &gpio_init);

    // USART RX
    gpio_init.GPIO_Pin = PIN_USART_RX;
    gpio_init.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(BANK_USART_RX, &gpio_init);

    NVIC_DisableIRQ(USART_IRQ);
    USART_ITConfig(USART_IT, USART_IT_RXNE, DISABLE);
    USART_ClearITPendingBit(USART_IT, USART_IT_RXNE);
    USART_ClearITPendingBit(USART_IT, USART_IT_ORE);

    USART_Cmd(USART_IT, DISABLE);

#if defined(RS41)
    RCC_APB2PeriphClockCmd(APBPERIPHERAL_USART, ENABLE);
#elif defined(DFM17)
    RCC_APB1PeriphClockCmd(APBPERIPHERAL_USART, ENABLE);
#endif 

    USART_InitTypeDef usart_init;
    usart_init.USART_BaudRate = baud_rate;
    usart_init.USART_WordLength = USART_WordLength_8b;
    usart_init.USART_StopBits = USART_StopBits_1;
    usart_init.USART_Parity = USART_Parity_No;
    usart_init.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    usart_init.USART_Mode = USART_Mode_Tx | USART_Mode_Rx;
    USART_Init(USART_IT, &usart_init);

    NVIC_InitTypeDef nvic_init;
    nvic_init.NVIC_IRQChannel = USART_IRQ;
    nvic_init.NVIC_IRQChannelPreemptionPriority = 3;
    nvic_init.NVIC_IRQChannelSubPriority = 2;
    nvic_init.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&nvic_init);

    USART_Cmd(USART_IT, ENABLE);
    if (enable_irq) {
        USART_ITConfig(USART_IT, USART_IT_RXNE, ENABLE);
        NVIC_EnableIRQ(USART_IRQ);
    }
}

static void usart_gps_enable_irq(bool enabled) {
    if (enabled) {
        USART_ITConfig(USART_IT, USART_IT_RXNE, ENABLE);
        NVIC_EnableIRQ(USART_IRQ);
    } else {
        NVIC_DisableIRQ(USART_IRQ);
        USART_ITConfig(USART_IT, USART_IT_RXNE, DISABLE);
    }
    USART_ClearITPendingBit(USART_IT, USART_IT_RXNE);
    USART_ClearITPendingBit(USART_IT, USART_IT_ORE);
}

void usart_gps_uninit()
{
    usart_gps_enable_irq(false);
    USART_Cmd(USART_IT, DISABLE);
    USART_DeInit(USART_IT);
#if defined(RS41)
    RCC_APB2PeriphClockCmd(APBPERIPHERAL_USART, DISABLE);
#elif defined(DFM17)
    RCC_APB1PeriphClockCmd(APBPERIPHERAL_USART, DISABLE);
#endif
}

void usart_gps_enable(bool enabled)
{
    usart_gps_enable_irq(enabled);
    USART_Cmd(USART_IT, enabled ? ENABLE : DISABLE);
}

void usart_gps_send_byte(uint8_t data)
{
    while (USART_GetFlagStatus(USART_IT, USART_FLAG_TC) == RESET) {}
    USART_SendData(USART_IT, data);
    while (USART_GetFlagStatus(USART_IT, USART_FLAG_TC) == RESET) {}
}

void USART_IRQ_HANDLER(void)
{
    if (USART_GetITStatus(USART_IT, USART_IT_RXNE) != RESET) {
        USART_ClearITPendingBit(USART_IT, USART_IT_RXNE);
        uint8_t data = (uint8_t) USART_ReceiveData(USART_IT);
        usart_gps_handle_incoming_byte(data);
    } else if (USART_GetITStatus(USART_IT, USART_IT_ORE) != RESET) {
        USART_ClearITPendingBit(USART_IT, USART_IT_ORE);
        USART_ReceiveData(USART_IT);
    } else {
        USART_ReceiveData(USART_IT);
    }
}
