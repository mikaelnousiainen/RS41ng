#include <stdbool.h>
#include <stdlib.h>

#include "codecs/ax25/ax25.h"
#include "bell.h"

#define FSK_TONE_INDEX_BELL_SPACE 0
#define FSK_TONE_INDEX_BELL_MARK 1

#define BELL_TONE_COUNT 2

typedef struct _bell_encoder {
    uint32_t symbol_rate;
    uint16_t flag_field_count;
    fsk_tone *tones;

    uint16_t data_length;
    uint8_t *data;

    uint16_t current_byte_index;
    uint8_t current_bit_index;
    uint8_t current_byte;
    uint8_t bit_stuffing_counter;

    uint16_t current_flag_field_count;
    int8_t current_tone_index;
} bell_encoder;

fsk_tone bell202_tones[] = {
        {
                .index = FSK_TONE_INDEX_BELL_SPACE,
                .frequency_hz_100 = 2200 * 100,
        },
        {
                .index = FSK_TONE_INDEX_BELL_MARK,
                .frequency_hz_100 = 1200 * 100,
        },
};

fsk_tone bell103_tones[] = {
        {
                .index = FSK_TONE_INDEX_BELL_SPACE,
                .frequency_hz_100 = 1070 * 100,
        },
        {
                .index = FSK_TONE_INDEX_BELL_MARK,
                .frequency_hz_100 = 1270 * 100,
        },
};

void bell_encoder_new(fsk_encoder *encoder, uint32_t symbol_rate, uint16_t flag_field_count, fsk_tone *tones)
{
    encoder->priv = malloc(sizeof(bell_encoder));
    memset(encoder->priv, 0, sizeof(bell_encoder));

    bell_encoder *bell = (bell_encoder *) encoder->priv;
    bell->symbol_rate = symbol_rate;
    bell->flag_field_count = flag_field_count;
    bell->tones = tones;
}

void bell_encoder_destroy(fsk_encoder *encoder)
{
    if (encoder->priv != NULL) {
        free(encoder->priv);
        encoder->priv = NULL;
    }
}

void bell_encoder_set_data(fsk_encoder *encoder, uint16_t data_length, uint8_t *data)
{
    bell_encoder *bell = (bell_encoder *) encoder->priv;

    bell->data = data;
    bell->data_length = data_length;

    bell->current_tone_index = FSK_TONE_INDEX_BELL_MARK;
    bell->current_byte_index = 0;
    bell->current_bit_index = 0;
    bell->current_byte = data[0];
    bell->bit_stuffing_counter = 0;
    bell->current_flag_field_count = 0;
}

void bell_encoder_get_tones(fsk_encoder *encoder, int8_t *tone_count, fsk_tone **tones)
{
    bell_encoder *bell = (bell_encoder *) encoder->priv;

    *tone_count = BELL_TONE_COUNT;
    *tones = bell->tones;
}

uint32_t bell_encoder_get_tone_spacing(fsk_encoder *encoder)
{
    return 0;
}

uint32_t bell_encoder_get_symbol_rate(fsk_encoder *encoder)
{
    bell_encoder *bell = (bell_encoder *) encoder->priv;
    return bell->symbol_rate;
}

uint32_t bell_encoder_get_symbol_delay(fsk_encoder *encoder)
{
    return 0;
}

static inline void bell_encoder_toggle_tone(fsk_encoder *encoder)
{
    bell_encoder *bell = (bell_encoder *) encoder->priv;

    bell->current_tone_index =
            (bell->current_tone_index == FSK_TONE_INDEX_BELL_MARK)
            ? FSK_TONE_INDEX_BELL_SPACE
            : FSK_TONE_INDEX_BELL_MARK;
}

int8_t bell_encoder_next_tone(fsk_encoder *encoder)
{
    bell_encoder *bell = (bell_encoder *) encoder->priv;

    if (bell->current_byte_index >= bell->data_length) {
        return -1;
    }

    bool is_flag = bell->current_byte == AX25_PACKET_FLAG;

    if (is_flag) {
        bell->bit_stuffing_counter = 0;
    }

    bool bit = (bell->current_byte >> bell->current_bit_index) & 1U;

    if (bit) {
        bell->bit_stuffing_counter++;
        if (bell->bit_stuffing_counter == 5) {
            int8_t previous_tone_index = bell->current_tone_index;
            bell_encoder_toggle_tone(encoder);
            bell->bit_stuffing_counter = 0;
            return previous_tone_index;
        }
    } else {
        bell_encoder_toggle_tone(encoder);
        bell->bit_stuffing_counter = 0;
    }

    bell->current_bit_index = (bell->current_bit_index + 1) % 8;

    if (bell->current_bit_index == 0) {
        if (bell->current_byte_index == 0 && bell->current_flag_field_count < bell->flag_field_count) {
            bell->current_flag_field_count++;
        } else {
            // return -1;
            bell->current_byte_index++;
        }
        if (bell->current_byte_index < bell->data_length) {
            bell->current_byte = bell->data[bell->current_byte_index];
        }
    }

    return bell->current_tone_index;
}

fsk_encoder_api bell_fsk_encoder_api = {
        .get_tones = bell_encoder_get_tones,
        .get_tone_spacing = bell_encoder_get_tone_spacing,
        .get_symbol_rate = bell_encoder_get_symbol_rate,
        .get_symbol_delay = bell_encoder_get_symbol_delay,
        .set_data = bell_encoder_set_data,
        .next_tone = bell_encoder_next_tone,
};
