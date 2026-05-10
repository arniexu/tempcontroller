#include "bsp_oled.h"
#include "bsp_lcd8080.h"
#include <string.h>

#define BSP_OLED_LINE_COUNT 4
#define BSP_OLED_LINE_CHARS 24
#define BSP_LCD_WIDTH 240U
#define BSP_LCD_HEIGHT 320U

static char g_lines[BSP_OLED_LINE_COUNT][BSP_OLED_LINE_CHARS + 1U];
static uint8_t g_refresh_pending = 0U;
static uint8_t g_refresh_line = 0U;

void bsp_oled_init(void)
{
    uint8_t i;
    LCD_Init();
    for (i = 0U; i < BSP_OLED_LINE_COUNT; ++i)
        g_lines[i][0] = '\0';
    g_refresh_pending = 0U;
    g_refresh_line = 0U;
}

void bsp_oled_clear(void)
{
    uint8_t i;
    for (i = 0U; i < BSP_OLED_LINE_COUNT; ++i)
        g_lines[i][0] = '\0';
    LCD_FillScreen(LCD_COLOR_BLACK);
}

void bsp_oled_fill_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color)
{
    LCD_FillRect(x, y, w, h, color);
}

void bsp_oled_draw_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color)
{
    uint16_t i;
    for (i = 0; i < w; ++i) {
        LCD_DrawPixel(x + i, y, color);
        LCD_DrawPixel(x + i, y + h - 1, color);
    }
    for (i = 0; i < h; ++i) {
        LCD_DrawPixel(x, y + i, color);
        LCD_DrawPixel(x + w - 1, y + i, color);
    }
}

void bsp_oled_fill_round_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t radius, uint16_t color)
{
    // 简化：直接用普通矩形
    LCD_FillRect(x, y, w, h, color);
}

void bsp_oled_draw_line(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t color)
{
    // Bresenham
    int dx = (x1 > x0) ? (x1 - x0) : (x0 - x1);
    int sx = (x0 < x1) ? 1 : -1;
    int dy = (y1 > y0) ? (y1 - y0) : (y0 - y1);
    int sy = (y0 < y1) ? 1 : -1;
    int err = (dx > dy ? dx : -dy) / 2, e2;
    while (1) {
        LCD_DrawPixel(x0, y0, color);
        if (x0 == x1 && y0 == y1) break;
        e2 = err;
        if (e2 > -dx) { err -= dy; x0 += sx; }
        if (e2 < dy) { err += dx; y0 += sy; }
    }
}

void bsp_oled_draw_circle(uint16_t cx, uint16_t cy, uint16_t radius, uint16_t color)
{
    int x = -radius, y = 0, err = 2 - 2 * radius;
    do {
        LCD_DrawPixel(cx - x, cy + y, color);
        LCD_DrawPixel(cx - y, cy - x, color);
        LCD_DrawPixel(cx + x, cy - y, color);
        LCD_DrawPixel(cx + y, cy + x, color);
        int e2 = err;
        if (e2 <= y) { y++; err += y * 2 + 1; }
        if (e2 > x || err > y) { x++; err += x * 2 + 1; }
    } while (x <= 0);
}

void bsp_oled_draw_text_xy(uint16_t x, uint16_t y, const char *text, uint8_t scale, uint16_t color)
{
    // 只支持 scale=1，直接用 Font16
    if (text)
        LCD_DrawStringASCII(x, y, text, &Font16, color, LCD_COLOR_BLACK);
}

void bsp_oled_draw_text(uint8_t line, const char *text)
{
    if (line >= BSP_OLED_LINE_COUNT || !text) return;
    strncpy(g_lines[line], text, BSP_OLED_LINE_CHARS);
    g_lines[line][BSP_OLED_LINE_CHARS] = '\0';
}

void bsp_oled_refresh(void)
{
    g_refresh_line = 0U;
    g_refresh_pending = 1U;
}

int bsp_oled_process(void)
{
    if (!g_refresh_pending) return 0;
    uint16_t y = 8U + g_refresh_line * 24U;
    LCD_DrawStringASCII(8U, y, g_lines[g_refresh_line], &Font16, LCD_COLOR_WHITE, LCD_COLOR_BLACK);
    g_refresh_line++;
    if (g_refresh_line >= BSP_OLED_LINE_COUNT) g_refresh_pending = 0U;
    return (g_refresh_pending != 0U) ? 1 : 0;
}

int bsp_oled_is_busy(void)
{
    return (g_refresh_pending != 0U) ? 1 : 0;
}

const char *bsp_oled_mock_get_line(uint8_t line)
{
    if (line >= BSP_OLED_LINE_COUNT) return "";
    return g_lines[line];
}
