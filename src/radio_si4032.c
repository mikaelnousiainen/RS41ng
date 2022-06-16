#include <stdint.h>
#include <string.h>

#include "hal/system.h"
#include "hal/spi.h"
#include "hal/pwm.h"
#include "hal/delay.h"
#include "hal/datatimer.h"
#include "drivers/si4032/si4032.h"
#include "log.h"

#include "radio_si4032.h"
#include "codecs/mfsk/mfsk.h"

#define CW_SYMBOL_RATE_MULTIPLIER 4

/**
 * I have attempted to implement Bell 202 frequency generation using hardware DMA and PWM, but have failed to generate
 * correct symbol rate that other APRS equipment are able to decode. I have tried to decode the DMA-based modulation with
 * some tools intended for debugging APRS and while some bytes are decoded correctly every once in a while,
 * the timings are mostly off for some unknown reason.
 *
 * The Bell 202 modulation implementation uses hardware PWM to generate the individual tone frequencies,
 * but when si4032_use_dma is false, the symbol timing is created in a loop with delay that was chosen
 * carefully via experiments.
 */
static bool si4032_use_dma = false;

// TODO: Add support for multiple APRS baud rates
uint16_t symbol_delay_bell_202_1200bps_us = 823;

static volatile bool radio_si4032_state_change = false;
static volatile uint32_t radio_si4032_freq = 0;
static volatile int8_t radio_dma_transfer_stop_after_counter = -1;

uint32_t precalculated_pwm_periods[FSK_TONE_COUNT_MAX];

uint16_t radio_si4032_fill_pwm_buffer(uint16_t offset, uint16_t length, uint16_t *buffer);

bool radio_start_transmit_si4032(radio_transmit_entry *entry, radio_module_state *shared_state)
{
    uint16_t frequency_offset;
    si4032_modulation_type modulation_type;
    bool use_direct_mode;

    switch (entry->data_mode) {
        case RADIO_DATA_MODE_CW:
        case RADIO_DATA_MODE_PIP:
            frequency_offset = 1;
            modulation_type = SI4032_MODULATION_TYPE_OOK;
            use_direct_mode = false;

            data_timer_init(entry->symbol_rate * CW_SYMBOL_RATE_MULTIPLIER);
            break;
        case RADIO_DATA_MODE_RTTY:
            frequency_offset = 0;
            modulation_type = SI4032_MODULATION_TYPE_NONE;
            use_direct_mode = false;
            break;
        case RADIO_DATA_MODE_APRS_1200:
            frequency_offset = 0;
            modulation_type = SI4032_MODULATION_TYPE_FSK;
            use_direct_mode = true;
            if (si4032_use_dma) {
                pwm_data_timer_init();
                radio_si4032_fill_pwm_buffer(0, PWM_TIMER_DMA_BUFFER_SIZE, pwm_timer_dma_buffer);
            }
            break;
        case RADIO_DATA_MODE_HORUS_V1:
        case RADIO_DATA_MODE_HORUS_V2: {
            fsk_tone *idle_tone = mfsk_get_idle_tone(&entry->fsk_encoder);
            frequency_offset = (uint16_t) idle_tone->index + HORUS_FREQUENCY_OFFSET_SI4032;
            modulation_type = SI4032_MODULATION_TYPE_OOK;
            use_direct_mode = false;

            data_timer_init(entry->fsk_encoder_api->get_symbol_rate(&entry->fsk_encoder));
            break;
        }
        default:
            return false;
    }

    si4032_set_tx_frequency(((float) entry->frequency) / 1000000.0f);
    si4032_set_tx_power(entry->tx_power);
    si4032_set_frequency_offset(frequency_offset);
    si4032_set_modulation_type(modulation_type);

    si4032_enable_tx();

    if (use_direct_mode) {
        spi_uninit();
        pwm_timer_init(100 * 100); // TODO: Idle tone
        pwm_timer_use(true);
        pwm_timer_pwm_enable(true);
        si4032_use_direct_mode(true);
    }

    switch (entry->data_mode) {
        case RADIO_DATA_MODE_CW:
        case RADIO_DATA_MODE_PIP:
            spi_uninit();
            system_disable_tick();
            si4032_use_sdi_pin(true);
            shared_state->radio_interrupt_transmit_active = true;
            break;
        case RADIO_DATA_MODE_APRS_1200:
            if (si4032_use_dma) {
                shared_state->radio_dma_transfer_active = true;
                radio_dma_transfer_stop_after_counter = -1;
                system_disable_tick();
                pwm_dma_start();
            } else {
                shared_state->radio_manual_transmit_active = true;
            }
            break;
        case RADIO_DATA_MODE_HORUS_V1:
        case RADIO_DATA_MODE_HORUS_V2:
            system_disable_tick();
            shared_state->radio_interrupt_transmit_active = true;
            break;
        default:
            break;
    }

    return true;
}

