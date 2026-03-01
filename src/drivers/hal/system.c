#include <stdint.h>
#include <string.h>
#include "config.h"

#ifdef RS41_RSM4x4
#include <stm32l4xx_hal.h>
#else
#include <stm32f1xx_hal.h>
#endif

#include "system.h"
#include "timers.h"
#include "i2c.h"
#include "delay.h"
#include "log.h"
#include "gpio.h"

#define BUTTON_PRESS_LONG_COUNT SYSTEM_SCHEDULER_TIMER_TICKS_PER_SECOND

#define ADC1_DR_Address ((uint32_t) 0x4001244C)

__IO uint16_t dma_buffer_adc[2];

volatile uint32_t button_pressed = 0;

void (*system_handle_timer_tick)() = NULL;

static volatile uint32_t systick_counter = 0;

DMA_HandleTypeDef hdma_adc1;
ADC_HandleTypeDef hadc1;

// Removed with LL conversion.  This is not defined in any of our compiles, so this code would never trigger anyway.
//static void nvic_init()
//{
//#ifdef  VECT_TAB_RAM
    //SCB->VTOR = SRAM_BASE;
//#else  // VECT_TAB_FLASH
    //SCB->VTOR = FLASH_BASE;
//#endif
//}

// TODO: Find out how to configure watchdog!

static void rcc_init()
{
    __HAL_RCC_PWR_CLK_ENABLE();

    RCC_OscInitTypeDef RCC_OscInitStruct = {0};

#ifdef RS41
#ifdef RS41_RSM4x4
    //log_info("PWREx\n");
    if (HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1) != HAL_OK) {
        while(1);
    }

    while (__HAL_PWR_GET_FLAG(PWR_FLAG_VOSF))
    {
        __NOP();
    }
#endif //RS41_RSM4x4

    // The RS41 hardware uses an external clock at 24 MHz
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState = RCC_HSE_ON;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE; // Do not configure PLL now

    //log_info("OscC\n");
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
        while(1);
    }

#endif
#ifdef DFM17
    // HSI (8 MHz) / 2 = 4 MHz into PLL, × 6 = 24 MHz SYSCLK.
    // No external clock required at boot.
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
    RCC_OscInitStruct.HSIState = RCC_HSI_ON;
    RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI_DIV2;
    RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL6;

    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
        while(1);
    }
#endif

    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
#ifdef RS41
    // Use the 24 MHz external clock as SYSCLK
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSE;
#ifdef RS41_RSM4x4
   log_info("RCC_Clock\n");
    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK) {
      while(1);
    }
#else // RS41
    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK) {
      while(1);
    }
#endif //RS41_RSM4x4
#endif // RS41
#ifdef DFM17
    // Use HSI PLL (24 MHz) as SYSCLK; FLASH_LATENCY_0 valid for ≤24 MHz on STM32F1
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK) {
        while(1);
    }
#endif //DFM17

  RCC_PeriphCLKInitTypeDef PeriphClkInit;
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_ADC;
#ifndef RS41_RSM4x4
  // STM32L412xx does not have this AdcClockSelecdtion option.  Just define it for the non-L4 processors.
  PeriphClkInit.AdcClockSelection = RCC_ADCPCLK2_DIV2;
#endif //RS41_RSM4x4
  HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit);

#ifndef RS41_RSM4x4
    __HAL_RCC_AFIO_CLK_ENABLE();
    __HAL_RCC_AFIO_FORCE_RESET();
    __HAL_RCC_AFIO_RELEASE_RESET();
    __HAL_RCC_PWR_CLK_ENABLE();
#else // RS41_RSM4x4
    __HAL_RCC_SYSCFG_CLK_ENABLE();
    __HAL_RCC_PWR_CLK_ENABLE();
#endif
}

