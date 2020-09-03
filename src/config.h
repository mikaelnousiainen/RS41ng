#ifndef __CONFIG_H
#define __CONFIG_H

// Enable semihosting to receive debug logs during development
//#define SEMIHOSTING_ENABLE
//#define LOGGING_ENABLE

#include <stdbool.h>

#define RADIO_PAYLOAD_MAX_LENGTH 256
#define RADIO_SYMBOL_DATA_MAX_LENGTH 512
#define APRS_COMMENT_MAX_LENGTH 128

#define SENSOR_BMP280_ENABLE false

#define RADIO_SI5351_ENABLE true

#define RADIO_POST_TRANSMIT_DELAY_MS 5000
#define RADIO_TIME_SYNC_THRESHOLD_MS 1500

// Si4032 transmit power: 0..100%
#define RADIO_SI4032_TX_POWER 100
#define RADIO_SI4032_TX_FREQUENCY_CW   432060000
#define RADIO_SI4032_TX_FREQUENCY_RTTY 432060000
#define RADIO_SI4032_TX_FREQUENCY_APRS 432500000

#define RADIO_SI5351_TX_POWER 100
#define RADIO_SI5351_TX_FREQUENCY_JT9        14078700UL
#define RADIO_SI5351_TX_FREQUENCY_JT65       14078300UL
#define RADIO_SI5351_TX_FREQUENCY_JT4        14078500UL
#define RADIO_SI5351_TX_FREQUENCY_WSPR       14085000UL     // Was: 14097200UL
#define RADIO_SI5351_TX_FREQUENCY_FSQ        7105350UL     // Base freq is 1350 Hz higher than dial freq in USB
#define RADIO_SI5351_TX_FREQUENCY_FT8        14085000UL    // Was: 14075000UL

#define LOCATOR_PAIR_COUNT_FULL 6 // max. 6 (12 characters WWL)

#define WSPR_CALLSIGN "OH3BHX"
#define WSPR_LOCATOR_FIXED_ENABLED false
#define WSPR_LOCATOR_FIXED "AA00"
#define WSPR_DBM 10

#define FT8_CALLSIGN "OH3BHX"
#define FT8_LOCATOR_FIXED_ENABLED false
#define FT8_LOCATOR_FIXED "AA00"

#define FSQ_CALLSIGN_FROM "OH3BHX"
#define FSQ_CALLSIGN_TO "N0CALL"
#define FSQ_COMMAND ' '

/**
 * APRS SSID:
 *
 * 0 Your primary station usually fixed and message capable
 * 1 generic additional station, digi, mobile, wx, etc
 * 2 generic additional station, digi, mobile, wx, etc
 * 3 generic additional station, digi, mobile, wx, etc
 * 4 generic additional station, digi, mobile, wx, etc
 * 5 Other networks (Dstar, Iphones, Androids, Blackberry's etc)
 * 6 Special activity, Satellite ops, camping or 6 meters, etc
 * 7 walkie talkies, HT's or other human portable
 * 8 boats, sailboats, RV's or second main mobile
 * 9 Primary Mobile (usually message capable)
 * A internet, Igates, echolink, winlink, AVRS, APRN, etc
 * B balloons, aircraft, spacecraft, etc
 * C APRStt, DTMF, RFID, devices, one-way trackers*, etc
 * D Weather stations
 * E Truckers or generally full time drivers
 * F generic additional station, digi, mobile, wx, etc
 */

#define APRS_CALLSIGN "OH3BHX"
#define APRS_SSID 'B'
// See APRS symbol table documentation in: http://www.aprs.org/symbols/symbolsX.txt
#define APRS_SYMBOL_TABLE '/' // '/' denotes primary and '\\' denotes alternate APRS symbol table
#define APRS_SYMBOL '['
#define APRS_COMMENT " RS41ng custom radiosonde firmware testing"
#define APRS_RELAYS "WIDE1-1,WIDE2-1"
#define APRS_DESTINATION "APZ41N"
#define APRS_DESTINATION_SSID '0'

#define RTTY_LOCATOR_PAIR_COUNT 4 // max. 6 (12 characters WWL)
#define RTTY_7BIT   1 // if 0 --> 5 bits

#define CW_LOCATOR_PAIR_COUNT 4 // max. 6 (12 characters WWL)

extern bool bmp280_enabled;
extern bool si5351_enabled;

extern volatile bool system_initialized;

#endif