static uint32_t radio_next_symbol_si4032(radio_transmit_entry *entry, radio_module_state *shared_state)
{
    switch (entry->data_mode) {
        case RADIO_DATA_MODE_CW:
        case RADIO_DATA_MODE_PIP:
            return 0;
        case RADIO_DATA_MODE_RTTY:
            return 0;
        case RADIO_DATA_MODE_APRS_1200: {
            int8_t next_tone_index = entry->fsk_encoder_api->next_tone(&entry->fsk_encoder);
            if (next_tone_index < 0) {
                return 0;
            }

            return shared_state->radio_current_fsk_tones[next_tone_index].frequency_hz_100;
        }
        default:
            return 0;
    }
}

bool radio_transmit_symbol_si4032(radio_transmit_entry *entry, radio_module_state *shared_state)
{
    uint32_t frequency = radio_next_symbol_si4032(entry, shared_state);

    if (frequency == 0) {
        return false;
    }

    radio_si4032_freq = frequency;
    radio_si4032_state_change = true;

    return true;
}

static void radio_handle_main_loop_manual_si4032(radio_transmit_entry *entry, radio_module_state *shared_state)
{
    fsk_encoder_api *fsk_encoder_api = entry->fsk_encoder_api;
    fsk_encoder *fsk_enc = &entry->fsk_encoder;

    for (uint8_t i = 0; i < shared_state->radio_current_fsk_tone_count; i++) {
        precalculated_pwm_periods[i] = pwm_calculate_period(shared_state->radio_current_fsk_tones[i].frequency_hz_100);
    }

    system_disable_tick();

    switch (entry->data_mode) {
        case RADIO_DATA_MODE_APRS_1200: {
            int8_t tone_index;

            while ((tone_index = fsk_encoder_api->next_tone(fsk_enc)) >= 0) {
                pwm_timer_set_frequency(precalculated_pwm_periods[tone_index]);
                shared_state->radio_symbol_count_loop++;
                delay_us(symbol_delay_bell_202_1200bps_us);
            }

            radio_si4032_state_change = false;
            shared_state->radio_transmission_finished = true;
            break;
        }
        default:
            break;
    }

    system_enable_tick();
}

void radio_handle_main_loop_si4032(radio_transmit_entry *entry, radio_module_state *shared_state)
{
    if (entry->radio_type != RADIO_TYPE_SI4032 || shared_state->radio_interrupt_transmit_active) {
        return;
    }

    if (shared_state->radio_manual_transmit_active) {
        radio_handle_main_loop_manual_si4032(entry, shared_state);
        return;
    }

    if (radio_si4032_state_change) {
        radio_si4032_state_change = false;
        pwm_timer_set_frequency(radio_si4032_freq);
        shared_state->radio_symbol_count_loop++;
    }
}

inline void radio_handle_data_timer_si4032()
{
    static int cw_symbol_rate_multiplier = CW_SYMBOL_RATE_MULTIPLIER;

    if (radio_current_transmit_entry->radio_type != RADIO_TYPE_SI4032 || !radio_shared_state.radio_interrupt_transmit_active) {
        return;
    }

    switch (radio_current_transmit_entry->data_mode) {
        case RADIO_DATA_MODE_CW:
        case RADIO_DATA_MODE_PIP: {
            cw_symbol_rate_multiplier--;
            if (cw_symbol_rate_multiplier > 0) {
                break;
            }

            cw_symbol_rate_multiplier = CW_SYMBOL_RATE_MULTIPLIER;

            fsk_encoder_api *fsk_encoder_api = radio_current_transmit_entry->fsk_encoder_api;
            fsk_encoder *fsk_enc = &radio_current_transmit_entry->fsk_encoder;
            int8_t tone_index;

            tone_index = fsk_encoder_api->next_tone(fsk_enc);
            if (tone_index < 0) {
                si4032_set_sdi_pin(false);
                log_info("CW TX finished\n");
                radio_shared_state.radio_interrupt_transmit_active = false;
                radio_shared_state.radio_transmission_finished = true;
                system_enable_tick();
                break;
            }

            si4032_set_sdi_pin(tone_index == 0 ? false : true);

            radio_shared_state.radio_symbol_count_interrupt++;
            break;
        }
        case RADIO_DATA_MODE_HORUS_V1:
        case RADIO_DATA_MODE_HORUS_V2: {
            fsk_encoder_api *fsk_encoder_api = radio_current_transmit_entry->fsk_encoder_api;
            fsk_encoder *fsk_enc = &radio_current_transmit_entry->fsk_encoder;
            int8_t tone_index;

            tone_index = fsk_encoder_api->next_tone(fsk_enc);
            if (tone_index < 0) {
                log_info("Horus V1 TX finished\n");
                radio_shared_state.radio_interrupt_transmit_active = false;
                radio_shared_state.radio_transmission_finished = true;
                system_enable_tick();
                break;
            }

            si4032_set_frequency_offset_small(tone_index + HORUS_FREQUENCY_OFFSET_SI4032);

            radio_shared_state.radio_symbol_count_interrupt++;
            break;
        }
        default:
            break;
    }
}

