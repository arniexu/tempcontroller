#ifndef TINYGFX_H
#define TINYGFX_H

#include <stdint.h>

void TGFX_Init(uint16_t width, uint16_t height);
void TGFX_Clear(uint16_t color);
void TGFX_DrawPixel(int16_t x, int16_t y, uint16_t color);
void TGFX_DrawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color);
void TGFX_DrawRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);
void TGFX_FillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);
void TGFX_DrawCircle(int16_t x0, int16_t y0, int16_t r, uint16_t color);
void TGFX_DrawString(int16_t x, int16_t y, const char *text, uint16_t fg, uint16_t bg);

#endif
