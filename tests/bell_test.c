#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>

#include "codecs/bell/bell.h"
#include "codecs/aprs/aprs_position.h"
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

    // Zeroed telemetry + built-in config produce a deterministic APRS position report.
    assert(aprs_length == 73);
    aprs_packet[aprs_length] = '\0';
    assert(strstr((char *) aprs_packet, "!0000.00S/00000.00WO000/000/A=000000") != NULL);
    assert(strstr((char *) aprs_packet, "https://amateur.sondehub.org/4FSKTEST") != NULL);

    uint8_t payload[256];
    memset(payload, 0, sizeof(payload));
    size_t payload_length = ax25_encode_packet_aprs(APRS_CALLSIGN, APRS_SSID, APRS_DESTINATION, APRS_DESTINATION_SSID, APRS_RELAYS,
            (char *) aprs_packet, aprs_length, payload);

    printf("Full payload length: %zu\n", payload_length);
    assert(payload_length == 93);

    // Encode the actual packet (payload_length), not the whole buffer.
    bell_encoder_set_data(&fsk_encoder, payload_length, payload);

    size_t tone_count = 0;
    int8_t tone;
    while ((tone = bell_encoder_next_tone(&fsk_encoder)) >= 0) {
        assert(tone == 0 || tone == 1);
        tone_count++;
    }
    printf("Bell202 tone count: %zu\n", tone_count);
    assert(tone_count == 745);

    bell_encoder_destroy(&fsk_encoder);

    printf("All bell_test assertions passed\n");

    return 0;
}
