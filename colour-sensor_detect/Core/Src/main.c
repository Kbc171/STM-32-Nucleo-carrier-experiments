/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  * STM32F401RE + TCS3200 Color Sensor
  * Pins:
  *   S0 = PC14
  *   S1 = PC15
  *   S2 = PA11
  *   S3 = PA12
  *   OE = PB7 (Active LOW)
  *   OUT = PB4 (EXTI4 Interrupt)
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include <stdio.h>
#include <string.h>

/* Private variables ---------------------------------------------------------*/
TIM_HandleTypeDef htim2;
UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */
volatile uint32_t pulseCount = 0;
char msg[100];
/* USER CODE END PV */

/* Function prototypes -------------------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);

/* USER CODE BEGIN PFP */
void TCS3200_SetColor(uint8_t s2, uint8_t s3);
uint32_t TCS3200_ReadFreq(void);
void Convert_To_RGB(uint32_t Rf, uint32_t Gf, uint32_t Bf,
                    uint8_t *R8, uint8_t *G8, uint8_t *B8);
char* DetectColor(uint8_t R, uint8_t G, uint8_t B);
void UART_Print(char *text);
/* USER CODE END PFP */

/* Application entry point */
int main(void)
{
  uint32_t Rf, Gf, Bf;
  uint8_t R8, G8, B8;

  HAL_Init();
  SystemClock_Config();
  MX_GPIO_Init();
  MX_USART2_UART_Init();

  UART_Print("TCS3200 Initialized\r\n");

  // Enable sensor output (OE LOW)
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_7, GPIO_PIN_RESET);

  // Frequency scaling S0=1, S1=0 (20%)
  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_14, GPIO_PIN_SET);
  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_15, GPIO_PIN_RESET);

  while (1)
  {
      // --- READ RED ---
      TCS3200_SetColor(0,0);
      HAL_Delay(100);
      Rf = TCS3200_ReadFreq();

      // --- READ GREEN ---
      TCS3200_SetColor(1,1);
      HAL_Delay(100);
      Gf = TCS3200_ReadFreq();

      // --- READ BLUE ---
      TCS3200_SetColor(0,1);
      HAL_Delay(100);
      Bf = TCS3200_ReadFreq();

      // Convert to 8-bit RGB
      Convert_To_RGB(Rf, Gf, Bf, &R8, &G8, &B8);

      // Detect final color name
      char *color = DetectColor(R8, G8, B8);

      sprintf(msg,
          "FREQ => R=%lu  G=%lu  B=%lu  |  RGB => %d,%d,%d  | Color=%s\r\n",
          Rf, Gf, Bf, R8, G8, B8, color);

      UART_Print(msg);
  }
}

/* ------------------------ SYSTEM CLOCK ------------------------ */
void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    __HAL_RCC_PWR_CLK_ENABLE();
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE2);

    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
    RCC_OscInitStruct.HSIState = RCC_HSI_ON;
    RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
    RCC_OscInitStruct.PLL.PLLM = 16;
    RCC_OscInitStruct.PLL.PLLN = 336;
    RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV4;
    RCC_OscInitStruct.PLL.PLLQ = 7;

    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) { Error_Handler(); }

    RCC_ClkInitStruct.ClockType =
        RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK |
        RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;

    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider  = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

    if(HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2)!=HAL_OK)
        Error_Handler();
}

/* ------------------------ GPIO ------------------------ */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();

  /* S0 PC14, S1 PC15 */
  GPIO_InitStruct.Pin = GPIO_PIN_14 | GPIO_PIN_15;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /* S2 PA11, S3 PA12 */
  GPIO_InitStruct.Pin = GPIO_PIN_11 | GPIO_PIN_12;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /* OE PB7 */
  GPIO_InitStruct.Pin = GPIO_PIN_7;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /* OUT PB4 (EXTI4 Rising edge) */
  GPIO_InitStruct.Pin = GPIO_PIN_4;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /* Enable EXTI4 interrupt */
  HAL_NVIC_SetPriority(EXTI4_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI4_IRQn);
}

/* ------------------------ UART2 ------------------------ */
static void MX_USART2_UART_Init(void)
{
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 115200;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;

  if (HAL_UART_Init(&huart2) != HAL_OK)
    Error_Handler();
}

/* ------------------------ USER FUNCTIONS ------------------------ */

uint8_t mapColor(uint32_t v, uint32_t min, uint32_t max)
{
    if (v < min) v = min;
    if (v > max) v = max;
    return (uint8_t)((v - min) * 255 / (max - min));
}

void Convert_To_RGB(uint32_t Rf, uint32_t Gf, uint32_t Bf,
                    uint8_t *R8, uint8_t *G8, uint8_t *B8)
{
    *R8 = mapColor(Rf, 9000, 40000);
    *G8 = mapColor(Gf, 8000, 35000);
    *B8 = mapColor(Bf, 7000, 36000);
}

char* DetectColor(uint8_t R, uint8_t G, uint8_t B)
{
    uint8_t maxC = (R > G && R > B) ? R : (G > B ? G : B);
    uint8_t minC = (R < G && R < B) ? R : (G < B ? G : B);

    if (maxC < 40) return "Black";
    if (maxC > 200 && (maxC - minC) < 30) return "White";
    if ((maxC - minC) < 15) return "Grey";
    if (R > 180 && G < 80  && B < 80)  return "Red";
    if (B > 150 && R < 80  && G < 120) return "Blue";
    if (G > 150 && R < 100 && B < 120) return "Green";
    if (R > 180 && G > 160 && B < 90)  return "Yellow";
    if (R > 180 && G > 80  && G < 140 && B < 80) return "Orange";
    if (R > 120 && B > 140 && G < 90) return "Purple";
    if (R > 120 && G < 100 && B < 90) return "Brown";

    return "Unknown";
}

/* Count pulses coming from TCS3200 OUT pin */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    if(GPIO_Pin == GPIO_PIN_4)
        pulseCount++;
}

uint32_t TCS3200_ReadFreq(void)
{
    pulseCount = 0;
    HAL_Delay(100);
    return pulseCount * 10;   // multiply to get per-second frequency
}

void TCS3200_SetColor(uint8_t s2, uint8_t s3)
{
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_11, s2 ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_12, s3 ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

void UART_Print(char *text)
{
    HAL_UART_Transmit(&huart2, (uint8_t*)text, strlen(text), HAL_MAX_DELAY);
}

/* EXTI4 HANDLER — MUST HAVE */
void EXTI4_IRQHandler(void)
{
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_4);
}

/* Error Handler */
void Error_Handler(void)
{
  __disable_irq();
  while (1) {}
}
