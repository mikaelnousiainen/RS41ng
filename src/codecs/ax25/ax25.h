#ifndef __AX25_H
#define __AX25_H

#include <stdint.h>

#define AX25_PACKET_FLAG 0x7E

#define AX25_CONTROL_FIELD_UI_FRAME 0x03
#define AX25_PROTOCOL_ID_NO_LAYER_3 0xF0

typedef struct _ax25_packet_header {
    uint8_t flag;
    char destination[6];
    uint8_t destination_ssid;
    char source[6];
    char source_ssid;
} ax25_packet_header;

typedef struct _ax25_packet_header_end {
    uint8_t control_field;
    uint8_t protocol_id;
    char information_field[];
} ax25_packet_header_end;

typedef struct _ax25_packet_footer {
    uint8_t frame_check_sequence[2];
    uint8_t flag;
} ax25_packet_footer;

uint16_t ax25_encode_packet_aprs(char *source, uint8_t source_ssid, char *destination, uint8_t destination_ssid,
        char *digipeater_addresses, char *information_field, uint16_t length, uint8_t *packet_data);

#endif
