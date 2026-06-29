#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "strlcpy.h"
#include "template.h"
#include "codecs/aprs/aprs.h"

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

    assert(strcmp(dest, "DE 4FSKTEST: 3247 KP21FA, 18:33:51, 110022330, Ti-7 Te24 68 1023") == 0);

    // $apc reads the shared global APRS packet counter
    aprs_packet_counter = 1234;
    data.gps.satellites_visible = 10;
    data.gps.climb_cm_per_second = 100;
    data.button_adc_value = 456;
    data.radiation_intensity_uR_h = 1231;
    data.data_counter = 1235;
    data.gps.updated = 1;
    data.gps.latitude_degrees_10000000 = 1234567890;
    data.gps.longitude_degrees_10000000 = 1324567890;
    data.gps.altitude_mm = 12345;
    data.error_code = 23;

    source = "/P$apcS$svT$tiV$bvC$clA$bu $ri $dc $gu $lat $lon $alt E$err";
    template_replace(dest, sizeof(dest), source, &data);

    printf("%s\n", dest);

    // $err renders as a zero-padded 2-digit decimal
    assert(strcmp(dest, "/P1234S10T-7V3247C1A456 1231 1235 1 123456 132456 12 E23") == 0);

    // Regression: a template longer than the destination buffer (as happens with
    // a 64-byte message buffer and a long comment, for example) must still expand the
    // template variables and truncate the tail - never transmit literal $vars.
    char small[64];
    source = "/P$apcS$svT$tiV$bvC$clA$bu high-altitude balloon APRS 432.500, Horus 432.300, Wenet 436.750";
    template_replace(small, sizeof(small), source, &data);

    printf("%s\n", small);

    assert(strlen(small) < sizeof(small));
    // Variables are expanded, not left as literal tokens...
    assert(strstr(small, "$bv") == NULL);
    assert(strstr(small, "$bu") == NULL);
    assert(strstr(small, "$apc") == NULL);
    assert(strstr(small, "$ti") == NULL);
    // ...and their substituted values are present.
    assert(strncmp(small, "/P1234S10T-7V3247C1A456 ", 23) == 0);

    printf("All template_test assertions passed\n");

    return 0;
}
