#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "codecs/morse/morse.h"
#include "config.h"

int main(void)
{
    fsk_encoder morse;

    // PARIS timing: symbol rate (ms per unit) derived from words-per-minute.
    printf("WPM rates: %d %d %d\n",
            MORSE_WPM_TO_SYMBOL_RATE(20), MORSE_WPM_TO_SYMBOL_RATE(15), MORSE_WPM_TO_SYMBOL_RATE(10));
    assert(MORSE_WPM_TO_SYMBOL_RATE(20) == 16);
    assert(MORSE_WPM_TO_SYMBOL_RATE(15) == 12);
    assert(MORSE_WPM_TO_SYMBOL_RATE(10) == 8);

    char *input = "TEST T";

    // Expected on/off keying for "TEST T":
    //   T = dah (111), E = dit (1), S = dit dit dit (10101), T = dah (111)
    //   inter-letter gap = 000, word gap = 0000000
    int8_t expected[] = {
            1, 1, 1,                // T
            0, 0, 0,                // letter gap
            1,                      // E
            0, 0, 0,                // letter gap
            1, 0, 1, 0, 1,          // S
            0, 0, 0,                // letter gap
            1, 1, 1,                // T
            0, 0, 0, 0, 0, 0, 0,    // word gap
            1, 1, 1,                // T
    };
    size_t expected_count = sizeof(expected) / sizeof(expected[0]);

    morse_encoder_new(&morse, 25);
    morse_encoder_set_data(&morse, strlen(input), (uint8_t *) input);

    int8_t tone_index;
    size_t i = 0;
    while ((tone_index = morse_encoder_next_tone(&morse)) >= 0) {
        printf("CW: %d\n", tone_index);
        assert(i < expected_count);
        assert(tone_index == expected[i]);
        i++;
    }
    assert(i == expected_count);

    morse_encoder_destroy(&morse);

    printf("All morse_test assertions passed\n");

    return 0;
}
