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
  const uint16_t *label;
  uint8_t label_len;
  AppValueType value_type;
  const char *value_ascii;
  const uint16_t *value_zh;
  uint8_t value_zh_len;
} AppRow;

#define ARR_LEN(a)  ((uint8_t)(sizeof(a) / sizeof((a)[0])))

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

#define UI_KEY_SIZE  14U
#define UI_KEY_GAP    4U
#define UI_KEY_CX   120U
#define UI_KEY_CY   254U

#define UI_FOOT_X    10U
#define UI_FOOT_Y   282U
#define UI_FOOT_W   220U
#define UI_FOOT_H    28U

/* Common Chinese text (UCS-2 code points) */
static const uint16_t TXT_TITLE[] = {0x6E29, 0x63A7, 0x5668};                 /* U+6E29 U+63A7 U+5668 */
static const uint16_t TXT_CUR_TEMP[] = {0x5F53, 0x524D, 0x6E29, 0x5EA6};      /* U+5F53 U+524D U+6E29 U+5EA6 */
static const uint16_t TXT_DU[] = {0x5EA6};                                    /* U+5EA6 */

static const uint16_t TXT_TAB_NORMAL[] = {0x666E, 0x901A};                    /* U+666E U+901A */
static const uint16_t TXT_TAB_GUIDE[] = {0x5F15, 0x5BFC};                     /* U+5F15 U+5BFC */
static const uint16_t TXT_TAB_EXPERT[] = {0x4E13, 0x5BB6};                    /* U+4E13 U+5BB6 */

static const uint16_t TXT_LABEL_TARGET[] = {0x76EE, 0x6807};                  /* U+76EE U+6807 */
static const uint16_t TXT_LABEL_SCENE[] = {0x573A, 0x666F};                   /* U+573A U+666F */
static const uint16_t TXT_LABEL_KP[] = {0x6BD4, 0x4F8B};                      /* U+6BD4 U+4F8B */
static const uint16_t TXT_LABEL_KI[] = {0x79EF, 0x5206};                      /* U+79EF U+5206 */
static const uint16_t TXT_LABEL_KD[] = {0x5FAE, 0x5206};                      /* U+5FAE U+5206 */

static const uint16_t TXT_LABEL_STEP[] = {0x6B65, 0x9AA4};                    /* U+6B65 U+9AA4 */
static const uint16_t TXT_LABEL_METRIC[] = {0x6307, 0x6807};                  /* U+6307 U+6807 */
static const uint16_t TXT_LABEL_ISSUE[] = {0x73B0, 0x8C61};                   /* U+73B0 U+8C61 */
static const uint16_t TXT_LABEL_ACTION[] = {0x52A8, 0x4F5C};                  /* U+52A8 U+4F5C */
static const uint16_t TXT_LABEL_PARAM[] = {0x53C2, 0x6570};                   /* U+53C2 U+6570 */
static const uint16_t TXT_LABEL_RESULT[] = {0x7ED3, 0x679C};                  /* U+7ED3 U+679C */

static const uint16_t TXT_SCENE_DAILY[] = {0x65E5, 0x5E38, 0x4FDD, 0x6E29};   /* U+65E5 U+5E38 U+4FDD U+6E29 */
static const uint16_t TXT_STEP_APPROACH[] = {0x903C, 0x8FD1, 0x76EE, 0x6807}; /* U+903C U+8FD1 U+76EE U+6807 */
static const uint16_t TXT_PEAK_TEMP[] = {0x5CF0, 0x503C, 0x6E29, 0x5EA6};     /* U+5CF0 U+503C U+6E29 U+5EA6 */
static const uint16_t TXT_OVERSHOOT_BIG[] = {0x8D85, 0x8C03, 0x8F83, 0x5927};  /* U+8D85 U+8C03 U+8F83 U+5927 */
static const uint16_t TXT_APPLY_SUG[] = {0x5E94, 0x7528, 0x5EFA, 0x8BAE};      /* U+5E94 U+7528 U+5EFA U+8BAE */
static const uint16_t TXT_CUR_PARAM[] = {0x5F53, 0x524D, 0x53C2, 0x6570};      /* U+5F53 U+524D U+53C2 U+6570 */
static const uint16_t TXT_OBSERVING[] = {0x89C2, 0x5BDF, 0x4E2D};              /* U+89C2 U+5BDF U+4E2D */

static const uint16_t TXT_LABEL_LOW[] = {0x4F4E, 0x6E29, 0x9650};              /* U+4F4E U+6E29 U+9650 */
static const uint16_t TXT_LABEL_HIGH[] = {0x9AD8, 0x6E29, 0x9650};             /* U+9AD8 U+6E29 U+9650 */

static const uint16_t TXT_CONFIRM[] = {0x786E, 0x8BA4};                        /* U+786E U+8BA4 */
static const uint16_t TXT_EXECUTE[] = {0x6267, 0x884C};                        /* U+6267 U+884C */
static const uint16_t TXT_WRITE[] = {0x5199, 0x5165};                          /* U+5199 U+5165 */

