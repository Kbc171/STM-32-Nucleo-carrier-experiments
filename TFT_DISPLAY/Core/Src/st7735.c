#include "st7735.h"
#include "spi.h"

#define ST7735_WIDTH 128
#define ST7735_HEIGHT 160

#define CS_LOW()   HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, GPIO_PIN_RESET)
#define CS_HIGH()  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, GPIO_PIN_SET)

#define DC_LOW()   HAL_GPIO_WritePin(GPIOA, GPIO_PIN_9, GPIO_PIN_RESET)
#define DC_HIGH()  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_9, GPIO_PIN_SET)

#define RST_LOW()  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_7, GPIO_PIN_RESET)
#define RST_HIGH() HAL_GPIO_WritePin(GPIOC, GPIO_PIN_7, GPIO_PIN_SET)

static void ST7735_SendCmd(uint8_t cmd)
{
    DC_LOW();
    CS_LOW();
    HAL_SPI_Transmit(&hspi1, &cmd, 1, 100);
    CS_HIGH();
}

static void ST7735_SendData(uint8_t data)
{
    DC_HIGH();
    CS_LOW();
    HAL_SPI_Transmit(&hspi1, &data, 1, 100);
    CS_HIGH();
}

static void ST7735_SendDataMulti(uint8_t* data, uint16_t len)
{
    DC_HIGH();
    CS_LOW();
    HAL_SPI_Transmit(&hspi1, data, len, 1000);
    CS_HIGH();
}

static void ST7735_SetAddressWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1)
{
    ST7735_SendCmd(0x2A); // Column Address Set
    ST7735_SendData((x0 >> 8) & 0xFF);
    ST7735_SendData(x0 & 0xFF);
    ST7735_SendData((x1 >> 8) & 0xFF);
    ST7735_SendData(x1 & 0xFF);

    ST7735_SendCmd(0x2B); // Row Address Set
    ST7735_SendData((y0 >> 8) & 0xFF);
    ST7735_SendData(y0 & 0xFF);
    ST7735_SendData((y1 >> 8) & 0xFF);
    ST7735_SendData(y1 & 0xFF);

    ST7735_SendCmd(0x2C); // Memory Write
}

void ST7735_Init(void)
{
    // Hardware reset sequence
    RST_HIGH();
    HAL_Delay(10);
    RST_LOW();
    HAL_Delay(10);
    RST_HIGH();
    HAL_Delay(120);

    // Software Reset
    ST7735_SendCmd(0x01);
    HAL_Delay(150);

    // Sleep Out
    ST7735_SendCmd(0x11);
    HAL_Delay(120);

    // Frame Rate Control (Normal mode)
    ST7735_SendCmd(0xB1);
    ST7735_SendData(0x01);
    ST7735_SendData(0x2C);
    ST7735_SendData(0x2D);

    // Frame Rate Control (Idle mode)
    ST7735_SendCmd(0xB2);
    ST7735_SendData(0x01);
    ST7735_SendData(0x2C);
    ST7735_SendData(0x2D);

    // Frame Rate Control (Partial mode)
    ST7735_SendCmd(0xB3);
    ST7735_SendData(0x01);
    ST7735_SendData(0x2C);
    ST7735_SendData(0x2D);
    ST7735_SendData(0x01);
    ST7735_SendData(0x2C);
    ST7735_SendData(0x2D);

    // Display Inversion Control
    ST7735_SendCmd(0xB4);
    ST7735_SendData(0x07);

    // Power Control 1
    ST7735_SendCmd(0xC0);
    ST7735_SendData(0xA2);
    ST7735_SendData(0x02);
    ST7735_SendData(0x84);

    // Power Control 2
    ST7735_SendCmd(0xC1);
    ST7735_SendData(0xC5);

    // Power Control 3
    ST7735_SendCmd(0xC2);
    ST7735_SendData(0x0A);
    ST7735_SendData(0x00);

    // Power Control 4
    ST7735_SendCmd(0xC3);
    ST7735_SendData(0x8A);
    ST7735_SendData(0x2A);

    // Power Control 5
    ST7735_SendCmd(0xC4);
    ST7735_SendData(0x8A);
    ST7735_SendData(0xEE);

    // VCOM Control 1
    ST7735_SendCmd(0xC5);
    ST7735_SendData(0x0E);

    // Memory Access Control
    ST7735_SendCmd(0x36);
    ST7735_SendData(0xC8); // Row/Column exchange, BGR color order

    // Pixel Format Set (16-bit color)
    ST7735_SendCmd(0x3A);
    ST7735_SendData(0x05);

    // Gamma Set
    ST7735_SendCmd(0x26);
    ST7735_SendData(0x01);

    // Positive Gamma Correction
    ST7735_SendCmd(0xE0);
    ST7735_SendData(0x0F);
    ST7735_SendData(0x1A);
    ST7735_SendData(0x0F);
    ST7735_SendData(0x18);
    ST7735_SendData(0x2F);
    ST7735_SendData(0x28);
    ST7735_SendData(0x20);
    ST7735_SendData(0x22);
    ST7735_SendData(0x1F);
    ST7735_SendData(0x1B);
    ST7735_SendData(0x23);
    ST7735_SendData(0x37);
    ST7735_SendData(0x00);
    ST7735_SendData(0x07);
    ST7735_SendData(0x02);
    ST7735_SendData(0x10);

    // Negative Gamma Correction
    ST7735_SendCmd(0xE1);
    ST7735_SendData(0x0F);
    ST7735_SendData(0x1B);
    ST7735_SendData(0x0F);
    ST7735_SendData(0x17);
    ST7735_SendData(0x33);
    ST7735_SendData(0x2C);
    ST7735_SendData(0x29);
    ST7735_SendData(0x2E);
    ST7735_SendData(0x30);
    ST7735_SendData(0x30);
    ST7735_SendData(0x39);
    ST7735_SendData(0x3F);
    ST7735_SendData(0x00);
    ST7735_SendData(0x07);
    ST7735_SendData(0x03);
    ST7735_SendData(0x10);

    // Column Address Set
    ST7735_SendCmd(0x2A);
    ST7735_SendData(0x00);
    ST7735_SendData(0x00);
    ST7735_SendData(0x00);
    ST7735_SendData(0x7F);

    // Row Address Set
    ST7735_SendCmd(0x2B);
    ST7735_SendData(0x00);
    ST7735_SendData(0x00);
    ST7735_SendData(0x00);
    ST7735_SendData(0x9F);

    // Normal Display Mode On
    ST7735_SendCmd(0x13);
    HAL_Delay(10);

    // Display ON
    ST7735_SendCmd(0x29);
    HAL_Delay(10);
}

