#include <stdio.h>

#include "config.h"
#include "payload.h"
#include "telemetry.h"
#include "log.h"
#include "hal/system.h"
#include "hal/delay.h"
#include "hal/spi.h"
#include "hal/pwm.h"
#include "hal/usart_gps.h"
#include "codecs/fsk/fsk.h"
#include "codecs/bell/bell.h"
#include "codecs/aprs/aprs.h"
#include "codecs/ax25/ax25.h"
#include "codecs/jtencode/jtencode.h"
#include "drivers/si4032/si4032.h"
#include "si5351_handler.h"

typedef enum _radio_type {
    RADIO_TYPE_SI4032 = 1,
    RADIO_TYPE_SI5351,
} radio_type;

typedef enum _radio_data_mode {
    RADIO_DATA_MODE_CW = 1,
    RADIO_DATA_MODE_RTTY,
    RADIO_DATA_MODE_APRS,
    RADIO_DATA_MODE_WSPR,
    RADIO_DATA_MODE_FT8,
    RADIO_DATA_MODE_JT65,
    RADIO_DATA_MODE_JT4,
    RADIO_DATA_MODE_JT9,
    RADIO_DATA_MODE_FSQ_2,
    RADIO_DATA_MODE_FSQ_3,
    RADIO_DATA_MODE_FSQ_4_5,
    RADIO_DATA_MODE_FSQ_6,
} radio_data_mode;

typedef struct _radio_transmit_entry {
    radio_type radio_type;
    radio_data_mode data_mode;

    uint32_t frequency;
    uint8_t tx_power;
    uint32_t symbol_rate;

    payload_encoder *payload_encoder;
    fsk_encoder_api *fsk_encoder_api;

    jtencode_mode_type jtencode_mode_type;

    fsk_encoder fsk_encoder;
} radio_transmit_entry;

static bool si4032_use_dma = false;

static volatile bool radio_transmission_active = false;
static volatile bool radio_transmission_finished = false;
static volatile radio_transmit_entry *radio_current_transmit_entry = NULL;
static volatile int radio_current_transmit_entry_index = 0;
static volatile uint32_t radio_post_transmit_delay_counter = 0;
static volatile uint32_t radio_next_symbol_counter = 0;

static volatile bool radio_si5351_state_change = false;
static volatile uint64_t radio_si5351_freq = 0;

static volatile bool radio_si4032_state_change = false;
static volatile uint32_t radio_si4032_freq = 0;

static volatile radio_transmit_entry *radio_start_transmit_entry = NULL;
static volatile radio_transmit_entry *radio_stop_transmit_entry = NULL;
static volatile bool radio_transmit_next_symbol_flag = false;

static volatile uint32_t radio_symbol_count_interrupt = 0;
static volatile uint32_t radio_symbol_count_loop = 0;

static volatile bool radio_dma_transfer_active = false;
static volatile int8_t radio_dma_transfer_stop_after_counter = -1;

static volatile bool radio_manual_transmit_active = false;

uint8_t radio_current_payload[RADIO_PAYLOAD_MAX_LENGTH];
size_t radio_current_payload_length = 0;

fsk_tone *radio_current_fsk_tones = NULL;
int8_t radio_current_fsk_tone_count = 0;
uint32_t radio_current_tone_spacing_hz_100 = 0;

uint32_t radio_current_symbol_rate = 0;
uint32_t radio_current_symbol_delay_ms_100 = 0;

telemetry_data current_telemetry_data;

uint8_t aprs_packet[RADIO_PAYLOAD_MAX_LENGTH];

static volatile uint32_t start_tick = 0, end_tick = 0;

static size_t radio_fill_pwm_buffer(size_t offset, size_t length, uint16_t *buffer);

size_t radio_aprs_encode(uint8_t *payload, size_t length, telemetry_data *telemetry_data)
{
    aprs_generate_position_without_timestamp(
            aprs_packet, sizeof(aprs_packet), telemetry_data, APRS_SYMBOL_TABLE, APRS_SYMBOL, APRS_COMMENT);

    log_debug("aprs: %s\n", aprs_packet);

    return ax25_encode_packet_aprs(APRS_CALLSIGN, APRS_SSID, APRS_DESTINATION, APRS_DESTINATION_SSID, APRS_RELAYS,
            (char *) aprs_packet, length, payload);
}

payload_encoder radio_aprs_payload_encoder = {
        .encode = radio_aprs_encode,
};

