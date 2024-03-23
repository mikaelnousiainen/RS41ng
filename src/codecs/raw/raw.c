#include <stdlib.h>

#include "raw.h"

typedef struct _raw_encoder {
    uint8_t *data;
    uint16_t len;
} raw_encoder;

void raw_encoder_new(fsk_encoder *encoder)
{
    encoder->priv = malloc(sizeof(raw_encoder));
}

void raw_encoder_destroy(fsk_encoder *encoder)
{
    if (encoder->priv != NULL) {
        free(encoder->priv);
        encoder->priv = NULL;
    }
}

void raw_encoder_set_data(fsk_encoder *encoder, uint16_t data_length, uint8_t *data)
{
    raw_encoder *raw = (raw_encoder *) encoder->priv;
    raw->data = data;
    raw->len = data_length;
}

uint8_t *raw_encoder_get_data(fsk_encoder *encoder)
{
    raw_encoder *raw = (raw_encoder *) encoder->priv;
    return raw->data;
}

uint16_t raw_encoder_get_data_len(fsk_encoder *encoder)
{
    raw_encoder *raw = (raw_encoder *) encoder->priv;
    return raw->len;
}

fsk_encoder_api raw_fsk_encoder_api = {
        .set_data = raw_encoder_set_data,
        .get_data = raw_encoder_get_data,
        .get_data_len = raw_encoder_get_data_len,
};
