#include <stdio.h>
#include <stdlib.h>
#include "utils.h"
#include "config.h"
#include "template.h"

size_t template_replace(char *dest, size_t dest_len, char *src, telemetry_data *data)
{
    char *temp = malloc(dest_len);
    if (temp == NULL) {
        return 0;
    }

    char replacement[32];

    strlcpy(replacement, CALLSIGN, sizeof(replacement));
    strlcpy(temp, src, dest_len);
    str_replace(dest, dest_len, temp, "$cs", replacement);

    strlcpy(replacement, data->locator, 4 + 1);
    strlcpy(temp, dest, dest_len);
    str_replace(dest, dest_len, temp, "$loc4", replacement);

    strlcpy(replacement, data->locator, 6 + 1);
    strlcpy(temp, dest, dest_len);
    str_replace(dest, dest_len, temp, "$loc6", replacement);

    strlcpy(replacement, data->locator, 8 + 1);
    strlcpy(temp, dest, dest_len);
    str_replace(dest, dest_len, temp, "$loc8", replacement);

    strlcpy(replacement, data->locator, 12 + 1);
    strlcpy(temp, dest, dest_len);
    str_replace(dest, dest_len, temp, "$loc12", replacement);

    snprintf(replacement, sizeof(replacement), "%d", data->battery_voltage_millivolts);
    strlcpy(temp, dest, dest_len);
    str_replace(dest, dest_len, temp, "$bv", replacement);

    snprintf(replacement, sizeof(replacement), "%d", data->button_adc_value);
    strlcpy(temp, dest, dest_len);
    str_replace(dest, dest_len, temp, "$bu", replacement);

    snprintf(replacement, sizeof(replacement), "%d", (int) data->temperature_celsius_100 / 100);
    strlcpy(temp, dest, dest_len);
    str_replace(dest, dest_len, temp, "$te", replacement);

    snprintf(replacement, sizeof(replacement), "%d", (int) data->internal_temperature_celsius_100 / 100);
    strlcpy(temp, dest, dest_len);
    str_replace(dest, dest_len, temp, "$ti", replacement);

    snprintf(replacement, sizeof(replacement), "%d", (int) data->humidity_percentage_100 / 100);
    strlcpy(temp, dest, dest_len);
    str_replace(dest, dest_len, temp, "$hu", replacement);

    snprintf(replacement, sizeof(replacement), "%d", (int) data->pressure_mbar_100 / 100);
    strlcpy(temp, dest, dest_len);
    str_replace(dest, dest_len, temp, "$pr", replacement);

    snprintf(replacement, sizeof(replacement), "%u", (unsigned int) data->gps.time_of_week_millis);
    strlcpy(temp, dest, dest_len);
    str_replace(dest, dest_len, temp, "$tow", replacement);

    snprintf(replacement, sizeof(replacement), "%02d", data->gps.hours);
    strlcpy(temp, dest, dest_len);
    str_replace(dest, dest_len, temp, "$hh", replacement);

    snprintf(replacement, sizeof(replacement), "%02d", data->gps.minutes);
    strlcpy(temp, dest, dest_len);
    str_replace(dest, dest_len, temp, "$mm", replacement);

    snprintf(replacement, sizeof(replacement), "%02d", data->gps.seconds);
    strlcpy(temp, dest, dest_len);
    str_replace(dest, dest_len, temp, "$ss", replacement);

    snprintf(replacement, sizeof(replacement), "%d", data->gps.satellites_visible);
    strlcpy(temp, dest, dest_len);
    str_replace(dest, dest_len, temp, "$sv", replacement);

    snprintf(replacement, sizeof(replacement), "%05d", (int) data->gps.latitude_degrees_1000000 / 10000);
    strlcpy(temp, dest, dest_len);
    str_replace(dest, dest_len, temp, "$lat", replacement);

    snprintf(replacement, sizeof(replacement), "%05d", (int) data->gps.longitude_degrees_1000000 / 10000);
    strlcpy(temp, dest, dest_len);
    str_replace(dest, dest_len, temp, "$lon", replacement);

    snprintf(replacement, sizeof(replacement), "%d", (int) data->gps.altitude_mm / 1000);
    strlcpy(temp, dest, dest_len);
    str_replace(dest, dest_len, temp, "$alt", replacement);

    snprintf(replacement, sizeof(replacement), "%d",
            (int) ((float) data->gps.ground_speed_cm_per_second * 3.6f / 100.0f));
    strlcpy(temp, dest, dest_len);
    str_replace(dest, dest_len, temp, "$gs", replacement);

    snprintf(replacement, sizeof(replacement), "%d", (int) data->gps.climb_cm_per_second / 100);
    strlcpy(temp, dest, dest_len);
    str_replace(dest, dest_len, temp, "$cl", replacement);

    snprintf(replacement, sizeof(replacement), "%03d", (int) data->gps.heading_degrees_100000 / 100000);
    strlcpy(temp, dest, dest_len);
    size_t len = str_replace(dest, dest_len, temp, "$he", replacement);

    snprintf(replacement, sizeof(replacement), "%d", (int) data->pulse_count);
    strlcpy(temp, dest, dest_len);
    str_replace(dest, dest_len, temp, "$pc", replacement);

    free(temp);

    return len;
}
