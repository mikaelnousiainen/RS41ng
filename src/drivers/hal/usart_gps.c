#include <stddef.h>
#include "config.h"

#ifdef RS41_RSM4x4
#include <stm32l4xx_hal.h>
#else
#include <stm32f1xx_hal.h>
#endif

#include "usart_gps.h"
#include "gpio.h"
#include "log.h"
#include "delay.h"

volatile uint32_t gps_ints = 0;

void (*usart_gps_handle_incoming_byte)(uint8_t data) = NULL;

UART_HandleTypeDef usart1;

void usart_gps_init(uint32_t baud_rate, bool enable_irq)
{
    GPIO_InitTypeDef gpio_init;

    /* Step 1: Kill IRQ and clear NVIC pending FIRST, before anything else.
     * On STM32L4, soft reset does not reset peripheral registers or the NVIC.
     * USART1 may be left clocked with ISR flags set and NVIC enabled from the
     * previous run, causing the IRQ to fire before usart1.Instance is even set. */
    HAL_NVIC_DisableIRQ(USART1_IRQn);
    NVIC_ClearPendingIRQ(USART1_IRQn);

#ifdef RS41_RSM4x4
    /* Step 2: RCC peripheral reset clears all USART1 registers including
     * stale TXEIE/TCIE/PEIE bits that survive soft reset and fire the IRQ
     * before init completes. This is the only reliable fix. */
    __HAL_RCC_USART1_FORCE_RESET();
    __NOP(); __NOP(); __NOP(); __NOP();
    __HAL_RCC_USART1_RELEASE_RESET();
    __NOP(); __NOP(); __NOP(); __NOP();
#else
    if ((usart1.Instance->CR1 & USART_CR1_UE) != 0U) {
        while((usart1.Instance->SR & USART_SR_TC) == 0) { __NOP(); }
    }
    __HAL_UART_DISABLE(&usart1);
    HAL_UART_DeInit(&usart1);
#endif

    /* Step 3: Enable clock and proceed */
    __HAL_RCC_USART1_CLK_ENABLE();

    usart1.Instance = USART1;

    // GPIO - USART TX
    gpio_init.Pin = PIN_USART_TX;
    gpio_init.Speed = GPIO_SPEED_FREQ_HIGH;
    gpio_init.Mode = GPIO_MODE_AF_PP;
#ifdef RS41_RSM4x4
    gpio_init.Alternate = GPIO_AF7_USART1;
#endif //RS41_RSM4x4
    HAL_GPIO_Init(BANK_USART_TX, &gpio_init);

    // USART RX
    gpio_init.Pin = PIN_USART_RX;
    gpio_init.Pull = GPIO_NOPULL;
    gpio_init.Mode = GPIO_MODE_INPUT;
#ifdef RS41_RSM4x4
    gpio_init.Mode = GPIO_MODE_AF_PP;
    gpio_init.Alternate = GPIO_AF7_USART1;
#endif //RS41_RSM4x4
    HAL_GPIO_Init(BANK_USART_RX, &gpio_init);

#ifdef RS41  // both models of RS41 have a GPS power pin.  Unclear whether it's needed.
    // USART PWR
    gpio_init.Pin = PIN_USART_PWR;
    gpio_init.Mode = GPIO_MODE_OUTPUT_PP;
    gpio_init.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(BANK_USART_PWR, &gpio_init);

    // Enable GPS
    HAL_GPIO_WritePin(BANK_USART_PWR, PIN_USART_PWR, GPIO_PIN_SET);
#endif //RS41 power pin

    HAL_NVIC_DisableIRQ(USART1_IRQn);
    NVIC_ClearPendingIRQ(USART1_IRQn);

    usart1.Init.BaudRate = baud_rate;
    usart1.Init.WordLength = UART_WORDLENGTH_8B;
    usart1.Init.StopBits = UART_STOPBITS_1;
    usart1.Init.Parity = UART_PARITY_NONE;
    usart1.Init.Mode = USART_MODE_TX_RX; // Enable only transmit for now | USART_Mode_Rx;
    usart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    usart1.Init.OverSampling = UART_OVERSAMPLING_16;
    usart1.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
    usart1.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;

    if (HAL_UART_Init(&usart1) != HAL_OK) {
      log_info("HAL_UART_INIT fail\n");
    }

    // Clear all pending error flags before enabling IRQ
#ifdef RS41_RSM4x4
    usart1.Instance->ICR = USART_ICR_ORECF | USART_ICR_FECF | USART_ICR_NECF | USART_ICR_PECF;
    volatile uint32_t tmp = usart1.Instance->RDR; (void)tmp;
#endif
    NVIC_ClearPendingIRQ(USART1_IRQn);

    // Priority 5 > TIM6(3) and TIM2(2) - prevents USART preempting delay_ms()
    HAL_NVIC_SetPriority(USART1_IRQn, 5, 0);

    if (enable_irq) {
        HAL_NVIC_EnableIRQ(USART1_IRQn);
        __HAL_UART_ENABLE_IT(&usart1, UART_IT_RXNE);
        // Enable error interrupt so FE/NE/ORE flags generate an IRQ we can clear
        __HAL_UART_ENABLE_IT(&usart1, UART_IT_ERR);
    }

}

