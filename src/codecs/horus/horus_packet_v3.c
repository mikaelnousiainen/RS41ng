#include <string.h>
#include "horus_packet_v3.h"
#include "horus_l2.h"
#include "config.h"
#include "log.h"

volatile uint16_t horus_v3_packet_counter = 0;

size_t horus_packet_v3_create(uint8_t *payload, telemetry_data *data){
    // Horus v3 packets are encoded using ASN1, and are encapsulated in packets
    // of sizes 32, 48, 64, 96 or 128 bytes (before coding)
    // The CRC16 for these packets is located at the *start* of the packet, still little-endian encoded

    // Increment packet count
    horus_v3_packet_counter++;

    gps_data *gps_data = &data->gps;

    horusTelemetry asnMessage = {
        .payloadCallsign  = HORUS_V3_PAYLOAD_CALLSIGN,
        .sequenceNumber = horus_v3_packet_counter,
        .timeOfDaySeconds  = gps_data->hours*3600 + gps_data->minutes*60 + gps_data->seconds,
        .latitude = (int)(gps_data->latitude_degrees_1000000),
        .longitude = (int)(gps_data->longitude_degrees_1000000),
        .altitudeMeters = ((gps_data->altitude_mm > 0 ? gps_data->altitude_mm : 0) / 1000),
        .velocityHorizontalKilometersPerHour = (uint8_t) ((float) gps_data->ground_speed_cm_per_second * 0.036),
        .gnssSatellitesVisible = gps_data->satellites_visible,
        .ascentRateCentimetersPerSecond = (int16_t) gps_data->climb_cm_per_second, // m/s -> cm/s
        .temperatureCelsius_x10 = {
            .internal = (int16_t) ((float) data->internal_temperature_celsius_100 / 10.0f),
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
        asnMessage.temperatureCelsius_x10.external = (int16_t) (data->temperature_celsius_100 / 10.0f);
        asnMessage.temperatureCelsius_x10.exist.external = true;

        asnMessage.humidityPercentage = (uint8_t) (data->humidity_percentage_100 / 100.0f);
        asnMessage.exist.humidityPercentage = true;
    }

    if (data->pressure_mbar_100 > 0) {
        asnMessage.pressurehPa_x10 = (uint16_t) (data->pressure_mbar_100 / 10.0f);
        asnMessage.exist.pressurehPa_x10 = true;
    }

    // Add radsens data to packet if enabled
#if SENSOR_RADSENS_ENABLE
    if (asnMessage.extraSensors.nCount < 4) {
        // Unit: µR/h
        asnMessage.exist.extraSensors = true;
        horusAdditionalSensorType radsens_struct = {
            .name = "radsens",
            .exist = true,
            .values = {
                .kind = horusInt_PRESENT,
                .u = {
                    .horusInt = (uint16_t) data->radiation_intensity_uR_h,
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
            .name = "radsens",
            .exist = true,
            .values = {
                .kind = horusInt_PRESENT,
                .u = {
                    .horusInt = (uint16_t) data->pulse_count,
                }
            }
        };
        asnMessage.extraSensors.arr[asnMessage.extraSensors.nCount] = pulse_count_struct;
        asnMessage.extraSensors.nCount += 1;
    }
#endif

    // The encoder needs a data structure for the serialization
    // Again - how much memory is allocated here?
    BitStream encodedMessage;

    // The Encoder may fail and update an error code
    int errCode;

    // Initialization associates the buffer to the bit stream
    // We want to write the uncoded message starting at 2 bytes into the message.

    BitStream_Init (&encodedMessage,
                    (unsigned char*)(payload+2),
                    HORUS_UNCODED_BUFFER_SIZE-1
    );
    // Originally this function call used a MUCH larger value for count
    //horusTelemetry_REQUIRED_BYTES_FOR_ENCODING);
    
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
