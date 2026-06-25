#include <stddef.h>
#include <string.h>
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
 * gpio.h provides GPS_USART_IT (instance), GPS_USART_IRQ (IRQn), and GPS_USART_IRQ_HANDLER.
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

/*
 * DMA channel mapping for GPS USART RX.
 * STM32F1: fixed mapping — USART1_RX = DMA1_Ch5, USART2_RX = DMA1_Ch6
 * STM32L4: any channel via CSELR mux — we use DMA1_Ch5 with request 2 (USART1_RX)
 */
#if defined(DFM17)
#define GPS_DMA_CHANNEL     DMA1_Channel6
#define GPS_DMA_IRQn        DMA1_Channel6_IRQn
#elif defined(RS41_RSM4x4)
#define GPS_DMA_CHANNEL     DMA1_Channel5
#define GPS_DMA_IRQn        DMA1_Channel5_IRQn
#define GPS_DMA_REQUEST     2   /* USART1_RX on STM32L412 CSELR */
#else  /* RS41_RSM4x2 */
#define GPS_DMA_CHANNEL     DMA1_Channel5
#define GPS_DMA_IRQn        DMA1_Channel5_IRQn
#endif

#define GPS_DMA_BUF_SIZE    256

volatile uint32_t gps_ints = 0;
volatile uint32_t drain_interrupted = 0;
volatile uint32_t drain_not_enabled = 0;
volatile uint32_t drain_null_instance = 0;
volatile uint32_t drain_dma_not_running = 0;
volatile uint32_t drain_byte_calls = 0;
volatile uint32_t reset_gps_parse = 0;


void (*usart_gps_handle_incoming_byte)(uint8_t data, uint8_t reset) = NULL;

UART_HandleTypeDef gps_usart;

static DMA_HandleTypeDef hdma_usart_rx;
static uint8_t dma_rx_buf[GPS_DMA_BUF_SIZE];
static volatile uint16_t dma_rd_pos = 0;
static volatile bool dma_drain_enabled = true;

static void dma_rx_init(void)
{
    __HAL_RCC_DMA1_CLK_ENABLE();

    /* Force-stop any running DMA and clear HAL handle state.
     * HAL_DMA_Start_IT locks the handle and expects a DMA ISR to unlock it,
     * but we never enable the DMA NVIC interrupt.  On re-init the stale lock
     * causes HAL_DMA_Init to silently return HAL_BUSY, leaving CCR unconfigured
     * (MINC=0 means circular buffer broken). */
    if (hdma_usart_rx.Instance != NULL) {
        hdma_usart_rx.Instance->CCR &= ~DMA_CCR_EN;
    }
    hdma_usart_rx.Lock  = HAL_UNLOCKED;
    hdma_usart_rx.State = HAL_DMA_STATE_RESET;
    gps_usart.Lock      = HAL_UNLOCKED;
    gps_usart.RxState   = HAL_UART_STATE_READY;

    hdma_usart_rx.Instance = GPS_DMA_CHANNEL;
#ifdef RS41_RSM4x4
    hdma_usart_rx.Init.Request = GPS_DMA_REQUEST;
#endif
    hdma_usart_rx.Init.Direction = DMA_PERIPH_TO_MEMORY;
    hdma_usart_rx.Init.PeriphInc = DMA_PINC_DISABLE;
    hdma_usart_rx.Init.MemInc = DMA_MINC_ENABLE;
    hdma_usart_rx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    hdma_usart_rx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
    hdma_usart_rx.Init.Mode = DMA_CIRCULAR;
    hdma_usart_rx.Init.Priority = DMA_PRIORITY_HIGH;

    HAL_DMA_Init(&hdma_usart_rx);

    __HAL_LINKDMA(&gps_usart, hdmarx, hdma_usart_rx);

    dma_rd_pos = 0;

    /* Start circular DMA: USART RDR/DR -> dma_rx_buf */
    HAL_UART_Receive_DMA(&gps_usart, dma_rx_buf, GPS_DMA_BUF_SIZE);

    /* We don't need DMA half-transfer or transfer-complete interrupts —
     * we poll NDTR from the main loop. Disable them. */
    __HAL_DMA_DISABLE_IT(&hdma_usart_rx, DMA_IT_HT | DMA_IT_TC);
}

static void dma_rx_stop(void)
{
    HAL_UART_DMAStop(&gps_usart);
    dma_rd_pos = 0;
}

static void dma_rx_restart(void)
{
    dma_rd_pos = 0;
    HAL_UART_Receive_DMA(&gps_usart, dma_rx_buf, GPS_DMA_BUF_SIZE);
    __HAL_DMA_DISABLE_IT(&hdma_usart_rx, DMA_IT_HT | DMA_IT_TC);
}

