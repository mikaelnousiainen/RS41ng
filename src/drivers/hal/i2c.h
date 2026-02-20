#ifndef __HAL_I2C_H
#define __HAL_I2C_H

#ifdef RS41_RSM4x4
// STM32L412 has I2C1 and I2C3 only (no I2C2). PB10/PB11 AF4 = I2C3.
#define I2C_PORT I2C3
#define I2C_GPIO GPIOB
// PB10: I2C3_SCL (AF4)
#define I2C_PIN_SCL GPIO_PIN_10
// PB11: I2C3_SDA (AF4)
#define I2C_PIN_SDA GPIO_PIN_11
#else
#define I2C_PORT I2C2
#define I2C_PORT_RCC_PERIPH RCC_APB1Periph_I2C2
#define I2C_GPIO GPIOB
// PB10: I2C2_SCL/USART3_TX
#define I2C_PIN_SCL GPIO_PIN_10
// PB11: I2C2_SDA/USART3_RX
#define I2C_PIN_SDA GPIO_PIN_11
#endif

#include "hal.h"

typedef struct _i2c_port i2c_port;

extern i2c_port DEFAULT_I2C_PORT;

#ifdef __cplusplus
extern "C" {
#endif

void i2c_init(uint32_t clock_speed);
void i2c_uninit();
int i2c_read_bytes(struct _i2c_port *port, uint8_t address, uint8_t reg, uint8_t size, uint8_t *data);
int i2c_read_byte(struct _i2c_port *port, uint8_t address, uint8_t reg, uint8_t *data);
int i2c_write_bytes(struct _i2c_port *port, uint8_t address, uint8_t reg, uint8_t size, uint8_t *data);
int i2c_write_byte(struct _i2c_port *port, uint8_t address, uint8_t reg, uint8_t data);

#ifdef __cplusplus
};
#endif

#endif
