#ifndef __FIFO_H
#define __FIFO_H

#include <stdint.h>

#include "codecs/fsk/fsk.h"

void fifo_encoder_new(fsk_encoder *encoder);
void fifo_encoder_destroy(fsk_encoder *encoder);

extern fsk_encoder_api fifo_fsk_encoder_api;

#endif
