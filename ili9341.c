#include "ili9341.h"
#include "fonts.h"  // Подключаем шрифты

#define LCD_CS_LOW   HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_RESET)
#define LCD_CS_HIGH  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_SET)
#define LCD_DC_CMD   HAL_GPIO_WritePin(GPIOB, GPIO_PIN_1, GPIO_PIN_RESET)
#define LCD_DC_DATA  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_1, GPIO_PIN_SET)
#define LCD_RST_LOW  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_RESET)
#define LCD_RST_HIGH HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_SET)

void LCD_WriteCmd(uint8_t cmd) {
    LCD_CS_LOW;
    LCD_DC_CMD;
    HAL_SPI_Transmit(&hspi2, &cmd, 1, 10);
    LCD_CS_HIGH;
}

void LCD_WriteData(uint8_t data) {
    LCD_CS_LOW;
    LCD_DC_DATA;
    HAL_SPI_Transmit(&hspi2, &data, 1, 10);
    LCD_CS_HIGH;
}

void LCD_SetAddress(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2) {
    LCD_WriteCmd(0x2A);
    LCD_WriteData(x1 >> 8); LCD_WriteData(x1);
    LCD_WriteData(x2 >> 8); LCD_WriteData(x2);

    LCD_WriteCmd(0x2B);
    LCD_WriteData(y1 >> 8); LCD_WriteData(y1);
    LCD_WriteData(y2 >> 8); LCD_WriteData(y2);

    LCD_WriteCmd(0x2C);
}

void LCD_DrawString(int16_t x, int16_t y, char* str, uint8_t font, uint16_t color, uint16_t bg) {
    FontDef* f = (font == 1) ? &Font_11x18 : &Font_16x26;
    while (*str) {
        if (*str == '\n') { y += f->height; x = 0; }
        else LCD_DrawChar(x, y, *str, f, color, bg);
        x += f->width;
        str++;
    }
}
