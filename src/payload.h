#ifndef __PAYLOAD_H
#define __PAYLOAD_H

#include <stddef.h>
#include <stdint.h>

#include "telemetry.h"

typedef struct _payload_encoder {
    size_t (*encode)(uint8_t *payload, size_t length, telemetry_data *data);
} payload_encoder;

#endif
