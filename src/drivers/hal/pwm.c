#include "config.h"

#ifdef RS41_RSM4x4
#include <stm32l4xx_hal.h>
#else
#include <stm32f1xx_hal.h>
#endif


#include "pwm.h"
#include "timers.h"
#include "log.h"

uint16_t pwm_timer_dma_buffer[PWM_TIMER_DMA_BUFFER_SIZE];

uint16_t (*pwm_handle_dma_transfer_half)(uint16_t buffer_size, uint16_t *buffer) = NULL;
uint16_t (*pwm_handle_dma_transfer_full)(uint16_t buffer_size, uint16_t *buffer) = NULL;

DMA_Channel_TypeDef *pwm_dma_channel = DMA1_Channel2;

void pwm_data_timer_init()
{
    // Timer frequency = TIM_CLK/(TIM_PSC+1)/(TIM_ARR + 1)
    // TIM_CLK =
    // TIM_PSC = Prescaler
    // TIM_ARR = Period

    __HAL_RCC_TIM2_CLK_ENABLE();

    htim2.Instance = TIM2;
    
    HAL_TIM_Base_DeInit(&htim2);

    htim2.Init.Prescaler = 2 - 1; // tick every 1/12000000 s
    htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim2.Init.Period = 10000 - 1; // update every 1/1200 s
    htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim2.Init.RepetitionCounter = 0;

    HAL_TIM_Base_Init(&htim2);

    // No interrupts necessary for data timer, as it is only used for triggering DMA transfers
    /*
    TIM_ClearITPendingBit(TIM2, TIM_IT_Update);
    TIM_ITConfig(TIM2, TIM_IT_Update, ENABLE);

    NVIC_InitTypeDef nvic_init;
    nvic_init.NVIC_IRQChannel = TIM2_IRQn;
    nvic_init.NVIC_IRQChannelPreemptionPriority = 2;
    nvic_init.NVIC_IRQChannelSubPriority = 2;
    nvic_init.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&nvic_init);
    */

    HAL_TIM_Base_Start(&htim2);
}

void pwm_data_timer_uninit()
{
    HAL_TIM_Base_Stop(&htim2);
}

