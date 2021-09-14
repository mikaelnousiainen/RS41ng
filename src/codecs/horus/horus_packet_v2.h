#ifndef __HORUS_PACKET_V2_H
#define __HORUS_PACKET_V2_H

#include <stdint.h>
#include <stdlib.h>
#include "telemetry.h"

// Horus Binary v2 Packet Format
// See: https://github.com/projecthorus/horusdemodlib/wiki/5-Customising-a-Horus-Binary-v2-Packet
// Note that we need to pack this to 1-byte alignment, hence the #pragma flags below
// Refer: https://gcc.gnu.org/onlinedocs/gcc-4.4.4/gcc/Structure_002dPacking-Pragmas.html
#pragma pack(push, 1)
typedef struct _horus_packet_v2 {
    uint16_t PayloadID; // Payload ID (0-65535)
    uint16_t Counter; // Sequence number
    uint8_t Hours; // Time of day, Hours
    uint8_t Minutes; // Time of day, Minutes
    uint8_t Seconds; // Time of day, Seconds
    float Latitude; // Latitude in degrees
    float Longitude; // Longitude in degrees
    uint16_t Altitude; // Altitude in meters
    uint8_t Speed; // Speed in km/h
    uint8_t Sats; // Number of GPS satellites visible
    int8_t Temp; // Temperature in Celsius, as a signed value (-128 to +128, though sensor limited to -64 to +64 deg C)
    uint8_t BattVoltage; // 0 = 0v, 255 = 5.0V, linear steps in-between.
    uint8_t CustomData[9]; // Custom data, see: https://github.com/projecthorus/horusdemodlib/wiki/5-Customising-a-Horus-Binary-v2-Packet#interpreting-the-custom-data-section
    uint16_t Checksum; // CRC16-CCITT Checksum.
} horus_packet_v2;  //  __attribute__ ((packed)); // Doesn't work?
#pragma pack(pop)

size_t horus_packet_v2_create(uint8_t *payload, size_t length, telemetry_data *data, uint16_t payload_id);

#endif
