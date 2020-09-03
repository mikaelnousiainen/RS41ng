#include <stdio.h>
#include <stdbool.h>

#include "codecs/bell/bell.h"
#include "codecs/aprs/aprs.h"
#include "codecs/ax25/ax25.h"
#include "telemetry.h"
#include "config.h"

int main(void)
{
    fsk_encoder fsk_encoder;

    bell_encoder_new(&fsk_encoder, 1200, 0, bell202_tones);

    telemetry_data telemetry;
    memset(&telemetry, 0, sizeof(telemetry_data));

    uint8_t aprs_packet[256];
    size_t aprs_length = aprs_generate_position(
                aprs_packet, sizeof(aprs_packet), &telemetry, APRS_SYMBOL_TABLE, APRS_SYMBOL, false, APRS_COMMENT);

    uint8_t payload[256];
    size_t payload_length = ax25_encode_packet_aprs(APRS_CALLSIGN, APRS_SSID, APRS_DESTINATION, APRS_DESTINATION_SSID, APRS_RELAYS,
               (char *) aprs_packet, aprs_length, payload);

    printf("Full payload length: %ld\n", payload_length);

    for (int i = 0; i < payload_length; i++) {
        uint8_t c = payload[i];
        if (c >= 0x20 && c <= 0x7e) {
            printf("%c", c);
        } else {
            printf(" [%02X] ", c);
        }
    }

    printf("\n");

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
