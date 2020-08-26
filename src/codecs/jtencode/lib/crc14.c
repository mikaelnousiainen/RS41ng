/**
 * \file
 * Functions and types for CRC checks.
 *
 * Generated on Thu Dec  6 17:52:34 2018
 * by pycrc v0.9.1, https://pycrc.org
 * using the configuration:
 *  - Width         = 14
 *  - Poly          = 0x2757
 *  - XorIn         = Undefined
 *  - ReflectIn     = Undefined
 *  - XorOut        = Undefined
 *  - ReflectOut    = Undefined
 *  - Algorithm     = bit-by-bit
 */
#include "crc14.h"     /* include the header file generated with pycrc */
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

static crc_t crc_reflect(crc_t data, size_t data_len);



crc_t crc_reflect(crc_t data, size_t data_len)
{
    unsigned int i;
    crc_t ret;

    ret = data & 0x01;
    for (i = 1; i < data_len; i++) {
        data >>= 1;
        ret = (ret << 1) | (data & 0x01);
    }
    return ret;
}


crc_t crc_init(const crc_cfg_t *cfg)
{
    unsigned int i;
    bool bit;
    crc_t crc = cfg->xor_in;
    for (i = 0; i < 14; i++) {
        bit = crc & 0x01;
        if (bit) {
            crc = ((crc ^ 0x2757) >> 1) | 0x2000;
        } else {
            crc >>= 1;
        }
    }
    return crc & 0x3fff;
}


crc_t crc_update(const crc_cfg_t *cfg, crc_t crc, const void *data, size_t data_len)
{
    const unsigned char *d = (const unsigned char *)data;
    unsigned int i;
    bool bit;
    unsigned char c;

    while (data_len--) {
        if (cfg->reflect_in) {
            c = crc_reflect(*d++, 8);
        } else {
            c = *d++;
        }
        for (i = 0; i < 8; i++) {
            bit = crc & 0x2000;
            crc = (crc << 1) | ((c >> (7 - i)) & 0x01);
            if (bit) {
                crc ^= 0x2757;
            }
        }
        crc &= 0x3fff;
    }
    return crc & 0x3fff;
}


crc_t crc_finalize(const crc_cfg_t *cfg, crc_t crc)
{
    unsigned int i;
    bool bit;

    for (i = 0; i < 14; i++) {
        bit = crc & 0x2000;
        crc <<= 1;
        if (bit) {
            crc ^= 0x2757;
        }
    }
    if (cfg->reflect_out) {
        crc = crc_reflect(crc, 14);
    }
    return (crc ^ cfg->xor_out) & 0x3fff;
}
