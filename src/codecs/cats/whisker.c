#include <string.h>
#include <assert.h>

#include "whisker.h"

void cats_append_identification_whisker(cats_packet *packet, char *callsign, uint8_t ssid, uint16_t icon)
{
    packet->data[packet->len++] = 0x00; // type = identidication
    size_t callsign_len = strlen(callsign);
    packet->data[packet->len++] = callsign_len + 3; // len
    packet->data[packet->len++] = icon & 0xFF; // icon
    packet->data[packet->len++] = (icon >> 8) & 0xFF; // icon
    memcpy(packet->data + packet->len, callsign, callsign_len); //callsign
    packet->len += callsign_len;
    packet->data[packet->len++] = ssid; // ssid
}

void cats_append_comment_whisker(cats_packet *packet, char *comment)
{
    packet->data[packet->len++] = 0x03; // type = comment
    size_t comment_len = strlen(comment);
    assert(comment_len <= 255);
    packet->data[packet->len++] = comment_len;
    memcpy(packet->data + packet->len, comment, comment_len);
    packet->len += comment_len;
}
