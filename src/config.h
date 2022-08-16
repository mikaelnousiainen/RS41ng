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

// Disable LEDs above the specified altitude (in meters) to save power. Set to zero to disable this behavior.
#define LEDS_DISABLE_ALTITUDE_METERS 1000

// Allow powering off the sonde by pressing the button for over a second (when the sonde is not transmitting)
#define ALLOW_POWER_OFF false

// Define the I²C bus clock speed in Hz.
// The default of 100000 (= 100 kHz) should be used with the Si5351 clock generator to allow fast frequency changes.
// Note that some BMP280 sensors may require decreasing the clock speed to 10000 (= 10 kHz)
#define I2C_BUS_CLOCK_SPEED 100000

// Enable use of an externally connected I²C BMP280/BME280 atmospheric sensor
// NOTE: Only BME280 sensors will report humidity. For BMP280 humidity readings will be zero.
#define SENSOR_BMP280_ENABLE false

// Enable use of an externally connected I²C Si5351 clock generator chip for HF radio transmissions
#define RADIO_SI5351_ENABLE false

// Number of character pairs to include in locator
#define LOCATOR_PAIR_COUNT_FULL 6 // max. 6 (12 characters WWL)

// Delay after transmission for modes that do not use time synchronization. Zero delay allows continuous transmit mode for Horus V1 and V2.
#define RADIO_POST_TRANSMIT_DELAY_MS 1000

// Threshold for time-synchronized modes regarding how far from scheduled transmission time the transmission is still allowed
#define RADIO_TIME_SYNC_THRESHOLD_MS 2000

// Enable this setting to require 3D fix (altitude required, enable for airborne use), otherwise 2D fix is enough
#define GPS_REQUIRE_3D_FIX true

// Enable NMEA output from GPS via external serial port. This disables use of I²C bus (Si5351 and sensors) because the pins are shared.
#define GPS_NMEA_OUTPUT_VIA_SERIAL_PORT_ENABLE false

#if (GPS_NMEA_OUTPUT_VIA_SERIAL_PORT_ENABLE) && ((RADIO_SI5351_ENABLE) || (SENSOR_BMP280_ENABLE))
#error GPS NMEA output via serial port cannot be enabled simultaneously with the I2C bus.
#endif

// Enable pulse counter via expansion header pin for use with devices like Geiger counters.
// This disables the external I²C bus and the serial port as the expansion header pin 2 (I2C2_SDA (PB11) / UART3 RX) is used for pulse input.
// Also changes the Horus 4FSK V2 data format and adds an additional custom data field for pulse count.
#define PULSE_COUNTER_ENABLE false

#if (PULSE_COUNTER_ENABLE) && ((GPS_NMEA_OUTPUT_VIA_SERIAL_PORT_ENABLE) || (RADIO_SI5351_ENABLE) || (SENSOR_BMP280_ENABLE))
#error Pulse counter cannot be enabled simultaneously with GPS NMEA output or I2C bus sensors.
#endif

/**
 * Built-in Si4032 radio chip transmission configuration
 */

// Si4032 transmit power: 0..7
// 0 = -1dBm, 1 = 2dBm, 2 = 5dBm, 3 = 8dBm, 4 = 11dBm, 5 = 14dBm, 6 = 17dBm, 7 = 20dBm
#define RADIO_SI4032_TX_POWER 7

// Which modes to transmit using the built-in Si4032 transmitter chip
#define RADIO_SI4032_TX_CW false
#define RADIO_SI4032_TX_CW_COUNT 1
#define RADIO_SI4032_TX_PIP false
#define RADIO_SI4032_TX_PIP_COUNT 6
#define RADIO_SI4032_TX_APRS true
#define RADIO_SI4032_TX_APRS_COUNT 2
#define RADIO_SI4032_TX_HORUS_V1 false
#define RADIO_SI4032_TX_HORUS_V1_COUNT 1
#define RADIO_SI4032_TX_HORUS_V2 true
#define RADIO_SI4032_TX_HORUS_V2_COUNT 6

// Continuous transmit mode can be enabled for *either* Horus V1 or V2, but not both. This disables all other transmission modes.
// The continuous mode transmits Horus 4FSK preamble between transmissions
// to allow Horus receivers to keep frequency synchronization at all times, which improves reception.
#define RADIO_SI4032_TX_HORUS_V1_CONTINUOUS false
#define RADIO_SI4032_TX_HORUS_V2_CONTINUOUS false

// Transmit frequencies for the Si4032 transmitter modes
#define RADIO_SI4032_TX_FREQUENCY_CW        432500000
#define RADIO_SI4032_TX_FREQUENCY_PIP       432500000
#define RADIO_SI4032_TX_FREQUENCY_APRS_1200 432500000
// Use a frequency offset to place FSK tones slightly above the defined frequency for SSB reception
#define RADIO_SI4032_TX_FREQUENCY_HORUS_V1  432501000
#define RADIO_SI4032_TX_FREQUENCY_HORUS_V2  432501000

/**
 * External Si5351 radio chip transmission configuration
 */

// Si5351 transmit power: 0..3
// Si5351 drive strength: 0 = 2mA, 1 = 4mA, 2 = 6mA, 3 = 8mA
#define RADIO_SI5351_TX_POWER 3

