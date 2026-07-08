/*
 * Vaisala RS41 sensor-boom reader (temperature / humidity / pressure).
 *
 * Clean-room reimplementation: this module is written from independent, public
 * sources only -- it is NOT derived from the RS41-NFW source code (GPL-3.0).
 *
 * Sources used:
 *  - Boom measurement circuit, multiplexer truth table and STM32F100 pin map:
 *    bazjo/radiosonde_hardware (Vaisala_RS41 schematic + logic-analyzer capture).
 *  - PT1000 / humidity calibration maths: standard physics + the publicly
 *    documented RS41 PTU calibration format (bazjo/RS41_Decoding, rs1729/RS).
 *  - RPM411 pressure readout frame: own logic-analyzer / firmware captures.
 *  - STM32L412 (RSM4x4) GPIO numbers are hardware facts (verify on hardware).
 *
 * Licensed under GPL-2.0 (same as RS41ng).
 */

#ifndef __VAISALA_BOOM_H
#define __VAISALA_BOOM_H

#include <stdbool.h>
#include <stdint.h>
#include "telemetry.h"

// Boom measurement channels (see the multiplexer truth table in the schematic
// derivation). The temperature oscillator covers the first four; the humidity
// oscillator the last three.
typedef enum {
    BOOM_REF_R1 = 0,   // reference resistor 1   (SPST1)
    BOOM_REF_R2,       // reference resistor 2   (SPST2)
    BOOM_T_HEATER,     // humidity-module temp   (SPST3, "Hygro")
    BOOM_T_AIR,        // air temperature PT1000 (SPST4, "Temp")
    BOOM_HUMIDITY,     // humidity capacitor     (SPDT2, "Hygro")
    BOOM_REF_C_HI,     // reference capacitor    (SPDT1)
    BOOM_REF_C_LO,     // reference capacitor    (SPDT3)
    BOOM_CHANNEL_COUNT
} vaisala_boom_channel;

// Initialise the boom GPIOs (multiplexer + oscillator enables) and, when the
// RPM411 pressure board is used, its chip-select and measurement clock.
void vaisala_boom_init(void);

// Measure the ring-oscillator frequency (Hz) of one channel. Returns 0 on error.
float vaisala_boom_frequency(vaisala_boom_channel channel);

// Fill temperature / humidity / pressure into the telemetry struct.
// Returns true if at least one quantity was produced.
bool vaisala_boom_read(telemetry_data *data);

#endif
