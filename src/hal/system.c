#include <stdint.h>
#include <system_stm32f10x.h>
#include <stm32f10x_rcc.h>
#include <stm32f10x_flash.h>
#include <stm32f10x_gpio.h>
#include <stm32f10x_dma.h>
#include <stm32f10x_adc.h>
#include <stm32f10x_tim.h>
#include <misc.h>

#include "system.h"
#include "delay.h"
#include "log.h"
#include "gpio.h"
#include "millis.h"

#define BUTTON_PRESS_LONG_COUNT SYSTEM_SCHEDULER_TIMER_TICKS_PER_SECOND

#define ADC1_DR_Address ((uint32_t) 0x4001244C)

__IO uint16_t dma_buffer_adc[2];

volatile uint32_t button_pressed = 0;

void (*system_handle_timer_tick)() = NULL;

static volatile uint32_t systick_counter = 0;

static void nvic_init()
{
#ifdef  VECT_TAB_RAM
    NVIC_SetVectorTable(NVIC_VectTab_RAM, 0x0);
#else  // VECT_TAB_FLASH
    NVIC_SetVectorTable(NVIC_VectTab_FLASH, 0x0);
#endif
}

// TODO: Find out how to configure watchdog!

static void rcc_init()
{
    RCC_DeInit();
#ifdef RS41
    // The RS41 hardware uses an external clock at 24 MHz
    RCC_HSEConfig(RCC_HSE_ON);

    ErrorStatus hse_status = RCC_WaitForHSEStartUp();
    if (hse_status != SUCCESS) {
        // If HSE fails to start up, the application will have incorrect clock configuration.
        while (true) {}
    }
#endif
#ifdef DFM17
    // The DFM17 hardware uses the internal clock
    RCC_AdjustHSICalibrationValue(0x10U);
    // Set up a 24 MHz PLL for 24 MHz SYSCLK (8 MHz / 2) * 6 = 24 MHz
    RCC_PLLConfig(RCC_PLLSource_HSI_Div2, RCC_PLLMul_6);
    RCC_HSICmd(ENABLE);
    RCC_PLLCmd(ENABLE);
    while (RCC_GetFlagStatus(RCC_FLAG_PLLRDY) == RESET);
#endif

    //SystemInit();

    FLASH_PrefetchBufferCmd(FLASH_PrefetchBuffer_Enable);
    FLASH_SetLatency(FLASH_Latency_0);

    // TODO: Check what the delay timer TIM3 settings really should be and WTF the clock tick really is!?!?!?

    RCC_HCLKConfig(RCC_SYSCLK_Div1);
    RCC_PCLK2Config(RCC_HCLK_Div1);
    RCC_PCLK1Config(RCC_HCLK_Div1);
#ifdef RS41
    // Use the 24 MHz external clock as SYSCLK
    RCC_SYSCLKConfig(RCC_SYSCLKSource_HSE);

    while (RCC_GetSYSCLKSource() != 0x04);
#endif
#ifdef DFM17
    // Use the 24 MHz PLL as SYSCLK
    RCC_SYSCLKConfig(RCC_SYSCLKSource_PLLCLK);

    while (RCC_GetSYSCLKSource() != 0x08);
#endif
}

static void gpio_init()
{
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);
    RCC_APB2PeriphResetCmd(RCC_APB2Periph_AFIO, DISABLE);

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);

    GPIO_InitTypeDef gpio_init;

    // Shutdown request
    gpio_init.GPIO_Pin = PIN_SHUTDOWN;
    gpio_init.GPIO_Mode = GPIO_Mode_Out_PP;
    gpio_init.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(BANK_SHUTDOWN, &gpio_init);
#ifdef DFM17
    GPIO_SetBits(BANK_SHUTDOWN, PIN_SHUTDOWN);		// Pull high to keep BMS from removing battery power after startup
#endif

    // Battery voltage (analog)
    gpio_init.GPIO_Pin = PIN_VOLTAGE;
    gpio_init.GPIO_Mode = GPIO_Mode_AIN;
    gpio_init.GPIO_Speed = GPIO_Speed_10MHz;
    GPIO_Init(BANK_VOLTAGE, &gpio_init);

    // Button state (analog)
    gpio_init.GPIO_Pin = PIN_BUTTON;
    gpio_init.GPIO_Mode = GPIO_Mode_AIN;
    gpio_init.GPIO_Speed = GPIO_Speed_10MHz;
    GPIO_Init(BANK_BUTTON, &gpio_init);

    // Green LED
    gpio_init.GPIO_Pin = PIN_GREEN_LED;
    gpio_init.GPIO_Mode = GPIO_Mode_Out_PP;
    gpio_init.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(BANK_GREEN_LED, &gpio_init);

    // Red LED
    gpio_init.GPIO_Pin = PIN_RED_LED;
    gpio_init.GPIO_Mode = GPIO_Mode_Out_PP;
    gpio_init.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(BANK_RED_LED, &gpio_init);

