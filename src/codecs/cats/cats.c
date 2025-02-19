#include <stdint.h>

#include "cats.h"
#include "crc.h"
#include "whiten.h"
#include "ldpc.h"
#include "interleaver.h"

cats_packet cats_create(uint8_t *data)
{
    cats_packet c = {.data = data, .len = 0};
    return c;
}

// This makes changes to packet->data.
// Don't call it more than once, and don't use
// the packet after calling!
size_t cats_fully_encode(cats_packet packet, uint8_t *out)
{
    // 0. CRC
    size_t new_len = cats_append_crc(packet.data, packet.len);
    // 1. whiten
    cats_whiten(packet.data, new_len);
    // 2. ldpc
    new_len = cats_ldpc_encode(packet.data, new_len);
    // 3. length
    out[0] = new_len;
    out[1] = new_len >> 8;
    // 4. interleave
    cats_interleave(out + 2, packet.data, new_len);

    return new_len + 2;
}
