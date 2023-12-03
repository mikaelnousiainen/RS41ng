#include <stdio.h>

#include "config.h"
#include "log.h"
#include "strlcpy.h"
#include "template.h"
#include "hal/system.h"
#include "hal/delay.h"
#include "hal/usart_gps.h"
#include "codecs/morse/morse.h"
#include "codecs/bell/bell.h"
#include "codecs/mfsk/mfsk.h"
#include "codecs/jtencode/jtencode.h"
#include "drivers/ubxg6010/ubxg6010.h"
#include "radio.h"
#include "radio_internal.h"
#include "radio_si4032.h"
#include "radio_si5351.h"
#include "radio_payload_cw.h"
#include "radio_payload_aprs_position.h"
#include "radio_payload_aprs_weather.h"
#include "radio_payload_horus_v1.h"
#include "radio_payload_horus_v2.h"
#include "radio_payload_wspr.h"
#include "radio_payload_jtencode.h"
#include "radio_payload_fsq.h"

radio_transmit_entry radio_transmit_schedule[] = {
#if RADIO_SI4032_TX_HORUS_V1_CONTINUOUS
        {
                .enabled = RADIO_SI4032_TX_HORUS_V1,
                .radio_type = RADIO_TYPE_SI4032,
                .data_mode = RADIO_DATA_MODE_HORUS_V1,
                .time_sync_seconds = HORUS_V1_TIME_SYNC_SECONDS,
                .time_sync_seconds_offset = HORUS_V1_TIME_SYNC_OFFSET_SECONDS,
                .frequency = RADIO_SI4032_TX_FREQUENCY_HORUS_V1,
                .tx_power = RADIO_SI4032_TX_POWER,
                .symbol_rate = HORUS_V1_BAUD_RATE_SI4032,
                .payload_encoder = &radio_horus_v1_payload_encoder,
                .fsk_encoder_api = &mfsk_fsk_encoder_api,
        },
        {
                .enabled = RADIO_SI4032_TX_HORUS_V1,
                .radio_type = RADIO_TYPE_SI4032,
                .data_mode = RADIO_DATA_MODE_HORUS_V1,
                .time_sync_seconds = HORUS_V1_TIME_SYNC_SECONDS,
                .time_sync_seconds_offset = HORUS_V1_TIME_SYNC_OFFSET_SECONDS,
                .frequency = RADIO_SI4032_TX_FREQUENCY_HORUS_V1,
                .tx_power = RADIO_SI4032_TX_POWER,
                .symbol_rate = HORUS_V1_BAUD_RATE_SI4032,
                .payload_encoder = &radio_horus_v1_idle_encoder,
                .fsk_encoder_api = &mfsk_fsk_encoder_api,
        },
#elif RADIO_SI4032_TX_HORUS_V2_CONTINUOUS
        {
                .enabled = RADIO_SI4032_TX_HORUS_V2,
                .radio_type = RADIO_TYPE_SI4032,
                .data_mode = RADIO_DATA_MODE_HORUS_V2,
                .time_sync_seconds = HORUS_V2_TIME_SYNC_SECONDS,
                .time_sync_seconds_offset = HORUS_V2_TIME_SYNC_OFFSET_SECONDS,
                .frequency = RADIO_SI4032_TX_FREQUENCY_HORUS_V2,
                .tx_power = RADIO_SI4032_TX_POWER,
                .symbol_rate = HORUS_V2_BAUD_RATE_SI4032,
                .payload_encoder = &radio_horus_v2_payload_encoder,
                .fsk_encoder_api = &mfsk_fsk_encoder_api,
        },
        {
                .enabled = RADIO_SI4032_TX_HORUS_V2,
                .radio_type = RADIO_TYPE_SI4032,
                .data_mode = RADIO_DATA_MODE_HORUS_V2,
                .time_sync_seconds = HORUS_V2_TIME_SYNC_SECONDS,
                .time_sync_seconds_offset = HORUS_V2_TIME_SYNC_OFFSET_SECONDS,
                .frequency = RADIO_SI4032_TX_FREQUENCY_HORUS_V2,
                .tx_power = RADIO_SI4032_TX_POWER,
                .symbol_rate = HORUS_V2_BAUD_RATE_SI4032,
                .payload_encoder = &radio_horus_v2_idle_encoder,
                .fsk_encoder_api = &mfsk_fsk_encoder_api,
        },
#else
#if RADIO_SI4032_TX_PIP
        {
                .enabled = RADIO_SI4032_TX_PIP,
                .radio_type = RADIO_TYPE_SI4032,
                .data_mode = RADIO_DATA_MODE_PIP,
                .transmit_count = RADIO_SI4032_TX_PIP_COUNT,
                .time_sync_seconds = PIP_TIME_SYNC_SECONDS,
                .time_sync_seconds_offset = PIP_TIME_SYNC_OFFSET_SECONDS,
                .frequency = RADIO_SI4032_TX_FREQUENCY_PIP,
                .tx_power = RADIO_SI4032_TX_POWER,
                .symbol_rate = MORSE_WPM_TO_SYMBOL_RATE(PIP_SPEED_WPM),
                .payload_encoder = &radio_cw_payload_encoder,
                .fsk_encoder_api = &morse_fsk_encoder_api,
        },
#endif
#if RADIO_SI4032_TX_CW
        {
                .enabled = RADIO_SI4032_TX_CW,
                .radio_type = RADIO_TYPE_SI4032,
                .data_mode = RADIO_DATA_MODE_CW,
                .transmit_count = RADIO_SI4032_TX_CW_COUNT,
                .time_sync_seconds = CW_TIME_SYNC_SECONDS,
                .time_sync_seconds_offset = CW_TIME_SYNC_OFFSET_SECONDS,
                .frequency = RADIO_SI4032_TX_FREQUENCY_CW,
                .tx_power = RADIO_SI4032_TX_POWER,
                .symbol_rate = MORSE_WPM_TO_SYMBOL_RATE(CW_SPEED_WPM),
                .payload_encoder = &radio_cw_payload_encoder,
                .fsk_encoder_api = &morse_fsk_encoder_api,
        },
#endif
#if RADIO_SI4032_TX_APRS
        {
                .enabled = RADIO_SI4032_TX_APRS,
                .radio_type = RADIO_TYPE_SI4032,
                .data_mode = RADIO_DATA_MODE_APRS_1200,
                .transmit_count = RADIO_SI4032_TX_APRS_COUNT,
                .time_sync_seconds = APRS_TIME_SYNC_SECONDS,
                .time_sync_seconds_offset = APRS_TIME_SYNC_OFFSET_SECONDS,
                .frequency = RADIO_SI4032_TX_FREQUENCY_APRS_1200,
                .tx_power = RADIO_SI4032_TX_POWER,
                .symbol_rate = 1200,
#if APRS_WEATHER_REPORT_ENABLE
                .payload_encoder = &radio_aprs_weather_report_payload_encoder,
#else
                .payload_encoder = &radio_aprs_position_payload_encoder,
#endif
                .fsk_encoder_api = &bell_fsk_encoder_api,
        },
#endif
#if RADIO_SI4032_TX_HORUS_V1
        {
                .enabled = RADIO_SI4032_TX_HORUS_V1,
                .radio_type = RADIO_TYPE_SI4032,
                .data_mode = RADIO_DATA_MODE_HORUS_V1,
                .transmit_count = RADIO_SI4032_TX_HORUS_V1_COUNT,
                .time_sync_seconds = HORUS_V1_TIME_SYNC_SECONDS,
                .time_sync_seconds_offset = HORUS_V1_TIME_SYNC_OFFSET_SECONDS,
                .frequency = RADIO_SI4032_TX_FREQUENCY_HORUS_V1,
                .tx_power = RADIO_SI4032_TX_POWER,
                .symbol_rate = HORUS_V1_BAUD_RATE_SI4032,
                .payload_encoder = &radio_horus_v1_payload_encoder,
                .fsk_encoder_api = &mfsk_fsk_encoder_api,
        },
#endif
#if RADIO_SI4032_TX_HORUS_V2
        {
                .enabled = RADIO_SI4032_TX_HORUS_V2,
                .radio_type = RADIO_TYPE_SI4032,
                .data_mode = RADIO_DATA_MODE_HORUS_V2,
                .transmit_count = RADIO_SI4032_TX_HORUS_V2_COUNT,
                .time_sync_seconds = HORUS_V2_TIME_SYNC_SECONDS,
                .time_sync_seconds_offset = HORUS_V2_TIME_SYNC_OFFSET_SECONDS,
                .frequency = RADIO_SI4032_TX_FREQUENCY_HORUS_V2,
                .tx_power = RADIO_SI4032_TX_POWER,
                .symbol_rate = HORUS_V2_BAUD_RATE_SI4032,
                .payload_encoder = &radio_horus_v2_payload_encoder,
                .fsk_encoder_api = &mfsk_fsk_encoder_api,
        },
#endif
#if RADIO_SI5351_ENABLE
#if RADIO_SI5351_TX_PIP
        {
                .enabled = RADIO_SI5351_TX_PIP,
                .radio_type = RADIO_TYPE_SI5351,
                .data_mode = RADIO_DATA_MODE_PIP,
                .transmit_count = RADIO_SI5351_TX_PIP_COUNT,
                .time_sync_seconds = PIP_TIME_SYNC_SECONDS,
                .time_sync_seconds_offset = PIP_TIME_SYNC_OFFSET_SECONDS,
                .frequency = RADIO_SI5351_TX_FREQUENCY_PIP,
                .tx_power = RADIO_SI5351_TX_POWER,
                .symbol_rate = MORSE_WPM_TO_SYMBOL_RATE(PIP_SPEED_WPM),
                .payload_encoder = &radio_cw_payload_encoder,
                .fsk_encoder_api = &morse_fsk_encoder_api,
        },
#endif
#if RADIO_SI5351_TX_CW
        {
                .enabled = RADIO_SI5351_TX_CW,
                .radio_type = RADIO_TYPE_SI5351,
                .data_mode = RADIO_DATA_MODE_CW,
                .transmit_count = RADIO_SI5351_TX_CW_COUNT,
                .time_sync_seconds = CW_TIME_SYNC_SECONDS,
                .time_sync_seconds_offset = CW_TIME_SYNC_OFFSET_SECONDS,
                .frequency = RADIO_SI5351_TX_FREQUENCY_CW,
                .tx_power = RADIO_SI5351_TX_POWER,
                .symbol_rate = MORSE_WPM_TO_SYMBOL_RATE(CW_SPEED_WPM),
                .payload_encoder = &radio_cw_payload_encoder,
                .fsk_encoder_api = &morse_fsk_encoder_api,
        },
#endif
#if RADIO_SI5351_TX_HORUS_V1
        {
                .enabled = RADIO_SI5351_TX_HORUS_V1,
                .radio_type = RADIO_TYPE_SI5351,
                .data_mode = RADIO_DATA_MODE_HORUS_V1,
                .transmit_count = RADIO_SI5351_TX_HORUS_V1_COUNT,
                .time_sync_seconds = HORUS_V1_TIME_SYNC_SECONDS,
                .time_sync_seconds_offset = HORUS_V1_TIME_SYNC_OFFSET_SECONDS,
                .frequency = RADIO_SI5351_TX_FREQUENCY_HORUS_V1,
                .tx_power = RADIO_SI5351_TX_POWER,
                .symbol_rate = HORUS_V1_BAUD_RATE_SI5351,
                .payload_encoder = &radio_horus_v1_payload_encoder,
                .fsk_encoder_api = &mfsk_fsk_encoder_api,
        },
#endif
#if RADIO_SI5351_TX_HORUS_V2
        {
                .enabled = RADIO_SI5351_TX_HORUS_V2,
                .radio_type = RADIO_TYPE_SI5351,
                .data_mode = RADIO_DATA_MODE_HORUS_V2,
                .transmit_count = RADIO_SI5351_TX_HORUS_V2_COUNT,
                .time_sync_seconds = HORUS_V2_TIME_SYNC_SECONDS,
                .time_sync_seconds_offset = HORUS_V2_TIME_SYNC_OFFSET_SECONDS,
                .frequency = RADIO_SI5351_TX_FREQUENCY_HORUS_V2,
                .tx_power = RADIO_SI5351_TX_POWER,
                .symbol_rate = HORUS_V2_BAUD_RATE_SI5351,
                .payload_encoder = &radio_horus_v2_payload_encoder,
                .fsk_encoder_api = &mfsk_fsk_encoder_api,
        },
#endif
#if RADIO_SI5351_TX_WSPR
        {
                .enabled = RADIO_SI5351_TX_WSPR,
                .radio_type = RADIO_TYPE_SI5351,
                .data_mode = RADIO_DATA_MODE_WSPR,
                .transmit_count = RADIO_SI5351_TX_WSPR_COUNT,
                .time_sync_seconds = WSPR_TIME_SYNC_SECONDS,
                .time_sync_seconds_offset = WSPR_TIME_SYNC_OFFSET_SECONDS,
                .frequency = RADIO_SI5351_TX_FREQUENCY_WSPR,
                .tx_power = RADIO_SI5351_TX_POWER,
                .payload_encoder = &radio_wspr_payload_encoder,
                .fsk_encoder_api = &jtencode_fsk_encoder_api,
        },
#endif
#if RADIO_SI5351_TX_FT8
        {
                .enabled = RADIO_SI5351_TX_FT8,
                .radio_type = RADIO_TYPE_SI5351,
                .data_mode = RADIO_DATA_MODE_FT8,
                .transmit_count = RADIO_SI5351_TX_FT8_COUNT,
                .time_sync_seconds = FT8_TIME_SYNC_SECONDS,
                .time_sync_seconds_offset = FT8_TIME_SYNC_OFFSET_SECONDS,
                .frequency = RADIO_SI5351_TX_FREQUENCY_FT8,
                .tx_power = RADIO_SI5351_TX_POWER,
                .payload_encoder = &radio_ft8_payload_encoder,
                .fsk_encoder_api = &jtencode_fsk_encoder_api,
        },
#endif
#if RADIO_SI5351_TX_JT9
        {
                .enabled = RADIO_SI5351_TX_JT9,
                .radio_type = RADIO_TYPE_SI5351,
                .data_mode = RADIO_DATA_MODE_JT9,
                .transmit_count = RADIO_SI5351_TX_JT9_COUNT,
                .time_sync_seconds = JT9_TIME_SYNC_SECONDS,
                .time_sync_seconds_offset = JT9_TIME_SYNC_OFFSET_SECONDS,
                .frequency = RADIO_SI5351_TX_FREQUENCY_JT9,
                .tx_power = RADIO_SI5351_TX_POWER,
                .payload_encoder = &radio_jt9_payload_encoder,
                .fsk_encoder_api = &jtencode_fsk_encoder_api,
        },
#endif
#if RADIO_SI5351_TX_JT4
        {
                .enabled = RADIO_SI5351_TX_JT4,
                .radio_type = RADIO_TYPE_SI5351,
                .data_mode = RADIO_DATA_MODE_JT4,
                .transmit_count = RADIO_SI5351_TX_JT4_COUNT,
                .time_sync_seconds = JT4_TIME_SYNC_SECONDS,
                .time_sync_seconds_offset = JT4_TIME_SYNC_OFFSET_SECONDS,
                .frequency = RADIO_SI5351_TX_FREQUENCY_JT4,
                .tx_power = RADIO_SI5351_TX_POWER,
                .payload_encoder = &radio_jt4_payload_encoder,
                .fsk_encoder_api = &jtencode_fsk_encoder_api,
        },
#endif
#if RADIO_SI5351_TX_JT65
        {
                .enabled = RADIO_SI5351_TX_JT65,
                .radio_type = RADIO_TYPE_SI5351,
                .data_mode = RADIO_DATA_MODE_JT65,
                .transmit_count = RADIO_SI5351_TX_JT65_COUNT,
                .time_sync_seconds = JT65_TIME_SYNC_SECONDS,
                .time_sync_seconds_offset = JT65_TIME_SYNC_OFFSET_SECONDS,
                .frequency = RADIO_SI5351_TX_FREQUENCY_JT65,
                .tx_power = RADIO_SI5351_TX_POWER,
                .payload_encoder = &radio_jt65_payload_encoder,
                .fsk_encoder_api = &jtencode_fsk_encoder_api,
        },
#endif
#if RADIO_SI5351_TX_FSQ
        {
                .enabled = RADIO_SI5351_TX_FSQ,
                .radio_type = RADIO_TYPE_SI5351,
                .data_mode = FSQ_SUBMODE,
                .transmit_count = RADIO_SI5351_TX_FSQ_COUNT,
                .time_sync_seconds = FSQ_TIME_SYNC_SECONDS,
                .time_sync_seconds_offset = FSQ_TIME_SYNC_OFFSET_SECONDS,
                .frequency = RADIO_SI5351_TX_FREQUENCY_FSQ,
                .tx_power = RADIO_SI5351_TX_POWER,
                .payload_encoder = &radio_fsq_payload_encoder,
                .fsk_encoder_api = &jtencode_fsk_encoder_api,
        },
#endif
#endif
#endif
        {
                .end = true,
        }
};

