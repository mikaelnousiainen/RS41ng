#include "config.h"

#ifdef RS41_RSM4x4
#include <stm32l4xx_hal.h>
#else
#include <stm32f1xx_hal.h>
#endif


#include "hal.h"
#include "i2c.h"
#include "delay.h"
#include "log.h"

#define I2C_TIMEOUT_COUNTER 0x4FFFF

I2C_HandleTypeDef hi2c2;

struct _i2c_port {
    I2C_TypeDef *i2c;
};

i2c_port DEFAULT_I2C_PORT = {
        .i2c = I2C_PORT,
};

void i2c_init(uint32_t clock_speed)
{
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_I2C2_CLK_ENABLE();

    GPIO_InitTypeDef gpio_init;

    gpio_init.Pin = I2C_PIN_SCL;
    gpio_init.Mode = GPIO_MODE_AF_OD;
    gpio_init.Pull = GPIO_PULLUP;
    gpio_init.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(I2C_GPIO, &gpio_init);

    gpio_init.Pin = I2C_PIN_SDA;
    gpio_init.Mode = GPIO_MODE_AF_OD;
    gpio_init.Pull = GPIO_PULLUP;
    gpio_init.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(I2C_GPIO, &gpio_init);

    // NOTE: I2C chip reset is necessary here!
    __HAL_RCC_I2C2_FORCE_RESET();
    delay_ms(2);
    __HAL_RCC_I2C2_RELEASE_RESET();
    delay_ms(2);

    hi2c2.Instance = I2C2;
    hi2c2.Init.ClockSpeed = clock_speed;
    hi2c2.Init.DutyCycle = I2C_DUTYCYCLE_2;
    hi2c2.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
    hi2c2.Init.OwnAddress1     = 0;
    hi2c2.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
    hi2c2.Init.OwnAddress2     = 0;
    hi2c2.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
    hi2c2.Init.NoStretchMode   = I2C_NOSTRETCH_DISABLE;

    HAL_I2C_Init(&hi2c2);

    // This loop may get stuck if there is nothing connected in the I²C port pins
    int count = 1000;
    while (__HAL_I2C_GET_FLAG(&hi2c2, I2C_FLAG_BUSY) && count--) {
        delay_ms(1);
    }
    if (count == 0) {
        log_error("ERROR: I²C bus busy during initialization\n");
    }
}

void i2c_uninit()
{
    HAL_I2C_DeInit(&hi2c2);
    __HAL_RCC_I2C2_FORCE_RESET();
    delay_ms(2);
    __HAL_RCC_I2C2_RELEASE_RESET();
}


int i2c_read_bytes(i2c_port *port, uint8_t address, uint8_t reg, uint8_t size, uint8_t *data)
{
    if(HAL_I2C_Mem_Read(&hi2c2, ((uint16_t)address << 1), (uint16_t)reg, 1, data, (uint16_t)size, 1000) != HAL_OK)
    {
        HAL_I2C_DeInit(&hi2c2);
        HAL_I2C_Init(&hi2c2);
        return HAL_ERROR;
    }
    return HAL_OK;
}

int i2c_read_byte(i2c_port *port, uint8_t address, uint8_t reg, uint8_t *data)
{
    return i2c_read_bytes(port, address, reg, 1, data);
}

int i2c_write_bytes(i2c_port *port, uint8_t address, uint8_t reg, uint8_t size, uint8_t *data)
{
    if(HAL_I2C_Mem_Write(&hi2c2,  ((uint16_t)address << 1), (uint16_t)reg, 1, data, (uint16_t)size, 1000) != HAL_OK)
    {
        HAL_I2C_DeInit(&hi2c2);
        HAL_I2C_Init(&hi2c2);
        return HAL_ERROR;
    }
    return HAL_OK;
}

int i2c_write_byte(i2c_port *port, uint8_t address, uint8_t reg, uint8_t data)
{
    return i2c_write_bytes(port, address, reg, 1, &data);
}
