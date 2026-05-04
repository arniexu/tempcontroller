#include "app_config.h"

#if defined(APP_DISPLAY_DRIVER_OLED_I2C_96X96)

#include "bsp_oled_i2c_96x96.h"

#include <string.h>

#define OLED96_FB_BYTES             ((BSP_OLED96_WIDTH * BSP_OLED96_HEIGHT) / 8U)
#define OLED96_DEFAULT_I2C_ADDR     (0x3CU)

static char g_lines[BSP_OLED96_LINE_COUNT][BSP_OLED96_LINE_CHARS + 1U];
static char g_drawn_lines[BSP_OLED96_LINE_COUNT][BSP_OLED96_LINE_CHARS + 1U];
static uint8_t g_framebuffer[OLED96_FB_BYTES];

static uint8_t g_refresh_pending = 0U;
static uint8_t g_refresh_step = 0U;
static uint8_t g_i2c_addr_7bit = OLED96_DEFAULT_I2C_ADDR;
static bsp_oled96_i2c_tx_fn_t g_tx_fn = 0;

void bsp_oled96_i2c_set_tx_fn(bsp_oled96_i2c_tx_fn_t tx_fn)
{
    g_tx_fn = tx_fn;
}

void bsp_oled96_i2c_set_addr_7bit(uint8_t addr_7bit)
{
    g_i2c_addr_7bit = addr_7bit;
}

void bsp_oled96_i2c_init(void)
{
    unsigned int i;

    g_refresh_pending = 0U;
    g_refresh_step = 0U;

    memset(g_framebuffer, 0, sizeof(g_framebuffer));
    for (i = 0U; i < BSP_OLED96_LINE_COUNT; ++i)
    {
        g_lines[i][0] = '\0';
        g_drawn_lines[i][0] = '\0';
    }
}

void bsp_oled96_i2c_clear(void)
{
    unsigned int i;

    memset(g_framebuffer, 0, sizeof(g_framebuffer));
    for (i = 0U; i < BSP_OLED96_LINE_COUNT; ++i)
    {
        g_lines[i][0] = '\0';
        g_drawn_lines[i][0] = '\0';
    }
    g_refresh_pending = 1U;
    g_refresh_step = 0U;
}

void bsp_oled96_i2c_fill_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color)
{
    (void)x;
    (void)y;
    (void)w;
    (void)h;
    (void)color;
    g_refresh_pending = 1U;
}

void bsp_oled96_i2c_draw_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color)
{
    (void)x;
    (void)y;
    (void)w;
    (void)h;
    (void)color;
    g_refresh_pending = 1U;
}

void bsp_oled96_i2c_fill_round_rect(uint16_t x,
                                    uint16_t y,
                                    uint16_t w,
                                    uint16_t h,
                                    uint16_t radius,
                                    uint16_t color)
{
    (void)x;
    (void)y;
    (void)w;
    (void)h;
    (void)radius;
    (void)color;
    g_refresh_pending = 1U;
}

void bsp_oled96_i2c_draw_line(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t color)
{
    (void)x0;
    (void)y0;
    (void)x1;
    (void)y1;
    (void)color;
    g_refresh_pending = 1U;
}

void bsp_oled96_i2c_draw_circle(uint16_t cx, uint16_t cy, uint16_t radius, uint16_t color)
{
    (void)cx;
    (void)cy;
    (void)radius;
    (void)color;
    g_refresh_pending = 1U;
}

void bsp_oled96_i2c_draw_text_xy(uint16_t x, uint16_t y, const char *text, uint8_t scale, uint16_t color)
{
    (void)x;
    (void)y;
    (void)text;
    (void)scale;
    (void)color;
    g_refresh_pending = 1U;
}

void bsp_oled96_i2c_write_area_rgb565(uint16_t x,
                                      uint16_t y,
                                      uint16_t w,
                                      uint16_t h,
                                      const uint16_t *pixels)
{
    (void)x;
    (void)y;
    (void)w;
    (void)h;
    (void)pixels;
    g_refresh_pending = 1U;
}

void bsp_oled96_i2c_draw_text(uint8_t line, const char *text)
{
    if ((line >= BSP_OLED96_LINE_COUNT) || (text == 0))
    {
        return;
    }

    strncpy(g_lines[line], text, BSP_OLED96_LINE_CHARS);
    g_lines[line][BSP_OLED96_LINE_CHARS] = '\0';

    if (strncmp(g_lines[line], g_drawn_lines[line], BSP_OLED96_LINE_CHARS + 1U) != 0)
    {
        g_refresh_pending = 1U;
    }
}

void bsp_oled96_i2c_refresh(void)
{
    g_refresh_pending = 1U;
    g_refresh_step = 0U;
}

int bsp_oled96_i2c_process(void)
{
    if (g_refresh_pending == 0U)
    {
        return 0;
    }

    /*
     * Lightweight staged flush scaffold:
     * - step 0: optional init command stream
     * - step 1..N: page/data stream
     */
    if ((g_tx_fn != 0) && (g_refresh_step == 0U))
    {
        static const uint8_t init_cmds[] = {
            0x00U, /* control byte: command stream */
            0xAEU, /* display off */
            0xA6U, /* normal display */
            0xAFU  /* display on */
        };
        (void)g_tx_fn(g_i2c_addr_7bit, init_cmds, (uint16_t)sizeof(init_cmds));
    }

    if ((g_tx_fn != 0) && (g_refresh_step == 1U))
    {
        uint8_t packet[1U + 16U];

        packet[0] = 0x40U; /* control byte: data stream */
        memset(&packet[1], 0x00, 16U);
        (void)g_tx_fn(g_i2c_addr_7bit, packet, (uint16_t)sizeof(packet));
    }

    g_refresh_step++;
    if (g_refresh_step >= 2U)
    {
        unsigned int i;

        for (i = 0U; i < BSP_OLED96_LINE_COUNT; ++i)
        {
            strncpy(g_drawn_lines[i], g_lines[i], BSP_OLED96_LINE_CHARS + 1U);
        }
        g_refresh_pending = 0U;
        g_refresh_step = 0U;
    }

    return (g_refresh_pending != 0U) ? 1 : 0;
}

int bsp_oled96_i2c_is_busy(void)
{
    return (g_refresh_pending != 0U) ? 1 : 0;
}

const char *bsp_oled96_i2c_mock_get_line(uint8_t line)
{
    if (line >= BSP_OLED96_LINE_COUNT)
    {
        return "";
    }

    return g_lines[line];
}

#endif
