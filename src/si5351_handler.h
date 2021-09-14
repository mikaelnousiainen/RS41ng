#ifndef __SI5351_HANDLER_H
#define __SI5351_HANDLER_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum _si5351_clock_id {
    SI5351_CLOCK_CLK0,
    SI5351_CLOCK_CLK1,
    SI5351_CLOCK_CLK2,
    SI5351_CLOCK_CLK3,
    SI5351_CLOCK_CLK4,
    SI5351_CLOCK_CLK5,
    SI5351_CLOCK_CLK6,
    SI5351_CLOCK_CLK7,
} si5351_clock_id;

bool si5351_handler_init();
bool si5351_set_frequency(si5351_clock_id clock, uint64_t frequency_hz_100);
void si5351_output_enable(si5351_clock_id clock, bool enabled);
void si5351_set_drive_strength(si5351_clock_id clock, uint8_t drive);
bool si5351_set_frequency_fast(si5351_clock_id clock, uint64_t frequency_hz_100);
void si5351_output_enable_fast(si5351_clock_id clock, bool enabled);
void si5351_set_drive_strength_fast(si5351_clock_id clock, uint8_t drive);

#ifdef __cplusplus
}
#endif

#endif
