// Based on HamLib's locator routines
// OK1TE 2018-10
#ifndef __LOCATOR_H_
#define __LOCATOR_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

uint8_t longlat2locator(int32_t longitude, int32_t latitude, char locator[]);

#ifdef __cplusplus
}
#endif
#endif
