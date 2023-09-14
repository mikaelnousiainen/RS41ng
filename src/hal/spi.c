#include <stm32f10x_rcc.h>
#include <stm32f10x_gpio.h>
#include <stm32f10x_spi.h>

#include "spi.h"
#include "gpio.h"

void spi_init()
{
    GPIO_InitTypeDef gpio_init;

    // SCK 
    gpio_init.GPIO_Pin = PIN_SCK;
    gpio_init.GPIO_Mode = GPIO_Mode_AF_PP;
    gpio_init.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(BANK_SCK, &gpio_init);

    // MOSI
    gpio_init.GPIO_Pin = PIN_MOSI;
    gpio_init.GPIO_Mode = GPIO_Mode_AF_PP;
    gpio_init.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(BANK_MOSI, &gpio_init);

    // MISO
    gpio_init.GPIO_Pin = PIN_MISO;
#ifdef RS41
    gpio_init.GPIO_Mode = GPIO_Mode_IN_FLOATING;
#endif
#ifdef DFM17
    gpio_init.GPIO_Mode = GPIO_Mode_IPU;
#endif
    gpio_init.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(BANK_MISO, &gpio_init);

    RCC_SPIPeriphClockCmd(APBPERIPHERAL_SPI, ENABLE);

    SPI_InitTypeDef spi_init;
    SPI_StructInit(&spi_init);

    spi_init.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
    spi_init.SPI_Mode = SPI_Mode_Master;
#ifdef RS41
    spi_init.SPI_DataSize = SPI_DataSize_16b;
#endif
#ifdef DFM17
    spi_init.SPI_DataSize = SPI_DataSize_8b;
#endif
    spi_init.SPI_CPOL = SPI_CPOL_Low;
    spi_init.SPI_CPHA = SPI_CPHA_1Edge;
#ifdef RS41
    spi_init.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_256;
#endif
#ifdef DFM17
    spi_init.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_32;
#endif
    spi_init.SPI_FirstBit = SPI_FirstBit_MSB;
#ifdef RS41
    spi_init.SPI_CRCPolynomial = 7;
#endif
#ifdef DFM17
    spi_init.SPI_CRCPolynomial = 10;
    spi_init.SPI_NSS = SPI_NSS_Soft;
#endif
    SPI_Init(PERIPHERAL_SPI, &spi_init);

#ifdef RS41
    SPI_SSOutputCmd(PERIPHERAL_SPI, ENABLE);
#endif
#ifdef DFM17
    SPI_CalculateCRC(PERIPHERAL_SPI, DISABLE);
#endif

    SPI_Cmd(PERIPHERAL_SPI, ENABLE);
#ifdef RS41
    // TODO: Why is this call even here?
    SPI_Init(PERIPHERAL_SPI, &spi_init);
#endif
}

void spi_uninit()
{
    SPI_I2S_DeInit(PERIPHERAL_SPI);
    SPI_Cmd(PERIPHERAL_SPI, DISABLE);
    SPI_SSOutputCmd(PERIPHERAL_SPI, DISABLE);
    RCC_SPIPeriphClockCmd(APBPERIPHERAL_SPI, DISABLE);

    GPIO_InitTypeDef gpio_init;

    gpio_init.GPIO_Pin = PIN_MISO;
    gpio_init.GPIO_Mode = GPIO_Mode_AF_PP;
    gpio_init.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(BANK_MISO, &gpio_init);

    gpio_init.GPIO_Pin = PIN_MOSI;
    gpio_init.GPIO_Mode = GPIO_Mode_AF_PP; // was: GPIO_Mode_Out_PP; // GPIO_Mode_AF_PP
    gpio_init.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(BANK_MOSI, &gpio_init);
}

void spi_send(uint16_t data)
{
    // Wait for TX buffer
    while (SPI_I2S_GetFlagStatus(PERIPHERAL_SPI, SPI_I2S_FLAG_TXE) == RESET);
    SPI_I2S_SendData(PERIPHERAL_SPI, data);
#ifdef DFM17
    while (SPI_I2S_GetFlagStatus(PERIPHERAL_SPI, SPI_I2S_FLAG_TXE) == RESET);
    while (SPI_I2S_GetFlagStatus(PERIPHERAL_SPI, SPI_I2S_FLAG_BSY) == SET);

    // Reset the overrun error by reading the data and status registers
    // NOTE: It seems this sequence is required to make Si4063 SPI communication work on DFM17 radiosondes
    SPI_I2S_ReceiveData(PERIPHERAL_SPI);
    SPI_I2S_GetFlagStatus(PERIPHERAL_SPI, SPI_I2S_FLAG_OVR);
#endif
}

uint8_t spi_receive()
{
    // Wait for data in RX buffer
#ifdef DFM17
    while (SPI_I2S_GetFlagStatus(PERIPHERAL_SPI, SPI_I2S_FLAG_BSY) == SET);
#endif
    while (SPI_I2S_GetFlagStatus(PERIPHERAL_SPI, SPI_I2S_FLAG_RXNE) == RESET);
    return (uint8_t) SPI_I2S_ReceiveData(PERIPHERAL_SPI);
}

uint8_t spi_read()
{
    while (SPI_I2S_GetFlagStatus(PERIPHERAL_SPI, SPI_I2S_FLAG_BSY) == SET);
    // Send dummy data to read in bidirectional mode
    SPI_I2S_SendData(PERIPHERAL_SPI, 0xFF);
    // Wait for data in RX buffer
    while (SPI_I2S_GetFlagStatus(PERIPHERAL_SPI, SPI_I2S_FLAG_RXNE) == RESET);
    return (uint8_t) SPI_I2S_ReceiveData(PERIPHERAL_SPI);
}

void spi_set_chip_select(GPIO_TypeDef *gpio_cs, uint16_t pin_cs, bool select)
{
    if (select) {
        GPIO_ResetBits(gpio_cs, pin_cs);
    } else {
        GPIO_SetBits(gpio_cs, pin_cs);
    }
}

uint8_t spi_send_and_receive(GPIO_TypeDef *gpio_cs, uint16_t pin_cs, uint16_t data) {
    GPIO_ResetBits(gpio_cs, pin_cs);

    while (SPI_I2S_GetFlagStatus(PERIPHERAL_SPI, SPI_I2S_FLAG_TXE) == RESET);
    SPI_I2S_SendData(PERIPHERAL_SPI, data);

    while (SPI_I2S_GetFlagStatus(PERIPHERAL_SPI, SPI_I2S_FLAG_RXNE) == RESET);
    GPIO_SetBits(gpio_cs, pin_cs);
    return (uint8_t) SPI_I2S_ReceiveData(PERIPHERAL_SPI);
}
