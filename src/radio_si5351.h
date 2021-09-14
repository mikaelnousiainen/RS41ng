#ifndef __RADIO_SI5351_H
#define __RADIO_SI5351_H

#include "radio_internal.h"

bool radio_start_transmit_si5351(radio_transmit_entry *entry, radio_module_state *shared_state);
bool radio_transmit_symbol_si5351(radio_transmit_entry *entry, radio_module_state *shared_state);
void radio_handle_main_loop_si5351(radio_transmit_entry *entry, radio_module_state *shared_state);
void radio_handle_data_timer_si5351();
bool radio_stop_transmit_si5351(radio_transmit_entry *entry, radio_module_state *shared_state);

#endif
