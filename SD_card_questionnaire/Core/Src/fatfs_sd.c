#include "fatfs_sd.h"
#include "main.h"

extern SPI_HandleTypeDef hspi3;

#define SD_CS_LOW()     HAL_GPIO_WritePin(GPIOA, GPIO_PIN_15, GPIO_PIN_RESET)
#define SD_CS_HIGH()    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_15, GPIO_PIN_SET)

uint8_t SD_SPI_Transmit(uint8_t data)
{
    uint8_t rx;
    HAL_SPI_TransmitReceive(&hspi3, &data, &rx, 1, HAL_MAX_DELAY);
    return rx;
}

void SD_Delay(uint32_t ms)
{
    HAL_Delay(ms);
}
