#ifndef HW_PLATFORM_PORT_H
#define HW_PLATFORM_PORT_H

#include <stdbool.h>
#include <stdint.h>

typedef enum
{
    HW_KEY_SET = 0,
    HW_KEY_UP,
    HW_KEY_DOWN,
    HW_KEY_BACK,
    HW_KEY_COUNT
} hw_key_id_t;

#if !defined(APP_DISPLAY_DRIVER_LCD_8080) && !defined(APP_DISPLAY_DRIVER_OLED_I2C_96X96)
#define APP_DISPLAY_DRIVER_LCD_8080
#endif

#if defined(APP_DISPLAY_DRIVER_LCD_8080) && defined(APP_DISPLAY_DRIVER_OLED_I2C_96X96)
#error "Exactly one display driver must be enabled."
#endif

#if !defined(BSP_LCD_WIDTH)
#define BSP_LCD_WIDTH        (240U)
#endif

#if !defined(BSP_LCD_HEIGHT)
#define BSP_LCD_HEIGHT       (320U)
#endif

#if !defined(BSP_LCD_COLOR_BLACK)
#define BSP_LCD_COLOR_BLACK  (0x0000U)
#endif

#if !defined(BSP_LCD_COLOR_WHITE)
#define BSP_LCD_COLOR_WHITE  (0xFFFFU)
#endif

#if !defined(APP_HW_PLATFORM_PORT_CUSTOM)
#include "bsp_buzzer.h"
#include "bsp_eeprom.h"
#include "bsp_key.h"
#if defined(APP_DISPLAY_DRIVER_LCD_8080)
#include "bsp_lcd_8080.h"
#define HW_OLED_LINE_COUNT      BSP_OLED_LINE_COUNT
#define HW_OLED_LINE_CHARS      BSP_OLED_LINE_CHARS
#define HW_OLED_COLOR_BLACK     BSP_OLED_COLOR_BLACK
#define HW_OLED_COLOR_WHITE     BSP_OLED_COLOR_WHITE
#define HW_OLED_PROCESS_CALL()  bsp_oled_process()
#define HW_OLED_INIT_CALL()     bsp_oled_init()
#define HW_OLED_CLEAR_CALL()    bsp_oled_clear()
#define HW_OLED_TEXT_CALL(line, text) bsp_oled_draw_text((line), (text))
#define HW_OLED_FILL_RECT_CALL(x, y, w, h, color) bsp_oled_fill_rect((x), (y), (w), (h), (color))
#define HW_OLED_DRAW_RECT_CALL(x, y, w, h, color) bsp_oled_draw_rect((x), (y), (w), (h), (color))
#define HW_OLED_FILL_ROUND_RECT_CALL(x, y, w, h, radius, color) bsp_oled_fill_round_rect((x), (y), (w), (h), (radius), (color))
#define HW_OLED_DRAW_LINE_CALL(x0, y0, x1, y1, color) bsp_oled_draw_line((x0), (y0), (x1), (y1), (color))
#define HW_OLED_DRAW_CIRCLE_CALL(cx, cy, radius, color) bsp_oled_draw_circle((cx), (cy), (radius), (color))
#define HW_OLED_DRAW_TEXT_XY_CALL(x, y, text, scale, color) bsp_oled_draw_text_xy((x), (y), (text), (scale), (color))
#define HW_OLED_REFRESH_CALL()  bsp_oled_refresh()
#elif defined(APP_DISPLAY_DRIVER_OLED_I2C_96X96)
#include "bsp_oled_i2c_96x96.h"
#undef BSP_LCD_WIDTH
#undef BSP_LCD_HEIGHT
#undef BSP_LCD_COLOR_BLACK
#undef BSP_LCD_COLOR_WHITE
#define BSP_LCD_WIDTH            BSP_OLED96_WIDTH
#define BSP_LCD_HEIGHT           BSP_OLED96_HEIGHT
#define BSP_LCD_COLOR_BLACK      BSP_OLED96_COLOR_BLACK
#define BSP_LCD_COLOR_WHITE      BSP_OLED96_COLOR_WHITE
#define HW_OLED_LINE_COUNT       BSP_OLED96_LINE_COUNT
#define HW_OLED_LINE_CHARS       BSP_OLED96_LINE_CHARS
#define HW_OLED_COLOR_BLACK      BSP_OLED96_COLOR_BLACK
#define HW_OLED_COLOR_WHITE      BSP_OLED96_COLOR_WHITE
#define HW_OLED_PROCESS_CALL()   bsp_oled96_i2c_process()
#define HW_OLED_INIT_CALL()      bsp_oled96_i2c_init()
#define HW_OLED_CLEAR_CALL()     bsp_oled96_i2c_clear()
#define HW_OLED_TEXT_CALL(line, text) bsp_oled96_i2c_draw_text((line), (text))
#define HW_OLED_FILL_RECT_CALL(x, y, w, h, color) bsp_oled96_i2c_fill_rect((x), (y), (w), (h), (color))
#define HW_OLED_DRAW_RECT_CALL(x, y, w, h, color) bsp_oled96_i2c_draw_rect((x), (y), (w), (h), (color))
#define HW_OLED_FILL_ROUND_RECT_CALL(x, y, w, h, radius, color) bsp_oled96_i2c_fill_round_rect((x), (y), (w), (h), (radius), (color))
#define HW_OLED_DRAW_LINE_CALL(x0, y0, x1, y1, color) bsp_oled96_i2c_draw_line((x0), (y0), (x1), (y1), (color))
#define HW_OLED_DRAW_CIRCLE_CALL(cx, cy, radius, color) bsp_oled96_i2c_draw_circle((cx), (cy), (radius), (color))
#define HW_OLED_DRAW_TEXT_XY_CALL(x, y, text, scale, color) bsp_oled96_i2c_draw_text_xy((x), (y), (text), (scale), (color))
#define HW_OLED_REFRESH_CALL()   bsp_oled96_i2c_refresh()
#else
#error "No display driver selected."
#endif
#include "bsp_relay.h"
#include "bsp_rtc.h"
#include "bsp_spiflash.h"
#include "bsp_uart.h"