#ifdef DFM17
    // Yellow LED (only in DFM-17)
    gpio_init.GPIO_Pin = PIN_YELLOW_LED;
    gpio_init.GPIO_Mode = GPIO_Mode_Out_PP;
    gpio_init.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(BANK_YELLOW_LED, &gpio_init);
#endif
}

/**
 * Configure continuous DMA transfer for ADC from pins PA5 and PA6
 * to have battery voltage and button state available at all times.
 */
static void dma_adc_init()
{
    DMA_DeInit(DMA1_Channel1);

    DMA_InitTypeDef dma_init;

    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);

#ifdef RS41
    dma_init.DMA_BufferSize = 2;
#endif
#ifdef DFM17
    dma_init.DMA_BufferSize = 1;
#endif
    dma_init.DMA_DIR = DMA_DIR_PeripheralSRC;
    dma_init.DMA_M2M = DMA_M2M_Disable;
    dma_init.DMA_MemoryBaseAddr = (uint32_t) &dma_buffer_adc;
    dma_init.DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord;
    dma_init.DMA_MemoryInc = DMA_MemoryInc_Enable;
    dma_init.DMA_Mode = DMA_Mode_Circular;
    dma_init.DMA_PeripheralBaseAddr = ADC1_DR_Address;
    dma_init.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord;
    dma_init.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
    dma_init.DMA_Priority = DMA_Priority_High;
    DMA_Init(DMA1_Channel1, &dma_init);

    DMA_Cmd(DMA1_Channel1, ENABLE);

    RCC_ADCCLKConfig(RCC_PCLK2_Div2);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);

    ADC_InitTypeDef adc_init;
    adc_init.ADC_Mode = ADC_Mode_Independent;
    adc_init.ADC_ScanConvMode = ENABLE;
    adc_init.ADC_ContinuousConvMode = ENABLE;
    adc_init.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;
    adc_init.ADC_DataAlign = ADC_DataAlign_Right;
#ifdef RS41
    adc_init.ADC_NbrOfChannel = 2;
#endif
#ifdef DFM17
    adc_init.ADC_NbrOfChannel = 1;
#endif
    ADC_Init(ADC1, &adc_init);

    ADC_RegularChannelConfig(ADC1, CHANNEL_VOLTAGE, 1, ADC_SampleTime_28Cycles5);
#ifdef RS41
    ADC_RegularChannelConfig(ADC1, CHANNEL_BUTTON, 2, ADC_SampleTime_28Cycles5);
#endif
#ifdef DFM17
// Not using ADC for button on DFM17
#endif

    // ADC1 DMA requests are routed to DMA1 Channel1
    ADC_DMACmd(ADC1, ENABLE);
    ADC_Cmd(ADC1, ENABLE);

    ADC_ResetCalibration(ADC1);
    while (ADC_GetResetCalibrationStatus(ADC1));

    // Start new calibration (ADC must be off at that time)
    ADC_StartCalibration(ADC1);
    while (ADC_GetCalibrationStatus(ADC1));

    // Start conversion (will be endless as we are in continuous mode)
    ADC_SoftwareStartConvCmd(ADC1, ENABLE);
}

uint16_t system_get_battery_voltage_millivolts()
{
#ifdef RS41
    return (uint16_t) (((float) dma_buffer_adc[0]) * 10.0f * 600.0f / 4096.0f);
#else  //DFM17
    // Kludge:  DFM17 voltage is more than 5 volts, so cut the value in half.
    // Changed 600 to 690 to try and get it more accurate to actual voltage
    // 690 yields fairly linear, accurate results between 5v6 and 6v6
    return (uint16_t) ( ((float) dma_buffer_adc[0]) * 10.0f * 690.0f / 8192.0f );
#endif 
}

uint16_t system_get_button_adc_value()
{
#ifdef RS41
    return (uint16_t) dma_buffer_adc[1];
#endif
#ifdef DFM17
    // Fake being an ADC.  Take the binary value and if non-zero, make it trigger button-down
    return ( ((int) GPIO_ReadInputDataBit(BANK_BUTTON,PIN_BUTTON)) * 2100);
#endif
}

void system_shutdown()
{
#ifdef RS41
    GPIO_SetBits(BANK_SHUTDOWN, PIN_SHUTDOWN);
#endif
#ifdef DFM17
    GPIO_ResetBits(BANK_SHUTDOWN, PIN_SHUTDOWN);
#endif
}

void system_handle_button()
{
    static uint16_t button_pressed_threshold = 2000;
    static bool shutdown = false;

    // ~1650-1850 - button up
    // ~2000-2200 - button down

    uint16_t current_value = system_get_button_adc_value();

    if (current_value > button_pressed_threshold) {
        button_pressed++;
        if (button_pressed >= BUTTON_PRESS_LONG_COUNT) {
            shutdown = true;
        }
    } else {
        if (shutdown) {
            system_shutdown();
        }
        button_pressed = 0;
    }

    if (button_pressed == 0) {
        button_pressed_threshold = current_value * 1.1;
    }
}

