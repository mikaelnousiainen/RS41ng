#ifndef __CONFIG_H
#define __CONFIG_H

// Enable semihosting to receive debug logs during development
// NOTE: Semihosting has to be disabled when the RS41 radiosonde is not connected to the STM32 programmer dongle, otherwise the firmware will not run.
//#define SEMIHOSTING_ENABLE
//#define LOGGING_ENABLE

#include <stdbool.h>

#define CALLSIGN "OH3BHX"

#define RADIO_PAYLOAD_MAX_LENGTH 256
#define RADIO_SYMBOL_DATA_MAX_LENGTH 512
#define RADIO_PAYLOAD_MESSAGE_MAX_LENGTH 128

#define LEDS_ENABLE true

#define SENSOR_BMP280_ENABLE false

#define RADIO_SI5351_ENABLE true

#define RADIO_POST_TRANSMIT_DELAY_MS 5000
#define RADIO_TIME_SYNC_THRESHOLD_MS 1500

/**
 * Si4032 transmit power: 0..7
 * 0 = -1dBm, 1 = 2dBm, 2 = 5dBm, 3 = 8dBm, 4 = 11dBm, 5 = 14dBm, 6 = 17dBm, 7 = 20dBm
 */
#define RADIO_SI4032_TX_POWER 7
#define RADIO_SI4032_TX_FREQUENCY_CW   432060000
#define RADIO_SI4032_TX_FREQUENCY_RTTY 432060000
#define RADIO_SI4032_TX_FREQUENCY_APRS_1200 432500000

/**
 * Si5351 transmit power: 0..3
 * Si5351 drive strength: 0 = 2mA, 1 = 4mA, 2 = 6mA, 3 = 8mA
 */
#define RADIO_SI5351_TX_POWER 3
#define RADIO_SI5351_TX_FREQUENCY_JT9        14085000UL    // Was: 14078700UL
#define RADIO_SI5351_TX_FREQUENCY_JT65       14085000UL    // Was: 14078300UL
#define RADIO_SI5351_TX_FREQUENCY_JT4        14085000UL    // Was: 14078500UL
#define RADIO_SI5351_TX_FREQUENCY_WSPR       14085000UL    // Was: 14097200UL
#define RADIO_SI5351_TX_FREQUENCY_FSQ        14085000UL    // Was: 7105350UL     // Base freq is 1350 Hz higher than dial freq in USB
#define RADIO_SI5351_TX_FREQUENCY_FT8        14085000UL    // Was: 14075000UL

#define LOCATOR_PAIR_COUNT_FULL 6 // max. 6 (12 characters WWL)

#define WSPR_CALLSIGN CALLSIGN
#define WSPR_LOCATOR_FIXED_ENABLED false
#define WSPR_LOCATOR_FIXED "AA00"
#define WSPR_DBM 10

#define FSQ_CALLSIGN_FROM CALLSIGN

/**
 * APRS SSID:
 *
 * '0' = (-0) Your primary station usually fixed and message capable
 * '1' = (-1) generic additional station, digi, mobile, wx, etc
 * '2' = (-2) generic additional station, digi, mobile, wx, etc
 * '3' = (-3) generic additional station, digi, mobile, wx, etc
 * '4' = (-4) generic additional station, digi, mobile, wx, etc
 * '5' = (-5) Other networks (Dstar, Iphones, Androids, Blackberry's etc)
 * '6' = (-6) Special activity, Satellite ops, camping or 6 meters, etc
 * '7' = (-7) walkie talkies, HT's or other human portable
 * '8' = (-8) boats, sailboats, RV's or second main mobile
 * '9' = (-9) Primary Mobile (usually message capable)
 * 'A' = (-10) internet, Igates, echolink, winlink, AVRS, APRN, etc
 * 'B' = (-11) balloons, aircraft, spacecraft, etc
 * 'C' = (-12) APRStt, DTMF, RFID, devices, one-way trackers*, etc
 * 'D' = (-13) Weather stations
 * 'E' = (-14) Truckers or generally full time drivers
 * 'F' = (-15) generic additional station, digi, mobile, wx, etc
 */

#define APRS_CALLSIGN CALLSIGN
#define APRS_SSID 'B'
// See APRS symbol table documentation in: http://www.aprs.org/symbols/symbolsX.txt
#define APRS_SYMBOL_TABLE '/' // '/' denotes primary and '\\' denotes alternate APRS symbol table
#define APRS_SYMBOL 'O'
#define APRS_COMMENT " RS41ng radiosonde firmware test"
#define APRS_RELAYS "WIDE1-1,WIDE2-1"
#define APRS_DESTINATION "APZ41N"
#define APRS_DESTINATION_SSID '0'

// TODO: RTTY and CW settings (once modes are implemented)

#define RTTY_LOCATOR_PAIR_COUNT 4 // max. 6 (12 characters WWL)
#define RTTY_7BIT   1 // if 0 --> 5 bits

#define CW_LOCATOR_PAIR_COUNT 4 // max. 6 (12 characters WWL)

extern bool leds_enabled;
extern bool bmp280_enabled;
extern bool si5351_enabled;

extern volatile bool system_initialized;

extern char *aprs_comment_templates[];
extern char *fsq_comment_templates[];
extern char *ftjt_message_templates[];

#endif
