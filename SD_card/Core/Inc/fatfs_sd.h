#ifndef __FATFS_SD_H
#define __FATFS_SD_H

#include "main.h"

uint8_t SD_SPI_Transmit(uint8_t data);
void SD_Delay(uint32_t ms);

#endif