size_t radio_ft8_encode(uint8_t *payload, size_t length, telemetry_data *telemetry_data)
{
    // TODO: Encode locator for FT8
    return snprintf((char *) payload, length, "%s %s", FT8_CALLSIGN, FT8_LOCATOR);
}

payload_encoder radio_ft8_payload_encoder = {
        .encode = radio_ft8_encode,
};

size_t radio_wspr_encode(uint8_t *payload, size_t length, telemetry_data *telemetry_data)
{
    // TODO: Encode locator for WSPR
    return snprintf((char *) payload, length, "");
}

payload_encoder radio_wspr_payload_encoder = {
        .encode = radio_wspr_encode,
};

#define RADIO_TRANSMIT_ENTRY_COUNT 1

static radio_transmit_entry radio_transmit_schedule[] = {
        {
                .radio_type = RADIO_TYPE_SI4032,
                .data_mode = RADIO_DATA_MODE_APRS,
                .frequency = RADIO_SI4032_TX_FREQUENCY_APRS,
                .tx_power = RADIO_SI4032_TX_POWER,
                .symbol_rate = 1200,
                .payload_encoder = &radio_aprs_payload_encoder,
                .fsk_encoder_api = &bell_fsk_encoder_api,
        },
/*        {
                .radio_type = RADIO_TYPE_SI5351,
                .data_mode = RADIO_DATA_MODE_FT8,
                .frequency = RADIO_SI5351_TX_FREQUENCY_FT8,
                .tx_power = RADIO_SI5351_TX_POWER,
                .payload_encoder = &radio_ft8_payload_encoder,
                .fsk_encoder_api = &jtencode_fsk_encoder_api,
                .jtencode_mode_type = JTENCODE_MODE_FT8,
        },
        {
                .radio_type = RADIO_TYPE_SI5351,
                .data_mode = RADIO_DATA_MODE_WSPR,
                .frequency = RADIO_SI5351_TX_FREQUENCY_WSPR,
                .tx_power = RADIO_SI5351_TX_POWER,
                .payload_encoder = &radio_wspr_payload_encoder,
                .fsk_encoder_api = &jtencode_fsk_encoder_api,
                .jtencode_mode_type = JTENCODE_MODE_WSPR,
        },*/
};

static bool radio_start_transmit_si4032(radio_transmit_entry *entry)
{
    uint16_t frequency_offset;
    si4032_modulation_type modulation_type;
    bool use_direct_mode;

    switch (entry->data_mode) {
        case RADIO_DATA_MODE_CW:
            frequency_offset = 1;
            modulation_type = SI4032_MODULATION_TYPE_OOK;
            use_direct_mode = true;
            break;
        case RADIO_DATA_MODE_RTTY:
            frequency_offset = 0;
            modulation_type = SI4032_MODULATION_TYPE_NONE;
            use_direct_mode = false;
            break;
        case RADIO_DATA_MODE_APRS:
            frequency_offset = 0;
            modulation_type = SI4032_MODULATION_TYPE_FSK;
            use_direct_mode = true;
            if (si4032_use_dma) {
                radio_fill_pwm_buffer(0, PWM_TIMER_DMA_BUFFER_SIZE, pwm_timer_dma_buffer);
            }
            break;
        default:
            return false;
    }

    si4032_set_tx_frequency(((float) entry->frequency) / 1000000.0f);
    si4032_set_tx_power(entry->tx_power * 7 / 100);
    si4032_set_frequency_offset(frequency_offset);
    si4032_set_modulation_type(modulation_type);

    si4032_enable_tx();

    if (use_direct_mode) {
        //delay_us(100);
        spi_uninit();
        pwm_timer_use(true);
        pwm_timer_pwm_enable(true);
        si4032_use_direct_mode(true);
    }

    switch (entry->data_mode) {
        case RADIO_DATA_MODE_APRS:
            if (si4032_use_dma) {
                radio_dma_transfer_active = true;
                radio_dma_transfer_stop_after_counter = -1;
                system_disable_tick();
                pwm_dma_start();
            } else {
                log_info("Si4032 manual TX\n");
                radio_manual_transmit_active = true;
            }
            break;
        default:
            break;
    }

    return true;
}

static bool radio_start_transmit_si5351(radio_transmit_entry *entry)
{
    si5351_set_drive_strength(SI5351_CLOCK_CLK0, entry->tx_power * 3 / 100);
    si5351_set_frequency(SI5351_CLOCK_CLK0, ((uint64_t) entry->frequency) * 100ULL);
    si5351_output_enable(SI5351_CLOCK_CLK0, true);
    return true;
}

