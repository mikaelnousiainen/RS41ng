#include <stdbool.h>

#include "whiten.h"

uint8_t lfsr_byte(uint16_t *state);
void lfsr(uint16_t *state);

void cats_whiten(uint8_t *data, uint8_t len)
{
    uint16_t state = 0xE9CF;

    for (int i = 0; i < len; i++) {
        uint8_t b = lfsr_byte(&state);
        data[i] ^= b;
    }
}

uint8_t lfsr_byte(uint16_t *state)
{
    uint8_t out = 0;
    for (int i = 7; i >= 0; i--) {
        out |= (*state & 1) << i;
        lfsr(state);
    }

    return out;
}

void lfsr(uint16_t *state)
{
    bool lsb = *state & 1;
    *state >>= 1;
    if (lsb) {
        *state ^= 0xB400;
    }
}
