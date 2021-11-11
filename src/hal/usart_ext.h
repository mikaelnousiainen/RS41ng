#ifndef __USART_EXT_H
#define __USART_EXT_H

#include <stdint.h>
#include <stdbool.h>

void usart_ext_init(uint32_t baud_rate);
void usart_ext_uninit();
void usart_ext_enable(bool enabled);
void usart_ext_send_byte(uint8_t data);

#endif