static volatile bool radio_module_initialized = false;

const bool wspr_locator_fixed_enabled = WSPR_LOCATOR_FIXED_ENABLED;

radio_transmit_entry *radio_current_transmit_entry = NULL;
static uint8_t radio_current_transmit_entry_index = 0;
static uint8_t radio_transmit_entry_count = 0;

static volatile uint32_t radio_post_transmit_delay_counter = 0;
static volatile uint32_t radio_next_symbol_counter = 0;

static radio_transmit_entry *radio_start_transmit_entry = NULL;
static uint32_t radio_alternate_frequency_v1 = 1;
static uint32_t radio_alternate_frequency_v2 = 1;
static uint32_t radio_tx_watchdog_counter = 0;


static uint32_t radio_previous_time_sync_scheduled = 0;

char radio_current_payload_message[RADIO_PAYLOAD_MESSAGE_MAX_LENGTH];

uint8_t radio_current_payload[RADIO_PAYLOAD_MAX_LENGTH];
uint16_t radio_current_payload_length = 0;

uint8_t radio_current_symbol_data[RADIO_SYMBOL_DATA_MAX_LENGTH];

static volatile uint32_t start_tick = 0, end_tick = 0;

bool led_state_radio = true;

telemetry_data current_telemetry_data;

radio_module_state radio_shared_state = {
        .radio_transmission_active = false,
        .radio_transmission_finished = false,
        .radio_transmit_next_symbol_flag = false,
        .radio_symbol_count_interrupt = 0,
        .radio_symbol_count_loop = 0,
        .radio_dma_transfer_active = false,
        .radio_manual_transmit_active = false,
        .radio_interrupt_transmit_active = false,

        .radio_current_fsk_tones = NULL,
        .radio_current_fsk_tone_count = 0,
        .radio_current_tone_spacing_hz_100 = 0,

        .radio_current_symbol_rate = 0,
        .radio_current_symbol_delay_ms_100 = 0
};

