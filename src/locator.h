#ifndef __LOCATOR_H
#define __LOCATOR_H

#include <stdint.h>

void locator_from_lonlat(int32_t longitude, int32_t latitude, uint8_t pair_count, char *locator);

#endif