static void dma_rx_recover(void)
{
    DMA_Channel_TypeDef *ch = hdma_usart_rx.Instance;

    /* Clear all DMA error flags for this channel */
#ifdef RS41_RSM4x4
    // hdma_usart_rx.DmaBaseAddress->IFCR = DMA_FLAG_GL5 | DMA_FLAG_TE5;
    hdma_usart_rx.DmaBaseAddress->IFCR = DMA_ISR_GIF5 | DMA_ISR_TEIF5;
#elif defined(DFM17)
    DMA1->IFCR = DMA_ISR_GIF6 | DMA_ISR_TEIF6;
#else
    DMA1->IFCR = DMA_ISR_GIF5 | DMA_ISR_TEIF5;
#endif

    /* Clear USART error flags */
#ifdef RS41_RSM4x4
    gps_usart.Instance->ICR = USART_ICR_ORECF | USART_ICR_FECF | USART_ICR_NECF | USART_ICR_PECF;
    volatile uint32_t tmp = gps_usart.Instance->RDR; (void)tmp;
#else
    volatile uint32_t sr = gps_usart.Instance->SR;
    volatile uint32_t dr = gps_usart.Instance->DR;
    (void)sr; (void)dr;
#endif

    /* Disable the channel, reconfigure, and re-enable */
    ch->CCR &= ~DMA_CCR_EN;
    ch->CNDTR = GPS_DMA_BUF_SIZE;
    ch->CPAR = (uint32_t)&gps_usart.Instance->
#ifdef RS41_RSM4x4
        RDR;
#else
        DR;
#endif
    ch->CMAR = (uint32_t)dma_rx_buf;
    ch->CCR |= DMA_CCR_EN;

    /* Ensure USART DMA receive request is enabled */
    gps_usart.Instance->CR3 |= USART_CR3_DMAR;

    dma_rd_pos = 0;
    reset_gps_parse = 1;    // We reset our read position.  Any in-flight GPS strings are garbage.  Reset parsers.
}

void usart_gps_drain_dma(void)
{
    static volatile bool draining = false;
    if (draining) {
       drain_interrupted++;
       return;           // ISR hit us mid-drain — skip
    }
    if (!dma_drain_enabled) {
       drain_not_enabled++;
       return;
    }
    if (hdma_usart_rx.Instance == NULL) {
       drain_null_instance++;
       return;
    }

    draining = true;

    /* Check if DMA channel is still running. It can stop due to:
     * - DMA transfer error (clears EN bit automatically)
     * - USART error causing DMAR to be cleared by HAL error paths
     * If stopped, restart it. */
    DMA_Channel_TypeDef *ch = hdma_usart_rx.Instance;
    if (!(ch->CCR & DMA_CCR_EN) || !(gps_usart.Instance->CR3 & USART_CR3_DMAR)) {
        drain_dma_not_running++;
        dma_rx_recover();
        draining = false;
        return;
    }

    /* Clear USART error flags (ORE/FE/NE) to prevent them from
     * inhibiting DMA requests on some STM32 variants. */
#ifdef RS41_RSM4x4
    uint32_t isr = gps_usart.Instance->ISR;
    if (isr & (USART_ISR_ORE | USART_ISR_FE | USART_ISR_NE)) {
        gps_usart.Instance->ICR = USART_ICR_ORECF | USART_ICR_FECF | USART_ICR_NECF;
    }
#else
    volatile uint32_t sr = gps_usart.Instance->SR;
    if (sr & (USART_SR_ORE | USART_SR_FE | USART_SR_NE)) {
        volatile uint32_t dr_discard = gps_usart.Instance->DR;
        (void)dr_discard;
    }
#endif

    uint16_t wr_pos = GPS_DMA_BUF_SIZE - __HAL_DMA_GET_COUNTER(&hdma_usart_rx);

    while (dma_rd_pos != wr_pos) {
        uint8_t byte = dma_rx_buf[dma_rd_pos];
        dma_rd_pos = (dma_rd_pos + 1) % GPS_DMA_BUF_SIZE;
        if (usart_gps_handle_incoming_byte) {
            drain_byte_calls++;
            usart_gps_handle_incoming_byte(byte,reset_gps_parse);
        }
        reset_gps_parse = 0;	// We're processing bytes.  Clear the parse reset until/unless something bad happens.
    }

    draining = false;
}

