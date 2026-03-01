#include "config.h"

#ifdef DFM17

#include <stm32f1xx_hal.h>
#include "system.h"
#include "clock_calibration.h"

// Register definition for reading the HSI current trim out of the Calibration Register (CR).
// Resulting value will be between 0-31.
#define CURRENT_TRIM    ((RCC->CR & RCC_CR_HSITRIM) >> RCC_CR_HSITRIM_Pos)

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

    // __HAL_RCC_HSI_CALIBRATIONVALUE_ADJUST(trim_suggestion);
    trim_current = trim_suggestion;

    calibration_change_count++;

    calibration_indicator_state = !calibration_indicator_state;
    system_set_yellow_led(calibration_indicator_state);
}

void timepulse_init()
{
    // Initialize pin PB8 as floating input with rising-edge EXTI interrupt
    GPIO_InitTypeDef gpio_init = {0};
    gpio_init.Pin = GPIO_PIN_8;
    gpio_init.Mode = GPIO_MODE_IT_RISING;
    gpio_init.Pull = GPIO_NOPULL;
    gpio_init.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOB, &gpio_init);

    // PB8 is connected to EXTI_Line8, which has EXTI9_5_IRQn vector. Use priority 0 for now.
    HAL_NVIC_SetPriority(EXTI9_5_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(EXTI9_5_IRQn);

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
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_8);
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    if (GPIO_Pin != GPIO_PIN_8) {
        return;
    }

    uint32_t current_millis = HAL_GetTick();

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

#endif // DFM17
