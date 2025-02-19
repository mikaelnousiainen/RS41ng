#include "config.h"

#ifdef DFM17
#include "hal/system.h"
#include "hal/spi.h"
#include "hal/pwm.h"
#include "hal/delay.h"
#include "hal/datatimer.h"
#include "drivers/si4063/si4063.h"
#include "log.h"

#include "radio_si4063.h"
#include "codecs/mfsk/mfsk.h"

#define SI4063_DEVIATION_HZ_RTTY 200.0
#define SI4063_DEVIATION_HZ_APRS 2600.0
#define SI4063_DEVIATION_HZ_CATS 4800.0

#define CW_SYMBOL_RATE_MULTIPLIER 4

// TODO: Add support for multiple APRS baud rates
// This delay is for DFM-17 radiosondes
#define symbol_delay_bell_202_1200bps_us 821

static volatile bool radio_si4063_state_change = false;
static volatile uint32_t radio_si4063_freq = 0;

bool radio_start_transmit_si4063(radio_transmit_entry *entry, radio_module_state *shared_state)
{
    uint16_t frequency_offset;
    uint32_t frequency_deviation = 0;
    uint32_t data_rate = 0;
    si4063_modulation_type modulation_type;
    bool use_direct_mode;
    bool use_fifo_mode = false;

    switch (entry->data_mode) {
        case RADIO_DATA_MODE_CW:
        case RADIO_DATA_MODE_PIP:
            frequency_offset = 1;
            modulation_type = SI4063_MODULATION_TYPE_OOK;
            use_direct_mode = false;

            data_timer_init(entry->symbol_rate * CW_SYMBOL_RATE_MULTIPLIER);
            break;
        case RADIO_DATA_MODE_RTTY:
            frequency_offset = 0;
            frequency_deviation = SI4063_DEVIATION_HZ_RTTY;
            modulation_type = SI4063_MODULATION_TYPE_CW;
            use_direct_mode = false;
            break;
        case RADIO_DATA_MODE_APRS_1200:
            frequency_offset = 0;
            frequency_deviation = SI4063_DEVIATION_HZ_APRS;
            modulation_type = SI4063_MODULATION_TYPE_FSK;
            use_direct_mode = true;
            break;
        case RADIO_DATA_MODE_HORUS_V1:
        case RADIO_DATA_MODE_HORUS_V2: {
            fsk_tone *idle_tone = mfsk_get_idle_tone(&entry->fsk_encoder);
            frequency_offset = (uint16_t) idle_tone->index + HORUS_FREQUENCY_OFFSET_SI4063;
            modulation_type = SI4063_MODULATION_TYPE_CW;
            use_direct_mode = false;

            data_timer_init(entry->fsk_encoder_api->get_symbol_rate(&entry->fsk_encoder));
            break;
        }
        case RADIO_DATA_MODE_CATS:
            frequency_offset = 0;
            frequency_deviation = SI4063_DEVIATION_HZ_CATS;
            modulation_type = SI4063_MODULATION_TYPE_FIFO_FSK;
            use_direct_mode = false;
            use_fifo_mode = true;
            data_rate = 9600;
            break;
        default:
            return false;
    }

    si4063_set_tx_frequency(entry->frequency);
    si4063_set_tx_power(entry->tx_power);
    si4063_set_frequency_offset(frequency_offset);
    si4063_set_modulation_type(modulation_type);
    si4063_set_frequency_deviation(frequency_deviation);

    if (use_fifo_mode) {
        si4063_set_data_rate(data_rate);
    }
    else {
        si4063_enable_tx();
    }

    if (use_direct_mode) {
        spi_uninit();
        pwm_timer_init(100 * 100); // TODO: Idle tone
        pwm_timer_use(true);
        pwm_timer_pwm_enable(true);
    }

    switch (entry->data_mode) {
        case RADIO_DATA_MODE_CW:
        case RADIO_DATA_MODE_PIP:
            spi_uninit();
            system_disable_tick();
            shared_state->radio_interrupt_transmit_active = true;
            break;
        case RADIO_DATA_MODE_APRS_1200:
            shared_state->radio_manual_transmit_active = true;
            break;
        case RADIO_DATA_MODE_HORUS_V1:
        case RADIO_DATA_MODE_HORUS_V2:
            system_disable_tick();
            shared_state->radio_interrupt_transmit_active = true;
            break;
        case RADIO_DATA_MODE_CATS:
            shared_state->radio_fifo_transmit_active = true;
            break;
        default:
            break;
    }

    return true;
}

static uint32_t radio_next_symbol_si4063(radio_transmit_entry *entry, radio_module_state *shared_state)
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

bool radio_transmit_symbol_si4063(radio_transmit_entry *entry, radio_module_state *shared_state)
{
    uint32_t frequency = radio_next_symbol_si4063(entry, shared_state);

    if (frequency == 0) {
        return false;
    }

    radio_si4063_freq = frequency;
    radio_si4063_state_change = true;

    return true;
}

