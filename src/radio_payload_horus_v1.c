#include <stdint.h>

#include "codecs/horus/horus_packet_v1.h"
#include "codecs/horus/horus_l2.h"
#include "config.h"
#include "telemetry.h"
#include "radio_payload_horus_v1.h"

#ifdef SEMIHOSTING_ENABLE
#include "log.h"
#endif

#define HORUS_V1_PREAMBLE_BYTE 0x1B

uint16_t radio_horus_v1_encode(uint8_t *payload, uint16_t length, telemetry_data *telemetry_data, char *message)
{
    horus_packet_v1 horus_packet;

    size_t packet_length = horus_packet_v1_create((uint8_t *) &horus_packet, sizeof(horus_packet),
            telemetry_data, HORUS_V1_PAYLOAD_ID);

#ifdef SEMIHOSTING_ENABLE
    log_info("Horus V1 packet: ");
    log_bytes_hex((int) packet_length, (char *) &horus_packet);
    log_info("\n");
#endif

    // Preamble to help the decoder lock-on after a quiet period.
    for (int i = 0; i < HORUS_V1_PREAMBLE_LENGTH; i++) {
        payload[i] = HORUS_V1_PREAMBLE_BYTE;
    }

    // Encode the packet, and write into the mfsk buffer.
    int encoded_length = horus_l2_encode_tx_packet(
            (unsigned char *) payload + HORUS_V1_PREAMBLE_LENGTH,
            (unsigned char *) &horus_packet, (int) packet_length);

    return encoded_length + HORUS_V1_PREAMBLE_LENGTH;
}

uint16_t radio_horus_v1_idle_encode(uint8_t *payload, uint16_t length, telemetry_data *telemetry_data, char *message)
{
    // Use the preamble for idle tones during continuous transmit mode
    for (int i = 0; i < HORUS_V1_IDLE_PREAMBLE_LENGTH; i++) {
        payload[i] = HORUS_V1_PREAMBLE_BYTE;
    }

    return HORUS_V1_IDLE_PREAMBLE_LENGTH;
}

payload_encoder radio_horus_v1_payload_encoder = {
        .encode = radio_horus_v1_encode,
};

payload_encoder radio_horus_v1_idle_encoder = {
        .encode = radio_horus_v1_idle_encode,
};
