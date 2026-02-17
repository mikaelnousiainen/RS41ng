#ifndef __UBXM10050_H
#define __UBXM10050_H

/**
 * Driver for u-blox M10 generation GPS chips (UBX-M10050, UBX-M10150).
 *
 * The M10 generation uses the Generation 10 configuration interface (protocol
 * version 34+). The key differences from Gen6/Gen8 (ubxg6010 driver) are:
 *
 *   - All configuration is done via CFG-VALSET (0x06 0x8A) with 32-bit key IDs.
 *     Legacy CFG-PRT, CFG-MSG, CFG-NAV5, CFG-RXM messages are NOT supported.
 *   - UART baud rate: key 0x40520001 (CFG-UART1-BAUDRATE), U4 value.
 *   - Message output rates: CFG-MSGOUT-* keys, U1 value per port.
 *   - Dynamic model: key 0x20110021 (CFG-NAVSPG-DYNMODEL), U1 value.
 *   - Power save: UBX-RXM-PMREQ (0x02 0x41) - same as M8, no CFG-RXM.
 *   - NAV-PVT (0x01 0x07) is the recommended primary message.
 *   - UBX frame format, checksum algorithm, and ACK/NAK are unchanged.
 *
 * Public API is intentionally identical to ubxg6010 so main.c only needs
 * a compile-time switch between the two drivers.
 */

#include <stdint.h>
#include <stdbool.h>

#include "src/gps.h"

bool ubxm10050_init(void);

bool ubxm10050_enable_power_save_mode(void);

void ubxm10050_request_gpstime(void);

bool ubxm10050_get_current_gps_data(gps_data *data);

void ubxm10050_handle_incoming_byte(uint8_t data);

void ubxm10050_reset_parser(void);

#endif /* __UBXM10050_H */
