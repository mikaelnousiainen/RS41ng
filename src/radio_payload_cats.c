#include <stdint.h>

#include "config.h"
#include "telemetry.h"
#include "payload.h"
#include "log.h"

#define CATS_PREAMBLE_BYTE 0x55
#define CATS_PREAMBLE_LENGTH 40
#define CATS_SYNC_WORD 0xABCDEF12
#define CATS_SYNC_WORD_LENGTH 4

uint16_t radio_cats_encode(uint8_t *payload, uint16_t length, telemetry_data *telemetry_data, char *message)
{
    uint8_t *cur = payload;

    // preamble
    for(int i = 0; i < CATS_PREAMBLE_LENGTH; i++) {
        *(cur++) = CATS_PREAMBLE_BYTE;
    }

    // sync word
    for(int i = 0; i < CATS_SYNC_WORD_LENGTH; i++) {
        int j = CATS_SYNC_WORD_LENGTH - i - 1;
        *(cur++) = (CATS_SYNC_WORD & (0xFF << (j * 8))) >> (j * 8);
    }

    // data - todo fix this
    uint8_t msg[] = {140, 0, 163, 12, 172, 77, 255, 254, 201, 107, 102, 245, 143, 108, 185, 202, 207, 194, 243, 54, 138, 46, 62, 36, 41, 70, 203, 43, 32, 76, 254, 214, 159, 124, 132, 48, 135, 189, 2, 147, 188, 229, 24, 195, 194, 177, 162, 193, 105, 140, 31, 11, 51, 214, 51, 206, 150, 112, 105, 73, 234, 161, 46, 10, 93, 79, 68, 145, 150, 25, 185, 46, 166, 86, 190, 116, 218, 236, 204, 161, 28, 57, 3, 1, 46, 90, 207, 253, 69, 133, 62, 36, 173, 20, 220, 49, 12, 222, 0, 251, 73, 46, 57, 188, 62, 122, 142, 35, 144, 187, 27, 215, 75, 128, 189, 147, 76, 41, 53, 203, 30, 234, 222, 52, 73, 206, 209, 233, 81, 173, 58, 179, 194, 172, 207, 220, 169, 32, 24, 196, 40, 44};

    for(int i = 0; i < sizeof(msg); i++) {
        (*cur++) = msg[i];
    }

    return (uint16_t)(cur - payload);
}

payload_encoder radio_cats_payload_encoder = {
    .encode = radio_cats_encode,
};
