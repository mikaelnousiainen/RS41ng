#include "config.h"

#ifdef DFM17

#include <stm32f1xx_hal.h>
#include "system.h"
#include "clock_calibration.h"
#include "radio_internal.h"
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
 *      calling si4063_set_crystal_capacitance(). Uses a Perturb & Observe (P&O)
 *      algorithm to handle the non-monotonic XO_TUNE frequency response:
 *      steps cap_trim_offset, observes whether error improved, reverses if not.
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

// ---- GPS-disciplined Si4063 XO_TUNE state (Perturb & Observe) ---------------
//
// The XO_TUNE register has a non-monotonic frequency response, so a simple
// integrating loop cannot determine the correct correction direction.
// Instead, a P&O algorithm steps cap_trim_offset, observes whether the error
// improved, and reverses direction if it worsened.

// Signed offset applied on top of the temperature LUT baseline.
// Volatile because it is written in the ISR and read from the main context.
static volatile int cap_trim_offset = 0;

// Last measured timepulse error in µs (delta_ticks - 1,000,000).
// Nominally 0; sign indicates direction of frequency error.
static volatile int32_t last_us_error = 0;

// P&O state machine
typedef enum {
    PO_STARTUP,     // Collecting initial baseline measurement
    PO_SETTLING,    // Waiting for a step to take effect on hardware
    PO_OBSERVING,   // Collecting post-step error samples
    PO_LOCKED       // Error within dead band, monitoring for drift
} po_state_t;

static volatile po_state_t po_state = PO_STARTUP;
static int8_t   step_direction      = +1;      // +1 or -1
static uint8_t  settle_countdown    = 0;
static uint8_t  obs_count           = 0;
static uint32_t obs_abs_error_sum   = 0;
static uint32_t baseline_abs_error  = 0;
static uint8_t  locked_count        = 0;
static uint8_t  bad_pulse_count     = 0;
static bool     last_tx_active      = false;

// Expected timer ticks for a 1-second GPS timepulse at 1 MHz.
#define TIMEPULSE_EXPECTED_TICKS    1000000UL

// Maximum GPS-derived offset from the temperature LUT baseline (±steps).
#define CAP_TRIM_OFFSET_MAX     30
#define CAP_TRIM_OFFSET_MIN    -90

// Sanity window: reject timepulse deltas outside this range (µs).
#define TIMEPULSE_MIN_TICKS     950000UL    // 0.95 s
#define TIMEPULSE_MAX_TICKS    1050000UL    // 1.05 s

// P&O tuning constants
#define PO_SETTLE_PULSES         3    // pulses to wait after stepping
#define PO_OBSERVE_PULSES        4    // pulses to average per measurement
#define PO_DEADBAND_US           1    // ±1 µs (~1 ppm) = "locked"
#define PO_LOCK_THRESHOLD        3    // consecutive in-deadband before LOCKED
#define PO_LOCKED_RECHECK_PULSES 8    // check for drift every 8s when locked
#define PO_MAX_BAD_PULSES       10    // consecutive bad pulses → reset to STARTUP

// ---- P&O helper -------------------------------------------------------------

static void po_apply_step(int8_t direction)
{
    int new_val = cap_trim_offset + direction;
    if (new_val < CAP_TRIM_OFFSET_MIN) new_val = CAP_TRIM_OFFSET_MIN;
    if (new_val > CAP_TRIM_OFFSET_MAX) new_val = CAP_TRIM_OFFSET_MAX;

    if (new_val == cap_trim_offset) {
        // Hit the rail — force a direction reversal
        step_direction = -step_direction;
    }
    cap_trim_offset = new_val;
}

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

