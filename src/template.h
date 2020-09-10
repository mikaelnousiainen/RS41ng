#ifndef __TEMPLATE_H
#define __TEMPLATE_H

#include <string.h>
#include "telemetry.h"

size_t template_replace(char *dest, size_t dest_len, char *src, telemetry_data *data);

#endif
