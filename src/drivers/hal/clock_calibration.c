#include "config.h"

#ifdef DFM17

#include <stm32f1xx_hal.h>
#include "system.h"
#include "clock_calibration.h"
#include "log.h"

// Register definition for reading the HSI current trim out of the Calibration Register (CR).
// Resulting value will be between 0-31.
#define CURRENT_TRIM    ((RCC->CR & RCC_CR_HSITRIM) >> RCC_CR_HSITRIM_Pos)

/**
 * GPS-disciplined oscillator (GPSDO) for the DFM-17 Si4063.
 *
 * GPS timepulse on PB8 is routed to TIM4_CH3 (default mapping on STM32F100) for
 * hardware input capture at 1 µs resolution (24 MHz PCLK / prescaler 24 = 1 MHz).
 * A 32-bit virtual counter is maintained: (tim4_overflow << 16) | CCR3.
 *
 * Each GPS timepulse captures a timestamp. The delta between consecutive captures
 * (expected: 1,000,000 µs) feeds two control loops:
 *
 *   1. HSI trim suggestion (HSITRIM[4:0], 0-31, ~40 kHz/step at 8 MHz, ~10,000 ppm/step
 *      after the ×3 PLL to 24 MHz). Apply via clock_calibration_adjust() from main loop.
 *      Currently commented out in main.c; kept for reference and optional use.
 *
 *   2. Si4063 XO_TUNE GPS PLL (cap_trim_offset, ±CAP_TRIM_OFFSET_MAX steps).
 *      A signed offset added to the temperature LUT value in telemetry.c before
 *      calling si4063_set_crystal_capacitance(). Uses a slow error accumulator so
 *      sub-ppm residuals integrate into corrections over multiple seconds.
 *      Each XO_TUNE step ≈ 1-3 ppm; with 1 µs resolution the loop is sensitive to
 *      errors well below one step.
 *
 * The temperature LUT handles the large open-loop temperature compensation.
 * The GPS PLL handles manufacturing variance, aging, and LUT residual error.
 * Both corrections are additive: applied cap = c_value[t_look] + cap_trim_offset.
 */

// ---- TIM4 input capture state -----------------------------------------------

static volatile uint32_t tim4_overflow   = 0;
static volatile uint32_t last_capture    = 0;

// ---- HSI trim state (legacy, kept for optional use) -------------------------

int trim_suggestion  = 16;
int trim_current     = 16;
uint32_t millis_delta = 0;
uint16_t calibration_change_count = 0;
bool calibration_indicator_state = true;

// ---- GPS-disciplined Si4063 XO_TUNE state -----------------------------------

// Signed offset applied on top of the temperature LUT baseline.
// Volatile because it is written in the ISR and read from the main context.
static volatile int cap_trim_offset = 0;

// Last measured timepulse error in µs (delta_ticks - 1,000,000).
// Nominally 0; sign indicates direction of frequency error.
static volatile int32_t last_us_error = 0;

// Integrating error accumulator (µs). One XO_TUNE step is issued per
// CAP_TRIM_ACCUM_THRESHOLD µs of accumulated error.
// Tune CAP_TRIM_ACCUM_THRESHOLD to set the loop speed:
//   Lower value → faster convergence but more noise sensitivity.
//   Higher value → slower convergence but smoother output.
//   Starting point: 4 µs ≈ 4 ppm (a few seconds to act on a ~1 ppm offset).
static int32_t cap_error_accumulator = 0;

// Expected timer ticks for a 1-second GPS timepulse at 1 MHz.
#define TIMEPULSE_EXPECTED_TICKS    1000000UL

// Accumulator threshold for one XO_TUNE step (µs). 
#define CAP_TRIM_ACCUM_THRESHOLD    4

// Maximum GPS-derived offset from the temperature LUT baseline (±steps).
// Limits runaway if GPS timepulse quality is poor.
#define CAP_TRIM_OFFSET_MAX     30
#define CAP_TRIM_OFFSET_MIN    -30

