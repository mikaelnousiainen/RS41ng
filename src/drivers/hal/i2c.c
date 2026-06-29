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

I2C_HandleTypeDef hi2c;

struct _i2c_port {
    I2C_TypeDef *i2c;
};

i2c_port DEFAULT_I2C_PORT = {
        .i2c = I2C_PORT,
};

void i2c_init()
{
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_I2C2_CLK_ENABLE();

    GPIO_InitTypeDef gpio_init;

    gpio_init.Pin = I2C_PIN_SCL;
    gpio_init.Mode = GPIO_MODE_AF_OD;
    gpio_init.Pull = GPIO_PULLUP;
    gpio_init.Speed = GPIO_SPEED_FREQ_HIGH;
#ifdef RS41_RSM4x4
    gpio_init.Alternate = GPIO_AF4_I2C2;
#endif
    HAL_GPIO_Init(I2C_GPIO, &gpio_init);

    gpio_init.Pin = I2C_PIN_SDA;
    gpio_init.Mode = GPIO_MODE_AF_OD;
    gpio_init.Pull = GPIO_PULLUP;
    gpio_init.Speed = GPIO_SPEED_FREQ_HIGH;
#ifdef RS41_RSM4x4
    gpio_init.Alternate = GPIO_AF4_I2C2;
#endif
    HAL_GPIO_Init(I2C_GPIO, &gpio_init);

    // NOTE: I2C chip reset is necessary here!
    __HAL_RCC_I2C2_FORCE_RESET();
    delay_ms(2);
    __HAL_RCC_I2C2_RELEASE_RESET();
    delay_ms(2);

    hi2c.Instance = I2C_PORT;
    hi2c.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
    hi2c.Init.OwnAddress1     = 0;
    hi2c.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
    hi2c.Init.OwnAddress2     = 0;
    hi2c.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
    hi2c.Init.NoStretchMode   = I2C_NOSTRETCH_DISABLE;
#ifdef RS41_RSM4x4
    // I2C timing for 24 MHz PCLK1 (configured in config.h)
    // See config.h for TIMINGR register breakdown
    hi2c.Init.Timing = I2C_BUS_TIMING;
    hi2c.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
#else  // F100 processor
    hi2c.Init.ClockSpeed = I2C_BUS_CLOCK_SPEED;
    hi2c.Init.DutyCycle = I2C_DUTYCYCLE_2;
#endif // Processor type

    HAL_I2C_Init(&hi2c);

#ifdef RS41_RSM4x4
    // Configure Analog filter
    if (HAL_I2CEx_ConfigAnalogFilter(&hi2c, I2C_ANALOGFILTER_ENABLE) != HAL_OK) {
      while(1);
    }

    // Configure Digital filter
    if (HAL_I2CEx_ConfigDigitalFilter(&hi2c, 2) != HAL_OK) {
      while(1);
    }
#endif //RS41_RSM4x4
    // This loop may get stuck if there is nothing connected in the I²C port pins
    int count = 100;
    while (__HAL_I2C_GET_FLAG(&hi2c, I2C_FLAG_BUSY) && count--) {
        delay_ms(1);
    }
    if (count == 0) {
        log_error("ERROR: I²C bus busy during initialization\n");
        set_error_code(ERROR_I2C_BUS_BUSY);
    }
}

void i2c_uninit()
{
    HAL_I2C_DeInit(&hi2c);
    __HAL_RCC_I2C2_FORCE_RESET();
    delay_ms(2);
    __HAL_RCC_I2C2_RELEASE_RESET();
}


int i2c_read_bytes(i2c_port *port, uint8_t address, uint8_t reg, uint8_t size, uint8_t *data)
{
    if(HAL_I2C_Mem_Read(&hi2c, ((uint16_t)address << 1), (uint16_t)reg, 1, data, (uint16_t)size, 100) != HAL_OK)
    {
        HAL_I2C_DeInit(&hi2c);
        HAL_I2C_Init(&hi2c);
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
    if(HAL_I2C_Mem_Write(&hi2c,  ((uint16_t)address << 1), (uint16_t)reg, 1, data, (uint16_t)size, 100) != HAL_OK)
    {
        HAL_I2C_DeInit(&hi2c);
        HAL_I2C_Init(&hi2c);
        return HAL_ERROR;
    }
    return HAL_OK;
}

int i2c_write_byte(i2c_port *port, uint8_t address, uint8_t reg, uint8_t data)
{
    return i2c_write_bytes(port, address, reg, 1, &data);
}
