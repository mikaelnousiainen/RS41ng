/**
 * The DFM17 radiosonde-compatible Si4063 driver code has been inspired by:
 * - pAVAR9: https://github.com/Upuaut/pAVAR9/
 * - uTrak: https://github.com/thasti/utrak/
 * - DFM17 APRS/RTTY firmware by Derek Rowland
 */

#include <stdbool.h>
#include <stm32f10x_gpio.h>
#include <stm32f10x_tim.h>

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
#define SI4063_COMMAND_FIFO_INFO    0x15
#define SI4063_COMMAND_GET_INT_STATUS 0x20
#define SI4063_COMMAND_START_TX     0x31
#define SI4063_COMMAND_REQUEST_DEVICE_STATE 0x33
#define SI4063_COMMAND_CHANGE_STATE 0x34
#define SI4063_COMMAND_GET_ADC_READING  0x14
#define SI4063_COMMAND_READ_CMD_BUFF    0x44
#define SI4063_COMMAND_WRITE_TX_FIFO 0x66

#define SI4063_STATE_SLEEP        0x01
#define SI4063_STATE_SPI_ACTIVE   0x02
#define SI4063_STATE_READY        0x03
#define SI4063_STATE_READY2       0x04
#define SI4063_STATE_TX_TUNE      0x05
#define SI4063_STATE_TX           0x07

/**
 * Modulation settings from Derek Rowland's firmware
 */

#define SI4063_DATA_RATE_APRS 4400

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

uint32_t current_frequency_hz = 434000000UL;
uint32_t current_deviation_hz = 0;

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

// Returns number of bytes sent from *data
// If less than len, remaining bytes will need to be used to top up the buffer
uint16_t si4063_start_tx(uint8_t *data, int len)
{
    // Clear fifo underflow interrupt
    si4063_fifo_underflow();

    // Clear TX FIFO
    uint8_t fifo_clear_data[] = {1};
    si4063_send_command(SI4063_COMMAND_FIFO_INFO, 1, fifo_clear_data);
    si4063_wait_for_cts();

    // Add our data to the TX FIFO
    int fifo_len = len;
    if(fifo_len > 64) {
        fifo_len = 64;
    }
    si4063_send_command(SI4063_COMMAND_WRITE_TX_FIFO, fifo_len, data);

    // Start transmitting
    uint8_t tx_cmd[] = {
        0, // channel
        SI4063_STATE_SLEEP << 4,
        len >> 8,
        len & 0xFF,
        0 // delay
    };
    si4063_send_command(SI4063_COMMAND_START_TX, sizeof(tx_cmd), tx_cmd);
    si4063_wait_for_cts();

    return fifo_len;
}

// Add additional bytes to the si4063's FIFO buffer
// Needed for large packets that don't fit in its buffer.
// Keep refilling it while transmitting
// Returns number of bytes taken from *data
// If less than len, you will need to keep calling this
uint16_t si4063_refill_buffer(uint8_t *data, int len)
{
    uint8_t response[2] = {0, 0};

    // Check how many bytes we have free
    si4063_send_command(SI4063_COMMAND_FIFO_INFO, 0, NULL);
    si4063_read_response(sizeof(response), response);

    uint8_t free_space = response[1];
    if(free_space < len) {
        len = free_space;
    }

    si4063_send_command(SI4063_COMMAND_WRITE_TX_FIFO, len, data);

    return len;
}

// Wait for our buffer to be emptied, and for the si4063 to leave TX mode
// If timeout, we force it to sleep
int si4063_wait_for_tx_complete(int timeout_ms)
{
    for(int i = 0; i < timeout_ms; i++) {
        uint8_t status = 0;
        si4063_send_command(SI4063_COMMAND_REQUEST_DEVICE_STATE, 0, NULL);
        si4063_read_response(1, &status);

        if(status == SI4063_STATE_SLEEP ||
           status == SI4063_STATE_READY ||
           status == SI4063_STATE_READY2 ||
           status == SI4063_STATE_SPI_ACTIVE) {
            return HAL_OK;
        }

        delay_ms(1);
    }

    si4063_disable_tx();
    si4063_wait_for_cts();

    return HAL_ERROR_TIMEOUT;
}

static int si4063_get_outdiv(const uint32_t frequency_hz)
{
    // Select the output divider according to the recommended ranges in the Si406x datasheet
    if (frequency_hz < 177000000UL) {
        return 24;
    } else if (frequency_hz < 239000000UL) {
        return 16;
    } else if (frequency_hz < 353000000UL) {
        return 12;
    } else if (frequency_hz < 525000000UL) {
        return 8;
    } else if (frequency_hz < 705000000UL) {
        return 6;
    }

    return 4;
}

static int si4063_get_band(const uint32_t frequency_hz)
{
    if (frequency_hz < 177000000UL)      {
        return 5;
    } else if (frequency_hz < 239000000UL) {
        return 4;
    } else if (frequency_hz < 353000000UL) {
        return 3;
    } else if (frequency_hz < 525000000UL) {
        return 2;
    } else if (frequency_hz < 705000000UL) {
        return 1;
    }

    return 0;
}

// Also clears status
bool si4063_fifo_underflow()
{
    uint8_t data[] = {0xFF, 0xFF, ~0x20}; // Clear underflow status
    si4063_send_command(SI4063_COMMAND_GET_INT_STATUS, sizeof(data), data);
    uint8_t response[7];
    si4063_read_response(sizeof(response), response);

    bool fifo_underflow_pending = response[6] & 0x20;
    return fifo_underflow_pending;
}

