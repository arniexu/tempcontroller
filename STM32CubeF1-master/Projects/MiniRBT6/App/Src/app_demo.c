#include "app_demo.h"

#include <string.h>

#include "lcd8080.h"
#include "tinygfx.h"

typedef enum
{
  APP_TAB_NORMAL = 0,
  APP_TAB_GUIDE,
  APP_TAB_EXPERT,
  APP_TAB_COUNT
} AppTab;

typedef enum
{
  APP_VAL_ASCII_ONLY = 0,
  APP_VAL_ZH_ONLY,
  APP_VAL_ASCII_ZH
} AppValueType;

typedef struct
{
  const char *label_ascii;
  const uint16_t *label_zh;
  uint8_t label_zh_len;
  AppValueType value_type;
  const char *value_ascii;
  const uint16_t *value_zh;
  uint8_t value_zh_len;
} AppRow;

#define ARR_LEN(a) ((uint8_t)(sizeof(a) / sizeof((a)[0])))

#define UI_BG      LCD_COLOR_BLACK
#define UI_FG      LCD_COLOR_GREEN
#define UI_FG_DIM  LCD_COLOR_GRAY
#define UI_INV_BG  LCD_COLOR_GREEN
#define UI_INV_FG  LCD_COLOR_BLACK

#define UI_HDR_X     10U
#define UI_HDR_Y     10U
#define UI_HDR_W    220U
#define UI_HDR_H     20U

#define UI_TEMP_X    10U
#define UI_TEMP_Y    36U
#define UI_TEMP_W   220U
#define UI_TEMP_H    34U

#define UI_TAB_X     10U
#define UI_TAB_Y     76U
#define UI_TAB_W    220U
#define UI_TAB_H     24U

#define UI_BODY_X    10U
#define UI_BODY_Y   106U
#define UI_BODY_W   220U
#define UI_BODY_H   168U
#define UI_ROW_H     24U

#define UI_FOOT_X    10U
#define UI_FOOT_Y   282U
#define UI_FOOT_W   220U
#define UI_FOOT_H    28U

static const uint16_t TXT_TITLE[] = {0x6E29, 0x63A7, 0x5668};
static const uint16_t TXT_CUR_TEMP[] = {0x5F53, 0x524D, 0x6E29, 0x5EA6};
static const uint16_t TXT_OVER_TEMP[] = {0x8FC7, 0x6E29};
static const uint16_t TXT_DU[] = {0x5EA6};
static const uint16_t TXT_CONFIRM[] = {0x786E, 0x8BA4};

static const uint16_t TXT_TAB_NORMAL[] = {0x666E, 0x901A};
static const uint16_t TXT_TAB_GUIDE[] = {0x5F15, 0x5BFC};
static const uint16_t TXT_TAB_EXPERT[] = {0x4E13, 0x5BB6};

static const uint16_t TXT_SCENE_LABEL[] = {0x573A, 0x666F};
static const uint16_t TXT_SCENE_DAILY[] = {0x65E5, 0x5E38, 0x4FDD, 0x6E29};
static const uint16_t TXT_STEP_APPROACH[] = {0x903C, 0x8FD1, 0x76EE, 0x6807};
static const uint16_t TXT_PEAK_TEMP[] = {0x5CF0, 0x503C, 0x6E29, 0x5EA6};
static const uint16_t TXT_OVERSHOOT_BIG[] = {0x8D85, 0x8C03, 0x8F83, 0x5927};
static const uint16_t TXT_APPLY_SUG[] = {0x5E94, 0x7528, 0x5EFA, 0x8BAE};
static const uint16_t TXT_OBSERVING[] = {0x89C2, 0x5BDF, 0x4E2D};

static const AppRow g_normal_rows[] = {
  {"TGT", NULL, 0U, APP_VAL_ASCII_ZH, "60", TXT_DU, ARR_LEN(TXT_DU)},
  {NULL, TXT_SCENE_LABEL, ARR_LEN(TXT_SCENE_LABEL), APP_VAL_ZH_ONLY, NULL, TXT_SCENE_DAILY, ARR_LEN(TXT_SCENE_DAILY)}
};

