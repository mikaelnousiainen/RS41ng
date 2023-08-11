#include <string.h>
#include "horus_packet_v2.h"
#include "horus_common.h"
#include "config.h"

volatile uint16_t horus_v2_packet_counter = 0;

size_t horus_packet_v2_create(uint8_t *payload, size_t length, telemetry_data *data, uint16_t payload_id)
{
    if (length < sizeof(horus_packet_v2)) {
        return 0;
    }

    gps_data *gps_data = &data->gps;

    float float_lat = (float) gps_data->latitude_degrees_1000000 / 10000000.0f;
    float float_lon = (float) gps_data->longitude_degrees_1000000 / 10000000.0f;

    uint8_t volts_scaled = (uint8_t) (255 * (float) data->battery_voltage_millivolts / 5000.0f);

    horus_v2_packet_counter++;

    // Assemble a binary packet
    horus_packet_v2 horus_packet;

    horus_packet.PayloadID = payload_id;
    horus_packet.Counter = horus_v2_packet_counter;
    horus_packet.Hours = gps_data->hours;
    horus_packet.Minutes = gps_data->minutes;
    horus_packet.Seconds = gps_data->seconds;
    horus_packet.Latitude = float_lat;
    horus_packet.Longitude = float_lon;
    horus_packet.Altitude = (uint16_t) ((gps_data->altitude_mm > 0 ? gps_data->altitude_mm : 0) / 1000);
    horus_packet.Speed = (uint8_t) ((float) gps_data->ground_speed_cm_per_second * 0.036);

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

    memset(horus_packet.CustomData, 0, sizeof(horus_packet.CustomData));

    uint8_t *custom_data_pointer = horus_packet.CustomData;

    if (radsens_enabled) {
        // Unit: µR/h
        uint16_t ext_radiation_intensity_uR_h = (uint16_t) data->radiation_intensity_uR_h;
        memcpy(custom_data_pointer, &ext_radiation_intensity_uR_h, sizeof(ext_radiation_intensity_uR_h));
        custom_data_pointer += sizeof(ext_radiation_intensity_uR_h);
    } else {
        // Unit: cm/s
        int16_t gps_climb_cm_per_second = (int16_t) gps_data->climb_cm_per_second;
        memcpy(custom_data_pointer, &gps_climb_cm_per_second, sizeof(gps_climb_cm_per_second));
        custom_data_pointer += sizeof(gps_climb_cm_per_second);
    }

    // Unit: Celsius * 10
    int16_t ext_temp_celsius_10 = (int16_t) (data->temperature_celsius_100 / 10.0f);
    memcpy(custom_data_pointer, &ext_temp_celsius_10, sizeof(ext_temp_celsius_10));
    custom_data_pointer += sizeof(ext_temp_celsius_10);

    // Unit: %
    uint8_t ext_humidity_percentage = (uint8_t) (data->humidity_percentage_100 / 100.0f);
    memcpy(custom_data_pointer, &ext_humidity_percentage, sizeof(ext_humidity_percentage));
    custom_data_pointer += sizeof(ext_humidity_percentage);

    // Unit: mbar * 10
    uint16_t ext_pressure_mbar = (uint16_t) (data->pressure_mbar_100 / 10.0f);
    memcpy(custom_data_pointer, &ext_pressure_mbar, sizeof(ext_pressure_mbar));

    if (pulse_counter_enabled || radsens_enabled) {
        // Unit: pulse count
        custom_data_pointer += sizeof(ext_pressure_mbar);
        uint16_t ext_pulse_count = (uint16_t) data->pulse_count;
        memcpy(custom_data_pointer, &ext_pulse_count, sizeof(ext_pulse_count));
    }

    horus_packet.Checksum = (uint16_t) calculate_crc16_checksum((char *) &horus_packet,
            sizeof(horus_packet) - sizeof(horus_packet.Checksum));

    memcpy(payload, &horus_packet, sizeof(horus_packet));

    return sizeof(horus_packet);
}
