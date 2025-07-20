#ifndef __USART_GPS_H
#define __USART_GPS_H

#include <stdint.h>
#include <stdbool.h>

void usart_gps_init(uint32_t baud_rate, bool enable_irq);
void usart_gps_uninit();
void usart_gps_enable(bool enabled);
void usart_gps_send_byte(uint8_t data);

extern void (*usart_gps_handle_incoming_byte)(uint8_t data);

#endif
