#include <stdio.h>

#include "hal/delay.h"
#include "drivers/si5351/si5351.h"
#include "si5351_test.h"

int si5351_test()
{
    Si5351 si5351 = Si5351(&DEFAULT_I2C_PORT);

    printf("Si5351 init\n");
    bool i2c_found = si5351.init(SI5351_CRYSTAL_LOAD_8PF, 0, 0);
    if (!i2c_found) {
        printf("Si5351 not found\n");
    }

    printf("Si5351 set freq 1\n");

    // Set CLK0 to output 14 MHz
    si5351.set_freq(1400000000ULL, SI5351_CLK0);

    // Set CLK1 to output 175 MHz
    printf("Si5351 set ms source\n");
    //si5351.set_ms_source(SI5351_CLK1, SI5351_PLLB);

    printf("Si5351 set freq 2\n");
    //si5351.set_freq_manual(17500000000ULL, 70000000000ULL, SI5351_CLK1);

    printf("Si5351 update status\n");
    // Query a status update and wait a bit to let the Si5351 populate the
    // status flags correctly.
    si5351.update_status();

    delay_ms(500);

    printf("Loop\n");

    int counter = 0;
    while (true) {
        si5351.update_status();
        printf("SYS_INIT: %d, LOL_A: %d, LOL_B: %d, LOS: %d, REVID: %d\n",
                si5351.dev_status.SYS_INIT, si5351.dev_status.LOL_A, si5351.dev_status.LOL_B,
                si5351.dev_status.LOS, si5351.dev_status.REVID);
        delay_ms(1000);
        si5351.set_freq(1400000000ULL + counter * 10 * 100, SI5351_CLK0);
        //si5351.set_freq_manual(17500000000ULL + counter * 10, 70000000000ULL, SI5351_CLK1);
        counter++;
    }
}
