#include <stm32f1xx_hal.h>

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
    gpio_init.Pull = GPIO_NOPULL;
    gpio_init.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(I2C_GPIO, &gpio_init);

    gpio_init.Pin = I2C_PIN_SDA;
    gpio_init.Mode = GPIO_MODE_AF_OD;
    gpio_init.Pull = GPIO_NOPULL;
    gpio_init.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(I2C_GPIO, &gpio_init);

    __HAL_RCC_I2C2_RELEASE_RESET();
    delay_ms(2);
    __HAL_RCC_I2C2_FORCE_RESET();
    // NOTE: I2C chip reset is necessary here!

    delay_ms(2);

    hi2c2.Instance = I2C2;
    hi2c2.Init.ClockSpeed = clock_speed;
    hi2c2.Init.DutyCycle = I2C_DUTYCYCLE_2;
    hi2c2.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;

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
    __HAL_RCC_I2C2_RELEASE_RESET();
}

// static void i2c_reset_bus(i2c_port *port)
// {
//     for (int i = 0; i < 10; i++) {
//         I2C1->CR1 |= I2C_CR1_STOP;
//         delay_us(100);
//         I2C1->CR1 &= ~(I2C_CR1_STOP);
//         delay_us(100);
//     }

//     __HAL_I2C_CLEAR_FLAG(&hi2c2, I2C_FLAG_OVR | I2C_FLAG_AF | I2C_FLAG_ARLO | I2C_FLAG_BERR);
// }

// static int i2c_wait_until_not_busy(i2c_port *port)
// {
//     __IO uint32_t timeout = I2C_TIMEOUT_COUNTER;

//     while (__HAL_I2C_GET_FLAG(&hi2c2, I2C_FLAG_BUSY)) {
//         if (--timeout == 0) {
//             log_error("ERROR: Timeout - I2C bus busy\n");
//             i2c_reset_bus(port);
//             return HAL_ERROR_TIMEOUT;
//         }
//     }

//     return HAL_OK;
// }

// static bool i2c_wait_event(i2c_port *port, uint32_t flag)
// {
//     __IO uint32_t timeout = I2C_TIMEOUT_COUNTER;

//     while (__HAL_I2C_GET_FLAG(&hi2c2, flag) != SUCCESS) {
//         if (--timeout == 0) {
//             log_error("ERROR: WE event timeout: 0x%08lx\n", flag);
//             i2c_reset_bus(port);
//             return false;
//         }
//     }

//     return true;
// }

// static int i2c_prepare_op(i2c_port *port, uint8_t address)
// {
//     I2C_AcknowledgeConfig(port->i2c, ENABLE);
//     I2C_GenerateSTART(port->i2c, ENABLE);

//     if (!i2c_wait_event(port, I2C_EVENT_MASTER_MODE_SELECT)) {
//         log_error("ERROR: PO MMS timeout: 0x%02x\n", address);
//         return HAL_ERROR_TIMEOUT;
//     }
//     I2C_Send7bitAddress(port->i2c, address, I2C_Direction_Transmitter);

//     if (!i2c_wait_event(port, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED)) {
//         log_error("ERROR: PO MTMS timeout: 0x%02x\n", address);
//         return HAL_ERROR_TIMEOUT;
//     }

//     return HAL_OK;
// }

// static int i2c_prepare_op_with_register(i2c_port *port, uint8_t address, uint8_t reg)
// {
//     int result = i2c_prepare_op(port, address);
//     if (result != HAL_OK) {
//         return result;
//     }

//     I2C_SendData(port->i2c, reg);
//     if (!i2c_wait_event(port, I2C_EVENT_MASTER_BYTE_TRANSMITTED)) {
//         log_error("ERROR: POWR MBT timeout: 0x%02x 0x%02x\n", address, reg);
//         return HAL_ERROR_TIMEOUT;
//     }

//     return HAL_OK;
// }

// static int i2c_prepare_read(i2c_port *port, uint8_t address, uint8_t reg)
// {
//     int result = i2c_prepare_op_with_register(port, address, reg);
//     if (result != HAL_OK) {
//         return result;
//     }

//     I2C_GenerateSTART(port->i2c, ENABLE);
//     if (!i2c_wait_event(port, I2C_EVENT_MASTER_MODE_SELECT)) {
//         log_error("ERROR: PR MMS timeout: 0x%02x 0x%02x\n", address, reg);
//         return HAL_ERROR_TIMEOUT;
//     }

