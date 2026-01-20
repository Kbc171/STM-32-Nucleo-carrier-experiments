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
#include "fatfs.h"
#include "spi.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <string.h>
#include <stdio.h>
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
static void UART_SendStr(const char *s)
{
  HAL_UART_Transmit(&huart2, (uint8_t*)s, (uint16_t)strlen(s), HAL_MAX_DELAY);
}

static uint16_t UART_ReadLine(char *buf, uint16_t maxlen)
{
  uint16_t idx = 0;
  uint8_t ch;
  while (idx < (uint16_t)(maxlen - 1))
  {
    if (HAL_UART_Receive(&huart2, &ch, 1, HAL_MAX_DELAY) != HAL_OK)
      break;
    if (ch == '\r' || ch == '\n')
    {
      break;
    }
    buf[idx++] = (char)ch;
  }
  buf[idx] = '\0';
  return idx;
}

typedef struct {
  const char *question;
  const char *options;
} QA;
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_USART2_UART_Init();
  MX_SPI3_Init();
  MX_FATFS_Init();
  /* USER CODE BEGIN 2 */

  FATFS fs;
  FIL file;
  FRESULT res;
  char name[32];
  char fname[48];
  char answer[8];
  const QA survey[] = {
    { "What is your favorite color? (a)Red (b)Green (c)Blue", "abc" },
    { "Do you want email updates? (y/n)", "yn" },
    { "Rate your experience 1-5", "12345" }
  };
  const uint8_t qcount = sizeof(survey) / sizeof(survey[0]);

  UART_SendStr("Questionnaire starting...\r\n");
  UART_SendStr("Mounting SD...\r\n");

  res = f_mount(&fs, "", 1);
  if (res != FR_OK)
  {
    char msg[48];
    int n = snprintf(msg, sizeof(msg), "SD mount FAILED (%d)\r\n", res);
    UART_SendStr(msg);
    Error_Handler();
  }
  UART_SendStr("SD mount OK\r\n");

  UART_SendStr("Enter your name: ");
  UART_ReadLine(name, sizeof(name));
  UART_SendStr("\r\n");
  if (strlen(name) == 0)
  {
    strcpy(name, "anon");
  }

  // build filename "<name>.txt"
  snprintf(fname, sizeof(fname), "%s.txt", name);

  res = f_open(&file, fname, FA_WRITE | FA_CREATE_ALWAYS);
  if (res != FR_OK)
  {
    UART_SendStr("File open FAILED\r\n");
    Error_Handler();
  }

  f_puts("Questionnaire responses\r\n", &file);
  f_puts("-----------------------\r\n", &file);
  f_puts("Name: ", &file); f_puts(name, &file); f_puts("\r\n", &file);

  for (uint8_t i = 0; i < qcount; i++)
  {
    UART_SendStr("\r\n");
    UART_SendStr(survey[i].question);
    UART_SendStr("\r\nYour answer: ");
    UART_ReadLine(answer, sizeof(answer));
    UART_SendStr("\r\n");

    // store first char or whole answer
    f_puts("Q: ", &file); f_puts(survey[i].question, &file); f_puts("\r\n", &file);
    f_puts("A: ", &file); f_puts(answer, &file); f_puts("\r\n", &file);
  }

  f_close(&file);
  UART_SendStr("\r\nSaved responses to ");
  UART_SendStr(fname);
  UART_SendStr("\r\n");

  // simple LED blink to indicate completion
  HAL_GPIO_TogglePin(LD2_GPIO_Port, LD2_Pin);

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
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
