#include <string.h>

#include "horus_packet_v3.h"
#include "horus_l2.h"
#include "config.h"
#include "log.h"

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

    memset(&asnMessage, 0, sizeof(asnMessage));

    asnMessage = (horusTelemetry){
        .payloadCallsign  = HORUS_V3_PAYLOAD_CALLSIGN,
        .sequenceNumber = horus_v3_packet_counter,
        .timeOfDaySeconds  = data->gps.hours*3600 + data->gps.minutes*60 + data->gps.seconds,
        .latitude = (int32_t)(data->gps.latitude_degrees_10000000 / 100),
        .longitude = (int32_t)(data->gps.longitude_degrees_10000000 / 100),
        .altitudeMeters = ((data->gps.altitude_mm > 0 ? data->gps.altitude_mm : 0) / 1000),
        .velocityHorizontalKilometersPerHour = (velocity_horizontal > 255) ? 255 : (uint8_t)velocity_horizontal,
        .gnssSatellitesVisible = data->gps.satellites_visible,
        .ascentRateCentimetersPerSecond = (int16_t) data->gps.climb_cm_per_second, // m/s -> cm/s
        .temperatureCelsius_x10 = {
            .internal = (int16_t) (data->internal_temperature_celsius_100 / 10),
            .exist = {
                .internal = true,
                .external = false,
                .custom1 = false,
                .custom2 = false
            }
        },
        .milliVolts = {
            .battery = data->battery_voltage_millivolts,
            .exist = {
                .battery = true,
                .solar = false,
                .custom1 = false,
                .custom2 = false
            }
        },
        // We need to explicitly specify which optional fields we want to include in the packet
        .exist = {
            .extraSensors = false,
            .velocityHorizontalKilometersPerHour = true,
            .gnssSatellitesVisible = true,
            .ascentRateCentimetersPerSecond = true,
            .pressurehPa_x10 = false,
            .temperatureCelsius_x10 = true,
            .humidityPercentage = false,
            .milliVolts = true
        },
        .extraSensors = {
          .nCount=0, // this is incremented further down, depending on sensors enabled
          .arr = {
            
          }
        },
    };


    // External temperature & humidity sensors (typically BMP280 -- can expand to others)
    if (data->temperature_celsius_100 != 0 && data->humidity_percentage_100 != 0) {
        if (data->temperature_celsius_100 >= -10230 && data->temperature_celsius_100 <= 10230) {
            asnMessage.temperatureCelsius_x10.external = (int16_t)(data->temperature_celsius_100 / 10);
            asnMessage.temperatureCelsius_x10.exist.external = true;
        }

        if (data->humidity_percentage_100 >= 0 && data->humidity_percentage_100 <= 10000) {
            asnMessage.humidityPercentage = (uint8_t) (data->humidity_percentage_100 / 100);
            asnMessage.exist.humidityPercentage = true;
        }
    }

    if (data->pressure_mbar_100 > 0 && data->pressure_mbar_100 <= 12000) {
        asnMessage.pressurehPa_x10 = (uint16_t) (data->pressure_mbar_100 / 10);
        asnMessage.exist.pressurehPa_x10 = true;
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
                        .arr[0] = (uint16_t) data->radiation_intensity_uR_h
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
                        .arr[0] = (uint16_t) data->pulse_count
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
        horusAdditionalSensorType radio_capacitance_struct = {
            .name = "radiotrm",
            .exist = { 
                .name = 1, 
                .values = 1 
            },
            .values = {
                .kind = horusInt_PRESENT,
                .u = {
                    .horusInt = {
                        .nCount = 1,
                        .arr[0] = data->si4063_capacitance_trim
                    }   
                }
            }
        };
        asnMessage.extraSensors.arr[asnMessage.extraSensors.nCount] = radio_capacitance_struct;
        asnMessage.extraSensors.nCount += 1;
    }
    if (asnMessage.extraSensors.nCount < 4) {
        // Timepulse error in µs (delta_ticks - 1,000,000), biased +32767 for unsigned transport.
        // Decode: displayed - 32767 = signed µs error. Nominally 0 when PLL is locked.
        // If 0 (no GPS fix yet), the field is still transmitted so the absence of lock is visible.
        horusAdditionalSensorType us_error_struct = {
            .name = "usofs",
            .exist = {
                .name = 1,
                .values = 1
            },
            .values = {
                .kind = horusInt_PRESENT,
                .u = {
                    .horusInt = {
                        .nCount = 1,
                        .arr[0] = (uint16_t)(data->timepulse_error_us + 32767)
                    }
                }
            }
        };
        asnMessage.extraSensors.arr[asnMessage.extraSensors.nCount] = us_error_struct;
        asnMessage.extraSensors.nCount += 1;
    }
    if (asnMessage.extraSensors.nCount < 4) {
        // GPS-derived XO_TUNE signed offset (cap_trim_offset + 127 to make it unsigned).
        // Decode: gpsofs - 127 gives the signed correction applied on top of the 0x60 baseline.
        horusAdditionalSensorType gps_offset_struct = {
            .name = "gpsofs",
            .exist = {
                .name = 1,
                .values = 1
            },
            .values = {
                .kind = horusInt_PRESENT,
                .u = {
                    .horusInt = {
                        .nCount = 1,
                        .arr[0] = (uint16_t)(data->cap_trim_offset + 127)
                    }
                }
            }
        };
        asnMessage.extraSensors.arr[asnMessage.extraSensors.nCount] = gps_offset_struct;
        asnMessage.extraSensors.nCount += 1;
    }
#endif

#if 0
    // Unit: raw ADC
    asnMessage.exist.extraSensors = true;
    horusAdditionalSensorType button_adc_struct = {
        .name = "button",
        .exist = { 
            .name = 1, 
            .values = 1 
        },
        .values = {
            .kind = horusInt_PRESENT,
            .u = {
                .horusInt = {
                    .nCount = 1,
                    .arr[0] = (uint16_t) data->button_adc_value
                }   
            }
        }
    };
    asnMessage.extraSensors.arr[asnMessage.extraSensors.nCount] = button_adc_struct;
    asnMessage.extraSensors.nCount += 1;
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
