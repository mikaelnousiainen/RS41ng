#include <stdint.h>

#include <stm32f1xx_hal.h>

#include "system.h"
#include "timers.h"
#include "i2c.h"
#include "delay.h"
#include "log.h"
#include "gpio.h"
#include "millis.h"

#define BUTTON_PRESS_LONG_COUNT SYSTEM_SCHEDULER_TIMER_TICKS_PER_SECOND

#define ADC1_DR_Address ((uint32_t) 0x4001244C)

__IO uint32_t dma_buffer_adc[2];

volatile uint32_t button_pressed = 0;

void (*system_handle_timer_tick)() = NULL;

static volatile uint32_t systick_counter = 0;

DMA_HandleTypeDef dma_channel1;
ADC_HandleTypeDef adc1;

static void nvic_init()
{
#ifdef  VECT_TAB_RAM
    SCB->VTOR = SRAM_BASE;
#else  // VECT_TAB_FLASH
    SCB->VTOR = FLASH_BASE;
#endif
}

// TODO: Find out how to configure watchdog!

static void rcc_init()
{
    // HAL_RCC_DeInit();

    RCC_OscInitTypeDef RCC_OscInitStruct;

#ifdef RS41
    // The RS41 hardware uses an external clock at 24 MHz

    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState = RCC_HSE_ON;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE; // Do not configure PLL now

    HAL_RCC_OscConfig(&RCC_OscInitStruct);

    // If HSE fails to start up, the application will have incorrect clock configuration.
    //while (!__HAL_RCC_GET_FLAG(RCC_FLAG_HSERDY));
    
#endif
#ifdef DFM17
    // The DFM17 hardware uses the internal clock
    RCC_OscInitStruct.HSICalibrationValue = 0x10U;

    // Set up a 24 MHz PLL for 24 MHz SYSCLK (8 MHz / 2) * 6 = 24 MHz
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
    RCC_OscInitStruct.HSIState = RCC_HSI_ON;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI_DIV2;
    RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL6;

    HAL_RCC_OscConfig(&RCC_OscInitStruct);

    // while (!__HAL_RCC_GET_FLAG(RCC_FLAG_PLLRDY));
#endif

    //SystemInit();

    // FLASH_PrefetchBufferCmd(FLASH_PrefetchBuffer_Enable);
    // FLASH_SetLatency(FLASH_Latency_0);

    // TODO: Check what the delay timer TIM3 settings really should be and WTF the clock tick really is!?!?!?

    RCC_ClkInitTypeDef RCC_ClkInitStruct;

    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
#ifdef RS41
    // Use the 24 MHz external clock as SYSCLK
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSE;
    HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0);
    // while (__HAL_RCC_GET_SYSCLK_SOURCE() != 0x04);
#endif
#ifdef DFM17
    // Use the 24 MHz PLL as SYSCLK
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0);
    // while (__HAL_RCC_GET_SYSCLK_SOURCE() != 0x08);
#endif

  RCC_PeriphCLKInitTypeDef PeriphClkInit;
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_ADC;
  PeriphClkInit.AdcClockSelection = RCC_ADCPCLK2_DIV2;
  HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit);
}

