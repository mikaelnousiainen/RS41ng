#include "config.h"

bool bmp280_enabled = SENSOR_BMP280_ENABLE;
bool si5351_enabled = RADIO_SI5351_ENABLE;

volatile bool system_initialized = false;
