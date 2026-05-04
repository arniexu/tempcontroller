#include "ui_service.h"

#include <stdio.h>
#include <string.h>

#include "hw_platform_port.h"
#include "log_service.h"

#define UI_SPLASH_TICKS             (8U)

#define UI_LCD_BG                   BSP_LCD_COLOR_BLACK
#define UI_LCD_TEXT                 (0xC638U)
#define UI_LCD_PANEL                (0x10A2U)
#define UI_LCD_PANEL_ALT            (0x18C3U)
#define UI_LCD_ACCENT               (0x0596U)
#define UI_LCD_WARM                 (0xFD20U)
#define UI_LCD_GOOD                 (0x3666U)
#define UI_LCD_ALERT                (0xD8A7U)

typedef struct
{
    ui_page_t page;
    int editing;
    unsigned int pid_field;
    ui_key_event_t pending_key;
    ui_key_event_t queue[8];
    unsigned int q_head;
    unsigned int q_tail;
    struct
    {
        int prev_pressed;
        unsigned int hold_ticks;
        int long_fired;
        int short_fired;
        unsigned int repeat_ticks;
    } key_track[HW_KEY_COUNT];
    app_mode_t last_mode;
    temp_snapshot_t last_temp;
    app_params_t last_params;
    float last_pid_out;
    int last_heater_on;
    int last_alarm_on;
    unsigned int info_reset_feedback_ticks;
    unsigned int info_reset_arm_ticks;
    unsigned int splash_ticks;
    int render_cache_valid;
    char rendered_lines[HW_OLED_LINE_COUNT][HW_OLED_LINE_CHARS + 1U];
} ui_ctx_t;

static ui_ctx_t g_ui;

static void render_page(void);
#if defined(USE_HAL_DRIVER)
static void render_settings_value_card(void);
#endif

typedef enum
{
    UI_REDRAW_NONE = 0,
    UI_REDRAW_FULL,
    UI_REDRAW_SETTINGS_CARD
} ui_redraw_mode_t;

static int queue_push(ui_key_event_t key)
{
    unsigned int next = (g_ui.q_head + 1U) % (unsigned int)(sizeof(g_ui.queue) / sizeof(g_ui.queue[0]));
    if (next == g_ui.q_tail)
    {
        /* Drop the oldest event so newest key events are not lost under bursts. */
        g_ui.q_tail = (g_ui.q_tail + 1U) % (unsigned int)(sizeof(g_ui.queue) / sizeof(g_ui.queue[0]));
    }
    g_ui.queue[g_ui.q_head] = key;
    g_ui.q_head = next;
    return 1;
}

static int queue_pop(ui_key_event_t *key)
{
    if ((key == 0) || (g_ui.q_tail == g_ui.q_head))
    {
        return 0;
    }
    *key = g_ui.queue[g_ui.q_tail];
    g_ui.q_tail = (g_ui.q_tail + 1U) % (unsigned int)(sizeof(g_ui.queue) / sizeof(g_ui.queue[0]));
    return 1;
}

static float clampf(float v, float min_v, float max_v)
{
    if (v < min_v)
    {
        return min_v;
    }
    if (v > max_v)
    {
        return max_v;
    }
    return v;
}

static int page_editable(ui_page_t page)
{
    return (page == UI_PAGE_SET_TEMP) || (page == UI_PAGE_PID);
}

static int page_in_browse_ring(ui_page_t page)
{
    return (page == UI_PAGE_HOME) ||
           (page == UI_PAGE_SET_TEMP) ||
           (page == UI_PAGE_PID) ||
           (page == UI_PAGE_ALARM) ||
           (page == UI_PAGE_EXPORT);
}

static unsigned int browse_index(ui_page_t page)
{
    switch (page)
    {
    case UI_PAGE_HOME:
        return 0U;
    case UI_PAGE_SET_TEMP:
        return 1U;
    case UI_PAGE_PID:
        return 2U;
    case UI_PAGE_ALARM:
        return 3U;
    case UI_PAGE_EXPORT:
        return 4U;
    default:
        return 0U;
    }
}

static ui_page_t browse_page_by_index(unsigned int idx)
{
    switch (idx % 5U)
    {
    case 0U:
        return UI_PAGE_HOME;
    case 1U:
        return UI_PAGE_SET_TEMP;
    case 2U:
        return UI_PAGE_PID;
    case 3U:
        return UI_PAGE_ALARM;
    case 4U:
    default:
        return UI_PAGE_EXPORT;
    }
}

static void apply_pid_preset(app_params_t *params, unsigned int preset)
{
    if (params == 0)
    {
        return;
    }

    switch (preset % 3U)
    {
    case 0U: /* Fast heat */
        params->kp = 10.0f;
        params->ki = 0.6f;
        params->kd = 20.0f;
        break;
    case 1U: /* Balanced */
        params->kp = 8.0f;
        params->ki = 0.3f;
        params->kd = 15.0f;
        break;
    case 2U: /* Keep warm */
    default:
        params->kp = 5.0f;
        params->ki = 0.15f;
        params->kd = 10.0f;
        break;
    }
}

