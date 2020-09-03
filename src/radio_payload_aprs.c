#include <stdio.h>
#include <stdint.h>

#include "codecs/ax25/ax25.h"
#include "codecs/aprs/aprs.h"
#include "config.h"
#include "telemetry.h"
#include "log.h"
#include "radio_payload_aprs.h"

uint8_t aprs_packet[RADIO_PAYLOAD_MAX_LENGTH];
char aprs_comment[APRS_COMMENT_MAX_LENGTH];

const char *aprs_comment_format = " RS41ng test, time is %02d:%02d:%02d, locator %s, TOW %lu ms, gs %lu, hd %ld";

uint16_t radio_aprs_encode(uint8_t *payload, uint16_t length, telemetry_data *telemetry_data)
{
    gps_data *gps = &telemetry_data->gps;

    snprintf(aprs_comment, sizeof(aprs_comment),
            aprs_comment_format,
            gps->hours, gps->minutes, gps->seconds, telemetry_data->locator, gps->time_of_week_millis,
            gps->ground_speed_cm_per_second, gps->heading_degrees_100000);

    aprs_generate_position(aprs_packet, sizeof(aprs_packet), telemetry_data,
            APRS_SYMBOL_TABLE, APRS_SYMBOL, false, aprs_comment);

    log_debug("APRS packet: %s\n", aprs_packet);

    return ax25_encode_packet_aprs(APRS_CALLSIGN, APRS_SSID, APRS_DESTINATION, APRS_DESTINATION_SSID, APRS_RELAYS,
            (char *) aprs_packet, length, payload);
}

payload_encoder radio_aprs_payload_encoder = {
        .encode = radio_aprs_encode,
};
