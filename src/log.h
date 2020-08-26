#ifndef __LOG_H
#define __LOG_H

#define SEMIHOSTING_ENABLE

#ifdef SEMIHOSTING_ENABLE

#include <stdio.h>

#define log_error(...)
#define log_warn(...)
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

#endif