static jtencode_mode_type radio_jtencode_mode_type_for(radio_data_mode mode)
{
    switch (mode) {
        case RADIO_DATA_MODE_WSPR:
            return JTENCODE_MODE_WSPR;
        case RADIO_DATA_MODE_FT8:
            return JTENCODE_MODE_FT8;
        case RADIO_DATA_MODE_JT9:
            return JTENCODE_MODE_JT9;
        case RADIO_DATA_MODE_JT65:
            return JTENCODE_MODE_JT65;
        case RADIO_DATA_MODE_JT4:
            return JTENCODE_MODE_JT4;
        case RADIO_DATA_MODE_FSQ_6:
            return JTENCODE_MODE_FSQ_6;
        case RADIO_DATA_MODE_FSQ_4_5:
            return JTENCODE_MODE_FSQ_4_5;
        case RADIO_DATA_MODE_FSQ_3:
            return JTENCODE_MODE_FSQ_3;
        case RADIO_DATA_MODE_FSQ_2:
            return JTENCODE_MODE_FSQ_2;
        default:
            return 0;
    }
}

static inline void radio_reset_next_symbol_counter()
{
    if (radio_shared_state.radio_current_symbol_rate > 0) {
        radio_next_symbol_counter = (uint32_t) (((float) SYSTEM_SCHEDULER_TIMER_TICKS_PER_SECOND) /
                                                (float) radio_shared_state.radio_current_symbol_rate);
    } else {
        radio_next_symbol_counter = (uint32_t) (((float) radio_shared_state.radio_current_symbol_delay_ms_100) *
                                                (float) SYSTEM_SCHEDULER_TIMER_TICKS_PER_SECOND / 100000.0f);
    }
}

