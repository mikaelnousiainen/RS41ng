#include <stdint.h>

#include "hal/system.h"
#include "hal/datatimer.h"
#include "si5351_handler.h"
#include "radio_si5351.h"
#include "log.h"

#define CW_SYMBOL_RATE_MULTIPLIER 4

static bool use_fast_si5351 = false;

static volatile bool radio_si5351_state_change = false;
static volatile uint64_t radio_si5351_freq = 0;
static volatile bool radio_si5351_frequency_not_set = false;

bool radio_start_transmit_si5351(radio_transmit_entry *entry, radio_module_state *shared_state)
{
    bool set_frequency_early = true;

    radio_si5351_frequency_not_set = false;

    switch (entry->data_mode) {
        case RADIO_DATA_MODE_CW:
        case RADIO_DATA_MODE_PIP:
            data_timer_init(entry->symbol_rate * CW_SYMBOL_RATE_MULTIPLIER);
            set_frequency_early = false;
            break;
        case RADIO_DATA_MODE_HORUS_V1:
            data_timer_init(entry->fsk_encoder_api->get_symbol_rate(&entry->fsk_encoder));
            //use_fast_si5351 = true;
            break;
        default:
            break;
    }

    // TODO: handle Si5351 errors
    if (use_fast_si5351) {
        si5351_set_drive_strength_fast(SI5351_CLOCK_CLK0, entry->tx_power);
        if (set_frequency_early) {
            si5351_set_frequency_fast(SI5351_CLOCK_CLK0, ((uint64_t) entry->frequency) * 100ULL);
            si5351_output_enable_fast(SI5351_CLOCK_CLK0, true);
        }
    } else {
        si5351_set_drive_strength(SI5351_CLOCK_CLK0, entry->tx_power);
        if (set_frequency_early) {
            si5351_set_frequency(SI5351_CLOCK_CLK0, ((uint64_t) entry->frequency) * 100ULL);
            si5351_output_enable(SI5351_CLOCK_CLK0, true);
        }
    }

    switch (entry->data_mode) {
        case RADIO_DATA_MODE_CW:
        case RADIO_DATA_MODE_PIP:
            system_disable_tick();
            shared_state->radio_interrupt_transmit_active = true;
            // Setting the frequency turns on the output
            radio_si5351_frequency_not_set = true;
            break;
        case RADIO_DATA_MODE_HORUS_V1:
            system_disable_tick();
            shared_state->radio_interrupt_transmit_active = true;
            break;
        default:
            shared_state->radio_interrupt_transmit_active = false;
    }

    return true;
}

bool radio_transmit_symbol_si5351(radio_transmit_entry *entry, radio_module_state *shared_state)
{
    switch (entry->data_mode) {
        case RADIO_DATA_MODE_CW:
        case RADIO_DATA_MODE_PIP:
            return false;
        case RADIO_DATA_MODE_HORUS_V1:
            return false;
        default: {
            int8_t next_tone_index = entry->fsk_encoder_api->next_tone(&entry->fsk_encoder);
            if (next_tone_index < 0) {
                return false;
            }

            log_trace("Tone: %d\n", next_tone_index);

            uint64_t frequency =
                    ((uint64_t) entry->frequency) * 100UL + (next_tone_index * shared_state->radio_current_tone_spacing_hz_100);
            radio_si5351_freq = frequency;
            radio_si5351_state_change = true;
            break;
        }
    }

    return true;
}

void radio_handle_main_loop_si5351(radio_transmit_entry *entry, radio_module_state *shared_state)
{
    if (entry->radio_type != RADIO_TYPE_SI5351 || shared_state->radio_interrupt_transmit_active) {
        return;
    }

    // TODO: handle Si5351 errors
    if (radio_si5351_state_change) {
        radio_si5351_state_change = false;
        si5351_set_frequency(SI5351_CLOCK_CLK0, radio_si5351_freq);
    }
}

inline void radio_handle_data_timer_si5351()
{
    static int cw_symbol_rate_multiplier = CW_SYMBOL_RATE_MULTIPLIER;

    if (radio_current_transmit_entry->radio_type != RADIO_TYPE_SI5351 || !radio_shared_state.radio_interrupt_transmit_active) {
        return;
    }

    // TODO: handle Si5351 errors
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
                si5351_output_enable(SI5351_CLOCK_CLK0, false);
                log_info("CW TX finished\n");
                radio_shared_state.radio_interrupt_transmit_active = false;
                radio_shared_state.radio_transmission_finished = true;
                system_enable_tick();
                break;
            }

            bool enable = tone_index != 0;

            if (enable && radio_si5351_frequency_not_set) {
                si5351_set_frequency(SI5351_CLOCK_CLK0, ((uint64_t) radio_current_transmit_entry->frequency) * 100ULL);
                radio_si5351_frequency_not_set = false;
            }
            si5351_output_enable(SI5351_CLOCK_CLK0, enable);

            radio_shared_state.radio_symbol_count_interrupt++;
            break;
        }
        case RADIO_DATA_MODE_HORUS_V1: {
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

            uint64_t frequency =
                    ((uint64_t) radio_current_transmit_entry->frequency) * 100UL + (tone_index * radio_shared_state.radio_current_tone_spacing_hz_100);

            si5351_set_frequency(SI5351_CLOCK_CLK0, frequency);

            radio_shared_state.radio_symbol_count_interrupt++;
            break;
        }
        default:
            break;
    }
}

bool radio_stop_transmit_si5351(radio_transmit_entry *entry, radio_module_state *shared_state)
{
    switch (entry->data_mode) {
        case RADIO_DATA_MODE_CW:
        case RADIO_DATA_MODE_PIP:
            break;
        case RADIO_DATA_MODE_HORUS_V1:
            // use_fast_si5351 = true;
            break;
        default:
            break;
    }

    if (use_fast_si5351) {
        si5351_output_enable_fast(SI5351_CLOCK_CLK0, false);
    } else {
        si5351_output_enable(SI5351_CLOCK_CLK0, false);
    }

    switch (entry->data_mode) {
        case RADIO_DATA_MODE_CW:
        case RADIO_DATA_MODE_PIP:
            data_timer_uninit();
            system_enable_tick();
            break;
        case RADIO_DATA_MODE_HORUS_V1:
            data_timer_uninit();
            system_enable_tick();
            break;
        default:
            break;
    }

    return true;
}