void pwm_timer_init(uint32_t frequency_hz_100)
{
    TIM_MasterConfigTypeDef sMasterConfig = {0};
    htim15.Instance = TIM15;
#ifdef RS41
    TIM_BreakDeadTimeConfigTypeDef sBreakDeadTimeConfig = {0};
#endif

    __HAL_RCC_TIM15_CLK_ENABLE();

    HAL_TIM_Base_DeInit(&htim15);
// #ifdef RS41
//     GPIO_PinRemapConfig(GPIO_Remap_TIM15, DISABLE);
// #endif

    htim15.Init.Prescaler = 24 - 1; // tick every 1/1000000 s
    htim15.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim15.Init.Period = (uint16_t) (((100.0f * 1000000.0f) / (frequency_hz_100 * 2.0f))) - 1;
    htim15.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim15.Init.RepetitionCounter = 0;
    htim15.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;

    //HAL_TIM_Base_Init(&htim15);
    hang_if_bad("HAL_TIM_PWM_Init", (HAL_TIM_PWM_Init(&htim15)));

    sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
    sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
    hang_if_bad("HAL_TIMEx_MasterConfigSynchronization", 
                HAL_TIMEx_MasterConfigSynchronization(&htim15, &sMasterConfig)
               );

#ifdef RS41
    TIM_OC_InitTypeDef TIM15_OCInitStruct;

    TIM15_OCInitStruct.Pulse = 0; // Was: tim_init.TIM_Period / 2
    TIM15_OCInitStruct.OCMode = TIM_OCMODE_TOGGLE; // Was: TIM_OCMode_PWM1
    TIM15_OCInitStruct.OCPolarity = TIM_OCPOLARITY_HIGH;
    TIM15_OCInitStruct.OCFastMode = TIM_OCFAST_ENABLE;
    TIM15_OCInitStruct.OCIdleState = TIM_OCIDLESTATE_RESET;
    TIM15_OCInitStruct.OCNIdleState = TIM_OCNIDLESTATE_RESET;

    // TIM15 channel 2 can be used to drive pin PB15, which is connected to RS41 Si4032 SDI pin for direct modulation
    hang_if_bad("HAL_TIM_OC_ConfigChannel", 
                HAL_TIM_OC_ConfigChannel(&htim15, &TIM15_OCInitStruct, TIM_CHANNEL_2)
               );

    sBreakDeadTimeConfig.OffStateRunMode = TIM_OSSR_DISABLE;
    sBreakDeadTimeConfig.OffStateIDLEMode = TIM_OSSI_DISABLE;
    sBreakDeadTimeConfig.LockLevel = TIM_LOCKLEVEL_OFF;
    sBreakDeadTimeConfig.DeadTime = 0;
    sBreakDeadTimeConfig.BreakState = TIM_BREAK_DISABLE;
    sBreakDeadTimeConfig.BreakPolarity = TIM_BREAKPOLARITY_HIGH;
    sBreakDeadTimeConfig.AutomaticOutput = TIM_AUTOMATICOUTPUT_DISABLE;
    hang_if_bad("HAL_TIMEx_ConfigBreakDeadTim", 
                HAL_TIMEx_ConfigBreakDeadTime(&htim15, &sBreakDeadTimeConfig)
               );

    // These are not needed?
    // TIM_SelectOCxM(TIM15, TIM_Channel_2, TIM_OCMode_PWM1);
    // TIM_CCxCmd(TIM15, TIM_Channel_2, TIM_CCx_Enable);

    // The following settings make transitions between generated frequencies smooth
    //__HAL_TIM_ENABLE_OCxPRELOAD(&htim15, TIM_CHANNEL_2);


    // __HAL_TIM_MOE_DISABLE(&htim15);
#endif
#ifdef DFM17
    // For DFM17 we don't have a PWM pin in the right place, so we manually toggle the pin in the ISR
    __HAL_TIM_CLEAR_IT(&htim15, TIM_IT_UPDATE);
    __HAL_TIM_ENABLE_IT(&htim15, TIM_IT_UPDATE);

    HAL_NVIC_SetPriority(TIM1_BRK_TIM15_IRQn, 2, 1);
    HAL_NVIC_EnableIRQ(TIM1_BRK_TIM15_IRQn);
#endif

//    __HAL_TIM_ENABLE(&htim15);
    hang_if_bad("HAL_TIM_PWM_Start",
                HAL_TIM_PWM_Start(&htim15, TIM_CHANNEL_2)
               );
}

void pwm_timer_pwm_enable(bool enabled)
{
#ifdef RS41
    if(enabled) 
    {
        __HAL_TIM_MOE_ENABLE(&htim15);
    } else {
        __HAL_TIM_MOE_DISABLE(&htim15);
    }
#endif
}

void pwm_timer_use(bool use)
{
#if defined(RS41) && !defined(RS41_RSM4x4)			// rsm4x4 does this on the GPIO pin in the spi routine directly
    // Remapping the TIM15 outputs will allow TIM15 channel 2 can be used to drive pin PB15,
    // which is connected to RS41 Si4032 SDI pin for direct modulation
    if(use)
    {
        __HAL_AFIO_REMAP_TIM15_ENABLE();
    } else {
        __HAL_AFIO_REMAP_TIM15_DISABLE();
    }
#endif
}

void pwm_timer_uninit()
{
    __HAL_TIM_MOE_DISABLE(&htim15);
    __HAL_TIM_DISABLE(&htim15);
    hang_if_bad("HAL_TIM_PWM_Stop",
                HAL_TIM_PWM_Stop(&htim15, TIM_CHANNEL_2)
               );
    hang_if_bad("HAL_TIM_PWM_DeInit",
                HAL_TIM_PWM_DeInit(&htim15)
               );
    //HAL_TIM_Base_DeInit(&htim15);

#if defined(RS41) && !defined(RS41_RSM4x4)                      // rsm4x4 does this on the GPIO pin in the spi routine directly
    __HAL_AFIO_REMAP_TIM15_DISABLE();
#endif

    __HAL_RCC_TIM15_CLK_DISABLE();
}

inline uint16_t pwm_calculate_period(uint32_t frequency_hz_100)
{
    return (uint16_t) (((100.0f * 1000000.0f) / (frequency_hz_100 * 2.0f))) - 1;
}

inline void pwm_timer_set_frequency(uint32_t pwm_period)
{
    __HAL_TIM_SET_AUTORELOAD(&htim15, pwm_period);
}