// Sanity window: reject timepulse deltas outside this range (µs).
// Catches GPS dropouts, counter wrap, and multi-pulse events.
#define TIMEPULSE_MIN_TICKS     950000UL    // 0.95 s
#define TIMEPULSE_MAX_TICKS    1050000UL    // 1.05 s

// ---- Public getters ---------------------------------------------------------

uint8_t clock_calibration_get_trim()
{
    return CURRENT_TRIM;
}

uint16_t clock_calibration_get_change_count()
{
    return calibration_change_count;
}

uint32_t clock_calibration_get_millis_delta()
{
    return millis_delta;
}

int clock_calibration_get_cap_trim_offset()
{
    return cap_trim_offset;
}

int32_t clock_calibration_get_us_error()
{
    return last_us_error;
}

// ---- HSI trim application (optional, call from main loop) -------------------

void clock_calibration_adjust()
{
    if (trim_suggestion == trim_current) {
        return;
    }

    __HAL_RCC_HSI_CALIBRATIONVALUE_ADJUST(trim_suggestion);
    trim_current = trim_suggestion;

    calibration_change_count++;

    calibration_indicator_state = !calibration_indicator_state;
    system_set_yellow_led(calibration_indicator_state);
}

// ---- TIM4 initialisation ----------------------------------------------------

static void tim4_init(void)
{
    __HAL_RCC_TIM4_CLK_ENABLE();

    // Free-running 1 MHz up-counter (16-bit, overflows every 65.536 ms).
    // Prescaler: 24 MHz PCLK1 / 24 = 1 MHz → 1 µs per tick.
    TIM4->PSC  = 24U - 1U;
    TIM4->ARR  = 0xFFFFU;
    TIM4->CNT  = 0U;
    TIM4->CR1  = TIM_CR1_CEN;   // enable, up-count, no preload

    // CH3 input capture on TI3 (PB8), 4-clock noise filter, no prescaler.
    // CCMR2[7:0] controls CH3: CC3S=01 (IC3←TI3), IC3F=0100 (4 clocks filter).
    TIM4->CCMR2 = (TIM4->CCMR2 & 0xFFFFFF00U)
                | (0x01U << 0U)    // CC3S = 01 → IC3 mapped to TI3
                | (0x04U << 4U);   // IC3F = 0100 → 4-clock debounce filter

    // CC3E=1 (capture enabled), CC3P=0 (rising edge), CC3NP=0.
    TIM4->CCER = (TIM4->CCER & ~(TIM_CCER_CC3P | TIM_CCER_CC3NP))
               | TIM_CCER_CC3E;

    // Enable overflow (UIE) and channel-3 capture (CC3IE) interrupts.
    TIM4->DIER = TIM_DIER_UIE | TIM_DIER_CC3IE;

    // Clear any stale flags before enabling NVIC.
    TIM4->SR = 0U;
}

// ---- Public init ------------------------------------------------------------

void timepulse_init()
{
    // Configure PB8 as a plain input (pull-down) for TIM4_CH3.
    // STM32F1 timer input pins do not need alternate-function mode.
    GPIO_InitTypeDef gpio_init = {0};
    gpio_init.Pin   = GPIO_PIN_8;
    gpio_init.Mode  = GPIO_MODE_INPUT;
    gpio_init.Pull  = GPIO_PULLDOWN;
    gpio_init.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOB, &gpio_init);

    tim4_init();

    // Same priority as the former EXTI9_5: lower urgency than GPS USART (5).
    HAL_NVIC_SetPriority(TIM4_IRQn, 6, 0);
    HAL_NVIC_EnableIRQ(TIM4_IRQn);

    // Seed trim tracking from current hardware state.
    trim_current     = CURRENT_TRIM;
    trim_suggestion  = trim_current;
    cap_trim_offset  = 0;
    cap_error_accumulator = 0;
    last_capture     = 0;
    tim4_overflow    = 0;

    system_set_yellow_led(calibration_indicator_state);
}

// ---- TIM4 ISR ---------------------------------------------------------------