static inline bool radio_should_transmit_next_symbol()
{
    return radio_next_symbol_counter == 0;
}

static bool radio_start_transmit(radio_transmit_entry *entry)
{
    bool success;

    radio_shared_state.radio_symbol_count_interrupt = 0;
    radio_shared_state.radio_symbol_count_loop = 0;

    telemetry_collect(&current_telemetry_data);
#ifdef SEMIHOSTING_ENABLE
        log_info("radio_start_transmit template\n");
#endif

    if (entry->messages != NULL && entry->message_count > 0) {
#ifdef SEMIHOSTING_ENABLE
        log_info("radio_start_transmit template\n");
#endif
        template_replace(radio_current_payload_message, sizeof(radio_current_payload_message),
                entry->messages[entry->current_message_index], &current_telemetry_data);
    } else {
#ifdef SEMIHOSTING_ENABLE
            log_info("radio_start_transmit no template filled\n");
#endif      
            radio_current_payload_message[0] = '\0';
    }

    radio_current_payload_length = entry->payload_encoder->encode(
            radio_current_payload, sizeof(radio_current_payload),
            &current_telemetry_data, radio_current_payload_message);

    log_info("Full payload length: %d\n", radio_current_payload_length);


#ifdef SEMIHOSTING_ENABLE
    log_info("Payload: ");
    log_bytes_hex(radio_current_payload_length, (char *) radio_current_payload);
    log_info("\n    ");
//    log_bytes(radio_current_payload_length, (char *) radio_current_payload);
//    log_info("\n");
//    log_info("Battery: %d mV\n", current_telemetry_data.battery_voltage_millivolts);
#endif

    // USART interrupts may interfere with transmission timing
    bool enable_gps_during_transmit = false;

    switch (entry->data_mode) {
        case RADIO_DATA_MODE_CW:
        case RADIO_DATA_MODE_PIP:
            // CW timing is not as critical
            enable_gps_during_transmit = true;

            morse_encoder_new(&entry->fsk_encoder, entry->symbol_rate);
            radio_shared_state.radio_current_symbol_rate = entry->fsk_encoder_api->get_symbol_rate(&entry->fsk_encoder);
            entry->fsk_encoder_api->get_tones(&entry->fsk_encoder, &radio_shared_state.radio_current_fsk_tone_count,
                    &radio_shared_state.radio_current_fsk_tones);
            entry->fsk_encoder_api->set_data(&entry->fsk_encoder, radio_current_payload_length, radio_current_payload);
            break;
        case RADIO_DATA_MODE_RTTY:
            break;
        case RADIO_DATA_MODE_APRS_1200:
            // TODO: make bell tones and flag field count configurable
            bell_encoder_new(&entry->fsk_encoder, entry->symbol_rate, BELL_FLAG_FIELD_COUNT_1200, bell202_tones);
            radio_shared_state.radio_current_symbol_rate = entry->fsk_encoder_api->get_symbol_rate(&entry->fsk_encoder);
            entry->fsk_encoder_api->get_tones(&entry->fsk_encoder, &radio_shared_state.radio_current_fsk_tone_count,
                    &radio_shared_state.radio_current_fsk_tones);
            entry->fsk_encoder_api->set_data(&entry->fsk_encoder, radio_current_payload_length, radio_current_payload);
            break;
        case RADIO_DATA_MODE_HORUS_V1:
            // GPS should not disturb the timing of Horus modes
            enable_gps_during_transmit = true;

            mfsk_encoder_new(&entry->fsk_encoder, MFSK_4, entry->symbol_rate, HORUS_V1_TONE_SPACING_HZ_SI5351 * 100);
            radio_shared_state.radio_current_symbol_rate = entry->fsk_encoder_api->get_symbol_rate(&entry->fsk_encoder);
            entry->fsk_encoder_api->get_tones(&entry->fsk_encoder, &radio_shared_state.radio_current_fsk_tone_count,
                    &radio_shared_state.radio_current_fsk_tones);
            radio_shared_state.radio_current_tone_spacing_hz_100 = entry->fsk_encoder_api->get_tone_spacing(
                    &entry->fsk_encoder);
            entry->fsk_encoder_api->set_data(&entry->fsk_encoder, radio_current_payload_length, radio_current_payload);
            break;
        case RADIO_DATA_MODE_HORUS_V2:
            // GPS should not disturb the timing of Horus modes
// WOHA - Temp false
            enable_gps_during_transmit = false;

            mfsk_encoder_new(&entry->fsk_encoder, MFSK_4, entry->symbol_rate, HORUS_V2_TONE_SPACING_HZ_SI5351 * 100);
            radio_shared_state.radio_current_symbol_rate = entry->fsk_encoder_api->get_symbol_rate(&entry->fsk_encoder);
            entry->fsk_encoder_api->get_tones(&entry->fsk_encoder, &radio_shared_state.radio_current_fsk_tone_count,
                    &radio_shared_state.radio_current_fsk_tones);
            radio_shared_state.radio_current_tone_spacing_hz_100 = entry->fsk_encoder_api->get_tone_spacing(
                    &entry->fsk_encoder);
            entry->fsk_encoder_api->set_data(&entry->fsk_encoder, radio_current_payload_length, radio_current_payload);
            break;
        case RADIO_DATA_MODE_WSPR:
        case RADIO_DATA_MODE_FT8:
        case RADIO_DATA_MODE_JT65:
        case RADIO_DATA_MODE_JT4:
        case RADIO_DATA_MODE_JT9:
        case RADIO_DATA_MODE_FSQ_2:
        case RADIO_DATA_MODE_FSQ_3:
        case RADIO_DATA_MODE_FSQ_4_5:
        case RADIO_DATA_MODE_FSQ_6: {
            // Timing of these slow modes is not as critical
            enable_gps_during_transmit = true;

            char locator[5];
            jtencode_mode_type jtencode_mode = radio_jtencode_mode_type_for(entry->data_mode);

            if (wspr_locator_fixed_enabled) {
                strlcpy(locator, WSPR_LOCATOR_FIXED, 4 + 1);
            } else {
                strlcpy(locator, current_telemetry_data.locator, 4 + 1);
            }

            success = jtencode_encoder_new(&entry->fsk_encoder, sizeof(radio_current_symbol_data),
                    radio_current_symbol_data, jtencode_mode, WSPR_CALLSIGN, locator, WSPR_DBM, FSQ_CALLSIGN_FROM);
            if (!success) {
                return false;
            }
            radio_shared_state.radio_current_symbol_delay_ms_100 = entry->fsk_encoder_api->get_symbol_delay(
                    &entry->fsk_encoder);
            radio_shared_state.radio_current_tone_spacing_hz_100 = entry->fsk_encoder_api->get_tone_spacing(
                    &entry->fsk_encoder);
            entry->fsk_encoder_api->set_data(&entry->fsk_encoder, radio_current_payload_length, radio_current_payload);
            break;
        }
        default:
            return false;
    }

    usart_gps_enable(enable_gps_during_transmit);
    if (!enable_gps_during_transmit) {
        ubxg6010_reset_parser();
    }
    // to be sure:
    success = false;

    switch (entry->radio_type) {
        case RADIO_TYPE_SI4032:
            success = radio_start_transmit_si4032(entry, &radio_shared_state);
            break;
        case RADIO_TYPE_SI5351:
            success = radio_start_transmit_si5351(entry, &radio_shared_state);
            break;
        default:
            return false;
    }

    if (!success) {
        usart_gps_enable(true);
        // TODO: stop transmit here
        return false;
    }

    log_info("TX start\n");

    if (leds_enabled) {
        set_red_led(true);
    }

    radio_shared_state.radio_transmission_active = true;

    return true;
}

