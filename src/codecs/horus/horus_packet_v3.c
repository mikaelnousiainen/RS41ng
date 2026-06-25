/*
 * Horus Binary v3 packet assembler
 *
 * Credits:
 *  - Mark VK5QI
 *  - Michaela @xssfox
 *  - Andrew KE5GDB * 
 *  
 * See https://github.com/xssfox/horusbinaryv3/ for more info
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#include <string.h>

#include "horus_packet_v3.h"
#include "horus_l2.h"
#include "config.h"
#include "log.h"

#define CLAMP(x, lo, hi) ((x) < (lo) ? (lo) : ((x) > (hi) ? (hi) : (x)))

volatile uint16_t horus_v3_packet_counter = 0;
static horusTelemetry asnMessage;
static BitStream encodedMessage;

size_t horus_packet_v3_create(uint8_t *payload, telemetry_data *data){
    // Horus v3 packets are encoded using ASN1, and are encapsulated in packets
    // of sizes 32, 48, 64, 96 or 128 bytes (before coding)
    // The CRC16 for these packets is located at the *start* of the packet, still little-endian encoded

    // Increment packet count
    horus_v3_packet_counter++;

    uint32_t velocity_horizontal = (data->gps.ground_speed_cm_per_second * 36) / 1000;

    int32_t time_of_day = data->gps.hours*3600 + data->gps.minutes*60 + data->gps.seconds;
    int32_t lat = (int32_t)(data->gps.latitude_degrees_10000000 / 100);
    int32_t lon = (int32_t)(data->gps.longitude_degrees_10000000 / 100);
    int32_t alt = (int32_t)(data->gps.altitude_mm / 1000);

    memset(&asnMessage, 0, sizeof(asnMessage));

    asnMessage = (horusTelemetry){
        .extensionMarkerForASN1CC = HORUS_V3_EXTENSIONS,
        .fieldCount = HORUS_V3_EXTRA_FIELD_COUNT, // extension field count this will be number of extra extension fields -1 (the first one is free)
        .payloadCallsign  = HORUS_V3_PAYLOAD_CALLSIGN HORUS_V3_CALLSIGN_SUFFIX,
        .sequenceNumber = horus_v3_packet_counter, // uint16_t, naturally 0..65535
        .timeOfDaySeconds  = CLAMP(time_of_day, -1, 86400),
        .latitude = CLAMP(lat, -9000000, 9000000),
        .longitude = CLAMP(lon, -18000000, 18000000),
        .altitudeMeters = CLAMP(alt, -1000, 50000),
        .velocityHorizontalKilometersPerHour = CLAMP(velocity_horizontal, 0, 511),
        .gnssSatellitesVisible = CLAMP(data->gps.satellites_visible, 0, 31),
        .ascentRateCentimetersPerSecond = CLAMP((int32_t)data->gps.climb_cm_per_second, -32767, 32767),
        .temperatureCelsius_x10 = {
            .internal = CLAMP((int16_t)(data->internal_temperature_celsius_100 / 10), -1023, 1023),
            .exist = {
                .internal = true,
                .external = false,
                .custom1 = false,
                .custom2 = false
            }
        },
        .milliVolts = {
            .battery = CLAMP(data->battery_voltage_millivolts, 0, 16383),
            .exist = {
                .battery = true,
                .solar = false,
                .custom1 = false,
                .custom2 = false
            }
        },
        .gnssPowerSaveState = (horusGnssPowerSaveState)data->gps.power_safe_mode_state,
        // We need to explicitly specify which optional fields we want to include in the packet
        .exist = {
            .extraSensors = false,
            .velocityHorizontalKilometersPerHour = true,
            .gnssSatellitesVisible = true,
        #if GPS_POWER_SAVING_ENABLE
            .gnssPowerSaveState = true,
        #endif
            .ascentRateCentimetersPerSecond = true,
            .pressurehPa_x10 = false,
            .temperatureCelsius_x10 = true,
            .humidityPercentage = false,
            .milliVolts = true,
        #if HORUS_V3_EXTENSIONS
            .fieldCount = true, 
            // Note that HorusBinaryV3.c/h have been modified to add this exists field with as optional in horusTelemetry_Encode without causing an additional optional bitflag
            // This works around asn1scc not supporting extension markers
        #endif
        },
        .extraSensors = {
          .nCount=0, // this is incremented further down, depending on sensors enabled
          .arr = {
            
          }
        },
    };


    if(data->ext_sensor_type != NO_EXT_SENSOR) {
        if (data->temperature_celsius_100 >= -10230 && data->temperature_celsius_100 <= 10230) {
            asnMessage.temperatureCelsius_x10.external = CLAMP((int16_t)(data->temperature_celsius_100 / 10), -1023, 1023);
            asnMessage.temperatureCelsius_x10.exist.external = true;
        }

        if (data->humidity_percentage_100 >= 0 && data->humidity_percentage_100 <= 10000 && data->ext_sensor_type != SENSOR_BMP280) {
            asnMessage.humidityPercentage = CLAMP((uint8_t)(data->humidity_percentage_100 / 100), 0, 100);
            asnMessage.exist.humidityPercentage = true;
        }

        if (data->pressure_mbar_100 > 0 && data->pressure_mbar_100 <= 120000) {
            asnMessage.pressurehPa_x10 = CLAMP((uint16_t)(data->pressure_mbar_100 / 10), 0, 12000);
            asnMessage.exist.pressurehPa_x10 = true;
        }
    }

    // Add radsens data to packet if enabled
#if SENSOR_RADSENS_ENABLE
    if (asnMessage.extraSensors.nCount < 4) {
        // Unit: µR/h
        asnMessage.exist.extraSensors = true;
        horusAdditionalSensorType radsens_struct = {
            .name = "radsens",
            .exist = { 
                .name = 1, 
                .values = 1 
            },
            .values = {
                .kind = horusInt_PRESENT,
                .u = {
                    .horusInt = {
                        .nCount = 1,
                        .arr[0] = CLAMP((int64_t)data->radiation_intensity_uR_h, 0, UINT16_MAX)
                    }   
                }
            }
        };
        asnMessage.extraSensors.arr[asnMessage.extraSensors.nCount] = radsens_struct;
        asnMessage.extraSensors.nCount += 1;
    }
#endif

#if PULSE_COUNTER_ENABLE
    // Add pulse count data to packet if enabled
    if (asnMessage.extraSensors.nCount < 4) {
        // Unit: pulse count
        asnMessage.exist.extraSensors = true;
        horusAdditionalSensorType pulse_count_struct = {
            .name = "pulse",
            .exist = { 
                .name = 1, 
                .values = 1 
            },
            .values = {
                .kind = horusInt_PRESENT,
                .u = {
                    .horusInt = {
                        .nCount = 1,
                        .arr[0] = CLAMP((int64_t)data->pulse_count, 0, UINT16_MAX)
                    }   
                }
            }
        };
        asnMessage.extraSensors.arr[asnMessage.extraSensors.nCount] = pulse_count_struct;
        asnMessage.extraSensors.nCount += 1;
    }
#endif

// Add BME6XX gas data to packet if enabled
#if SENSOR_BME_6XX_GAS_MEASUREMENT
    if (asnMessage.extraSensors.nCount < 4) {
        // Unit: µR/h
        asnMessage.exist.extraSensors = true;
        horusAdditionalSensorType bme_gas_struct = {
            .name = "gas",
            .exist = { 
                .name = 1, 
                .values = 1 
            },
            .values = {
                .kind = horusReal_PRESENT,
                .u = {
                    .horusReal = {
                        .nCount = 1,
                        .arr[0] = (float) data->bme6xx_gas_r
                    }   
                }
            }
        };
        asnMessage.extraSensors.arr[asnMessage.extraSensors.nCount] = bme_gas_struct;
        asnMessage.extraSensors.nCount += 1;
    }
#endif

#if TX_DFM_ADDITIONAL_TELEM && defined(DFM17)
    asnMessage.exist.extraSensors = true;
    if (asnMessage.extraSensors.nCount < 4) {
        // GPS-derived XO_TUNE signed offset (cap_trim_offset + 127 to make it unsigned).
        // Decode: gpsofs - 127 gives the signed correction applied on top of the 0x60 baseline.
        horusAdditionalSensorType gps_offset_struct = {
            .name = "cal",
            .exist = {
                .name = 1,
                .values = 1
            },
            .values = {
                .kind = horusInt_PRESENT,
                .u = {
                    .horusInt = {
                        .nCount = 4,
                        .arr[0] = data->si4063_capacitance_trim,
                        .arr[1] = (int64_t)(data->cap_trim_offset),
                        .arr[2] = (int64_t)(data->timepulse_error_us),
                        .arr[3] = data->po_state
                    }
                }
            }
        };
        asnMessage.extraSensors.arr[asnMessage.extraSensors.nCount] = gps_offset_struct;
        asnMessage.extraSensors.nCount += 1;
    }
#endif

#if 0
// Reevaluate this at a later time -- something isn't scaled right
// #ifdef DFM17
    if (asnMessage.extraSensors.nCount < 4 && data->current_milliamps > 0) {
        asnMessage.exist.extraSensors = true;
        horusAdditionalSensorType current_struct = {
            .name = "cur",
            .exist = {
                .name = 1,
                .values = 1
            },
            .values = {
                .kind = horusInt_PRESENT,
                .u = {
                    .horusInt = {
                        .nCount = 1,
                        .arr[0] = CLAMP((int64_t)data->current_milliamps, 0, UINT16_MAX)
                    }
                }
            }
        };
        asnMessage.extraSensors.arr[asnMessage.extraSensors.nCount] = current_struct;
        asnMessage.extraSensors.nCount += 1;
    }
#endif

#ifdef DEBUG_TX_BUTTON_ADC
    // Unit: raw ADC
    if (asnMessage.extraSensors.nCount < 4) {
        asnMessage.exist.extraSensors = true;
        horusAdditionalSensorType button_adc_struct = {
            .name = "adcbutton",
            .exist = {
                .name = 1,
                .values = 2
            },
            .values = {
                .kind = horusInt_PRESENT,
                .u = {
                    .horusInt = {
                        .nCount = 1,
                        .arr[0] = CLAMP((int64_t)data->button_adc_value, 0, UINT16_MAX)
                    }
                }
            }
        };
        asnMessage.extraSensors.arr[asnMessage.extraSensors.nCount] = button_adc_struct;
        asnMessage.extraSensors.nCount += 1;
    }
#endif

    memset(&encodedMessage, 0, sizeof(encodedMessage));

    // The Encoder may fail and update an error code
    int errCode;

    // Initialization associates the buffer to the bit stream
    // We want to write the uncoded message starting at 2 bytes into the message.
    BitStream_Init (&encodedMessage,
                    (unsigned char*)(payload+2),
                    HORUS_UNCODED_BUFFER_SIZE-2);
    
    // Encode the message using uPER encoding rule

    // We patch in assert functionality in assert_override.h
    // Before running encode we set assert_value = 0
    // Then check the value in assert_value
    assert_value = 0;

    if (!horusTelemetry_Encode(&asnMessage,
                        &encodedMessage,
                        &errCode,
                        true) || assert_value != 0)
    {  
        // Not at this error helps that much in a flight, but it helps
        // us when debugging!   
        if(errCode > 0) {
            log_error("[error]: HORUS v3 Encoding Failed: %i\n", errCode);
        }
        if(assert_value != 0){
            log_error("[error]: HORUS v3 Assert Failure, maybe hit buffer size limit\n");
        }
        // Need to check what happens here.
        return 0;
    } else {
        // Encoding was successful!
        // Now we try to add extensions if we have any

        #if HORUS_V3_EXTENSIONS 
        horusExtensions asnExtensions = {
            #if HORUS_V3_NOHUB
            .via = Via_nohub, 
            #endif
            .exist = {
                #if HORUS_V3_NOHUB
                .via = true
                #endif
            }
        };

        if (!horusExtensions_Encode(&asnExtensions,
                        &encodedMessage,
                        &errCode,
                        true) || assert_value != 0)
        {  
            // Not at this error helps that much in a flight, but it helps
            // us when debugging!   
            if(errCode > 0) {
                log_error("[error]: HORUS v3 Extension Encoding Failed: %i\n", errCode);
            }
            if(assert_value != 0){
                log_error("[error]: HORUS v3 Extension Assert Failure, maybe hit buffer size limit\n");
            }
            // Need to check what happens here.
            return 0;
        }

        #endif

        // Now we need to figure out the required frame size, and add the CRC.
        int encodedSize = BitStream_GetLength(&encodedMessage);

        // Determine the required frame size.
        // Probably should do this from a list of valid sizes in a neater manner
        int frameSize = 128;
        if (encodedSize <= 30){
            frameSize = 32;
        } else if (encodedSize <= 46){
            frameSize = 48;
        } else if (encodedSize <= 62){
            frameSize = 64;
        } else if (encodedSize <= 94){
            frameSize = 96;
        } else if (encodedSize <= 126){
            frameSize = 128;
        }

        // Calculate CRC16 over the frame, starting at byte 2
        uint16_t packetCrc = (uint16_t)gen_crc16((unsigned char *)(payload + 2),
                                     frameSize - 2);
        // Write CRC into bytes 0–1 of the packet
        memcpy(payload, &packetCrc, sizeof(packetCrc));  // little‑endian on STM32

        log_info("HORUS v3 ASN1: %i Frame: %i\n", encodedSize, frameSize);

        return frameSize;
    }

    return 0;
}
