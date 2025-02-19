#ifndef __CONFIG_INTERNAL_H
#define __CONFIG_INTERNAL_H

#define GPS_SERIAL_PORT_BAUD_RATE 38400

// The external serial port baud rate must be higher than the GPS serial port baud rate (38400)
#define EXTERNAL_SERIAL_PORT_BAUD_RATE 115200

#define RADIO_PAYLOAD_MAX_LENGTH 320
#define RADIO_SYMBOL_DATA_MAX_LENGTH 512
#define RADIO_PAYLOAD_MESSAGE_MAX_LENGTH 128

// PARIS: 50 dot durations, 20 WPM -> 60ms per unit
#define MORSE_WPM_TO_SYMBOL_RATE(wpm) (1000 / (60 * 20 / wpm))

// Experimental fast frequency change routine for Si5351, not tested
#define SI5351_FAST_ENABLE false

#include <stdbool.h>

extern bool leds_enabled;
extern bool gps_nmea_output_enabled;
extern bool bmp280_enabled;
extern bool radsens_enabled;
extern bool si5351_enabled;
extern bool pulse_counter_enabled;

extern volatile bool system_initialized;

extern char *cw_message_templates[];
extern char *pip_message_templates[];
extern char *aprs_comment_templates[];
extern char *cats_comment_templates[];
extern char *fsq_comment_templates[];
extern char *ftjt_message_templates[];

void set_green_led(bool enabled);
void set_red_led(bool enabled);

#endif
