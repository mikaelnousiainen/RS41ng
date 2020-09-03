#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "radio_payload_ft8.h"
#include "config.h"

const bool ft8_locator_fixed_enabled = FT8_LOCATOR_FIXED_ENABLED;

uint16_t radio_ft8_encode(uint8_t *payload, uint16_t length, telemetry_data *telemetry_data)
{
    char locator[5];

    if (ft8_locator_fixed_enabled) {
        strlcpy(locator, FT8_LOCATOR_FIXED, 4 + 1);
    } else {
        strlcpy(locator, telemetry_data->locator, 4 + 1);
    }

    return snprintf((char *) payload, length, "%s %s", FT8_CALLSIGN, locator);
}

payload_encoder radio_ft8_payload_encoder = {
        .encode = radio_ft8_encode,
};
