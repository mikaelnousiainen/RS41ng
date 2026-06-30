/*
 * Vaisala RS41 sensor-boom reader -- clean-room implementation.
 * See vaisala_boom.h for the source/provenance notes and licensing (GPL-2.0).
 */

#include "config.h"

#if SENSOR_VAISALA_BOOM_ENABLE

#ifdef RS41_RSM4x4
#include <stm32l4xx_hal.h>
#else
#include <stm32f1xx_hal.h>
#endif

#include "vaisala_boom.h"
#include "gpio.h"
#include "hal/delay.h"
#include "log.h"
#include <math.h>

#if SENSOR_VAISALA_BOOM_PRESSURE_ENABLE
#include <string.h>
extern SPI_HandleTypeDef hspi;        // SPI2, initialised by spi_init() (radio bus)
#define RPM411_CS_PORT  GPIOB
#define RPM411_CS_PIN   GPIO_PIN_2    // RPM411 chip-select (PB2)
#endif

/* ---- Board pin map ---------------------------------------------------------
 * Net names + the F100 pins are taken from the bazjo/radiosonde_hardware
 * schematic; the RSM4x4 (L412) pins are hardware facts (to verify on hardware).
 * Boom oscillator output (MEAS_OUT) is on PA1 = TIM2_CH2 on both boards. */

#define OSC_OUT_PORT      GPIOA
#define OSC_OUT_PIN       GPIO_PIN_1        // PA1, TIM2_CH2 (MEAS_OUT)

typedef struct { GPIO_TypeDef *port; uint16_t pin; } boom_pin;

#if defined(RS41_RSM4x4)   // RSM4x4 / RSM4x5, STM32L412 (pins to verify)
#define OSC_EN_TEMP_PORT  GPIOB
#define OSC_EN_TEMP_PIN   GPIO_PIN_12       // PULLUP_TM
#define OSC_EN_HYG_PORT   GPIOA
#define OSC_EN_HYG_PIN    GPIO_PIN_2        // PULLUP_HYG
static const boom_pin boom_switch[BOOM_CHANNEL_COUNT] = {
    [BOOM_REF_R1]   = { GPIOB, GPIO_PIN_3  },   // SPST1
    [BOOM_REF_R2]   = { GPIOA, GPIO_PIN_3  },   // SPST2
    [BOOM_T_HEATER] = { GPIOC, GPIO_PIN_14 },   // SPST3
    [BOOM_T_AIR]    = { GPIOC, GPIO_PIN_15 },   // SPST4
    [BOOM_HUMIDITY] = { GPIOC, GPIO_PIN_11 },   // SPDT2
    [BOOM_REF_C_HI] = { GPIOC, GPIO_PIN_10 },   // SPDT1
    [BOOM_REF_C_LO] = { GPIOC, GPIO_PIN_12 },   // SPDT3
};
#else                       // RSM4x2 / RSM4x1, STM32F100 (from bazjo schematic)
#define OSC_EN_TEMP_PORT  GPIOB
#define OSC_EN_TEMP_PIN   GPIO_PIN_12       // PULLUP_TM
#define OSC_EN_HYG_PORT   GPIOA
#define OSC_EN_HYG_PIN    GPIO_PIN_2        // PULLUP_HYG
static const boom_pin boom_switch[BOOM_CHANNEL_COUNT] = {
    [BOOM_REF_R1]   = { GPIOB, GPIO_PIN_6  },   // SPST1
    [BOOM_REF_R2]   = { GPIOA, GPIO_PIN_3  },   // SPST2
    [BOOM_T_HEATER] = { GPIOC, GPIO_PIN_14 },   // SPST3
    [BOOM_T_AIR]    = { GPIOC, GPIO_PIN_15 },   // SPST4
    [BOOM_HUMIDITY] = { GPIOB, GPIO_PIN_4  },   // SPDT2
    [BOOM_REF_C_HI] = { GPIOB, GPIO_PIN_3  },   // SPDT1
    [BOOM_REF_C_LO] = { GPIOB, GPIO_PIN_5  },   // SPDT3
};
#endif

