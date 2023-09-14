/**
 * The DFM17 radiosonde-compatible Si4063 driver code has been inspired by:
 * - pAVAR9: https://github.com/Upuaut/pAVAR9/
 * - uTrak: https://github.com/thasti/utrak/
 * - DFM17 APRS/RTTY firmware by Derek Rowland
 */

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
#define SI4063_COMMAND_GPIO_PIN_CFG 0x13
#define SI4063_COMMAND_START_TX     0x31
#define SI4063_COMMAND_CHANGE_STATE 0x34
#define SI4063_COMMAND_GET_ADC_READING  0x14
#define SI4063_COMMAND_READ_CMD_BUFF    0x44

#define SI4063_STATE_SLEEP        0x01
#define SI4063_STATE_SPI_ACTIVE   0x02
#define SI4063_STATE_READY        0x03
#define SI4063_STATE_TX_TUNE      0x05
#define SI4063_STATE_TX           0x07

/**
 * Modulation settings from Derek Rowland's firmware
 */

#define SI4063_DATA_RATE_APRS 4400

#define RF_DEVIATION_HZ_RTTY 200.0f
#define RF_DEVIATION_HZ_APRS 1300.0f

#define OUTDIV_70CM 8
#define OUTDIV_2M   24
#define OUTDIV_DFM  10

#define FREQUENCY_DEVIATION_RTTY ((((uint32_t)1 << 19) * OUTDIV_2M * RF_DEVIATION_HZ_RTTY)/(2*SI4063_CLOCK))
#define FREQUENCY_DEVIATION_APRS ((((uint32_t)1 << 19) * OUTDIV_2M * RF_DEVIATION_HZ_APRS)/(2*SI4063_CLOCK))
#define FREQUENCY_DEVIATION_APRS_DFM ((((uint32_t)1 << 19) * OUTDIV_DFM * RF_DEVIATION_HZ_APRS)/(2*SI4063_CLOCK))
#define FREQUENCY_DEVIATION_DFM 0x90000

/**
 * Filters from uTrak: https://github.com/thasti/utrak
 *
 * These filters consist of low-pass filters for harmonics of the square wave modulation waveform
 * and a high-pass filter for APRS pre-emphasis.
 */

/**
 * 6dB@1200 Hz, 2400 Hz
 */
uint8_t si4063_filter_6db_1200_2400[9] = {0x1d, 0xe5, 0xb8, 0xaa, 0xc0, 0xf5, 0x36, 0x6b, 0x7f};
/**
 * 3db@1200 Hz, 2400 Hz
 */
uint8_t si4063_filter_3db_1200_2400[9] = {0x07, 0xde, 0xbf, 0xb9, 0xd4, 0x05, 0x40, 0x6d, 0x7f};
/**
 * LP only, 2400 Hz
 */
uint8_t si4063_filter_lp_2400[9] = {0xfa, 0xe5, 0xd8, 0xde, 0xf8, 0x21, 0x4f, 0x71, 0x7f};
/**
 * LP only, 4800 Hz
 */
uint8_t si4063_filter_lp_4800[9] = {0xd9, 0xf1, 0x0c, 0x29, 0x44, 0x5d, 0x70, 0x7c, 0x7f};
/**
 * LP only, 4400 Hz
 */
uint8_t si4063_filter_lp_4400[9] = {0xd5, 0xe9, 0x03, 0x20, 0x3d, 0x58, 0x6d, 0x7a, 0x7f};
/**
 * 6dB@1200Hz, 4400 Hz (bad stopband)
 */
uint8_t si4063_filter_6db_1200_4400[9] = {0x81, 0x9f, 0xc4, 0xee, 0x18, 0x3e, 0x5c, 0x70, 0x76};

static inline void si4063_set_chip_select(bool select)
{
    spi_set_chip_select(GPIO_SI4063_NSEL, GPIO_PIN_SI4063_NSEL, select);

    // Output enable time, 20ns
    for (uint32_t i = 0; i < 0xFFFF; i++);
}

