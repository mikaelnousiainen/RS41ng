#include <string.h>
#include <stdbool.h>

#include "aprs_9600.h"
#include "codecs/ax25/ax25.h"

#define G3RUH_PREAMBLE_FLAGS 64

typedef struct {
    uint8_t *output;
    uint16_t max_length;
    uint16_t byte_index;
    uint8_t bit_index;
    uint32_t scrambler_state;
    uint8_t nrzi_state;
} g3ruh_state;

static void g3ruh_init(g3ruh_state *state, uint8_t *output, uint16_t max_length)
{
    memset(state, 0, sizeof(g3ruh_state));
    state->output = output;
    state->max_length = max_length;
    state->bit_index = 7;
    memset(output, 0, max_length);
}

/**
 * Emit one bit through the G3RUH scrambler into the output buffer.
 *
 * G3RUH uses a self-synchronizing scrambler with polynomial x^17 + x^12 + 1:
 *   out[n] = in[n] XOR out[n-12] XOR out[n-17]
 *
 * Output bits are packed MSB-first into bytes for the SI4032 FIFO.
 */
static void g3ruh_emit_bit(g3ruh_state *state, uint8_t bit)
{
    if (state->byte_index >= state->max_length) {
        return;
    }

    uint8_t tap12 = (state->scrambler_state >> 11) & 1;
    uint8_t tap17 = (state->scrambler_state >> 16) & 1;
    uint8_t scrambled = (bit ^ tap12 ^ tap17) & 1;
    state->scrambler_state = (state->scrambler_state << 1) | scrambled;

    // NRZI encode: 0 bit = toggle, 1 bit = no change
    if (!scrambled) {
        state->nrzi_state ^= 1;
    }

    if (state->nrzi_state) {
        state->output[state->byte_index] |= (1U << state->bit_index);
    }

    if (state->bit_index == 0) {
        state->bit_index = 7;
        state->byte_index++;
    } else {
        state->bit_index--;
    }
}

/**
 * Emit a byte without bit stuffing, LSB first (AX.25 bit order).
 * Used for flag bytes (0x7E).
 */
static void g3ruh_emit_byte_raw(g3ruh_state *state, uint8_t byte)
{
    for (uint8_t i = 0; i < 8; i++) {
        g3ruh_emit_bit(state, (byte >> i) & 1);
    }
}

/**
 * Emit a byte with bit stuffing, LSB first.
 * Inserts a 0 bit after every 5 consecutive 1 bits.
 * Returns updated consecutive-ones count.
 */
static uint8_t g3ruh_emit_byte_stuffed(g3ruh_state *state, uint8_t byte, uint8_t ones_count)
{
    for (uint8_t i = 0; i < 8; i++) {
        uint8_t bit = (byte >> i) & 1;
        g3ruh_emit_bit(state, bit);

        if (bit) {
            ones_count++;
            if (ones_count == 5) {
                g3ruh_emit_bit(state, 0);
                ones_count = 0;
            }
        } else {
            ones_count = 0;
        }
    }
    return ones_count;
}

static uint16_t g3ruh_get_output_length(g3ruh_state *state)
{
    uint16_t total = state->byte_index;
    if (state->bit_index < 7) {
        total++;
    }
    return total;
}

/**
 * Encode an AX.25 frame for 9600 baud G3RUH transmission.
 *
 * Takes a raw AX.25 frame (with opening/closing 0x7E flags) and produces
 * a complete G3RUH-encoded byte stream ready for the SI4032 FIFO:
 *   1. Preamble flags (for descrambler synchronization)
 *   2. Opening flag
 *   3. Bit-stuffed frame data (addresses, control, PID, info, FCS)
 *   4. Closing flag
 *   All bits are passed through the G3RUH scrambler.
 */
uint16_t g3ruh_encode(uint8_t *ax25_frame, uint16_t ax25_length, uint8_t *output, uint16_t max_output_length)
{
    g3ruh_state state;
    g3ruh_init(&state, output, max_output_length);

    // Preamble: flag bytes for receiver descrambler synchronization
    for (uint16_t i = 0; i < G3RUH_PREAMBLE_FLAGS; i++) {
        g3ruh_emit_byte_raw(&state, AX25_PACKET_FLAG);
    }

    // Opening flag
    g3ruh_emit_byte_raw(&state, AX25_PACKET_FLAG);

    // Data portion with bit stuffing (skip opening flag [0] and closing flag [last])
    uint8_t ones_count = 0;
    for (uint16_t i = 1; i < ax25_length - 1; i++) {
        ones_count = g3ruh_emit_byte_stuffed(&state, ax25_frame[i], ones_count);
    }

    // Closing flag
    g3ruh_emit_byte_raw(&state, AX25_PACKET_FLAG);

    return g3ruh_get_output_length(&state);
}
