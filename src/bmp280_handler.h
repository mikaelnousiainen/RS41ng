#ifndef __BMP280_HANDLER_H
#define __BMP280_HANDLER_H

#include <stdbool.h>
#include "telemetry.h"

bool bmp280_handler_init();
bool bmp280_read(int32_t *temperature_celsius_100, uint32_t *pressure_mbar_100, uint32_t *humidity_percentage_100);
bool bmp280_read_telemetry(telemetry_data *data);

#endif
