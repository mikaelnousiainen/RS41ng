#include <stdint.h>
#include <stdlib.h>

#include "config.h"
#include "telemetry.h"
#include "payload.h"
#include "log.h"
#include "codecs/cats/cats.h"
#include "codecs/cats/whisker.h"

#define CATS_PREAMBLE_BYTE 0x55
#define CATS_PREAMBLE_LENGTH 4
#define CATS_SYNC_WORD 0xABCDEF12
#define CATS_SYNC_WORD_LENGTH 4

uint16_t radio_cats_encode(uint8_t *payload, uint16_t length, telemetry_data *telemetry_data, char *message)
{
    uint8_t *cur = payload;

    // preamble
    for (int i = 0; i < CATS_PREAMBLE_LENGTH; i++) { // 4
        *(cur++) = CATS_PREAMBLE_BYTE;
    }

    // sync word
    for (int i = CATS_SYNC_WORD_LENGTH - 1; i >= 0; i--) { // 4
        *(cur++) = (CATS_SYNC_WORD >> (i * 8));
    }

    uint8_t *data = malloc(length);
    cats_packet packet = cats_create(data);
    cats_append_identification_whisker(&packet, CATS_CALLSIGN, CATS_SSID, CATS_ICON); // 11
    cats_append_comment_whisker(&packet, message); // 102

    if (GPS_HAS_FIX(telemetry_data->gps) &&
       (telemetry_data->gps.latitude_degrees_1000000 != 0 ||
        telemetry_data->gps.longitude_degrees_1000000 != 0)) {
        cats_append_gps_whisker(&packet, telemetry_data->gps); // 16
    }

    cats_append_node_info_whisker(&packet, telemetry_data); // 16

    size_t len = cats_fully_encode(packet, cur);
    log_info("CATS packet length: %ld\n", len + CATS_PREAMBLE_LENGTH + CATS_SYNC_WORD_LENGTH);
    free(data);

    return (uint16_t)(CATS_PREAMBLE_LENGTH + CATS_SYNC_WORD_LENGTH + len);
}

payload_encoder radio_cats_payload_encoder = {
    .encode = radio_cats_encode,
};
