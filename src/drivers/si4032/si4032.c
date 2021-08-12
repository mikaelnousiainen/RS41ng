#include <stm32f10x_gpio.h>

#include "hal/spi.h"
#include "si4032.h"

#define SPI_WRITE_FLAG 0x80

#define SI4032_CLOCK 26.0f

#define GPIO_SI4032_NSEL GPIOC
#define GPIO_PIN_SI4032_NSEL GPIO_Pin_13

#define GPIO_SI4032_SDI GPIOB
#define GPIO_PIN_SI4032_SDI GPIO_Pin_15

static inline uint8_t si4032_write(uint8_t reg, uint8_t value)
{
    return spi_send_and_receive(GPIO_SI4032_NSEL, GPIO_PIN_SI4032_NSEL, ((reg | SPI_WRITE_FLAG) << 8U) | value);
}

static inline uint8_t si4032_read(uint8_t reg)
{
    return spi_send_and_receive(GPIO_SI4032_NSEL, GPIO_PIN_SI4032_NSEL, (reg << 8U) | 0xFFU);
}

void si4032_soft_reset()
{
    si4032_write(0x07, 0x80);
}

void si4032_enable_tx()
{
    // Modified to set the PLL and Crystal enable bits to high. Not sure if this makes much difference.
    si4032_write(0x07, 0x4B);
}

void si4032_inhibit_tx()
{
    // Sleep mode, but with PLL idle mode enabled, in an attempt to reduce drift on key-up.
    si4032_write(0x07, 0x43);
}

void si4032_disable_tx()
{
    si4032_write(0x07, 0x40);
}

void si4032_use_direct_mode(bool use)
{
    if (use) {
        GPIO_SetBits(GPIO_SI4032_NSEL, GPIO_PIN_SI4032_NSEL);
    } else {
        GPIO_ResetBits(GPIO_SI4032_NSEL, GPIO_PIN_SI4032_NSEL);
    }
}

void si4032_set_tx_frequency(const float frequency_mhz)
{
    uint8_t hbsel = (uint8_t) ((frequency_mhz * (30.0f / SI4032_CLOCK)) >= 480.0f ? 1 : 0);

    uint8_t fb = (uint8_t) ((((uint8_t) ((frequency_mhz * (30.0f / SI4032_CLOCK)) / 10) - 24) - (24 * hbsel)) /
                            (1 + hbsel));
    uint8_t gen_div = 3;  // constant - not possible to change!
    uint16_t fc = (uint16_t) (((frequency_mhz / ((SI4032_CLOCK / gen_div) * (hbsel + 1))) - fb - 24) * 64000);
    si4032_write(0x75, (uint8_t) (0b01000000 | (fb & 0b11111) | ((hbsel & 0b1) << 5)));
    si4032_write(0x76, (uint8_t) (((uint16_t) fc >> 8U) & 0xffU));
    si4032_write(0x77, (uint8_t) ((uint16_t) fc & 0xff));
}

void si4032_set_tx_power(uint8_t power)
{
    si4032_write(0x6D, power & 0x7U);
}

/**
 * The frequency offset can be calculated as Offset = 156.25 Hz x (hbsel + 1) x fo[7:0]. fo[9:0] is a twos complement value. fo[9] is the sign bit.
 * For 70cm band hbsel is 1, so offset step is 312.5 Hz
 */
void si4032_set_frequency_offset(uint16_t offset)
{
    si4032_write(0x73, offset);
    si4032_write(0x74, 0);
}

inline void si4032_set_frequency_offset_small(uint8_t offset)
{
    si4032_write(0x73, offset);
}

void si4032_set_frequency_deviation(uint8_t deviation)
{
    // The frequency deviation can be calculated: Fd = 625 Hz x fd[8:0].
    // Zero disables deviation between 0/1 bits
    si4032_write(0x72, deviation);
}

void si4032_set_modulation_type(si4032_modulation_type type)
{
    uint8_t value;
    switch (type) {
        case SI4032_MODULATION_TYPE_NONE:
            // No modulation (for modulating via frequency offset, e.g. for RTTY)
            value = 0x00;
            break;
        case SI4032_MODULATION_TYPE_OOK:
            // Direct Async Mode with OOK modulation
            value = 0b00010001;
            break;
        case SI4032_MODULATION_TYPE_FSK:
            // Direct Async Mode with FSK modulation
            value = 0b00010010;
            break;
        default:
            return;
    }

    si4032_write(0x71, value);
}

int32_t si4032_read_temperature_celsius_100()
{
    // Set the input for ADC to the temperature sensor, "Register 0Fh. ADC Configuration"—adcsel[2:0] = 000
    // Set the reference for ADC, "Register 0Fh. ADC Configuration"—adcref[1:0] = 00
    si4032_write(0x0f, 0b00000000);
    // Set the temperature range for ADC, "Register 12h. Temperature Sensor Calibration"—tsrange[1:0]
    // Range: –64 ... 64 °C, Slope 8 mV/°C, ADC8 LSB 0.5 °C
    si4032_write(0x12, 0b00100000);
    // Set entsoffs = 1, "Register 12h. Temperature Sensor Calibration"
    // Trigger ADC reading, "Register 0Fh. ADC Configuration"—adcstart = 1
    si4032_write(0x0f, 0b10000000);
    // Read temperature value—Read contents of "Register 11h. ADC Value"
    int32_t raw_value = (int32_t) si4032_read(0x11);

    int32_t temperature = (int32_t) (-6400 + (raw_value * 100 * 500 / 1000));

    return temperature;
}

static void si4032_set_nsel_pin(bool high)
{
    if (high) {
        GPIO_SetBits(GPIO_SI4032_NSEL, GPIO_PIN_SI4032_NSEL);
    } else {
        GPIO_ResetBits(GPIO_SI4032_NSEL, GPIO_PIN_SI4032_NSEL);
    }
}

void si4032_set_sdi_pin(bool high)
{
    if (high) {
        GPIO_SetBits(GPIO_SI4032_SDI, GPIO_PIN_SI4032_SDI);
    } else {
        GPIO_ResetBits(GPIO_SI4032_SDI, GPIO_PIN_SI4032_SDI);
    }
}

void si4032_use_sdi_pin(bool use)
{
    GPIO_InitTypeDef gpio_init;

    si4032_set_nsel_pin(true);

    gpio_init.GPIO_Pin = GPIO_PIN_SI4032_SDI;
    gpio_init.GPIO_Mode = use ? GPIO_Mode_Out_PP : GPIO_Mode_AF_PP;
    gpio_init.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIO_SI4032_SDI, &gpio_init);

    si4032_set_sdi_pin(false);
}

void si4032_init()
{
    GPIO_InitTypeDef gpio_init;

    // Si4032 chip select pin
    gpio_init.GPIO_Pin = GPIO_PIN_SI4032_NSEL;
    gpio_init.GPIO_Mode = GPIO_Mode_Out_PP;
    gpio_init.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIO_SI4032_NSEL, &gpio_init);

    si4032_set_nsel_pin(true);

    si4032_soft_reset();
    si4032_set_tx_power(0);

    // Temperature Value Offset
    si4032_write(0x13, 0xF0);
    // Temperature Sensor Calibration
    si4032_write(0x12, 0x00);

    // ADC configuration
    si4032_write(0x0f, 0x80);

    si4032_set_frequency_offset(0);
    si4032_set_frequency_deviation(5); // Was: 5 for APRS in RS41HUP?

    si4032_set_modulation_type(SI4032_MODULATION_TYPE_NONE);
}