static inline void radio_reset_transmit_state()
{
    radio_shared_state.radio_transmission_active = false;

    radio_next_symbol_counter = 0;
    radio_current_payload_length = 0;

    radio_shared_state.radio_current_fsk_tones = NULL;
    radio_shared_state.radio_current_fsk_tone_count = 0;
    radio_shared_state.radio_current_tone_spacing_hz_100 = 0;

    radio_shared_state.radio_current_symbol_rate = 0;
    radio_shared_state.radio_current_symbol_delay_ms_100 = 0;
}

static bool radio_stop_transmit(radio_transmit_entry *entry)
{
    bool success;

    switch (entry->radio_type) {
        case RADIO_TYPE_SI4032:
            success = radio_stop_transmit_si4032(entry, &radio_shared_state);
            break;
        case RADIO_TYPE_SI5351:
            success = radio_stop_transmit_si5351(entry, &radio_shared_state);
            break;
        default:
            return false;
    }

    radio_reset_transmit_state();

    radio_shared_state.radio_manual_transmit_active = false;
    radio_shared_state.radio_interrupt_transmit_active = false;
    radio_shared_state.radio_dma_transfer_active = false;

    switch (entry->data_mode) {
        case RADIO_DATA_MODE_CW:
        case RADIO_DATA_MODE_PIP:
            morse_encoder_destroy(&entry->fsk_encoder);
            break;
        case RADIO_DATA_MODE_RTTY:
            break;
        case RADIO_DATA_MODE_APRS_1200:
            bell_encoder_destroy(&entry->fsk_encoder);
            break;
        case RADIO_DATA_MODE_HORUS_V1:
        case RADIO_DATA_MODE_HORUS_V2:
            mfsk_encoder_destroy(&entry->fsk_encoder);
            break;
        case RADIO_DATA_MODE_WSPR:
        case RADIO_DATA_MODE_FT8:
        case RADIO_DATA_MODE_JT65:
        case RADIO_DATA_MODE_JT4:
        case RADIO_DATA_MODE_JT9:
        case RADIO_DATA_MODE_FSQ_2:
        case RADIO_DATA_MODE_FSQ_3:
        case RADIO_DATA_MODE_FSQ_4_5:
        case RADIO_DATA_MODE_FSQ_6:
            jtencode_encoder_destroy(&entry->fsk_encoder);
            break;
        default:
            return false;
    }

    usart_gps_enable(true);
    if (leds_enabled) {
        set_red_led(false);
    }

    return success;
}

