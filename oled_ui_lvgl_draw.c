#include "lvgl.h"
#include <stdbool.h>
#include <stdio.h>

typedef enum {
    OLED_FOCUS_TARGET = 0,
    OLED_FOCUS_ERROR = 1
} oled_focus_t;

static lv_obj_t *ui_status_label;
static lv_obj_t *ui_temp_label;
static lv_obj_t *ui_tolerance_label;

static lv_style_t style_screen;
static lv_style_t style_ascii;
static lv_style_t style_status;
static lv_style_t style_temp;

static void init_styles(void)
{
    static bool inited = false;
    if (inited) {
        return;
    }
    inited = true;

    lv_style_init(&style_screen);
    lv_style_set_bg_opa(&style_screen, LV_OPA_COVER);
    lv_style_set_bg_color(&style_screen, lv_color_hex(0x05110b));
    lv_style_set_text_color(&style_screen, lv_color_hex(0xd9ffe9));
    lv_style_set_border_width(&style_screen, 0);
    lv_style_set_pad_all(&style_screen, 0);

    lv_style_init(&style_ascii);
    lv_style_set_text_font(&style_ascii, &lv_font_montserrat_8);
    lv_style_set_text_color(&style_ascii, lv_color_hex(0xd9ffe9));
    lv_style_set_bg_opa(&style_ascii, LV_OPA_TRANSP);
    lv_style_set_pad_all(&style_ascii, 0);

    lv_style_init(&style_status);
    lv_style_set_text_font(&style_status, &lv_font_montserrat_12);
    lv_style_set_text_color(&style_status, lv_color_hex(0xd9ffe9));
    lv_style_set_border_width(&style_status, 1);
    lv_style_set_border_color(&style_status, lv_color_hex(0xd9ffe9));
    lv_style_set_pad_hor(&style_status, 1);
    lv_style_set_pad_ver(&style_status, 0);
    lv_style_set_radius(&style_status, 0);
    lv_style_set_bg_opa(&style_status, LV_OPA_TRANSP);

    lv_style_init(&style_temp);
    lv_style_set_text_font(&style_temp, &lv_font_montserrat_28);
    lv_style_set_text_color(&style_temp, lv_color_hex(0xd9ffe9));
    lv_style_set_bg_opa(&style_temp, LV_OPA_TRANSP);
    lv_style_set_pad_all(&style_temp, 0);
}

void tempcontroller_oled_ui_set_focus(oled_focus_t focus)
{
    (void)focus;
}

void tempcontroller_oled_ui_set_state(bool heating)
{
    lv_label_set_text(ui_status_label, heating ? "加热中" : "恒温保持");
}

void tempcontroller_oled_ui_set_values(float current_temp, float target_temp, float error)
{
    char buf[16];
    (void)target_temp;

    lv_snprintf(buf, sizeof(buf), "%.1fC", (double)current_temp);
    lv_label_set_text(ui_temp_label, buf);

    lv_snprintf(buf, sizeof(buf), "±%.1fC", (double)error);
    lv_label_set_text(ui_tolerance_label, buf);
}

void tempcontroller_oled_ui_create(lv_obj_t *parent)
{
    init_styles();

    lv_obj_set_size(parent, 128, 64);
    lv_obj_add_style(parent, &style_screen, LV_PART_MAIN);
    lv_obj_clear_flag(parent, LV_OBJ_FLAG_SCROLLABLE);

    ui_status_label = lv_label_create(parent);
    lv_obj_add_style(ui_status_label, &style_status, LV_PART_MAIN);
    lv_label_set_text(ui_status_label, "加热中");
    lv_obj_set_pos(ui_status_label, 0, 0);

    ui_temp_label = lv_label_create(parent);
    lv_obj_add_style(ui_temp_label, &style_temp, LV_PART_MAIN);
    lv_label_set_text(ui_temp_label, "23.8C");
    lv_obj_set_pos(ui_temp_label, 14, 18);

    ui_tolerance_label = lv_label_create(parent);
    lv_obj_add_style(ui_tolerance_label, &style_ascii, LV_PART_MAIN);
    lv_label_set_text(ui_tolerance_label, "±0.5C");
    lv_obj_set_pos(ui_tolerance_label, 84, 54);
}
