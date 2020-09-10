#include <stdint.h>

#include "si5351_handler.h"
#include "radio_si5351.h"
#include "log.h"

static volatile bool radio_si5351_state_change = false;
static volatile uint64_t radio_si5351_freq = 0;

bool radio_start_transmit_si5351(radio_transmit_entry *entry, radio_module_state *shared_state)
{
    si5351_set_drive_strength(SI5351_CLOCK_CLK0, entry->tx_power);
    si5351_set_frequency(SI5351_CLOCK_CLK0, ((uint64_t) entry->frequency) * 100ULL);
    si5351_output_enable(SI5351_CLOCK_CLK0, true);
    return true;
}

bool radio_transmit_symbol_si5351(radio_transmit_entry *entry, radio_module_state *shared_state)
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
    if (entry->radio_type != RADIO_TYPE_SI5351) {
        return;
    }

    if (radio_si5351_state_change) {
        radio_si5351_state_change = false;
        si5351_set_frequency(SI5351_CLOCK_CLK0, radio_si5351_freq);
    }
}

bool radio_stop_transmit_si5351(radio_transmit_entry *entry, radio_module_state *shared_state)
{
    si5351_output_enable(SI5351_CLOCK_CLK0, false);
    return true;
}