static void radio_handle_main_loop_manual_si4063(radio_transmit_entry *entry, radio_module_state *shared_state)
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

            radio_si4063_state_change = false;
            shared_state->radio_transmission_finished = true;
            break;
        }
        default:
            break;
    }

    system_enable_tick();
}

void radio_handle_fifo_si4063(radio_transmit_entry *entry, radio_module_state *shared_state) {
    log_debug("Start FIFO TX\n");
    fsk_encoder_api *fsk_encoder_api = entry->fsk_encoder_api;
    fsk_encoder *fsk_enc = &entry->fsk_encoder;

    uint8_t *data = fsk_encoder_api->get_data(fsk_enc);
    uint16_t len = fsk_encoder_api->get_data_len(fsk_enc);

    uint16_t written = si4063_start_tx(data, len);
    data += written;
    len -= written;
    
    while(len > 0) {
        uint16_t written = si4063_refill_buffer(data, len);
        data += written;
        len -= written;

        if(si4063_fifo_underflow()) {
            log_info("FIFO underflow - Aborting\n");
            shared_state->radio_transmission_finished = true;

            return;
        }
    }

    int err = si4063_wait_for_tx_complete(1000);
    if(err != HAL_OK) {
        log_info("Error waiting for tx complete: %d\n", err);
    }

    log_debug("Finished FIFO TX\n");

    shared_state->radio_transmission_finished = true;
}

void radio_handle_main_loop_si4063(radio_transmit_entry *entry, radio_module_state *shared_state)
{
    if (entry->radio_type != RADIO_TYPE_SI4063 || shared_state->radio_interrupt_transmit_active || shared_state->radio_transmission_finished) {
        return;
    }

    if (shared_state->radio_manual_transmit_active) {
        radio_handle_main_loop_manual_si4063(entry, shared_state);
        return;
    }

    if (shared_state->radio_fifo_transmit_active) {
        radio_handle_fifo_si4063(entry, shared_state);
        return;
    }

    if (radio_si4063_state_change) {
        radio_si4063_state_change = false;
        pwm_timer_set_frequency(radio_si4063_freq);
        shared_state->radio_symbol_count_loop++;
    }
}

inline void radio_handle_data_timer_si4063()
{
    static int cw_symbol_rate_multiplier = CW_SYMBOL_RATE_MULTIPLIER;

    if (radio_current_transmit_entry->radio_type != RADIO_TYPE_SI4063 || !radio_shared_state.radio_interrupt_transmit_active) {
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
                si4063_set_direct_mode_pin(false);
                log_info("CW TX finished\n");
                radio_shared_state.radio_interrupt_transmit_active = false;
                radio_shared_state.radio_transmission_finished = true;
                system_enable_tick();
                break;
            }

            si4063_set_direct_mode_pin(tone_index == 0 ? false : true);

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
                log_info("Horus TX finished\n");
                radio_shared_state.radio_interrupt_transmit_active = false;
                radio_shared_state.radio_transmission_finished = true;
                system_enable_tick();
                break;
            }

            // NOTE: The factor of 23 will produce a tone spacing of about 270 Hz, which is the standard spacing for Horus 4FSK
            si4063_set_frequency_offset(tone_index * 23 + HORUS_FREQUENCY_OFFSET_SI4063);

            radio_shared_state.radio_symbol_count_interrupt++;
            break;
        }
        default:
            break;
    }
}

bool radio_stop_transmit_si4063(radio_transmit_entry *entry, radio_module_state *shared_state)
{
    bool use_direct_mode = false;

    switch (entry->data_mode) {
        case RADIO_DATA_MODE_CW:
        case RADIO_DATA_MODE_PIP:
            data_timer_uninit();
            spi_init();
            break;
        case RADIO_DATA_MODE_RTTY:
        case RADIO_DATA_MODE_HORUS_V1:
        case RADIO_DATA_MODE_HORUS_V2:
            data_timer_uninit();
            break;
        case RADIO_DATA_MODE_CATS:
            break;
        case RADIO_DATA_MODE_APRS_1200:
            use_direct_mode = true;
            break;
        default:
            break;
    }

    if (use_direct_mode) {
        pwm_timer_pwm_enable(false);
        pwm_timer_use(false);
        pwm_timer_uninit();
        spi_init();
    }

    si4063_inhibit_tx();

    switch (entry->data_mode) {
        case RADIO_DATA_MODE_CW:
        case RADIO_DATA_MODE_PIP:
            system_enable_tick();
            break;
        case RADIO_DATA_MODE_APRS_1200:
            break;
        case RADIO_DATA_MODE_HORUS_V1:
        case RADIO_DATA_MODE_HORUS_V2:
            system_enable_tick();
            break;
        case RADIO_DATA_MODE_CATS:
            break;
        default:
            break;
    }

    return true;
}


void radio_init_si4063()
{
}
#endif
