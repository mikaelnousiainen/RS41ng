#include <stdio.h>
#include <string.h>
#include "strlcpy.h"
#include "template.h"

int main(void)
{
    char *source = "DE $cs: $bv $loc6, $hh:$mm:$ss, $tow, Ti$ti Te$te $hu $pr";
    char dest[512];
    telemetry_data data;

    data.battery_voltage_millivolts = 3247;
    data.internal_temperature_celsius_100 = -7 * 100;
    data.temperature_celsius_100 = 24 * 100;
    data.humidity_percentage_100 = 68 * 100;
    data.pressure_mbar_100 = 1023 * 100;
    strlcpy(data.locator, "KP21FA35jk45", sizeof(data.locator));

    data.gps.time_of_week_millis = 110022330;
    data.gps.hours = 18;
    data.gps.minutes = 33;
    data.gps.seconds = 51;

    template_replace(dest, sizeof(dest), source, &data);

    printf("%s\n", dest);

    printf("%03d\n", data.internal_temperature_celsius_100 / 100);
}
