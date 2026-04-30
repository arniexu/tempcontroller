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
#if defined(USE_STDPERIPH_DRIVER)
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
    return (page == UI_PAGE_SET_TEMP) || (page == UI_PAGE_PID) || (page == UI_PAGE_ALARM);
}

static void page_next(void)
{
    g_ui.page = (ui_page_t)(((unsigned int)g_ui.page + 1U) % (unsigned int)UI_PAGE_COUNT);
}

static void page_prev(void)
{
    if (g_ui.page == UI_PAGE_HOME)
    {
        g_ui.page = (ui_page_t)((unsigned int)UI_PAGE_COUNT - 1U);
    }
    else
    {
        g_ui.page = (ui_page_t)((unsigned int)g_ui.page - 1U);
    }
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
        if (g_ui.pid_field == 0U)
        {
            params->kp = clampf(params->kp + (0.2f * (float)dir), 0.0f, 30.0f);
        }
        else if (g_ui.pid_field == 1U)
        {
            params->ki = clampf(params->ki + (0.05f * (float)dir), 0.0f, 5.0f);
        }
        else
        {
            params->kd = clampf(params->kd + (0.5f * (float)dir), 0.0f, 50.0f);
        }
        break;
    case UI_PAGE_ALARM:
        params->alarm_threshold_c = clampf(params->alarm_threshold_c + (0.5f * (float)dir), 40.0f, 90.0f);
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
#if defined(USE_STDPERIPH_DRIVER)
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
            page_next();
            redraw_mode = UI_REDRAW_FULL;
            break;
        case UI_KEY_DOWN:
        case UI_KEY_DOWN_REPEAT:
            page_prev();
            redraw_mode = UI_REDRAW_FULL;
            break;
        case UI_KEY_BACK:
            g_ui.page = UI_PAGE_HOME;
            redraw_mode = UI_REDRAW_FULL;
            break;
        case UI_KEY_SET:
            if (page_editable(g_ui.page))
            {
                g_ui.editing = 1;
                g_ui.pid_field = 0U;
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
                g_ui.pid_field = 0U;
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
        redraw_mode = page_editable(g_ui.page) ? UI_REDRAW_SETTINGS_CARD : UI_REDRAW_FULL;
        break;
    case UI_KEY_DOWN:
    case UI_KEY_DOWN_REPEAT:
        adjust_param(params, -1);
        redraw_mode = page_editable(g_ui.page) ? UI_REDRAW_SETTINGS_CARD : UI_REDRAW_FULL;
        break;
    case UI_KEY_BACK:
        g_ui.editing = 0;
        g_ui.pid_field = 0U;
        redraw_mode = UI_REDRAW_FULL;
        break;
    case UI_KEY_SET:
        if (g_ui.page == UI_PAGE_PID)
        {
            g_ui.pid_field = (g_ui.pid_field + 1U) % 3U;
            redraw_mode = UI_REDRAW_FULL;
        }
        else
        {
            g_ui.editing = 0;
            redraw_mode = UI_REDRAW_FULL;
        }
        break;
    case UI_KEY_SET_LONG:
        g_ui.editing = 0;
        g_ui.pid_field = 0U;
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
                g_ui.key_track[i].repeat_ticks = 0U;
            }
            else
            {
                g_ui.key_track[i].hold_ticks++;
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
                    if (!g_ui.key_track[i].long_fired)
                    {
                        (void)queue_push(UI_KEY_SET);
                    }
                }
                else if (i == (unsigned int)HW_KEY_UP)
                {
                    (void)queue_push(UI_KEY_UP);
                }
                else if (i == (unsigned int)HW_KEY_DOWN)
                {
                    if (!g_ui.key_track[i].long_fired)
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

static void format_hhmm(unsigned int minutes, char *buf, unsigned int buf_len)
{
    unsigned int hh = (minutes / 60U) % 24U;
    unsigned int mm = minutes % 60U;

    (void)snprintf(buf, buf_len, "%02u:%02u", hh, mm);
}

static unsigned int schedule_next_boundary(unsigned int now_min, unsigned int start_min, unsigned int end_min)
{
    unsigned int start = start_min % 1440U;
    unsigned int end = end_min % 1440U;
    int active;

    if (start == end)
    {
        return start;
    }

    if (start < end)
    {
        active = ((now_min >= start) && (now_min < end)) ? 1 : 0;
    }
    else
    {
        active = ((now_min >= start) || (now_min < end)) ? 1 : 0;
    }

    return active ? end : start;
}

#if defined(USE_STDPERIPH_DRIVER)
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

static void render_coordinate_overlay(void)
{
    const uint16_t axis_x = 60U;
    const uint16_t axis_y = 60U;

    /* Coordinate overlay: origin at top-left, X increases right, Y increases down. */
    hw_oled_draw_line(0U, 0U, axis_x, 0U, UI_LCD_ALERT);
    hw_oled_draw_line(0U, 0U, 0U, axis_y, UI_LCD_GOOD);

    hw_oled_draw_line(axis_x, 0U, (uint16_t)(axis_x - 5U), 3U, UI_LCD_ALERT);
    hw_oled_draw_line(axis_x, 0U, (uint16_t)(axis_x - 5U), 0U, UI_LCD_ALERT);
    hw_oled_draw_line(0U, axis_y, 3U, (uint16_t)(axis_y - 5U), UI_LCD_GOOD);
    hw_oled_draw_line(0U, axis_y, 0U, (uint16_t)(axis_y - 5U), UI_LCD_GOOD);

    hw_oled_draw_text_xy(2U, 2U, "O(0,0)", 1U, UI_LCD_TEXT);
    hw_oled_draw_text_xy((uint16_t)(axis_x - 12U), 4U, "X+", 1U, UI_LCD_ALERT);
    hw_oled_draw_text_xy(4U, (uint16_t)(axis_y - 10U), "Y+", 1U, UI_LCD_GOOD);
}

static void render_key_state_overlay(void)
{
    char key_state[20];
    int set_on = hw_key_get_state(HW_KEY_SET) ? 1 : 0;
    int up_on = hw_key_get_state(HW_KEY_UP) ? 1 : 0;
    int down_on = hw_key_get_state(HW_KEY_DOWN) ? 1 : 0;
    int back_on = hw_key_get_state(HW_KEY_BACK) ? 1 : 0;

    (void)snprintf(key_state, sizeof(key_state), "K S%d U%d D%d B%d", set_on, up_on, down_on, back_on);
    hw_oled_fill_rect(0U, 306U, BSP_LCD_WIDTH, 14U, UI_LCD_BG);
    hw_oled_draw_text_xy(2U, 308U, key_state, 1U, UI_LCD_TEXT);
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

static void render_startup_splash(void)
{
    hw_oled_fill_rect(0U, 0U, BSP_LCD_WIDTH, BSP_LCD_HEIGHT, UI_LCD_BG);
    render_centered_text(88U, "TEMP", 5U, UI_LCD_TEXT);
    render_centered_text(142U, "CTRL", 5U, UI_LCD_WARM);
    hw_oled_draw_line(46U, 202U, 194U, 202U, UI_LCD_ACCENT);
    render_centered_text(228U, "STM32 WATER", 2U, UI_LCD_PANEL_ALT);
    render_centered_text(252U, "CONTROL", 2U, UI_LCD_PANEL_ALT);
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
    (void)snprintf(chip_text, sizeof(chip_text), "%s", mode_text(g_ui.last_mode));
    (void)snprintf(footer_note, sizeof(footer_note), "HEATER %s", g_ui.last_heater_on ? "ON" : "OFF");
    (void)snprintf(footer_sub,
                   sizeof(footer_sub),
                   "%s",
                   g_ui.last_alarm_on ? "ALARM ACTIVE" : "STABLE CONTROL");

    hw_oled_fill_rect(0U, 0U, BSP_LCD_WIDTH, BSP_LCD_HEIGHT, UI_LCD_BG);

    hw_oled_draw_text_xy(26U, 26U, "CURRENT TEMP", 2U, UI_LCD_TEXT);
    render_chip(168U, 24U, 48U, chip_text);
    hw_oled_draw_text_xy(28U, 78U, temp_big, 6U, UI_LCD_WARM);

    hw_oled_draw_line(24U, 182U, 216U, 182U, UI_LCD_PANEL_ALT);
    hw_oled_draw_text_xy(28U, 200U, "SET", 2U, UI_LCD_TEXT);
    hw_oled_draw_text_xy(96U, 196U, set_text, 3U, UI_LCD_GOOD);

    hw_oled_draw_text_xy(28U, 254U, footer_note, 2U, g_ui.last_heater_on ? UI_LCD_TEXT : UI_LCD_PANEL_ALT);
    hw_oled_draw_text_xy(28U, 284U, footer_sub, 1U, g_ui.last_alarm_on ? UI_LCD_ALERT : UI_LCD_PANEL_ALT);
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
        render_value_card(20U, 106U, 200U, 110U, "SETPOINT", value_text, 5U, UI_LCD_GOOD);
        break;
    case UI_PAGE_PID:
        if (g_ui.pid_field == 0U)
        {
            (void)snprintf(value_text, sizeof(value_text), "K%.1f", g_ui.last_params.kp);
            render_value_card(20U, 106U, 200U, 110U, "KP", value_text, 5U, UI_LCD_ACCENT);
        }
        else if (g_ui.pid_field == 1U)
        {
            (void)snprintf(value_text, sizeof(value_text), "I%.2f", g_ui.last_params.ki);
            render_value_card(20U, 106U, 200U, 110U, "KI", value_text, 5U, UI_LCD_GOOD);
        }
        else
        {
            (void)snprintf(value_text, sizeof(value_text), "D%.1f", g_ui.last_params.kd);
            render_value_card(20U, 106U, 200U, 110U, "KD", value_text, 5U, UI_LCD_WARM);
        }
        break;
    case UI_PAGE_ALARM:
        (void)snprintf(value_text, sizeof(value_text), "%.1fC", g_ui.last_params.alarm_threshold_c);
        render_value_card(20U, 106U, 200U, 110U, "ALARM LIMIT", value_text, 5U, UI_LCD_ALERT);
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
    const char *title = page_text(g_ui.page);
    const char *subtitle = mode_text(g_ui.last_mode);
    const char *card_label = "";
    char value_text[16];
    char footer[28];

    switch (g_ui.page)
    {
    case UI_PAGE_SET_TEMP:
        subtitle = mode_text(g_ui.last_mode);
        card_label = "SETPOINT";
        render_page_shell(title, subtitle);
        (void)snprintf(value_text, sizeof(value_text), "%.1fC", g_ui.last_params.set_temp_c);
        render_value_card(20U, 106U, 200U, 110U, card_label, value_text, 5U, UI_LCD_GOOD);
        (void)snprintf(footer, sizeof(footer), "CTRL %.1fC", g_ui.last_temp.t_ctrl);
        render_status_line(246U, footer, UI_LCD_TEXT);
        render_status_line(272U, g_ui.editing ? "UP/DOWN ADJUST" : "SET TO EDIT", UI_LCD_PANEL_ALT);
        break;

    case UI_PAGE_PID:
        (void)snprintf(footer, sizeof(footer), "FIELD %u / 3", (unsigned int)(g_ui.pid_field + 1U));
        subtitle = footer;
        render_page_shell(title, subtitle);
        if (g_ui.pid_field == 0U)
        {
            (void)snprintf(value_text, sizeof(value_text), "K%.1f", g_ui.last_params.kp);
            card_label = "KP";
        }
        else if (g_ui.pid_field == 1U)
        {
            (void)snprintf(value_text, sizeof(value_text), "I%.2f", g_ui.last_params.ki);
            card_label = "KI";
        }
        else
        {
            (void)snprintf(value_text, sizeof(value_text), "D%.1f", g_ui.last_params.kd);
            card_label = "KD";
        }
        render_value_card(20U,
                          106U,
                          200U,
                          110U,
                          card_label,
                          value_text,
                          5U,
                          g_ui.pid_field == 0U ? UI_LCD_ACCENT : (g_ui.pid_field == 1U ? UI_LCD_GOOD : UI_LCD_WARM));
        (void)snprintf(footer, sizeof(footer), "FIELD %u/3", (unsigned int)(g_ui.pid_field + 1U));
        render_status_line(246U, footer, UI_LCD_TEXT);
        render_status_line(272U, g_ui.editing ? "SET NEXT / LONG SET EXIT" : "SET TO EDIT", UI_LCD_PANEL_ALT);
        break;

    case UI_PAGE_ALARM:
        render_page_shell(title, "SAFETY LIMIT");
        (void)snprintf(value_text, sizeof(value_text), "%.1fC", g_ui.last_params.alarm_threshold_c);
        render_value_card(20U, 106U, 200U, 110U, "ALARM LIMIT", value_text, 5U, UI_LCD_ALERT);
        (void)snprintf(footer, sizeof(footer), "%s", g_ui.last_alarm_on ? "ALARM ACTIVE" : "ALARM READY");
        render_status_line(246U, footer, g_ui.last_alarm_on ? UI_LCD_ALERT : UI_LCD_TEXT);
        render_status_line(272U, g_ui.editing ? "UP/DOWN ADJUST" : "SET TO EDIT", UI_LCD_PANEL_ALT);
        break;

    default:
        break;
    }
}

static void render_schedule_page(void)
{
    char now_text[6] = "--:--";
    char next_text[6] = "--:--";
    char start_text[6];
    char end_text[6];
    char value_text[18];
    char footer[28];
    uint16_t now_min;

    format_hhmm(g_ui.last_params.schedule_start_min, start_text, sizeof(start_text));
    format_hhmm(g_ui.last_params.schedule_end_min, end_text, sizeof(end_text));

    if (hw_rtc_get_minutes_of_day(&now_min))
    {
        format_hhmm(now_min, now_text, sizeof(now_text));
        format_hhmm(schedule_next_boundary((unsigned int)now_min,
                                           g_ui.last_params.schedule_start_min,
                                           g_ui.last_params.schedule_end_min),
                    next_text,
                    sizeof(next_text));
    }

    render_page_shell("SCHEDULE", g_ui.last_params.schedule_enabled != 0U ? "WINDOW ON" : "WINDOW OFF");
    (void)snprintf(value_text, sizeof(value_text), "%s-%s", start_text, end_text);
    render_value_card(20U, 106U, 200U, 88U, "ACTIVE WINDOW", value_text, 3U, UI_LCD_ACCENT);
    (void)snprintf(footer, sizeof(footer), "NOW %s  NEXT %s", now_text, next_text);
    render_status_line(234U, footer, UI_LCD_TEXT);
    render_status_line(260U, g_ui.last_heater_on ? "HEAT PERMITTED" : "HEAT BLOCKED", UI_LCD_PANEL_ALT);
}

static void render_export_page(void)
{
    char value_text[16];
    char footer[28];

    render_page_shell("EXPORT", "UART OUTPUT");
    (void)snprintf(value_text, sizeof(value_text), "%u", log_service_count());
    render_value_card(20U, 106U, 200U, 88U, "LOG COUNT", value_text, 5U, UI_LCD_GOOD);
    (void)snprintf(footer, sizeof(footer), "PERIOD %us", g_ui.last_params.log_period_s);
    render_status_line(234U, footer, UI_LCD_TEXT);
    render_status_line(260U, "UART CMD LOG_EXPORT", UI_LCD_PANEL_ALT);
}

static void render_info_page(void)
{
    const char *status_text = "LONG SET=RESET";
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

    render_page_shell("INFO", "DEVICE PROFILE");
    render_value_card(20U, 106U, 200U, 88U, "PROFILE", "TEMP CTRL", 3U, UI_LCD_ACCENT);
    render_status_line(234U, status_text, status_color);
    render_status_line(260U, "LONG SET TO RESET", UI_LCD_PANEL_ALT);
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
        (void)snprintf(line3, sizeof(line3), "K%.1f I%.2f D%.1f", g_ui.last_params.kp, g_ui.last_params.ki, g_ui.last_params.kd);
        break;
    case UI_PAGE_ALARM:
        (void)snprintf(line3, sizeof(line3), "ALM %.1fC", g_ui.last_params.alarm_threshold_c);
        break;
    case UI_PAGE_SCHEDULE:
        {
            char start_text[6];
            char end_text[6];
            char now_text[6] = "--:--";
            char next_text[6] = "--:--";
            uint16_t now_min;

            format_hhmm(g_ui.last_params.schedule_start_min, start_text, sizeof(start_text));
            format_hhmm(g_ui.last_params.schedule_end_min, end_text, sizeof(end_text));

            if (hw_rtc_get_minutes_of_day(&now_min))
            {
                format_hhmm(now_min, now_text, sizeof(now_text));
                format_hhmm(schedule_next_boundary((unsigned int)now_min,
                                                   g_ui.last_params.schedule_start_min,
                                                   g_ui.last_params.schedule_end_min),
                            next_text,
                            sizeof(next_text));
            }

            (void)snprintf(line2, sizeof(line2), "NOW %s NX %s", now_text, next_text);
            (void)snprintf(line3,
                           sizeof(line3),
                           "%s %s-%s",
                           g_ui.last_params.schedule_enabled != 0U ? "ON" : "OFF",
                           start_text,
                           end_text);
        }
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
#if defined(USE_STDPERIPH_DRIVER)
            if (g_ui.splash_ticks > 0U)
            {
                render_startup_splash();
            }
            else if (g_ui.page == UI_PAGE_HOME)
            {
                render_home_dashboard();
            }
            else if ((g_ui.page == UI_PAGE_SET_TEMP) ||
                     (g_ui.page == UI_PAGE_PID) ||
                     (g_ui.page == UI_PAGE_ALARM))
            {
                render_settings_page();
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

            render_coordinate_overlay();
            render_key_state_overlay();
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
#if defined(USE_STDPERIPH_DRIVER)
    render_key_state_overlay();
#endif
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

    render_page();

    if (g_ui.splash_ticks > 0U)
    {
        g_ui.splash_ticks--;
        if (g_ui.splash_ticks == 0U)
        {
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
