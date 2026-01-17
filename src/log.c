#include "config.h"

#ifndef RS41_RSM4x4
    #include <stm32f1xx_hal.h>
#else
    #include <stm32l4xx_hal.h>
#endif

#include "log.h"

void log_bytes(int length, char *data)
{
    for (int i = 0; i < length; i++) {
        char c = data[i];
        if (c >= 0x20 && c <= 0x7e) {
            log_info("%c", c);
        } else {
            log_info(" [%02X] ", c);
        }
    }
}

void log_bytes_hex(int length, char *data)
{
    for (int i = 0; i < length; i++) {
        log_info("%02X ", data[i]);
    }
}

void hang_if_bad(char *routine_name, int hal_ok)
{
   if (hal_ok != HAL_OK) {
      log_info("%s failed\n",routine_name);
      while(1);
   } else {
      log_info("%s succeeded\n", routine_name);
   }
}
