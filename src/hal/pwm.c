#include <stm32f10x_rcc.h>
#include <stm32f10x_gpio.h>
#include <stm32f10x_tim.h>
#include <stm32f10x_dma.h>
#include <misc.h>

#include "pwm.h"
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

    TIM_DeInit(TIM2);

    TIM_TimeBaseInitTypeDef tim_init;
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);
    RCC_APB2PeriphResetCmd(RCC_APB1Periph_TIM2, DISABLE);

    tim_init.TIM_Prescaler = 2 - 1; // tick every 1/12000000 s
    tim_init.TIM_CounterMode = TIM_CounterMode_Up;
    tim_init.TIM_Period = 10000 - 1; // update every 1/1200 s
    tim_init.TIM_ClockDivision = TIM_CKD_DIV1;
    tim_init.TIM_RepetitionCounter = 0;

    TIM_TimeBaseInit(TIM2, &tim_init);

    // No interrupts necessary for data timer, as it is only used for triggering DMA transfers
    /*
    TIM_ClearITPendingBit(TIM2, TIM_IT_Update);
    TIM_ITConfig(TIM2, TIM_IT_Update, ENABLE);

    NVIC_InitTypeDef nvic_init;
    nvic_init.NVIC_IRQChannel = TIM2_IRQn;
    nvic_init.NVIC_IRQChannelPreemptionPriority = 0;
    nvic_init.NVIC_IRQChannelSubPriority = 1;
    nvic_init.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&nvic_init);
    */

    TIM_Cmd(TIM2, ENABLE);
}

void pwm_data_timer_dma_request_enable(bool enabled)
{
    // TIM2 Update DMA requests are routed to DMA1 Channel2
    TIM_DMACmd(TIM2, TIM_DMA_Update, enabled ? ENABLE : DISABLE);
}

void pwm_data_timer_uninit()
{
    TIM_Cmd(TIM2, DISABLE);
}

void pwm_timer_init(uint32_t frequency_hz_100)
{
    TIM_DeInit(TIM15);
    GPIO_PinRemapConfig(GPIO_Remap_TIM15, DISABLE);
    // Not needed: AFIO->MAPR2 |= AFIO_MAPR2_TIM15_REMAP;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM15, ENABLE);
    RCC_APB2PeriphResetCmd(RCC_APB2Periph_TIM15, DISABLE);

    TIM_TimeBaseInitTypeDef tim_init;

    // Not needed: TIM_InternalClockConfig(TIM15);

    tim_init.TIM_Prescaler = 24 - 1; // tick every 1/1000000 s
    tim_init.TIM_CounterMode = TIM_CounterMode_Up;
    tim_init.TIM_Period = (uint16_t) (((100.0f * 1000000.0f) / (frequency_hz_100 * 2.0f))) - 1;
    tim_init.TIM_ClockDivision = TIM_CKD_DIV1;
    tim_init.TIM_RepetitionCounter = 0;

    TIM_TimeBaseInit(TIM15, &tim_init);

    TIM_OCInitTypeDef TIM15_OCInitStruct;

    TIM_OCStructInit(&TIM15_OCInitStruct);
    TIM15_OCInitStruct.TIM_Pulse = 0; // Was: tim_init.TIM_Period / 2
    TIM15_OCInitStruct.TIM_OCMode = TIM_OCMode_Toggle; // Was: TIM_OCMode_PWM1
    TIM15_OCInitStruct.TIM_OutputState = TIM_OutputState_Enable;
    TIM15_OCInitStruct.TIM_OCPolarity = TIM_OCPolarity_High;

    TIM_OC2Init(TIM15, &TIM15_OCInitStruct);

    // These are not needed?
    // TIM_SelectOCxM(TIM15, TIM_Channel_2, TIM_OCMode_PWM1);
    // TIM_CCxCmd(TIM15, TIM_Channel_2, TIM_CCx_Enable);

    // The following settings make transitions between generated frequencies smooth
    TIM_ARRPreloadConfig(TIM15, ENABLE);
    TIM_OC2PreloadConfig(TIM15, TIM_OCPreload_Enable);

    TIM_OC2FastConfig(TIM15, TIM_OCFast_Enable);

    TIM_CtrlPWMOutputs(TIM15, DISABLE);

    TIM_Cmd(TIM15, ENABLE);
}

