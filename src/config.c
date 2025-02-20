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
 * $pc - Pulse counter value (wraps to zero at 65535, 16-bit unsigned value)
 * $ri - Radiation intensity in µR/h (up to 5 chars)
 * $dc - Data counter value, increases by one every time telemetry is read (wraps to zero at 65535, 16-bit unsigned value)
 * $gu - GPS data update indicator, 1 if GPS data was updated since time telemetry was read, 0 otherwise
 * $ct - Clock calibration trim value (0-31, only for DFM-17)
 * $cc - Clock calibration change count (only for DFM-17)
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
bool radsens_enabled = SENSOR_RADSENS_ENABLE;
bool si5351_enabled = RADIO_SI5351_ENABLE;
bool gps_nmea_output_enabled = GPS_NMEA_OUTPUT_VIA_SERIAL_PORT_ENABLE;
bool pulse_counter_enabled = PULSE_COUNTER_ENABLE;

volatile bool system_initialized = false;

/**
 * CW mode messages.
 * Maximum length: 64 characters.
 */
char *cw_message_templates[] = {
        "$cs",
//        "$cs $loc6 $altm $gs km/h $tiC",
//        "$cs $loc6",
//        "$alt m",
//        "$gs km/h $ti C",
        NULL
};

/**
 * "Pip" mode messages. Transmitted as CW, because a single "pip" can be represented as the 'E' character.
 * Maximum length: 64 characters.
 */
char *pip_message_templates[] = {
        "E", // An 'E' character in CW represents a single "pip".
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
//        " $loc12 - " APRS_COMMENT,
//        " $teC $hu% $prmb PC $pc RI $ri uR/h - " APRS_COMMENT,
        " " APRS_COMMENT,
        NULL
};

/**
 * CATS mode comment messages.
 * The maximum CATS comment length supported by RS41ng is about 100 characters.
 * The CATS standard allows for up to 255 characters.
 */
char *cats_comment_templates[] = {
//    "$dc $gu $sv $lat $lon - $hh:$mm:$ss @ $tow ms",
//    "T:$teC H:$hu% P:$prmb - " CATS_COMMENT,
//    "T:$teC H:$hu% P:$prmb PC:$pc RI:$ri uR/h - " CATS_COMMENT,
    CATS_COMMENT,
    NULL
};

/**
 * FSQ mode comment message templates.
 * Maximum length: 130 characters.
 */
char *fsq_comment_templates[] = {
//        "TEST $loc6 $altm $tiC",
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
