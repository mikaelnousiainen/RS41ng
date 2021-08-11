#ifndef __BELL_H
#define __BELL_H

#include <stdint.h>
#include <string.h>

#include "codecs/fsk/fsk.h"

#define BELL_FLAG_FIELD_COUNT_1200 45
#define BELL_FLAG_FIELD_COUNT_300 45

void bell_encoder_new(fsk_encoder *encoder, uint32_t symbol_rate, uint16_t flag_field_count, fsk_tone *tones);
void bell_encoder_destroy(fsk_encoder *encoder);
void bell_encoder_set_data(fsk_encoder *encoder, uint16_t data_length, uint8_t *data);
void bell_encoder_get_tones(fsk_encoder *encoder, int8_t *tone_count, fsk_tone **tones);
uint32_t bell_encoder_get_symbol_rate(fsk_encoder *encoder);
int8_t bell_encoder_next_tone(fsk_encoder *encoder);

extern fsk_tone bell202_tones[];
extern fsk_tone bell103_tones[];
extern fsk_encoder_api bell_fsk_encoder_api;

#endif