static const AppRow g_guide_rows[] = {
  {"STEP", NULL, 0U, APP_VAL_ASCII_ZH, "2/5", TXT_STEP_APPROACH, ARR_LEN(TXT_STEP_APPROACH)},
  {"MET", NULL, 0U, APP_VAL_ZH_ONLY, NULL, TXT_PEAK_TEMP, ARR_LEN(TXT_PEAK_TEMP)},
  {"ISS", NULL, 0U, APP_VAL_ZH_ONLY, NULL, TXT_OVERSHOOT_BIG, ARR_LEN(TXT_OVERSHOOT_BIG)},
  {"ACT", NULL, 0U, APP_VAL_ZH_ONLY, NULL, TXT_APPLY_SUG, ARR_LEN(TXT_APPLY_SUG)},
  {"Kp", NULL, 0U, APP_VAL_ASCII_ONLY, "7.8", NULL, 0U},
  {"Ki", NULL, 0U, APP_VAL_ASCII_ONLY, "0.28", NULL, 0U},
  {"Kd", NULL, 0U, APP_VAL_ASCII_ONLY, "15.2", NULL, 0U},
  {"RES", NULL, 0U, APP_VAL_ZH_ONLY, NULL, TXT_OBSERVING, ARR_LEN(TXT_OBSERVING)}
};

static const AppRow g_expert_rows[] = {
  {"KP", NULL, 0U, APP_VAL_ASCII_ONLY, "1.6", NULL, 0U},
  {"KI", NULL, 0U, APP_VAL_ASCII_ONLY, "0.42", NULL, 0U},
  {"KD", NULL, 0U, APP_VAL_ASCII_ONLY, "0.08", NULL, 0U},
  {"TMIN", NULL, 0U, APP_VAL_ASCII_ZH, "20", TXT_DU, ARR_LEN(TXT_DU)},
  {"TMAX", NULL, 0U, APP_VAL_ASCII_ZH, "95", TXT_DU, ARR_LEN(TXT_DU)}
};

static const uint8_t g_selected_row[APP_TAB_COUNT] = {0U, 2U, 1U};
static uint8_t g_ui_static_drawn = 0U;

static void App_DrawPage(AppTab tab);
static void App_DrawHeader(void);
static void App_DrawTempBar(AppTab tab);
static void App_DrawTabs(AppTab tab);
static void App_DrawBody(AppTab tab);
static void App_DrawRows(uint16_t start_y, const AppRow *rows, uint8_t row_count, uint8_t selected_row);
static void App_DrawFooter(AppTab tab);
static void App_DrawRowLabel(uint16_t x, uint16_t y, const AppRow *row, uint16_t fg, uint16_t bg);
static void App_DrawRowValue(uint16_t x, uint16_t y, const AppRow *row, uint16_t fg, uint16_t bg);
static void App_DrawText(uint16_t x, uint16_t y, const char *text, uint16_t fg, uint16_t bg);
static void App_DrawZHText16(uint16_t x, uint16_t y, const uint16_t *codes, uint8_t count, uint16_t fg, uint16_t bg);
static void App_FillRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color);

void App_Demo_Init(void)
{
  LCD_InitWithProfile(0U);
  LCD_SetBacklightRaw(1U);
  LCD_SetPixelWriteMode(LCD_PIXEL_MODE_16BIT_NORMAL);
  TGFX_Init(LCD_WIDTH, LCD_HEIGHT);

  App_DrawPage(APP_TAB_NORMAL);
}

void App_Demo_Run(void)
{
  static uint32_t last_tick = 0U;
  static AppTab current_tab = APP_TAB_NORMAL;
  uint32_t now = HAL_GetTick();

  if ((now - last_tick) < 3500U)
  {
    return;
  }

  last_tick = now;
  current_tab = (AppTab)(((uint8_t)current_tab + 1U) % (uint8_t)APP_TAB_COUNT);
  App_DrawPage(current_tab);
}

