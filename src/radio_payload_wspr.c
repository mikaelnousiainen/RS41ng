#include <stdint.h>
#include "telemetry.h"
#include "radio_payload_wspr.h"

uint16_t radio_wspr_encode(uint8_t *payload, uint16_t length, telemetry_data *telemetry_data, char *message)
{
    // Not used for WSPR
    return 0;
}

payload_encoder radio_wspr_payload_encoder = {
        .encode = radio_wspr_encode,
};
