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

/*
 * Hardware abstraction macros.
 * gpio.h provides USART_IT (instance), USART_IRQ (IRQn), and USART_IRQ_HANDLER.
 * We add clock enable/disable and the correct PCLK source for baud rate calculation.
 * USART1 sits on APB2 (PCLK2); USART2 sits on APB1 (PCLK1).
 */
#if defined(DFM17)
#define GPS_USART_CLK_ENABLE()   __HAL_RCC_USART2_CLK_ENABLE()
#define GPS_USART_CLK_DISABLE()  __HAL_RCC_USART2_CLK_DISABLE()
#define GPS_USART_PCLK_FREQ()   HAL_RCC_GetPCLK1Freq()
#else /* RS41 — both RSM4x2 and RSM4x4 use USART1 */
#define GPS_USART_CLK_ENABLE()   __HAL_RCC_USART1_CLK_ENABLE()
#define GPS_USART_CLK_DISABLE()  __HAL_RCC_USART1_CLK_DISABLE()
#define GPS_USART_PCLK_FREQ()   HAL_RCC_GetPCLK2Freq()
#endif

volatile uint32_t gps_ints = 0;

void (*usart_gps_handle_incoming_byte)(uint8_t data) = NULL;

UART_HandleTypeDef gps_usart;

void usart_gps_init(uint32_t baud_rate, bool enable_irq)
{
    GPIO_InitTypeDef gpio_init;

    /* Step 1: Kill IRQ and clear NVIC pending FIRST, before anything else.
     * On STM32L4, soft reset does not reset peripheral registers or the NVIC.
     * The USART may be left clocked with ISR flags set and NVIC enabled from the
     * previous run, causing the IRQ to fire before gps_usart.Instance is even set. */
    HAL_NVIC_DisableIRQ(USART_IRQ);
    NVIC_ClearPendingIRQ(USART_IRQ);

#ifdef RS41_RSM4x4
    /* Step 2: RCC peripheral reset clears all USART registers including
     * stale TXEIE/TCIE/PEIE bits that survive soft reset and fire the IRQ
     * before init completes. This is the only reliable fix. */
    __HAL_RCC_USART1_FORCE_RESET();
    __NOP(); __NOP(); __NOP(); __NOP();
    __HAL_RCC_USART1_RELEASE_RESET();
    __NOP(); __NOP(); __NOP(); __NOP();
#else
    if (gps_usart.Instance != NULL) {
        if ((gps_usart.Instance->CR1 & USART_CR1_UE) != 0U) {
            while((gps_usart.Instance->SR & USART_SR_TC) == 0) { __NOP(); }
        }
        __HAL_UART_DISABLE(&gps_usart);
        HAL_UART_DeInit(&gps_usart);
    }
#endif

    /* Step 3: Enable clock and proceed */
    GPS_USART_CLK_ENABLE();

    gps_usart.Instance = USART_IT;

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

    HAL_NVIC_DisableIRQ(USART_IRQ);
    NVIC_ClearPendingIRQ(USART_IRQ);

    gps_usart.Init.BaudRate = baud_rate;
    gps_usart.Init.WordLength = UART_WORDLENGTH_8B;
    gps_usart.Init.StopBits = UART_STOPBITS_1;
    gps_usart.Init.Parity = UART_PARITY_NONE;
    gps_usart.Init.Mode = USART_MODE_TX_RX; // Enable only transmit for now | USART_Mode_Rx;
    gps_usart.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    gps_usart.Init.OverSampling = UART_OVERSAMPLING_16;
#ifdef RS41_RSM4x4
    gps_usart.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
    gps_usart.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
#endif

    if (HAL_UART_Init(&gps_usart) != HAL_OK) {
      log_info("HAL_UART_INIT fail\n");
    }

    // Clear all pending error flags before enabling IRQ
#ifdef RS41_RSM4x4
    gps_usart.Instance->ICR = USART_ICR_ORECF | USART_ICR_FECF | USART_ICR_NECF | USART_ICR_PECF;
    volatile uint32_t tmp = gps_usart.Instance->RDR; (void)tmp;
#endif
    NVIC_ClearPendingIRQ(USART_IRQ);

    // Priority 5 > TIM6(3) and TIM2(2) - prevents USART preempting delay_ms()
    HAL_NVIC_SetPriority(USART_IRQ, 5, 0);

    if (enable_irq) {
        HAL_NVIC_EnableIRQ(USART_IRQ);
        __HAL_UART_ENABLE_IT(&gps_usart, UART_IT_RXNE);
        // Enable error interrupt so FE/NE/ORE flags generate an IRQ we can clear
        __HAL_UART_ENABLE_IT(&gps_usart, UART_IT_ERR);
    }

}