void si4063_set_tx_frequency(const uint32_t frequency_hz)
{
    uint8_t outdiv, band;
    uint32_t f_pfd, n, m;
    float ratio, rest;

    log_debug("Si4063: Set frequency %lu\n", frequency_hz);

    outdiv = si4063_get_outdiv(frequency_hz);
    band = si4063_get_band(frequency_hz);

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

    current_frequency_hz = frequency_hz;

    // Deviation depends on the frequency band
    si4063_set_frequency_deviation(current_deviation_hz);
}

void si4063_set_data_rate(const uint32_t rate_bps)
{
    int rate = rate_bps * 10;
    // Set TX_NCO_MODE to our crystal frequency, as recommended by the data sheet for rates <= 200kbps
    // Set MODEM_DATA_RATE to rate_bps * 10 (will get downsampled because NCO_MODE defaults to 10x)
    uint8_t data[] = {
        0x20, // Group
        0x07, // Set 7 properties
        0x03, // Start from MODEM_DATA_RATE
        (rate >> 16) & 0xFF,
        (rate >> 8) & 0xFF,
        rate & 0xFF,
        (SI4063_CLOCK >> 24) & 0xFF,
        (SI4063_CLOCK >> 16) & 0xFF,
        (SI4063_CLOCK >> 8) & 0xFF,
        SI4063_CLOCK & 0xFF,
    };
    si4063_send_command(SI4063_COMMAND_SET_PROPERTY, sizeof(data), data);
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

static uint32_t si4063_calculate_deviation(uint32_t deviation_hz)
{
    uint8_t outdiv = si4063_get_outdiv(current_frequency_hz);

    // SY_SEL = Div-by-2
    return (uint32_t) (((double) (1 << 19) * outdiv * deviation_hz) / (2 * SI4063_CLOCK));
}

void si4063_set_frequency_deviation(uint32_t deviation_hz)
{
    uint32_t deviation = si4063_calculate_deviation(deviation_hz);

    uint8_t data[] = {
            0x20, // 0x20 = Group MODEM
            0x03, // Set 3 properties (3 bytes)
            0x0A, // 0x0A = MODEM_FREQ_DEV
            (deviation >> 16) & 0xFF,
            (deviation >> 8) & 0xFF,
            deviation & 0xFF
    };

    log_info("Si4063: Set frequency deviation to value %lu with %lu Hz\n", deviation, deviation_hz);

    si4063_send_command(SI4063_COMMAND_SET_PROPERTY, sizeof(data), data);

    current_deviation_hz = deviation_hz;
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
        case SI4063_MODULATION_TYPE_FIFO_FSK:
            // FIFO with FSK modulation
            data[3] = 0x02;
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

inline void si4063_set_direct_mode_pin(bool high)
{
    if (high) {
        GPIO_SetBits(GPIO_SI4063_GPIO3, GPIO_PIN_SI4063_GPIO3);
    } else {
        GPIO_ResetBits(GPIO_SI4063_GPIO3, GPIO_PIN_SI4063_GPIO3);
    }
}

void si4063_set_crystal_capacitance(uint8_t c_count)
{
        uint8_t data[] = {
                0x00, // 0x00 = Group GLOBAL
                0x01, // Set 1 property
                0x00, // 0x00 = GLOBAL_XO_TUNE
                c_count  // Value of 62 standard determined for DFM17 radiosondes
        };

        si4063_send_command(SI4063_COMMAND_SET_PROPERTY, sizeof(data), data);
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
                0x10 | // 129-byte FIFO
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

    {
        /*
          2. Detailed Errata Descriptions
2.1 If Configured to Skip Sync and Preamble on Transmit, the TX Data from the FIFO is Corrupted
Description of Errata
If preamble and sync word are excluded from the transmitted data (PREMABLE_TX_LENGTH = 0 and SYNC_CONFIG: SKIP_TX = 1), data from the FIFO is not transmitted correctly.
Affected Conditions / Impacts
Some number of missed bytes will occur at the beginning of the packet and some number of repeated bytes at the end of the packet.
Workaround
Set PKT_FIELD_1_CRC_CONFIG: CRC_START to 1. This will trigger the packet handler and result in transmitting the correct data,
while still not sending a CRC unless enabled in a FIELD configuration. A fix has been identified and will be included in a future release
        */

        // In other words, without this, the FIFO buffer gets corrupted while TXing, because we're not using
        // the preamble/sync word stuff
        // To be clear - we're not doing any CRC stuff! This is just the recommended workaround
        uint8_t data[] = {
            0x12, // Group
            0x01, // Set 1 property
            0x10, // PKT_FIELD_1_CRC_CONFIG
            0x80, // CRC_START
        };
        si4063_send_command(SI4063_COMMAND_SET_PROPERTY, sizeof(data), data);
    }
}

// Not used yet, for future use
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

// Not used yet, for future use
void si4063_configure_aprs()
{
    // For APRS in direct mode with GFSK modulation
    si4063_configure_data_rate(SI4063_DATA_RATE_APRS);

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

// Not used yet, for future use
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

    // Set deviation to zero for non-FSK modulations
    si4063_set_frequency_deviation(0);

    si4063_set_modulation_type(SI4063_MODULATION_TYPE_CW);

    si4063_set_state(SI4063_STATE_READY);

    return HAL_OK;
}

void TIM1_BRK_TIM15_IRQHandler(void)
{
#ifdef DFM17
    static bool pin_state = false;
#endif

    if (TIM_GetITStatus(TIM15, TIM_IT_Update) != RESET) {
        TIM_ClearITPendingBit(TIM15, TIM_IT_Update);
#ifdef DFM17
        // Restrict the interrupt to DFM17 only just in case this ISR gets called on RS41
        pin_state = !pin_state;
        si4063_set_direct_mode_pin(pin_state);
#endif
    }
}
