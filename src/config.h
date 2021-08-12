#ifndef __CONFIG_H
#define __CONFIG_H

// Enable semihosting to receive debug logs during development
// NOTE: Semihosting has to be disabled when the RS41 radiosonde is not connected to the STM32 programmer dongle, otherwise the firmware will not run.
//#define SEMIHOSTING_ENABLE
//#define LOGGING_ENABLE

/**
 * Global configuration
 */

// Set the tracker amateur radio call sign here
#define CALLSIGN "MYCALL"

// Disabling LEDs will save power
// Red LED: Lit during initialization and transmit.
// Green LED: Blinking fast when there is no GPS fix. Blinking slowly when the GPS has a fix.
#define LEDS_ENABLE true

// Enable use of an externally connected I²C BMP280 atmospheric sensor
#define SENSOR_BMP280_ENABLE false

// Enable use of an externally connected I²C Si5351 clock generator chip for HF radio transmissions
#define RADIO_SI5351_ENABLE false

// Number of character pairs to include in locator
#define LOCATOR_PAIR_COUNT_FULL 6 // max. 6 (12 characters WWL)

// Delay after transmission for modes that do not use time synchronization
#define RADIO_POST_TRANSMIT_DELAY_MS 5000

// Threshold for time-synchronized modes regarding how far from scheduled transmission time the transmission is still allowed
#define RADIO_TIME_SYNC_THRESHOLD_MS 2000

/**
 * Built-in Si4032 radio chip transmission configuration
 */

// Si4032 transmit power: 0..7
// 0 = -1dBm, 1 = 2dBm, 2 = 5dBm, 3 = 8dBm, 4 = 11dBm, 5 = 14dBm, 6 = 17dBm, 7 = 20dBm
#define RADIO_SI4032_TX_POWER 7

// Which modes to transmit using the built-in Si4032 transmitter chip
#define RADIO_SI4032_TX_CW false
#define RADIO_SI4032_TX_RTTY false
#define RADIO_SI4032_TX_APRS true
#define RADIO_SI4032_TX_HORUS_V1 true

// Transmit frequencies for the Si4032 transmitter modes
#define RADIO_SI4032_TX_FREQUENCY_CW   432060000
#define RADIO_SI4032_TX_FREQUENCY_RTTY 432060000
#define RADIO_SI4032_TX_FREQUENCY_APRS_1200 432500000
// Use a frequency offset to place FSK tones slightly above the defined frequency for SSB reception
#define RADIO_SI4032_TX_FREQUENCY_HORUS_V1  432501000

/**
 * External Si5351 radio chip transmission configuration
 */

// Si5351 transmit power: 0..3
// Si5351 drive strength: 0 = 2mA, 1 = 4mA, 2 = 6mA, 3 = 8mA
#define RADIO_SI5351_TX_POWER 3

// Which modes to transmit using an externally connected Si5351 chip in the I²C bus
#define RADIO_SI5351_TX_JT9 false
#define RADIO_SI5351_TX_JT65 false
#define RADIO_SI5351_TX_JT4 false
#define RADIO_SI5351_TX_WSPR false
#define RADIO_SI5351_TX_FSQ false
#define RADIO_SI5351_TX_FT8 false

// Transmit frequencies for the Si5351 transmitter modes
#define RADIO_SI5351_TX_FREQUENCY_JT9        14085000UL    // Was: 14078700UL
#define RADIO_SI5351_TX_FREQUENCY_JT65       14085000UL    // Was: 14078300UL
#define RADIO_SI5351_TX_FREQUENCY_JT4        14085000UL    // Was: 14078500UL
#define RADIO_SI5351_TX_FREQUENCY_WSPR       14085000UL    // Was: 14097200UL
#define RADIO_SI5351_TX_FREQUENCY_FSQ        14085000UL    // Was: 7105350UL     // Base freq is 1350 Hz higher than dial freq in USB
#define RADIO_SI5351_TX_FREQUENCY_FT8        14085000UL    // Was: 14075000UL

/**
 * APRS mode settings
 *
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
#define APRS_COMMENT "RS41ng radiosonde firmware test"
#define APRS_RELAYS "WIDE1-1,WIDE2-1"
#define APRS_DESTINATION "APZ41N"
#define APRS_DESTINATION_SSID '0'

// Schedule transmission every N seconds, counting from beginning of an hour (based on GPS time). Set to zero to disable time sync.
#define APRS_TIME_SYNC_SECONDS 1
// Delay transmission for an N second offset after the scheduled time.
#define APRS_TIME_SYNC_OFFSET_SECONDS 0

/**
 * Horus V1 4FSK mode settings
 */

// Use Horus payload ID 0 for tests (4FSKTEST)
#define HORUS_V1_PAYLOAD_ID 0
#define HORUS_V1_BAUD_RATE 100
#define HORUS_V1_FREQUENCY_OFFSET 0
#define HORUS_V1_PREAMBLE_LENGTH 16

// Schedule transmission every N seconds, counting from beginning of an hour (based on GPS time). Set to zero to disable time sync.
#define HORUS_V1_TIME_SYNC_SECONDS 1
// Delay transmission for an N second offset after the scheduled time.
#define HORUS_V1_TIME_SYNC_OFFSET_SECONDS 0

/**
 * TODO: CW settings (once implemented)
 */

#define CW_LOCATOR_PAIR_COUNT 4 // max. 6 (12 characters WWL)

/**
 * WSPR settings
 */
#define WSPR_CALLSIGN CALLSIGN
#define WSPR_LOCATOR_FIXED_ENABLED false
#define WSPR_LOCATOR_FIXED "AA00"
#define WSPR_DBM 10

#define WSPR_TIME_SYNC_SECONDS 120
#define WSPR_TIME_SYNC_OFFSET_SECONDS 1

/**
 * FSQ settings
 */
#define FSQ_CALLSIGN_FROM CALLSIGN
#define FSQ_COMMENT "RS41ng radiosonde firmware test"

#define FSQ_SUBMODE RADIO_DATA_MODE_FSQ_6

#define FSQ_TIME_SYNC_SECONDS 0
#define FSQ_TIME_SYNC_OFFSET_SECONDS 0

/**
 * FT/JT mode settings
 */

// Schedule transmission every 15 seconds, counting from beginning of an hour (based on GPS time). Set to zero to disable time sync.
#define FT8_TIME_SYNC_SECONDS 15
// Delay transmission for 1 second after the scheduled time.
#define FT8_TIME_SYNC_OFFSET_SECONDS 1

#define JT9_TIME_SYNC_SECONDS 60
#define JT9_TIME_SYNC_OFFSET_SECONDS 1

#define JT4_TIME_SYNC_SECONDS 60
#define JT4_TIME_SYNC_OFFSET_SECONDS 1

#define JT65_TIME_SYNC_SECONDS 60
#define JT65_TIME_SYNC_OFFSET_SECONDS 1

#include "config_internal.h"

#endif
