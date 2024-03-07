#ifndef __WHISKER_H
#define __WHISKER_H

#include <stdint.h>

#include "cats.h"
#include "../../gps.h"
#include "../../telemetry.h"

void cats_append_identification_whisker(cats_packet *packet, char *callsign, uint8_t ssid, uint16_t icon);
void cats_append_gps_whisker(cats_packet *packet, gps_data gps);
void cats_append_comment_whisker(cats_packet *packet, char *message);
void cats_append_node_info_whisker(cats_packet *packet, telemetry_data *data);

#endif
