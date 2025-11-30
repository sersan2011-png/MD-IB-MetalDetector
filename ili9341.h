#ifndef ILI9341_H
#define ILI9341_H

#include "stm32f4xx_hal.h"
#include <stdint.h>

// Цвета
#define LCD_BLACK       0x0000
#define LCD_WHITE       0xFFFF
#define LCD_RED         0xF800
#define LCD_GREEN       0x07E0
#define LCD_BLUE        0x001F
#define LCD_CYAN        0x07FF
#define LCD_MAGENTA     0xF81F
#define LCD_YELLOW      0xFFE0
#define LCD_GRAY        0x8410

void LCD_Init(void);
void LCD_WriteCmd(uint8_t cmd);
void LCD_WriteData(uint8_t data);
void LCD_WriteBuffer(uint8_t* buffer, uint16_t size);
void LCD_SetAddress(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2);
void LCD_Clear(uint16_t color);
void LCD_DrawPixel(int16_t x, int16_t y, uint16_t color);
void LCD_DrawString(int16_t x, int16_t y, char* str, uint8_t font, uint16_t color, uint16_t bg);
void LCD_FillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);

#endif
