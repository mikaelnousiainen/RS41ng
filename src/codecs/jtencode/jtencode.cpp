#include <cstring>
#include <cstdlib>
#include <drivers/si5351/si5351.h>

#include "codecs/fsk/fsk.h"
#include "lib/JTEncode.h"

#include "jtencode.h"

// Some of the code is based on JTEncode examples:
//
// Simple JT65/JT9/JT4/FT8/WSPR/FSQ beacon for Arduino, with the Etherkit
// Si5351A Breakout Board, by Jason Milldrum NT7S.
//
// Transmit an abritrary message of up to 13 valid characters
// (a Type 6 message) in JT65, JT9, JT4, a type 0.0 or type 0.5 FT8 message,
// a FSQ message, or a standard Type 1 message in WSPR.
//
// Connect a momentary push button to pin 12 to use as the
// transmit trigger. Get fancy by adding your own code to trigger
// off of the time from a GPS or your PC via virtual serial.
//
// Original code based on Feld Hell beacon for Arduino by Mark
// Vandewettering K6HX, adapted for the Si5351A by Robert
// Liesenfeld AK6L <ak6l@ak6l.org>.
//
// Permission is hereby granted, free of charge, to any person obtaining
// a copy of this software and associated documentation files (the
// "Software"), to deal in the Software without restriction, including
// without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to
// permit persons to whom the Software is furnished to do so, subject
// to the following conditions:
//
// The above copyright notice and this permission notice shall be
// included in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
// IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR
// ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
// CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
// WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
//

// Mode defines
#define JTENCODE_TONE_SPACING_JT9        174          // ~1.74 Hz
#define JTENCODE_TONE_SPACING_JT65       269          // ~2.69 Hz
#define JTENCODE_TONE_SPACING_JT4        437          // ~4.37 Hz
#define JTENCODE_TONE_SPACING_WSPR       146          // ~1.46 Hz
#define JTENCODE_TONE_SPACING_FT8        625          // ~6.25 Hz
#define JTENCODE_TONE_SPACING_FSQ        879          // ~8.79 Hz

#define JTENCODE_TONE_DELAY_JT9               576 * 100          // Delay value for JT9-1
#define JTENCODE_TONE_DELAY_JT65              371 * 100          // Delay value for JT65A
#define JTENCODE_TONE_DELAY_JT4               229 * 100          // Delay value for JT4A
#define JTENCODE_TONE_DELAY_WSPR              683 * 100          // Delay value for WSPR
#define JTENCODE_TONE_DELAY_FT8               159 * 100          // Delay value for FT8
#define JTENCODE_TONE_DELAY_FSQ_2             500 * 100          // Delay value for 2 baud FSQ
#define JTENCODE_TONE_DELAY_FSQ_3             333 * 100          // Delay value for 3 baud FSQ
#define JTENCODE_TONE_DELAY_FSQ_4_5           222 * 100          // Delay value for 4.5 baud FSQ
#define JTENCODE_TONE_DELAY_FSQ_6             167 * 100          // Delay value for 6 baud FSQ

typedef struct _jtencode_mode {
    uint16_t symbol_count;
    uint32_t tone_delay_ms_100;
    uint32_t tone_spacing_hz_100;
} jtencode_mode_descriptor;

jtencode_mode_descriptor jtencode_modes[] = {
        {
                .symbol_count = JT9_SYMBOL_COUNT,
                .tone_delay_ms_100 = JTENCODE_TONE_DELAY_JT9,
                .tone_spacing_hz_100 = JTENCODE_TONE_SPACING_JT9,
        },
        {
                .symbol_count = JT65_SYMBOL_COUNT,
                .tone_delay_ms_100 = JTENCODE_TONE_DELAY_JT65,
                .tone_spacing_hz_100 = JTENCODE_TONE_SPACING_JT65,
        },
        {
                .symbol_count = JT4_SYMBOL_COUNT,
                .tone_delay_ms_100 = JTENCODE_TONE_DELAY_JT4,
                .tone_spacing_hz_100 = JTENCODE_TONE_SPACING_JT4,
        },
        {
                .symbol_count = WSPR_SYMBOL_COUNT,
                .tone_delay_ms_100 = JTENCODE_TONE_DELAY_WSPR,
                .tone_spacing_hz_100 = JTENCODE_TONE_SPACING_WSPR,
        },
        {
                .symbol_count = FT8_SYMBOL_COUNT,
                .tone_delay_ms_100 = JTENCODE_TONE_DELAY_FT8,
                .tone_spacing_hz_100 = JTENCODE_TONE_SPACING_FT8,
        },
        {
                .symbol_count = 0,
                .tone_delay_ms_100 = JTENCODE_TONE_DELAY_FSQ_2,
                .tone_spacing_hz_100 = JTENCODE_TONE_SPACING_FSQ,
        },
        {
                .symbol_count = 0,
                .tone_delay_ms_100 = JTENCODE_TONE_DELAY_FSQ_3,
                .tone_spacing_hz_100 = JTENCODE_TONE_SPACING_FSQ,
        },
        {
                .symbol_count = 0,
                .tone_delay_ms_100 = JTENCODE_TONE_DELAY_FSQ_4_5,
                .tone_spacing_hz_100 = JTENCODE_TONE_SPACING_FSQ,
        },
        {
                .symbol_count = 0,
                .tone_delay_ms_100 = JTENCODE_TONE_DELAY_FSQ_6,
                .tone_spacing_hz_100 = JTENCODE_TONE_SPACING_FSQ,
        },
};

