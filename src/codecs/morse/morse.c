#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "morse.h"

// Morse code definitions adapted from code by Mark Jessop VK5QI and OK1TE, also based on
// https://github.com/Paradoxis/Arduino-morse-code-translator/blob/master/main.ino

#define MORSE_UNITS_DOT 1
#define MORSE_UNITS_DASH 3
#define MORSE_UNITS_GAP 3
#define MORSE_UNITS_SPACE 7

const char MORSE_DOT = '.';
const char MORSE_DASH = '-';
const char MORSE_SPACE = ' ';

const char *const morse_letters[] = {
        ".-",     // A
        "-...",   // B
        "-.-.",   // C
        "-..",    // D
        ".",      // E
        "..-.",   // F
        "--.",    // G
        "....",   // H
        "..",     // I
        ".---",   // J
        "-.-",    // K
        ".-..",   // L
        "--",     // M
        "-.",     // N
        "---",    // O
        ".--.",   // P
        "--.-",   // Q
        ".-.",    // R
        "...",    // S
        "-",      // T
        "..-",    // U
        "...-",   // V
        ".--",    // W
        "-..-",   // X
        "-.--",   // Y
        "--.."    // Z
};

const char *const morse_numbers[] = {
        "-----",   // 0
        ".----",   // 1
        "..---",   // 2
        "...--",   // 3
        "....-",   // 4
        ".....",   // 5
        "-....",   // 6
        "--...",   // 7
        "---..",   // 8
        "----."    // 9
};

const char morse_stroke[] = "-..-.";
const char morse_equal[] = "-...-";
const char morse_full_stop[] = ".-.-.-";
const char morse_comma[] = "--..--";
const char morse_question_mark[] = "..--..";
const char morse_plus[] = ".-.-.";
const char morse_at_sign[] = ".--.-.";
const char morse_space[] = " ";

static const char *morse_get_sequence(char c)
{
    if (c >= 'A' && c <= 'Z') { // Uppercase letters
        return morse_letters[c - 'A'];
    } else if (c >= 'a' && c <= 'z') { // Lowercase letters
        return morse_letters[c - 'a'];
    } else if (c >= '0' && c <= '9') { // Numbers
        return morse_numbers[c - '0'];
    }

    switch (c) {
        case '/':
            return morse_stroke;
        case '=':
            return morse_equal;
        case '.':
            return morse_full_stop;
        case ',':
            return morse_comma;
        case '?':
            return morse_question_mark;
        case '+':
            return morse_plus;
        case '@':
            return morse_at_sign;
        default:
            // Treat all other characters as a space
            return morse_space;
    }
}

typedef struct _morse_encoder {
    uint32_t symbol_rate;

    uint16_t data_length;
    uint8_t *data;

    uint16_t current_byte_index;

    const char *current_sequence;
    uint8_t current_sequence_index;

    bool tone_active;
    uint8_t units_left;

    bool start;

    fsk_tone tones[2];
} morse_encoder;

void morse_encoder_new(fsk_encoder *encoder, uint32_t symbol_rate)
{
    encoder->priv = malloc(sizeof(morse_encoder));
    memset(encoder->priv, 0, sizeof(morse_encoder));

    morse_encoder *morse = (morse_encoder *) encoder->priv;
    morse->symbol_rate = symbol_rate;

    for (int i = 0; i < 2; i++) {
        morse->tones[i].index = (int8_t) i;
        morse->tones[i].frequency_hz_100 = i;
    }
}

void morse_encoder_destroy(fsk_encoder *encoder)
{
    if (encoder->priv != NULL) {
        free(encoder->priv);
        encoder->priv = NULL;
    }
}

void morse_encoder_set_data(fsk_encoder *encoder, uint16_t data_length, uint8_t *data)
{
    morse_encoder *morse = (morse_encoder *) encoder->priv;

    morse->data = data;
    morse->data_length = data_length;

    morse->current_byte_index = 0;
    morse->current_sequence = morse_get_sequence((char) data[0]);
    morse->current_sequence_index = 0;
    morse->tone_active = false;
    morse->units_left = 0;
    morse->start = true;
}

void morse_encoder_get_tones(fsk_encoder *encoder, int8_t *tone_count, fsk_tone **tones)
{
    morse_encoder *morse = (morse_encoder *) encoder->priv;

    *tone_count = 2;
    *tones = morse->tones;
}

uint32_t morse_encoder_get_tone_spacing(fsk_encoder *encoder)
{
    return 0;
}

uint32_t morse_encoder_get_symbol_rate(fsk_encoder *encoder)
{
    morse_encoder *morse = (morse_encoder *) encoder->priv;
    return morse->symbol_rate;
}

uint32_t morse_encoder_get_symbol_delay(fsk_encoder *encoder)
{
    return 0;
}

int8_t morse_encoder_next_tone(fsk_encoder *encoder)
{
    morse_encoder *morse = (morse_encoder *) encoder->priv;

    if (morse->current_byte_index >= morse->data_length) {
        return -1;
    }

    if (!morse->start && morse->units_left == 0) {
        char next_element = morse->current_sequence[morse->current_sequence_index + 1];
        if (morse->tone_active) {
            if (next_element == 0) {
                bool char_gap = true;

                if (morse->current_byte_index + 1 < morse->data_length) {
                    const char *next_byte_sequence = morse_get_sequence((char) morse->data[morse->current_byte_index + 1]);
                    if (next_byte_sequence[0] == MORSE_SPACE) {
                        char_gap = false;
                    }
                } else {
                    char_gap = false;
                }

                if (char_gap) {
                    // Char gap
                    morse->tone_active = false;
                    morse->units_left = MORSE_UNITS_GAP - 1;
                    return 0;
                }
            } else {
                // Unit gap
                morse->tone_active = false;
                return 0;
            }
        }

        morse->current_sequence_index++;

        if (next_element == 0) {
            morse->current_byte_index++;

            if (morse->current_byte_index >= morse->data_length) {
                return -1;
            }

            morse->current_sequence = morse_get_sequence((char) morse->data[morse->current_byte_index]);
            morse->current_sequence_index = 0;

            morse->tone_active = false;
        }
    }

    morse->start = false;

    char element = morse->current_sequence[morse->current_sequence_index];

    if (morse->units_left > 0) {
        morse->units_left--;
        return morse->tone_active ? 1 : 0;
    }

    if (element == MORSE_DOT) {
        morse->units_left = MORSE_UNITS_DOT;
        morse->tone_active = true;
    } else if (element == MORSE_DASH) {
        morse->units_left = MORSE_UNITS_DASH;
        morse->tone_active = true;
    } else {
        morse->units_left = MORSE_UNITS_SPACE;
        morse->tone_active = false;
    }

    morse->units_left--;

    return morse->tone_active ? 1 : 0;
}

fsk_encoder_api morse_fsk_encoder_api = {
        .get_tones = morse_encoder_get_tones,
        .get_tone_spacing = morse_encoder_get_tone_spacing,
        .get_symbol_rate = morse_encoder_get_symbol_rate,
        .get_symbol_delay = morse_encoder_get_symbol_delay,
        .set_data = morse_encoder_set_data,
        .next_tone = morse_encoder_next_tone,
};
