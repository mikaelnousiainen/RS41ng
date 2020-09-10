#include <stdio.h>
#include <stdint.h>
#include "radio_payload_fsq.h"

uint16_t radio_fsq_encode(uint8_t *payload, uint16_t length, telemetry_data *telemetry_data, char *message)
{
    return snprintf((char *) payload, length, "%s", message);
}

payload_encoder radio_fsq_payload_encoder = {
        .encode = radio_fsq_encode,
};