void TIM4_IRQHandler(void)
{
    uint32_t sr = TIM4->SR;

    // --- Overflow event: extend the 16-bit counter to 32 bits ---
    if (sr & TIM_SR_UIF) {
        TIM4->SR = ~TIM_SR_UIF;   // rc_w0: writing 0 clears the flag
        tim4_overflow++;
    }

    // --- CH3 input capture: GPS timepulse arrived ---
    if (sr & TIM_SR_CC3IF) {
        TIM4->SR = ~TIM_SR_CC3IF;

        // Build a 32-bit virtual timestamp from the overflow counter and
        // the captured 16-bit value. Handle the race where the counter
        // overflowed simultaneously with the capture: if the overflow flag
        // is now set and the capture value is in the lower half of the range,
        // the overflow occurred just before the capture was latched.
        uint16_t ccr3     = (uint16_t)TIM4->CCR3;
        uint32_t hi       = tim4_overflow;
        if ((TIM4->SR & TIM_SR_UIF) && (ccr3 < 0x8000U)) {
            hi++;
        }
        uint32_t captured = (hi << 16U) | (uint32_t)ccr3;

        // First pulse: seed the reference timestamp and wait for the next.
        if (last_capture == 0U) {
            last_capture = captured;
            return;
        }

        uint32_t delta_ticks = captured - last_capture;  // unsigned wrap-safe
        last_capture = captured;

        // Sanity-check the interval. Reject outliers caused by GPS dropouts,
        // spurious edges, or the 32-bit virtual counter wrapping after ~72 min.
        if (delta_ticks < TIMEPULSE_MIN_TICKS || delta_ticks > TIMEPULSE_MAX_TICKS) {
            cap_error_accumulator = 0;  // reset integrator on bad pulse
            return;
        }

        millis_delta = delta_ticks / 1000U;

        // ---- Si4063 XO_TUNE GPS PLL (integrating loop) ----------------------
        //
        // error > 0: interval too long  → MCU/XO running slow → increase cap → cap_trim_offset++
        // error < 0: interval too short → MCU/XO running fast → decrease cap → cap_trim_offset--
        //
        // The accumulator absorbs sub-step errors. A correction is issued only
        // when the accumulated error exceeds one threshold unit (≈ a few µs).

        int32_t error = (int32_t)delta_ticks - (int32_t)TIMEPULSE_EXPECTED_TICKS;
        last_us_error = error;
        cap_error_accumulator -= error;

        if (cap_error_accumulator >= (int32_t)CAP_TRIM_ACCUM_THRESHOLD) {
            if (cap_trim_offset > CAP_TRIM_OFFSET_MIN) {
                cap_trim_offset--;
            }
            cap_error_accumulator -= (int32_t)CAP_TRIM_ACCUM_THRESHOLD;
        } else if (cap_error_accumulator <= -(int32_t)CAP_TRIM_ACCUM_THRESHOLD) {
            if (cap_trim_offset < CAP_TRIM_OFFSET_MAX) {
                cap_trim_offset++;
            }
            cap_error_accumulator += (int32_t)CAP_TRIM_ACCUM_THRESHOLD;
        }

        // ---- Legacy HSI trim suggestion (coarser, kept for optional use) ----
        //
        // Each HSITRIM step ≈ 40 kHz at 8 MHz HSI → ~10,000 ppm at 24 MHz SYSCLK.
        // The 1 ms divisor gives limited resolution; the XO_TUNE loop above is
        // far more sensitive. Uncomment clock_calibration_adjust() in main.c to
        // apply HSITRIM adjustments.

        int32_t ms_error     = 1000 - (int32_t)millis_delta;
        int     hsitrim_step = (int)(ms_error / 5);
        if (hsitrim_step >  1) hsitrim_step =  1;
        if (hsitrim_step < -1) hsitrim_step = -1;
        int new_trim = trim_current + hsitrim_step;
        if (new_trim >= 0 && new_trim <= 31) {
            trim_suggestion = new_trim;
        }
    }
}

#endif // DFM17