static unsigned int detect_pid_preset(const app_params_t *params)
{
    if (params == 0)
    {
        return 1U;
    }

    if ((params->kp >= 9.0f) && (params->ki >= 0.45f) && (params->kd >= 18.0f))
    {
        return 0U;
    }
    if ((params->kp <= 6.0f) && (params->ki <= 0.2f) && (params->kd <= 12.0f))
    {
        return 2U;
    }
    return 1U;
}

static int page_is_exception_locked(void)
{
    if (g_ui.splash_ticks > 0U)
    {
        return 1;
    }

    if (g_ui.last_alarm_on && (g_ui.page == UI_PAGE_ALARM))
    {
        return 1;
    }

    if (g_ui.last_temp.sensor_fault && (g_ui.page == UI_PAGE_INFO))
    {
        return 1;
    }

    return 0;
}

static void update_exception_page_state(void)
{
    if (g_ui.splash_ticks > 0U)
    {
        return;
    }

    if (g_ui.last_alarm_on)
    {
        g_ui.page = UI_PAGE_ALARM;
        g_ui.editing = 0;
        return;
    }

    if (g_ui.last_temp.sensor_fault)
    {
        g_ui.page = UI_PAGE_INFO;
        g_ui.editing = 0;
        return;
    }

    if (g_ui.page == UI_PAGE_INFO)
    {
        g_ui.page = UI_PAGE_HOME;
    }
}

static void page_next(void)
{
    unsigned int idx = browse_index(g_ui.page);
    g_ui.page = browse_page_by_index(idx + 1U);
}

static void page_prev(void)
{
    unsigned int idx = browse_index(g_ui.page);
    g_ui.page = browse_page_by_index((idx + 4U) % 5U);
}

static void adjust_param(app_params_t *params, int dir)
{
    if ((params == 0) || (dir == 0))
    {
        return;
    }

    switch (g_ui.page)
    {
    case UI_PAGE_SET_TEMP:
        params->set_temp_c = clampf(params->set_temp_c + (0.5f * (float)dir), 20.0f, 60.0f);
        break;
    case UI_PAGE_PID:
        if (dir > 0)
        {
            g_ui.pid_field = (g_ui.pid_field + 1U) % 3U;
        }
        else
        {
            g_ui.pid_field = (g_ui.pid_field + 2U) % 3U;
        }
        apply_pid_preset(params, g_ui.pid_field);
        break;
    default:
        break;
    }
}

static void render_page_sync(app_params_t *params, ui_redraw_mode_t redraw_mode)
{
    if (params != 0)
    {
        g_ui.last_params = *params;
    }

    if (redraw_mode == UI_REDRAW_SETTINGS_CARD)
    {
#if defined(USE_HAL_DRIVER)
        render_settings_value_card();
#else
        g_ui.render_cache_valid = 0;
        render_page();
#endif
    }
    else if (redraw_mode == UI_REDRAW_FULL)
    {
        g_ui.render_cache_valid = 0;
        render_page();
    }
}

static ui_redraw_mode_t process_key_event(app_params_t *params, ui_key_event_t key)
{
    ui_redraw_mode_t redraw_mode = UI_REDRAW_NONE;

    if (key == UI_KEY_NONE)
    {
        return UI_REDRAW_NONE;
    }

    if (!g_ui.editing)
    {
        switch (key)
        {
        case UI_KEY_UP:
        case UI_KEY_UP_REPEAT:
            if (!page_is_exception_locked() && page_in_browse_ring(g_ui.page))
            {
                page_next();
                redraw_mode = UI_REDRAW_FULL;
            }
            break;
        case UI_KEY_DOWN:
        case UI_KEY_DOWN_REPEAT:
            if (!page_is_exception_locked() && page_in_browse_ring(g_ui.page))
            {
                page_prev();
                redraw_mode = UI_REDRAW_FULL;
            }
            break;
        case UI_KEY_BACK:
            g_ui.page = UI_PAGE_HOME;
            redraw_mode = UI_REDRAW_FULL;
            break;
        case UI_KEY_SET:
            if (page_editable(g_ui.page))
            {
                g_ui.editing = 1;
                if (g_ui.page == UI_PAGE_PID)
                {
                    g_ui.pid_field = detect_pid_preset(params != 0 ? params : &g_ui.last_params);
                }
                redraw_mode = UI_REDRAW_FULL;
            }
            else if (g_ui.page == UI_PAGE_SCHEDULE)
            {
                g_ui.page = UI_PAGE_HOME;
                redraw_mode = UI_REDRAW_FULL;
            }
            break;
        case UI_KEY_SET_LONG:
            if ((g_ui.page == UI_PAGE_INFO) && (params != 0))
            {
                if (g_ui.info_reset_arm_ticks > 0U)
                {
                    param_store_load_defaults(params);
                    g_ui.info_reset_feedback_ticks = 20U;
                    g_ui.info_reset_arm_ticks = 0U;
                    redraw_mode = UI_REDRAW_FULL;
                }
                else
                {
                    g_ui.info_reset_arm_ticks = 30U;
                    redraw_mode = UI_REDRAW_FULL;
                }
            }
            else if (page_editable(g_ui.page))
            {
                g_ui.editing = 1;
                if (g_ui.page == UI_PAGE_PID)
                {
                    g_ui.pid_field = detect_pid_preset(params != 0 ? params : &g_ui.last_params);
                }
                redraw_mode = UI_REDRAW_FULL;
            }
            break;
        default:
            break;
        }
        return redraw_mode;
    }

    switch (key)
    {
    case UI_KEY_UP:
    case UI_KEY_UP_REPEAT:
        adjust_param(params, +1);
        redraw_mode = UI_REDRAW_SETTINGS_CARD;
        break;
    case UI_KEY_DOWN:
    case UI_KEY_DOWN_REPEAT:
        adjust_param(params, -1);
        redraw_mode = UI_REDRAW_SETTINGS_CARD;
        break;
    case UI_KEY_BACK:
        g_ui.editing = 0;
        redraw_mode = UI_REDRAW_FULL;
        break;
    case UI_KEY_SET:
        g_ui.editing = 0;
        redraw_mode = UI_REDRAW_FULL;
        break;
    case UI_KEY_SET_LONG:
        g_ui.editing = 0;
        redraw_mode = UI_REDRAW_FULL;
        break;
    default:
        break;
    }

    return redraw_mode;
}