static inline void radio_reset_next_symbol_counter()
{
    if (radio_current_symbol_rate > 0) {
        radio_next_symbol_counter = (uint32_t) (((float) SYSTEM_SCHEDULER_TIMER_TICKS_PER_SECOND) /
                                                (float) radio_current_symbol_rate);
    } else {
        radio_next_symbol_counter = (uint32_t) (((float) radio_current_symbol_delay_ms_100) *
                                                (float) SYSTEM_SCHEDULER_TIMER_TICKS_PER_SECOND / 100000.0f);
    }
}

static inline bool radio_should_transmit_next_symbol()
{
    return radio_next_symbol_counter == 0;
}

static bool radio_start_transmit(radio_transmit_entry *entry)
{
    bool success;

    radio_symbol_count_interrupt = 0;
    radio_symbol_count_loop = 0;

    telemetry_collect(&current_telemetry_data);

    radio_current_payload_length = entry->payload_encoder->encode(
            radio_current_payload, sizeof(radio_current_payload), &current_telemetry_data);

    log_info("Full payload length: %d\n", radio_current_payload_length);

    for (int i = 0; i < radio_current_payload_length; i++) {
        char c = radio_current_payload[i];
        if (c >= 0x20 && c <= 0x7e) {
            log_info("%c", c);
        } else {
            log_info(" [%02X] ", c);
        }
    }

    log_info("\n");

    switch (entry->data_mode) {
        case RADIO_DATA_MODE_CW:
            break;
        case RADIO_DATA_MODE_RTTY:
            break;
        case RADIO_DATA_MODE_APRS:
            // TODO: make bell tones and flag field count configurable
            bell_encoder_new(&entry->fsk_encoder, entry->symbol_rate, BELL_FLAG_FIELD_COUNT_1200, bell202_tones);
            radio_current_symbol_rate = entry->fsk_encoder_api->get_symbol_rate(&entry->fsk_encoder);
            entry->fsk_encoder_api->get_tones(&entry->fsk_encoder, &radio_current_fsk_tone_count,
                    &radio_current_fsk_tones);
            entry->fsk_encoder_api->set_data(&entry->fsk_encoder, radio_current_payload_length, radio_current_payload);
            break;
        case RADIO_DATA_MODE_WSPR:
        case RADIO_DATA_MODE_FT8:
        case RADIO_DATA_MODE_JT65:
        case RADIO_DATA_MODE_JT4:
        case RADIO_DATA_MODE_JT9:
        case RADIO_DATA_MODE_FSQ_2:
        case RADIO_DATA_MODE_FSQ_3:
        case RADIO_DATA_MODE_FSQ_4_5:
        case RADIO_DATA_MODE_FSQ_6:
            // TODO: Encode WSPR locator
            jtencode_encoder_new(&entry->fsk_encoder, entry->jtencode_mode_type, WSPR_CALLSIGN, WSPR_LOCATOR, WSPR_DBM,
                    FSQ_CALLSIGN_FROM, FSQ_CALLSIGN_TO, FSQ_COMMAND);
            radio_current_symbol_delay_ms_100 = entry->fsk_encoder_api->get_symbol_delay(&entry->fsk_encoder);
            radio_current_tone_spacing_hz_100 = entry->fsk_encoder_api->get_tone_spacing(&entry->fsk_encoder);
            entry->fsk_encoder_api->set_data(&entry->fsk_encoder, radio_current_payload_length, radio_current_payload);
            break;
        default:
            return false;
    }

    // USART interrupts may interfere with transmission timing
    usart_gps_enable(false);

    switch (entry->radio_type) {
        case RADIO_TYPE_SI4032:
            success = radio_start_transmit_si4032(entry);
            break;
        case RADIO_TYPE_SI5351:
            success = radio_start_transmit_si5351(entry);
            break;
        default:
            return false;
    }

    if (!success) {
        usart_gps_enable(true);
        // TODO: stop transmit here
        return false;
    }

    log_info("TX enabled\n");

    system_set_red_led(true);

    radio_transmission_active = true;

    return true;
}

