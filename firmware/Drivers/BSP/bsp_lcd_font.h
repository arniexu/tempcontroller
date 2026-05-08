#ifndef BSP_LCD_FONT_H
#define BSP_LCD_FONT_H

#include <stdint.h>

#define LCD_FONT_ASCII_WIDTH_5X7   (5U)
#define LCD_FONT_ASCII_HEIGHT_5X7  (7U)
#define LCD_FONT_CJK_WIDTH_16X16   (16U)
#define LCD_FONT_CJK_HEIGHT_16X16  (16U)

void bsp_lcd_font_get_ascii_5x7(char c, uint8_t out[5], uint8_t *width, uint8_t *height);
int bsp_lcd_font_utf8_decode_one(const char *text, uint16_t *codepoint, uint8_t *consumed);
int bsp_lcd_font_get_cjk_16x16(uint16_t codepoint, uint16_t out[LCD_FONT_CJK_HEIGHT_16X16]);

#endif
