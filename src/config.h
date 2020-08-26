#ifndef __CONFIG_H
#define __CONFIG_H

#include <stdbool.h>

#define RADIO_PAYLOAD_MAX_LENGTH 256

#define SENSOR_BMP280_ENABLE false

#define RADIO_SI5351_ENABLE true

#define RADIO_POST_TRANSMIT_DELAY 2000

// Si4032 transmit power: 0..100%
#define RADIO_SI4032_TX_POWER 100
#define RADIO_SI4032_TX_FREQUENCY_CW   432500000
#define RADIO_SI4032_TX_FREQUENCY_RTTY 434250000
#define RADIO_SI4032_TX_FREQUENCY_APRS 434250000

#define RADIO_SI5351_TX_POWER 100
#define RADIO_SI5351_TX_FREQUENCY_JT9        14078700UL
#define RADIO_SI5351_TX_FREQUENCY_JT65       14078300UL
#define RADIO_SI5351_TX_FREQUENCY_JT4        14078500UL
#define RADIO_SI5351_TX_FREQUENCY_WSPR       14085000UL     // Was: 14097200UL
#define RADIO_SI5351_TX_FREQUENCY_FSQ        7105350UL     // Base freq is 1350 Hz higher than dial freq in USB
#define RADIO_SI5351_TX_FREQUENCY_FT8        14085000UL    // Was: 14075000UL

#define WSPR_CALLSIGN "OH3BHX"
#define WSPR_LOCATOR "AA00"
#define WSPR_DBM 10

#define FT8_CALLSIGN "OH3BHX"
#define FT8_LOCATOR "AA00"

#define FSQ_CALLSIGN_FROM "OH3BHX"
#define FSQ_CALLSIGN_TO "N0CALL"
#define FSQ_COMMAND ' '

#define APRS_CALLSIGN "OH3BHX"
#define APRS_SSID 13
#define APRS_SYMBOL 'O'
#define APRS_COMMENT "Testing 123"
#define APRS_RELAYS "WIDE1-1,WIDE2-1"
#define APRS_DESTINATION "APZ41N"
#define APRS_DESTINATION_SSID 0

#define PAIR_COUNT 4 // max. 6 (12 characters WWL)
#define RTTY_7BIT   1 // if 0 --> 5 bits

extern bool bmp280_enabled;
extern bool si5351_enabled;

extern volatile bool system_initialized;

#endif
