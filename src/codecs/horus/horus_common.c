#include "horus_common.h"

uint16_t calculate_crc16_checksum(char *string, int len)
{
    uint16_t crc = 0xffff;
    char i;
    int ptr = 0;
    while (ptr < len) {
        ptr++;
        crc = crc ^ (*(string++) << 8);
        for (i = 0; i < 8; i++) {
            if (crc & 0x8000)
                crc = (uint16_t)((crc << 1) ^ 0x1021);
            else
                crc <<= 1;
        }
    }
    return crc;
}
