#include "app_demo.h"

#include "lcd8080.h"
#include "tinygfx.h"

static void App_DelayMs(unsigned long delay_ms);
static void App_RunFontDemo(void);
static void App_DrawChineseText16(unsigned short x, unsigned short y, const unsigned short *codes, unsigned short count, unsigned short fg, unsigned short bg);

void App_Demo_Init(void)
{
  LCD_InitWithProfile(0U);
  LCD_SetBacklightRaw(1U);
  LCD_SetPixelWriteMode(LCD_PIXEL_MODE_16BIT_NORMAL);
  TGFX_Init(LCD_WIDTH, LCD_HEIGHT);
}

void App_Demo_Run(void)
{
  App_RunFontDemo();
}

static void App_RunFontDemo(void)
{
  static const unsigned short text_utf8_mix[] = {0x4E2D, 0x6587, 0x6DF7, 0x6392};
  static const unsigned short text_controller[] = {0x6E29, 0x63A7, 0x5668};
  static const unsigned short text_graphics[] = {0x56FE, 0x5F62, 0x5E93};
  static const unsigned short text_english_cn[] = {0x4E2D, 0x6587};
  static const unsigned short text_display_ok[] = {0x663E, 0x793A, 0x6B63, 0x5E38};
  static const unsigned short text_font_page[] = {0x4E2D, 0x82F1, 0x6587, 0x5B57, 0x5E93, 0x9875, 0x9762};
  static const unsigned short text_dir_check[] = {0x5B57, 0x4F53, 0x65B9, 0x5411, 0x6821, 0x9A8C};
  static const unsigned short text_done[] = {0x5B8C, 0x6210};

  LCD_FillScreen(LCD_COLOR_BLACK);
  LCD_DrawStringASCII(8U, 8U, "Chinese Font Test", &Font16, LCD_COLOR_GREEN, LCD_COLOR_BLACK);
  LCD_DrawStringASCII(8U, 32U, "ASCII + UTF8 mixed", &Font16, LCD_COLOR_WHITE, LCD_COLOR_BLACK);

  LCD_DrawChinese16x16(8U, 64U, 0x4E2DU, LCD_COLOR_YELLOW, LCD_COLOR_BLACK);
  LCD_DrawChinese16x16(24U, 64U, 0x6587U, LCD_COLOR_YELLOW, LCD_COLOR_BLACK);
  LCD_DrawChinese16x16(40U, 64U, 0x5B57U, LCD_COLOR_YELLOW, LCD_COLOR_BLACK);
  LCD_DrawChinese16x16(56U, 64U, 0x5E93U, LCD_COLOR_YELLOW, LCD_COLOR_BLACK);

  LCD_DrawChinese16x16(8U, 96U, 0x6ED1U, LCD_COLOR_CYAN, LCD_COLOR_BLACK);
  LCD_DrawChinese16x16(24U, 96U, 0x5757U, LCD_COLOR_CYAN, LCD_COLOR_BLACK);
  LCD_DrawChinese16x16(40U, 96U, 0x6D4BU, LCD_COLOR_CYAN, LCD_COLOR_BLACK);
  LCD_DrawChinese16x16(56U, 96U, 0x8BD5U, LCD_COLOR_CYAN, LCD_COLOR_BLACK);

  App_DrawChineseText16(8U, 136U, text_utf8_mix, 4U, LCD_COLOR_WHITE, LCD_COLOR_BLACK);
  LCD_DrawStringASCII(72U, 136U, "UTF8", &Font16, LCD_COLOR_WHITE, LCD_COLOR_BLACK);
  App_DrawChineseText16(8U, 168U, text_controller, 3U, LCD_COLOR_GREEN, LCD_COLOR_BLACK);
  LCD_DrawStringASCII(56U, 168U, " LCD ", &Font16, LCD_COLOR_GREEN, LCD_COLOR_BLACK);
  App_DrawChineseText16(120U, 168U, text_graphics, 3U, LCD_COLOR_GREEN, LCD_COLOR_BLACK);
  LCD_DrawStringASCII(8U, 200U, "English + ", &Font16, LCD_COLOR_RED, LCD_COLOR_BLACK);
  App_DrawChineseText16(104U, 200U, text_english_cn, 2U, LCD_COLOR_RED, LCD_COLOR_BLACK);
  App_DrawChineseText16(8U, 232U, text_display_ok, 4U, LCD_COLOR_YELLOW, LCD_COLOR_BLACK);
  App_DelayMs(1800U);

  LCD_FillScreen(LCD_COLOR_WHITE);
  App_DrawChineseText16(8U, 16U, text_font_page, 7U, LCD_COLOR_BLUE, LCD_COLOR_WHITE);
  App_DrawChineseText16(8U, 56U, text_dir_check, 6U, LCD_COLOR_BLACK, LCD_COLOR_WHITE);
  LCD_DrawStringUTF8(8U, 96U, "0123456789 ABCD", LCD_COLOR_RED, LCD_COLOR_WHITE);
  App_DrawChineseText16(8U, 136U, text_done, 2U, LCD_COLOR_GREEN, LCD_COLOR_WHITE);
  App_DelayMs(1800U);
}

static void App_DrawChineseText16(unsigned short x, unsigned short y, const unsigned short *codes, unsigned short count, unsigned short fg, unsigned short bg)
{
  unsigned short i;

  if (codes == 0)
  {
    return;
  }

  for (i = 0U; i < count; i++)
  {
    LCD_DrawChinese16x16((unsigned short)(x + i * 16U), y, codes[i], fg, bg);
  }
}

static void App_DelayMs(unsigned long delay_ms)
{
  unsigned long start_tick = HAL_GetTick();

  while ((HAL_GetTick() - start_tick) < delay_ms)
  {
  }
}
