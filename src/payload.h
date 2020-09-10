#ifndef __PAYLOAD_H
#define __PAYLOAD_H

#include <stddef.h>
#include <stdint.h>

#include "telemetry.h"

typedef struct _payload_encoder {
    uint16_t (*encode)(uint8_t *payload, uint16_t length, telemetry_data *data, char *message);
} payload_encoder;

#endif
