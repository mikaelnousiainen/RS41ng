#ifndef __SI4063_H
#define __SI4063_H

#include <stdint.h>
#include <stdbool.h>

typedef enum _si4063_modulation_type {
    SI4063_MODULATION_TYPE_CW = 0,
    SI4063_MODULATION_TYPE_OOK,
    SI4063_MODULATION_TYPE_FSK,
} si4063_modulation_type;

void si4063_enable_tx();
void si4063_inhibit_tx();
void si4063_disable_tx();
void si4063_use_direct_mode(bool use);
void si4063_set_tx_frequency(uint32_t frequency_hz);
void si4063_set_tx_power(uint8_t power);
void si4063_set_frequency_offset(uint16_t offset);
void si4063_set_frequency_deviation(uint32_t deviation);
void si4063_set_modulation_type(si4063_modulation_type type);
int32_t si4063_read_temperature_celsius_100();
void si4063_set_sdi_pin(bool high);
void si4063_use_sdi_pin(bool use);
void si4063_init();

#endif
