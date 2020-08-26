#ifndef __DELAY_H
#define __DELAY_H

#ifdef __cplusplus
extern "C" {
#endif

void delay_init();

void delay_us(uint16_t us);

void delay_ms(uint32_t ms);

#ifdef __cplusplus
}
#endif

#endif
