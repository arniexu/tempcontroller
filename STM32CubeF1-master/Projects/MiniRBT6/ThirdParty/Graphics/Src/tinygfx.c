#include "tinygfx.h"

#include "bsp_lcd8080.h"
#include "fonts.h"

static uint16_t g_tgfx_w = LCD_WIDTH;
static uint16_t g_tgfx_h = LCD_HEIGHT;

static int16_t TGFX_Abs(int16_t v)
{
  return (v < 0) ? (int16_t)(-v) : v;
}

void TGFX_Init(uint16_t width, uint16_t height)
{
  g_tgfx_w = width;
  g_tgfx_h = height;
}

void TGFX_Clear(uint16_t color)
{
  LCD_FillScreen(color);
}

void TGFX_DrawPixel(int16_t x, int16_t y, uint16_t color)
{
  if ((x < 0) || (y < 0) || (x >= (int16_t)g_tgfx_w) || (y >= (int16_t)g_tgfx_h))
  {
    return;
  }

  LCD_DrawPixel((uint16_t)x, (uint16_t)y, color);
}

void TGFX_DrawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color)
{
  int16_t dx = TGFX_Abs((int16_t)(x1 - x0));
  int16_t sx = (x0 < x1) ? 1 : -1;
  int16_t dy = (int16_t)-TGFX_Abs((int16_t)(y1 - y0));
  int16_t sy = (y0 < y1) ? 1 : -1;
  int16_t err = (int16_t)(dx + dy);

  while (1)
  {
    TGFX_DrawPixel(x0, y0, color);
    if ((x0 == x1) && (y0 == y1))
    {
      break;
    }

    {
      int16_t e2 = (int16_t)(2 * err);
      if (e2 >= dy)
      {
        err = (int16_t)(err + dy);
        x0 = (int16_t)(x0 + sx);
      }
      if (e2 <= dx)
      {
        err = (int16_t)(err + dx);
        y0 = (int16_t)(y0 + sy);
      }
    }
  }
}

void TGFX_DrawRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color)
{
  if ((w <= 0) || (h <= 0))
  {
    return;
  }

  TGFX_DrawLine(x, y, (int16_t)(x + w - 1), y, color);
  TGFX_DrawLine(x, (int16_t)(y + h - 1), (int16_t)(x + w - 1), (int16_t)(y + h - 1), color);
  TGFX_DrawLine(x, y, x, (int16_t)(y + h - 1), color);
  TGFX_DrawLine((int16_t)(x + w - 1), y, (int16_t)(x + w - 1), (int16_t)(y + h - 1), color);
}

void TGFX_FillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color)
{
  if ((w <= 0) || (h <= 0))
  {
    return;
  }

  if ((x >= (int16_t)g_tgfx_w) || (y >= (int16_t)g_tgfx_h))
  {
    return;
  }

  if (x < 0)
  {
    w = (int16_t)(w + x);
    x = 0;
  }
  if (y < 0)
  {
    h = (int16_t)(h + y);
    y = 0;
  }

  if ((x + w) > (int16_t)g_tgfx_w)
  {
    w = (int16_t)(g_tgfx_w - (uint16_t)x);
  }
  if ((y + h) > (int16_t)g_tgfx_h)
  {
    h = (int16_t)(g_tgfx_h - (uint16_t)y);
  }

  if ((w <= 0) || (h <= 0))
  {
    return;
  }

  LCD_FillRect((uint16_t)x, (uint16_t)y, (uint16_t)w, (uint16_t)h, color);
}

void TGFX_DrawCircle(int16_t x0, int16_t y0, int16_t r, uint16_t color)
{
  int16_t x = r;
  int16_t y = 0;
  int16_t err = 0;

  if (r <= 0)
  {
    return;
  }

  while (x >= y)
  {
    TGFX_DrawPixel((int16_t)(x0 + x), (int16_t)(y0 + y), color);
    TGFX_DrawPixel((int16_t)(x0 + y), (int16_t)(y0 + x), color);
    TGFX_DrawPixel((int16_t)(x0 - y), (int16_t)(y0 + x), color);
    TGFX_DrawPixel((int16_t)(x0 - x), (int16_t)(y0 + y), color);
    TGFX_DrawPixel((int16_t)(x0 - x), (int16_t)(y0 - y), color);
    TGFX_DrawPixel((int16_t)(x0 - y), (int16_t)(y0 - x), color);
    TGFX_DrawPixel((int16_t)(x0 + y), (int16_t)(y0 - x), color);
    TGFX_DrawPixel((int16_t)(x0 + x), (int16_t)(y0 - y), color);

    y = (int16_t)(y + 1);
    if (err <= 0)
    {
      err = (int16_t)(err + (2 * y) + 1);
    }
    if (err > 0)
    {
      x = (int16_t)(x - 1);
      err = (int16_t)(err - (2 * x) - 1);
    }
  }
}

void TGFX_DrawString(int16_t x, int16_t y, const char *text, uint16_t fg, uint16_t bg)
{
  if ((x < 0) || (y < 0) || (text == 0))
  {
    return;
  }

  LCD_DrawStringASCII((uint16_t)x, (uint16_t)y, text, &Font16, fg, bg);
}
