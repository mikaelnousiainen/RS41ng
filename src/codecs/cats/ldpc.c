#include <assert.h>
#include <string.h>

#include "ldpc.h"
#include "ldpc_matrices.h"

#define GET_BIT(byte, bit) (((byte) & (1 << (7-(bit)))) != 0)
#define SET_BIT(byte, bit) (byte |= 1 << 7-bit)
#define CLEAR_BIT(byte, bit) (byte &= ~(1 << 7-bit))
#define FLIP_BIT(byte, bit) (byte ^= (1 << (7 - bit)))

cats_ldpc_code_t *cats_ldpc_pick_code(int len)
{
    if(128 <= len)
        return &tm2048;
    else if(32 <= len)
        return &tc512;
    else if(16 <= len)
        return &tc256;
    else
        return &tc128;
}

// returns # of bytes written to parity_out
size_t cats_ldpc_encode_chunk(uint8_t *data, cats_ldpc_code_t *code, uint8_t *parity_out)
{
    // Code parameters
    int k = code->k;
    int r = code->n - code->k;
    int b = code->circulant_size;
    const uint64_t* gc = code->matrix;
    int rowLen = r/64;

    memset(parity_out, 0x00, (code->k)/8);

    for (int offset = 0; offset < b; offset++) {
        for (int crow = 0; crow < k/b; crow++) {
            int bit = crow*b + offset;
            if(GET_BIT(data[bit/8], bit%8)) {
                for (int idx = 0; idx < rowLen; idx++) {
                    uint64_t circ = gc[(crow*rowLen)+idx];
                    for(int j = 0; j < 8; j++) {
                        parity_out[idx*8 + j] ^= (uint8_t)(circ >> ((7 - j) * 8));
                    }
                }
            }
        }

        for (int block = 0; block < r/b; block++) {
            uint8_t* parityblock = &parity_out[block*b/8];
            uint8_t carry = parityblock[0] >> 7;
            for (int x = (b/8)-1; x >= 0; x--) {
                uint8_t c = parityblock[x] >> 7;
                parityblock[x] = (parityblock[x] << 1) | carry;
                carry = c;
            }
        }
    }

    return k / 8;
}

size_t cats_ldpc_encode(uint8_t *data, size_t len)
{
    // Didn't implement the 512-byte LDPC variant - unnecessary and uses a lot of space
    // This means we can only encode up to 511 bytes
    assert(len < 512);
    
    uint8_t parity[128];

    // Split data into blocks and encode each block
    int i = 0;
    while(i < len) {
        cats_ldpc_code_t *code = cats_ldpc_pick_code(len - i);
        int k = code->k;
        
        uint8_t chunk[code->n / 8];
        memset(chunk, 0xAA, k/8);
        memcpy(chunk, data + i, (len-i < k/8) ? (len-i) : (k/8));

        size_t parity_len = cats_ldpc_encode_chunk(chunk, code, parity);
        memcpy(data + len + i, parity, parity_len); // Parity

        i += parity_len;
    }

    int new_len = (len*2) + (i-len) + 2; // (Data+Parity) + (Padded parity) + length
    data[new_len - 2] = len;
    data[new_len - 1] = len >> 8;
    return new_len;
}
