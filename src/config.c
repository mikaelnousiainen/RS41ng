/**
 * The tracker firmware will transmit each of the message templates defined here in rotation, one by one,
 * starting again from the beginning once the last message for a particular mode is transmitted.
 *
 * Supported variable references in templates:
 *
 * $cs - Call sign
 * $loc4 - Locator (4 chars)
 * $loc6 - Locator (6 chars)
 * $loc8 - Locator (8 chars)
 * $loc12 - Locator (12 chars)
 * $bv - Battery voltage in millivolts (up to 4 chars)
 * $te - External temperature in C (up to 3 chars)
 * $ti - Internal temperature in C (up to 3 chars)
 * $hu - Humidity percentage (up to 3 chars)
 * $pr - Atmospheric pressure in millibars (up to 4 chars)
 * $tow - GPS time of week in milliseconds
 * $hh - Current hour (2 chars)
 * $mm - Current minute (2 chars)
 * $ss - Current second (2 chars)
 * $sv - GPS satellites visible (up to 2 chars)
 * $lat - Latitude in degrees * 1000 (up to 6 chars)
 * $lon - Longitude in degrees * 1000 (up to 6 chars)
 * $alt - Altitude in meters (up to 5 chars)
 * $gs - Ground speed in km/h (up to 3 chars)
 * $cl - Climb in m/s (up to 2 chars)
 * $he - Heading in degrees (up to 3 chars)
 *
 * Allowed message lengths:
 *
 * APRS comment - Free text up to 127 chars
 * FT8 - Free text up to 13 chars (Type 0.0 free text message, Type 0.5 telemetry message)
 * JT65 - Free text up to 13 chars (Plaintext Type 6 message)
 * JT9 - Free text up to 13 chars (Plaintext Type 6 message)
 * JT4 - Free text up to 13 chars (Plaintext Type 6 message)
 * FSQ - Call sign up to 20 chars, free text up to 130 chars
 * WSPR - Call sign up to 6 chars, locator 4 chars, output power in dBm
 */

#include <stdlib.h>
#include "config.h"

bool leds_enabled = LEDS_ENABLE;
bool bmp280_enabled = SENSOR_BMP280_ENABLE;
bool si5351_enabled = RADIO_SI5351_ENABLE;

volatile bool system_initialized = false;

/**
 * CW mode messages.
 * Maximum length: 64 characters.
 */
char *cw_message_templates[] = {
        "$cs TEST $loc6 $altm $tiC",
        NULL
};

/**
 * APRS mode comment messages.
 * Maximum length: depends on the packet contents, but keeping this under 100 characters is usually safe.
 * Note that many hardware APRS receivers show a limited number of APRS comment characters, such as 43 or 67 chars.
 */
char *aprs_comment_templates[] = {
//        " B$bu $teC $hu% $prmb $hh:$mm:$ss @ $tow ms - " APRS_COMMENT,
//        " B$bu $teC $hu% $prmb - " APRS_COMMENT,
//        " B$bu $loc12 $hh:$mm:$ss - " APRS_COMMENT,
        " $loc12 - " APRS_COMMENT,
        NULL
};

/**
 * FSQ mode comment message templates.
 * Maximum length: 130 characters.
 */
char *fsq_comment_templates[] = {
        "TEST $loc6 $altm $tiC",
//        " $lat $lon, $alt m, $cl m/s, $gs km/h, $he deg - " FSQ_COMMENT,
//        " $loc12, $teC $hu% $prmb $hh:$mm:$ss @ $tow ms - " FSQ_COMMENT,
        NULL
};

/**
 * FTx/JTxx mode message templates.
 * Maximum length: 13 characters allowed by the protocols.
 */
char *ftjt_message_templates[] = {
//        "$cs $loc4",
//        "$loc12",
//        "$altm $cl",
//        "$bvmV $tiC",
//        "$hu% $prmb",
        NULL
};
