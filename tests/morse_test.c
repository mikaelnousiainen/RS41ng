#include <stdio.h>
#include <string.h>

#include "codecs/morse/morse.h"
#include "config.h"

int main4(void)
{
    fsk_encoder morse;

    printf("%d\n", MORSE_WPM_TO_SYMBOL_RATE(20));
    printf("%d\n", MORSE_WPM_TO_SYMBOL_RATE(15));
    printf("%d\n", MORSE_WPM_TO_SYMBOL_RATE(10));

    char *input = "TEST T";

    morse_encoder_new(&morse, 25);

    morse_encoder_set_data(&morse, strlen(input), (uint8_t *) input);

    int8_t tone_index;

    while ((tone_index = morse_encoder_next_tone(&morse)) >= 0) {
        printf("CW: %d\n", tone_index);
    }

    morse_encoder_destroy(&morse);
}