static bool radio_stop_transmit_si4032(radio_transmit_entry *entry)
{
    bool use_direct_mode;

    switch (entry->data_mode) {
        case RADIO_DATA_MODE_CW:
            use_direct_mode = true;
            break;
        case RADIO_DATA_MODE_RTTY:
            use_direct_mode = false;
            break;
        case RADIO_DATA_MODE_APRS:
            use_direct_mode = true;
            if (si4032_use_dma) {
                system_enable_tick();
            }
            break;
        default:
            break;
    }

    if (use_direct_mode) {
        si4032_use_direct_mode(false);
        pwm_timer_pwm_enable(false);
        //delay_us(100);
        pwm_timer_use(false);
        //delay_us(100);
        spi_init();
        //delay_us(100);
    }

    si4032_inhibit_tx();

    switch (entry->data_mode) {
        case RADIO_DATA_MODE_APRS:
            if (si4032_use_dma) {
                system_enable_tick();
            }
            break;
        default:
            break;
    }

    return true;
}

static bool radio_stop_transmit_si5351(radio_transmit_entry *entry)
{
    si5351_output_enable(SI5351_CLOCK_CLK0, false);
    return true;
}

static inline void radio_reset_transmit_state()
{
    radio_transmission_active = false;
    radio_next_symbol_counter = 0;

    radio_current_payload_length = 0;

    radio_current_fsk_tones = NULL;
    radio_current_fsk_tone_count = 0;
    radio_current_tone_spacing_hz_100 = 0;

    radio_current_symbol_rate = 0;
    radio_current_symbol_delay_ms_100 = 0;
}

static bool radio_stop_transmit(radio_transmit_entry *entry)
{
    bool success;

    switch (entry->radio_type) {
        case RADIO_TYPE_SI4032:
            success = radio_stop_transmit_si4032(entry);
            break;
        case RADIO_TYPE_SI5351:
            success = radio_stop_transmit_si5351(entry);
            break;
        default:
            return false;
    }

    radio_reset_transmit_state();

    radio_manual_transmit_active = false;
    radio_dma_transfer_active = false;

    switch (entry->data_mode) {
        case RADIO_DATA_MODE_CW:
            break;
        case RADIO_DATA_MODE_RTTY:
            break;
        case RADIO_DATA_MODE_APRS:
            bell_encoder_destroy(&entry->fsk_encoder);
            break;
        case RADIO_DATA_MODE_WSPR:
        case RADIO_DATA_MODE_FT8:
        case RADIO_DATA_MODE_JT65:
        case RADIO_DATA_MODE_JT4:
        case RADIO_DATA_MODE_JT9:
        case RADIO_DATA_MODE_FSQ_2:
        case RADIO_DATA_MODE_FSQ_3:
        case RADIO_DATA_MODE_FSQ_4_5:
        case RADIO_DATA_MODE_FSQ_6:
            jtencode_encoder_destroy(&entry->fsk_encoder);
            break;
        default:
            return false;
    }

    usart_gps_enable(true);
    system_set_red_led(false);

    return success;
}

static uint32_t radio_next_symbol_si4032(radio_transmit_entry *entry)
{
    switch (entry->data_mode) {
        case RADIO_DATA_MODE_CW:
            return 0;
        case RADIO_DATA_MODE_RTTY:
            return 0;
        case RADIO_DATA_MODE_APRS: {
            int8_t next_tone_index = entry->fsk_encoder_api->next_tone(&entry->fsk_encoder);
            if (next_tone_index < 0) {
                return 0;
            }

            return radio_current_fsk_tones[next_tone_index].frequency_hz_100;
        }
        default:
            return 0;
    }
}

static bool radio_transmit_symbol_si4032(radio_transmit_entry *entry)
{
    uint32_t frequency = radio_next_symbol_si4032(entry);

    if (frequency == 0) {
        return false;
    }

    radio_si4032_freq = frequency;
    radio_si4032_state_change = true;

    return true;
}

static bool radio_transmit_symbol_si5351(radio_transmit_entry *entry)
{
    switch (entry->data_mode) {
        case RADIO_DATA_MODE_CW:
            return false;
        default: {
            int8_t next_tone_index = entry->fsk_encoder_api->next_tone(&entry->fsk_encoder);
            if (next_tone_index < 0) {
                return false;
            }

            log_trace("Tone: %d\n", next_tone_index);

            uint64_t frequency =
                    ((uint64_t) entry->frequency) * 100UL + (next_tone_index * radio_current_tone_spacing_hz_100);
            radio_si5351_freq = frequency;
            radio_si5351_state_change = true;
            break;
        }
    }

    return true;
}

static bool radio_transmit_symbol(radio_transmit_entry *entry)
{
    bool success;

    switch (entry->radio_type) {
        case RADIO_TYPE_SI4032:
            success = radio_transmit_symbol_si4032(entry);
            break;
        case RADIO_TYPE_SI5351:
            success = radio_transmit_symbol_si5351(entry);
            break;
        default:
            return false;
    }

    if (success) {
        radio_symbol_count_interrupt++;
    }

    return success;
}

