#ifndef __RADSENS_HANDLER_H
#define __RADSENS_HANDLER_H

#include <stdbool.h>
#include "telemetry.h"

#ifdef __cplusplus
extern "C" {
#endif

bool radsens_handler_init();
bool radsens_read(uint16_t *pulse_count, float *dynamic_intensity, float *static_intensity);
bool radsens_read_telemetry(telemetry_data *data);

#ifdef __cplusplus
}
#endif

#endif