void usart_gps_set_baud_rate(uint32_t baud_rate) {
    log_info("Setting baud to %ld, clock freq is %ld\n",(uint32_t) baud_rate,(uint32_t) GPS_USART_PCLK_FREQ());
    delay_ms(1000);
    __HAL_UART_DISABLE(&gps_usart);
#ifndef RS41_RSM4x4
    gps_usart.Instance->BRR = UART_BRR_SAMPLING16(GPS_USART_PCLK_FREQ(), baud_rate);
#else // Calculated differently for L4
    gps_usart.Instance->BRR = (uint32_t) (GPS_USART_PCLK_FREQ() / baud_rate);
#endif // RSM4x4
    __HAL_UART_ENABLE(&gps_usart);
}

static void usart_gps_enable_irq(bool enabled) {
    if (enabled) {
        HAL_NVIC_EnableIRQ(USART_IRQ);
        __HAL_UART_ENABLE_IT(&gps_usart, USART_IT_RXNE);
        __HAL_UART_ENABLE_IT(&gps_usart, UART_IT_ERR);
    } else {
        HAL_NVIC_DisableIRQ(USART_IRQ);
        NVIC_ClearPendingIRQ(USART_IRQ);
        __HAL_UART_DISABLE_IT(&gps_usart, USART_IT_RXNE);
        __HAL_UART_DISABLE_IT(&gps_usart, UART_IT_ERR);
    }
#ifdef RS41_RSM4x4
    gps_usart.Instance->ICR = USART_ICR_ORECF | USART_ICR_FECF | USART_ICR_NECF | USART_ICR_PECF;
#else
    __HAL_UART_CLEAR_OREFLAG(&gps_usart);
#endif
}

void usart_gps_uninit()
{
    usart_gps_enable_irq(false);
    __HAL_UART_DISABLE(&gps_usart);
    HAL_UART_DeInit(&gps_usart);
    NVIC_ClearPendingIRQ(USART_IRQ);
    GPS_USART_CLK_DISABLE();

#ifdef RS41  // both models of RS41 have a GPS power pin
    // Disable GPS
    HAL_GPIO_WritePin(BANK_USART_PWR, PIN_USART_PWR, GPIO_PIN_RESET);
#endif //RS41 power pin

}

void usart_gps_enable(bool enabled)
{
    usart_gps_enable_irq(enabled);
    if(enabled) {
        __HAL_UART_ENABLE(&gps_usart);
    } else {
        __HAL_UART_DISABLE(&gps_usart);
    }
}

void usart_gps_send_byte(uint8_t data)
{
    while ((__HAL_UART_GET_FLAG(&gps_usart, USART_FLAG_TXE)) == RESET) {}
#ifdef RS41_RSM4x4
    gps_usart.Instance->TDR = data;
#else
    gps_usart.Instance->DR = data;
#endif
    // optional: wait for TC if absolutely needed
    while ((__HAL_UART_GET_FLAG(&gps_usart, USART_FLAG_TC)) == RESET) {}
}

#ifdef RS41_RSM4x4
void USART_IRQ_HANDLER(UART_HandleTypeDef *huart)
{
    gps_ints++;
    uint32_t isr = READ_REG(gps_usart.Instance->ISR);

    /* Clear ALL error flags atomically at entry.
     * On STM32L4, PE/FE/NE/ORE all set simultaneously when GPS powers up
     * and must be cleared via ICR in one write - clearing one at a time
     * with early returns causes the ISR to re-fire for each remaining flag. */
    if (isr & (USART_ISR_PE | USART_ISR_FE | USART_ISR_NE | USART_ISR_ORE))
    {
        gps_usart.Instance->ICR = USART_ICR_PECF | USART_ICR_FECF |
                               USART_ICR_NECF | USART_ICR_ORECF;
        /* If RXNE is also set alongside errors, the byte is likely corrupt -
         * read and discard RDR to clear RXNE, then return. */
        if (isr & USART_ISR_RXNE)
        {
            volatile uint32_t tmp = gps_usart.Instance->RDR; (void)tmp;
        }
        return;
    }

    /* RX not empty - clean byte with no errors */
    if (isr & USART_ISR_RXNE)
    {
        uint8_t byte = (uint8_t)(READ_REG(gps_usart.Instance->RDR) & 0xFF);
        if (usart_gps_handle_incoming_byte)
        {
            usart_gps_handle_incoming_byte(byte);
        }
        return;
    }

    /* Catch-all: clear anything else to prevent re-entry */
    gps_usart.Instance->ICR = 0xFFFFFFFF;
}
#else // stm32f1xx
void USART_IRQ_HANDLER(UART_HandleTypeDef *huart)
{
    gps_ints++;		// Bump the global counter
    uint32_t sr = READ_REG(gps_usart.Instance->SR);

    /* Overrun error: clear by reading SR then DR (we already read SR) */
    if (sr & USART_SR_ORE)
    {
        volatile uint32_t tmp = gps_usart.Instance->DR; (void)tmp; // read DR to clear ORE
        // log_info("ORE\n");
        return;
    }

    /* RX not empty */
    if (sr & USART_SR_RXNE)
    {
        uint8_t byte = (uint8_t)(READ_REG(gps_usart.Instance->DR) & 0xFF); // reading DR clears RXNE
        if(usart_gps_handle_incoming_byte)
        {
            usart_gps_handle_incoming_byte(byte);
        }
        return;
    }
}
#endif