void usart_gps_set_baud_rate(uint32_t baud_rate) {
    log_info("Setting baud to %ld, clock freq is %ld\n",(uint32_t) baud_rate,(uint32_t) HAL_RCC_GetPCLK2Freq());
    delay_ms(1000);
    __HAL_UART_DISABLE(&usart1);
#ifndef RS41_RSM4x4
    usart1.Instance->BRR = UART_BRR_SAMPLING16(HAL_RCC_GetPCLK2Freq(), baud_rate);
#else // Calculated differently for L4
    usart1.Instance->BRR = (uint32_t) (HAL_RCC_GetPCLK2Freq() / baud_rate);
#endif // RSM4x4
    __HAL_UART_ENABLE(&usart1);
}

static void usart_gps_enable_irq(bool enabled) {
    if (enabled) {
        HAL_NVIC_EnableIRQ(USART1_IRQn);
        __HAL_UART_ENABLE_IT(&usart1, USART_IT_RXNE);
        __HAL_UART_ENABLE_IT(&usart1, UART_IT_ERR);
    } else {
        HAL_NVIC_DisableIRQ(USART1_IRQn);
        NVIC_ClearPendingIRQ(USART1_IRQn);
        __HAL_UART_DISABLE_IT(&usart1, USART_IT_RXNE);
        __HAL_UART_DISABLE_IT(&usart1, UART_IT_ERR);
    }
#ifdef RS41_RSM4x4
    usart1.Instance->ICR = USART_ICR_ORECF | USART_ICR_FECF | USART_ICR_NECF | USART_ICR_PECF;
#else
    __HAL_UART_CLEAR_OREFLAG(&usart1);
#endif
}

void usart_gps_uninit()
{
    usart_gps_enable_irq(false);
    __HAL_UART_DISABLE(&usart1);
    HAL_UART_DeInit(&usart1);
    NVIC_ClearPendingIRQ(USART1_IRQn);
    __HAL_RCC_USART1_CLK_DISABLE();

#ifdef RS41  // both models of RS41 have a GPS power pin
    // Disable GPS
    HAL_GPIO_WritePin(BANK_USART_PWR, PIN_USART_PWR, GPIO_PIN_RESET);
#endif //RS41 power pin

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
#ifdef RS41_RSM4x4
    usart1.Instance->TDR = data;
#else
    usart1.Instance->DR = data;
#endif
    // optional: wait for TC if absolutely needed
    while ((__HAL_UART_GET_FLAG(&usart1, USART_FLAG_TC)) == RESET) {}
}

#ifdef RS41_RSM4x4
void USART1_IRQHandler(UART_HandleTypeDef *huart)
{
    gps_ints++;
    uint32_t isr = READ_REG(usart1.Instance->ISR);

    /* Clear ALL error flags atomically at entry.
     * On STM32L4, PE/FE/NE/ORE all set simultaneously when GPS powers up
     * and must be cleared via ICR in one write - clearing one at a time
     * with early returns causes the ISR to re-fire for each remaining flag. */
    if (isr & (USART_ISR_PE | USART_ISR_FE | USART_ISR_NE | USART_ISR_ORE))
    {
        usart1.Instance->ICR = USART_ICR_PECF | USART_ICR_FECF |
                               USART_ICR_NECF | USART_ICR_ORECF;
        /* If RXNE is also set alongside errors, the byte is likely corrupt -
         * read and discard RDR to clear RXNE, then return. */
        if (isr & USART_ISR_RXNE)
        {
            volatile uint32_t tmp = usart1.Instance->RDR; (void)tmp;
        }
        return;
    }

    /* RX not empty - clean byte with no errors */
    if (isr & USART_ISR_RXNE)
    {
        uint8_t byte = (uint8_t)(READ_REG(usart1.Instance->RDR) & 0xFF);
        if (usart_gps_handle_incoming_byte)
        {
            usart_gps_handle_incoming_byte(byte);
        }
        return;
    }

    /* Catch-all: clear anything else to prevent re-entry */
    usart1.Instance->ICR = 0xFFFFFFFF;
}
#else // stm32f1xx
void USART1_IRQHandler(UART_HandleTypeDef *huart)
{
    gps_ints++;		// Bump the global counter
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
#endif
