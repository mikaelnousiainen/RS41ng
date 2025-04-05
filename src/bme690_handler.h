#ifndef __BME690_HANDLER_H
#define __BME690_HANDLER_H

#include <stdbool.h>
#include "telemetry.h"

bool bme690_handler_init();
bool bme690_read(int32_t *temperature_celsius_100, uint32_t *pressure_mbar_100, uint32_t *humidity_percentage_100, uint16_t *air_quality_index);
bool bme690_read_telemetry(telemetry_data *data);

#endif
