#ifndef __RADIO_SI4032_H
#define __RADIO_SI4032_H

#include "radio_internal.h"

bool radio_start_transmit_si4032(radio_transmit_entry *entry, radio_module_state *shared_state);
bool radio_transmit_symbol_si4032(radio_transmit_entry *entry, radio_module_state *shared_state);
void radio_handle_main_loop_si4032(radio_transmit_entry *entry, radio_module_state *shared_state);
void radio_handle_data_timer_si4032();
bool radio_stop_transmit_si4032(radio_transmit_entry *entry, radio_module_state *shared_state);
void radio_init_si4032();

#endif
