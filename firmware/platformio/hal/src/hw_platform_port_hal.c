#include "hw_platform_port.h"

#include "bsp_buzzer.h"
#include "bsp_eeprom.h"
#include "bsp_key.h"
#include "bsp_lcd_8080.h"
#include "bsp_relay.h"
#include "bsp_rtc.h"
#include "bsp_spiflash.h"
#include "bsp_uart.h"

void hw_key_init(void)
{
    bsp_key_init();
}

bool hw_key_get_state(hw_key_id_t key)
{
    if (key == HW_KEY_BACK)
    {
        return false;
    }

    return bsp_key_get_state((bsp_key_id_t)key);
}

void hw_buzzer_init(void)
{
    bsp_buzzer_init();
}

void hw_buzzer_set(bool on)
{
    bsp_buzzer_set(on);
}

void hw_eeprom_init(void)
{
    bsp_eeprom_init();
}

int hw_eeprom_read(uint16_t addr, uint8_t *buf, uint16_t len)
{
    return bsp_eeprom_read(addr, buf, len);
}

int hw_eeprom_write(uint16_t addr, const uint8_t *buf, uint16_t len)
{
    return bsp_eeprom_write(addr, buf, len);
}

void hw_eeprom_process(void)
{
    bsp_eeprom_process();
}

void hw_spiflash_init(void)
{
    bsp_spiflash_init();
}

int hw_spiflash_read(uint32_t addr, uint8_t *buf, uint32_t len)
{
    return bsp_spiflash_read(addr, buf, len);
}

int hw_spiflash_write(uint32_t addr, const uint8_t *buf, uint32_t len)
{
    return bsp_spiflash_write(addr, buf, len);
}

void hw_spiflash_process(void)
{
    bsp_spiflash_process();
}

void hw_oled_init(void)
{
    bsp_oled_init();
}

void hw_oled_clear(void)
{
    bsp_oled_clear();
}

void hw_oled_draw_text(uint8_t line, const char *text)
{
    bsp_oled_draw_text(line, text);
}

void hw_oled_fill_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color)
{
    bsp_oled_fill_rect(x, y, w, h, color);
}

void hw_oled_draw_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color)
{
    bsp_oled_draw_rect(x, y, w, h, color);
}

void hw_oled_fill_round_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t radius, uint16_t color)
{
    bsp_oled_fill_round_rect(x, y, w, h, radius, color);
}

void hw_oled_draw_line(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t color)
{
    bsp_oled_draw_line(x0, y0, x1, y1, color);
}

void hw_oled_draw_circle(uint16_t cx, uint16_t cy, uint16_t radius, uint16_t color)
{
    bsp_oled_draw_circle(cx, cy, radius, color);
}

void hw_oled_draw_text_xy(uint16_t x, uint16_t y, const char *text, uint8_t scale, uint16_t color)
{
    bsp_oled_draw_text_xy(x, y, text, scale, color);
}

void hw_oled_refresh(void)
{
    bsp_oled_refresh();
}

int hw_oled_process(void)
{
    return bsp_oled_process();
}

void hw_relay_init(void)
{
    bsp_relay_init();
}

void hw_relay_set(bool on)
{
    bsp_relay_set(on);
}

void hw_uart_init(void)
{
    bsp_uart_init();
}

bool hw_uart_read_line(char *buf, unsigned int buf_size)
{
    return bsp_uart_read_line(buf, buf_size);
}

void hw_uart_write(const char *text)
{
    bsp_uart_write(text);
}

void hw_rtc_init(void)
{
    bsp_rtc_init();
}

bool hw_rtc_get_minutes_of_day(uint16_t *minutes_of_day)
{
    return bsp_rtc_get_minutes_of_day(minutes_of_day);
}
