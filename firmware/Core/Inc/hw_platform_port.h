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

#if !defined(APP_HW_PLATFORM_PORT_CUSTOM)
#include "bsp_buzzer.h"
#include "bsp_eeprom.h"
#include "bsp_key.h"
#include "bsp_oled.h"
#include "bsp_relay.h"
#include "bsp_rtc.h"
#include "bsp_spiflash.h"
#include "bsp_uart.h"

#define HW_OLED_LINE_COUNT  BSP_OLED_LINE_COUNT
#define HW_OLED_LINE_CHARS  BSP_OLED_LINE_CHARS

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
    return bsp_oled_process();
}

static inline void hw_oled_init(void)
{
    bsp_oled_init();
}

static inline void hw_oled_clear(void)
{
    bsp_oled_clear();
}

static inline void hw_oled_draw_text(uint8_t line, const char *text)
{
    bsp_oled_draw_text(line, text);
}

static inline void hw_oled_fill_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color)
{
    bsp_oled_fill_rect(x, y, w, h, color);
}

static inline void hw_oled_draw_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color)
{
    bsp_oled_draw_rect(x, y, w, h, color);
}

static inline void hw_oled_fill_round_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t radius, uint16_t color)
{
    bsp_oled_fill_round_rect(x, y, w, h, radius, color);
}

static inline void hw_oled_draw_line(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t color)
{
    bsp_oled_draw_line(x0, y0, x1, y1, color);
}

static inline void hw_oled_draw_circle(uint16_t cx, uint16_t cy, uint16_t radius, uint16_t color)
{
    bsp_oled_draw_circle(cx, cy, radius, color);
}

static inline void hw_oled_draw_text_xy(uint16_t x, uint16_t y, const char *text, uint8_t scale, uint16_t color)
{
    bsp_oled_draw_text_xy(x, y, text, scale, color);
}

static inline void hw_oled_refresh(void)
{
    bsp_oled_refresh();
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
