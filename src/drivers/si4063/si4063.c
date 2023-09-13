#include <stdbool.h>
#include <stm32f10x_gpio.h>

#include "hal/hal.h"
#include "hal/delay.h"
#include "hal/spi.h"
#include "si4063.h"
#include "gpio.h"
#include "log.h"

#define SI4063_CLOCK 25600000UL

#define GPIO_SI4063_SDN GPIOC
#define GPIO_PIN_SI4063_SDN GPIO_Pin_3

#define GPIO_SI4063_NSEL GPIOB
#define GPIO_PIN_SI4063_NSEL GPIO_Pin_2

#define GPIO_SI4063_SDI GPIOA
#define GPIO_PIN_SI4063_SDI GPIO_Pin_7

#define GPIO_SI4063_GPIO2 GPIOD
#define GPIO_PIN_SI4063_GPIO2 GPIO_Pin_0

#define GPIO_SI4063_GPIO3 GPIOA
#define GPIO_PIN_SI4063_GPIO3 GPIO_Pin_4

#define SI4063_COMMAND_PART_INFO    0x01
#define SI4063_COMMAND_POWER_UP     0x02
#define SI4063_COMMAND_SET_PROPERTY 0x11
#define SI4063_COMMAND_START_TX     0x31
#define SI4063_COMMAND_CHANGE_STATE 0x34
#define SI4063_COMMAND_GET_ADC_READING  0x14

#define SI4063_STATE_SLEEP        0x01
#define SI4063_STATE_SPI_ACTIVE   0x02
#define SI4063_STATE_READY        0x03
#define SI4063_STATE_TX_TUNE      0x05
#define SI4063_STATE_TX           0x07

static inline void si4063_set_chip_select(bool select)
{
    spi_set_chip_select(GPIO_SI4063_NSEL, GPIO_PIN_SI4063_NSEL, select);
}

static int si4063_wait_for_cts()
{
    uint16_t timeout = 0xFFFF;
    uint8_t response;

    // Poll CTS over SPI
    do
    {
        si4063_set_chip_select(true);
        spi_send(0x44);
        response = spi_read();
        si4063_set_chip_select(false);
    } while (response != 0xFF && timeout--);

    return timeout > 0 ? HAL_OK : HAL_ERROR;
}

static int si4063_read_response(uint8_t length, uint8_t *data)
{
    uint16_t timeout = 0xFFFF;
    uint8_t response;

    // Poll CTS over SPI
    do {
        si4063_set_chip_select(true);
        spi_send(0x44);
        response = spi_read();
        if (response == 0xFF) {
            break;
        }
        si4063_set_chip_select(false);

        delay_us(10);
    } while(timeout--);

    if (timeout == 0) {
        si4063_set_chip_select(false);
        return HAL_ERROR;
    }

    // Read the requested data
    while (length--) {
        *(data++) = spi_read();
    }

    si4063_set_chip_select(false);

    return HAL_OK;
}

static void si4063_send_command(uint8_t command, uint8_t length, uint8_t *data)
{
    si4063_wait_for_cts();

    si4063_set_chip_select(true);

    // Output enable time, 20ns
    for (uint32_t i = 0; i < 0xFFFFF; i++);

    spi_send(command);

    while (length--) {
        spi_send(*(data++));
    }

    si4063_set_chip_select(false);
}

static void si4063_power_up()
{
    uint8_t data[] = {
            0x01, // 0x01 = FUNC PRO - Power the chip up into EZRadio PRO functional mode.
            0x01, // 0x01 = Reference signal is derived from an external TCXO.
            (SI4063_CLOCK >> 24) & 0xFF, // VCXO frequency
            (SI4063_CLOCK >> 16) & 0xFF,
            (SI4063_CLOCK >> 8) & 0xFF,
            SI4063_CLOCK & 0xFF
    };

    si4063_send_command(SI4063_COMMAND_POWER_UP, sizeof(data), data);
}

static void si4603_set_shutdown(bool active)
{
    if (active) {
        GPIO_SetBits(GPIO_SI4063_SDN, GPIO_PIN_SI4063_SDN);
    } else {
        GPIO_ResetBits(GPIO_SI4063_SDN, GPIO_PIN_SI4063_SDN);
    }
}

static void si4063_set_state(uint8_t state)
{
    si4063_send_command(SI4063_COMMAND_CHANGE_STATE, 1, &state);
}

void si4063_enable_tx()
{
    si4063_set_state(SI4063_STATE_TX);
}