void usart_gps_init(uint32_t baud_rate, bool enable_irq)
{
    GPIO_InitTypeDef gpio_init;

    /* Step 1: Kill any stale IRQ from previous run */
    HAL_NVIC_DisableIRQ(GPS_USART_IRQ);
    NVIC_ClearPendingIRQ(GPS_USART_IRQ);

#ifdef RS41_RSM4x4
    /* RCC peripheral reset clears all USART registers including
     * stale TXEIE/TCIE/PEIE bits that survive soft reset. */
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

    /* Step 2: Enable clock and proceed */
    GPS_USART_CLK_ENABLE();

    gps_usart.Instance = GPS_USART_IT;

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

#ifdef RS41  // both models of RS41 have a GPS reset_n pin. This must be asserted for the GPS to boot.
    // USART PWR
    gpio_init.Pin = GPS_RESET_N;
    gpio_init.Mode = GPIO_MODE_OUTPUT_PP;
    gpio_init.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(BANK_USART_PWR, &gpio_init);

    // Enable GPS
    HAL_GPIO_WritePin(BANK_USART_PWR, GPS_RESET_N, GPIO_PIN_SET);
#endif //RS41 power pin

    gps_usart.Init.BaudRate = baud_rate;
    gps_usart.Init.WordLength = UART_WORDLENGTH_8B;
    gps_usart.Init.StopBits = UART_STOPBITS_1;
    gps_usart.Init.Parity = UART_PARITY_NONE;
    gps_usart.Init.Mode = USART_MODE_TX_RX;
    gps_usart.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    gps_usart.Init.OverSampling = UART_OVERSAMPLING_16;
#ifdef RS41_RSM4x4
    gps_usart.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
    gps_usart.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
#endif

    if (HAL_UART_Init(&gps_usart) != HAL_OK) {
      log_info("HAL_UART_INIT fail\n");
    }

    // Clear all pending error flags
#ifdef RS41_RSM4x4
    gps_usart.Instance->ICR = USART_ICR_ORECF | USART_ICR_FECF | USART_ICR_NECF | USART_ICR_PECF;
    volatile uint32_t tmp = gps_usart.Instance->RDR; (void)tmp;
#endif

    /* Start DMA-based circular receive (replaces RXNE interrupt) */
    if (enable_irq) {
        dma_rx_init();
    }
}

void usart_gps_set_baud_rate(uint32_t baud_rate) {
    log_info("Setting baud to %ld, clock freq is %ld\n",(uint32_t) baud_rate,(uint32_t) GPS_USART_PCLK_FREQ());
    // delay_ms(1000);

    dma_rx_stop();
    __HAL_UART_DISABLE(&gps_usart);

#ifndef RS41_RSM4x4
    gps_usart.Instance->BRR = UART_BRR_SAMPLING16(GPS_USART_PCLK_FREQ(), baud_rate);
#else // Calculated differently for L4
    gps_usart.Instance->BRR = (uint32_t) (GPS_USART_PCLK_FREQ() / baud_rate);
#endif // RSM4x4

    __HAL_UART_ENABLE(&gps_usart);
    dma_rx_restart();
}

void usart_gps_uninit()
{
    dma_rx_stop();
    __HAL_UART_DISABLE(&gps_usart);
    HAL_UART_DeInit(&gps_usart);
    HAL_DMA_DeInit(&hdma_usart_rx);
    GPS_USART_CLK_DISABLE();

#ifdef RS41  // both models of RS41 have a GPS reset_n pin
    // Disable GPS
    HAL_GPIO_WritePin(BANK_USART_PWR, GPS_RESET_N, GPIO_PIN_RESET);
#endif //RS41 power pin
}

void usart_gps_enable(bool enabled)
{
    dma_drain_enabled = enabled;
    if (enabled && hdma_usart_rx.Instance != NULL) {
        /* After a long TX, the circular DMA buffer may have wrapped multiple
         * times. Skip stale data by syncing rd_pos to the current write
         * position. The parser was already reset by radio.c before disabling. */
        dma_rd_pos = GPS_DMA_BUF_SIZE - __HAL_DMA_GET_COUNTER(&hdma_usart_rx);
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

/* Send a UART break frame: continuous low on TX for 10-11 bit times.
 * Used as a strong wake signal for the GPS in backup mode. */
void usart_gps_send_break(void)
{
    /* Drain any in-flight byte first so the break frame isn't truncated. */
    while ((__HAL_UART_GET_FLAG(&gps_usart, USART_FLAG_TC)) == RESET) {}
#ifdef RS41_RSM4x4
    /* L4: write SBKRQ in RQR. Self-clearing once the break has been sent. */
    gps_usart.Instance->RQR = USART_RQR_SBKRQ;
#else
    /* F1: set SBK in CR1. Hardware clears it once the break is sent. */
    gps_usart.Instance->CR1 |= USART_CR1_SBK;
    while (gps_usart.Instance->CR1 & USART_CR1_SBK) {}
#endif
}
