#ifndef BSP_OLED_I2C_96X96_H
#define BSP_OLED_I2C_96X96_H

#include <stdint.h>

#define BSP_OLED96_WIDTH            (96U)
#define BSP_OLED96_HEIGHT           (96U)
#define BSP_OLED96_LINE_COUNT       (6U)
#define BSP_OLED96_LINE_CHARS       (16U)
#define BSP_OLED96_COLOR_BLACK      (0x0000U)
#define BSP_OLED96_COLOR_WHITE      (0xFFFFU)

typedef int (*bsp_oled96_i2c_tx_fn_t)(uint8_t addr_7bit, const uint8_t *data, uint16_t len);

void bsp_oled96_i2c_set_tx_fn(bsp_oled96_i2c_tx_fn_t tx_fn);
void bsp_oled96_i2c_set_addr_7bit(uint8_t addr_7bit);

void bsp_oled96_i2c_init(void);
void bsp_oled96_i2c_clear(void);

void bsp_oled96_i2c_fill_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color);
void bsp_oled96_i2c_draw_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color);
void bsp_oled96_i2c_fill_round_rect(uint16_t x,
                                    uint16_t y,
                                    uint16_t w,
                                    uint16_t h,
                                    uint16_t radius,
                                    uint16_t color);
void bsp_oled96_i2c_draw_line(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t color);
void bsp_oled96_i2c_draw_circle(uint16_t cx, uint16_t cy, uint16_t radius, uint16_t color);
void bsp_oled96_i2c_draw_text_xy(uint16_t x, uint16_t y, const char *text, uint8_t scale, uint16_t color);
void bsp_oled96_i2c_write_area_rgb565(uint16_t x,
                                      uint16_t y,
                                      uint16_t w,
                                      uint16_t h,
                                      const uint16_t *pixels);

void bsp_oled96_i2c_draw_text(uint8_t line, const char *text);
void bsp_oled96_i2c_refresh(void);
int bsp_oled96_i2c_process(void);
int bsp_oled96_i2c_is_busy(void);
const char *bsp_oled96_i2c_mock_get_line(uint8_t line);

#endif
