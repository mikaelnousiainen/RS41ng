#ifndef __RADIO_SI4063_H
#define __RADIO_SI4063_H
#ifdef DFM17
#include "radio_internal.h"

bool radio_start_transmit_si4063(radio_transmit_entry *entry, radio_module_state *shared_state);
bool radio_transmit_symbol_si4063(radio_transmit_entry *entry, radio_module_state *shared_state);
void radio_handle_main_loop_si4063(radio_transmit_entry *entry, radio_module_state *shared_state);
void radio_handle_data_timer_si4063();
bool radio_stop_transmit_si4063(radio_transmit_entry *entry, radio_module_state *shared_state);
void radio_init_si4063();
#endif
#endif
