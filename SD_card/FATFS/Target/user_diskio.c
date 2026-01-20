/* USER CODE BEGIN Header */
/**
 ******************************************************************************
  * @file    user_diskio.c
  * @brief   This file includes a diskio driver skeleton to be completed by the user.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
 /* USER CODE END Header */

#ifdef USE_OBSOLETE_USER_CODE_SECTION_0
/*
 * Warning: the user section 0 is no more in use (starting from CubeMx version 4.16.0)
 * To be suppressed in the future.
 * Kept to ensure backward compatibility with previous CubeMx versions when
 * migrating projects.
 * User code previously added there should be copied in the new user sections before
 * the section contents can be deleted.
 */
/* USER CODE BEGIN 0 */
/* USER CODE END 0 */
#endif

/* USER CODE BEGIN DECL */

/* Includes ------------------------------------------------------------------*/
#include <string.h>
#include <stdio.h>
#include "ff_gen_drv.h"
#include "main.h"
#include "stm32f4xx_hal.h"
#include "fatfs_sd.h"

extern SPI_HandleTypeDef hspi3;
extern UART_HandleTypeDef huart2;

#define SD_CS_GPIO_Port GPIOA
#define SD_CS_Pin       GPIO_PIN_15

#define SD_CS_LOW()     HAL_GPIO_WritePin(SD_CS_GPIO_Port, SD_CS_Pin, GPIO_PIN_RESET)
#define SD_CS_HIGH()    HAL_GPIO_WritePin(SD_CS_GPIO_Port, SD_CS_Pin, GPIO_PIN_SET)

#define SD_DUMMY_BYTE   0xFF
#define SD_TOKEN_START  0xFE

/* Card type flags (CardType) */
#define CT_MMC   0x01      /* MMC ver 3 */
#define CT_SD1   0x02      /* SD ver 1 */
#define CT_SD2   0x04      /* SD ver 2 */
#define CT_SDC   (CT_SD1|CT_SD2) /* SD */
#define CT_BLOCK 0x08      /* Block addressing */

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
/* Disk status */
static volatile DSTATUS Stat = STA_NOINIT;
static BYTE CardType = 0;

/* Expose CardType for basic debug */
BYTE USER_GetCardType(void)
{
  return CardType;
}

static void dbg_tx(const char *s)
{
  HAL_UART_Transmit(&huart2, (uint8_t *)s, (uint16_t)strlen(s), 50);
}

/* USER CODE END DECL */

/* Private function prototypes -----------------------------------------------*/
DSTATUS USER_initialize (BYTE pdrv);
DSTATUS USER_status (BYTE pdrv);
DRESULT USER_read (BYTE pdrv, BYTE *buff, DWORD sector, UINT count);
#if _USE_WRITE == 1
  DRESULT USER_write (BYTE pdrv, const BYTE *buff, DWORD sector, UINT count);
#endif /* _USE_WRITE == 1 */
#if _USE_IOCTL == 1
  DRESULT USER_ioctl (BYTE pdrv, BYTE cmd, void *buff);
#endif /* _USE_IOCTL == 1 */

Diskio_drvTypeDef  USER_Driver =
{
  USER_initialize,
  USER_status,
  USER_read,
#if  _USE_WRITE
  USER_write,
#endif  /* _USE_WRITE == 1 */
#if  _USE_IOCTL == 1
  USER_ioctl,
#endif /* _USE_IOCTL == 1 */
};

/* Private functions ---------------------------------------------------------*/

static uint8_t SD_SPI_TxRx(uint8_t data)
{
  uint8_t rx;
  HAL_SPI_TransmitReceive(&hspi3, &data, &rx, 1, HAL_MAX_DELAY);
  return rx;
}

static void SD_SendDummyClocks(uint8_t count)
{
  SD_CS_HIGH();
  while (count--)
  {
    SD_SPI_TxRx(SD_DUMMY_BYTE);
  }
}

static uint8_t SD_WaitReady(void)
{
  uint8_t resp;
  uint32_t t = HAL_GetTick();
  do
  {
    resp = SD_SPI_TxRx(SD_DUMMY_BYTE);
    if (resp == 0xFF) return 1;
  } while ((HAL_GetTick() - t) < 500);
  return 0;
}