static void poll_keys_to_events(void)
{
    unsigned int i;

    for (i = 0U; i < (unsigned int)HW_KEY_COUNT; ++i)
    {
        int pressed = hw_key_get_state((hw_key_id_t)i) ? 1 : 0;

        if (pressed)
        {
            if (!g_ui.key_track[i].prev_pressed)
            {
                g_ui.key_track[i].prev_pressed = 1;
                g_ui.key_track[i].hold_ticks = 1U;
                g_ui.key_track[i].long_fired = 0;
                g_ui.key_track[i].short_fired = 0;
                g_ui.key_track[i].repeat_ticks = 0U;
            }
            else
            {
                g_ui.key_track[i].hold_ticks++;
            }

            /* Fallback for noisy/sticky lines: emit one short-key event on hold. */
            if ((!g_ui.key_track[i].short_fired) && (g_ui.key_track[i].hold_ticks >= 3U))
            {
                if (i == (unsigned int)HW_KEY_SET)
                {
                    (void)queue_push(UI_KEY_SET);
                    g_ui.key_track[i].short_fired = 1;
                }
                else if (i == (unsigned int)HW_KEY_UP)
                {
                    (void)queue_push(UI_KEY_UP);
                    g_ui.key_track[i].short_fired = 1;
                }
                else if (i == (unsigned int)HW_KEY_DOWN)
                {
                    (void)queue_push(UI_KEY_DOWN);
                    g_ui.key_track[i].short_fired = 1;
                }
            }

            if (i == (unsigned int)HW_KEY_SET)
            {
                if ((!g_ui.key_track[i].long_fired) && (g_ui.key_track[i].hold_ticks >= 10U))
                {
                    (void)queue_push(UI_KEY_SET_LONG);
                    g_ui.key_track[i].long_fired = 1;
                }
            }
            else if (i == (unsigned int)HW_KEY_DOWN)
            {
                if ((!g_ui.key_track[i].long_fired) && (g_ui.key_track[i].hold_ticks >= 10U))
                {
                    (void)queue_push(UI_KEY_BACK);
                    g_ui.key_track[i].long_fired = 1;
                }

                if ((!g_ui.key_track[i].long_fired) && (g_ui.key_track[i].hold_ticks >= 5U))
                {
                    g_ui.key_track[i].repeat_ticks++;
                    if (g_ui.key_track[i].repeat_ticks >= 2U)
                    {
                        (void)queue_push(UI_KEY_DOWN_REPEAT);
                        g_ui.key_track[i].repeat_ticks = 0U;
                    }
                }
            }
            else if (i == (unsigned int)HW_KEY_UP)
            {
                if (g_ui.key_track[i].hold_ticks >= 5U)
                {
                    g_ui.key_track[i].repeat_ticks++;
                    if (g_ui.key_track[i].repeat_ticks >= 2U)
                    {
                        (void)queue_push(UI_KEY_UP_REPEAT);
                        g_ui.key_track[i].repeat_ticks = 0U;
                    }
                }
            }
        }
        else
        {
            if (g_ui.key_track[i].prev_pressed)
            {
                if (i == (unsigned int)HW_KEY_SET)
                {
                    if ((!g_ui.key_track[i].long_fired) && (!g_ui.key_track[i].short_fired))
                    {
                        (void)queue_push(UI_KEY_SET);
                    }
                }
                else if (i == (unsigned int)HW_KEY_UP)
                {
                    if (!g_ui.key_track[i].short_fired)
                    {
                        (void)queue_push(UI_KEY_UP);
                    }
                }
                else if (i == (unsigned int)HW_KEY_DOWN)
                {
                    if ((!g_ui.key_track[i].long_fired) && (!g_ui.key_track[i].short_fired))
                    {
                        (void)queue_push(UI_KEY_DOWN);
                    }
                }
                else if (i == (unsigned int)HW_KEY_BACK)
                {
                    (void)queue_push(UI_KEY_BACK);
                }

                g_ui.key_track[i].prev_pressed = 0;
                g_ui.key_track[i].hold_ticks = 0U;
                g_ui.key_track[i].long_fired = 0;
                g_ui.key_track[i].short_fired = 0;
                g_ui.key_track[i].repeat_ticks = 0U;
            }
        }
    }
}

