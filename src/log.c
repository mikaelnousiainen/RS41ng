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