static void gpio_init()
{
    //log_info("Starting GPIO clocks\n");
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE();

    GPIO_InitTypeDef gpio_init = {0};

    // Shutdown request
    //log_info("shutdown pin\n");
    gpio_init.Pin = PIN_SHUTDOWN;
    gpio_init.Mode = GPIO_MODE_OUTPUT_PP;
    gpio_init.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(BANK_SHUTDOWN, &gpio_init);
#ifdef DFM17
    HAL_GPIO_WritePin(BANK_SHUTDOWN, PIN_SHUTDOWN, GPIO_PIN_SET);		// Pull high to keep BMS from removing battery power after startup
#endif

    // Battery voltage (analog)
    //log_info("battery pin\n");
    gpio_init.Pin = PIN_VOLTAGE;
    gpio_init.Mode = GPIO_MODE_ANALOG;
    gpio_init.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(BANK_VOLTAGE, &gpio_init);

    // Button state (analog)
    //log_info("button pin\n");
    gpio_init.Pin = PIN_BUTTON;
    gpio_init.Mode = GPIO_MODE_ANALOG;
    gpio_init.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(BANK_BUTTON, &gpio_init);

    // Green LED
    //log_info("green pin\n");
    gpio_init.Pin = PIN_GREEN_LED;
    gpio_init.Mode = GPIO_MODE_OUTPUT_PP;
    gpio_init.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(BANK_GREEN_LED, &gpio_init);

    // Red LED
    //log_info("red pin\n");
    gpio_init.Pin = PIN_RED_LED;
    gpio_init.Mode = GPIO_MODE_OUTPUT_PP;
    gpio_init.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(BANK_RED_LED, &gpio_init);

#ifdef DFM17
    // Yellow LED (only in DFM-17)
    //log_info("yellow pin\n");
    gpio_init.Pin = PIN_YELLOW_LED;
    gpio_init.Mode = GPIO_MODE_OUTPUT_PP;
    gpio_init.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(BANK_YELLOW_LED, &gpio_init);
#endif
}

/**
 * Configure continuous DMA transfer for ADC from pins PA5 and PA6
 * to have battery voltage and button state available at all times.
 */