static const char *mode_text(app_mode_t mode)
{
    switch (mode)
    {
    case APP_MODE_IDLE:
        return "IDLE";
    case APP_MODE_HEATING:
        return "HEAT";
    case APP_MODE_SCHEDULED:
        return "SCHED";
    case APP_MODE_ALARM:
        return "ALARM";
    case APP_MODE_EXPORT:
        return "EXPT";
    default:
        return "UNKN";
    }
}

static const char *page_text(ui_page_t page)
{
    switch (page)
    {
    case UI_PAGE_HOME:
        return "HOME";
    case UI_PAGE_SET_TEMP:
        return "SET";
    case UI_PAGE_PID:
        return "PID";
    case UI_PAGE_ALARM:
        return "ALRM";
    case UI_PAGE_SCHEDULE:
        return "SCH";
    case UI_PAGE_EXPORT:
        return "LOG";
    case UI_PAGE_INFO:
        return "INFO";
    default:
        return "NA";
    }
}

#if defined(USE_HAL_DRIVER)
static uint16_t ui_text_width(const char *text, uint8_t scale)
{
    size_t len;

    if (text == 0)
    {
        return 0U;
    }

    if (scale == 0U)
    {
        scale = 1U;
    }

    len = strlen(text);
    if (len == 0U)
    {
        return 0U;
    }

    return (uint16_t)((len * ((5U * (size_t)scale) + 1U)) - 1U);
}

static void render_centered_text(uint16_t y, const char *text, uint8_t scale, uint16_t color)
{
    uint16_t text_w = ui_text_width(text, scale);
    uint16_t x = 0U;

    if (text_w < BSP_LCD_WIDTH)
    {
        x = (uint16_t)((BSP_LCD_WIDTH - text_w) / 2U);
    }

    hw_oled_draw_text_xy(x, y, text, scale, color);
}

static void render_chip(uint16_t x, uint16_t y, uint16_t w, const char *text)
{
    uint16_t text_x = (uint16_t)(x + 8U);
    uint16_t text_w = ui_text_width(text, 1U);

    hw_oled_fill_round_rect(x, y, w, 18U, 8U, UI_LCD_PANEL);
    hw_oled_draw_rect(x, y, w, 18U, UI_LCD_PANEL_ALT);

    if (text_w < (uint16_t)(w - 8U))
    {
        text_x = (uint16_t)(x + ((w - text_w) / 2U));
    }

    hw_oled_draw_text_xy(text_x, (uint16_t)(y + 5U), text, 1U, UI_LCD_TEXT);
}

static void render_hint_symbol(const char *symbol)
{
    if ((symbol != 0) && (symbol[0] != '\0'))
    {
        hw_oled_draw_text_xy(188U, 304U, symbol, 1U, UI_LCD_PANEL_ALT);
    }
}

static void render_startup_splash(void)
{
    hw_oled_fill_rect(0U, 0U, BSP_LCD_WIDTH, BSP_LCD_HEIGHT, UI_LCD_BG);
    render_centered_text(88U, "WATER", 5U, UI_LCD_TEXT);
    render_centered_text(142U, "TEMP", 5U, UI_LCD_WARM);
    hw_oled_draw_line(46U, 202U, 194U, 202U, UI_LCD_ACCENT);
    render_centered_text(230U, "3-POINT FUSED", 2U, UI_LCD_PANEL_ALT);
    render_centered_text(254U, "CONTROL", 2U, UI_LCD_PANEL_ALT);
}

