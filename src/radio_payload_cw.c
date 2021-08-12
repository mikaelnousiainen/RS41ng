#include <stdio.h>
#include <stdint.h>
#include "radio_payload_cw.h"

uint16_t radio_cw_encode(uint8_t *payload, uint16_t length, telemetry_data *telemetry_data, char *message)
{
    return snprintf((char *) payload, length, "%s", message);
}

payload_encoder radio_cw_payload_encoder = {
        .encode = radio_cw_encode,
};
