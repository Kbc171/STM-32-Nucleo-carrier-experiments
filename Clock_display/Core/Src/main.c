/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
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
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "usart.h"
#include "gpio.h"
#include "stm32f4xx_hal.h"
#include <stdint.h>
#include <string.h>
/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#define TM1637_CLK_Pin GPIO_PIN_8
#define TM1637_CLK_Port GPIOA
#define TM1637_DIO_Pin GPIO_PIN_9
#define TM1637_DIO_Port GPIOC

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
void TM1637_start(void);
void TM1637_stop(void);
void TM1637_writeByte(uint8_t b);
void TM1637_displayDigit(uint8_t pos, uint8_t digit);
void TM1637_displayNumber(int num);
void TM1637_sendCommand(uint8_t cmd);
void TM1637_setBrightness(uint8_t brightness);

const uint8_t digitToSegment[] = {
    0x3f, // 0
    0x06, // 1
    0x5b, // 2
    0x4f, // 3
    0x66, // 4
    0x6d, // 5
    0x7d, // 6
    0x07, // 7
    0x7f, // 8
    0x6f  // 9
};

void TM1637_delay(void)
{
    for (volatile int i = 0; i < 200; i++);
}

void TM1637_start(void)
{
    HAL_GPIO_WritePin(TM1637_DIO_Port, TM1637_DIO_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(TM1637_CLK_Port, TM1637_CLK_Pin, GPIO_PIN_SET);
    TM1637_delay();
    HAL_GPIO_WritePin(TM1637_DIO_Port, TM1637_DIO_Pin, GPIO_PIN_RESET);
}

void TM1637_stop(void)
{
    HAL_GPIO_WritePin(TM1637_CLK_Port, TM1637_CLK_Pin, GPIO_PIN_RESET);
    TM1637_delay();
    HAL_GPIO_WritePin(TM1637_DIO_Port, TM1637_DIO_Pin, GPIO_PIN_RESET);
    TM1637_delay();
    HAL_GPIO_WritePin(TM1637_CLK_Port, TM1637_CLK_Pin, GPIO_PIN_SET);
    TM1637_delay();
    HAL_GPIO_WritePin(TM1637_DIO_Port, TM1637_DIO_Pin, GPIO_PIN_SET);
}

void TM1637_writeByte(uint8_t b)
{
    for (int i = 0; i < 8; i++)
    {
        HAL_GPIO_WritePin(TM1637_CLK_Port, TM1637_CLK_Pin, GPIO_PIN_RESET);
        TM1637_delay();
        if (b & 0x01)
            HAL_GPIO_WritePin(TM1637_DIO_Port, TM1637_DIO_Pin, GPIO_PIN_SET);
        else
            HAL_GPIO_WritePin(TM1637_DIO_Port, TM1637_DIO_Pin, GPIO_PIN_RESET);
        TM1637_delay();
        HAL_GPIO_WritePin(TM1637_CLK_Port, TM1637_CLK_Pin, GPIO_PIN_SET);
        TM1637_delay();
        b >>= 1;
    }

    // ACK bit
    HAL_GPIO_WritePin(TM1637_CLK_Port, TM1637_CLK_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(TM1637_DIO_Port, TM1637_DIO_Pin, GPIO_PIN_SET);
    TM1637_delay();
    HAL_GPIO_WritePin(TM1637_CLK_Port, TM1637_CLK_Pin, GPIO_PIN_SET);
    TM1637_delay();
}

void TM1637_sendCommand(uint8_t cmd)
{
    TM1637_start();
    TM1637_writeByte(cmd);
    TM1637_stop();
}

void TM1637_setBrightness(uint8_t brightness)
{
    TM1637_sendCommand(0x88 | (brightness & 0x07)); // brightness 0–7
}

void TM1637_displayDigit(uint8_t pos, uint8_t digit)
{
    TM1637_start();
    TM1637_writeByte(0x44); // fixed address mode
    TM1637_stop();

    TM1637_start();
    TM1637_writeByte(0xC0 | pos);
    TM1637_writeByte(digitToSegment[digit]);
    TM1637_stop();
}

void TM1637_displayNumber(int num)
{
    uint8_t d[4] = {0};
    d[0] = (num / 1000) % 10;
    d[1] = (num / 100) % 10;
    d[2] = (num / 10) % 10;
    d[3] = num % 10;

    TM1637_sendCommand(0x40); // auto increment
    TM1637_start();
    TM1637_writeByte(0xC0); // starting at address 0
    for (int i = 0; i < 4; i++)
        TM1637_writeByte(digitToSegment[d[i]]);
    TM1637_stop();
}

int main(void)
{
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();

    // Set both pins as output
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = TM1637_CLK_Pin | TM1637_DIO_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    TM1637_setBrightness(7);

    int count = 0;

    while (1)
    {
        TM1637_displayNumber(count);
        HAL_Delay(500);
        count++;
        if (count > 9999) count = 0;
    }
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE2);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 16;
  RCC_OscInitStruct.PLL.PLLN = 336;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV4;
  RCC_OscInitStruct.PLL.PLLQ = 7;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
