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

#ifndef __HORUS_PACKET_V3_H
#define __HORUS_PACKET_V3_H

#include <stdint.h>
#include <stdlib.h>
#include "telemetry.h"
#include "asn1/HorusBinaryV3.h"

size_t horus_packet_v3_create(uint8_t *payload, telemetry_data *data);

// Currently only NOHUB feature will trigger extensions,
#if HORUS_V3_NOHUB
// extensions enabled
#define HORUS_V3_EXTENSIONS 1
// Eventually we would count how many extension fields are active
#define HORUS_V3_EXTRA_FIELD_COUNT 0
#else
#define HORUS_V3_EXTENSIONS 0
#define HORUS_V3_EXTRA_FIELD_COUNT 0
#endif


#endif
