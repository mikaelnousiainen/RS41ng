#ifndef __HORUS_PACKET_V3_H
#define __HORUS_PACKET_V3_H

#include <stdint.h>
#include <stdlib.h>
#include "telemetry.h"
#include "asn1/HorusBinaryV3.h"

size_t horus_packet_v3_create(uint8_t *payload, telemetry_data *data);

// Currently only NOHUB feature will trigger extensions,
#if HORUS_V3_NOHUB
// extensions enabled
#define HORUS_V3_EXTENSIONS 1
// Eventually we would count how many extension fields are active
#define HORUS_V3_EXTRA_FIELD_COUNT 0
#else
#define HORUS_V3_EXTENSIONS 0
#define HORUS_V3_EXTRA_FIELD_COUNT 0
#endif


#endif