static int si4063_wait_for_cts()
{
    uint16_t timeout = 0xFFFF;
    uint8_t response;

    // Poll CTS over SPI
    do
    {
        si4063_set_chip_select(true);
        spi_send(SI4063_COMMAND_READ_CMD_BUFF);
        response = spi_read();
        si4063_set_chip_select(false);
    } while (response != 0xFF && timeout--);

    if (timeout == 0) {
        log_error("ERROR: Si4063 timeout\n");
    }

    return timeout > 0 ? HAL_OK : HAL_ERROR;
}

static int si4063_read_response(uint8_t length, uint8_t *data)
{
    uint16_t timeout = 0xFFFF;
    uint8_t response;

    // Poll CTS over SPI
    do {
        si4063_set_chip_select(true);
        spi_send(SI4063_COMMAND_READ_CMD_BUFF);
        response = spi_read();
        if (response == 0xFF) {
            break;
        }
        si4063_set_chip_select(false);

        delay_us(10);
    } while(timeout--);

    if (timeout == 0) {
        log_error("ERROR: Si4063 timeout\n");
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

    spi_send(command);

    while (length--) {
        spi_send(*(data++));
    }

    si4063_set_chip_select(false);
}

static int si4063_power_up()
{
    si4063_wait_for_cts();

    uint8_t data[] = {
            0x01, // 0x01 = FUNC PRO - Power the chip up into EZRadio PRO functional mode.
            0x01, // 0x01 = Reference signal is derived from an external TCXO.
            (SI4063_CLOCK >> 24) & 0xFF, // VCXO frequency
            (SI4063_CLOCK >> 16) & 0xFF,
            (SI4063_CLOCK >> 8) & 0xFF,
            SI4063_CLOCK & 0xFF
    };

    si4063_send_command(SI4063_COMMAND_POWER_UP, sizeof(data), data);

    return si4063_wait_for_cts();
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
    log_debug("Si4063: Set state %02x\n", state);
    si4063_send_command(SI4063_COMMAND_CHANGE_STATE, 1, &state);
}

void si4063_enable_tx()
{
    log_debug("Si4063: Enable TX\n");
    si4063_set_state(SI4063_STATE_TX);
}

void si4063_inhibit_tx()
{
    log_debug("Si4063: Inhibit TX\n");
    si4063_set_state(SI4063_STATE_READY);
}

void si4063_disable_tx()
{
    // Is this needed?
    si4063_set_state(SI4063_STATE_SLEEP);
}

void si4063_set_tx_frequency(const uint32_t frequency_hz)
{
    uint8_t outdiv, band;
    uint32_t f_pfd, n, m;
    float ratio, rest;

    log_debug("Si4063: Set frequency %lu\n", frequency_hz);

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

    log_debug("Si4063: Set TX power %02x\n", power);

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

    log_debug("Si4063: Set freq deviation %lu\n", deviation);

    si4063_send_command(SI4063_COMMAND_SET_PROPERTY, sizeof(data), data);
}

void si4063_set_modulation_type(si4063_modulation_type type)
{
    uint8_t data[] = {
            0x20, // 0x20 = Group MODEM
            0x01, // Set 1 property
            0x00, // 0x00 = MODEM_MOD_TYPE
            0x80 | // 0x80 = Direct async mode (MCU-controlled)
            0x60 | // 0x60 = Use GPIO3 as source for direct mode modulation
            0x08 // 0x08 = Direct modulation source (MCU-controlled)
    };

    log_debug("Si4063: Set modulation type %d\n", type);

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
            0x10, // Measure internal temperature ADC reading only
            0x00
    };

    si4063_send_command(SI4063_COMMAND_GET_ADC_READING, sizeof(data), data);

    si4063_read_response(sizeof(response), response);

    // Calculate the temperature in C * 10
    temperature  = (response[4] << 8) | response[5];
    temperature *= 568;
    temperature /= 256;
    temperature -= 2970;

    return temperature * 10;
}

