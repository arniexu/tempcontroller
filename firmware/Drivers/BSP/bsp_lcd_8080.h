#ifndef BSP_LCD_H
#define BSP_LCD_H

#include <stdint.h>
#include "app_config.h"


#define BSP_LCD_LINE_COUNT      (4U)
#define BSP_LCD_LINE_CHARS      (21U)

/* 0.96-inch LCD panel profile. */
#define BSP_LCD_PANEL_INCH_X100 (280)
#define BSP_LCD_WIDTH           (240)
#define BSP_LCD_HEIGHT          (320)
#define BSP_LCD_COLOR_BLACK     (0x0000U)
#define BSP_LCD_COLOR_WHITE     (0xFFFFU)

typedef enum
{
	BSP_LCD_OP_FILL_RECT = 1,
	BSP_LCD_OP_DRAW_RECT,
	BSP_LCD_OP_FILL_ROUND_RECT,
	BSP_LCD_OP_DRAW_LINE,
	BSP_LCD_OP_DRAW_CIRCLE,
	BSP_LCD_OP_DRAW_TEXT_XY,
	BSP_LCD_OP_WRITE_AREA_RGB565
} bsp_lcd_mock_op_kind_t;

typedef struct
{
	uint8_t kind;
	uint16_t x0;
	uint16_t y0;
	uint16_t x1;
	uint16_t y1;
	uint16_t x2;
	uint16_t y2;
	uint16_t color;
	uint8_t scale;
	char text[24];
} bsp_lcd_mock_op_t;

void bsp_lcd_fill_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color);
void bsp_lcd_draw_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color);
void bsp_lcd_fill_round_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t radius, uint16_t color);
void bsp_lcd_draw_line(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t color);
void bsp_lcd_draw_circle(uint16_t cx, uint16_t cy, uint16_t radius, uint16_t color);
void bsp_lcd_draw_text_xy(uint16_t x, uint16_t y, const char *text, uint8_t scale, uint16_t color);
void bsp_lcd_write_area_rgb565(uint16_t x,
								uint16_t y,
								uint16_t w,
								uint16_t h,
								const uint16_t *pixels);

void bsp_lcd_init(void);
void bsp_lcd_clear(void);
void bsp_lcd_draw_text(uint8_t line, const char *text);
void bsp_lcd_refresh(void);
int bsp_lcd_process(void);
int bsp_lcd_is_busy(void);
const char *bsp_lcd_mock_get_line(uint8_t line);
void bsp_lcd_mock_reset_draw_ops(void);
uint16_t bsp_lcd_mock_draw_op_count(void);
int bsp_lcd_mock_get_draw_op(uint16_t index, bsp_lcd_mock_op_t *out);

#endif
