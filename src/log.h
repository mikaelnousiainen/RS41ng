#ifndef __LOG_H
#define __LOG_H

#include "config.h"

#if defined(SEMIHOSTING_ENABLE) && defined(LOGGING_ENABLE)

#include <stdio.h>

#define log_error printf
#define log_warn printf
#define log_info printf
#define log_debug(...)
#define log_trace(...)

#else

#define log_error(...)
#define log_warn(...)
#define log_info(...)
#define log_debug(...)
#define log_trace(...)

#endif

void log_bytes(int length, char *data);
void log_bytes_hex(int length, char *data);

#endif
