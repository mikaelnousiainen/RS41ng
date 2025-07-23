#ifndef __SPI_H
#define __SPI_H

#include <stdbool.h>
#include <stdint.h>

void spi_init();

void spi_uninit();

void spi_send(uint8_t data);

uint8_t spi_receive();

uint8_t spi_read();

void spi_set_chip_select(GPIO_TypeDef *gpio_cs, uint16_t pin_cs, bool select);

uint8_t spi_send_and_receive(GPIO_TypeDef *gpio_cs, uint16_t pin_cs, uint16_t data);

#endif