static void render_home_dashboard(void)
{
    char temp_big[12];
    char set_text[12];
    char chip_text[12];
    char footer_note[20];
    char footer_sub[28];

    (void)snprintf(temp_big, sizeof(temp_big), "%.1fC", g_ui.last_temp.t_ctrl);
    (void)snprintf(set_text, sizeof(set_text), "%.1fC", g_ui.last_params.set_temp_c);
    (void)snprintf(chip_text, sizeof(chip_text), "%s", g_ui.last_heater_on ? "HEAT" : "HOLD");
    (void)snprintf(footer_note, sizeof(footer_note), "HEATER %s / STABLE", g_ui.last_heater_on ? "ON" : "OFF");
    (void)snprintf(footer_sub,
                   sizeof(footer_sub),
                   "SAFE MARGIN %.1fC",
                   (double)(g_ui.last_params.alarm_threshold_c -
                            (g_ui.last_temp.t1 > g_ui.last_temp.t2 ?
                                 (g_ui.last_temp.t1 > g_ui.last_temp.t3 ? g_ui.last_temp.t1 : g_ui.last_temp.t3) :
                                 (g_ui.last_temp.t2 > g_ui.last_temp.t3 ? g_ui.last_temp.t2 : g_ui.last_temp.t3))));

    hw_oled_fill_rect(0U, 0U, BSP_LCD_WIDTH, BSP_LCD_HEIGHT, UI_LCD_BG);

    hw_oled_draw_text_xy(26U, 26U, "FUSED TEMP", 2U, UI_LCD_TEXT);
    render_chip(168U, 24U, 48U, chip_text);
    hw_oled_draw_text_xy(28U, 78U, temp_big, 6U, UI_LCD_WARM);

    hw_oled_draw_line(24U, 182U, 216U, 182U, UI_LCD_PANEL_ALT);
    hw_oled_draw_text_xy(28U, 200U, "SET", 2U, UI_LCD_TEXT);
    hw_oled_draw_text_xy(96U, 196U, set_text, 3U, UI_LCD_GOOD);

    hw_oled_draw_text_xy(28U, 254U, footer_note, 1U, g_ui.last_heater_on ? UI_LCD_TEXT : UI_LCD_PANEL_ALT);
    hw_oled_draw_text_xy(28U, 276U, footer_sub, 1U, UI_LCD_PANEL_ALT);
    render_hint_symbol("< >");
}

static void render_page_shell(const char *title, const char *subtitle)
{
    hw_oled_fill_rect(0U, 0U, BSP_LCD_WIDTH, BSP_LCD_HEIGHT, UI_LCD_BG);
    hw_oled_draw_text_xy(24U, 24U, title, 3U, UI_LCD_TEXT);
    if ((subtitle != 0) && (subtitle[0] != '\0'))
    {
        hw_oled_draw_text_xy(26U, 56U, subtitle, 1U, UI_LCD_PANEL_ALT);
    }
    hw_oled_draw_line(24U, 82U, 216U, 82U, UI_LCD_ACCENT);
}

static void render_value_card(uint16_t x,
                              uint16_t y,
                              uint16_t w,
                              uint16_t h,
                              const char *label,
                              const char *value,
                              uint8_t value_scale,
                              uint16_t accent_color)
{
    hw_oled_fill_round_rect(x, y, w, h, 20U, UI_LCD_PANEL);
    hw_oled_draw_rect(x, y, w, h, UI_LCD_PANEL_ALT);
    hw_oled_draw_text_xy((uint16_t)(x + 18U), (uint16_t)(y + 16U), label, 1U, UI_LCD_PANEL_ALT);
    hw_oled_draw_text_xy((uint16_t)(x + 18U), (uint16_t)(y + 44U), value, value_scale, accent_color);
}

static void render_settings_value_card(void)
{
    char value_text[16];

    switch (g_ui.page)
    {
    case UI_PAGE_SET_TEMP:
        (void)snprintf(value_text, sizeof(value_text), "%.1fC", g_ui.last_params.set_temp_c);
        render_value_card(20U, 106U, 200U, 110U, "TARGET", value_text, 5U, UI_LCD_GOOD);
        break;
    case UI_PAGE_PID:
        if (g_ui.pid_field == 0U)
        {
            (void)snprintf(value_text, sizeof(value_text), "FAST");
        }
        else if (g_ui.pid_field == 1U)
        {
            (void)snprintf(value_text, sizeof(value_text), "BAL");
        }
        else
        {
            (void)snprintf(value_text, sizeof(value_text), "WARM");
        }
        render_value_card(20U, 106U, 200U, 96U, "PRESET", value_text, 5U, UI_LCD_ACCENT);
        break;
    default:
        break;
    }
}

static void render_status_line(uint16_t y, const char *text, uint16_t color)
{
    hw_oled_draw_text_xy(28U, y, text, 1U, color);
}