void si4063_inhibit_tx()
{
    si4063_set_state(SI4063_STATE_READY);
}

void si4063_disable_tx()
{
    // Is this needed?
    si4063_set_state(SI4063_STATE_SLEEP);
}

void si4063_use_direct_mode(bool use)
{
    if (use) {
        GPIO_SetBits(GPIO_SI4063_NSEL, GPIO_PIN_SI4063_NSEL);
    } else {
        GPIO_ResetBits(GPIO_SI4063_NSEL, GPIO_PIN_SI4063_NSEL);
    }
}

void si4063_set_tx_frequency(const uint32_t frequency_hz)
{
    uint8_t outdiv, band;
    uint32_t f_pfd, n, m;
    float ratio, rest;

    /* Set the output divider according to the recommended ranges in the si406x datasheet */
    if (frequency_hz < 177000000UL)      {
        outdiv = 24;
        band = 5;
    } else if (frequency_hz < 239000000UL) {
        outdiv = 16;
        band = 4;
    } else if (frequency_hz < 353000000UL) {
        outdiv = 12;
        band = 3;
    } else if (frequency_hz < 525000000UL) {
        outdiv = 8;
        band = 2;
    } else if (frequency_hz < 705000000UL) {
        outdiv = 6;
        band = 1;
    } else {
        outdiv = 4;
        band = 0;
    }

    f_pfd = 2 * SI4063_CLOCK / outdiv;
    n = frequency_hz / f_pfd - 1;

    ratio = (float) frequency_hz / f_pfd;
    rest  = ratio - n;

    m = rest * 524288UL;

    // Set the frequency band
    {
        uint8_t data[] = {
                0x20, // 0x20 = Group MODEM
                0x01, // Set 1 property
                0x51, // 0x51 = MODEM_CLKGEN_BAND
                0x08 + band // 0x08 = SY_SEL: High Performance mode (fixed prescaler = Div-by-2). Finer tuning.
        };

        si4063_send_command(SI4063_COMMAND_SET_PROPERTY, sizeof(data), data);
    }

    // Set the PLL parameters
    {
        uint8_t data[] = {
                0x40, // 0x40 = Group FREQ_CONTROL
                0x06, // Set 6 properties
                0x00, // 0x00 = Start from FREQ_CONTROL_INTE
                n, // 0 (FREQ_CONTROL_INTE): Frac-N PLL Synthesizer integer divide number.
                (m >> 16) & 0xFF, // 1 (FREQ_CONTROL_FRAC): Frac-N PLL fraction number.
                (m >> 8) & 0xFF, // 2 (FREQ_CONTROL_FRAC): Frac-N PLL fraction number.
                m & 0xFF, // 3 (FREQ_CONTROL_FRAC): Frac-N PLL fraction number.
                0x00, // 4 (FREQ_CONTROL_CHANNEL_STEP_SIZE): EZ Frequency Programming channel step size.
                0x02 // 5 (FREQ_CONTROL_CHANNEL_STEP_SIZE): EZ Frequency Programming channel step size.
        };

        si4063_send_command(SI4063_COMMAND_SET_PROPERTY, sizeof(data), data);
    }
}

void si4063_set_tx_power(uint8_t power)
{
    uint8_t data[] = {
            0x22, // 0x20 = Group PA
            0x01, // Set 1 property
            0x01, // 0x01 = PA_PWR_LVL
            power & 0x7F // Power level from 00..7F
    };

    si4063_send_command(SI4063_COMMAND_SET_PROPERTY, sizeof(data), data);
}

void si4063_set_frequency_offset(uint16_t offset)
{
    uint8_t data[] = {
            0x20, // 0x20 = Group MODEM
            0x02, // Set 2 properties (2 bytes)
            0x0D, // 0x0D = MODEM_FREQ_OFFSET
            offset >> 8, // Upper 8 bits of the offset
            offset & 0xFF // Lower 8 bits of the offset
    };

    si4063_send_command(SI4063_COMMAND_SET_PROPERTY, sizeof(data), data);
}

void si4063_set_frequency_deviation(uint32_t deviation)
{
    uint8_t data[] = {
            0x20, // 0x20 = Group MODEM
            0x03, // Set 3 properties (3 bytes)
            0x0A, // 0x0A = MODEM_FREQ_DEV
            (deviation >> 16) & 0xFF,
            (deviation >> 8) & 0xFF,
            deviation & 0xFF
    };

    si4063_send_command(SI4063_COMMAND_SET_PROPERTY, sizeof(data), data);
}

