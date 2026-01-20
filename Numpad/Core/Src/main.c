#include "main.h"
#include <stdio.h>
#include <string.h>

UART_HandleTypeDef huart2;

void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);

char read_keypad(void);
void send_char(char c);

int main(void)
{
  HAL_Init();
  SystemClock_Config();
  MX_GPIO_Init();
  MX_USART2_UART_Init();

  while (1)
  {
    char key = read_keypad();
    if (key != 0)
    {
      send_char(key);
      HAL_Delay(300); // debounce delay
    }
  }
}

/* -------------------- Keypad Functions -------------------- */

char read_keypad(void)
{
  const char keypad[4][3] = {
      {'1', '2', '3'},
      {'4', '5', '6'},
      {'7', '8', '9'},
      {'*', '0', '#'}};

  GPIO_TypeDef *row_ports[4] = {GPIOA, GPIOB, GPIOB, GPIOB};
  uint16_t row_pins[4] = {GPIO_PIN_10, GPIO_PIN_15, GPIO_PIN_14, GPIO_PIN_13};

  GPIO_TypeDef *col_ports[3] = {GPIOB, GPIOC, GPIOB};
  uint16_t col_pins[3] = {GPIO_PIN_3, GPIO_PIN_4, GPIO_PIN_5};

  for (int col = 0; col < 3; col++)
  {
    // Set all columns HIGH first
    for (int c = 0; c < 3; c++)
      HAL_GPIO_WritePin(col_ports[c], col_pins[c], GPIO_PIN_SET);

    // Drive current column LOW
    HAL_GPIO_WritePin(col_ports[col], col_pins[col], GPIO_PIN_RESET);

    // Check all rows
    for (int row = 0; row < 4; row++)
    {
      if (HAL_GPIO_ReadPin(row_ports[row], row_pins[row]) == GPIO_PIN_RESET)
      {
        return keypad[row][col];
      }
    }
  }

  return 0; // no key pressed
}

void send_char(char c)
{
  char msg[10];
  sprintf(msg, "%c\r\n", c);
  HAL_UART_Transmit(&huart2, (uint8_t *)msg, strlen(msg), HAL_MAX_DELAY);
}

/* -------------------- Peripheral Configurations -------------------- */

void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 16;
  RCC_OscInitStruct.PLL.PLLN = 336;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV4;
  RCC_OscInitStruct.PLL.PLLQ = 7;
  HAL_RCC_OscConfig(&RCC_OscInitStruct);

  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
  HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2);
}

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
  HAL_UART_Init(&huart2);
}

static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();

  /* Rows as INPUT (with pull-up) */
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;

  GPIO_InitStruct.Pin = GPIO_PIN_10;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = GPIO_PIN_15 | GPIO_PIN_14 | GPIO_PIN_13;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /* Columns as OUTPUT */
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;

  GPIO_InitStruct.Pin = GPIO_PIN_3 | GPIO_PIN_5;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = GPIO_PIN_4;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
}
