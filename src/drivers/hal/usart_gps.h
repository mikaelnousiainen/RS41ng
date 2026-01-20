#ifndef __USART_GPS_H
#define __USART_GPS_H

#include <stdint.h>
#include <stdbool.h>

void usart_gps_init(uint32_t baud_rate, bool enable_irq);
void usart_gps_set_baud_rate(uint32_t baud_rate);
void usart_gps_uninit();
void usart_gps_enable(bool enabled);
void usart_gps_send_byte(uint8_t data);
void USART1_IRQHandler(UART_HandleTypeDef *huart);

extern void (*usart_gps_handle_incoming_byte)(uint8_t data);
extern volatile uint32_t gps_ints;

#endif