static void App_DrawPage(AppTab tab)
{
  if (g_ui_static_drawn == 0U)
  {
    LCD_FillScreen(UI_BG);
    App_DrawHeader();
    g_ui_static_drawn = 1U;
  }

  App_DrawTempBar(tab);
  App_DrawTabs(tab);
  App_DrawBody(tab);
  App_DrawFooter(tab);
}

static void App_DrawHeader(void)
{
  TGFX_DrawRect((int16_t)UI_HDR_X, (int16_t)UI_HDR_Y, (int16_t)UI_HDR_W, (int16_t)UI_HDR_H, UI_FG_DIM);
  App_DrawZHText16((uint16_t)(UI_HDR_X + 4U), (uint16_t)(UI_HDR_Y + 2U), TXT_TITLE, ARR_LEN(TXT_TITLE), UI_FG, UI_BG);
  App_DrawText((uint16_t)(UI_HDR_X + UI_HDR_W - 68U), (uint16_t)(UI_HDR_Y + 2U), "12:30", UI_FG_DIM, UI_BG);
}

static void App_DrawTempBar(AppTab tab)
{
  const char *temp_text;
  uint8_t over_temp = 0U;

  if (tab == APP_TAB_NORMAL)
  {
    temp_text = "58.6";
  }
  else if (tab == APP_TAB_GUIDE)
  {
    temp_text = "61.2";
  }
  else
  {
    temp_text = "57.9";
  }

  App_FillRect(UI_TEMP_X, UI_TEMP_Y, UI_TEMP_W, UI_TEMP_H, UI_BG);
  TGFX_DrawRect((int16_t)UI_TEMP_X, (int16_t)UI_TEMP_Y, (int16_t)UI_TEMP_W, (int16_t)UI_TEMP_H, UI_FG_DIM);
  App_DrawZHText16((uint16_t)(UI_TEMP_X + 4U), (uint16_t)(UI_TEMP_Y + 6U), TXT_CUR_TEMP, ARR_LEN(TXT_CUR_TEMP), UI_FG_DIM, UI_BG);

  App_DrawText((uint16_t)(UI_TEMP_X + 110U), (uint16_t)(UI_TEMP_Y + 7U), temp_text, UI_FG, UI_BG);
  App_DrawZHText16((uint16_t)(UI_TEMP_X + 162U), (uint16_t)(UI_TEMP_Y + 7U), TXT_DU, ARR_LEN(TXT_DU), UI_FG, UI_BG);

  if (over_temp != 0U)
  {
    App_FillRect((uint16_t)(UI_TEMP_X + 176U), (uint16_t)(UI_TEMP_Y + 6U), 38U, 20U, UI_INV_BG);
    App_DrawZHText16((uint16_t)(UI_TEMP_X + 180U), (uint16_t)(UI_TEMP_Y + 8U), TXT_OVER_TEMP, ARR_LEN(TXT_OVER_TEMP), UI_INV_FG, UI_INV_BG);
  }
  else
  {
    TGFX_DrawRect((int16_t)(UI_TEMP_X + 176U), (int16_t)(UI_TEMP_Y + 6U), 38, 20, UI_FG_DIM);
    App_DrawZHText16((uint16_t)(UI_TEMP_X + 180U), (uint16_t)(UI_TEMP_Y + 8U), TXT_OVER_TEMP, ARR_LEN(TXT_OVER_TEMP), UI_FG_DIM, UI_BG);
  }
}

