#ifndef __ERRORS_H
#define __ERRORS_H

#include <stdint.h>

/*
 * System error codes for diagnostics.
 *
 * A single global error code (system_error_code) records the most recent error
 * encountered anywhere in the firmware. It is mirrored into telemetry_data by
 * telemetry_collect() and can be transmitted:
 *   - in text messages (APRS / CW / ...) via the $err template variable
 *     (rendered as a zero-padded decimal, e.g. "00", "23")
 *   - in Horus Binary v3 as an "err" extra sensor field when
 *     HORUS_V3_TX_ERROR_CODE is enabled
 *
 * Semantics: latest error wins (set_error_code overwrites). Recoverable
 * subsystems clear their own code on recovery via clear_error_code(), which
 * only resets to ERROR_NONE if the current code still belongs to that subsystem
 * (so it does not wipe a more recent error from another subsystem).
 *
 * Codes are grouped in blocks of 20 to leave room to expand each category.
 */

#define ERROR_NONE                  0

// 1-19: startup / initialization failures
#define ERROR_GPS_INIT              1
#define ERROR_BMP280_INIT           2
#define ERROR_BME68X_INIT           3
#define ERROR_BME690_INIT           4
#define ERROR_RADSENS_INIT          5
#define ERROR_SI5351_INIT           6

// 20-39: runtime sensor read failures (recoverable in flight)
#define ERROR_BMP280_READ           20
#define ERROR_BME68X_READ           21
#define ERROR_BME690_READ           22
#define ERROR_RADSENS_READ          23

// 40-59: telemetry encoding failures
#define ERROR_HORUS_V3_ENCODE       40
#define ERROR_HORUS_V3_EXT_ENCODE   41

// 60-79: radio / I2C low-level failures
#define ERROR_I2C_BUS_BUSY          60
#define ERROR_SI4063_TIMEOUT        61
#define ERROR_SI4063_POWERUP        62
#define ERROR_SI4063_PART_NUMBER    63
#define ERROR_RADIO_TX_TIMEOUT      64

// 80-99: GPS runtime failures
#define ERROR_GPS_CONFIG            80
#define ERROR_GPS_POWER_SAVE        81

#ifdef __cplusplus
extern "C" {
#endif

// Most recent error code seen anywhere in the firmware. ERROR_NONE when healthy.
extern volatile uint8_t system_error_code;

// Record an error (latest wins).
void set_error_code(uint8_t code);

// Clear an error on recovery: resets to ERROR_NONE only if the current code is
// still `code`, so a newer error from another subsystem is preserved.
void clear_error_code(uint8_t code);

#ifdef __cplusplus
}
#endif

#endif
