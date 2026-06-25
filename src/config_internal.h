#ifndef __CONFIG_INTERNAL_H
#define __CONFIG_INTERNAL_H

#define GPS_SERIAL_PORT_BAUD_RATE 38400

// The external serial port baud rate must be higher than the GPS serial port baud rate (38400)
#define EXTERNAL_SERIAL_PORT_BAUD_RATE 115200

#define RADIO_PAYLOAD_MAX_LENGTH 512
#define RADIO_SYMBOL_DATA_MAX_LENGTH 256
#define RADIO_PAYLOAD_MESSAGE_MAX_LENGTH 64

#define HORUS_UNCODED_BUFFER_SIZE 128
#define HORUS_CODED_BUFFER_SIZE 256

// PARIS: 50 dot durations, 20 WPM -> 60ms per unit
#define MORSE_WPM_TO_SYMBOL_RATE(wpm) (1000 / (60 * 20 / wpm))

// Experimental fast frequency change routine for Si5351, not tested
#define SI5351_FAST_ENABLE false

// Bench test: auto-enter STABILIZING after this many seconds of uptime,
// bypassing the altitude arm and descent checks.  Set to 0 for flight mode.
#define LANDED_MODE_TEST_SECONDS 0

// GPS sleep test: put GPS to sleep after this many seconds of uptime.
// Radio and everything else continues normally.  Set to 0 to disable.
#define GPS_SLEEP_TEST_SECONDS 0

// Number of consecutive failed GPS init attempts before the red error strobe is
// armed at startup. A cold GPS commonly needs a retry or two to ACK, which is
// normal, so the first few failures stay silent to avoid a spurious 5s strobe.
#define GPS_INIT_RED_LED_RETRY_THRESHOLD 3

#include <stdbool.h>

extern volatile bool system_initialized;

extern char *cw_message_templates[];
extern char *pip_message_templates[];
extern char *aprs_comment_templates[];
extern char *cats_comment_templates[];
extern char *fsq_comment_templates[];
extern char *ftjt_message_templates[];

void set_green_led(bool enabled);
void set_red_led(bool enabled);

typedef enum _sensor_type {
    NO_EXT_SENSOR = 0,
    SENSOR_BMP280,
    SENSOR_BME280,
    SENSOR_BME68X,
    SENSOR_BME690,
} sensor_type;

#endif
