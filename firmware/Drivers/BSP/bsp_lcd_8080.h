#ifndef BSP_OLED_H
#define BSP_OLED_H

#include <stdint.h>

#define BSP_OLED_LINE_COUNT      (4U)
#define BSP_OLED_LINE_CHARS      (21U)

/* 0.96-inch OLED panel profile. */
#define BSP_OLED_PANEL_INCH_X100 (96U)
#define BSP_OLED_WIDTH           (128U)
#define BSP_OLED_HEIGHT          (64U)
#define BSP_OLED_COLOR_BLACK     (0x0000U)
#define BSP_OLED_COLOR_WHITE     (0xFFFFU)

void bsp_oled_fill_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color);
void bsp_oled_draw_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color);
void bsp_oled_fill_round_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t radius, uint16_t color);
void bsp_oled_draw_line(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t color);
void bsp_oled_draw_circle(uint16_t cx, uint16_t cy, uint16_t radius, uint16_t color);
void bsp_oled_draw_text_xy(uint16_t x, uint16_t y, const char *text, uint8_t scale, uint16_t color);
void bsp_oled_write_area_rgb565(uint16_t x,
								uint16_t y,
								uint16_t w,
								uint16_t h,
								const uint16_t *pixels);

void bsp_oled_init(void);
void bsp_oled_clear(void);
void bsp_oled_draw_text(uint8_t line, const char *text);
void bsp_oled_refresh(void);
int bsp_oled_process(void);
int bsp_oled_is_busy(void);
const char *bsp_oled_mock_get_line(uint8_t line);

#endif