void ST7735_FillScreen(uint16_t color)
{
    ST7735_SetAddressWindow(0, 0, ST7735_WIDTH - 1, ST7735_HEIGHT - 1);

    uint8_t data[2] = { (color >> 8) & 0xFF, color & 0xFF };
    uint32_t pixels = ST7735_WIDTH * ST7735_HEIGHT;

    DC_HIGH();
    CS_LOW();
    for (uint32_t i = 0; i < pixels; i++)
    {
        HAL_SPI_Transmit(&hspi1, data, 2, 100);
    }
    CS_HIGH();
}

void ST7735_DrawPixel(uint16_t x, uint16_t y, uint16_t color)
{
    if (x >= ST7735_WIDTH || y >= ST7735_HEIGHT)
        return;

    ST7735_SetAddressWindow(x, y, x, y);
    
    uint8_t data[2] = { (color >> 8) & 0xFF, color & 0xFF };
    DC_HIGH();
    CS_LOW();
    HAL_SPI_Transmit(&hspi1, data, 2, 100);
    CS_HIGH();
}

void ST7735_WriteChar(uint16_t x, uint16_t y, char ch, FontDef font, uint16_t color, uint16_t bgcolor)
{
    uint32_t i, b, j;
    uint8_t pixel;
    
    // Set address window for character
    ST7735_SetAddressWindow(x, y, x + font.width - 1, y + font.height - 1);
    
    // For now, draw a simple placeholder (since font data is minimal)
    // In a full implementation, you would use the font data array
    uint8_t color_high = (color >> 8) & 0xFF;
    uint8_t color_low = color & 0xFF;
    uint8_t bg_high = (bgcolor >> 8) & 0xFF;
    uint8_t bg_low = bgcolor & 0xFF;
    
    DC_HIGH();
    CS_LOW();
    
    for (i = 0; i < font.height; i++)
    {
        for (j = 0; j < font.width; j++)
        {
            // Simple placeholder: draw background
            // In real implementation, check font bitmap
            uint8_t pixel_data[2];
            pixel_data[0] = bg_high;
            pixel_data[1] = bg_low;
            HAL_SPI_Transmit(&hspi1, pixel_data, 2, 100);
        }
    }
    
    CS_HIGH();
}

void ST7735_WriteString(uint16_t x, uint16_t y, const char* str, FontDef font, uint16_t color, uint16_t bgcolor)
{
    while (*str)
    {
        ST7735_WriteChar(x, y, *str, font, color, bgcolor);
        x += font.width;
        str++;
    }
}

void ST7735_DrawRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color)
{
    // Top edge
    for (uint16_t i = x; i < x + w; i++)
    {
        ST7735_DrawPixel(i, y, color);
    }
    // Bottom edge
    for (uint16_t i = x; i < x + w; i++)
    {
        ST7735_DrawPixel(i, y + h - 1, color);
    }
    // Left edge
    for (uint16_t i = y; i < y + h; i++)
    {
        ST7735_DrawPixel(x, i, color);
    }
    // Right edge
    for (uint16_t i = y; i < y + h; i++)
    {
        ST7735_DrawPixel(x + w - 1, i, color);
    }
}

void ST7735_FillRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color)
{
    if (x + w > ST7735_WIDTH) w = ST7735_WIDTH - x;
    if (y + h > ST7735_HEIGHT) h = ST7735_HEIGHT - y;
    
    ST7735_SetAddressWindow(x, y, x + w - 1, y + h - 1);
    
    uint8_t data[2] = { (color >> 8) & 0xFF, color & 0xFF };
    uint32_t pixels = w * h;
    
    DC_HIGH();
    CS_LOW();
    for (uint32_t i = 0; i < pixels; i++)
    {
        HAL_SPI_Transmit(&hspi1, data, 2, 100);
    }
    CS_HIGH();
}

void ST7735_DrawImage(uint16_t x, uint16_t y, uint16_t w, uint16_t h, const uint16_t* image)
{
    if (x + w > ST7735_WIDTH) w = ST7735_WIDTH - x;
    if (y + h > ST7735_HEIGHT) h = ST7735_HEIGHT - y;
    
    ST7735_SetAddressWindow(x, y, x + w - 1, y + h - 1);
    
    DC_HIGH();
    CS_LOW();
    
    uint32_t pixels = w * h;
    for (uint32_t i = 0; i < pixels; i++)
    {
        uint16_t pixel = image[i];
        uint8_t data[2] = { (pixel >> 8) & 0xFF, pixel & 0xFF };
        HAL_SPI_Transmit(&hspi1, data, 2, 100);
    }
    
    CS_HIGH();
}
