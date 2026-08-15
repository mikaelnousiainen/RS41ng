#include <stdint.h>

#include "codecs/ax25/ax25.h"
#include "codecs/aprs/aprs.h"
#include "codecs/aprs/aprs_weather.h"
#include "config.h"
#include "telemetry.h"
#include "log.h"
#include "radio.h"
#include "radio_payload_aprs_weather.h"

uint16_t radio_aprs_weather_report_encode(uint8_t *payload, uint16_t length, telemetry_data *telemetry_data, char *message)
{
    uint8_t aprs_packet[RADIO_APRS_PAYLOAD_MAX_LENGTH];

    gps_data gps;
    if (radio_gps_get_position(&gps)) {
        struct _telemetry_data local_data = *telemetry_data;
        local_data.gps = gps;
        aprs_generate_weather_report(aprs_packet, sizeof(aprs_packet), &local_data, true, message);
    } else {
        aprs_generate_status(aprs_packet, sizeof(aprs_packet), message);
    }

    log_debug("APRS packet: %s\n", aprs_packet);

    return ax25_encode_packet_aprs(APRS_CALLSIGN, APRS_SSID, APRS_DESTINATION, APRS_DESTINATION_SSID, APRS_RELAYS,
            (char *) aprs_packet, length, payload);
}

payload_encoder radio_aprs_weather_report_payload_encoder = {
        .encode = radio_aprs_weather_report_encode,
};