uint16_t si4063_read_part_info()
{
    uint8_t response[8];

    si4063_send_command(SI4063_COMMAND_PART_INFO, 0, NULL);

    si4063_read_response(sizeof(response), response);

    // Return part number
    return response[1] << 8 | response[2];
}

void si4063_set_direct_mode_pin(bool high)
{
    if (high) {
        GPIO_SetBits(GPIO_SI4063_GPIO3, GPIO_PIN_SI4063_GPIO3);
    } else {
        GPIO_ResetBits(GPIO_SI4063_GPIO3, GPIO_PIN_SI4063_GPIO3);
    }
}

void si4063_configure()
{
    {
        uint8_t data[] = {
                0x00, // 0x00 = Group GLOBAL
                0x01, // Set 1 property
                0x00, // 0x00 = GLOBAL_XO_TUNE
                0x62  // Value determined for DFM17 radiosondes
        };

        si4063_send_command(SI4063_COMMAND_SET_PROPERTY, sizeof(data), data);
    }

    {
        uint8_t data[] = {
                0x00, // 0x00 = Group GLOBAL
                0x01, // Set 1 property
                0x01, // 0x00 = GLOBAL_CLK_CFG
                0x00  // No clock output needed
        };

        si4063_send_command(SI4063_COMMAND_SET_PROPERTY, sizeof(data), data);
    }

    {
        uint8_t data[] = {
                0x00, // 0x00 = Group GLOBAL
                0x01, // Set 1 property
                0x03, // 0x03 = GLOBAL_CONFIG
                0x40 | // 0x40 = Reserved, needs to be set to 1
                0x20 | // 0x20 = Fast sequencer mode
                0x00   // High-performance mode
        };

        si4063_send_command(SI4063_COMMAND_SET_PROPERTY, sizeof(data), data);
    }

    {
        uint8_t data[] = {
                0x01, // 0x01 = Group INT_CTL
                0x01, // Set 1 property
                0x00, // 0x00 = INT_CTL_ENABLE
                0x00 // 0x00 = Disable all hardware interrupts
        };

        si4063_send_command(SI4063_COMMAND_SET_PROPERTY, sizeof(data), data);
    }

    {
        uint8_t data[] = {
                0x02, // 0x02 = Group FRR_CTL
                0x04, // Set 4 properties
                0x00, // 0x00 = FRR_CTL_A_MODE
                0x00, // Disable all FRR values
                0x00,
                0x00,
                0x00
        };

        si4063_send_command(SI4063_COMMAND_SET_PROPERTY, sizeof(data), data);
    }
// HERE
    {
        // Used only in synchronous mode (for GFSK modulation/filtering)
        uint8_t data[] = {
                0x10, // 0x10 = Group PREAMBLE
                0x01, // Set 1 property
                0x00, // 0x00 = PREAMBLE_TX_LENGTH
                0x00 // 0x00 = Disable preamble
        };

        si4063_send_command(SI4063_COMMAND_SET_PROPERTY, sizeof(data), data);
    }

    {
        // Used only in synchronous mode (for GFSK modulation/filtering)
        uint8_t data[] = {
                0x11, // 0x11 = Group SYNC
                0x01, // Set 1 property
                0x00, // 0x00 = SYNC_CONFIG
                0x80 // 0x80 = Sync word is not transmitted
        };

        si4063_send_command(SI4063_COMMAND_SET_PROPERTY, sizeof(data), data);
    }

    {
        uint8_t data[] = {
                0x22, // 0x22 = Group PA
                0x01, // Set 1 property
                0x02, // 0x02 = PA_BIAS_CLKDUTY
                0x00 // 0x00 = Complementary drive signals, 50% duty cycle. For high-power applications.
                // Alternative: 0xC0 = Single-ended drive signal, 25% duty cycle. For low-power applications.
        };

        si4063_send_command(SI4063_COMMAND_SET_PROPERTY, sizeof(data), data);
    }
}

void si4063_set_filter(const uint8_t *filter)
{
    uint8_t data[12] = {
            0x20, // 0x20 = Group MODEM
            0x09, // Set 9 properties
            0x0F // 0x0F = MODEM_TX_FILTER_COEFF_8
    };

    for (uint8_t i = 0; i < 9; i++) {
        data[3 + i] = filter[i];
    }

    si4063_send_command(SI4063_COMMAND_SET_PROPERTY, sizeof(data), data);
}