static void SD_Release(void)
{
  SD_CS_HIGH();
  SD_SPI_TxRx(SD_DUMMY_BYTE);
}

static uint8_t SD_SendCmd(uint8_t cmd, uint32_t arg)
{
  uint8_t crc, res;
  if (!SD_WaitReady()) return 0xFF;

  SD_SPI_TxRx(0x40 | cmd);
  SD_SPI_TxRx((uint8_t)(arg >> 24));
  SD_SPI_TxRx((uint8_t)(arg >> 16));
  SD_SPI_TxRx((uint8_t)(arg >> 8));
  SD_SPI_TxRx((uint8_t)(arg));

  crc = 0x01; /* Dummy CRC + Stop */
  if (cmd == 0) crc = 0x95;
  if (cmd == 8) crc = 0x87;
  SD_SPI_TxRx(crc);

  for (uint8_t n = 0; n < 10; n++)
  {
    res = SD_SPI_TxRx(SD_DUMMY_BYTE);
    if (!(res & 0x80)) break;
  }
  /* Debug hook: last command response can be inspected by caller if needed */
  return res;
}

static int SD_ReadData(BYTE *buff, UINT len)
{
  uint8_t token;
  uint32_t t = HAL_GetTick();
  do
  {
    token = SD_SPI_TxRx(SD_DUMMY_BYTE);
    if (token == SD_TOKEN_START) break;
  } while ((HAL_GetTick() - t) < 200);

  if (token != SD_TOKEN_START)
  {
    dbg_tx("No data token\r\n");
    return 0;
  }

  while (len--) *buff++ = SD_SPI_TxRx(SD_DUMMY_BYTE);
  SD_SPI_TxRx(SD_DUMMY_BYTE); /* Discard CRC */
  SD_SPI_TxRx(SD_DUMMY_BYTE);
  return 1;
}

static int SD_WriteData(const BYTE *buff, BYTE token)
{
  if (!SD_WaitReady()) return 0;

  SD_SPI_TxRx(token);
  for (UINT i = 0; i < 512; i++)
  {
    SD_SPI_TxRx(buff[i]);
  }
  SD_SPI_TxRx(SD_DUMMY_BYTE); /* Dummy CRC */
  SD_SPI_TxRx(SD_DUMMY_BYTE);

  uint8_t resp = SD_SPI_TxRx(SD_DUMMY_BYTE);
  if ((resp & 0x1F) != 0x05) return 0;

  return SD_WaitReady();
}

static DRESULT SD_ReadSectors(BYTE *buff, DWORD sector, UINT count)
{
  SD_CS_LOW();
  if (!(CardType & CT_BLOCK)) sector *= 512;

  if (count == 1)
  {
    if (SD_SendCmd(17, sector) == 0 && SD_ReadData(buff, 512))
    {
      SD_Release();
      return RES_OK;
    }
  }
  else
  {
    if (SD_SendCmd(18, sector) == 0)
    {
      do
      {
        if (!SD_ReadData(buff, 512)) break;
        buff += 512;
      } while (--count);
      SD_SendCmd(12, 0); /* STOP_TRANSMISSION */
      SD_Release();
      if (count == 0) return RES_OK;
    }
  }
  SD_Release();
  return RES_ERROR;
}

static DRESULT SD_WriteSectors(const BYTE *buff, DWORD sector, UINT count)
{
  SD_CS_LOW();
  if (!(CardType & CT_BLOCK)) sector *= 512;

  if (count == 1)
  {
    if (SD_SendCmd(24, sector) == 0 && SD_WriteData(buff, 0xFE))
    {
      SD_Release();
      return RES_OK;
    }
  }
  else
  {
    if (SD_SendCmd(25, sector) == 0)
    {
      do
      {
        if (!SD_WriteData(buff, 0xFC)) break;
        buff += 512;
      } while (--count);
      SD_SPI_TxRx(0xFD); /* STOP_TRAN token */
      if (SD_WaitReady())
      {
        SD_Release();
        if (count == 0) return RES_OK;
      }
    }
  }
  SD_Release();
  return RES_ERROR;
}

static DRESULT SD_GetCSD(BYTE *csd)
{
  SD_CS_LOW();
  DRESULT res = RES_ERROR;
  if (SD_SendCmd(9, 0) == 0 && SD_ReadData(csd, 16))
  {
    res = RES_OK;
  }
  SD_Release();
  return res;
}

