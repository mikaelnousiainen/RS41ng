#include <stdlib.h>
#include <string.h>

#include "mfsk.h"

typedef struct _mfsk_encoder {
    mfsk_type type;
    uint32_t symbol_rate;
    uint32_t tone_spacing_hz_100;
    uint8_t max_nibble_index;

    uint16_t data_length;
    uint8_t *data;

    uint16_t current_byte_index;
    uint8_t current_nibble_index;
    uint8_t current_byte;

    int8_t idle_tone_index;
    int8_t current_tone_index;

    fsk_tone tones[16];
} mfsk_encoder;

#define MFSK_2_IDLE_TONE 0
#define MFSK_4_IDLE_TONE 3
#define MFSK_16_IDLE_TONE 3

void mfsk_encoder_new(fsk_encoder *encoder, mfsk_type type, uint32_t symbol_rate, uint32_t tone_spacing_hz_100)
{
    encoder->priv = malloc(sizeof(mfsk_encoder));
    memset(encoder->priv, 0, sizeof(mfsk_encoder));

    mfsk_encoder *mfsk = (mfsk_encoder *) encoder->priv;
    mfsk->type = type;
    mfsk->symbol_rate = symbol_rate;
    mfsk->tone_spacing_hz_100 = tone_spacing_hz_100;

    switch (mfsk->type) {
        case MFSK_2:
            mfsk->idle_tone_index = MFSK_2_IDLE_TONE;
            mfsk->max_nibble_index = 8;
            break;
        case MFSK_4:
            mfsk->idle_tone_index = MFSK_4_IDLE_TONE;
            mfsk->max_nibble_index = 4;
            break;
        case MFSK_16:
            mfsk->idle_tone_index = MFSK_16_IDLE_TONE;
            mfsk->max_nibble_index = 2;
            break;
        default:
            break;
    }

    for (int i = 0; i < type; i++) {
        mfsk->tones[i].index = (int8_t) i;
        mfsk->tones[i].frequency_hz_100 = i * tone_spacing_hz_100;
    }
}

void mfsk_encoder_destroy(fsk_encoder *encoder)
{
    if (encoder->priv != NULL) {
        free(encoder->priv);
        encoder->priv = NULL;
    }
}

fsk_tone *mfsk_get_idle_tone(fsk_encoder *encoder)
{
    mfsk_encoder *mfsk = (mfsk_encoder *) encoder->priv;
    return &mfsk->tones[mfsk->idle_tone_index];
}

void mfsk_encoder_set_data(fsk_encoder *encoder, uint16_t data_length, uint8_t *data)
{
    mfsk_encoder *mfsk = (mfsk_encoder *) encoder->priv;

    mfsk->data = data;
    mfsk->data_length = data_length;

    mfsk->current_tone_index = mfsk->idle_tone_index;
    mfsk->current_byte_index = 0;
    mfsk->current_nibble_index = 0;
    mfsk->current_byte = data[0];
}

void mfsk_encoder_get_tones(fsk_encoder *encoder, int8_t *tone_count, fsk_tone **tones)
{
    mfsk_encoder *mfsk = (mfsk_encoder *) encoder->priv;

    *tone_count = mfsk->type;
    *tones = mfsk->tones;
}

uint32_t mfsk_encoder_get_tone_spacing(fsk_encoder *encoder)
{
    mfsk_encoder *mfsk = (mfsk_encoder *) encoder->priv;
    return mfsk->tone_spacing_hz_100;
}

uint32_t mfsk_encoder_get_symbol_rate(fsk_encoder *encoder)
{
    mfsk_encoder *mfsk = (mfsk_encoder *) encoder->priv;
    return mfsk->symbol_rate;
}

uint32_t mfsk_encoder_get_symbol_delay(fsk_encoder *encoder)
{
    return 0;
}

int8_t mfsk_encoder_next_tone(fsk_encoder *encoder)
{
    mfsk_encoder *mfsk = (mfsk_encoder *) encoder->priv;

    if (mfsk->current_byte_index >= mfsk->data_length) {
        return -1;
    }

    if (mfsk->current_nibble_index == mfsk->max_nibble_index) {
        // Reached the end of the byte
        mfsk->current_nibble_index = 0;
        mfsk->current_byte_index++;
        if (mfsk->current_byte_index >= mfsk->data_length) {
            return -1;
        }
        mfsk->current_byte = mfsk->data[mfsk->current_byte_index];
    }

    int8_t symbol;

    switch (mfsk->type) {
        case MFSK_2:
            // Get the current symbol (the MSB).
            if (((uint8_t) mfsk->current_byte & 0x80) >> 7) {
                symbol = 2;
            } else {
                symbol = 0;
            }

            // Shift it left to the nibble we are up to.
            mfsk->current_byte = mfsk->current_byte << 1;
            break;
        case MFSK_4:
            // Get the current symbol (the 2 bits we need).
            symbol = (int8_t) ((mfsk->current_byte & 0xC0) >> 6);

            // Shift it left to the nibble we are up to.
            mfsk->current_byte = mfsk->current_byte << 2;
            break;
        case MFSK_16:
            // Get the current symbol (the 4 bits we need).
            symbol = (int8_t) ((mfsk->current_byte & 0xF0) >> 4);

            // Shift it left to the nibble we are up to.
            mfsk->current_byte = mfsk->current_byte << 4;
            break;
        default:
            symbol = 0;
            break;
    }

    mfsk->current_nibble_index++;

    return symbol;
}

fsk_encoder_api mfsk_fsk_encoder_api = {
        .get_tones = mfsk_encoder_get_tones,
        .get_tone_spacing = mfsk_encoder_get_tone_spacing,
        .get_symbol_rate = mfsk_encoder_get_symbol_rate,
        .get_symbol_delay = mfsk_encoder_get_symbol_delay,
        .set_data = mfsk_encoder_set_data,
        .next_tone = mfsk_encoder_next_tone,
};

#ifdef TEST
int mfsk_test_bits(char *buffer)
{
    // Add some test bits into the MFSK buffer, compatible with the first 200 test bits produced by fsk_get_test_bits from codec2-dev.
    // Decode using (with GQRX outputting data to UDP): nc -l -u localhost 7355 | ./fsk_demod 4 48000 100 - - | ./fsk_put_test_bits -
    sprintf(buffer,
            "\x1b\x1b\x1b\x1b\xd8\x2c\xa2\x56\x3f\x4c\x08\xa2\xba\x6f\x84\x07\x6d\x82\xca\x25\x63\xf4\xc0\x8a\x2b\xa6\xf8\x40\x76");
    return 29;
}
#endif
