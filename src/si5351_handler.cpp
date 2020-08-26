#include "drivers/si5351/si5351.h"
#include "si5351_handler.h"

// TODO: click-free impl: https://github.com/pavelmc/Si5351mcu

Si5351 *si5351;

bool si5351_handler_init()
{
    si5351 = new Si5351(&DEFAULT_I2C_PORT);

    // Change the 2nd parameter in init if using a reference oscillator other than 25 MHz
    bool si5351_found = si5351->init(SI5351_CRYSTAL_LOAD_8PF, 0, 0);
    if (!si5351_found) {
        // TODO
    }

    return si5351_found;
}

bool si5351_set_frequency(si5351_clock_id clock, uint64_t frequency_hz_100)
{
    return si5351->set_freq(frequency_hz_100, (enum si5351_clock) clock) == 0;
}

void si5351_output_enable(si5351_clock_id clock, bool enabled)
{
    si5351->output_enable((enum si5351_clock) clock, enabled ? 1 : 0);
}

void si5351_set_drive_strength(si5351_clock_id clock, uint8_t drive)
{
    si5351_drive si5351_drive;

    switch (drive) {
        case 0:
            si5351_drive = SI5351_DRIVE_2MA;
            break;
        case 1:
            si5351_drive = SI5351_DRIVE_4MA;
            break;
        case 2:
            si5351_drive = SI5351_DRIVE_6MA;
            break;
        case 3:
            si5351_drive = SI5351_DRIVE_8MA;
            break;
        default:
            si5351_drive = SI5351_DRIVE_2MA;
    }

    si5351->drive_strength((enum si5351_clock) clock, si5351_drive);
}
