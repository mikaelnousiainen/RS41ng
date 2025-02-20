#ifndef __CONFIG_H
#define __CONFIG_H

// Experimental support for Graw DFM-17 radiosondes added! Please test and report any issues!
// WARNING: Using a DFM-17 as a primary flight tracker is NOT recommended yet!
// NOTE: DFM-17 radiosondes require a GPS lock (and clear visibility to the sky) to calibrate its internal oscillator.
// DFM-17 transmissions, especially APRS, may not decode correctly because of incorrect timing before the internal oscillator has been calibrated.

// Define radiosonde type. Remove the "//" comment to select either RS41 or DFM17.
//#define RS41
//#define DFM17

#if !defined(RS41) && !defined(DFM17)
#error "No hardware type specified. Please define RS41 or DFM17."
#endif
#if defined(RS41) && defined(DFM17)
#error "Please define either RS41 or DFM17."
#endif

// Enable semihosting to receive debug logs during development
// See the README for details on how to set up debugging and debug logs with GDB
// NOTE: Semihosting has to be disabled when the radiosonde is not connected to an STM32 programmer dongle, otherwise the firmware will not run.
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

// Number of character pairs to include in locator
#define LOCATOR_PAIR_COUNT_FULL 6 // max. 6 (12 characters WWL)

// Delay after transmission for modes that do not use time synchronization. Zero delay allows continuous transmit mode for Horus V1 and V2.
#define RADIO_POST_TRANSMIT_DELAY_MS 1000

// Threshold for time-synchronized modes regarding how far from scheduled transmission time the transmission is still allowed
#define RADIO_TIME_SYNC_THRESHOLD_MS 2000

// Number of leap seconds to add to the raw GPS time reported by the GPS chip (see https://timetoolsltd.com/gps/what-is-gps-time/ for more info)
// This value is used by default, but if the received GPS data contains indication about leap seconds, that one is used instead.
#define GPS_TIME_LEAP_SECONDS 18

// Enable this setting to require 3D fix (altitude required, enable for airborne use), otherwise 2D fix is enough
#define GPS_REQUIRE_3D_FIX true

// Enable power-saving features of the GPS chip to save power.
// This option should be safe to enable, as it enters a selective power saving mode.
// If the GPS chip loses fix, it will enter a higher power state automatically.
// Note that power saving mode is only enabled after the GPS chip has acquired good GPS fix for the first time.
// It is not necessary to use power saving on short flights (e.g. less than 6 hours).
// Based on measurements Mark VK5QI, enabling this reduces power consumption by about 30-40 mA (~50%) to around 30-50 mA,
// where the consumption is 70-90 mA when power saving is not enabled and any radio transmitters are idle.
// See the README for details about power consumption.
#define GPS_POWER_SAVING_ENABLE false

// Enable NMEA output from GPS via external serial port. This disables use of I²C bus (Si5351 and sensors) because the pins are shared.
#define GPS_NMEA_OUTPUT_VIA_SERIAL_PORT_ENABLE false

#if (GPS_NMEA_OUTPUT_VIA_SERIAL_PORT_ENABLE) && ((RADIO_SI5351_ENABLE) || (SENSOR_BMP280_ENABLE))
#error GPS NMEA output via serial port cannot be enabled simultaneously with the I2C bus.
#endif

/**
 * Advanced GPS settings - Do not modify if you are not sure what you are doing!
 * The default settings are suitable for high-altitude balloon flights.
 */

// GPS Dynamic Platform model: (default: 6 - Airborne <1g)
// - 0 Portable
// - 2 Stationary
// - 3 Pedestrian
// - 4 Automotive
// - 5 Sea
// - 6 Airborne with <1g Acceleration
// - 7 Airborne with <2g Acceleration
// - 8 Airborne with <4g Acceleration
#define GPS_DYNAMIC_MODEL 6

// GPS Position Fixing Mode: (default: 2 - 3D only)
// - 1: 2D only
// - 2: 3D only
// - 3: Auto 2D/3D
#define GPS_POSITION_FIXING_MODE 2

// Measurement Rate in milliseconds (default: 1000)
// GPS measurements are taken every GPS_MEASUREMENT_RATE milliseconds
// The UBX-G6010 in the older RS41 radiosondes (PCB revisions 4x1 and 4x2) should support rates up to 5 Hz (200 ms).
// * Values < 200ms do not seem to work (not accepted by the GPS chip). 1000ms is a good value for most flights.
#define GPS_MEASUREMENT_RATE 1000

