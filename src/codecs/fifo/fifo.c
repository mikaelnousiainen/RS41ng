#include <stdlib.h>

#include "fifo.h"

typedef struct _fifo_encoder {
    uint8_t *data;
    uint16_t len;
} fifo_encoder;

void fifo_encoder_new(fsk_encoder *encoder)
{
    encoder->priv = malloc(sizeof(fifo_encoder));
}

void fifo_encoder_destroy(fsk_encoder *encoder)
{
    if (encoder->priv != NULL) {
        free(encoder->priv);
        encoder->priv = NULL;
    }
}

void fifo_encoder_set_data(fsk_encoder *encoder, uint16_t data_length, uint8_t *data)
{
    fifo_encoder *fifo = (fifo_encoder *) encoder->priv;
    fifo->data = data;
    fifo->len = data_length;
}

uint8_t *fifo_encoder_get_data(fsk_encoder *encoder)
{
    fifo_encoder *fifo = (fifo_encoder *) encoder->priv;
    return fifo->data;
}

uint16_t fifo_encoder_get_data_len(fsk_encoder *encoder)
{
    fifo_encoder *fifo = (fifo_encoder *) encoder->priv;
    return fifo->len;
}

fsk_encoder_api fifo_fsk_encoder_api = {
        .set_data = fifo_encoder_set_data,
        .get_data = fifo_encoder_get_data,
        .get_data_len = fifo_encoder_get_data_len,
};