static void pwm_dma_init_channel()
{
    DMA_InitTypeDef dma_init;
    dma_init.DMA_BufferSize = PWM_TIMER_DMA_BUFFER_SIZE;
    dma_init.DMA_DIR = DMA_DIR_PeripheralDST;
    dma_init.DMA_M2M = DMA_M2M_Disable;
    dma_init.DMA_MemoryBaseAddr = (uint32_t) pwm_timer_dma_buffer;
    dma_init.DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord;
    dma_init.DMA_MemoryInc = DMA_MemoryInc_Enable;
    dma_init.DMA_Mode = DMA_Mode_Circular;
    dma_init.DMA_PeripheralBaseAddr = (uint32_t) &TIM15->ARR;
    dma_init.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord; // DMA_PeripheralDataSize_Word ?
    dma_init.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
    dma_init.DMA_Priority = DMA_Priority_VeryHigh;
    DMA_Init(pwm_dma_channel, &dma_init);
}

inline void pwm_dma_interrupt_enable(bool enabled)
{
    DMA_ClearITPendingBit(DMA1_IT_HT2);
    DMA_ClearITPendingBit(DMA1_IT_TC2);
    DMA_ITConfig(pwm_dma_channel, DMA_IT_HT | DMA_IT_TC | DMA_IT_TE, enabled ? ENABLE : DISABLE);
}

void pwm_dma_init()
{
    DMA_DeInit(pwm_dma_channel);

    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);

    pwm_dma_init_channel();

    pwm_dma_interrupt_enable(true);

    DMA_Cmd(pwm_dma_channel, ENABLE);

    NVIC_InitTypeDef nvic_init;
    nvic_init.NVIC_IRQChannel = DMA1_Channel2_IRQn;
    nvic_init.NVIC_IRQChannelPreemptionPriority = 0;
    nvic_init.NVIC_IRQChannelSubPriority = 1;
    nvic_init.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&nvic_init);
}

void pwm_dma_start()
{
    //pwm_dma_init_channel();
    //pwm_dma_interrupt_enable(true);
    // TODO: Why doesn't timer DMA request restart without reinitializing the timer?
    pwm_timer_init(100 * 100);
    pwm_timer_pwm_enable(true);
    pwm_timer_use(true);
    DMA_SetCurrDataCounter(pwm_dma_channel, PWM_TIMER_DMA_BUFFER_SIZE);
    DMA_Cmd(pwm_dma_channel, ENABLE);
    pwm_data_timer_dma_request_enable(true);
}

void pwm_dma_stop()
{
    pwm_data_timer_dma_request_enable(false);
    DMA_Cmd(pwm_dma_channel, DISABLE);
    //pwm_dma_interrupt_enable(false);
}

void DMA1_Channel2_IRQHandler(void)
{
    if (DMA_GetITStatus(DMA1_IT_TE2)) {
        DMA_ClearITPendingBit(DMA1_IT_TE2);
        log_info("DMA Transfer Error\n");
    }
    if (DMA_GetITStatus(DMA1_IT_HT2)) {
        DMA_ClearITPendingBit(DMA1_IT_HT2);
        pwm_handle_dma_transfer_half(PWM_TIMER_DMA_BUFFER_SIZE, pwm_timer_dma_buffer);
    }
    if (DMA_GetITStatus(DMA1_IT_TC2)) {
        DMA_ClearITPendingBit(DMA1_IT_TC2);
        pwm_handle_dma_transfer_full(PWM_TIMER_DMA_BUFFER_SIZE, pwm_timer_dma_buffer);
    }
}

void pwm_timer_pwm_enable(bool enabled)
{
    TIM_CtrlPWMOutputs(TIM15, enabled ? ENABLE : DISABLE);
}

void pwm_timer_use(bool use)
{
    GPIO_PinRemapConfig(GPIO_Remap_TIM15, use ? ENABLE : DISABLE);
}

void pwm_timer_uninit()
{
    TIM_CtrlPWMOutputs(TIM15, DISABLE);
    TIM_Cmd(TIM15, DISABLE);

    TIM_DeInit(TIM15);

    GPIO_PinRemapConfig(GPIO_Remap_TIM15, DISABLE);

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM15, DISABLE);
}

inline uint16_t pwm_calculate_period(uint32_t frequency_hz_100)
{
    return (uint16_t) (((100.0f * 1000000.0f) / (frequency_hz_100 * 2.0f))) - 1;
}

inline void pwm_timer_set_frequency(uint32_t pwm_period)
{
    // TIM_CtrlPWMOutputs(TIM15, DISABLE);
    // TIM_Cmd(TIM15, DISABLE);

    TIM_SetAutoreload(TIM15, pwm_period);
    // TIM_SetCompare2(TIM15, pwm_period / 2);

    // TIM_Cmd(TIM15, ENABLE);
    // TIM_CtrlPWMOutputs(TIM15, ENABLE);
}