// Rate for GPS message updates sent by the GPS chip (default: 1, send message for every measurement done)
// Message rate is defined as the number of measurements between messages.
// For example:
// * Rate of 1 will lead to updates for every measurement (e.g. every second if measurement rate is set to 1000 ms)
// * Rate of 5 will lead to updates for every 5th measurement (e.g. every 5 seconds if measurement rate is set to 1000 ms)
#define GPS_POSITION_MESSAGE_RATE 1
#define GPS_TIME_MESSAGE_RATE 1

/**
 * RS41 only: Global configuration (there is no I²C bus exposed in DFM-17)
 */

// Define the I²C bus clock speed in Hz.
// The default of 100000 (= 100 kHz) should be used with the Si5351 clock generator to allow fast frequency changes.
// Note that some BMP280 sensors may require decreasing the clock speed to 10000 (= 10 kHz)
#define I2C_BUS_CLOCK_SPEED 100000

// Enable use of an externally connected I²C BMP280/BME280 atmospheric sensor
// NOTE: Only BME280 sensors will report humidity. For BMP280 humidity readings will be zero.
#define SENSOR_BMP280_ENABLE false
// BMP280/BME280 I²C device address is usually 0x76 or 0x77.
#define SENSOR_BMP280_I2C_ADDRESS 0x77

// Enable use of an externally connected I²C RadSens radiation sensor
#define SENSOR_RADSENS_ENABLE false
// Expected RadSens chip ID to verify initialization of the sensor, default is 0x7D.
#define SENSOR_RADSENS_CHIP_ID 0x7D
// RadSens I²C device address, default is 0x66.
#define SENSOR_RADSENS_I2C_ADDRESS 0x66
// Uncomment to set RadSens sensor sensitivity (imp/MKR). The default value is 105 imp/MKR.
// The value is stored in the non-volatile memory of the microcontroller.
#define SENSOR_RADSENS_SENSITIVITY 105

// Enable use of an externally connected I²C Si5351 clock generator chip for HF radio transmissions
#define RADIO_SI5351_ENABLE false

// Enable pulse counter via expansion header pin for use with devices like Geiger counters.
// This disables the external I²C bus and the serial port as the expansion header pin 2 (I2C2_SDA (PB11) / UART3 RX) is used for pulse input.
// Also changes the Horus 4FSK V2 data format and adds a custom data field for pulse count.
// The pulse count will wrap to zero at 65535 as it is stored as a 16-bit unsigned integer value.
#define PULSE_COUNTER_ENABLE false

// Pulse counter pin modes
#define PULSE_COUNTER_PIN_MODE_FLOATING 0
#define PULSE_COUNTER_PIN_MODE_INTERNAL_PULL_UP 1
#define PULSE_COUNTER_PIN_MODE_INTERNAL_PULL_DOWN 2

// Enable the internal pull-up or pull-down resistor on expansion header pin 2 (I2C2_SDA (PB11) / UART3 RX)
// This is necessary if the pulse counter needs to count pulses where the pin is pulled low (ground) or high (VCC) during the pulse.
// Set to "floating" if the circuit that generates the pulses already has a pull-up or a pull-down resistor.
#define PULSE_COUNTER_PIN_MODE PULSE_COUNTER_PIN_MODE_INTERNAL_PULL_UP

// Pulse counter interrupt edges
#define PULSE_COUNTER_INTERRUPT_EDGE_FALLING 1
#define PULSE_COUNTER_INTERRUPT_EDGE_RISING 2

// Set the edge of the pulse where the interrupt is triggered: falling or rising.
#define PULSE_COUNTER_INTERRUPT_EDGE PULSE_COUNTER_INTERRUPT_EDGE_FALLING

#if (PULSE_COUNTER_ENABLE) && ((GPS_NMEA_OUTPUT_VIA_SERIAL_PORT_ENABLE) || (RADIO_SI5351_ENABLE) || (SENSOR_BMP280_ENABLE))
#error Pulse counter cannot be enabled simultaneously with GPS NMEA output or I2C bus sensors.
#endif

/**
 * RS41 only: Built-in Si4032 radio chip transmission configuration
 */

// Si4032 transmit power: 0..7
// 0 = -1dBm, 1 = 2dBm, 2 = 5dBm (~3 mW), 3 = 8dBm (~6 mW), 4 = 11dBm (~12 mW), 5 = 14dBm (25 mW), 6 = 17dBm (50 mW), 7 = 20dBm (100 mW)
// This defaults to 5 (14 dBm, 25 mW), which is a good setting for Horus 4FSK transmissions and it saves power.
// For APRS usage, you might want to use maximum power setting of 7 (20 dBm, 100 mW). Note that this setting reduces battery life.
// See the README for details about power consumption.
#define RADIO_SI4032_TX_POWER 5