uint8_t clock_calibration_get_po_state()
{
    return (uint8_t)po_state;
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
    cap_trim_offset     = 0;
    last_us_error       = 0;
    po_state            = PO_STARTUP;
    step_direction      = +1;
    settle_countdown    = 0;
    obs_count           = 0;
    obs_abs_error_sum   = 0;
    baseline_abs_error  = 0;
    locked_count        = 0;
    bad_pulse_count     = 0;
    last_capture        = 0;
    tim4_overflow    = 0;
    last_tx_active      = false;

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
            bad_pulse_count++;
            if (bad_pulse_count >= PO_MAX_BAD_PULSES) {
                // Long GPS outage — restart P&O but keep current cap_trim_offset
                po_state = PO_STARTUP;
                obs_count = 0;
                obs_abs_error_sum = 0;
                locked_count = 0;
                bad_pulse_count = 0;
            }
            return;
        }
        bad_pulse_count = 0;

        millis_delta = delta_ticks / 1000U;

        // ---- Detect TX state transitions ------------------------------------
        // If radio_transmission_active changed since the last timepulse, the
        // observation window spans two different operating conditions (TX vs
        // idle).  Discard any partially-accumulated observation so the P&O
        // never acts on mixed-state data.  In continuous-TX or continuous-idle
        // modes there are no transitions, so the P&O runs unimpeded.

        bool tx_now = radio_shared_state.radio_transmission_active;
        if (tx_now != last_tx_active) {
            last_tx_active = tx_now;
            if (po_state == PO_OBSERVING || po_state == PO_STARTUP) {
                obs_count = 0;
                obs_abs_error_sum = 0;
            } else if (po_state == PO_SETTLING) {
                settle_countdown = PO_SETTLE_PULSES;
            }
            // PO_LOCKED: let the normal drift-recheck handle it
        }

        // ---- Si4063 XO_TUNE Perturb & Observe GPS PLL -----------------------

        int32_t error = (int32_t)delta_ticks - (int32_t)TIMEPULSE_EXPECTED_TICKS;
        last_us_error = error;
        uint32_t abs_error = (error >= 0) ? (uint32_t)error : (uint32_t)(-error);

        switch (po_state) {
        case PO_STARTUP:
            // Collect baseline avg(|error|) before any perturbation.
            obs_abs_error_sum += abs_error;
            obs_count++;
            if (obs_count >= PO_OBSERVE_PULSES) {
                baseline_abs_error = obs_abs_error_sum / obs_count;
                obs_abs_error_sum = 0;
                obs_count = 0;
                if (baseline_abs_error < PO_DEADBAND_US) {
                    locked_count = PO_LOCK_THRESHOLD;
                    po_state = PO_LOCKED;
                    settle_countdown = PO_LOCKED_RECHECK_PULSES;
                } else {
                    po_apply_step(step_direction);
                    settle_countdown = PO_SETTLE_PULSES;
                    po_state = PO_SETTLING;
                }
            }
            break;

        case PO_SETTLING:
            // Wait for hardware to apply the new cap_trim_offset.
            if (--settle_countdown == 0) {
                obs_abs_error_sum = 0;
                obs_count = 0;
                po_state = PO_OBSERVING;
            }
            break;

        case PO_OBSERVING:
            // Collect post-step avg(|error|) and compare to baseline.
            obs_abs_error_sum += abs_error;
            obs_count++;
            if (obs_count >= PO_OBSERVE_PULSES) {
                uint32_t new_avg = obs_abs_error_sum / obs_count;
                obs_abs_error_sum = 0;
                obs_count = 0;

                if (new_avg < PO_DEADBAND_US) {
                    // Within dead band — count toward lock
                    locked_count++;
                    if (locked_count >= PO_LOCK_THRESHOLD) {
                        po_state = PO_LOCKED;
                        settle_countdown = PO_LOCKED_RECHECK_PULSES;
                    } else {
                        baseline_abs_error = new_avg;
                        po_state = PO_STARTUP;
                    }
                } else if (new_avg < baseline_abs_error) {
                    // Step helped — keep going in the same direction
                    locked_count = 0;
                    baseline_abs_error = new_avg;
                    po_apply_step(step_direction);
                    settle_countdown = PO_SETTLE_PULSES;
                    po_state = PO_SETTLING;
                } else {
                    // Step hurt or no change — reverse direction
                    locked_count = 0;
                    step_direction = -step_direction;
                    po_apply_step(step_direction);  // undo bad step
                    po_apply_step(step_direction);  // step in new direction
                    baseline_abs_error = new_avg;
                    settle_countdown = PO_SETTLE_PULSES;
                    po_state = PO_SETTLING;
                }
            }
            break;

        case PO_LOCKED:
            // Periodically re-check for drift.
            if (--settle_countdown == 0) {
                if (abs_error >= PO_DEADBAND_US + 1) {
                    // Drifted out of lock — restart P&O
                    locked_count = 0;
                    po_state = PO_STARTUP;
                    obs_abs_error_sum = 0;
                    obs_count = 0;
                } else {
                    settle_countdown = PO_LOCKED_RECHECK_PULSES;
                }
            }
            break;
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
