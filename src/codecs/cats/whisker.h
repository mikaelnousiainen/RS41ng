#ifndef __WHISKER_H
#define __WHISKER_H

#include <stdint.h>

#include "cats.h"

void cats_append_identification_whisker(cats_packet *packet, char *callsign, uint8_t ssid, uint16_t icon);
void cats_append_comment_whisker(cats_packet *packet, char *message);

#endif
