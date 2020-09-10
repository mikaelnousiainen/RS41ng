#ifndef __HAL_SYSTEM_H
#define __HAL_SYSTEM_H

#include "hal.h"

#define GPIO_PIN_LED_GREEN GPIO_Pin_7
#define GPIO_PIN_LED_RED GPIO_Pin_8

#define SYSTEM_SCHEDULER_TIMER_TICKS_PER_SECOND 10000

void system_init();
void system_shutdown();
uint32_t system_get_tick();
void system_disable_tick();
void system_enable_tick();
void system_disable_irq();
void system_enable_irq();
void system_set_green_led(bool enabled);
void system_set_red_led(bool enabled);
uint16_t system_get_battery_voltage_millivolts();
uint16_t system_get_button_adc_value();

extern void (*system_handle_timer_tick)();

#endif