/**
  * @brief  Initializes a Drive
  * @param  pdrv: Physical drive number (0..)
  * @retval DSTATUS: Operation status
  */
DSTATUS USER_initialize (
	BYTE pdrv           /* Physical drive nmuber to identify the drive */
)
{
  /* USER CODE BEGIN INIT */
    if (pdrv != 0) return STA_NOINIT;

    SD_CS_HIGH();
    SD_SendDummyClocks(10);

    SD_CS_LOW();

    uint8_t r0 = SD_SendCmd(0, 0);
    if (r0 != 1)
    {
      dbg_tx("CMD0 fail\r\n");
      SD_Release();
      return STA_NOINIT;
    }
    dbg_tx("CMD0 OK\r\n");

    uint8_t ty = 0;
    uint8_t r8 = SD_SendCmd(8, 0x1AA);
    if (r8 == 1)
    {
      uint8_t ocr[4];
      ocr[0] = SD_SPI_TxRx(SD_DUMMY_BYTE);
      ocr[1] = SD_SPI_TxRx(SD_DUMMY_BYTE);
      ocr[2] = SD_SPI_TxRx(SD_DUMMY_BYTE);
      ocr[3] = SD_SPI_TxRx(SD_DUMMY_BYTE);
      if (ocr[2] == 0x01 && ocr[3] == 0xAA)
      {
        for (uint32_t t = HAL_GetTick(); (HAL_GetTick() - t) < 2000; )
        {
          uint8_t r55 = SD_SendCmd(55, 0);
          uint8_t r41 = SD_SendCmd(41, 1UL << 30);
          if (r55 <= 1 && r41 == 0) break;
        }
        if (SD_SendCmd(58, 0) == 0)
        {
          uint8_t ocrh = SD_SPI_TxRx(SD_DUMMY_BYTE);
          SD_SPI_TxRx(SD_DUMMY_BYTE);
          SD_SPI_TxRx(SD_DUMMY_BYTE);
          SD_SPI_TxRx(SD_DUMMY_BYTE);
          ty = (ocrh & 0x40) ? (CT_SD2 | CT_BLOCK) : CT_SD2;
        }
      }
      else
      {
        dbg_tx("CMD8 bad OCR\r\n");
      }
    }
    else
    {
      uint8_t cmd;
      ty = (SD_SendCmd(55, 0) <= 1 && SD_SendCmd(41, 0) <= 1) ? CT_SD1 : CT_MMC;
      cmd = (ty == CT_SD1) ? 41 : 1;
      for (uint32_t t = HAL_GetTick(); (HAL_GetTick() - t) < 2000; )
      {
        uint8_t r = SD_SendCmd(cmd, 0);
        if (r == 0) break;
      }
      if (SD_SendCmd(16, 512) != 0) ty = 0;
    }

    CardType = ty;
    SD_Release();

    Stat = (ty ? 0 : STA_NOINIT);
    if (Stat == 0)
    {
      dbg_tx("Init OK\r\n");
    }
    else
    {
      dbg_tx("Init FAIL\r\n");
    }
    return Stat;
  /* USER CODE END INIT */
}

/**
  * @brief  Gets Disk Status
  * @param  pdrv: Physical drive number (0..)
  * @retval DSTATUS: Operation status
  */
DSTATUS USER_status (
	BYTE pdrv       /* Physical drive number to identify the drive */
)
{
  /* USER CODE BEGIN STATUS */
    if (pdrv != 0) return STA_NOINIT;
    return Stat;
  /* USER CODE END STATUS */
}

/**
  * @brief  Reads Sector(s)
  * @param  pdrv: Physical drive number (0..)
  * @param  *buff: Data buffer to store read data
  * @param  sector: Sector address (LBA)
  * @param  count: Number of sectors to read (1..128)
  * @retval DRESULT: Operation result
  */
