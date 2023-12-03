#ifndef __RADIO_H
#define __RADIO_H

void radio_init();
void radio_handle_timer_tick();
void radio_handle_data_timer_tick();
void radio_handle_main_loop();

void radio_reset_tx_watchdog_counter ();
uint32_t radio_Inc_tx_watchdog_counter ();

#endif