// Which modes to transmit using an externally connected Si5351 chip in the I²C bus
#define RADIO_SI5351_TX_CW true
#define RADIO_SI5351_TX_CW_COUNT 1
#define RADIO_SI5351_TX_PIP false
#define RADIO_SI5351_TX_PIP_COUNT 6
#define RADIO_SI5351_TX_HORUS_V1 false
#define RADIO_SI5351_TX_HORUS_V1_COUNT 1
#define RADIO_SI5351_TX_HORUS_V2 true
#define RADIO_SI5351_TX_HORUS_V2_COUNT 4
#define RADIO_SI5351_TX_JT9 false
#define RADIO_SI5351_TX_JT9_COUNT 1
#define RADIO_SI5351_TX_JT65 false
#define RADIO_SI5351_TX_JT65_COUNT 1
#define RADIO_SI5351_TX_JT4 false
#define RADIO_SI5351_TX_JT4_COUNT 1
#define RADIO_SI5351_TX_WSPR false
#define RADIO_SI5351_TX_WSPR_COUNT 1
#define RADIO_SI5351_TX_FSQ false
#define RADIO_SI5351_TX_FSQ_COUNT 1
#define RADIO_SI5351_TX_FT8 false
#define RADIO_SI5351_TX_FT8_COUNT 1

// Transmit frequencies for the Si5351 transmitter modes
#define RADIO_SI5351_TX_FREQUENCY_CW         3595000UL
#define RADIO_SI5351_TX_FREQUENCY_PIP        3595000UL
#define RADIO_SI5351_TX_FREQUENCY_HORUS_V1   3608000UL
#define RADIO_SI5351_TX_FREQUENCY_HORUS_V2   3608000UL
#define RADIO_SI5351_TX_FREQUENCY_JT9        14085000UL    // Was: 14078700UL
#define RADIO_SI5351_TX_FREQUENCY_JT65       14085000UL    // Was: 14078300UL
#define RADIO_SI5351_TX_FREQUENCY_JT4        14085000UL    // Was: 14078500UL
#define RADIO_SI5351_TX_FREQUENCY_WSPR       14085000UL    // Was: 14097200UL
#define RADIO_SI5351_TX_FREQUENCY_FSQ        3608350UL    // Was: 7105350UL     // Base freq is 1350 Hz higher than dial freq in USB
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
// Generate an APRS weather report instead of a position report. This will override the APRS symbol with the weather station symbol.
#define APRS_WEATHER_REPORT_ENABLE false

// Schedule transmission every N seconds, counting from beginning of an hour (based on GPS time). Set to zero to disable time sync.
#define APRS_TIME_SYNC_SECONDS 0
// Delay transmission for an N second offset after the scheduled time.
#define APRS_TIME_SYNC_OFFSET_SECONDS 0

/**
 * Common Horus 4FSK mode settings
 */

#define HORUS_FREQUENCY_OFFSET_SI4032 0

/**
 * Horus V1 4FSK mode settings
 */

// Use Horus payload ID 0 for Horus V1 tests (4FSKTEST)
#define HORUS_V1_PAYLOAD_ID 0
#define HORUS_V1_BAUD_RATE_SI4032 100
#define HORUS_V1_BAUD_RATE_SI5351 50
#define HORUS_V1_PREAMBLE_LENGTH 16
#define HORUS_V1_IDLE_PREAMBLE_LENGTH 32
#define HORUS_V1_TONE_SPACING_HZ_SI5351 270

// Schedule transmission every N seconds, counting from beginning of an hour (based on GPS time). Set to zero to disable time sync.
#define HORUS_V1_TIME_SYNC_SECONDS 0
// Delay transmission for an N second offset after the scheduled time.
#define HORUS_V1_TIME_SYNC_OFFSET_SECONDS 0

/**
 * Horus V2 4FSK mode settings
 */

// Use Horus payload ID 256 for Horus V2 tests (4FSKTEST-V2)
#define HORUS_V2_PAYLOAD_ID 256
#define HORUS_V2_BAUD_RATE_SI4032 100
#define HORUS_V2_BAUD_RATE_SI5351 50
#define HORUS_V2_PREAMBLE_LENGTH 16
#define HORUS_V2_IDLE_PREAMBLE_LENGTH 32
#define HORUS_V2_TONE_SPACING_HZ_SI5351 270

// Schedule transmission every N seconds, counting from beginning of an hour (based on GPS time). Set to zero to disable time sync.
#define HORUS_V2_TIME_SYNC_SECONDS 0
// Delay transmission for an N second offset after the scheduled time.
#define HORUS_V2_TIME_SYNC_OFFSET_SECONDS 0

/**
 * CW settings
 */

// CW speed in WPM, range 5 - 40
#define CW_SPEED_WPM 20

// Schedule transmission every N seconds, counting from beginning of an hour (based on GPS time). Set to zero to disable time sync.
#define CW_TIME_SYNC_SECONDS 0
// Delay transmission for an N second offset after the scheduled time.
#define CW_TIME_SYNC_OFFSET_SECONDS 0

/**
 * Pip settings (short beep generated using CW to indicate presence of the transmitter)
 */

// Pip speed is defined as CW WPM, range 5 - 40
#define PIP_SPEED_WPM 18

// Schedule transmission every N seconds, counting from beginning of an hour (based on GPS time). Set to zero to disable time sync.
#define PIP_TIME_SYNC_SECONDS 0
// Delay transmission for an N second offset after the scheduled time.
#define PIP_TIME_SYNC_OFFSET_SECONDS 0

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

#define FSQ_SUBMODE RADIO_DATA_MODE_FSQ_3

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
