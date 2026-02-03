#ifndef __HORUS_PACKET_V3_H
#define __HORUS_PACKET_V3_H

#include <stdint.h>
#include <stdlib.h>
#include "telemetry.h"
#include "asn1/HorusBinaryV3.h"

size_t horus_packet_v3_create(uint8_t *payload, telemetry_data *data);

#endif