//     I2C_Send7bitAddress(port->i2c, address, I2C_Direction_Receiver);
//     if (!i2c_wait_event(port, I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED)) {
//         log_error("ERROR: PR MRMS timeout: 0x%02x 0x%02x\n", address, reg);
//         return HAL_ERROR_TIMEOUT;
//     }

//     return HAL_OK;
// }

// static int i2c_finish_read(i2c_port *port)
// {
//     I2C_GenerateSTOP(port->i2c, ENABLE);
//     I2C_AcknowledgeConfig(port->i2c, DISABLE);

//     if (!i2c_wait_event(port, I2C_EVENT_MASTER_BYTE_RECEIVED)) {
//         log_error("ERROR: FR MBR timeout\n");
//         return HAL_ERROR_TIMEOUT;
//     }
//     I2C_ReceiveData(port->i2c);

//     return HAL_OK;
// }

// static int i2c_prepare_write(i2c_port *port, uint8_t address, uint8_t reg)
// {
//     return i2c_prepare_op_with_register(port, address, reg);
// }

// static int i2c_finish_write(i2c_port *port)
// {
//     I2C_GenerateSTOP(port->i2c, ENABLE);

//     if (!i2c_wait_event(port, I2C_EVENT_MASTER_BYTE_TRANSMITTED)) {
//         log_error("ERROR: FW MBT timeout\n");
//         return HAL_ERROR_TIMEOUT;
//     }

//     return HAL_OK;
// }

// int i2c_read_bytes(i2c_port *port, uint8_t address, uint8_t reg, uint8_t size, uint8_t *data)
// {
//     int result = i2c_wait_until_not_busy(port);
//     if (result != HAL_OK) {
//         return result;
//     }

//     result = i2c_prepare_read(port, address, reg);
//     if (result != HAL_OK) {
//         return result;
//     }

//     for (uint8_t i = 0; i < size; i++) {
//         if (!i2c_wait_event(port, I2C_EVENT_MASTER_BYTE_RECEIVED)) {
//             log_error("ERROR: MBR timeout: 0x%02x 0x%02x\n", address, reg);
//             return HAL_ERROR_TIMEOUT;
//         }
//         data[i] = I2C_ReceiveData(port->i2c);
//     }

//     i2c_finish_read(port);

//     return HAL_OK;
// }

// int i2c_read_byte(i2c_port *port, uint8_t address, uint8_t reg, uint8_t *data)
// {
//     return i2c_read_bytes(port, address, reg, 1, data);
// }

// int i2c_write_bytes(i2c_port *port, uint8_t address, uint8_t reg, uint8_t size, uint8_t *data)
// {
//     int result = i2c_wait_until_not_busy(port);
//     if (result != HAL_OK) {
//         return result;
//     }

//     result = i2c_prepare_write(port, address, reg);
//     if (result != HAL_OK) {
//         return result;
//     }

//     for (uint8_t i = 0; i < size; i++) {
//         I2C_SendData(port->i2c, data[i]);
//         if (!i2c_wait_event(port, I2C_EVENT_MASTER_BYTE_TRANSMITTED)) {
//             log_error("ERROR: MBF timeout: 0x%02x 0x%02x\n", address, reg);
//             return HAL_ERROR_TIMEOUT;
//         }
//     }

//     i2c_finish_write(port);

//     return HAL_OK;
// }

// int i2c_write_byte(i2c_port *port, uint8_t address, uint8_t reg, uint8_t data)
// {
//     return i2c_write_bytes(port, address, reg, 1, &data);
// }

int i2c_read_bytes(i2c_port *port, uint8_t address, uint8_t reg, uint8_t size, uint8_t *data)
{
    HAL_I2C_Mem_Read(&hi2c2, (uint16_t)address, (uint16_t)reg, 1, data, (uint16_t)size, 10);

    return HAL_OK;
}

int i2c_read_byte(i2c_port *port, uint8_t address, uint8_t reg, uint8_t *data)
{
    return i2c_read_bytes(port, address, reg, 1, data);
}

int i2c_write_bytes(i2c_port *port, uint8_t address, uint8_t reg, uint8_t size, uint8_t *data)
{
    HAL_I2C_Mem_Write(&hi2c2, (uint16_t)address, (uint16_t)reg, 1, data, (uint16_t)size, 10);

    return HAL_OK;
}

int i2c_write_byte(i2c_port *port, uint8_t address, uint8_t reg, uint8_t data)
{
    return i2c_write_bytes(port, address, reg, 1, &data);
}