static const AppRow g_normal_rows[] = {
  {TXT_LABEL_TARGET, ARR_LEN(TXT_LABEL_TARGET), APP_VAL_ASCII_ZH, "60", TXT_DU, ARR_LEN(TXT_DU)},
  {TXT_LABEL_SCENE,  ARR_LEN(TXT_LABEL_SCENE),  APP_VAL_ZH_ONLY,   NULL, TXT_SCENE_DAILY, ARR_LEN(TXT_SCENE_DAILY)},
  {TXT_LABEL_KP,     ARR_LEN(TXT_LABEL_KP),     APP_VAL_ASCII_ONLY, "8.0", NULL, 0U},
  {TXT_LABEL_KI,     ARR_LEN(TXT_LABEL_KI),     APP_VAL_ASCII_ONLY, "0.30", NULL, 0U},
  {TXT_LABEL_KD,     ARR_LEN(TXT_LABEL_KD),     APP_VAL_ASCII_ONLY, "15.0", NULL, 0U}
};

static const AppRow g_guide_rows[] = {
  {TXT_LABEL_STEP,   ARR_LEN(TXT_LABEL_STEP),   APP_VAL_ASCII_ZH,   "2/5", TXT_STEP_APPROACH, ARR_LEN(TXT_STEP_APPROACH)},
  {TXT_LABEL_METRIC, ARR_LEN(TXT_LABEL_METRIC), APP_VAL_ZH_ONLY,    NULL, TXT_PEAK_TEMP, ARR_LEN(TXT_PEAK_TEMP)},
  {TXT_LABEL_ISSUE,  ARR_LEN(TXT_LABEL_ISSUE),  APP_VAL_ZH_ONLY,    NULL, TXT_OVERSHOOT_BIG, ARR_LEN(TXT_OVERSHOOT_BIG)},
  {TXT_LABEL_ACTION, ARR_LEN(TXT_LABEL_ACTION), APP_VAL_ZH_ONLY,    NULL, TXT_APPLY_SUG, ARR_LEN(TXT_APPLY_SUG)},
  {TXT_LABEL_PARAM,  ARR_LEN(TXT_LABEL_PARAM),  APP_VAL_ZH_ONLY,    NULL, TXT_CUR_PARAM, ARR_LEN(TXT_CUR_PARAM)},
  {TXT_LABEL_RESULT, ARR_LEN(TXT_LABEL_RESULT), APP_VAL_ZH_ONLY,    NULL, TXT_OBSERVING, ARR_LEN(TXT_OBSERVING)}
};

static const AppRow g_expert_rows[] = {
  {TXT_LABEL_KP,   ARR_LEN(TXT_LABEL_KP),   APP_VAL_ASCII_ONLY, "1.6", NULL, 0U},
  {TXT_LABEL_KI,   ARR_LEN(TXT_LABEL_KI),   APP_VAL_ASCII_ONLY, "0.42", NULL, 0U},
  {TXT_LABEL_KD,   ARR_LEN(TXT_LABEL_KD),   APP_VAL_ASCII_ONLY, "0.08", NULL, 0U},
  {TXT_LABEL_LOW,  ARR_LEN(TXT_LABEL_LOW),  APP_VAL_ASCII_ZH,   "20", TXT_DU, ARR_LEN(TXT_DU)},
  {TXT_LABEL_HIGH, ARR_LEN(TXT_LABEL_HIGH), APP_VAL_ASCII_ZH,   "95", TXT_DU, ARR_LEN(TXT_DU)}
};

static const uint8_t g_selected_row[APP_TAB_COUNT] = {0U, 2U, 1U};
static uint8_t g_ui_static_drawn = 0U;

static void App_DrawPage(AppTab tab);
static void App_DrawHeader(void);
static void App_DrawTempBar(AppTab tab);
static void App_DrawTabs(AppTab tab);
static void App_DrawBody(AppTab tab);
static void App_DrawRows(uint16_t start_y, const AppRow *rows, uint8_t row_count, uint8_t selected_row);
static void App_DrawKeyPlaceholders(void);
static void App_DrawFooter(AppTab tab);
static void App_DrawText(uint16_t x, uint16_t y, const char *text, uint16_t fg, uint16_t bg);
static void App_DrawZHText16(uint16_t x, uint16_t y, const uint16_t *codes, uint8_t count, uint16_t fg, uint16_t bg);
static void App_DrawRowValue(uint16_t x, uint16_t y, const AppRow *row, uint16_t fg, uint16_t bg);
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
    App_DrawKeyPlaceholders();
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
  App_FillRect((uint16_t)(UI_TEMP_X + UI_TEMP_W - 90U), (uint16_t)(UI_TEMP_Y + 3U), 84U, 26U, UI_INV_BG);
  App_DrawText((uint16_t)(UI_TEMP_X + UI_TEMP_W - 84U), (uint16_t)(UI_TEMP_Y + 7U), temp_text, UI_INV_FG, UI_INV_BG);
  App_DrawZHText16((uint16_t)(UI_TEMP_X + UI_TEMP_W - 28U), (uint16_t)(UI_TEMP_Y + 7U), TXT_DU, ARR_LEN(TXT_DU), UI_INV_FG, UI_INV_BG);
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
      App_FillRect(x, UI_TAB_Y + 1U, (uint16_t)(tab_w[i] - 1U), (uint16_t)(UI_TAB_H - 2U), UI_INV_BG);
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

  for (i = 0U; i < row_count; i++)
  {
    uint16_t y = (uint16_t)(start_y + (uint16_t)i * UI_ROW_H);
    uint16_t text_fg = UI_FG;
    uint16_t text_bg = UI_BG;

    if (i == selected_row)
    {
      App_FillRect((uint16_t)(UI_BODY_X + 2U), y, (uint16_t)(UI_BODY_W - 4U), (uint16_t)(UI_ROW_H - 2U), UI_INV_BG);
      text_fg = UI_INV_FG;
      text_bg = UI_INV_BG;
    }

    App_DrawZHText16((uint16_t)(UI_BODY_X + 8U), (uint16_t)(y + 4U), rows[i].label, rows[i].label_len, text_fg, text_bg);
    App_DrawRowValue((uint16_t)(UI_BODY_X + 92U), (uint16_t)(y + 4U), &rows[i], text_fg, text_bg);
  }
}

