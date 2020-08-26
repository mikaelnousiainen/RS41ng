#include "stm32f10x_rcc.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_i2c.h"

#include "hal.h"
#include "i2c.h"
#include "delay.h"

struct _i2c_port {
    I2C_TypeDef *i2c;
};

i2c_port DEFAULT_I2C_PORT = {
        .i2c = I2C_PORT,
};

void i2c_init()
{
    GPIO_InitTypeDef gpio_init;

    gpio_init.GPIO_Pin = I2C_PIN_SCL;
    gpio_init.GPIO_Mode = GPIO_Mode_AF_OD;
    gpio_init.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(I2C_GPIO, &gpio_init);

    gpio_init.GPIO_Pin = I2C_PIN_SDA;
    gpio_init.GPIO_Mode = GPIO_Mode_AF_OD;
    gpio_init.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(I2C_GPIO, &gpio_init);

    RCC_APB1PeriphClockCmd(I2C_PORT_RCC_PERIPH, ENABLE);

    I2C_SoftwareResetCmd(I2C_PORT, ENABLE);
    delay_ms(2);
    I2C_SoftwareResetCmd(I2C_PORT, DISABLE);

    RCC_APB1PeriphResetCmd(I2C_PORT_RCC_PERIPH, ENABLE);
    delay_ms(2);
    RCC_APB1PeriphResetCmd(I2C_PORT_RCC_PERIPH, DISABLE);

    // NOTE: I2C chip reset is necessary here!
    I2C_DeInit(I2C_PORT);

    I2C_InitTypeDef i2c_init;
    I2C_StructInit(&i2c_init);

    i2c_init.I2C_ClockSpeed = 10000;
    i2c_init.I2C_Mode = I2C_Mode_I2C;
    i2c_init.I2C_DutyCycle = I2C_DutyCycle_2;
    i2c_init.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit;
    i2c_init.I2C_Ack = I2C_Ack_Enable;

    I2C_Init(I2C_PORT, &i2c_init);

    while (I2C_GetFlagStatus(I2C_PORT, I2C_FLAG_BUSY));

    I2C_Cmd(I2C_PORT, ENABLE);
}

void i2c_uninit()
{
    I2C_Cmd(I2C_PORT, DISABLE);
    I2C_DeInit(I2C_PORT);
    RCC_APB1PeriphClockCmd(I2C_PORT_RCC_PERIPH, DISABLE);
}

static int i2c_prepare_op(i2c_port *port, uint8_t address)
{
    I2C_AcknowledgeConfig(port->i2c, ENABLE);
    I2C_GenerateSTART(port->i2c, ENABLE);

    while (!I2C_CheckEvent(port->i2c, I2C_EVENT_MASTER_MODE_SELECT));
    I2C_Send7bitAddress(port->i2c, address, I2C_Direction_Transmitter);

    while (!I2C_CheckEvent(port->i2c, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED));

    return HAL_OK;
}

static int i2c_prepare_op_with_register(i2c_port *port, uint8_t address, uint8_t reg)
{
    i2c_prepare_op(port, address);

    I2C_SendData(port->i2c, reg);
    while (!I2C_CheckEvent(port->i2c, I2C_EVENT_MASTER_BYTE_TRANSMITTED));

    return HAL_OK;
}

static int i2c_prepare_read(i2c_port *port, uint8_t address, uint8_t reg)
{
    i2c_prepare_op_with_register(port, address, reg);

    I2C_GenerateSTART(port->i2c, ENABLE);
    while (!I2C_CheckEvent(port->i2c, I2C_EVENT_MASTER_MODE_SELECT));

    I2C_Send7bitAddress(port->i2c, address, I2C_Direction_Receiver);
    while (!I2C_CheckEvent(port->i2c, I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED));

    return HAL_OK;
}

static int i2c_finish_read(i2c_port *port)
{
    I2C_GenerateSTOP(port->i2c, ENABLE);
    I2C_AcknowledgeConfig(port->i2c, DISABLE);

    while (!I2C_CheckEvent(port->i2c, I2C_EVENT_MASTER_BYTE_RECEIVED));
    I2C_ReceiveData(port->i2c);

    return HAL_OK;
}

static int i2c_prepare_write(i2c_port *port, uint8_t address, uint8_t reg)
{
    return i2c_prepare_op_with_register(port, address, reg);
}

static int i2c_finish_write(i2c_port *port)
{
    I2C_GenerateSTOP(port->i2c, ENABLE);

    while (!I2C_CheckEvent(port->i2c, I2C_EVENT_MASTER_BYTE_TRANSMITTED));

    return HAL_OK;
}

int i2c_read_bytes(i2c_port *port, uint8_t address, uint8_t reg, uint8_t size, uint8_t *data)
{
    i2c_prepare_read(port, address, reg);

    for (uint8_t i = 0; i < size; i++) {
        while (!I2C_CheckEvent(port->i2c, I2C_EVENT_MASTER_BYTE_RECEIVED));
        data[i] = I2C_ReceiveData(port->i2c);
    }

    i2c_finish_read(port);

    return HAL_OK;
}

int i2c_read_byte(i2c_port *port, uint8_t address, uint8_t reg, uint8_t *data)
{
    return i2c_read_bytes(port, address, reg, 1, data);
}

int i2c_write_bytes(i2c_port *port, uint8_t address, uint8_t reg, uint8_t size, uint8_t *data)
{
    i2c_prepare_write(port, address, reg);

    for (uint8_t i = 0; i < size; i++) {
        I2C_SendData(port->i2c, data[i]);
        while (!I2C_CheckEvent(port->i2c, I2C_EVENT_MASTER_BYTE_TRANSMITTED));
    }

    i2c_finish_write(port);

    return HAL_OK;
}

int i2c_write_byte(i2c_port *port, uint8_t address, uint8_t reg, uint8_t data)
{
    return i2c_write_bytes(port, address, reg, 1, &data);
}