void si4063_set_modulation_type(si4063_modulation_type type)
{
    uint8_t data[] = {
            0x20, // 0x20 = Group MODEM
            0x01, // Set 1 property
            0x00, // 0x00 = MODEM_MOD_TYPE
            0x80 | // 0x80 = Direct async mode (MCU-controlled)
            0x40 | // 0x40 = Use GPIO2 as source for FSK modulation (alternatively, use 0x60 for GPIO3)
            0x08 // 0x08 = Direct modulation source (MCU-controlled)
    };

    switch (type) {
        case SI4063_MODULATION_TYPE_CW:
            // Pure carrier wave modulation (for modulating via frequency offset, e.g. for RTTY)
            data[3] |= 0x00;
            break;
        case SI4063_MODULATION_TYPE_OOK:
            // Direct Async Mode with OOK modulation
            data[3] |= 0x01;
            break;
        case SI4063_MODULATION_TYPE_FSK:
            // Direct Async Mode with FSK modulation
            data[3] |= 0x02;
            break;
        default:
            return;
    }

    si4063_send_command(SI4063_COMMAND_SET_PROPERTY, sizeof(data), data);
}

int32_t si4063_read_temperature_celsius_100()
{
    uint8_t response[6];
    int32_t temperature;
    uint8_t data[] = {
            0x10,
            0x00
    };

    si4063_send_command(SI4063_COMMAND_GET_ADC_READING, sizeof(data), data);

    si4063_read_response(sizeof(response), response);

    for (int i = 0; i < sizeof(response); i++) {
        log_info("Si4063 ADC reading: %02x\n", response[i]);
    }

    // Calculate the temperature in C * 10
    temperature  = (response[4] << 8) | response[5];
    temperature *= 568;
    temperature /= 256;
    temperature -= 2970;

    log_info("temp: %d\n", (int) (temperature / 10));

    return temperature * 10;
}

uint8_t si4063_read_part_info()
{
    uint8_t response[8];

    si4063_send_command(SI4063_COMMAND_PART_INFO, 0, NULL);

    si4063_read_response(sizeof(response), response);

    for (int i = 0; i < sizeof(response); i++) {
        log_info("Si4063 part info: %02x\n", response[i]);
    }

    return response[0];
}

static void si4063_set_nsel_pin(bool high)
{
    if (high) {
        GPIO_SetBits(GPIO_SI4063_NSEL, GPIO_PIN_SI4063_NSEL);
    } else {
        GPIO_ResetBits(GPIO_SI4063_NSEL, GPIO_PIN_SI4063_NSEL);
    }
}

void si4063_set_sdi_pin(bool high)
{
    if (high) {
        GPIO_SetBits(GPIO_SI4063_SDI, GPIO_PIN_SI4063_SDI);
    } else {
        GPIO_ResetBits(GPIO_SI4063_SDI, GPIO_PIN_SI4063_SDI);
    }
}

void si4063_use_sdi_pin(bool use)
{
    GPIO_InitTypeDef gpio_init;

    si4063_set_nsel_pin(true);

    gpio_init.GPIO_Pin = GPIO_PIN_SI4063_SDI;
    gpio_init.GPIO_Mode = use ? GPIO_Mode_Out_PP : GPIO_Mode_AF_PP;
    gpio_init.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIO_SI4063_SDI, &gpio_init);

    si4063_set_sdi_pin(false);
}

void si4063_init()
{
    GPIO_InitTypeDef gpio_init;

    // Si4063 shutdown pin
    gpio_init.GPIO_Pin = GPIO_PIN_SI4063_SDN;
    gpio_init.GPIO_Mode = GPIO_Mode_Out_PP;
    gpio_init.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIO_SI4063_SDN, &gpio_init);

    // Si4063 chip select pin
    gpio_init.GPIO_Pin = GPIO_PIN_SI4063_NSEL;
    gpio_init.GPIO_Mode = GPIO_Mode_Out_PP;
    gpio_init.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIO_SI4063_NSEL, &gpio_init);

    si4603_set_shutdown(true);
    delay_us(20);
    si4603_set_shutdown(false);
    delay_us(50);

    si4063_power_up();

    si4063_read_part_info();

    si4063_set_tx_power(0x00);

    si4063_set_frequency_offset(0);
    // NOTE: 0x22 sets shift at about 450 Hz
    si4063_set_frequency_deviation(0x22);

    si4063_set_modulation_type(SI4063_MODULATION_TYPE_CW);

    si4063_set_state(SI4063_STATE_READY);
}