/**
 * Below are experimental DMA routines for supplying PWM data for APRS modulation.
 * This does not work correctly, but is left for future reference.
 */

// static void pwm_dma_init_channel()
// {
//     DMA_InitTypeDef dma_init;
//     dma_init.DMA_BufferSize = PWM_TIMER_DMA_BUFFER_SIZE;
//     dma_init.DMA_DIR = DMA_DIR_PeripheralDST;
//     dma_init.DMA_M2M = DMA_M2M_Disable;
//     dma_init.DMA_MemoryBaseAddr = (uint32_t) pwm_timer_dma_buffer;
//     dma_init.DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord;
//     dma_init.DMA_MemoryInc = DMA_MemoryInc_Enable;
//     dma_init.DMA_Mode = DMA_Mode_Circular;
//     dma_init.DMA_PeripheralBaseAddr = (uint32_t) &TIM15->ARR;
//     dma_init.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord; // DMA_PeripheralDataSize_Word ?
//     dma_init.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
//     dma_init.DMA_Priority = DMA_Priority_VeryHigh;
//     DMA_Init(pwm_dma_channel, &dma_init);
// }

// inline void pwm_dma_interrupt_enable(bool enabled)
// {
//     DMA_ClearITPendingBit(DMA1_IT_HT2);
//     DMA_ClearITPendingBit(DMA1_IT_TC2);
//     DMA_ITConfig(pwm_dma_channel, DMA_IT_HT | DMA_IT_TC | DMA_IT_TE, enabled ? ENABLE : DISABLE);
// }

void pwm_dma_init()
{
//     DMA_DeInit(pwm_dma_channel);

//     RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);

//     pwm_dma_init_channel();

//     pwm_dma_interrupt_enable(true);

//     DMA_Cmd(pwm_dma_channel, ENABLE);

//     NVIC_InitTypeDef nvic_init;
//     nvic_init.NVIC_IRQChannel = DMA1_Channel2_IRQn;
//     nvic_init.NVIC_IRQChannelPreemptionPriority = 2;
//     nvic_init.NVIC_IRQChannelSubPriority = 0;
//     nvic_init.NVIC_IRQChannelCmd = ENABLE;
//     NVIC_Init(&nvic_init);
}

void pwm_dma_start()
{
//     //pwm_dma_init_channel();
//     //pwm_dma_interrupt_enable(true);
//     // TODO: Why doesn't timer DMA request restart without reinitializing the timer?
//     pwm_timer_init(100 * 100);
//     pwm_timer_pwm_enable(true);
//     pwm_timer_use(true);
//     DMA_SetCurrDataCounter(pwm_dma_channel, PWM_TIMER_DMA_BUFFER_SIZE);
//     DMA_Cmd(pwm_dma_channel, ENABLE);
//     pwm_data_timer_dma_request_enable(true);
}

void pwm_dma_stop()
{
//     pwm_data_timer_dma_request_enable(false);
//     DMA_Cmd(pwm_dma_channel, DISABLE);
//     //pwm_dma_interrupt_enable(false);
}

// void pwm_data_timer_dma_request_enable(bool enabled)
// {
//     // TIM2 Update DMA requests are routed to DMA1 Channel2
//     TIM_DMACmd(TIM2, TIM_DMA_Update, enabled ? ENABLE : DISABLE);
// }

// void DMA1_Channel2_IRQHandler(void)
// {
//     if (DMA_GetITStatus(DMA1_IT_TE2)) {
//         DMA_ClearITPendingBit(DMA1_IT_TE2);
//         log_info("DMA Transfer Error\n");
//     }
//     if (DMA_GetITStatus(DMA1_IT_HT2)) {
//         DMA_ClearITPendingBit(DMA1_IT_HT2);
//         pwm_handle_dma_transfer_half(PWM_TIMER_DMA_BUFFER_SIZE, pwm_timer_dma_buffer);
//     }
//     if (DMA_GetITStatus(DMA1_IT_TC2)) {
//         DMA_ClearITPendingBit(DMA1_IT_TC2);
//         pwm_handle_dma_transfer_full(PWM_TIMER_DMA_BUFFER_SIZE, pwm_timer_dma_buffer);
//     }
// }
