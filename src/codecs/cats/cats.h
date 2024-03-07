#ifndef __CATS_H
#define __CATS_H

#include <stdint.h>
#include <stddef.h>

typedef struct _cats_packet {
    uint8_t *data;
    size_t len;
} cats_packet;

cats_packet cats_create(uint8_t *payload);
size_t cats_fully_encode(cats_packet packet, uint8_t *out);

#endif
