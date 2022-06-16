#ifndef __RADIO_INTERNAL_H
#define __RADIO_INTERNAL_H

#include <stdint.h>

#include "payload.h"
#include "codecs/jtencode/jtencode.h"
#include "codecs/fsk/fsk.h"

typedef enum _radio_type {
    RADIO_TYPE_SI4032 = 1,
    RADIO_TYPE_SI5351,
} radio_type;

typedef enum _radio_data_mode {
    RADIO_DATA_MODE_CW = 1,
    RADIO_DATA_MODE_PIP,
    RADIO_DATA_MODE_RTTY,
    RADIO_DATA_MODE_APRS_1200,
    RADIO_DATA_MODE_HORUS_V1,
    RADIO_DATA_MODE_HORUS_V2,
    RADIO_DATA_MODE_WSPR,
    RADIO_DATA_MODE_FT8,
    RADIO_DATA_MODE_JT65,
    RADIO_DATA_MODE_JT4,
    RADIO_DATA_MODE_JT9,
    RADIO_DATA_MODE_FSQ_2,
    RADIO_DATA_MODE_FSQ_3,
    RADIO_DATA_MODE_FSQ_4_5,
    RADIO_DATA_MODE_FSQ_6,
} radio_data_mode;

typedef struct _radio_transmit_entry {
    bool enabled;
    bool end;

    radio_type radio_type;
    radio_data_mode data_mode;

    uint16_t time_sync_seconds;
    uint16_t time_sync_seconds_offset;

    uint32_t frequency;
    uint8_t tx_power;
    uint32_t symbol_rate;

    char **messages;
    uint8_t current_message_index;
    uint8_t message_count;

    uint8_t current_transmit_index;
    uint8_t transmit_count;

    payload_encoder *payload_encoder;
    fsk_encoder_api *fsk_encoder_api;

    fsk_encoder fsk_encoder;
} radio_transmit_entry;

typedef struct _radio_module_state {
    volatile bool radio_transmission_active;
    volatile bool radio_transmission_finished;

    volatile bool radio_transmit_next_symbol_flag;

    volatile uint32_t radio_symbol_count_interrupt;
    volatile uint32_t radio_symbol_count_loop;

    volatile bool radio_dma_transfer_active;
    volatile bool radio_manual_transmit_active;
    volatile bool radio_interrupt_transmit_active;

    fsk_tone *radio_current_fsk_tones;
    int8_t radio_current_fsk_tone_count;
    uint32_t radio_current_tone_spacing_hz_100;

    uint32_t radio_current_symbol_rate;
    uint32_t radio_current_symbol_delay_ms_100;
} radio_module_state;

extern radio_transmit_entry *radio_current_transmit_entry;
extern radio_module_state radio_shared_state;

#endif