static void radio_next_transmit_entry()
{
    radio_current_transmit_entry_index = (radio_current_transmit_entry_index + 1) % RADIO_TRANSMIT_ENTRY_COUNT;
    radio_current_transmit_entry = &radio_transmit_schedule[radio_current_transmit_entry_index];
    radio_post_transmit_delay_counter = RADIO_POST_TRANSMIT_DELAY * SYSTEM_SCHEDULER_TIMER_TICKS_PER_SECOND / 1000;
}

void radio_handle_timer_tick()
{
    if (radio_dma_transfer_active || radio_manual_transmit_active) {
        return;
    }

    if (radio_next_symbol_counter > 0) {
        radio_next_symbol_counter--;
    }

    if (radio_transmission_active && radio_should_transmit_next_symbol()) {
        radio_transmit_next_symbol_flag = true;
        radio_reset_next_symbol_counter();
    }

    if (!radio_transmission_active && radio_post_transmit_delay_counter > 0) {
        radio_post_transmit_delay_counter--;
    }

    // TODO: specify which modes need time synchronization from GPS
    // TODO: implement time sync
}

uint16_t symbol_delay = 823; // -> good around ~823 for a tight loop

void radio_handle_main_loop()
{
    if (radio_post_transmit_delay_counter == 0) {
        telemetry_collect(&current_telemetry_data);
        log_info("Battery: %d mV\n", current_telemetry_data.battery_voltage_millivolts);
        log_info("Internal temperature: %ld C*100\n", current_telemetry_data.internal_temperature_celsius_100);
        log_info("Time: %02d:%02d:%02d\n",
                current_telemetry_data.gps.hours, current_telemetry_data.gps.minutes,
                current_telemetry_data.gps.seconds);
        log_info("Fix: %d, Sats: %d, OK packets: %d, Bad packets: %d\n",
                current_telemetry_data.gps.fix, current_telemetry_data.gps.sats_raw,
                current_telemetry_data.gps.ok_packets, current_telemetry_data.gps.bad_packets);
        log_info("Lat: %ld *1M, Lon: %ld *1M, Alt: %ld m\n",
                current_telemetry_data.gps.lat_raw / 10, current_telemetry_data.gps.lon_raw / 10,
                (current_telemetry_data.gps.alt_raw / 1000) * 3280 / 1000);

        radio_post_transmit_delay_counter = RADIO_POST_TRANSMIT_DELAY * SYSTEM_SCHEDULER_TIMER_TICKS_PER_SECOND / 1000;
        radio_start_transmit_entry = radio_current_transmit_entry;
    }

    if (radio_si4032_state_change) {
        if (radio_manual_transmit_active) {
            // TODO: Refactor this code for proper handling of APRS
            fsk_encoder_api *fsk_encoder_api = radio_current_transmit_entry->fsk_encoder_api;
            fsk_encoder *fsk_enc = &radio_current_transmit_entry->fsk_encoder;
            log_info("Si4032 manual TX start: %d\n", symbol_delay);
            system_disable_tick();
            int8_t tone_index = 0;
            uint32_t pwm_periods[2];
            pwm_periods[0] = pwm_calculate_period(radio_current_fsk_tones[0].frequency_hz_100);
            pwm_periods[1] = pwm_calculate_period(radio_current_fsk_tones[1].frequency_hz_100);
            switch (radio_current_transmit_entry->data_mode) {
                case RADIO_DATA_MODE_APRS:
                    do {
                        // radio_si4032_state_change = false;
                        pwm_timer_set_frequency(pwm_periods[tone_index]);
                        // radio_symbol_count_loop++;
                        delay_us(symbol_delay);
                        tone_index = fsk_encoder_api->next_tone(fsk_enc);
                    } while (tone_index >= 0);
                    //} while (radio_transmit_symbol(radio_current_transmit_entry));

                    radio_si4032_state_change = false;
                    radio_transmission_finished = true;
                    break;
                default:
                    break;
            }
            system_enable_tick();
            //symbol_delay += 1;
        } else {
            radio_si4032_state_change = false;
            pwm_timer_set_frequency(radio_si4032_freq);
            radio_symbol_count_loop++;
        }
        return;
    }

    if (radio_si5351_state_change) {
        radio_si5351_state_change = false;
        si5351_set_frequency(SI5351_CLOCK_CLK0, radio_si5351_freq);
        return;
    }

    bool first_symbol = false;
    if (radio_start_transmit_entry != NULL) {
        log_info("Start transmit\n");
        bool success = radio_start_transmit(radio_start_transmit_entry);
        start_tick = system_get_tick();

        radio_start_transmit_entry = NULL;
        if (!success) {
            radio_next_transmit_entry();
            return;
        }

        if (!radio_dma_transfer_active) {
            first_symbol = true;
            radio_transmit_next_symbol_flag = true;
        }
    }

    if (radio_transmission_active && radio_transmit_next_symbol_flag) {
        radio_transmit_next_symbol_flag = false;
        bool success = radio_transmit_symbol(radio_current_transmit_entry);
        if (!success) {
            radio_transmission_finished = true;
        }
        if (first_symbol) {
            radio_reset_next_symbol_counter();
        }
    }

    if (radio_transmission_finished) {
        radio_stop_transmit_entry = radio_current_transmit_entry;
        end_tick = system_get_tick();
        radio_transmission_finished = false;
    }

    if (radio_stop_transmit_entry != NULL) {
        radio_stop_transmit(radio_stop_transmit_entry);
        radio_stop_transmit_entry = NULL;

        radio_next_transmit_entry();

        log_info("Transmit stopped\n");
        log_info("Symbol count (interrupt): %ld\n", radio_symbol_count_interrupt);
        log_info("Symbol count (loop): %ld\n", radio_symbol_count_loop);
        log_info("Total ticks: %ld\n", end_tick - start_tick);
        log_info("Next symbol counter: %ld\n", radio_next_symbol_counter);
        log_info("Symbol rate: %ld\n", radio_current_symbol_rate);
        log_info("Symbol delay: %ld\n", radio_current_symbol_delay_ms_100);
        log_info("Tone spacing: %ld\n", radio_current_tone_spacing_hz_100);
    }
}

