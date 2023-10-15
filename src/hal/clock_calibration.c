#include "config.h"

#ifdef DFM17

#include "stm32f10x_exti.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x.h"
#include "stm32f10x_rcc.h"
#include "misc.h"
#include "system.h"
#include "millis.h"
#include "clock_calibration.h"

// The HSI (internal oscillator) trim register mask, copied from stm_lib/src/stm32f10x_rcc.c
#define CR_HSITRIM_Mask           ((uint32_t)0xFFFFFF07)

// Register definition for reading the HSI current trim out of the Calibration Register (CR).
// Resulting value will be between 0-31.
#define CURRENT_TRIM    ((RCC->CR & ~CR_HSITRIM_Mask) >>3)

/**
 * On the DFM-17, GPIO PB8 is wired to the GPS Timepulse. We take advantage of this to do a
 * processor speed calibration. HSITRIM[4:0] allows for 32 values to adjust the HSI clock
 * speed. The center (16) value is "neutral". Each trim value above or below 16 adjusts
 * the clock by approximately 40kHZ (0.5% of the 8MHZ clock speed) (per AN2868).
 * 0.5% is about 5ms per second, so if we detect that we're off by more than 5 milliseconds between time pulses,
 * we will suggest a recalibration.  The "trim_suggestion" variable is a static that will be maintained
 * by the time pulse IRQ and can be used at any time it's convenient to adjust the clock speed.
*/

// Defaults, will be set it in the init routine below.
int trim_suggestion = 16;
int trim_current = 16;

uint32_t old_millis = 0;
uint16_t calibration_change_count = 0;

bool calibration_indicator_state = true;

uint8_t clock_calibration_get_trim()
{
    return CURRENT_TRIM;
}

uint16_t clock_calibration_get_change_count()
{
    return calibration_change_count;
}

void clock_calibration_adjust()
{
    if (trim_suggestion == trim_current) {
        return;
    }

    RCC_AdjustHSICalibrationValue(trim_suggestion);
    trim_current = trim_suggestion;

    calibration_change_count++;

    calibration_indicator_state = !calibration_indicator_state;
    system_set_yellow_led(calibration_indicator_state);
}

void timepulse_init()
{
    // Initialize pin PB8 as floating input
    GPIO_InitTypeDef gpio_init;
    gpio_init.GPIO_Pin = GPIO_Pin_8;
    gpio_init.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    gpio_init.GPIO_Speed = GPIO_Speed_10MHz;
    GPIO_Init(GPIOB, &gpio_init);

    // PB8 is connected to interrupt line 8, set trigger on the configured edge and enable the interrupt
    EXTI_InitTypeDef exti_init;
    exti_init.EXTI_Line = EXTI_Line8;
    exti_init.EXTI_Mode = EXTI_Mode_Interrupt;
    exti_init.EXTI_Trigger = EXTI_Trigger_Rising;
    exti_init.EXTI_LineCmd = ENABLE;
    EXTI_Init(&exti_init);

    // Attach interrupt line to port B
    GPIO_EXTILineConfig(GPIO_PortSourceGPIOB, GPIO_PinSource8);

    // PB8 is connected to EXTI_Line8, which has EXTI9_5_IRQn vector. Use priority 0 for now.
    NVIC_InitTypeDef NVIC_InitStruct;
    NVIC_InitStruct.NVIC_IRQChannel = EXTI9_5_IRQn;
    NVIC_InitStruct.NVIC_IRQChannelPreemptionPriority = 0;
    NVIC_InitStruct.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStruct.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStruct);

    // Pull the current calibration to start
    trim_current = CURRENT_TRIM;
    trim_suggestion = trim_current;

    // Set the yellow LED to help identify calibration changes
    system_set_yellow_led(calibration_indicator_state);
}

// This handler is (at present) only being used for the GPS time pulse interrupt,
// so we shouldn't need to do additional testing for the cause of the interrupt.

void EXTI9_5_IRQHandler(void)
{
    uint32_t current_millis = millis();

    EXTI_ClearITPendingBit(EXTI_Line8);

    if (old_millis == 0) {
        // First timepulse. Just store millis.
        old_millis = current_millis;
        return;
    }

    if (current_millis < old_millis) {
        // Milliseconds value wrapped to zero. Wait for the next interrupt.
        return;
    }

    // Calculate milliseconds since last timepulse. Ideally there were 1000.
    uint32_t millis_delta = current_millis - old_millis;
    old_millis = current_millis;

    // If too few clicks, speed up clock.  If too many, slow down.
    int delta = (int) (1000 - millis_delta) / 5;

    // Take one step at a time in case we had a bad clock tick
    if (delta > 1) {
        delta = 1;
    }
    if (delta < -1) {
        delta = -1;
    }

    // Don't allow calibration suggestion to go out of range
    if (((delta + trim_current) >= 0) &&
        ((delta + trim_current <= 31))) {
        // If the delta makes sense, apply to the suggestion.  Otherwise, skip.
        trim_suggestion = trim_current + delta;
    }
}

#endif
