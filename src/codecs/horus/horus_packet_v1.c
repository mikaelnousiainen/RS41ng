#include <string.h>
#include "horus_packet_v1.h"
#include "horus_common.h"

volatile uint16_t horus_v1_packet_counter = 0;

size_t horus_packet_v1_create(uint8_t *payload, size_t length, telemetry_data *data, uint8_t payload_id)
{
    if (length < sizeof(horus_packet_v1)) {
        return 0;
    }

    gps_data *gps_data = &data->gps;

    float float_lat = (float) gps_data->latitude_degrees_1000000 / 10000000.0f;
    float float_lon = (float) gps_data->longitude_degrees_1000000 / 10000000.0f;

    uint8_t volts_scaled = (uint8_t)(255 * (float) data->battery_voltage_millivolts / 5000.0f);

    horus_v1_packet_counter++;

    // Assemble a binary packet
    horus_packet_v1 horus_packet;

    horus_packet.PayloadID = payload_id;
    horus_packet.Counter = horus_v1_packet_counter;
    horus_packet.Hours = gps_data->hours;
    horus_packet.Minutes = gps_data->minutes;
    horus_packet.Seconds = gps_data->seconds;
    horus_packet.Latitude = float_lat;
    horus_packet.Longitude = float_lon;
    horus_packet.Altitude = (uint16_t)((gps_data->altitude_mm > 0 ? gps_data->altitude_mm : 0) / 1000);
    horus_packet.Speed = (uint8_t)((float) gps_data->ground_speed_cm_per_second * 0.036);

    // Temporary pDOP info, to determine suitable pDOP limits.
    // float pDop = (float)gps_data.pDOP/10.0;
    // if (pDop>255.0){
    //  pDop = 255.0;
    // }
    // mfsk_buffer.Speed = (uint8_t)pDop;
    horus_packet.BattVoltage = volts_scaled;
    horus_packet.Sats = gps_data->satellites_visible;
    horus_packet.Temp = (int8_t) ((float) data->internal_temperature_celsius_100 / 100.0f);

    // Add onto the sats_raw value to indicate if the GPS is in regular tracking (+100)
    // or power optimized tracker (+200) modes.
    if (gps_data->power_safe_mode_state == POWER_SAFE_MODE_STATE_TRACKING) {
        horus_packet.Sats += 100;
    } else if (gps_data->power_safe_mode_state == POWER_SAFE_MODE_STATE_POWER_OPTIMIZED_TRACKING) {
        horus_packet.Sats += 200;
    } else if (gps_data->power_safe_mode_state == POWER_SAFE_MODE_STATE_INACTIVE) {
        // Inactive = Most parts of the receiver are switched off
        horus_packet.Sats += 50;
    }

    horus_packet.Checksum = (uint16_t) calculate_crc16_checksum((char *) &horus_packet, sizeof(horus_packet) - 2);

    memcpy(payload, &horus_packet, sizeof(horus_packet));

    return sizeof(horus_packet);
}