// Which modes to transmit using the built-in Si4032 transmitter chip
// The COUNT settings define the number of times that each type of transmission is repeated
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
#define RADIO_SI4032_TX_CATS false
#define RADIO_SI4032_TX_CATS_COUNT 1

// Continuous transmit mode can be enabled for *either* Horus V1 or V2, but not both. This disables all other transmission modes.
// The continuous mode transmits Horus 4FSK preamble between transmissions
// to allow Horus receivers to keep frequency synchronization at all times, which improves reception.
#define RADIO_SI4032_TX_HORUS_V1_CONTINUOUS false
#define RADIO_SI4032_TX_HORUS_V2_CONTINUOUS false

// Transmit frequencies for the Si4032 transmitter modes
#define RADIO_SI4032_TX_FREQUENCY_CW        432300000
#define RADIO_SI4032_TX_FREQUENCY_PIP       432300000
#define RADIO_SI4032_TX_FREQUENCY_APRS_1200 432500000
// Use a frequency offset to place FSK tones slightly above the defined frequency for SSB reception
#define RADIO_SI4032_TX_FREQUENCY_HORUS_V1  432301000
#define RADIO_SI4032_TX_FREQUENCY_HORUS_V2  432301000
#define RADIO_SI4032_TX_FREQUENCY_CATS      434100000

/**
 * DFM-17 only: Built-in Si4063 radio chip transmission configuration
 */

// Si4063 transmit power: 0..127
// TODO: Document Si4063 transmit power levels
#define RADIO_SI4063_TX_POWER 127

// Which modes to transmit using the built-in Si4063 transmitter chip
// The COUNT settings define the number of times that each type of transmission is repeated
#define RADIO_SI4063_TX_CW false
#define RADIO_SI4063_TX_CW_COUNT 1
#define RADIO_SI4063_TX_PIP false
#define RADIO_SI4063_TX_PIP_COUNT 6
#define RADIO_SI4063_TX_APRS true
#define RADIO_SI4063_TX_APRS_COUNT 2
#define RADIO_SI4063_TX_HORUS_V1 false
#define RADIO_SI4063_TX_HORUS_V1_COUNT 1
#define RADIO_SI4063_TX_HORUS_V2 true
#define RADIO_SI4063_TX_HORUS_V2_COUNT 6
#define RADIO_SI4063_TX_CATS false
#define RADIO_SI4063_TX_CATS_COUNT 1

// Continuous transmit mode can be enabled for *either* Horus V1 or V2, but not both. This disables all other transmission modes.
// The continuous mode transmits Horus 4FSK preamble between transmissions
// to allow Horus receivers to keep frequency synchronization at all times, which improves reception.
#define RADIO_SI4063_TX_HORUS_V1_CONTINUOUS false
#define RADIO_SI4063_TX_HORUS_V2_CONTINUOUS false

// Transmit frequencies for the Si4063 transmitter modes
#define RADIO_SI4063_TX_FREQUENCY_CW        432500000
#define RADIO_SI4063_TX_FREQUENCY_PIP       432500000
#define RADIO_SI4063_TX_FREQUENCY_APRS_1200 432500000
// Use a frequency offset to place FSK tones slightly above the defined frequency for SSB reception
#define RADIO_SI4063_TX_FREQUENCY_HORUS_V1  432501000
#define RADIO_SI4063_TX_FREQUENCY_HORUS_V2  432501000
#define RADIO_SI4063_TX_FREQUENCY_CATS      430500000

/**
 * RS41 only: External Si5351 radio chip transmission configuration
 */

// Si5351 transmit power: 0..3
// Si5351 drive strength: 0 = 2mA, 1 = 4mA, 2 = 6mA, 3 = 8mA
#define RADIO_SI5351_TX_POWER 3

// Which modes to transmit using an externally connected Si5351 chip in the I²C bus
// The COUNT settings define the number of times that each type of transmission is repeated
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
// Maximum length: depends on the packet contents, but keeping this under 100 characters is usually safe.
// Note that many hardware APRS receivers show a limited number of APRS comment characters, such as 43 or 67 chars.
#define APRS_COMMENT "RS41ng radiosonde firmware test"
#define APRS_RELAYS "WIDE1-1,WIDE2-1" // Do not include any spaces in the APRS_RELAYS
#define APRS_DESTINATION "APZ41N"
#define APRS_DESTINATION_SSID '0'
// Generate an APRS weather report instead of a position report. This will override the APRS symbol with the weather station symbol.
#define APRS_WEATHER_REPORT_ENABLE false

