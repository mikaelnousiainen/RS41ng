#ifndef __SI4032_H
#define __SI4032_H

#include <stdint.h>
#include <stdbool.h>

typedef enum _si4032_modulation_type {
    SI4032_MODULATION_TYPE_NONE = 0,
    SI4032_MODULATION_TYPE_OOK,
    SI4032_MODULATION_TYPE_FSK,
    SI4032_MODULATION_TYPE_FIFO_FSK,
} si4032_modulation_type;

void si4032_soft_reset();
void si4032_enable_tx();
void si4032_inhibit_tx();
void si4032_disable_tx();
uint16_t si4032_start_tx(uint8_t *data, int len);
uint16_t si4032_refill_buffer(uint8_t *data, int len, bool *overflow);
int si4032_wait_for_tx_complete(int timeout_ms);
void si4032_use_direct_mode(bool use);
void si4032_set_tx_frequency(float frequency_mhz);
void si4032_set_data_rate(const uint32_t rate_bps);
void si4032_set_tx_power(uint8_t power);
void si4032_set_frequency_offset(uint16_t offset);
void si4032_set_frequency_offset_small(uint8_t offset);
void si4032_set_frequency_deviation(uint8_t deviation);
void si4032_set_modulation_type(si4032_modulation_type type);
int32_t si4032_read_temperature_celsius_100();
void si4032_set_sdi_pin(bool high);
void si4032_use_sdi_pin(bool use);
void si4032_init();

#endif
