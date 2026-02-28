#include "config.h"

#ifdef RS41_RSM4x4
#include <stm32l4xx_hal.h>
#else
#include <stm32f1xx_hal.h>
#endif


#include "spi.h"
#include "gpio.h"
#include "log.h"

SPI_HandleTypeDef hspi;

void spi_init()
{
    GPIO_InitTypeDef gpio_init;

    // SCK
    gpio_init.Pin = PIN_SCK;
    gpio_init.Mode = GPIO_MODE_AF_PP;
    gpio_init.Speed = GPIO_SPEED_FREQ_HIGH;
    gpio_init.Pull = GPIO_NOPULL;
#ifdef RS41_RSM4x4
    gpio_init.Alternate = GPIO_AF5_SPI2;
#endif
    HAL_GPIO_Init(BANK_SCK, &gpio_init);

    // MOSI
    gpio_init.Pin = PIN_MOSI;
    gpio_init.Mode = GPIO_MODE_AF_PP;
    gpio_init.Speed = GPIO_SPEED_FREQ_HIGH;
#ifdef RS41_RSM4x4
    gpio_init.Alternate = GPIO_AF5_SPI2;
#endif
    HAL_GPIO_Init(BANK_MOSI, &gpio_init);

    // MISO
    gpio_init.Pin = PIN_MISO;
#ifdef RS41_RSM4x4
    // STM32L4 requires AF mode and explicit alternate function for MISO
    gpio_init.Mode = GPIO_MODE_AF_PP;
    gpio_init.Alternate = GPIO_AF5_SPI2;
#else
    gpio_init.Mode = GPIO_MODE_INPUT;
#endif
    gpio_init.Pull = GPIO_NOPULL;
#ifdef DFM17
    gpio_init.Pull = GPIO_PULLUP;
#endif
    gpio_init.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(BANK_MISO, &gpio_init);


#ifdef RS41
    __HAL_RCC_SPI2_CLK_ENABLE();
    hspi.Instance = SPI2;
#endif
#ifdef DFM17
    __HAL_RCC_SPI1_CLK_ENABLE();
    hspi.Instance = SPI1;
#endif

    hspi.Init.Direction = SPI_DIRECTION_2LINES;
    hspi.Init.Mode = SPI_MODE_MASTER;

#ifdef RS41
    hspi.Init.DataSize = SPI_DATASIZE_8BIT;
#endif
#ifdef DFM17
    hspi.Init.DataSize = SPI_DATASIZE_8BIT;
#endif
    hspi.Init.CLKPolarity = SPI_POLARITY_LOW;
    hspi.Init.CLKPhase = SPI_PHASE_1EDGE;

#ifdef RS41
    hspi.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_256;
#endif
#ifdef DFM17
    hspi.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_32;
#endif

    hspi.Init.FirstBit = SPI_FIRSTBIT_MSB;
    hspi.Init.NSS = SPI_NSS_SOFT;

#ifdef DFM17
    hspi.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
    hspi.Init.CRCPolynomial = 10;
#endif

    hang_if_bad("HAL_SPI_Init",
              HAL_SPI_Init(&hspi) 
               );
}

void spi_uninit()
{
    HAL_SPI_DeInit(&hspi);

#ifdef RS41
    __HAL_RCC_SPI2_CLK_DISABLE();
#endif
#ifdef DFM17
    __HAL_RCC_SPI1_CLK_DISABLE();
#endif

    GPIO_InitTypeDef gpio_init;

    gpio_init.Pin = PIN_MISO;
    gpio_init.Mode = GPIO_MODE_AF_PP;
    gpio_init.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(BANK_MISO, &gpio_init);

    gpio_init.Pin = PIN_MOSI;
    gpio_init.Mode = GPIO_MODE_AF_PP;
    gpio_init.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(BANK_MOSI, &gpio_init);
}

void spi_send(uint8_t data)
{
    // Wait for TX buffer
    while (__HAL_SPI_GET_FLAG(&hspi, SPI_FLAG_TXE) == RESET);
    HAL_SPI_Transmit(&hspi, &data, 1, 10);
#ifdef DFM17
    while (__HAL_SPI_GET_FLAG(&hspi, SPI_FLAG_TXE) == RESET);
    while (__HAL_SPI_GET_FLAG(&hspi, SPI_FLAG_BSY) == SET);

    // Clear overrun flag caused by full-duplex transmit discarding received data
    __HAL_SPI_CLEAR_OVRFLAG(&hspi);
#endif
}

uint8_t spi_receive()
{
    // Wait for data in RX buffer
#ifdef DFM17
    while (__HAL_SPI_GET_FLAG(&hspi, SPI_FLAG_BSY) == SET);
#endif
    while (__HAL_SPI_GET_FLAG(&hspi, SPI_FLAG_RXNE) == RESET);
    uint8_t data;
    HAL_SPI_Receive(&hspi, &data, 1, 10);
    return data;
}

uint8_t spi_read()
{
    uint8_t tx_data = 0xFF;
    uint8_t rx_data = 0x00;

    while (__HAL_SPI_GET_FLAG(&hspi, SPI_FLAG_BSY) == SET);
    // Send dummy data and read response in one operation
    HAL_SPI_TransmitReceive(&hspi, &tx_data, &rx_data, 1, 10);
    return rx_data;
}

void spi_set_chip_select(GPIO_TypeDef *gpio_cs, uint16_t pin_cs, bool select)
{
    HAL_GPIO_WritePin(gpio_cs, pin_cs, select ? GPIO_PIN_RESET : GPIO_PIN_SET);
}

uint8_t spi_send_and_receive(GPIO_TypeDef *gpio_cs, uint16_t pin_cs, uint16_t data) {
    uint8_t tx[2], rx[2];
    tx[0] = (uint8_t)(data >> 8);
    tx[1] = (uint8_t)(data & 0xFF);

    HAL_GPIO_WritePin(gpio_cs, pin_cs, GPIO_PIN_RESET);

    if (HAL_SPI_TransmitReceive(&hspi, tx, rx, 2, 100) != HAL_OK) {
        HAL_GPIO_WritePin(gpio_cs, pin_cs, GPIO_PIN_SET);
        return 0xFF;
    }

    HAL_GPIO_WritePin(gpio_cs, pin_cs, GPIO_PIN_SET);

    return rx[1];
}