// Schedule transmission every N seconds, counting from beginning of an hour (based on GPS time). Set to zero to disable time sync.
// See the README file for more detailed documentation about time sync and its offset setting
#define APRS_TIME_SYNC_SECONDS 0
// Delay transmission for an N second offset, counting from the scheduled time set with TIME_SYNC_SECONDS.
#define APRS_TIME_SYNC_OFFSET_SECONDS 0

/**
 * Common Horus 4FSK mode settings
 */

#define HORUS_FREQUENCY_OFFSET_SI4032 0
#define HORUS_FREQUENCY_OFFSET_SI4063 0

/**
 * Horus V1 4FSK mode settings (deprecated, please use Horus V2 mode)
 */

// NOTE: Horus 4FSK V1 mode is deprecated in favor of Horus 4FSK V2 mode. All new Horus 4FSK payload IDs are allocated for V2 mode.
// NOTE: Payload ID 0 (4FSKTEST) is for testing purposes only, and should not be used on an actual flight.
// Please request a new payload ID in GitHub according to the instructions at: https://github.com/projecthorus/horusdemodlib/wiki#how-do-i-transmit-it
#define HORUS_V1_PAYLOAD_ID 0
#define HORUS_V1_BAUD_RATE_SI4032 100
#define HORUS_V1_BAUD_RATE_SI4063 100
#define HORUS_V1_BAUD_RATE_SI5351 50
#define HORUS_V1_PREAMBLE_LENGTH 16
#define HORUS_V1_IDLE_PREAMBLE_LENGTH 32
#define HORUS_V1_TONE_SPACING_HZ_SI5351 270

// Schedule transmission every N seconds, counting from beginning of an hour (based on GPS time). Set to zero to disable time sync.
// See the README file for more detailed documentation about time sync and its offset setting
#define HORUS_V1_TIME_SYNC_SECONDS 0
// Delay transmission for an N second offset, counting from the scheduled time set with TIME_SYNC_SECONDS.
#define HORUS_V1_TIME_SYNC_OFFSET_SECONDS 0

/**
 * Horus V2 4FSK mode settings
 */

// NOTE: Payload ID 256 (4FSKTEST-V2) is for testing purposes only, and should not be used on an actual flight.
// Please request a new payload ID in GitHub according to the instructions at: https://github.com/projecthorus/horusdemodlib/wiki#how-do-i-transmit-it
#define HORUS_V2_PAYLOAD_ID 256
#define HORUS_V2_BAUD_RATE_SI4032 100
#define HORUS_V2_BAUD_RATE_SI4063 100
#define HORUS_V2_BAUD_RATE_SI5351 50
#define HORUS_V2_PREAMBLE_LENGTH 16
#define HORUS_V2_IDLE_PREAMBLE_LENGTH 32
#define HORUS_V2_TONE_SPACING_HZ_SI5351 270

// Schedule transmission every N seconds, counting from beginning of an hour (based on GPS time). Set to zero to disable time sync.
// See the README file for more detailed documentation about time sync and its offset setting
#define HORUS_V2_TIME_SYNC_SECONDS 0
// Delay transmission for an N second offset, counting from the scheduled time set with TIME_SYNC_SECONDS.
#define HORUS_V2_TIME_SYNC_OFFSET_SECONDS 0

/**
 * CATS mode settings
 */
// CATS is a new digital mode, vaguely meant to be a better APRS (but also much more powerful)
// While it offers a number of advantages, probably the best one for balloons is the rapid beacon rate.
// The protocol is meant to allow for a much higher channel capacity, so beaconing every second is totally fine.
// For more information, see here: https://cats.radio/
#define CATS_CALLSIGN CALLSIGN
// The CATS SSID value is a number from 0 to 255, used to differentiate between stations with the same call sign.
// There is no special meaning for the numbers (compared to APRS SSIDs).
// If you're relying on APRS gating, be sure to set an SSID below 100 or the APRS network may reject it.
#define CATS_SSID 11
// CATS icon
// Some common icons: 0 = no icon, 2 = car, 3 = house, 13 = balloon, 14 = airplane, 18 = person, 21 = satellite, 22 = computer
// See the CATS standard for more options https://gitlab.scd31.com/cats/cats-standard/builds/artifacts/master/file/standard.pdf?job=build
#define CATS_ICON 13
// The maximum CATS comment length supported by RS41ng is about 100 characters. The CATS standard allows for up to 255 characters.
#define CATS_COMMENT "RS41ng radiosonde firmware test"
#define CATS_REPORTED_TX_POWER_DBM 17
// CATS is balloon - You probably want this to be true for a balloon payload
// Set to false if you're using your radiosonde for something other than a balloon payload
// We don't want non-balloons showing up as balloons on FELINET!
#define CATS_IS_BALLOON true

