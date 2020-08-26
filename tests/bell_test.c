#include <stdio.h>
#include <stdbool.h>

#include "codecs/bell/bell.h"

int main(void)
{
    fsk_encoder fsk_encoder;

    bell_encoder_new(&fsk_encoder, 1200, 0, bell202_tones);

    // aprs_generate_position_without_timestamp(
    //            aprs_packet, sizeof(aprs_packet), telemetry_data, APRS_SYMBOL, APRS_COMMENT);
    // ax25_encode_packet_aprs(APRS_CALLSIGN, APRS_SSID, "APZ41N", 0, "",
    //            (char *) aprs_packet, length, payload);

    //uint8_t payload[] = { 0x7e, 0x82, 0xa0, 0xb4, 'h', 'b', 0x9c, 0x00, 0x7e};
    // 0x7e: 0 1 1 1 1 1 1 0
    // 0x82: 0 1 0 0 0 0 0 1
    // 0xa0: 0 0 0 0 0 1 0 1
    // 0xb4: 0 0 1 0 1 1 0 1

    uint8_t payload[] = { 0x7e, 0x3f, 0x7f };
    // 1 1 1 1 1   1 0 0
    // N N N N N S N C C

    // 1 1 1 1 1   1 1 0
    // N N N N N S N N C

    bell_encoder_set_data(&fsk_encoder, sizeof(payload), payload);

    size_t index = 0;
    while (true) {
        int8_t tone = bell_encoder_next_tone(&fsk_encoder);
        if (tone < 0) {
            break;
        }

        printf("%d ", tone);

        index++;

        if (index % 8 == 0) {
            printf("\n");
        }
    };

    bell_encoder_destroy(&fsk_encoder);
}
