#ifndef __FSK_H
#define __FSK_H

#define FSK_TONE_COUNT_MAX 20

typedef struct _fsk_tone {
    int8_t index;
    uint32_t frequency_hz_100;
} fsk_tone;

typedef struct _fsk_encoder {
    void *priv;
} fsk_encoder;

typedef struct _fsk_encoder_api {
    /**
     * @param encoder
     * @param tone_count Set to number of tones in returned array or 0 if not used
     * @param tones Set to point to array of FSK tones or NULL if not used
     */
    void (*get_tones)(fsk_encoder *encoder, int8_t *tone_count, fsk_tone **tones);

    /**
     * @param encoder
     * @return Tone spacing in 1/100th of Hz or 0 if not used
     */
    uint32_t (*get_tone_spacing)(fsk_encoder *encoder);

    /**
     * @param encoder
     * @return Symbol rate in symbols per second or 0 if not used
     */
    uint32_t (*get_symbol_rate)(fsk_encoder *encoder);

    /**
     * @param encoder
     * @return Symbol delay in 1/100th of millisecond or 0 if not used
     */
    uint32_t (*get_symbol_delay)(fsk_encoder *encoder);

    void (*set_data)(fsk_encoder *encoder, uint16_t data_length, uint8_t *data);

    int8_t (*next_tone)(fsk_encoder *encoder);
} fsk_encoder_api;

#endif
