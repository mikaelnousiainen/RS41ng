// Placeholder sensor-boom calibration. This file is checked into git so the
// tree compiles out of the box with SENSOR_BOOM_ENABLE=false.
//
// To use the RS41 sensor boom:
//   1. Capture the sonde with radiosonde_auto_rx BEFORE flashing RS41ng and
//      keep the resulting *_subframe.bin.
//   2. Run: python3 tools/extract_rs41_cal.py <sonde>_subframe.bin
//      which writes src/sensor_cal.h (this file's real sibling).
//   3. Set SENSOR_BOOM_ENABLE true in config.h and rebuild.
// See docs/sensor-boom.md for details.

#ifndef SENSOR_CAL_H
#define SENSOR_CAL_H

// --- Tier 1 ---
#define SENSOR_BOOM_RF1         750.0f
#define SENSOR_BOOM_RF2        1100.0f
#define SENSOR_BOOM_CO1        { -243.911f, 0.187654f, 8.2e-06f }
#define SENSOR_BOOM_CALT1      { 1.0f, 0.0f, 0.0f }
#define SENSOR_BOOM_CALH0      45.9f
#define SENSOR_BOOM_CO2        { -243.911f, 0.187654f, 8.2e-06f }
#define SENSOR_BOOM_CALT2      { 1.0f, 0.0f, 0.0f }

// --- Tier 2 ---
#define SENSOR_BOOM_CF1        0.0f
#define SENSOR_BOOM_CF2        47.0f
#define SENSOR_BOOM_CALH1      0.0f
#define SENSOR_BOOM_MTXH       { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, \
                                 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, \
                                 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, \
                                 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, \
                                 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, \
                                 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, \
                                 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f }
#define SENSOR_BOOM_CORHP      { 0.0f, 0.0f, 0.0f }
#define SENSOR_BOOM_CORHT      { 0.0f, 0.0f, 0.0f, 0.0f, \
                                 0.0f, 0.0f, 0.0f, 0.0f, \
                                 0.0f, 0.0f, 0.0f, 0.0f }

#endif  // SENSOR_CAL_H
