#include <stdbool.h>

#include "interleaver.h"

static inline bool get_bit(uint8_t *arr, size_t i);
static inline void set_bit(uint8_t *arr, size_t i, bool value);

void cats_interleave(uint8_t *dest, uint8_t *src, size_t len)
{
    size_t bit_len = len * 8;

    size_t out_i = 0;
    for(int i = 0; i < 32; i++) {
        for(int j = 0; j < bit_len; j += 32) {
            if(i + j >= bit_len) {
                continue;
            }

            set_bit(dest, out_i, get_bit(src, i + j));
            out_i++;
        }
    }
}

static inline bool get_bit(uint8_t *arr, size_t i)
{
    size_t byte_idx = i / 8;
    int bit_idx = 7 - (i % 8);

    return (arr[byte_idx] >> bit_idx) & 1;
}


static inline void set_bit(uint8_t *arr, size_t i, bool value)
{
    size_t byte_idx = i / 8;
    int bit_idx = 7 - (i % 8);

    if(value) {
        arr[byte_idx] |= 1 << bit_idx;
    }
    else {
        arr[byte_idx] &= ~(1 << bit_idx);
    }
}