static size_t radio_fill_pwm_buffer(size_t offset, size_t length, uint16_t *buffer)
{
    size_t count = 0;
    for (size_t i = offset; i < (offset + length); i++, count++) {
        uint32_t frequency = radio_next_symbol_si4032(radio_current_transmit_entry);
        if (frequency == 0) {
            // TODO: fill the other side of the buffer with zeroes too?
            memset(buffer + offset, 0, (length - i) * sizeof(uint16_t));
            break;
        }
        buffer[i] = pwm_calculate_period(frequency);
    }

    return count;
}

static bool radio_stop_dma_transfer_if_requested()
{
    if (radio_dma_transfer_stop_after_counter > 0) {
        radio_dma_transfer_stop_after_counter--;
    } else if (radio_dma_transfer_stop_after_counter == 0) {
        pwm_dma_stop();
        radio_dma_transfer_stop_after_counter = -1;
        radio_transmission_finished = true;
        radio_dma_transfer_active = false;
        return true;
    }

    return false;
}

size_t radio_handle_pwm_transfer_half(size_t buffer_size, uint16_t *buffer)
{
    if (radio_stop_dma_transfer_if_requested()) {
        return 0;
    }
    if (radio_transmission_finished) {
        log_info("Should not be here, half-transfer!\n");
    }

    size_t length = radio_fill_pwm_buffer(0, buffer_size / 2, buffer);
    if (radio_dma_transfer_stop_after_counter < 0 && length < buffer_size / 2) {
        radio_dma_transfer_stop_after_counter = 2;
    }

    return length;
}

size_t radio_handle_pwm_transfer_full(size_t buffer_size, uint16_t *buffer)
{
    if (radio_stop_dma_transfer_if_requested()) {
        return 0;
    }
    if (radio_transmission_finished) {
        log_info("Should not be here, transfer complete!\n");
    }

    size_t length = radio_fill_pwm_buffer(buffer_size / 2, buffer_size / 2, buffer);
    if (radio_dma_transfer_stop_after_counter < 0 && length < buffer_size / 2) {
        radio_dma_transfer_stop_after_counter = 2;
    }

    return length;
}

void radio_init()
{
    pwm_handle_dma_transfer_half = radio_handle_pwm_transfer_half;
    pwm_handle_dma_transfer_full = radio_handle_pwm_transfer_full;

    radio_current_transmit_entry = &radio_transmit_schedule[radio_current_transmit_entry_index];

    pwm_timer_init(100 * 100);

    if (si4032_use_dma) {
        pwm_data_timer_init();
        pwm_dma_init();
    }
}