bool system_is_button_pressed()
{
    return button_pressed > 0;
}

void system_scheduler_timer_init()
{
    // Timer frequency = TIM_CLK/(TIM_PSC+1)/(TIM_ARR + 1)
    // TIM_CLK =
    // TIM_PSC = Prescaler
    // TIM_ARR = Period
    TIM_DeInit(TIM4);

    TIM_TimeBaseInitTypeDef tim_init;
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM4, ENABLE);
    RCC_APB1PeriphResetCmd(RCC_APB1Periph_TIM4, DISABLE);

    tim_init.TIM_Prescaler = 24 - 1; // tick every 1/1000000 s
    tim_init.TIM_CounterMode = TIM_CounterMode_Up;
    tim_init.TIM_Period = 100 - 1; // update every 1/10000 s
    tim_init.TIM_ClockDivision = TIM_CKD_DIV1;
    tim_init.TIM_RepetitionCounter = 0;

    TIM_TimeBaseInit(TIM4, &tim_init);

    TIM_ClearITPendingBit(TIM4, TIM_IT_Update);
    TIM_ITConfig(TIM4, TIM_IT_Update, ENABLE);

    NVIC_InitTypeDef nvic_init;
    nvic_init.NVIC_IRQChannel = TIM4_IRQn;
    nvic_init.NVIC_IRQChannelPreemptionPriority = 3;
    nvic_init.NVIC_IRQChannelSubPriority = 1;
    nvic_init.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&nvic_init);

    TIM_Cmd(TIM4, ENABLE);
}

void system_disable_tick()
{
    TIM_Cmd(TIM4, DISABLE);
    NVIC_DisableIRQ(TIM4_IRQn);
    TIM_ITConfig(TIM4, TIM_IT_Update, DISABLE);
    TIM_ClearITPendingBit(TIM4, TIM_IT_Update);
}

void system_enable_tick()
{
    TIM_ClearITPendingBit(TIM4, TIM_IT_Update);
    TIM_ITConfig(TIM4, TIM_IT_Update, ENABLE);
    NVIC_EnableIRQ(TIM4_IRQn);
    TIM_Cmd(TIM4, ENABLE);
}

void system_set_green_led(bool enabled)
{
#ifdef RS41
    if (enabled) {
        GPIO_ResetBits(BANK_GREEN_LED, PIN_GREEN_LED);
    } else {
        GPIO_SetBits(BANK_GREEN_LED, PIN_GREEN_LED);
    }
#endif
#ifdef DFM17
    if (enabled) {
        GPIO_SetBits(BANK_GREEN_LED, PIN_GREEN_LED);
    } else {
        GPIO_ResetBits(BANK_GREEN_LED, PIN_GREEN_LED);
    }
#endif
}

void system_set_red_led(bool enabled)
{
#ifdef RS41
    if (enabled) {
        GPIO_ResetBits(BANK_RED_LED, PIN_RED_LED);
    } else {
        GPIO_SetBits(BANK_RED_LED, PIN_RED_LED);
    }
#endif
#ifdef DFM17
    if (enabled) {
        GPIO_SetBits(BANK_RED_LED, PIN_RED_LED);
    } else {
        GPIO_ResetBits(BANK_RED_LED, PIN_RED_LED);
    }
#endif
}

void system_set_yellow_led(bool enabled)
{
#ifdef DFM17
    // Only DFM-17 has a yellow LED
    if (enabled) {
        GPIO_SetBits(BANK_YELLOW_LED, PIN_YELLOW_LED);
    } else {
        GPIO_ResetBits(BANK_YELLOW_LED, PIN_YELLOW_LED);
    }
#endif
}

void system_disable_irq()
{
    __disable_irq();
}

void system_enable_irq()
{
    __enable_irq();
}

void system_init()
{
    rcc_init();
    nvic_init();
    gpio_init();
    dma_adc_init();
    delay_init();
#ifdef DFM17
    // The millis timer is used for clock calibration on DFM-17 only
    millis_timer_init();
#endif
    system_scheduler_timer_init();

    RCC_ClocksTypeDef RCC_Clocks;
    RCC_GetClocksFreq(&RCC_Clocks);

    log_info("HCLK: %ld\n", RCC_Clocks.HCLK_Frequency);
    log_info("SYSCLK: %ld\n", RCC_Clocks.SYSCLK_Frequency);
    log_info("SystemCoreClock: %ld\n", SystemCoreClock);

    delay_ms(100);

    SysTick_Config(SystemCoreClock / 10000);
}

uint32_t system_get_tick()
{
    return systick_counter;
}

void SysTick_Handler()
{
    systick_counter++;
}

void TIM4_IRQHandler(void)
{
    if (TIM_GetITStatus(TIM4, TIM_IT_Update) != RESET) {
        TIM_ClearITPendingBit(TIM4, TIM_IT_Update);

        system_handle_timer_tick();

#if ALLOW_POWER_OFF
        system_handle_button();
#endif
    }
}