static void App_DrawTabs(AppTab tab)
{
  uint16_t i;
  uint16_t x = UI_TAB_X;
  const uint16_t tab_w[APP_TAB_COUNT] = {73U, 73U, 74U};
  const uint16_t *tab_name[APP_TAB_COUNT] = {TXT_TAB_NORMAL, TXT_TAB_GUIDE, TXT_TAB_EXPERT};
  const uint8_t tab_len[APP_TAB_COUNT] = {ARR_LEN(TXT_TAB_NORMAL), ARR_LEN(TXT_TAB_GUIDE), ARR_LEN(TXT_TAB_EXPERT)};

  App_FillRect(UI_TAB_X, UI_TAB_Y, UI_TAB_W, UI_TAB_H, UI_BG);
  TGFX_DrawRect((int16_t)UI_TAB_X, (int16_t)UI_TAB_Y, (int16_t)UI_TAB_W, (int16_t)UI_TAB_H, UI_FG_DIM);

  for (i = 0U; i < (uint16_t)APP_TAB_COUNT; i++)
  {
    uint16_t fg = UI_FG_DIM;
    uint16_t bg = UI_BG;

    if (i == (uint16_t)tab)
    {
      App_FillRect(x, (uint16_t)(UI_TAB_Y + 1U), (uint16_t)(tab_w[i] - 1U), (uint16_t)(UI_TAB_H - 2U), UI_INV_BG);
      fg = UI_INV_FG;
      bg = UI_INV_BG;
    }

    App_DrawZHText16((uint16_t)(x + 20U), (uint16_t)(UI_TAB_Y + 4U), tab_name[i], tab_len[i], fg, bg);

    if (i < ((uint16_t)APP_TAB_COUNT - 1U))
    {
      TGFX_DrawLine((int16_t)(x + tab_w[i]), (int16_t)UI_TAB_Y, (int16_t)(x + tab_w[i]), (int16_t)(UI_TAB_Y + UI_TAB_H - 1U), UI_FG_DIM);
    }

    x = (uint16_t)(x + tab_w[i]);
  }
}

static void App_DrawBody(AppTab tab)
{
  App_FillRect(UI_BODY_X, UI_BODY_Y, UI_BODY_W, UI_BODY_H, UI_BG);
  TGFX_DrawRect((int16_t)UI_BODY_X, (int16_t)UI_BODY_Y, (int16_t)UI_BODY_W, (int16_t)UI_BODY_H, UI_FG_DIM);

  if (tab == APP_TAB_NORMAL)
  {
    App_DrawRows((uint16_t)(UI_BODY_Y + 2U), g_normal_rows, ARR_LEN(g_normal_rows), g_selected_row[APP_TAB_NORMAL]);
  }
  else if (tab == APP_TAB_GUIDE)
  {
    App_DrawRows((uint16_t)(UI_BODY_Y + 2U), g_guide_rows, ARR_LEN(g_guide_rows), g_selected_row[APP_TAB_GUIDE]);
  }
  else
  {
    App_DrawRows((uint16_t)(UI_BODY_Y + 2U), g_expert_rows, ARR_LEN(g_expert_rows), g_selected_row[APP_TAB_EXPERT]);
  }
}

static void App_DrawRows(uint16_t start_y, const AppRow *rows, uint8_t row_count, uint8_t selected_row)
{
  uint8_t i;
  uint16_t scrollbar_x = (uint16_t)(UI_BODY_X + UI_BODY_W - 8U);
  uint16_t scrollbar_y = start_y;
  uint16_t scrollbar_w = 6U;
  uint16_t scrollbar_h = (uint16_t)(UI_BODY_H - 4U);

  // Draw rows
  for (i = 0U; i < row_count; i++)
  {
    uint16_t y = (uint16_t)(start_y + (uint16_t)i * UI_ROW_H);
    uint16_t text_fg = UI_FG;
    uint16_t text_bg = UI_BG;

    if (i == selected_row)
    {
      App_FillRect((uint16_t)(UI_BODY_X + 2U), y, (uint16_t)(UI_BODY_W - 4U - 8U), (uint16_t)(UI_ROW_H - 2U), UI_INV_BG);
      text_fg = UI_INV_FG;
      text_bg = UI_INV_BG;
    }

    App_DrawRowLabel((uint16_t)(UI_BODY_X + 8U), (uint16_t)(y + 4U), &rows[i], text_fg, text_bg);
    App_DrawRowValue((uint16_t)(UI_BODY_X + 92U), (uint16_t)(y + 4U), &rows[i], text_fg, text_bg);
  }

  // Draw vertical scrollbar (always visible for UI fidelity)
  App_FillRect(scrollbar_x, scrollbar_y, scrollbar_w, scrollbar_h, UI_FG_DIM);
  // Draw thumb (fixed size and position for demo, can be improved for real scrolling)
  App_FillRect((uint16_t)(scrollbar_x + 1U), (uint16_t)(scrollbar_y + 8U), (uint16_t)(scrollbar_w - 2U), 32U, UI_FG);
}

