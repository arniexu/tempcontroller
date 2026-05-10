#ifndef LCD8080_H
#define LCD8080_H

#include "main.h"
#include "fonts.h"

#define LCD_WIDTH   240U
#define LCD_HEIGHT  320U
#define LCD_BL_ACTIVE_HIGH 1U
#define LCD_932X_ENTRY_MODE 0x1018U
#define LCD_932X_MIRROR_X   1U

#define LCD_COLOR_BLACK  0x0000U
#define LCD_COLOR_WHITE  0xFFFFU
#define LCD_COLOR_RED    0xF800U
#define LCD_COLOR_GREEN  0x07E0U
#define LCD_COLOR_BLUE   0x001FU
#define LCD_COLOR_YELLOW 0xFFE0U
#define LCD_COLOR_CYAN   0x07FFU
#define LCD_COLOR_GRAY   0x8410U

void LCD_Init(void);
void LCD_InitBare8080(void);
void LCD_InitWithProfile(uint8_t profile);
void LCD_SetBacklightRaw(uint8_t high_level);
void LCD_FillScreen(uint16_t color);
void LCD_FillRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color);
void LCD_DrawPixel(uint16_t x, uint16_t y, uint16_t color);
void LCD_DrawCharASCII(uint16_t x, uint16_t y, char ch, const sFONT *font, uint16_t fg, uint16_t bg);
void LCD_DrawStringASCII(uint16_t x, uint16_t y, const char *str, const sFONT *font, uint16_t fg, uint16_t bg);
void LCD_DrawChinese16x16(uint16_t x, uint16_t y, uint16_t unicode, uint16_t fg, uint16_t bg);
void LCD_DrawStringUTF8(uint16_t x, uint16_t y, const char *utf8, uint16_t fg, uint16_t bg);

void LCD_TestSolidColors(void);
void LCD_TestSlider(void);
void LCD_TestFonts(void);

#endif