static void render_settings_page(void)
{
    char value_text[16];
    char footer[28];

    switch (g_ui.page)
    {
    case UI_PAGE_SET_TEMP:
        render_page_shell("SET TEMP", "STEP 0.5C");
        (void)snprintf(value_text, sizeof(value_text), "%.1fC", g_ui.last_params.set_temp_c);
        render_value_card(20U, 106U, 200U, 110U, "TARGET", value_text, 5U, UI_LCD_GOOD);
        (void)snprintf(footer, sizeof(footer), "CTRL %.1fC", g_ui.last_temp.t_ctrl);
        render_status_line(246U, footer, UI_LCD_TEXT);
        render_status_line(272U, g_ui.editing ? "SET SAVE / LONG CANCEL" : "SET TO EDIT", UI_LCD_PANEL_ALT);
        render_hint_symbol("OK ^");
        break;

    case UI_PAGE_PID:
        (void)snprintf(footer, sizeof(footer), "PRESET %u OF 3", (unsigned int)(g_ui.pid_field + 1U));
        render_page_shell("PID PRESET", footer);
        if (g_ui.pid_field == 0U)
        {
            (void)snprintf(value_text, sizeof(value_text), "FAST");
        }
        else if (g_ui.pid_field == 1U)
        {
            (void)snprintf(value_text, sizeof(value_text), "BAL");
        }
        else
        {
            (void)snprintf(value_text, sizeof(value_text), "WARM");
        }
        render_value_card(20U, 106U, 200U, 96U, "PRESET", value_text, 5U, UI_LCD_ACCENT);
        render_status_line(246U, "FAST / BAL / WARM", UI_LCD_TEXT);
        render_status_line(272U, g_ui.editing ? "SET SAVE / LONG EXIT" : "SET TO EDIT", UI_LCD_PANEL_ALT);
        render_hint_symbol("OK <");
        break;

    default:
        break;
    }
}

static void render_runtime_page(void)
{
    char summary[32];

    render_page_shell("RUNTIME", "3-POINT + CTRL");
    render_value_card(20U, 102U, 200U, 112U, "TOP/MID/BOT", "59.0/58.4/57.2", 2U, UI_LCD_TEXT);
    (void)snprintf(summary, sizeof(summary), "SET %.1fC / FUSED %.1fC", g_ui.last_params.set_temp_c, g_ui.last_temp.t_ctrl);
    render_status_line(246U, summary, UI_LCD_TEXT);
    render_status_line(268U, "3 POINT NORMAL", UI_LCD_PANEL_ALT);
    render_hint_symbol("< >");
}

static void render_alarm_trip_page(void)
{
    char alarm_text[16];

    render_page_shell("OVER TEMP", "LIMIT EXCEEDED");
    (void)snprintf(alarm_text, sizeof(alarm_text), "%.1fC", g_ui.last_params.alarm_threshold_c);
    render_value_card(20U, 106U, 200U, 96U, "SAFE MAX", alarm_text, 5U, UI_LCD_ALERT);
    render_status_line(240U, "HEAT FORCED OFF", UI_LCD_ALERT);
    render_status_line(264U, "RECOVER < 58.0C FOR 60S", UI_LCD_PANEL_ALT);
    render_hint_symbol("OK");
}

static void render_schedule_page(void)
{
    render_page_shell("POWER CHECK", "BOOT RESULT");
    render_value_card(20U, 102U, 200U, 112U, "RESULT", g_ui.last_temp.sensor_fault ? "DEGRADED" : "PASSED", 3U,
                      g_ui.last_temp.sensor_fault ? UI_LCD_WARM : UI_LCD_GOOD);
    render_status_line(246U, g_ui.last_temp.sensor_fault ? "LIMIT: SCHEDULE OFF" : "SYSTEM READY", UI_LCD_TEXT);
    render_status_line(268U, "SET TO CONTINUE", UI_LCD_PANEL_ALT);
    render_hint_symbol("OK");
}

static void render_export_page(void)
{
    char value_text[16];
    char footer[28];

    render_page_shell("LOG EXPORT", "UART OUTPUT");
    (void)snprintf(value_text, sizeof(value_text), "%u", log_service_count());
    render_value_card(20U, 106U, 200U, 88U, "LOG COUNT", value_text, 5U, UI_LCD_GOOD);
    (void)snprintf(footer, sizeof(footer), "PERIOD %us", g_ui.last_params.log_period_s);
    render_status_line(246U, footer, UI_LCD_TEXT);
    render_status_line(268U, "UART CMD LOG_EXPORT", UI_LCD_PANEL_ALT);
    render_hint_symbol("OK v");
}

static void render_info_page(void)
{
    const char *status_text = "CHECK PROBE WIRING";
    uint16_t status_color = UI_LCD_TEXT;

    if (g_ui.info_reset_feedback_ticks > 0U)
    {
        status_text = "DEFAULTS APPLIED";
        status_color = UI_LCD_GOOD;
    }
    else if (g_ui.info_reset_arm_ticks > 0U)
    {
        status_text = "HOLD AGAIN RESET";
        status_color = UI_LCD_ALERT;
    }

    render_page_shell("PROBE FAULT", "DEGRADED MODE");
    render_value_card(20U, 102U, 200U, 112U, "FAULT ITEM", "LOW PROBE OFF", 2U, UI_LCD_ALERT);
    render_status_line(246U, "OUTPUT LIMIT 55%", UI_LCD_WARM);
    render_status_line(268U, status_text, status_color);
    render_hint_symbol("OK");
}
#endif

