#include "video_player.h"
#include "st7735.h"

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 160

// Bouncing Ball Animation
void VideoPlayer_PlayBouncingBall(void)
{
    int ball_x = 64, ball_y = 80;
    int ball_vx = 2, ball_vy = 2;
    int ball_size = 10;
    uint16_t ball_color = ST7735_RED;
    
    ST7735_FillScreen(ST7735_BLACK);
    
    for (int frame = 0; frame < 500; frame++)
    {
        // Clear previous ball position
        ST7735_FillRect(ball_x - ball_size/2, ball_y - ball_size/2, ball_size, ball_size, ST7735_BLACK);
        
        // Update position
        ball_x += ball_vx;
        ball_y += ball_vy;
        
        // Bounce off walls
        if (ball_x - ball_size/2 <= 0 || ball_x + ball_size/2 >= SCREEN_WIDTH - 1)
        {
            ball_vx = -ball_vx;
            ball_color = (ball_color == ST7735_RED) ? ST7735_GREEN : 
                        (ball_color == ST7735_GREEN) ? ST7735_BLUE : ST7735_RED;
        }
        if (ball_y - ball_size/2 <= 0 || ball_y + ball_size/2 >= SCREEN_HEIGHT - 1)
        {
            ball_vy = -ball_vy;
            ball_color = (ball_color == ST7735_RED) ? ST7735_YELLOW : 
                        (ball_color == ST7735_YELLOW) ? ST7735_CYAN : ST7735_RED;
        }
        
        // Keep ball in bounds
        if (ball_x < ball_size/2) ball_x = ball_size/2;
        if (ball_x > SCREEN_WIDTH - ball_size/2) ball_x = SCREEN_WIDTH - ball_size/2;
        if (ball_y < ball_size/2) ball_y = ball_size/2;
        if (ball_y > SCREEN_HEIGHT - ball_size/2) ball_y = SCREEN_HEIGHT - ball_size/2;
        
        // Draw ball
        ST7735_FillRect(ball_x - ball_size/2, ball_y - ball_size/2, ball_size, ball_size, ball_color);
        
        HAL_Delay(30); // ~33 FPS
    }
}

// Color Wave Animation (optimized with rectangles)
void VideoPlayer_PlayColorWave(void)
{
    for (int frame = 0; frame < 100; frame++)
    {
        // Draw horizontal color bands that move
        for (int y = 0; y < SCREEN_HEIGHT; y++)
        {
            int wave = (y + frame * 3) % 64;
            uint16_t color;
            
            if (wave < 16)
                color = ST7735_RED;
            else if (wave < 32)
                color = ST7735_GREEN;
            else if (wave < 48)
                color = ST7735_BLUE;
            else
                color = ST7735_YELLOW;
            
            ST7735_FillRect(0, y, SCREEN_WIDTH, 1, color);
        }
        HAL_Delay(50);
    }
}

// Rainbow Animation (optimized)
void VideoPlayer_PlayRainbow(void)
{
    for (int frame = 0; frame < 150; frame++)
    {
        // Draw vertical rainbow stripes that scroll
        for (int x = 0; x < SCREEN_WIDTH; x++)
        {
            int hue = (x + frame * 2) % 384;
            uint16_t color;
            
            if (hue < 64)
                color = ST7735_RED;
            else if (hue < 128)
                color = ST7735_YELLOW;
            else if (hue < 192)
                color = ST7735_GREEN;
            else if (hue < 256)
                color = ST7735_CYAN;
            else if (hue < 320)
                color = ST7735_BLUE;
            else
                color = ST7735_MAGENTA;
            
            ST7735_FillRect(x, 0, 1, SCREEN_HEIGHT, color);
        }
        HAL_Delay(50);
    }
}

// Multi-object animation
void VideoPlayer_PlayAnimation(void)
{
    // Multiple bouncing balls
    int balls[3][4] = {
        {32, 40, 2, 1},   // x, y, vx, vy
        {96, 120, -2, -1},
        {64, 80, 1, 2}
    };
    uint16_t ball_colors[3] = {ST7735_RED, ST7735_GREEN, ST7735_BLUE};
    int ball_size = 8;
    
    ST7735_FillScreen(ST7735_BLACK);
    
    for (int frame = 0; frame < 400; frame++)
    {
        // Clear previous positions
        for (int i = 0; i < 3; i++)
        {
            ST7735_FillRect(balls[i][0] - ball_size/2, balls[i][1] - ball_size/2, 
                           ball_size, ball_size, ST7735_BLACK);
        }
        
        // Update all balls
        for (int i = 0; i < 3; i++)
        {
            balls[i][0] += balls[i][2];
            balls[i][1] += balls[i][3];
            
            // Bounce off walls
            if (balls[i][0] - ball_size/2 <= 0 || balls[i][0] + ball_size/2 >= SCREEN_WIDTH - 1)
                balls[i][2] = -balls[i][2];
            if (balls[i][1] - ball_size/2 <= 0 || balls[i][1] + ball_size/2 >= SCREEN_HEIGHT - 1)
                balls[i][3] = -balls[i][3];
            
            // Keep in bounds
            if (balls[i][0] < ball_size/2) balls[i][0] = ball_size/2;
            if (balls[i][0] > SCREEN_WIDTH - ball_size/2) balls[i][0] = SCREEN_WIDTH - ball_size/2;
            if (balls[i][1] < ball_size/2) balls[i][1] = ball_size/2;
            if (balls[i][1] > SCREEN_HEIGHT - ball_size/2) balls[i][1] = SCREEN_HEIGHT - ball_size/2;
            
            // Draw ball
            ST7735_FillRect(balls[i][0] - ball_size/2, balls[i][1] - ball_size/2, 
                           ball_size, ball_size, ball_colors[i]);
        }
        
        HAL_Delay(30);
    }
}

void VideoPlayer_Init(void)
{
    // Initialization if needed
}