static inline void hw_key_init(void)
{
    bsp_key_init();
}

static inline bool hw_key_get_state(hw_key_id_t key)
{
    return bsp_key_get_state((bsp_key_id_t)key);
}

static inline void hw_buzzer_init(void)
{
    bsp_buzzer_init();
}

static inline void hw_buzzer_set(bool on)
{
    bsp_buzzer_set(on);
}

static inline void hw_eeprom_init(void)
{
    bsp_eeprom_init();
}

static inline int hw_eeprom_read(uint16_t addr, uint8_t *buf, uint16_t len)
{
    return bsp_eeprom_read(addr, buf, len);
}

static inline int hw_eeprom_write(uint16_t addr, const uint8_t *buf, uint16_t len)
{
    return bsp_eeprom_write(addr, buf, len);
}

static inline void hw_eeprom_process(void)
{
    bsp_eeprom_process();
}

static inline void hw_spiflash_init(void)
{
    bsp_spiflash_init();
}

static inline int hw_spiflash_read(uint32_t addr, uint8_t *buf, uint32_t len)
{
    return bsp_spiflash_read(addr, buf, len);
}

static inline int hw_spiflash_write(uint32_t addr, const uint8_t *buf, uint32_t len)
{
    return bsp_spiflash_write(addr, buf, len);
}

static inline void hw_spiflash_process(void)
{
    bsp_spiflash_process();
}

static inline int hw_oled_process(void)
{
    return HW_OLED_PROCESS_CALL();
}

static inline void hw_oled_init(void)
{
    HW_OLED_INIT_CALL();
}

static inline void hw_oled_clear(void)
{
    HW_OLED_CLEAR_CALL();
}

static inline void hw_oled_draw_text(uint8_t line, const char *text)
{
    HW_OLED_TEXT_CALL(line, text);
}

static inline void hw_oled_fill_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color)
{
    HW_OLED_FILL_RECT_CALL(x, y, w, h, color);
}

static inline void hw_oled_draw_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color)
{
    HW_OLED_DRAW_RECT_CALL(x, y, w, h, color);
}

static inline void hw_oled_fill_round_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t radius, uint16_t color)
{
    HW_OLED_FILL_ROUND_RECT_CALL(x, y, w, h, radius, color);
}

static inline void hw_oled_draw_line(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t color)
{
    HW_OLED_DRAW_LINE_CALL(x0, y0, x1, y1, color);
}

static inline void hw_oled_draw_circle(uint16_t cx, uint16_t cy, uint16_t radius, uint16_t color)
{
    HW_OLED_DRAW_CIRCLE_CALL(cx, cy, radius, color);
}

static inline void hw_oled_draw_text_xy(uint16_t x, uint16_t y, const char *text, uint8_t scale, uint16_t color)
{
    HW_OLED_DRAW_TEXT_XY_CALL(x, y, text, scale, color);
}

static inline void hw_oled_refresh(void)
{
    HW_OLED_REFRESH_CALL();
}

static inline void hw_relay_init(void)
{
    bsp_relay_init();
}

static inline void hw_relay_set(bool on)
{
    bsp_relay_set(on);
}

static inline void hw_uart_init(void)
{
    bsp_uart_init();
}

static inline bool hw_uart_read_line(char *buf, unsigned int buf_size)
{
    return bsp_uart_read_line(buf, buf_size);
}

static inline void hw_uart_write(const char *text)
{
    bsp_uart_write(text);
}

static inline void hw_rtc_init(void)
{
    bsp_rtc_init();
}

static inline bool hw_rtc_get_minutes_of_day(uint16_t *minutes_of_day)
{
    return bsp_rtc_get_minutes_of_day(minutes_of_day);
}
#else
#define HW_OLED_LINE_COUNT  (4U)
#define HW_OLED_LINE_CHARS  (21U)

void hw_key_init(void);
bool hw_key_get_state(hw_key_id_t key);

void hw_buzzer_init(void);
void hw_buzzer_set(bool on);

void hw_eeprom_init(void);
int hw_eeprom_read(uint16_t addr, uint8_t *buf, uint16_t len);
int hw_eeprom_write(uint16_t addr, const uint8_t *buf, uint16_t len);
void hw_eeprom_process(void);

void hw_spiflash_init(void);
int hw_spiflash_read(uint32_t addr, uint8_t *buf, uint32_t len);
int hw_spiflash_write(uint32_t addr, const uint8_t *buf, uint32_t len);
void hw_spiflash_process(void);

void hw_oled_init(void);
void hw_oled_clear(void);
void hw_oled_draw_text(uint8_t line, const char *text);
void hw_oled_fill_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color);
void hw_oled_draw_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color);
void hw_oled_fill_round_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t radius, uint16_t color);
void hw_oled_draw_line(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t color);
void hw_oled_draw_circle(uint16_t cx, uint16_t cy, uint16_t radius, uint16_t color);
void hw_oled_draw_text_xy(uint16_t x, uint16_t y, const char *text, uint8_t scale, uint16_t color);
void hw_oled_refresh(void);
int hw_oled_process(void);

void hw_relay_init(void);
void hw_relay_set(bool on);

void hw_uart_init(void);
bool hw_uart_read_line(char *buf, unsigned int buf_size);
void hw_uart_write(const char *text);

void hw_rtc_init(void);
bool hw_rtc_get_minutes_of_day(uint16_t *minutes_of_day);
#endif

#endif
