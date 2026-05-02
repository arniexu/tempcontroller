#include "ui_lvgl_view.h"

#include "app_config.h"

#if (APP_USE_LVGL_UI == 1U)

#include "lvgl.h"

#include <stdio.h>

static lv_obj_t *g_title_label = 0;
static lv_obj_t *g_temp_label = 0;
static lv_obj_t *g_set_label = 0;
static lv_obj_t *g_state_label = 0;

void ui_lvgl_view_create_home(void)
{
    lv_obj_t *screen = lv_screen_active();

    lv_obj_clean(screen);
    lv_obj_set_style_bg_color(screen, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, 0);

    g_title_label = lv_label_create(screen);
    lv_label_set_text(g_title_label, "Water Temp");
    lv_obj_set_style_text_color(g_title_label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(g_title_label, LV_ALIGN_TOP_MID, 0, 20);

    g_temp_label = lv_label_create(screen);
    lv_label_set_text(g_temp_label, "0.0C");
    lv_obj_set_style_text_color(g_temp_label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(g_temp_label, LV_ALIGN_CENTER, 0, -30);

    g_set_label = lv_label_create(screen);
    lv_label_set_text(g_set_label, "SET 45.0C");
    lv_obj_set_style_text_color(g_set_label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(g_set_label, LV_ALIGN_CENTER, 0, 20);

    g_state_label = lv_label_create(screen);
    lv_label_set_text(g_state_label, "HEATER OFF");
    lv_obj_set_style_text_color(g_state_label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(g_state_label, LV_ALIGN_BOTTOM_MID, 0, -24);
}

void ui_lvgl_view_update_home(float t_ctrl,
                              float set_temp,
                              int heater_on,
                              int alarm_on)
{
    char temp_text[24];
    char set_text[24];

    if ((g_temp_label == 0) || (g_set_label == 0) || (g_state_label == 0))
    {
        return;
    }

    (void)snprintf(temp_text, sizeof(temp_text), "%.1fC", (double)t_ctrl);
    (void)snprintf(set_text, sizeof(set_text), "SET %.1fC", (double)set_temp);

    lv_label_set_text(g_temp_label, temp_text);
    lv_label_set_text(g_set_label, set_text);
    lv_label_set_text(g_state_label,
                      alarm_on ? "ALARM" : (heater_on ? "HEATER ON" : "HEATER OFF"));
}

#else

void ui_lvgl_view_create_home(void)
{
}

void ui_lvgl_view_update_home(float t_ctrl,
                              float set_temp,
                              int heater_on,
                              int alarm_on)
{
    (void)t_ctrl;
    (void)set_temp;
    (void)heater_on;
    (void)alarm_on;
}

#endif