static void gpio_init()
{
    __HAL_RCC_AFIO_CLK_ENABLE();
    __HAL_RCC_AFIO_FORCE_RESET();
    __HAL_RCC_AFIO_RELEASE_RESET();

    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();

    GPIO_InitTypeDef gpio_init;

    // Shutdown request
    gpio_init.Pin = PIN_SHUTDOWN;
    gpio_init.Mode = GPIO_MODE_OUTPUT_PP;
    gpio_init.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(I2C_GPIO, &gpio_init);
#ifdef DFM17
    HAL_GPIO_WritePin(BANK_SHUTDOWN, PIN_SHUTDOWN, GPIO_PIN_SET);		// Pull high to keep BMS from removing battery power after startup
#endif

    // Battery voltage (analog)
    gpio_init.Pin = PIN_VOLTAGE;
    gpio_init.Mode = GPIO_MODE_ANALOG;
    gpio_init.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(BANK_VOLTAGE, &gpio_init);

    // Button state (analog)
    gpio_init.Pin = PIN_BUTTON;
    gpio_init.Mode = GPIO_MODE_ANALOG;
    gpio_init.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(BANK_BUTTON, &gpio_init);

    // Green LED
    gpio_init.Pin = PIN_GREEN_LED;
    gpio_init.Mode = GPIO_MODE_OUTPUT_PP;
    gpio_init.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(BANK_GREEN_LED, &gpio_init);

    // Red LED
    gpio_init.Pin = PIN_RED_LED;
    gpio_init.Mode = GPIO_MODE_OUTPUT_PP;
    gpio_init.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(BANK_RED_LED, &gpio_init);

#ifdef DFM17
    // Yellow LED (only in DFM-17)
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
    int rc;

    __HAL_RCC_ADC1_CLK_ENABLE();
    __HAL_RCC_DMA1_CLK_ENABLE();

    // HAL_DMA_DeInit(&dma_channel1);

    dma_channel1.Instance = DMA1_Channel1;
    dma_channel1.Init.Direction = DMA_PERIPH_TO_MEMORY;
    dma_channel1.Init.PeriphInc  = DMA_PINC_DISABLE;
    dma_channel1.Init.MemInc  = DMA_MINC_ENABLE;
    dma_channel1.Init.PeriphDataAlignment  = DMA_PDATAALIGN_HALFWORD;
    dma_channel1.Init.MemDataAlignment  = DMA_MDATAALIGN_HALFWORD;
    dma_channel1.Init.Mode = DMA_CIRCULAR;			// Normal or Circular??
    dma_channel1.Init.Priority = DMA_PRIORITY_LOW;

// #ifdef RS41
//     dma_channel1.Init.BufferSize = 2;
// #endif
// #ifdef DFM17
//     dma_init.DMA_BufferSize = 1;
// #endif

    if (HAL_DMA_Init(&dma_channel1) != HAL_OK) {
      log_info("HAL_DMA_Init fail\n");
       while (1);
    } else {
      log_info("HAL_DMA_Init successful\n");
    }

//    HAL_NVIC_SetPriority(DMA1_Channel1_IRQn, 0, 0);
//    HAL_NVIC_EnableIRQ(DMA1_Channel1_IRQn);

    adc1.Instance = ADC1;
    adc1.Init.ScanConvMode = ENABLE;
    adc1.Init.ContinuousConvMode = ENABLE;
    adc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
    adc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
#ifdef RS41
    adc1.Init.NbrOfConversion = 2;
#endif
#ifdef DFM17
    adc1.Init.NbrOfConversion = 1;
#endif


    if ((rc = HAL_ADC_Init(&adc1)) != HAL_OK) {
      //log_info("HAL_ADC_Init fail:  RC: %d, State: %d, ErrorCode: %d\n", rc, (int) adc1.State, (int) adc1.ErrorCode);
      while (1);
    } else {
      //log_info("HAL_ADC_Init successful\n");
    }

    ADC_ChannelConfTypeDef adc_channel;
    adc_channel.Channel = CHANNEL_VOLTAGE;
    adc_channel.Rank = ADC_REGULAR_RANK_1;
    adc_channel.SamplingTime = ADC_SAMPLETIME_28CYCLES_5;
    if (HAL_ADC_ConfigChannel(&adc1, &adc_channel) != HAL_OK) {
//      log_info("HAL_ADC_ConfigChannel for VOLTAGE fail\n");
      while (1);
    } else {
//      log_info("HAL_ADC_ConfigChannel for VOLTAGE successful\n");
    }

#ifdef RS41
    adc_channel.Channel = CHANNEL_BUTTON;
    adc_channel.Rank = ADC_REGULAR_RANK_2;
    adc_channel.SamplingTime = ADC_SAMPLETIME_28CYCLES_5;

    if (HAL_ADC_ConfigChannel(&adc1, &adc_channel) != HAL_OK) {
//      log_info("HAL_ADC_ConfigChannel for BUTTON fail\n");
      while (1);
    } else {
//      log_info("HAL_ADC_ConfigChannel for BUTTON successful\n");
    }
#endif

#ifdef DFM17
// Not using ADC for button on DFM17
#endif

    // HAL_ADCEx_Calibration_Start(&adc1);

    __HAL_LINKDMA(&adc1,DMA_Handle,dma_channel1);

#ifdef DFM17
    if (HAL_ADC_Start_DMA(&adc1, dma_buffer_adc, 1) != HAL_OK) {
      //log_info("HAL_Start_DMA fail\n");
      while (1);
    } else {
      //log_info("HAL_Start_DMA successful\n");
    }
#endif
#ifdef RS41
    if (HAL_ADC_Start_DMA(&adc1, dma_buffer_adc, 2) != HAL_OK) {
      //log_info("HAL_Start_DMA fail\n");
      while (1);
    } else {
      //log_info("HAL_Start_DMA successful\n");
    }
#endif
}