static bool radio_transmit_symbol(radio_transmit_entry *entry)
{
    bool success;

    switch (entry->radio_type) {
        case RADIO_TYPE_SI4032:
            success = radio_transmit_symbol_si4032(entry, &radio_shared_state);
            break;
        case RADIO_TYPE_SI5351:
            success = radio_transmit_symbol_si5351(entry, &radio_shared_state);
            break;
        default:
            return false;
    }

    if (success) {
        radio_shared_state.radio_symbol_count_interrupt++;
    }

    return success;
}

static void radio_reset_transmit_delay_counter()
{
    radio_post_transmit_delay_counter = RADIO_POST_TRANSMIT_DELAY_MS * SYSTEM_SCHEDULER_TIMER_TICKS_PER_SECOND / 1000;
}

static void radio_next_transmit_entry()
{
    radio_current_transmit_entry->current_message_index =
            (radio_current_transmit_entry->current_message_index + 1) % radio_current_transmit_entry->message_count;

    radio_current_transmit_entry->current_transmit_index =
            (radio_current_transmit_entry->current_transmit_index + 1) % radio_current_transmit_entry->transmit_count;

    // Proceed to next transmit entry when transmit counter wraps
    if (radio_current_transmit_entry->current_transmit_index == 0) {
        do {
            radio_current_transmit_entry_index = (radio_current_transmit_entry_index + 1) % radio_transmit_entry_count;
            radio_current_transmit_entry = &radio_transmit_schedule[radio_current_transmit_entry_index];
            // If this queue entry is enabled and its HORUS_V2 and 2nd frequency enabled then rotate frequencies each TX
            if (radio_current_transmit_entry->enabled) {
              if (radio_current_transmit_entry->data_mode == RADIO_DATA_MODE_HORUS_V2) {
                if (RADIO_SI4032_TX_FREQUENCY2_HORUS_V2_ACTIV) {
                  if (radio_alternate_frequency_v2 == 0) {
                      radio_alternate_frequency_v2 = 1;
                      radio_current_transmit_entry->frequency = RADIO_SI4032_TX_FREQUENCY2_HORUS_V2;
                  } else {
                      radio_alternate_frequency_v2 = 0;
                      radio_current_transmit_entry->frequency = RADIO_SI4032_TX_FREQUENCY_HORUS_V2;
                  }
                }
              }
              if (radio_current_transmit_entry->data_mode == RADIO_DATA_MODE_HORUS_V1) {
                if (RADIO_SI4032_TX_FREQUENCY2_HORUS_V1_ACTIV) {
                  if (radio_alternate_frequency_v1 == 0) {
                      radio_alternate_frequency_v1 = 1;
                      radio_current_transmit_entry->frequency = RADIO_SI4032_TX_FREQUENCY2_HORUS_V1;
                  } else {
                      radio_alternate_frequency_v1 = 0;
                      radio_current_transmit_entry->frequency = RADIO_SI4032_TX_FREQUENCY_HORUS_V1;
                  }
                }
              }
            }
        } while (!radio_current_transmit_entry->enabled);
    }

    radio_reset_transmit_delay_counter();
}