static void App_DrawKeyPlaceholders(void)
{
  uint16_t up_x = (uint16_t)(UI_KEY_CX - UI_KEY_SIZE / 2U);
  uint16_t up_y = (uint16_t)(UI_KEY_CY - UI_KEY_SIZE - UI_KEY_GAP);
  uint16_t down_x = up_x;
  uint16_t down_y = (uint16_t)(UI_KEY_CY + UI_KEY_GAP);
  uint16_t left_x = (uint16_t)(up_x - UI_KEY_SIZE - UI_KEY_GAP);
  uint16_t left_y = UI_KEY_CY;
  uint16_t right_x = (uint16_t)(up_x + UI_KEY_SIZE + UI_KEY_GAP);
  uint16_t right_y = UI_KEY_CY;
  uint16_t ok_x = up_x;
  uint16_t ok_y = UI_KEY_CY;

  TGFX_DrawRect((int16_t)up_x, (int16_t)up_y, (int16_t)UI_KEY_SIZE, (int16_t)UI_KEY_SIZE, UI_FG_DIM);
  TGFX_DrawRect((int16_t)down_x, (int16_t)down_y, (int16_t)UI_KEY_SIZE, (int16_t)UI_KEY_SIZE, UI_FG_DIM);
  TGFX_DrawRect((int16_t)left_x, (int16_t)left_y, (int16_t)UI_KEY_SIZE, (int16_t)UI_KEY_SIZE, UI_FG_DIM);
  TGFX_DrawRect((int16_t)right_x, (int16_t)right_y, (int16_t)UI_KEY_SIZE, (int16_t)UI_KEY_SIZE, UI_FG_DIM);

  App_FillRect(ok_x, ok_y, UI_KEY_SIZE, UI_KEY_SIZE, UI_INV_BG);
  TGFX_DrawRect((int16_t)ok_x, (int16_t)ok_y, (int16_t)UI_KEY_SIZE, (int16_t)UI_KEY_SIZE, UI_INV_BG);
}

static void App_DrawFooter(AppTab tab)
{
  const uint16_t *hint_text;
  uint8_t hint_len;
  const uint16_t *mode_text;
  uint8_t mode_len;

  if (tab == APP_TAB_NORMAL)
  {
    hint_text = TXT_CONFIRM;
    hint_len = ARR_LEN(TXT_CONFIRM);
    mode_text = TXT_TAB_NORMAL;
    mode_len = ARR_LEN(TXT_TAB_NORMAL);
  }
  else if (tab == APP_TAB_GUIDE)
  {
    hint_text = TXT_EXECUTE;
    hint_len = ARR_LEN(TXT_EXECUTE);
    mode_text = TXT_TAB_GUIDE;
    mode_len = ARR_LEN(TXT_TAB_GUIDE);
  }
  else
  {
    hint_text = TXT_WRITE;
    hint_len = ARR_LEN(TXT_WRITE);
    mode_text = TXT_TAB_EXPERT;
    mode_len = ARR_LEN(TXT_TAB_EXPERT);
  }

  App_FillRect(UI_FOOT_X, UI_FOOT_Y, UI_FOOT_W, UI_FOOT_H, UI_BG);
  TGFX_DrawRect((int16_t)UI_FOOT_X, (int16_t)UI_FOOT_Y, (int16_t)UI_FOOT_W, (int16_t)UI_FOOT_H, UI_FG_DIM);
  App_DrawZHText16((uint16_t)(UI_FOOT_X + 4U), (uint16_t)(UI_FOOT_Y + 6U), hint_text, hint_len, UI_FG_DIM, UI_BG);
  App_DrawZHText16((uint16_t)(UI_FOOT_X + UI_FOOT_W - 38U), (uint16_t)(UI_FOOT_Y + 6U), mode_text, mode_len, UI_FG, UI_BG);
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

static void App_FillRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color)
{
  LCD_FillRect(x, y, w, h, color);
}
