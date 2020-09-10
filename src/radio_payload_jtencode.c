#include <stdio.h>
#include <stdint.h>
#include "radio_payload_jtencode.h"

uint16_t radio_ft8_encode(uint8_t *payload, uint16_t length, telemetry_data *telemetry_data, char *message)
{
    return snprintf((char *) payload, length, "%s", message);
}

payload_encoder radio_ft8_payload_encoder = {
        .encode = radio_ft8_encode,
};

uint16_t radio_jt9_encode(uint8_t *payload, uint16_t length, telemetry_data *telemetry_data, char *message)
{
    return snprintf((char *) payload, length, "%s", message);
}

payload_encoder radio_jt9_payload_encoder = {
        .encode = radio_jt9_encode,
};

uint16_t radio_jt4_encode(uint8_t *payload, uint16_t length, telemetry_data *telemetry_data, char *message)
{
    return snprintf((char *) payload, length, "%s", message);
}

payload_encoder radio_jt4_payload_encoder = {
        .encode = radio_jt4_encode,
};

uint16_t radio_jt65_encode(uint8_t *payload, uint16_t length, telemetry_data *telemetry_data, char *message)
{
    return snprintf((char *) payload, length, "%s", message);
}

payload_encoder radio_jt65_payload_encoder = {
        .encode = radio_jt65_encode,
};
