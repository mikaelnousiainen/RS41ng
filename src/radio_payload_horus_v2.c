#include <stdint.h>

#include "codecs/horus/horus_packet_v2.h"
#include "codecs/horus/horus_l2.h"
#include "config.h"
#include "telemetry.h"
#include "radio_payload_horus_v2.h"

#ifdef SEMIHOSTING_ENABLE
#include "log.h"
#endif

#define HORUS_V2_PREAMBLE_BYTE 0xE4

uint16_t radio_horus_v2_encode(uint8_t *payload, uint16_t length, telemetry_data *telemetry_data, char *message)
{
    horus_packet_v2 horus_packet;

    size_t packet_length = horus_packet_v2_create((uint8_t *) &horus_packet, sizeof(horus_packet),
            telemetry_data, HORUS_V2_PAYLOAD_ID);

#ifdef SEMIHOSTING_ENABLE
    log_info("Horus V2 packet: ");
    log_bytes_hex((int) packet_length, (char *) &horus_packet);
    log_info("\n");
#endif

    // Preamble to help the decoder lock-on after a quiet period.
    for (int i = 0; i < HORUS_V2_PREAMBLE_LENGTH; i++) {
        payload[i] = HORUS_V2_PREAMBLE_BYTE;
    }

    // Encode the packet, and write into the mfsk buffer.
    int encoded_length = horus_l2_encode_tx_packet(
            (unsigned char *) payload + HORUS_V2_PREAMBLE_LENGTH,
            (unsigned char *) &horus_packet, (int) packet_length);

    return encoded_length + HORUS_V2_PREAMBLE_LENGTH;
}

uint16_t radio_horus_v2_idle_encode(uint8_t *payload, uint16_t length, telemetry_data *telemetry_data, char *message)
{
    // Use the preamble for idle tones during continuous transmit mode
    for (int i = 0; i < HORUS_V2_IDLE_PREAMBLE_LENGTH; i++) {
        payload[i] = HORUS_V2_PREAMBLE_BYTE;
    }

    return HORUS_V2_IDLE_PREAMBLE_LENGTH;
}

payload_encoder radio_horus_v2_payload_encoder = {
        .encode = radio_horus_v2_encode,
};

payload_encoder radio_horus_v2_idle_encoder = {
        .encode = radio_horus_v2_idle_encode,
};
