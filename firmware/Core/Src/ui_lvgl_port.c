#include "ui_lvgl_port.h"

#include "app_config.h"

#if (APP_USE_LVGL_UI == 1U)

#include "bsp_lcd_8080.h"
#include "hw_platform_port.h"
#include "lvgl.h"
#include "ui_lvgl_view.h"

static lv_display_t *g_lvgl_display = 0;
static uint16_t g_lvgl_buf[BSP_LCD_WIDTH * APP_LVGL_BUF_LINES];

#if (APP_LVGL_STARTUP_SELF_TEST == 1U)
static void ui_lvgl_port_delay_frame(void)
{
    volatile uint32_t i;

    for (i = 0UL; i < 900000UL; ++i)
    {
    }
}

static void ui_lvgl_port_run_self_test(void)
{
    static const uint16_t k_colors[3] = { 0xF800U, 0x07E0U, 0x001FU };
    uint32_t i;

    for (i = 0UL; i < 3UL; ++i)
    {
        hw_display_fill_rect(0U, 0U, BSP_LCD_WIDTH, BSP_LCD_HEIGHT, k_colors[i]);
        hw_display_draw_rect(2U,
                             2U,
                             (uint16_t)(BSP_LCD_WIDTH - 4U),
                             (uint16_t)(BSP_LCD_HEIGHT - 4U),
                             0xFFFFU);
        hw_display_draw_text_xy(10U, 10U, "SELF TEST", 2U, 0xFFFFU);
        ui_lvgl_port_delay_frame();
    }

    hw_display_clear();
    hw_display_refresh();
}
#endif

static void ui_lvgl_flush_cb(lv_display_t *disp,
                             const lv_area_t *area,
                             uint8_t *px_map)
{
    int32_t x1;
    int32_t y1;
    int32_t x2;
    int32_t y2;
    int32_t src_w;
    uint32_t src_offset;

    x1 = area->x1;
    y1 = area->y1;
    x2 = area->x2;
    y2 = area->y2;

    if ((x2 < 0) || (y2 < 0) ||
        (x1 >= (int32_t)BSP_LCD_WIDTH) ||
        (y1 >= (int32_t)BSP_LCD_HEIGHT))
    {
        lv_display_flush_ready(disp);
        return;
    }

    src_w = area->x2 - area->x1 + 1;

    if (x1 < 0)
    {
        x1 = 0;
    }
    if (y1 < 0)
    {
        y1 = 0;
    }
    if (x2 >= (int32_t)BSP_LCD_WIDTH)
    {
        x2 = (int32_t)BSP_LCD_WIDTH - 1;
    }
    if (y2 >= (int32_t)BSP_LCD_HEIGHT)
    {
        y2 = (int32_t)BSP_LCD_HEIGHT - 1;
    }

    src_offset = (uint32_t)(y1 - area->y1) * (uint32_t)src_w + (uint32_t)(x1 - area->x1);

    hw_display_write_area_rgb565((uint16_t)x1,
                                 (uint16_t)y1,
                                 (uint16_t)(x2 - x1 + 1),
                                 (uint16_t)(y2 - y1 + 1),
                                 ((const uint16_t *)px_map) + src_offset);

    lv_display_flush_ready(disp);
}

void ui_lvgl_port_init(void)
{
    /* Reuse the same display bring-up sequence as legacy UI path. */
    hw_oled_init();
    hw_oled_clear();
    hw_oled_refresh();

#if (APP_LVGL_STARTUP_SELF_TEST == 1U)
    ui_lvgl_port_run_self_test();
#endif

    lv_init();

    g_lvgl_display = lv_display_create(BSP_LCD_WIDTH, BSP_LCD_HEIGHT);
    lv_display_set_color_format(g_lvgl_display, LV_COLOR_FORMAT_RGB565);
    lv_display_set_buffers(g_lvgl_display,
                           g_lvgl_buf,
                           0,
                           sizeof(g_lvgl_buf),
                           LV_DISPLAY_RENDER_MODE_PARTIAL);
    lv_display_set_flush_cb(g_lvgl_display, ui_lvgl_flush_cb);

    ui_lvgl_view_create_home();
    lv_obj_invalidate(lv_screen_active());
}

void ui_lvgl_port_tick_1ms(void)
{
    lv_tick_inc(1U);
}

void ui_lvgl_port_task(void)
{
    (void)lv_timer_handler();
}

#else

void ui_lvgl_port_init(void)
{
}

void ui_lvgl_port_tick_1ms(void)
{
}

void ui_lvgl_port_task(void)
{
}

#endif