static void App_DrawFooter(AppTab tab)
{
  const uint16_t *mode_text;
  uint8_t mode_len;

  if (tab == APP_TAB_NORMAL)
  {
    mode_text = TXT_TAB_NORMAL;
    mode_len = ARR_LEN(TXT_TAB_NORMAL);
  }
  else if (tab == APP_TAB_GUIDE)
  {
    mode_text = TXT_TAB_GUIDE;
    mode_len = ARR_LEN(TXT_TAB_GUIDE);
  }
  else
  {
    mode_text = TXT_TAB_EXPERT;
    mode_len = ARR_LEN(TXT_TAB_EXPERT);
  }

  App_FillRect(UI_FOOT_X, UI_FOOT_Y, UI_FOOT_W, UI_FOOT_H, UI_BG);
  TGFX_DrawRect((int16_t)UI_FOOT_X, (int16_t)UI_FOOT_Y, (int16_t)UI_FOOT_W, (int16_t)UI_FOOT_H, UI_FG_DIM);

  App_FillRect((uint16_t)(UI_FOOT_X + 4U), (uint16_t)(UI_FOOT_Y + 4U), 28U, 18U, UI_INV_BG);
  App_DrawText((uint16_t)(UI_FOOT_X + 6U), (uint16_t)(UI_FOOT_Y + 6U), "INV", UI_INV_FG, UI_INV_BG);
  App_DrawZHText16((uint16_t)(UI_FOOT_X + 38U), (uint16_t)(UI_FOOT_Y + 6U), TXT_CONFIRM, ARR_LEN(TXT_CONFIRM), UI_FG_DIM, UI_BG);
  App_DrawZHText16((uint16_t)(UI_FOOT_X + UI_FOOT_W - 38U), (uint16_t)(UI_FOOT_Y + 6U), mode_text, mode_len, UI_FG, UI_BG);
}

static void App_DrawRowLabel(uint16_t x, uint16_t y, const AppRow *row, uint16_t fg, uint16_t bg)
{
  if (row->label_ascii != NULL)
  {
    App_DrawText(x, y, row->label_ascii, fg, bg);
    return;
  }

  App_DrawZHText16(x, y, row->label_zh, row->label_zh_len, fg, bg);
}

static void App_DrawRowValue(uint16_t x, uint16_t y, const AppRow *row, uint16_t fg, uint16_t bg)
{
  uint16_t cursor_x = x;

  if (row->value_type == APP_VAL_ASCII_ONLY)
  {
    App_DrawText(cursor_x, y, row->value_ascii, fg, bg);
    return;
  }

  if (row->value_type == APP_VAL_ZH_ONLY)
  {
    App_DrawZHText16(cursor_x, y, row->value_zh, row->value_zh_len, fg, bg);
    return;
  }

  if (row->value_ascii != NULL)
  {
    App_DrawText(cursor_x, y, row->value_ascii, fg, bg);
    cursor_x = (uint16_t)(cursor_x + (uint16_t)strlen(row->value_ascii) * Font16.Width + 2U);
  }

  App_DrawZHText16(cursor_x, y, row->value_zh, row->value_zh_len, fg, bg);
}

static void App_DrawText(uint16_t x, uint16_t y, const char *text, uint16_t fg, uint16_t bg)
{
  LCD_DrawStringASCII(x, y, text, &Font16, fg, bg);
}

static void App_DrawZHText16(uint16_t x, uint16_t y, const uint16_t *codes, uint8_t count, uint16_t fg, uint16_t bg)
{
  uint8_t i;

  if (codes == NULL)
  {
    return;
  }

  for (i = 0U; i < count; i++)
  {
    LCD_DrawChinese16x16((uint16_t)(x + (uint16_t)i * 16U), y, codes[i], fg, bg);
  }
}

static void App_FillRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color)
{
  LCD_FillRect(x, y, w, h, color);
}
