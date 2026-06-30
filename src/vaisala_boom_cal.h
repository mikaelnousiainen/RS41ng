/*
 * Per-sonde RS41 PTU factory calibration coefficients (calibration mode 2).
 *
 * These are SPECIFIC TO YOUR INDIVIDUAL RADIOSONDE. The values below are zeroed
 * placeholders; mode 2 produces valid readings only after you fill them in for
 * your sonde (obtain them from SondeHub by the boom serial number). Mode 1 (the
 * default) needs none of these.
 *
 * The calibration ALGORITHM is the publicly documented RS41 PTU method
 * (rs1729/RS get_T / get_RH2adv); these #defines are the per-sonde data it uses.
 * Clean-room: not derived from RS41-NFW. GPL-2.0.
 */

#ifndef __VAISALA_BOOM_CAL_H
#define __VAISALA_BOOM_CAL_H

// Reference resistors / capacitors (board constants, same for all sondes).
#define VBCAL_REF_R1   750.0f
#define VBCAL_REF_R2   1100.0f
#define VBCAL_REF_C1   0.0f
#define VBCAL_REF_C2   47.0f

// Air-temperature (PT1000): R = Rc * T_CAL;
//   T = (T_T0 + T_T1*R + T_T2*R^2 + T_POLY0) * (1 + T_POLY1)
#define VBCAL_T_T0     0.0f
#define VBCAL_T_T1     0.0f
#define VBCAL_T_T2     0.0f
#define VBCAL_T_CAL    0.0f
#define VBCAL_T_POLY0  0.0f
#define VBCAL_T_POLY1  0.0f

// Humidity-module (heater) temperature: same Taylor terms, own scale/poly.
#define VBCAL_TU_CAL   0.0f
#define VBCAL_TU_POLY0 0.0f
#define VBCAL_TU_POLY1 0.0f

// Humidity (mode 2, used in step "humidity factory"): capacitance normalisation
// + 7x6 calibration matrix (filled when the absolute-humidity mode is wired up).
#define VBCAL_H_U0     0.0f
#define VBCAL_H_U1     0.0f
#define VBCAL_H_MATRIX { \
    0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, \
    0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, \
    0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, \
    0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, \
    0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, \
    0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, \
    0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f }

#endif