bool radio_stop_transmit_si4032(radio_transmit_entry *entry, radio_module_state *shared_state)
{
    bool use_direct_mode = false;

    switch (entry->data_mode) {
        case RADIO_DATA_MODE_CW:
        case RADIO_DATA_MODE_PIP:
            si4032_use_sdi_pin(false);
            data_timer_uninit();
            spi_init();
            break;
        case RADIO_DATA_MODE_RTTY:
        case RADIO_DATA_MODE_HORUS_V1:
        case RADIO_DATA_MODE_HORUS_V2:
            data_timer_uninit();
            break;
        case RADIO_DATA_MODE_APRS_1200:
            use_direct_mode = true;
            break;
        default:
            break;
    }

    if (use_direct_mode) {
        si4032_use_direct_mode(false);
        pwm_timer_pwm_enable(false);
        pwm_timer_use(false);
        pwm_timer_uninit();
        spi_init();
    }

    si4032_inhibit_tx();

    switch (entry->data_mode) {
        case RADIO_DATA_MODE_CW:
        case RADIO_DATA_MODE_PIP:
            system_enable_tick();
            break;
        case RADIO_DATA_MODE_APRS_1200:
            if (si4032_use_dma) {
                pwm_data_timer_uninit();
                system_enable_tick();
            }
            break;
        case RADIO_DATA_MODE_HORUS_V1:
        case RADIO_DATA_MODE_HORUS_V2:
            system_enable_tick();
            break;
        default:
            break;
    }

    return true;
}

uint16_t radio_si4032_fill_pwm_buffer(uint16_t offset, uint16_t length, uint16_t *buffer)
{
    uint16_t count = 0;
    for (uint16_t i = offset; i < (offset + length); i++, count++) {
        uint32_t frequency = radio_next_symbol_si4032(radio_current_transmit_entry, &radio_shared_state);
        if (frequency == 0) {
            // TODO: fill the other side of the buffer with zeroes too?
            memset(buffer + offset, 0, (length - i) * sizeof(uint16_t));
            break;
        }
        buffer[i] = pwm_calculate_period(frequency);
    }

    return count;
}

bool radio_si4032_stop_dma_transfer_if_requested(radio_module_state *shared_state)
{
    if (radio_dma_transfer_stop_after_counter > 0) {
        radio_dma_transfer_stop_after_counter--;
    } else if (radio_dma_transfer_stop_after_counter == 0) {
        pwm_dma_stop();
        radio_dma_transfer_stop_after_counter = -1;
        shared_state->radio_transmission_finished = true;
        shared_state->radio_dma_transfer_active = false;
        return true;
    }

    return false;
}

uint16_t radio_si4032_handle_pwm_transfer_half(uint16_t buffer_size, uint16_t *buffer)
{
    if (radio_si4032_stop_dma_transfer_if_requested(&radio_shared_state)) {
        return 0;
    }
    if (radio_shared_state.radio_transmission_finished) {
        log_info("Should not be here, half-transfer!\n");
    }

    uint16_t length = radio_si4032_fill_pwm_buffer(0, buffer_size / 2, buffer);
    if (radio_dma_transfer_stop_after_counter < 0 && length < buffer_size / 2) {
        radio_dma_transfer_stop_after_counter = 2;
    }

    return length;
}

uint16_t radio_si4032_handle_pwm_transfer_full(uint16_t buffer_size, uint16_t *buffer)
{
    if (radio_si4032_stop_dma_transfer_if_requested(&radio_shared_state)) {
        return 0;
    }
    if (radio_shared_state.radio_transmission_finished) {
        log_info("Should not be here, transfer complete!\n");
    }

    uint16_t length = radio_si4032_fill_pwm_buffer(buffer_size / 2, buffer_size / 2, buffer);
    if (radio_dma_transfer_stop_after_counter < 0 && length < buffer_size / 2) {
        radio_dma_transfer_stop_after_counter = 2;
    }

    return length;
}

void radio_init_si4032()
{
    pwm_handle_dma_transfer_half = radio_si4032_handle_pwm_transfer_half;
    pwm_handle_dma_transfer_full = radio_si4032_handle_pwm_transfer_full;

    if (si4032_use_dma) {
        pwm_data_timer_init();
        pwm_dma_init();
    }
}
