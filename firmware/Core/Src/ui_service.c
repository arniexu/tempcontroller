#include "ui_service.h"

#include <stdio.h>
#include <string.h>

#include "hw_platform_port.h"
#include "log_service.h"

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
        int set_long_fired;
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
    int render_cache_valid;
    char rendered_lines[HW_OLED_LINE_COUNT][HW_OLED_LINE_CHARS + 1U];
} ui_ctx_t;

static ui_ctx_t g_ui;

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

static void process_key_event(app_params_t *params, ui_key_event_t key)
{
    if (key == UI_KEY_NONE)
    {
        return;
    }

    if (!g_ui.editing)
    {
        switch (key)
        {
        case UI_KEY_UP:
        case UI_KEY_UP_REPEAT:
            page_next();
            break;
        case UI_KEY_DOWN:
        case UI_KEY_DOWN_REPEAT:
            page_prev();
            break;
        case UI_KEY_BACK:
            g_ui.page = UI_PAGE_HOME;
            break;
        case UI_KEY_SET:
            if (page_editable(g_ui.page))
            {
                g_ui.editing = 1;
                g_ui.pid_field = 0U;
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
                }
                else
                {
                    g_ui.info_reset_arm_ticks = 30U;
                }
            }
            else if (page_editable(g_ui.page))
            {
                g_ui.editing = 1;
                g_ui.pid_field = 0U;
            }
            break;
        default:
            break;
        }
        return;
    }

    switch (key)
    {
    case UI_KEY_UP:
    case UI_KEY_UP_REPEAT:
        adjust_param(params, +1);
        break;
    case UI_KEY_DOWN:
    case UI_KEY_DOWN_REPEAT:
        adjust_param(params, -1);
        break;
    case UI_KEY_BACK:
        g_ui.editing = 0;
        g_ui.pid_field = 0U;
        break;
    case UI_KEY_SET:
        if (g_ui.page == UI_PAGE_PID)
        {
            g_ui.pid_field = (g_ui.pid_field + 1U) % 3U;
        }
        else
        {
            g_ui.editing = 0;
        }
        break;
    case UI_KEY_SET_LONG:
        g_ui.editing = 0;
        g_ui.pid_field = 0U;
        break;
    default:
        break;
    }
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
                g_ui.key_track[i].set_long_fired = 0;
                g_ui.key_track[i].repeat_ticks = 0U;
            }
            else
            {
                g_ui.key_track[i].hold_ticks++;
            }

            if (i == (unsigned int)HW_KEY_SET)
            {
                if ((!g_ui.key_track[i].set_long_fired) && (g_ui.key_track[i].hold_ticks >= 10U))
                {
                    (void)queue_push(UI_KEY_SET_LONG);
                    g_ui.key_track[i].set_long_fired = 1;
                }
            }
            else if ((i == (unsigned int)HW_KEY_UP) || (i == (unsigned int)HW_KEY_DOWN))
            {
                if (g_ui.key_track[i].hold_ticks >= 5U)
                {
                    g_ui.key_track[i].repeat_ticks++;
                    if (g_ui.key_track[i].repeat_ticks >= 2U)
                    {
                        (void)queue_push((i == (unsigned int)HW_KEY_UP) ? UI_KEY_UP_REPEAT : UI_KEY_DOWN_REPEAT);
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
                    if (!g_ui.key_track[i].set_long_fired)
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
                    (void)queue_push(UI_KEY_DOWN);
                }
                else if (i == (unsigned int)HW_KEY_BACK)
                {
                    (void)queue_push(UI_KEY_BACK);
                }

                g_ui.key_track[i].prev_pressed = 0;
                g_ui.key_track[i].hold_ticks = 0U;
                g_ui.key_track[i].set_long_fired = 0;
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
        int changed = 0;

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
            hw_oled_refresh();
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
            g_ui.key_track[i].set_long_fired = 0;
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
        process_key_event(params, key);
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
