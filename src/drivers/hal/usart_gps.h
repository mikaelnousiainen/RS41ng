#ifndef __USART_GPS_H
#define __USART_GPS_H

#include <stdint.h>
#include <stdbool.h>

void usart_gps_init(uint32_t baud_rate, bool enable_irq);
void usart_gps_set_baud_rate(uint32_t baud_rate);
void usart_gps_uninit();
void usart_gps_enable(bool enabled);
void usart_gps_send_byte(uint8_t data);
void usart_gps_drain_dma(void);

extern void (*usart_gps_handle_incoming_byte)(uint8_t data, uint8_t reset);
extern volatile uint32_t gps_ints;
extern volatile uint32_t drain_interrupted;
extern volatile uint32_t drain_not_enabled;
extern volatile uint32_t drain_null_instance;
extern volatile uint32_t drain_dma_not_running;
extern volatile uint32_t drain_byte_calls;

#endif
