#ifndef __MORSE_H
#define __MORSE_H

#include <stdint.h>

#include "codecs/fsk/fsk.h"

void morse_encoder_new(fsk_encoder *encoder, uint32_t symbol_rate);
void morse_encoder_destroy(fsk_encoder *encoder);
void morse_encoder_set_data(fsk_encoder *encoder, uint16_t data_length, uint8_t *data);
void morse_encoder_get_tones(fsk_encoder *encoder, int8_t *tone_count, fsk_tone **tones);
uint32_t morse_encoder_get_symbol_rate(fsk_encoder *encoder);
uint32_t morse_encoder_get_symbol_delay(fsk_encoder *encoder);
int8_t morse_encoder_next_tone(fsk_encoder *encoder);

extern fsk_encoder_api morse_fsk_encoder_api;

#endif
