#ifndef __RAW_H
#define __RAW_H

#include <stdint.h>

#include "codecs/fsk/fsk.h"

void raw_encoder_new(fsk_encoder *encoder);
void raw_encoder_destroy(fsk_encoder *encoder);

extern fsk_encoder_api raw_fsk_encoder_api;

#endif
