#ifndef __SI4032_H
#define __SI4032_h

#include <stdint.h>
#include <stdbool.h>

typedef enum _si4032_modulation_type {
    SI4032_MODULATION_TYPE_NONE = 0,
    SI4032_MODULATION_TYPE_OOK,
    SI4032_MODULATION_TYPE_FSK,
} si4032_modulation_type;

void si4032_soft_reset();
void si4032_enable_tx();
void si4032_inhibit_tx();
void si4032_disable_tx();
void si4032_use_direct_mode(bool use);
void si4032_set_tx_frequency(float frequency_mhz);
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