DRESULT USER_read (
	BYTE pdrv,      /* Physical drive nmuber to identify the drive */
	BYTE *buff,     /* Data buffer to store read data */
	DWORD sector,   /* Sector address in LBA */
	UINT count      /* Number of sectors to read */
)
{
  /* USER CODE BEGIN READ */
    if (pdrv != 0 || count == 0) return RES_PARERR;
    if (Stat & STA_NOINIT) return RES_NOTRDY;

    char msg[64];
    int n = snprintf(msg, sizeof(msg), "RD s=%lu c=%u\r\n", (unsigned long)sector, (unsigned int)count);
    if (n > 0) dbg_tx(msg);

    DRESULT res = SD_ReadSectors(buff, sector, count);
    if (res != RES_OK)
    {
      dbg_tx("RD fail\r\n");
    }
    else
    {
      if (sector == 0 || sector == 8192)
      {
        char dmsg[96];
        int n = snprintf(dmsg, sizeof(dmsg),
                         "RD ok sig=%02X%02X b0=%02X b1=%02X b2=%02X b3=%02X\r\n",
                         buff[510], buff[511],
                         buff[0], buff[1], buff[2], buff[3]);
        if (n > 0) dbg_tx(dmsg);
      }
    }
    return res;
  /* USER CODE END READ */
}

/**
  * @brief  Writes Sector(s)
  * @param  pdrv: Physical drive number (0..)
  * @param  *buff: Data to be written
  * @param  sector: Sector address (LBA)
  * @param  count: Number of sectors to write (1..128)
  * @retval DRESULT: Operation result
  */
#if _USE_WRITE == 1
DRESULT USER_write (
	BYTE pdrv,          /* Physical drive nmuber to identify the drive */
	const BYTE *buff,   /* Data to be written */
	DWORD sector,       /* Sector address in LBA */
	UINT count          /* Number of sectors to write */
)
{
  /* USER CODE BEGIN WRITE */
    if (pdrv != 0 || count == 0) return RES_PARERR;
    if (Stat & STA_NOINIT) return RES_NOTRDY;

    char msg[64];
    int n = snprintf(msg, sizeof(msg), "WR s=%lu c=%u\r\n", (unsigned long)sector, (unsigned int)count);
    if (n > 0) dbg_tx(msg);

    DRESULT res = SD_WriteSectors(buff, sector, count);
    if (res != RES_OK) dbg_tx("WR fail\r\n");
    return res;
  /* USER CODE END WRITE */
}
#endif /* _USE_WRITE == 1 */

/**
  * @brief  I/O control operation
  * @param  pdrv: Physical drive number (0..)
  * @param  cmd: Control code
  * @param  *buff: Buffer to send/receive control data
  * @retval DRESULT: Operation result
  */
#if _USE_IOCTL == 1
DRESULT USER_ioctl (
	BYTE pdrv,      /* Physical drive nmuber (0..) */
	BYTE cmd,       /* Control code */
	void *buff      /* Buffer to send/receive control data */
)
{
  /* USER CODE BEGIN IOCTL */
    if (pdrv != 0) return RES_PARERR;
    if (Stat & STA_NOINIT) return RES_NOTRDY;

    switch (cmd)
    {
      case CTRL_SYNC:
        if (SD_WaitReady()) return RES_OK;
        return RES_ERROR;

      case GET_SECTOR_SIZE:
        *(WORD*)buff = 512;
        return RES_OK;

      case GET_BLOCK_SIZE:
        *(DWORD*)buff = 1;
        return RES_OK;

      case GET_SECTOR_COUNT:
      {
        BYTE csd[16];
        if (SD_GetCSD(csd) != RES_OK) return RES_ERROR;
        uint32_t csize;
        if ((csd[0] >> 6) == 1)
        {
          csize = ((uint32_t)(csd[7] & 0x3F) << 16) | ((uint32_t)csd[8] << 8) | csd[9];
          *(DWORD*)buff = (csize + 1) << 10;
        }
        else
        {
          uint8_t n = (csd[5] & 0x0F) + ((csd[10] & 0x80) >> 7) + ((csd[9] & 0x03) << 1) + 2;
          csize = ((uint32_t)(csd[6] & 0x03) << 10) | ((uint32_t)csd[7] << 2) | ((csd[8] & 0xC0) >> 6);
          *(DWORD*)buff = (csize + 1) << (n - 9);
        }
        return RES_OK;
      }

      default:
        return RES_PARERR;
    }
  /* USER CODE END IOCTL */
}
#endif /* _USE_IOCTL == 1 */