static void render_page(void)
{
    char line0[HW_OLED_LINE_CHARS + 1U];
    char line1[HW_OLED_LINE_CHARS + 1U];
    char line2[HW_OLED_LINE_CHARS + 1U];
    char line3[HW_OLED_LINE_CHARS + 1U];

    (void)snprintf(line0, sizeof(line0), "%s %s%s", mode_text(g_ui.last_mode), page_text(g_ui.page), g_ui.editing ? "*" : "");
    (void)snprintf(line1, sizeof(line1), "TC %.1f ST %.1f", g_ui.last_temp.t_ctrl, g_ui.last_params.set_temp_c);
    (void)snprintf(line2, sizeof(line2), "P %.0f H %d A %d", g_ui.last_pid_out, g_ui.last_heater_on, g_ui.last_alarm_on);

    switch (g_ui.page)
    {
    case UI_PAGE_SET_TEMP:
        (void)snprintf(line3, sizeof(line3), "SET %.1fC", g_ui.last_params.set_temp_c);
        break;
    case UI_PAGE_PID:
        if (detect_pid_preset(&g_ui.last_params) == 0U)
        {
            (void)snprintf(line3, sizeof(line3), "PRESET FAST");
        }
        else if (detect_pid_preset(&g_ui.last_params) == 2U)
        {
            (void)snprintf(line3, sizeof(line3), "PRESET WARM");
        }
        else
        {
            (void)snprintf(line3, sizeof(line3), "PRESET BAL");
        }
        break;
    case UI_PAGE_ALARM:
        if (g_ui.last_alarm_on)
        {
            (void)snprintf(line3, sizeof(line3), "ALM %.1fC", g_ui.last_params.alarm_threshold_c);
        }
        else
        {
            (void)snprintf(line3, sizeof(line3), "TOP %.1f MID %.1f", g_ui.last_temp.t1, g_ui.last_temp.t2);
        }
        break;
    case UI_PAGE_SCHEDULE:
        (void)snprintf(line2, sizeof(line2), "POST BOOT CHECK");
        (void)snprintf(line3, sizeof(line3), "%s", g_ui.last_temp.sensor_fault ? "DEGRADED" : "PASSED");
        break;
    case UI_PAGE_EXPORT:
        (void)snprintf(line2,
                       sizeof(line2),
                       "LOG %u PER %us",
                       log_service_count(),
                       g_ui.last_params.log_period_s);
        (void)snprintf(line3, sizeof(line3), "CMD LOG_EXPORT");
        break;
    case UI_PAGE_INFO:
        if (g_ui.info_reset_feedback_ticks > 0U)
        {
            (void)snprintf(line3, sizeof(line3), "DEFAULTS APPLIED");
        }
        else if (g_ui.info_reset_arm_ticks > 0U)
        {
            (void)snprintf(line3, sizeof(line3), "HOLD AGAIN RESET");
        }
        else if (g_ui.last_temp.sensor_fault)
        {
            (void)snprintf(line3, sizeof(line3), "PROBE FAULT");
        }
        else
        {
            (void)snprintf(line3, sizeof(line3), "LONG SET=RESET");
        }
        break;
    case UI_PAGE_HOME:
    default:
        (void)snprintf(line3, sizeof(line3), "T1 %.1f T2 %.1f", g_ui.last_temp.t1, g_ui.last_temp.t2);
        break;
    }

    {
        const char *new_lines[HW_OLED_LINE_COUNT] = { line0, line1, line2, line3 };
        unsigned int i;
        int changed = (g_ui.splash_ticks > 0U) ? 1 : 0;

        for (i = 0U; i < HW_OLED_LINE_COUNT; ++i)
        {
            if ((!g_ui.render_cache_valid) || (strcmp(g_ui.rendered_lines[i], new_lines[i]) != 0))
            {
                strncpy(g_ui.rendered_lines[i], new_lines[i], HW_OLED_LINE_CHARS);
                g_ui.rendered_lines[i][HW_OLED_LINE_CHARS] = '\0';
                hw_oled_draw_text((uint8_t)i, g_ui.rendered_lines[i]);
                changed = 1;
            }
        }

        if (changed)
        {
#if defined(USE_HAL_DRIVER)
            if (g_ui.splash_ticks > 0U)
            {
                render_startup_splash();
            }
            else if (g_ui.page == UI_PAGE_HOME)
            {
                render_home_dashboard();
            }
            else if ((g_ui.page == UI_PAGE_SET_TEMP) ||
                     (g_ui.page == UI_PAGE_PID))
            {
                render_settings_page();
            }
            else if (g_ui.page == UI_PAGE_ALARM)
            {
                if (g_ui.last_alarm_on)
                {
                    render_alarm_trip_page();
                }
                else
                {
                    render_runtime_page();
                }
            }
            else if (g_ui.page == UI_PAGE_SCHEDULE)
            {
                render_schedule_page();
            }
            else if (g_ui.page == UI_PAGE_EXPORT)
            {
                render_export_page();
            }
            else if (g_ui.page == UI_PAGE_INFO)
            {
                render_info_page();
            }
            else
            {
                hw_oled_clear();
                for (i = 0U; i < HW_OLED_LINE_COUNT; ++i)
                {
                    hw_oled_draw_text((uint8_t)i, g_ui.rendered_lines[i]);
                }
                hw_oled_refresh();
            }
#else
            hw_oled_refresh();
#endif
            g_ui.render_cache_valid = 1;
        }
    }
}

