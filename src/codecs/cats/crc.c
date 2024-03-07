#include "crc.h"

uint16_t calc_crc(uint8_t *data, size_t length);

size_t cats_append_crc(uint8_t *data, size_t len)
{
    uint16_t crc = calc_crc(data, len);
    data[len++] = crc;
    data[len++] = crc >> 8;

    return len;
}

// https://stackoverflow.com/questions/69850602/converting-c-to-java-for-crc16-ibm-sdlc
uint16_t calc_crc(uint8_t *data, size_t length)
{
    uint8_t *ptr = data; 
    uint8_t crcbyte1 = 0xFF;
    uint8_t crcbyte2 = 0xFF;
    for (int i = 0; i < length; i++) {
        uint8_t r1 = *ptr++ ^ crcbyte2; 
        r1 = (r1 << 4) ^ r1;
        crcbyte2 = (r1 << 4) | (r1 >> 4); 
        crcbyte2 = (crcbyte2 & 0x0F ) ^ crcbyte1; 
        crcbyte1 = r1;
        r1 = (r1 << 3) | (r1 >> 5);
        crcbyte2 = crcbyte2 ^ (r1 & 0xF8);
        crcbyte1 = crcbyte1 ^ (r1 & 0x07);
    }
        
    return ~((crcbyte1 << 8) | crcbyte2);
}
