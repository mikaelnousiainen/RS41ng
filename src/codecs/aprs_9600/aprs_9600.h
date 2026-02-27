#ifndef __APRS_9600_H
#define __APRS_9600_H

#include <stdint.h>

uint16_t g3ruh_encode(uint8_t *ax25_frame, uint16_t ax25_length, uint8_t *output, uint16_t max_output_length);

#endif