static void dma_adc_init()
{
#ifdef RS41_RSM4x4    // L4 processor
    __HAL_RCC_ADC_CLK_ENABLE();
#else	// F1 processor
    __HAL_RCC_ADC1_CLK_ENABLE();
#endif // Clock specific enable

    __HAL_RCC_DMA1_CLK_ENABLE();

    // HAL_DMA_DeInit(&hdma_adc1);

    hdma_adc1.Instance = DMA1_Channel1;
#ifdef RS41_RSM4x4
    hdma_adc1.Init.Request = DMA_REQUEST_0;  // L4 requires DMA request source
#endif
    hdma_adc1.Init.Direction = DMA_PERIPH_TO_MEMORY;
    hdma_adc1.Init.PeriphInc  = DMA_PINC_DISABLE;
    hdma_adc1.Init.MemInc  = DMA_MINC_ENABLE;
    hdma_adc1.Init.PeriphDataAlignment  = DMA_PDATAALIGN_HALFWORD;
    hdma_adc1.Init.MemDataAlignment  = DMA_MDATAALIGN_HALFWORD;
    hdma_adc1.Init.Mode = DMA_CIRCULAR;			// Normal or Circular??
    hdma_adc1.Init.Priority = DMA_PRIORITY_LOW;

// #ifdef RS41
//     hdma_adc1.Init.BufferSize = 2;
// #endif
// #ifdef DFM17
//     dma_init.DMA_BufferSize = 1;
// #endif

    hang_if_bad("HAL_DMA_Init",
                HAL_DMA_Init(&hdma_adc1)
               );


    hadc1.Instance = ADC1;
#ifdef RS41_RSM4x4
    hadc1.Init.ScanConvMode = ADC_SCAN_ENABLE;
#else
    hadc1.Init.ScanConvMode = ENABLE;
#endif
    hadc1.Init.ContinuousConvMode = ENABLE;
    hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
    hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
#ifdef RS41
    hadc1.Init.NbrOfConversion = 2;
#endif
#ifdef RS41_RSM4x4
    hadc1.Init.ClockPrescaler        = ADC_CLOCK_SYNC_PCLK_DIV4;   // 24MHz/4 = 6MHz ADC clock
    hadc1.Init.Resolution            = ADC_RESOLUTION_12B;
    hadc1.Init.EOCSelection          = ADC_EOC_SINGLE_CONV;
    hadc1.Init.LowPowerAutoWait      = DISABLE;
    hadc1.Init.DMAContinuousRequests = ENABLE;
    hadc1.Init.Overrun               = ADC_OVR_DATA_OVERWRITTEN;
    hadc1.Init.OversamplingMode      = DISABLE;
#endif
#ifdef DFM17
    hadc1.Init.NbrOfConversion = 1;
#endif

    hang_if_bad("HAL_ADC_Init",
                HAL_ADC_Init(&hadc1)
               );


    ADC_ChannelConfTypeDef adc_channel = {0};   // zero the whole struct first
    adc_channel.Channel = CHANNEL_VOLTAGE;
    adc_channel.Rank = ADC_REGULAR_RANK_1;
#ifndef RS41_RSM4x4
    adc_channel.SamplingTime = ADC_SAMPLETIME_28CYCLES_5;
#else
    adc_channel.SamplingTime = ADC_SAMPLETIME_24CYCLES_5;
    adc_channel.SingleDiff   = ADC_SINGLE_ENDED;   // L4 required
    adc_channel.OffsetNumber = ADC_OFFSET_NONE;    // L4 required — THIS is what crashes
    adc_channel.Offset       = 0;                  // L4 required
#endif

    hang_if_bad("HAL_ADC_ConfigChannel - 1",
                HAL_ADC_ConfigChannel(&hadc1, &adc_channel)
               );

#ifdef RS41
    memset(&adc_channel, 0, sizeof(adc_channel));
    adc_channel.Channel = CHANNEL_BUTTON;
    adc_channel.Rank = ADC_REGULAR_RANK_2;
    #ifndef RS41_RSM4x4
        adc_channel.SamplingTime = ADC_SAMPLETIME_28CYCLES_5;
    #else
        adc_channel.SamplingTime = ADC_SAMPLETIME_24CYCLES_5;
        adc_channel.SingleDiff   = ADC_SINGLE_ENDED;   // L4 required
        adc_channel.OffsetNumber = ADC_OFFSET_NONE;    // L4 required — THIS is what crashes
        adc_channel.Offset       = 0;                  // L4 required
    #endif

    hang_if_bad("HAL_ADC_ConfigChannel - 2",
                HAL_ADC_ConfigChannel(&hadc1, &adc_channel)
               );
#endif

#ifdef DFM17
// Not using ADC for button on DFM17
#endif

#ifdef RS41_RSM4x4
    HAL_ADCEx_Calibration_Start(&hadc1, ADC_SINGLE_ENDED);
#endif

    __HAL_LINKDMA(&hadc1,DMA_Handle,hdma_adc1);

#ifdef DFM17
    hang_if_bad("HAL_ADC_Start_DMA",
                HAL_ADC_Start_DMA(&hadc1, (uint32_t *) dma_buffer_adc, 1) 
               );
#endif
#ifdef RS41
    hang_if_bad("HAL_ADC_Start_DMA",
                HAL_ADC_Start_DMA(&hadc1, (uint32_t *) dma_buffer_adc, 2) 
               );
#endif
    //HAL_NVIC_SetPriority(ADC1_IRQn, 1, 0);
    //HAL_NVIC_EnableIRQ(ADC1_IRQn);
    //HAL_NVIC_SetPriority(DMA1_Channel1_IRQn, 0, 0);
    //HAL_NVIC_EnableIRQ(DMA1_Channel1_IRQn);

}

uint16_t system_get_battery_voltage_millivolts()
{
#ifdef RS41
    // After some experimentation, 6250 seems closest - within a few hundredts of a volt
    // Not sure why this isn't 3300 microvolts * 2 (voltage divider) = 6600
    uint16_t voltage = (uint16_t)((uint32_t)dma_buffer_adc[0] * 6250UL / 4096UL);
#else  //DFM17
    // Changed 600 to 690 to try and get it more accurate to actual voltage
    // 690 yields fairly linear, accurate results between 5v6 and 6v6
    uint16_t voltage = (uint16_t)((uint32_t)dma_buffer_adc[0] * 6900UL / 4096UL);
#endif 
    if(voltage > 16383) {
        return 16383;
    } else {
        return voltage;
    }
}