typedef struct _jtencode_encoder {
    jtencode_mode_type mode_type;

    char *wspr_callsign;
    char *wspr_locator;
    uint8_t wspr_dbm;

    char *fsq_callsign_from;

    JTEncode *jtencode;
    uint16_t symbol_count;
    uint32_t tone_spacing;
    uint32_t tone_delay;

    size_t symbol_data_length;
    uint8_t *symbol_data;

    size_t current_byte_index;
} jtencode_encoder;

bool jtencode_encoder_new(fsk_encoder *encoder, size_t symbol_data_length, uint8_t *symbol_data,
        jtencode_mode_type mode_type, char *wspr_callsign, char *wspr_locator, uint8_t wspr_dbm,
        char *fsq_callsign_from)
{
    jtencode_mode_descriptor *mode_descriptor = &jtencode_modes[mode_type];
    if (mode_descriptor->symbol_count > 0) {
        if (mode_descriptor->symbol_count > symbol_data_length) {
            return false;
        }
    }

    encoder->priv = malloc(sizeof(jtencode_encoder));
    memset(encoder->priv, 0, sizeof(jtencode_encoder));

    auto *jte = (jtencode_encoder *) encoder->priv;
    jte->symbol_data_length = symbol_data_length;
    jte->symbol_data = symbol_data;

    jte->mode_type = mode_type;
    jte->symbol_count = mode_descriptor->symbol_count;
    jte->tone_spacing = mode_descriptor->tone_spacing_hz_100;
    jte->tone_delay = mode_descriptor->tone_delay_ms_100;

    jte->wspr_callsign = wspr_callsign;
    jte->wspr_locator = wspr_locator;
    jte->wspr_dbm = wspr_dbm;

    jte->fsq_callsign_from = fsq_callsign_from;

    jte->jtencode = new JTEncode();

    return true;
}

void jtencode_encoder_destroy(fsk_encoder *encoder)
{
    if (encoder->priv != nullptr) {
        auto *jte = (jtencode_encoder *) encoder->priv;
        delete jte->jtencode;
        free(encoder->priv);
        encoder->priv = nullptr;
    }
}

void jtencode_encoder_get_tones(fsk_encoder *encoder, int8_t *tone_count, fsk_tone **tones)
{
    *tone_count = 0;
    *tones = nullptr;
}

uint32_t jtencode_encoder_get_tone_spacing(fsk_encoder *encoder)
{
    auto *jte = (jtencode_encoder *) encoder->priv;
    return jte->tone_spacing;
}

uint32_t jtencode_encoder_get_symbol_rate(fsk_encoder *encoder)
{
    return 0;
}

uint32_t jtencode_encoder_get_symbol_delay(fsk_encoder *encoder)
{
    auto *jte = (jtencode_encoder *) encoder->priv;
    return jte->tone_delay;
}

void jtencode_encoder_set_data(fsk_encoder *encoder, uint16_t data_length, uint8_t *data)
{
    auto *jte = (jtencode_encoder *) encoder->priv;
    JTEncode *jtencode = jte->jtencode;
    uint8_t *symbol_data = jte->symbol_data;
    jtencode_mode_type mode_type = jte->mode_type;

    memset(symbol_data, 0, jte->symbol_data_length);

    switch (mode_type) {
        case JTENCODE_MODE_JT9:
            jtencode->jt9_encode((char *) data, symbol_data);
            break;
        case JTENCODE_MODE_JT65:
            jtencode->jt65_encode((char *) data, symbol_data);
            break;
        case JTENCODE_MODE_JT4:
            jtencode->jt4_encode((char *) data, symbol_data);
            break;
        case JTENCODE_MODE_WSPR:
            jtencode->wspr_encode(jte->wspr_callsign, jte->wspr_locator, jte->wspr_dbm, symbol_data);
            break;
        case JTENCODE_MODE_FT8:
            jtencode->ft8_encode((char *) data, symbol_data);
            break;
        case JTENCODE_MODE_FSQ_2:
        case JTENCODE_MODE_FSQ_3:
        case JTENCODE_MODE_FSQ_4_5:
        case JTENCODE_MODE_FSQ_6:
            jtencode->fsq_encode(jte->fsq_callsign_from, (const char *) data, symbol_data);

            uint8_t j = 0;
            while (symbol_data[j++] != 0xff);
            jte->symbol_count = j - 1;
            break;
    }

    jte->current_byte_index = 0;
}

int8_t jtencode_encoder_next_tone(fsk_encoder *encoder)
{
    auto *jte = (jtencode_encoder *) encoder->priv;

    size_t current_byte_index = jte->current_byte_index;
    if (current_byte_index >= jte->symbol_count) {
        return -1;
    }

    int8_t tone_index = jte->symbol_data[current_byte_index];
    jte->current_byte_index++;

    return tone_index;
}

fsk_encoder_api jtencode_fsk_encoder_api = {
        .get_tones = jtencode_encoder_get_tones,
        .get_tone_spacing = jtencode_encoder_get_tone_spacing,
        .get_symbol_rate = jtencode_encoder_get_symbol_rate,
        .get_symbol_delay = jtencode_encoder_get_symbol_delay,
        .set_data = jtencode_encoder_set_data,
        .next_tone = jtencode_encoder_next_tone,
};