// Schedule transmission every N seconds, counting from beginning of an hour (based on GPS time). Set to zero to disable time sync.
// See the README file for more detailed documentation about time sync and its offset setting
#define CATS_TIME_SYNC_SECONDS 0
// Delay transmission for an N second offset, counting from the scheduled time set with TIME_SYNC_SECONDS.
#define CATS_TIME_SYNC_OFFSET_SECONDS 0

/**
 * CW settings
 */

// CW speed in WPM, range 5 - 40
#define CW_SPEED_WPM 20

// Schedule transmission every N seconds, counting from beginning of an hour (based on GPS time). Set to zero to disable time sync.
// See the README file for more detailed documentation about time sync and its offset setting
#define CW_TIME_SYNC_SECONDS 0
// Delay transmission for an N second offset, counting from the scheduled time set with TIME_SYNC_SECONDS.
#define CW_TIME_SYNC_OFFSET_SECONDS 0

/**
 * Pip settings (short beep generated using CW to indicate presence of the transmitter)
 */

// Pip speed is defined as CW WPM, range 5 - 40
#define PIP_SPEED_WPM 18

// Schedule transmission every N seconds, counting from beginning of an hour (based on GPS time). Set to zero to disable time sync.
// See the README file for more detailed documentation about time sync and its offset setting
#define PIP_TIME_SYNC_SECONDS 0
// Delay transmission for an N second offset, counting from the scheduled time set with TIME_SYNC_SECONDS.
#define PIP_TIME_SYNC_OFFSET_SECONDS 0

/**
 * WSPR settings
 */
#define WSPR_CALLSIGN CALLSIGN
#define WSPR_LOCATOR_FIXED_ENABLED false
#define WSPR_LOCATOR_FIXED "AA00"
#define WSPR_DBM 10

// Schedule transmission every N seconds, counting from beginning of an hour (based on GPS time). Set to zero to disable time sync.
// See the README file for more detailed documentation about time sync and its offset setting
#define WSPR_TIME_SYNC_SECONDS 120
// Delay transmission for an N second offset, counting from the scheduled time set with TIME_SYNC_SECONDS.
#define WSPR_TIME_SYNC_OFFSET_SECONDS 1

/**
 * FSQ settings
 */
#define FSQ_CALLSIGN_FROM CALLSIGN

#define FSQ_SUBMODE RADIO_DATA_MODE_FSQ_3

// Schedule transmission every N seconds, counting from beginning of an hour (based on GPS time). Set to zero to disable time sync.
// See the README file for more detailed documentation about time sync and its offset setting
#define FSQ_TIME_SYNC_SECONDS 0
// Delay transmission for an N second offset, counting from the scheduled time set with TIME_SYNC_SECONDS.
#define FSQ_TIME_SYNC_OFFSET_SECONDS 0

/**
 * FT/JT mode settings
 */

// Schedule transmission every 15 seconds, counting from beginning of an hour (based on GPS time). Set to zero to disable time sync.
#define FT8_TIME_SYNC_SECONDS 15
// Delay transmission for 1 second after the scheduled time.
#define FT8_TIME_SYNC_OFFSET_SECONDS 1

// Schedule transmission every N seconds, counting from beginning of an hour (based on GPS time). Set to zero to disable time sync.
// See the README file for more detailed documentation about time sync and its offset setting
#define JT9_TIME_SYNC_SECONDS 60
// Delay transmission for an N second offset, counting from the scheduled time set with TIME_SYNC_SECONDS.
#define JT9_TIME_SYNC_OFFSET_SECONDS 1

// Schedule transmission every N seconds, counting from beginning of an hour (based on GPS time). Set to zero to disable time sync.
// See the README file for more detailed documentation about time sync and its offset setting
#define JT4_TIME_SYNC_SECONDS 60
// Delay transmission for an N second offset, counting from the scheduled time set with TIME_SYNC_SECONDS.
#define JT4_TIME_SYNC_OFFSET_SECONDS 1

// Schedule transmission every N seconds, counting from beginning of an hour (based on GPS time). Set to zero to disable time sync.
// See the README file for more detailed documentation about time sync and its offset setting
#define JT65_TIME_SYNC_SECONDS 60
// Delay transmission for an N second offset, counting from the scheduled time set with TIME_SYNC_SECONDS.
#define JT65_TIME_SYNC_OFFSET_SECONDS 1

#include "config_internal.h"

#endif