void si4063_configure_data_rate(uint32_t data_rate)
{
    // Used only for GFSK mode filtering
    uint8_t data[] = {
            0x20, // 0x20 = Group MODEM
            0x03, // Set 3 properties
            0x03, // 0x03 = MODEM_DATA_RATE
            (data_rate >> 16) & 0xFF,
            (data_rate >> 8) & 0xFF,
            data_rate & 0xFF
    };

    si4063_send_command(SI4063_COMMAND_SET_PROPERTY, sizeof(data), data);
}

void si4063_configure_aprs()
{
    // For APRS in direct mode with GFSK modulation
    si4063_configure_data_rate(4400);
    si4063_set_frequency_deviation((uint16_t)(2*FREQUENCY_DEVIATION_APRS));

    // Used only for GFSK mode filtering
    uint32_t nco_mod = SI4063_CLOCK / 10;
    uint8_t data[] = {
            0x20, // 0x20 = Group MODEM
            0x04, // Set 4 properties
            0x06, // 0x06 = MODEM_TX_NCO_MODE
            0x00 | // 0x00 = TX Gaussian filter oversampling ratio is 10x
            ((nco_mod >> 24) & 0x03),
            (nco_mod >> 16) & 0xFF,
            (nco_mod >> 8) & 0xFF,
            nco_mod & 0xFF
    };

    si4063_send_command(SI4063_COMMAND_SET_PROPERTY, sizeof(data), data);
}

void si4063_configure_rtty()
{
    // For RTTY:
    // NOTE: 0x22 sets shift at about 450 Hz for RTTY
    si4063_set_frequency_deviation(0x22);
}

void si4063_configure_gpio(uint8_t gpio0, uint8_t gpio1, uint8_t gpio2, uint8_t gpio3, uint8_t drive_strength) {
    uint8_t data[] = {
            gpio0,
            gpio1,
            gpio2,
            gpio3,
            0x00, // NIRQ = Do nothing
            11, // SDO 11 = Outputs the Serial Data Out (SDO) signal for the SPI bus
            drive_strength
    };

    si4063_send_command(SI4063_COMMAND_GPIO_PIN_CFG, sizeof(data), data);
}

int si4063_init()
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

    // Si4063 GPIO3 pin for direct mode transmission
    gpio_init.GPIO_Pin = GPIO_PIN_SI4063_GPIO3;
    gpio_init.GPIO_Mode = GPIO_Mode_Out_PP;
    gpio_init.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIO_SI4063_GPIO3, &gpio_init);

    si4063_set_direct_mode_pin(false);

    si4603_set_shutdown(false);
    delay_us(50);

    si4063_wait_for_cts();

    si4603_set_shutdown(true);
    delay_us(20);
    si4603_set_shutdown(false);
    delay_us(50);

    if (si4063_power_up() != HAL_OK) {
        log_error("ERROR: Error powering up Si4063\n");
        return HAL_ERROR;
    }

    // Assume Si4063 part number
    uint16_t part = si4063_read_part_info();
    if (part != 0x4063) {
        log_error("ERROR: Unknown or missing Si4063 part number: 0x%04x\n", part);
        return HAL_ERROR;
    }

    si4063_configure();

    si4063_configure_gpio(
            0x00, // GPIO0: Do nothing
            0x00, // GPIO1: Do nothing
            0x00, // GPIO2: Do nothing
            0x04, // GPIO3: Pin is configured as a CMOS input for direct mode transmissions.
            0x00 // Drive strength: HIGH
    );

    si4063_set_tx_power(0x00);

    si4063_set_frequency_offset(0);
    si4063_set_frequency_deviation(0x22);

    si4063_set_modulation_type(SI4063_MODULATION_TYPE_CW);

    si4063_set_state(SI4063_STATE_READY);

    return HAL_OK;
}