void radio_handle_timer_tick()
{
    if (!radio_module_initialized || radio_shared_state.radio_dma_transfer_active
        || radio_shared_state.radio_manual_transmit_active || radio_shared_state.radio_interrupt_transmit_active) {
        return;
    }

    if (radio_shared_state.radio_transmission_active) {
        if (radio_next_symbol_counter > 0) {
            radio_next_symbol_counter--;
        }

        if (radio_should_transmit_next_symbol()) {
            radio_shared_state.radio_transmit_next_symbol_flag = true;
            radio_reset_next_symbol_counter();
        }
    } else {
        if (radio_post_transmit_delay_counter > 0) {
            // TODO: use power saving
            radio_post_transmit_delay_counter--;
        }
    }
}

// Called by Interrupt TIM2_IRQ
void radio_handle_data_timer_tick()
{
    radio_handle_data_timer_si4032();

    radio_handle_data_timer_si5351();
}

bool radio_handle_time_sync()
{
    // TODO: How to poll GPS time?
    // TODO: Get time at millisecond precision!
    // ubxg6010_request_gpstime();

    gps_data gps;
    ubxg6010_get_current_gps_data(&gps);

    if (!gps.updated) {
      // The GPS data has not been updated yet
        return false;
    }

    if (false && gps.fix < 3) {
        log_info("Skipping transmit entry that requires time sync, no GPS fix");
        radio_next_transmit_entry();
        return false;
    }

    uint32_t time_millis = gps.time_of_week_millis - (gps_time_leap_seconds * 1000);

    if (time_millis == radio_previous_time_sync_scheduled) {
        // The GPS chip has not provided an updated time yet for some reason
        // WOHA
        // log_info("@ time_millis same as last\n");
        return false;
    }

    uint32_t time_sync_offset_millis = radio_current_transmit_entry->time_sync_seconds_offset * 1000;

    // Only at start of week, millis can be lower then offset :-)
    if (time_millis < time_sync_offset_millis) {
        // WOHA
        // log_info("@ Time millis: %lu lower %lu\n", time_millis, time_sync_offset_millis);
        return false;
    }

    uint32_t time_sync_millis = radio_current_transmit_entry->time_sync_seconds * 1000;
        
    uint32_t time_with_offset_millis = time_millis - time_sync_offset_millis;
    uint32_t time_sync_period_millis = time_with_offset_millis % time_sync_millis;

    bool is_scheduled_time = time_sync_period_millis < RADIO_TIME_SYNC_THRESHOLD_MS;

//    log_info("---- Time with offset: %lu, sync period: %lu, scheduled: %d\n", time_with_offset_millis, time_sync_period_millis, is_scheduled_time);

    if (!is_scheduled_time) {
/*
        log_info("Time: %lu, GPS fix: %d - SyncWait every %d sec+offset %d\n", time_millis,
                gps.fix,
                radio_current_transmit_entry->time_sync_seconds,
                radio_current_transmit_entry->time_sync_seconds_offset);
*/
        return false;
    }
/*
    log_info("Time: %lu, GPS fix: %d, sync period: %lu - Schedule TX at %d sec+offset %d\n",
            time_millis, gps.fix,
            time_sync_period_millis, radio_current_transmit_entry->time_sync_seconds,
            radio_current_transmit_entry->time_sync_seconds_offset);
*/
    radio_previous_time_sync_scheduled = time_millis;

    return true;
}

