#include <assert.h>
#include <string.h>

#include "ldpc.h"
#include "ldpc_matrices.h"

#define GET_BIT(byte, bit) (((byte) & (1 << (7-(bit)))) != 0)
#define SET_BIT(byte, bit) (byte |= 1 << 7-bit)
#define CLEAR_BIT(byte, bit) (byte &= ~(1 << 7-bit))
#define FLIP_BIT(byte, bit) (byte ^= (1 << (7 - bit)))

cats_ldpc_code_t *cats_ldpc_pick_code(size_t len)
{
    if (len >= 128) {
        return &tm2048;
    } else if (len >= 32) {
        return &tc512;
    } else if (len >= 16) {
        return &tc256;
    } else {
        return &tc128;
    }
}

// For more information on this, see the CATS standard
// https://gitlab.scd31.com/cats/cats-standard
// Section 5.2
// returns # of bytes written to parity_out
size_t cats_ldpc_encode_chunk(uint8_t *data, cats_ldpc_code_t *code, uint8_t *parity_out)
{
    // Code parameters
    int data_length_bits = code->data_length_bits;
    int parity_length_bits = code->code_length_bits - code->data_length_bits;
    int circ_size = (int) code->circulant_size;
    const uint64_t* gc = code->matrix;
    int row_len = parity_length_bits / 64;

    memset(parity_out, 0x00, parity_length_bits / 8);

    for (int offset = 0; offset < circ_size; offset++) {
        for (int crow = 0; crow < data_length_bits / circ_size; crow++) {
            int bit = crow * circ_size + offset;
            if (GET_BIT(data[bit / 8], bit % 8)) {
                for (int idx = 0; idx < row_len; idx++) {
                    uint64_t circ = gc[(crow * row_len) + idx];
                    for (int j = 0; j < 8; j++) {
                        parity_out[idx * 8 + j] ^= (uint8_t)(circ >> ((7 - j) * 8));
                    }
                }
            }
        }

        for (int block = 0; block < parity_length_bits / circ_size; block++) {
            uint8_t* parityblock = &parity_out[block * circ_size / 8];
            uint8_t carry = parityblock[0] >> 7;
            for (int x = (circ_size / 8) - 1; x >= 0; x--) {
                uint8_t c = parityblock[x] >> 7;
                parityblock[x] = (parityblock[x] << 1) | carry;
                carry = c;
            }
        }
    }

    return parity_length_bits / 8;
}

size_t cats_ldpc_encode(uint8_t *data, size_t len)
{
    // Didn't implement the 512-byte LDPC variant - unnecessary and uses a lot of space
    // This means we can only encode up to 511 bytes
    assert(len < 512);
    
    // A 128 byte array is needed to support CATS packets up to 511 bytes.
    uint8_t parity[128];

    // Split data into blocks and encode each block
    size_t i = 0;
    while (i < len) {
        cats_ldpc_code_t *code = cats_ldpc_pick_code(len - i);
        int data_length_bits = code->data_length_bits;
        int data_length = data_length_bits / 8;
        
        uint8_t chunk[code->code_length_bits / 8];
        memset(chunk, 0xAA, data_length);
        memcpy(chunk, data + i, (len - i < data_length) ? (len - i) : data_length);

        size_t parity_len = cats_ldpc_encode_chunk(chunk, code, parity);
        memcpy(data + len + i, parity, parity_len); // Parity

        i += parity_len;
    }

    size_t new_len = (len * 2) + (i - len) + 2; // (Data+Parity) + (Padded parity) + length
    data[new_len - 2] = len;
    data[new_len - 1] = len >> 8;
    return new_len;
}
