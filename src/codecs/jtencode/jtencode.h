#ifndef __JTENCODE_H
#define __JTENCODE_H

#include "codecs/fsk/fsk.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum _jtencode_mode_type {
    JTENCODE_MODE_JT9 = 0,
    JTENCODE_MODE_JT65,
    JTENCODE_MODE_JT4,
    JTENCODE_MODE_WSPR,
    JTENCODE_MODE_FT8,
    JTENCODE_MODE_FSQ_2,
    JTENCODE_MODE_FSQ_3,
    JTENCODE_MODE_FSQ_4_5,
    JTENCODE_MODE_FSQ_6,
} jtencode_mode_type;

bool jtencode_encoder_new(fsk_encoder *encoder, size_t symbol_data_length, uint8_t *symbol_data,
        jtencode_mode_type mode_type, char *wspr_callsign, char *wspr_locator, uint8_t wspr_dbm,
        char *fsq_callsign_from);
void jtencode_encoder_destroy(fsk_encoder *encoder);

extern fsk_encoder_api jtencode_fsk_encoder_api;

#ifdef __cplusplus
}
#endif

#endif
