#include <stdint.h>
#include <string.h>
#include "codecs/horus/horus_packet_v3.h"
#include "codecs/horus/horus_l2.h"
#include "config.h"
#include "telemetry.h"
#include "radio_payload_horus_v3.h"

#ifdef SEMIHOSTING_ENABLE
#include "log.h"
#endif

#define HORUS_V3_PREAMBLE_BYTE 0xE4

uint16_t radio_horus_v3_encode(uint8_t *payload, uint16_t length, telemetry_data *telemetry_data, char *message)
{
    char buffer[HORUS_UNCODED_BUFFER_SIZE];

    memset(buffer, 0, HORUS_UNCODED_BUFFER_SIZE);

    size_t packet_length = horus_packet_v3_create((uint8_t *) &buffer, telemetry_data);

#ifdef SEMIHOSTING_ENABLE
    log_info("Horus V3 packet: ");
    log_bytes_hex((int) packet_length, (char *) &buffer);
    log_info("\n");
#endif

    // Preamble to help the decoder lock-on after a quiet period.
    for (int i = 0; i < HORUS_V3_PREAMBLE_LENGTH; i++) {
        payload[i] = HORUS_V3_PREAMBLE_BYTE;
    }

    // Encode the packet, and write into the mfsk buffer.
    int encoded_length = horus_l2_encode_tx_packet(
            (unsigned char *) payload + HORUS_V3_PREAMBLE_LENGTH,
            (unsigned char *) &buffer, (int) packet_length);

    return encoded_length + HORUS_V3_PREAMBLE_LENGTH;
}

uint16_t radio_horus_v3_idle_encode(uint8_t *payload, uint16_t length, telemetry_data *telemetry_data, char *message)
{
    // Use the preamble for idle tones during continuous transmit mode
    for (int i = 0; i < HORUS_V3_IDLE_PREAMBLE_LENGTH; i++) {
        payload[i] = HORUS_V3_PREAMBLE_BYTE;
    }

    return HORUS_V3_IDLE_PREAMBLE_LENGTH;
}

payload_encoder radio_horus_v3_payload_encoder = {
        .encode = radio_horus_v3_encode,
};

payload_encoder radio_horus_v3_idle_encoder = {
        .encode = radio_horus_v3_idle_encode,
};
