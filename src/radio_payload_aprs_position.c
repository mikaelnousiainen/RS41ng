#include <stdint.h>

#include "codecs/ax25/ax25.h"
#include "codecs/aprs/aprs_position.h"
#include "config.h"
#include "telemetry.h"
#include "log.h"
#include "radio_payload_aprs_position.h"

uint16_t radio_aprs_position_encode(uint8_t *payload, uint16_t length, telemetry_data *telemetry_data, char *message)
{
    uint8_t aprs_packet[RADIO_PAYLOAD_MAX_LENGTH];

    aprs_generate_position(aprs_packet, sizeof(aprs_packet), telemetry_data,
            APRS_SYMBOL_TABLE, APRS_SYMBOL, false, message);

    log_debug("APRS packet: %s\n", aprs_packet);

    return ax25_encode_packet_aprs(APRS_CALLSIGN, APRS_SSID, APRS_DESTINATION, APRS_DESTINATION_SSID, APRS_RELAYS,
            (char *) aprs_packet, length, payload);
}

payload_encoder radio_aprs_position_payload_encoder = {
        .encode = radio_aprs_position_encode,
};