void ui_service_init(void)
{
    g_ui.page = UI_PAGE_HOME;
    g_ui.editing = 0;
    g_ui.pid_field = 0U;
    g_ui.pending_key = UI_KEY_NONE;
    g_ui.q_head = 0U;
    g_ui.q_tail = 0U;
    {
        unsigned int i;
        for (i = 0U; i < (unsigned int)HW_KEY_COUNT; ++i)
        {
            g_ui.key_track[i].prev_pressed = 0;
            g_ui.key_track[i].hold_ticks = 0U;
            g_ui.key_track[i].long_fired = 0;
            g_ui.key_track[i].short_fired = 0;
            g_ui.key_track[i].repeat_ticks = 0U;
        }
    }
    g_ui.last_mode = APP_MODE_IDLE;
    g_ui.last_temp.t1 = 0.0f;
    g_ui.last_temp.t2 = 0.0f;
    g_ui.last_temp.t3 = 0.0f;
    g_ui.last_temp.t_ctrl = 0.0f;
    g_ui.last_temp.valid_mask = 0U;
    g_ui.last_temp.sensor_degraded = false;
    g_ui.last_temp.sensor_fault = false;
    g_ui.last_params.set_temp_c = 0.0f;
    g_ui.last_params.alarm_threshold_c = 0.0f;
    g_ui.last_params.kp = 0.0f;
    g_ui.last_params.ki = 0.0f;
    g_ui.last_params.kd = 0.0f;
    g_ui.last_pid_out = 0.0f;
    g_ui.last_heater_on = 0;
    g_ui.last_alarm_on = 0;
    g_ui.info_reset_feedback_ticks = 0U;
    g_ui.info_reset_arm_ticks = 0U;
    g_ui.splash_ticks = UI_SPLASH_TICKS;
    g_ui.render_cache_valid = 0;
    {
        unsigned int i;
        for (i = 0U; i < HW_OLED_LINE_COUNT; ++i)
        {
            g_ui.rendered_lines[i][0] = '\0';
        }
    }

    hw_key_init();
    hw_oled_init();
    hw_oled_clear();
    hw_oled_refresh();
}

void ui_service_tick_100ms(app_params_t *params)
{
    ui_key_event_t key;

    if (g_ui.info_reset_feedback_ticks > 0U)
    {
        g_ui.info_reset_feedback_ticks--;
    }
    if (g_ui.info_reset_arm_ticks > 0U)
    {
        g_ui.info_reset_arm_ticks--;
    }

    poll_keys_to_events();
    if (g_ui.pending_key != UI_KEY_NONE)
    {
        (void)queue_push(g_ui.pending_key);
        g_ui.pending_key = UI_KEY_NONE;
    }

    while (queue_pop(&key))
    {
        ui_redraw_mode_t redraw_mode = process_key_event(params, key);

        if (redraw_mode != UI_REDRAW_NONE)
        {
            render_page_sync(params, redraw_mode);
        }
    }
}

void ui_service_tick_200ms(app_mode_t mode, const temp_snapshot_t *temp, const app_params_t *params, float pid_out, int heater_on, int alarm_on)
{
    g_ui.last_mode = mode;
    if (temp != 0)
    {
        g_ui.last_temp = *temp;
    }
    if (params != 0)
    {
        g_ui.last_params = *params;
    }
    g_ui.last_pid_out = pid_out;
    g_ui.last_heater_on = heater_on;
    g_ui.last_alarm_on = alarm_on;

    update_exception_page_state();

    render_page();

    if (g_ui.splash_ticks > 0U)
    {
        g_ui.splash_ticks--;
        if (g_ui.splash_ticks == 0U)
        {
            g_ui.page = UI_PAGE_SCHEDULE;
            g_ui.render_cache_valid = 0;
        }
    }
}

void ui_service_inject_key_event(ui_key_event_t key)
{
    (void)queue_push(key);
}

ui_page_t ui_service_get_page(void)
{
    return g_ui.page;
}

int ui_service_is_editing(void)
{
    return g_ui.editing;
}

unsigned int ui_service_get_pid_field(void)
{
    return g_ui.pid_field;
}