uint16_t system_get_button_adc_value()
{
#ifdef RS41
    return (uint16_t) dma_buffer_adc[1];
#endif
#ifdef DFM17
    // Fake being an ADC.  Take the binary value and if non-zero, make it trigger button-down
    return ( ((int) HAL_GPIO_ReadPin(BANK_BUTTON, PIN_BUTTON)) * 2100);
#endif
}

void system_shutdown()
{
#ifdef RS41
    HAL_GPIO_WritePin(BANK_SHUTDOWN, PIN_SHUTDOWN, GPIO_PIN_SET);
#endif
#ifdef DFM17
    HAL_GPIO_WritePin(BANK_SHUTDOWN, PIN_SHUTDOWN, GPIO_PIN_RESET);
#endif
}

void system_handle_button()
{

// #TODO fix this on the RS41 RSM_4x4
#ifndef RS41_RSM4x4
    static uint16_t button_pressed_threshold = 2000;
#else
    static uint16_t button_pressed_threshold = 1500;
#endif
    static bool shutdown = false;

    // RS41 (RSM412) & DFM17
    // ~1650-1850 - button up
    // ~2000-2200 - button down

    // RS41 RSM4x4
    // ~1300 - button up
    // ~1565 - button down

    uint16_t current_value = system_get_button_adc_value();

    if (current_value > button_pressed_threshold) {
        button_pressed++;
        if (button_pressed >= BUTTON_PRESS_LONG_COUNT) {
            shutdown = true;
            system_shutdown();
        }
    } else {
        if (!shutdown) {
           button_pressed = 0;		// Reset if we are not actually shutting down (accidental short press)
        }
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

    // HAL_TIM_Base_DeInit(&htim6);

#ifdef RS41_RSM4x4
    __HAL_RCC_TIM6_CLK_ENABLE();  // L4 style
#else
    __TIM6_CLK_ENABLE();           // F1 style  
#endif

    // The data timer assumes a 24 MHz clock source
    htim6.Instance = TIM6;
    htim6.Init.Prescaler = 24 - 1; // tick every 1/1000000 s
    htim6.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim6.Init.Period = 100 - 1; // update every 1/10000 s
    htim6.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim6.Init.RepetitionCounter = 0;
    htim6.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;

    hang_if_bad("HAL_TIM_Base_Init",
                HAL_TIM_Base_Init(&htim6)
               );

     __HAL_TIM_CLEAR_IT(&htim6, TIM_IT_UPDATE);
     __HAL_TIM_ENABLE_IT(&htim6, TIM_IT_UPDATE);

    HAL_TIM_RegisterCallback(&htim6, HAL_TIM_PERIOD_ELAPSED_CB_ID, User_TIM6_IRQHandler);

    hang_if_bad("HAL_TIM_Base_Start_IT",
                HAL_TIM_Base_Start_IT(&htim6)
               );
    HAL_NVIC_SetPriority(TIM6_IRQn, 3, 3);
    HAL_NVIC_EnableIRQ(TIM6_IRQn);
}

void system_disable_tick()
{
    HAL_TIM_Base_Stop_IT(&htim6);
    HAL_NVIC_DisableIRQ(TIM6_IRQn);
    __HAL_TIM_CLEAR_IT(&htim6, TIM_IT_UPDATE);
    __HAL_TIM_DISABLE_IT(&htim6, TIM_IT_UPDATE);
}

void system_enable_tick()
{
    __HAL_TIM_CLEAR_IT(&htim6, TIM_IT_UPDATE);
    __HAL_TIM_ENABLE_IT(&htim6, TIM_IT_UPDATE);
    HAL_TIM_Base_Start_IT(&htim6);
    HAL_NVIC_EnableIRQ(TIM6_IRQn);
}

void system_set_green_led(bool enabled)
{
#ifdef RS41
    HAL_GPIO_WritePin(BANK_GREEN_LED, PIN_GREEN_LED, enabled ? GPIO_PIN_RESET : GPIO_PIN_SET);
#endif
#ifdef DFM17
    HAL_GPIO_WritePin(BANK_GREEN_LED, PIN_GREEN_LED, enabled ? GPIO_PIN_SET : GPIO_PIN_RESET);
#endif
}

void system_set_red_led(bool enabled)
{
#ifdef RS41
    HAL_GPIO_WritePin(BANK_RED_LED, PIN_RED_LED, enabled ? GPIO_PIN_RESET : GPIO_PIN_SET);
#endif
#ifdef DFM17
    HAL_GPIO_WritePin(BANK_GREEN_LED, PIN_RED_LED, enabled ? GPIO_PIN_SET : GPIO_PIN_RESET);
#endif
}

void system_set_yellow_led(bool enabled)
{
#ifdef DFM17
    // Only DFM-17 has a yellow LED
    HAL_GPIO_WritePin(BANK_GREEN_LED, PIN_YELLOW_LED, enabled ? GPIO_PIN_SET : GPIO_PIN_RESET);

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
    HAL_Init();

    log_info("RCC Init\n");
    rcc_init();

    log_info("GPIO Init\n");
    gpio_init();

    log_info("Systick init\n");
    system_scheduler_timer_init();

    log_info("DMA Init\n");
    dma_adc_init();

    log_info("Delay Init\n");
    delay_init();

    log_info("HCLK: %ld\n", HAL_RCC_GetHCLKFreq());
    log_info("SYSCLK: %ld\n", HAL_RCC_GetSysClockFreq());
    log_info("SystemCoreClock: %ld\n", SystemCoreClock);

    log_info("Configuring SysTick\n");
    HAL_SYSTICK_Config(SystemCoreClock / 1000U);
    HAL_NVIC_SetPriority(SysTick_IRQn, 0, 0U);
}

#ifdef DFM17
void system_switch_to_hse_bypass()
{
    // Si4063 TCXO divided clock (12.8 MHz) is now active on GPIO2/PD0 (OSC_IN).
    // Use PLL to clean up the signal and produce exactly 24 MHz SYSCLK:
    //   12.8 MHz HSE bypass → PREDIV1 ÷8 → 1.6 MHz → PLL ×15 → 24 MHz
    delay_ms(100);

    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState = RCC_HSE_BYPASS;
    RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV8;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL15;

    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
        // Blink red LED to indicate failure
        for (int i = 0; i < 10; i++) {
            HAL_GPIO_TogglePin(BANK_RED_LED, PIN_RED_LED);
            for (volatile int j = 0; j < 200000; j++);
        }
        return;
    }

    // Switch SYSCLK from HSI to PLL (24 MHz, same as original DFM17 config)
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_SYSCLK;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;

    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK) {
        for (int i = 0; i < 20; i++) {
            HAL_GPIO_TogglePin(BANK_RED_LED, PIN_RED_LED);
            for (volatile int j = 0; j < 100000; j++);
        }
        return;
    }

    // Reconfigure SysTick for 24 MHz
    HAL_SYSTICK_Config(SystemCoreClock / 1000U);

    // Yellow LED flash to confirm clock switch success
    // HAL_GPIO_WritePin(BANK_YELLOW_LED, PIN_YELLOW_LED, GPIO_PIN_SET);
    // delay_ms(200);
    // HAL_GPIO_WritePin(BANK_YELLOW_LED, PIN_YELLOW_LED, GPIO_PIN_RESET);
}
#endif

void SysTick_Handler(void)
{
    HAL_IncTick();
}

// Provide our own stub that just calls the standard HAL IRQ Handler.  It will call our PeriodElapsedCallback routine  - User_TIM6_IRQHandler
extern void TIM6_IRQHandler()
{
    HAL_TIM_IRQHandler(&htim6);
}

void User_TIM6_IRQHandler(TIM_HandleTypeDef *htim)
{
    if (system_handle_timer_tick != NULL) {
        system_handle_timer_tick();
    }
#if ALLOW_POWER_OFF
        system_handle_button();
#endif
}

void DMA1_Channel1_IRQHandler(void)
{
    HAL_DMA_IRQHandler(&hdma_adc1);
}

void ADC1_IRQHandler(void)
{
  HAL_ADC_IRQHandler(&hadc1);
}
