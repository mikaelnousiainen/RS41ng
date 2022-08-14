#include <stdint.h>
#include <stdbool.h>

bool pulse_counter_init();
uint16_t pulse_counter_get_counts();
void GPIOAinit_TIM2(void);
void TIM2init_counter(void);