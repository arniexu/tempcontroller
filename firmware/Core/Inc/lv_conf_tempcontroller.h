#ifndef LV_CONF_TEMPCONTROLLER_H
#define LV_CONF_TEMPCONTROLLER_H

/*
 * Project-local LVGL configuration.
 * Keep this file focused on TempController constraints so upstream LVGL updates
 * do not get mixed with product-specific tuning.
 */

#define LV_COLOR_DEPTH                 16
#define LV_COLOR_16_SWAP               0

#define LV_USE_LOG                     0
#define LV_USE_ASSERT_NULL             0
#define LV_USE_ASSERT_MALLOC           0
#define LV_USE_ASSERT_STYLE            0
#define LV_USE_ASSERT_MEM_INTEGRITY    0
#define LV_USE_ASSERT_OBJ              0

#define LV_MEM_CUSTOM                  0

#define LV_DEF_REFR_PERIOD             20

#define LV_USE_OS                      LV_OS_NONE

#define LV_DRAW_SW_COMPLEX             0
#define LV_USE_THEME_DEFAULT           0
#define LV_USE_THEME_SIMPLE            0
#define LV_USE_THEME_MONO              0

#define LV_USE_GROUP                   0
#define LV_USE_ANIMIMG                 0
#define LV_USE_IMAGE                   0

#define LV_FONT_MONTSERRAT_14          1
#define LV_FONT_MONTSERRAT_20          1

#define LV_USE_LABEL                   1
#define LV_USE_BUTTON                  0
#define LV_USE_BAR                     0
#define LV_USE_SLIDER                  0
#define LV_USE_SPAN                    0

#endif