static inline bool channel_is_humidity(vaisala_boom_channel c)
{
    return c >= BOOM_HUMIDITY;
}

static TIM_HandleTypeDef boom_tim;   // TIM2, input capture on CH2 (PA1)

/* Drive a GPIO as push-pull output. */
static void out_pin(GPIO_TypeDef *port, uint16_t pin)
{
    GPIO_InitTypeDef g = {0};
    g.Pin = pin; g.Mode = GPIO_MODE_OUTPUT_PP; g.Pull = GPIO_NOPULL;
    g.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(port, &g);
}

void vaisala_boom_init(void)
{
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();

    out_pin(OSC_EN_TEMP_PORT, OSC_EN_TEMP_PIN);
    out_pin(OSC_EN_HYG_PORT,  OSC_EN_HYG_PIN);
    for (int c = 0; c < BOOM_CHANNEL_COUNT; c++) {
        out_pin(boom_switch[c].port, boom_switch[c].pin);
        HAL_GPIO_WritePin(boom_switch[c].port, boom_switch[c].pin, GPIO_PIN_RESET);
    }
    // Both oscillators off initially (active-high enables idle low).
    HAL_GPIO_WritePin(OSC_EN_TEMP_PORT, OSC_EN_TEMP_PIN, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(OSC_EN_HYG_PORT,  OSC_EN_HYG_PIN,  GPIO_PIN_RESET);

#if SENSOR_VAISALA_BOOM_PRESSURE_ENABLE
    // RPM411 chip-select (idle high) and the HSI measurement clock on MCO1/PA8
    // (labelled RPM_CLOCK in the schematic). SPI2 itself is set up by spi_init().
    out_pin(RPM411_CS_PORT, RPM411_CS_PIN);
    HAL_GPIO_WritePin(RPM411_CS_PORT, RPM411_CS_PIN, GPIO_PIN_SET);
    __HAL_RCC_HSI_ENABLE();
    while (!__HAL_RCC_GET_FLAG(RCC_FLAG_HSIRDY));
    HAL_RCC_MCOConfig(RCC_MCO1, RCC_MCO1SOURCE_HSI, RCC_MCODIV_1);
#endif
}

/* Select exactly one channel: enable the matching oscillator and close only that
 * channel's switch. Switch inputs are active-high; the P-MOSFET oscillator
 * enables are active-low (gate low = oscillator powered). */
static void boom_select(vaisala_boom_channel channel)
{
    for (int c = 0; c < BOOM_CHANNEL_COUNT; c++) {
        HAL_GPIO_WritePin(boom_switch[c].port, boom_switch[c].pin,
                          (c == channel) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    }
    // Oscillator enables are active-high (verified on hardware): drive the active
    // oscillator's enable HIGH and the other LOW.
    bool hum = channel_is_humidity(channel);
    HAL_GPIO_WritePin(OSC_EN_TEMP_PORT, OSC_EN_TEMP_PIN, hum ? GPIO_PIN_RESET : GPIO_PIN_SET);
    HAL_GPIO_WritePin(OSC_EN_HYG_PORT,  OSC_EN_HYG_PIN,  hum ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

/* TIM2 timebase frequency (Hz). On STM32 the timer clock is the APB1 clock,
 * doubled when the APB1 prescaler is not 1. */
static uint32_t boom_tim_clock(void)
{
    uint32_t pclk1 = HAL_RCC_GetPCLK1Freq();
    RCC_ClkInitTypeDef clk; uint32_t flash;
    HAL_RCC_GetClockConfig(&clk, &flash);
    return (clk.APB1CLKDivider == RCC_HCLK_DIV1) ? pclk1 : pclk1 * 2u;
}

/* Measure the selected channel's ring-oscillator frequency by timing a fixed
 * number of rising edges with TIM2 input capture on PA1. Returns Hz, 0 on error.
 * Ratiometric users only need the ratio of two channels, where the timer clock
 * cancels out. */
float vaisala_boom_frequency(vaisala_boom_channel channel)
{
    const int edges = 64;             // intervals to average
    const uint32_t poll_timeout = 200000u;

    boom_select(channel);
    delay_ms(2);                      // let the oscillator settle

    // PA1 -> TIM2_CH2, alternate function input.
    GPIO_InitTypeDef g = {0};
    g.Pin = OSC_OUT_PIN; g.Mode = GPIO_MODE_AF_PP; g.Pull = GPIO_NOPULL;
    g.Speed = GPIO_SPEED_FREQ_HIGH;
#if defined(RS41_RSM4x4)
    g.Alternate = GPIO_AF1_TIM2;
#endif
    HAL_GPIO_Init(OSC_OUT_PORT, &g);

    __HAL_RCC_TIM2_CLK_ENABLE();
    boom_tim.Instance = TIM2;
    boom_tim.Init.Prescaler = 0;
    boom_tim.Init.CounterMode = TIM_COUNTERMODE_UP;
    boom_tim.Init.Period = 0xFFFFFFFFu;       // 32-bit on L4; F1 caps at 0xFFFF
    boom_tim.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    if (HAL_TIM_IC_Init(&boom_tim) != HAL_OK) return 0.0f;

    TIM_IC_InitTypeDef ic = {0};
    ic.ICPolarity = TIM_ICPOLARITY_RISING;
    ic.ICSelection = TIM_ICSELECTION_DIRECTTI;
    ic.ICPrescaler = TIM_ICPSC_DIV1;
    ic.ICFilter = 0;
    if (HAL_TIM_IC_ConfigChannel(&boom_tim, &ic, TIM_CHANNEL_2) != HAL_OK) return 0.0f;
    HAL_TIM_IC_Start(&boom_tim, TIM_CHANNEL_2);

    uint32_t first = 0, last = 0;
    int got = 0;
    __disable_irq();                  // avoid ISR jitter during the short capture
    for (int i = 0; i <= edges; i++) {
        uint32_t to = poll_timeout;
        __HAL_TIM_CLEAR_FLAG(&boom_tim, TIM_FLAG_CC2);
        while (!__HAL_TIM_GET_FLAG(&boom_tim, TIM_FLAG_CC2)) {
            if (--to == 0) { __enable_irq(); HAL_TIM_IC_Stop(&boom_tim, TIM_CHANNEL_2); return 0.0f; }
        }
        uint32_t cc = HAL_TIM_ReadCapturedValue(&boom_tim, TIM_CHANNEL_2);
        if (i == 0) first = cc; else { last = cc; got++; }
    }
    __enable_irq();
    HAL_TIM_IC_Stop(&boom_tim, TIM_CHANNEL_2);

    uint32_t ticks = last - first;    // unsigned wrap-safe (32-bit on L4)
    if (got == 0 || ticks == 0) return 0.0f;
    return (float) got * (float) boom_tim_clock() / (float) ticks;
}

/* ---- Temperature -----------------------------------------------------------
 * The two reference resistors (board constants) and the PT1000 are each measured
 * as an oscillator frequency. The ring-oscillator period is proportional to the
 * resistance in the feedback (p = k*(R + Rstray)); a 2-point ratiometric fit
 * against the references cancels the oscillator gain k and the stray term, giving
 * absolute resistance with no per-sonde offset:
 *     R = R1 + (R2 - R1) * (p - p1) / (p2 - p1),   p = 1/f
 * PT1000 (IEC 60751) then converts resistance to temperature. */

#define BOOM_REF_R1_OHMS 750.0f     // SPST1 reference resistor (RS41 board value)
#define BOOM_REF_R2_OHMS 1100.0f    // SPST2 reference resistor
#define PT_R0 1000.0f
#define PT_A  3.9083e-3f
#define PT_B  -5.775e-7f

static float resistance_ratiometric(float f, float f_ref1, float f_ref2)
{
    if (f <= 0.0f || f_ref1 <= 0.0f || f_ref2 <= 0.0f) return -1.0f;
    float p = 1.0f / f, p1 = 1.0f / f_ref1, p2 = 1.0f / f_ref2;
    if (p2 == p1) return -1.0f;
    return BOOM_REF_R1_OHMS + (BOOM_REF_R2_OHMS - BOOM_REF_R1_OHMS) * (p - p1) / (p2 - p1);
}

#if SENSOR_VAISALA_BOOM_CAL_MODE != 2
/* Invert R = R0 (1 + A*T + B*T^2)  ->  T = (-A + sqrt(A^2 - 4B(1 - R/R0))) / (2B).
 * Exact for T >= 0; small (<~0.5 C) error in the sub-zero range, acceptable here. */
static float pt1000_temperature(float r)
{
    float disc = PT_A * PT_A - 4.0f * PT_B * (1.0f - r / PT_R0);
    if (disc < 0.0f) return -273.0f;
    return (-PT_A + sqrtf(disc)) / (2.0f * PT_B);
}
#endif

#if SENSOR_VAISALA_BOOM_CAL_MODE == 2
/* Factory (Vaisala) temperature from the ratiometric resistance Rc, per the
 * publicly documented RS41 PTU algorithm (rs1729 get_T):
 *   R = Rc * calT;  T = (T0 + T1*R + T2*R^2 + poly0) * (1 + poly1)
 * The Taylor terms (T0..T2) are shared; the air and heater sensors use their own
 * calT/poly. Per-sonde coefficients live in vaisala_boom_cal.h. */
static float factory_temperature(float rc, float cal, float poly0, float poly1)
{
    float R = rc * cal;
    return (VBCAL_T_T0 + VBCAL_T_T1 * R + VBCAL_T_T2 * R * R + poly0) * (1.0f + poly1);
}

/* Saturation vapour pressure (Hyland-Wexler), Tc in deg C -> Pa-ish (units cancel
 * in the ratio below). Same closed form used by the documented RS41 decoder. */
static float vapor_sat_p(float Tc)
{
    double T = (double) Tc + 273.15;
    return (float) exp(-5800.2206 / T + 1.3914993 + 6.5459673 * log(T)
                       - 4.8640239e-2 * T + 4.1764768e-5 * T * T - 1.4452093e-8 * T * T * T);
}

/* Factory (Vaisala) relative humidity per the documented RS41 PTU algorithm
 * (rs1729 get_RH2adv), pressure-correction term omitted (the P<=0 path):
 *   Cp = (cap/U0 - 1)*U1;  Trh = (Tmodule-20)/180;
 *   rh = sum_{j=0..6,k=0..5} Cp^j * Trh^k * mtxH[6j+k];
 *   rh *= vaporSatP(Tmodule)/vaporSatP(Tair).
 * cap is the measured humidity capacitance (pF); coefficients per-sonde. */
static float factory_humidity(float cap, float t_air, float t_module)
{
    if (VBCAL_H_U0 == 0.0f) return -1.0f;            // coefficients not filled in
    static const float mtx[42] = VBCAL_H_MATRIX;
    double Cp = ((double) cap / VBCAL_H_U0 - 1.0) * VBCAL_H_U1;
    double Trh = ((double) t_module - 20.0) / 180.0;

    double b[6], bk = 1.0;
    for (int k = 0; k < 6; k++) { b[k] = bk; bk *= Trh; }

    double rh = 0.0, aj = 1.0;
    for (int j = 0; j < 7; j++) {
        for (int k = 0; k < 6; k++) rh += aj * b[k] * mtx[6 * j + k];
        aj *= Cp;
    }
    if (t_air < -40.0f) rh += ((double) t_air + 40.0) / 12.0;   // low-temp term (P<=0)
    rh *= vapor_sat_p(t_module) / vapor_sat_p(t_air);
    if (rh < 0.0) rh = 0.0;
    if (rh > 100.0) rh = 100.0;
    return (float) rh;
}
#endif

/* ---- Humidity --------------------------------------------------------------
 * The humidity sensor is a capacitor whose value rises with relative humidity.
 * It is measured the same ratiometric way as the resistors, but against the two
 * reference capacitors (oscillator period p proportional to capacitance):
 *     C = Clo + (Chi - Clo) * (p - p_lo) / (p_hi - p_lo)
 * with Clo = 0 pF (baseline) and Chi the on-board reference capacitor.
 *
 * Converting capacitance to %RH accurately needs the per-sonde factory matrix
 * (planned step 3b, from the publicly documented RS41 calibration). For now a
 * simple span model is used: RH = (C - C0) / span * 100, with C0 (the 0 %RH
 * capacitance) auto-captured at the first read. This is APPROXIMATE / RELATIVE. */

#define BOOM_REF_C_HI_PF  47.0f     // SPDT1 reference capacitor (RS41 board value)
#if SENSOR_VAISALA_BOOM_CAL_MODE != 2
#define BOOM_RH_SPAN_PF   6.0f      // capacitance change 0->100 %RH (approx.)
static bool  rh_c0_captured = false;
static float rh_c0_pf = 0.0f;
#endif

static float capacitance_ratiometric(float f, float f_lo, float f_hi)
{
    if (f <= 0.0f || f_lo <= 0.0f || f_hi <= 0.0f) return -1.0f;
    float p = 1.0f / f, plo = 1.0f / f_lo, phi = 1.0f / f_hi;
    if (phi == plo) return -1.0f;
    return BOOM_REF_C_HI_PF * (p - plo) / (phi - plo);   // Clo = 0 pF
}

/* ---- RPM411 barometric pressure (RS41-SGP) ---------------------------------
 * The RPM411 board computes pressure on-board and returns it over SPI2. The
 * 33-byte readout frame carries a little-endian float32 pressure (hPa) at byte
 * offset 24 -- this layout was verified on our own hardware (clean-room
 * PROTOCOL.md). The init/trigger byte sequences are device interface facts (a
 * command sequence the board responds to, independently observable on the bus).
 * The board requires the HSI measurement clock on MCO1/PA8 (enabled in init). */
#if SENSOR_VAISALA_BOOM_PRESSURE_ENABLE

static const uint8_t rpm411_pre[5]     = { 0x01, 0x00, 0x3E, 0x2E, 0x00 };
static const uint8_t rpm411_trigger[5] = { 0x02, 0x00, 0x6D, 0x7B, 0x00 };

static uint8_t rpm411_spi(uint8_t tx)
{
    uint8_t rx = 0;
    while (__HAL_SPI_GET_FLAG(&hspi, SPI_FLAG_BSY) == SET);
    HAL_SPI_TransmitReceive(&hspi, &tx, &rx, 1, 10);
    return rx;
}

static bool rpm411_read_pressure(float *hpa)
{
    uint8_t d[33];
    bool ok = false;

    for (int i = 0; i < 5; i++) {
        HAL_GPIO_WritePin(RPM411_CS_PORT, RPM411_CS_PIN, GPIO_PIN_RESET); delay_us(100);
        rpm411_spi(rpm411_pre[i]);
        HAL_GPIO_WritePin(RPM411_CS_PORT, RPM411_CS_PIN, GPIO_PIN_SET);   delay_us(100);
    }
    delay_ms(250);                              // conversion time
    for (int i = 0; i < 5; i++) {
        HAL_GPIO_WritePin(RPM411_CS_PORT, RPM411_CS_PIN, GPIO_PIN_RESET); delay_us(100);
        d[i] = rpm411_spi(rpm411_trigger[i]);
        if (d[i] != 0xFF) ok = true;
        HAL_GPIO_WritePin(RPM411_CS_PORT, RPM411_CS_PIN, GPIO_PIN_SET);   delay_us(100);
    }
    delay_us(450);
    for (int i = 5; i < 33; i++) {
        HAL_GPIO_WritePin(RPM411_CS_PORT, RPM411_CS_PIN, GPIO_PIN_RESET); delay_us(100);
        d[i] = rpm411_spi(0x00);
        if (d[i] != 0xFF) ok = true;
        HAL_GPIO_WritePin(RPM411_CS_PORT, RPM411_CS_PIN, GPIO_PIN_SET);   delay_us(100);
    }
    if (!ok) return false;

    memcpy(hpa, &d[24], 4);                     // little-endian float32, offset 24
    return true;
}
#endif

bool vaisala_boom_read(telemetry_data *data)
{
    static bool initialised = false;
    if (!initialised) { vaisala_boom_init(); initialised = true; }

    bool any = false;

    float f_ref1 = vaisala_boom_frequency(BOOM_REF_R1);
    float f_ref2 = vaisala_boom_frequency(BOOM_REF_R2);

    // Air temperature (PT1000).
    float f_air = vaisala_boom_frequency(BOOM_T_AIR);
    float r_air = resistance_ratiometric(f_air, f_ref1, f_ref2);   // = Rc
    float air_temp_c = -300.0f;
    if (r_air > 0.0f) {
#if SENSOR_VAISALA_BOOM_CAL_MODE == 2
        float t = factory_temperature(r_air, VBCAL_T_CAL, VBCAL_T_POLY0, VBCAL_T_POLY1);
#else
        float t = pt1000_temperature(r_air);
#endif
        air_temp_c = t;
        data->temperature_celsius_100 = (int32_t) (t * 100.0f);
        log_info("Vaisala boom: T_air = %d (x100 C), R = %d mOhm\n",
                 (int) (t * 100.0f), (int) (r_air * 1000.0f));
        any = true;
    }
    (void) air_temp_c;   // used by the factory humidity (mode 2)

    // Humidity-module (heater) temperature -- same PT1000 ratiometric path.
    float f_heat = vaisala_boom_frequency(BOOM_T_HEATER);
    float r_heat = resistance_ratiometric(f_heat, f_ref1, f_ref2);
#if SENSOR_VAISALA_BOOM_CAL_MODE == 2
    float t_module = (r_heat > 0.0f)
        ? factory_temperature(r_heat, VBCAL_TU_CAL, VBCAL_TU_POLY0, VBCAL_TU_POLY1) : 0.0f;
#else
    float t_module = (r_heat > 0.0f) ? pt1000_temperature(r_heat) : 0.0f;
#endif
    (void) t_module;   // feeds the RH temperature correction in the humidity factory mode

    // Relative humidity (approximate -- see note above).
    float f_hum = vaisala_boom_frequency(BOOM_HUMIDITY);
    float f_clo = vaisala_boom_frequency(BOOM_REF_C_LO);
    float f_chi = vaisala_boom_frequency(BOOM_REF_C_HI);
    float c_hum = capacitance_ratiometric(f_hum, f_clo, f_chi);
    if (c_hum > 0.0f) {
#if SENSOR_VAISALA_BOOM_CAL_MODE == 2
        float rh = factory_humidity(c_hum, air_temp_c, t_module);
#else
        if (!rh_c0_captured) { rh_c0_pf = c_hum; rh_c0_captured = true; }
        float rh = (c_hum - rh_c0_pf) / BOOM_RH_SPAN_PF * 100.0f;
        if (rh < 0.0f) rh = 0.0f;
        if (rh > 100.0f) rh = 100.0f;
#endif
        if (rh >= 0.0f) {
            data->humidity_percentage_100 = (uint32_t) (rh * 100.0f);
            log_info("Vaisala boom: RH = %d (x100 %%), C = %d (x100 pF), Tmod = %d (x100 C)\n",
                     (int) (rh * 100.0f), (int) (c_hum * 100.0f), (int) (t_module * 100.0f));
            any = true;
        }
    }

#if SENSOR_VAISALA_BOOM_PRESSURE_ENABLE
    {
        float p_hpa = 0.0f;
        if (rpm411_read_pressure(&p_hpa) && p_hpa > 300.0f && p_hpa < 1200.0f) {
            data->pressure_mbar_100 = (uint32_t) (p_hpa * 100.0f);
            log_info("Vaisala boom: pressure = %d (x100 hPa)\n", (int) data->pressure_mbar_100);
            any = true;
        }
    }
#endif

    // Leave both oscillators disabled when idle (active-high enables low).
    HAL_GPIO_WritePin(OSC_EN_TEMP_PORT, OSC_EN_TEMP_PIN, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(OSC_EN_HYG_PORT,  OSC_EN_HYG_PIN,  GPIO_PIN_RESET);
    return any;
}

#endif // SENSOR_VAISALA_BOOM_ENABLE