void radio_handle_main_loop()
{
    bool time_sync_required = radio_current_transmit_entry->time_sync_seconds > 0;

    if (time_sync_required && !radio_shared_state.radio_transmission_active &&
        !radio_shared_state.radio_transmission_finished) {
        bool schedule_transmit = radio_handle_time_sync();
        if (!schedule_transmit) {
            // TODO: use power saving
            delay_ms(100);
            return;
        }
        radio_reset_transmit_delay_counter();
        radio_start_transmit_entry = radio_current_transmit_entry;
//        log_info("main_loop: oben\n");
    } else if (!radio_shared_state.radio_transmission_active && radio_post_transmit_delay_counter == 0) {
//        log_info("main_loop: unten\n");

        #if defined(SEMIHOSTING_ENABLE) && defined(LOGGING_ENABLE)
        telemetry_collect(&current_telemetry_data);
        log_info("Battery: %d mV.\n", current_telemetry_data.battery_voltage_millivolts);
        log_info("Internal temperature: %ld C\n", current_telemetry_data.internal_temperature_celsius_100 / 100);
        log_info("Time: %02d:%02d:%02d\n",
                current_telemetry_data.gps.hours, current_telemetry_data.gps.minutes,
                current_telemetry_data.gps.seconds);
        log_info("Fix: %d, Sats: %d, OK packets: %d, Bad packets: %d\n",
                current_telemetry_data.gps.fix, current_telemetry_data.gps.satellites_visible,
                current_telemetry_data.gps.ok_packets, current_telemetry_data.gps.bad_packets);
        log_info("Lat: %ld *1M, Lon: %ld *1M, Alt: %ld m\n",
                current_telemetry_data.gps.latitude_degrees_1000000 / 10,
                current_telemetry_data.gps.longitude_degrees_1000000 / 10,
                (current_telemetry_data.gps.altitude_mm / 1000) * 3280 / 1000);
        #endif

        radio_reset_transmit_delay_counter();
        radio_start_transmit_entry = radio_current_transmit_entry;
    }

    radio_handle_main_loop_si4032(radio_current_transmit_entry, &radio_shared_state);
    radio_handle_main_loop_si5351(radio_current_transmit_entry, &radio_shared_state);

    bool first_symbol = false;
    if (radio_start_transmit_entry != NULL) {
        log_info("Start TX\n");
        bool success = radio_start_transmit(radio_start_transmit_entry);
        start_tick = system_get_tick();

        radio_start_transmit_entry = NULL;
        if (!success) {
            log_info("main_loop: Fehler radio_start_transmit\n");
            radio_next_transmit_entry();
            return;
        }

        if (!radio_shared_state.radio_dma_transfer_active) {
            first_symbol = true;
            radio_shared_state.radio_transmit_next_symbol_flag = true;
        }
    }

    if (radio_shared_state.radio_transmission_active && radio_shared_state.radio_transmit_next_symbol_flag) {
        radio_shared_state.radio_transmit_next_symbol_flag = false;

        if (!radio_shared_state.radio_manual_transmit_active && !radio_shared_state.radio_interrupt_transmit_active) {
            bool success = radio_transmit_symbol(radio_current_transmit_entry);
            if (success) {
                if (first_symbol) {
                    radio_reset_next_symbol_counter();
                }
            } else {
                radio_shared_state.radio_transmission_finished = true;
            }
        }
    }

    if (radio_shared_state.radio_transmission_finished) {
        end_tick = system_get_tick();
        radio_stop_transmit(radio_current_transmit_entry);
        radio_shared_state.radio_transmission_finished = false;

        radio_next_transmit_entry();

        log_info("TX stop\n");

        // Watchdog: TX counter reset
        radio_reset_tx_watchdog_counter();
/*
        log_info("Symbol count (interrupt): %ld\n", radio_shared_state.radio_symbol_count_interrupt);
        log_info("Symbol count (loop): %ld\n", radio_shared_state.radio_symbol_count_loop);
        log_info("Total ticks: %ld\n", end_tick - start_tick);
        log_info("Next symbol counter: %ld\n", radio_next_symbol_counter);
        log_info("Symbol rate: %ld\n", radio_shared_state.radio_current_symbol_rate);
        log_info("Symbol delay: %ld\n", radio_shared_state.radio_current_symbol_delay_ms_100);
        log_info("Tone spacing: %ld\n", radio_shared_state.radio_current_tone_spacing_hz_100);
*/
    }
}


void radio_reset_tx_watchdog_counter ()
{
    radio_tx_watchdog_counter = 0;
}

uint32_t radio_Inc_tx_watchdog_counter ()
{
    radio_tx_watchdog_counter++;
    return radio_tx_watchdog_counter;
}

void radio_init()
{
    uint8_t count;
    for (count = 0; !radio_transmit_schedule[count].end; count++);
    radio_transmit_entry_count = count;

    memset(&current_telemetry_data, 0, sizeof(current_telemetry_data));

    telemetry_collect(&current_telemetry_data);

    for (uint8_t i = 0; i < radio_transmit_entry_count; i++) {
        radio_transmit_entry *entry = &radio_transmit_schedule[i];
        switch (entry->data_mode) {
            case RADIO_DATA_MODE_CW:
                entry->messages = cw_message_templates;
                break;
            case RADIO_DATA_MODE_PIP:
                entry->messages = pip_message_templates;
                break;
            case RADIO_DATA_MODE_APRS_1200:
                entry->messages = aprs_comment_templates;
                break;
            case RADIO_DATA_MODE_HORUS_V1:
            case RADIO_DATA_MODE_HORUS_V2:
                // No messages
                break;
            case RADIO_DATA_MODE_FT8:
            case RADIO_DATA_MODE_JT65:
            case RADIO_DATA_MODE_JT9:
            case RADIO_DATA_MODE_JT4:
                entry->messages = ftjt_message_templates;
                break;
            case RADIO_DATA_MODE_FSQ_6:
            case RADIO_DATA_MODE_FSQ_4_5:
            case RADIO_DATA_MODE_FSQ_3:
            case RADIO_DATA_MODE_FSQ_2:
                entry->messages = fsq_comment_templates;
                break;
            case RADIO_DATA_MODE_WSPR:
                // No messages
                break;
            default:
                break;
        }
        if (entry->messages != NULL) {
            for (entry->message_count = 0; entry->messages[entry->message_count] != NULL; entry->message_count++);
        }
        if (entry->transmit_count == 0) {
            entry->transmit_count = 1;
        }
    }

    radio_current_transmit_entry = &radio_transmit_schedule[radio_current_transmit_entry_index];

    while (!radio_current_transmit_entry->enabled) {
        radio_current_transmit_entry_index = (radio_current_transmit_entry_index + 1) % radio_transmit_entry_count;
        radio_current_transmit_entry = &radio_transmit_schedule[radio_current_transmit_entry_index];
    }

    radio_init_si4032();

    radio_module_initialized = true;
}
