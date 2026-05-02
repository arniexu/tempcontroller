#ifndef UI_LVGL_VIEW_H
#define UI_LVGL_VIEW_H

void ui_lvgl_view_create_home(void);
void ui_lvgl_view_update_home(float t_ctrl,
                              float set_temp,
                              int heater_on,
                              int alarm_on);

#endif
