#ifndef __MFSK_H
#define __MFSK_H

#include <stdint.h>

#include "codecs/fsk/fsk.h"

typedef enum _mfsk_type {
    MFSK_2 = 2,
    MFSK_4 = 4,
    MFSK_16 = 16,
} mfsk_type;

void mfsk_encoder_new(fsk_encoder *encoder, mfsk_type type, uint32_t symbol_rate, uint32_t tone_spacing_hz_100);
void mfsk_encoder_destroy(fsk_encoder *encoder);
fsk_tone *mfsk_get_idle_tone(fsk_encoder *encoder);
void mfsk_encoder_set_data(fsk_encoder *encoder, uint16_t data_length, uint8_t *data);
void mfsk_encoder_get_tones(fsk_encoder *encoder, int8_t *tone_count, fsk_tone **tones);
uint32_t mfsk_encoder_get_tone_spacing(fsk_encoder *encoder);
uint32_t mfsk_encoder_get_symbol_rate(fsk_encoder *encoder);
uint32_t mfsk_encoder_get_symbol_delay(fsk_encoder *encoder);
int8_t mfsk_encoder_next_tone(fsk_encoder *encoder);

extern fsk_encoder_api mfsk_fsk_encoder_api;

#endif