uint16_t system_get_battery_voltage_millivolts()
{
#ifdef RS41
    uint16_t temp = dma_buffer_adc[0];
    uint16_t return_val = (uint16_t) (((float) temp) * 10.0f * 650.0f / 4096.0f);
    log_info("Raw battery voltage is: %d\n", temp);
    log_info("Computed battery voltage is: %d\n", return_val);
    //return (uint16_t) (((float) dma_buffer_adc[0]) * 10.0f * 600.0f / 4096.0f);
    return return_val;
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
    HAL_GPIO_WritePin(BANK_SHUTDOWN, PIN_SHUTDOWN, GPIO_PIN_SET);
#endif
#ifdef DFM17
    HAL_GPIO_WritePin(BANK_SHUTDOWN, PIN_SHUTDOWN, GPIO_PIN_RESET);
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

    // HAL_TIM_Base_DeInit(&htim4);

    __TIM4_CLK_ENABLE();

    // The data timer assumes a 24 MHz clock source
    htim4.Instance = TIM4;
    htim4.Init.Prescaler = 24 - 1; // tick every 1/1000000 s
    htim4.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim4.Init.Period = 100 - 1; // update every 1/10000 s
    htim4.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim4.Init.RepetitionCounter = 0;
    htim4.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;

    if (HAL_TIM_Base_Init(&htim4) != HAL_OK) {
      log_info("HAL Base Init Error\n");
      while (1) ;
    } else {
      //log_info("HAL Base Init Success\n");
    }

     __HAL_TIM_CLEAR_IT(&htim4, TIM_IT_UPDATE);
     __HAL_TIM_ENABLE_IT(&htim4, TIM_IT_UPDATE);

    HAL_TIM_RegisterCallback(&htim4, HAL_TIM_PERIOD_ELAPSED_CB_ID, User_TIM4_IRQHandler);

    if (HAL_TIM_Base_Start_IT(&htim4) != HAL_OK) {
      log_info("HAL Base Init IT Error\n");
      while (1) ;
    } else {
      //log_info("HAL Base Init IT Success\n");
    }
    HAL_NVIC_SetPriority(TIM4_IRQn, 3, 3);
    HAL_NVIC_EnableIRQ(TIM4_IRQn);
}

void system_disable_tick()
{
    HAL_TIM_Base_Stop_IT(&htim4);
    HAL_NVIC_DisableIRQ(TIM4_IRQn);
    __HAL_TIM_CLEAR_IT(&htim4, TIM_IT_UPDATE);
    __HAL_TIM_DISABLE_IT(&htim4, TIM_IT_UPDATE);
}

void system_enable_tick()
{
    __HAL_TIM_CLEAR_IT(&htim4, TIM_IT_UPDATE);
    __HAL_TIM_ENABLE_IT(&htim4, TIM_IT_UPDATE);
    HAL_TIM_Base_Start_IT(&htim4);
    HAL_NVIC_EnableIRQ(TIM4_IRQn);
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
    HAL_GPIO_WritePin(BANK_GREEN_LED, PIN_RED_LED, enabled ? GPIO_PIN_RESET : GPIO_PIN_SET);
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

    log_info("NVIC Init\n");
    //nvic_init();

    log_info("GPIO Init\n");
    gpio_init();

#ifdef DFM17
    // The millis timer is used for clock calibration on DFM-17 only
    millis_timer_init();
#endif
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

void SysTick_Handler(void)
{
    HAL_IncTick();
}

// Provide our own stub that just calls the standard HAL IRQ Handler.  It will call our PeriodElapsedCallback routine  - User_TIM4_IRQHandler
extern void TIM4_IRQHandler()
{
    HAL_TIM_IRQHandler(&htim4);
}

void User_TIM4_IRQHandler(TIM_HandleTypeDef *htim)
{
        system_handle_timer_tick();
#if ALLOW_POWER_OFF
        system_handle_button();
#endif
}

void DMA1_Channel1_IRQHandler(void)
{
    HAL_DMA_IRQHandler(&dma_channel1);
}

