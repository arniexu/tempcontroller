#include "lcd8080.h"
#include "lcd_font_zh.h"

#include <stddef.h>

static uint8_t g_lcd_is_ili9341 = 0U;

static void LCD_DelayMs(uint32_t ms)
{
  uint32_t start = HAL_GetTick();
  while ((HAL_GetTick() - start) < ms)
  {
  }
}

static void LCD_WR_Strobe(void)
{
  LCD_CTRL_GPIO_Port->BRR = LCD_WR_Pin;
  LCD_CTRL_GPIO_Port->BSRR = LCD_WR_Pin;
}

static void LCD_SetData(uint16_t data)
{
  LCD_DATA_GPIO_Port->ODR = data;
}

static void LCD_DataBusToOutput(void)
{
  LCD_DATA_GPIO_Port->CRL = 0x33333333U;
  LCD_DATA_GPIO_Port->CRH = 0x33333333U;
}

static void LCD_DataBusToInput(void)
{
  LCD_DATA_GPIO_Port->CRL = 0x44444444U;
  LCD_DATA_GPIO_Port->CRH = 0x44444444U;
}

static void LCD_WriteCmd(uint16_t cmd)
{
  LCD_CTRL_GPIO_Port->BRR = LCD_CS_Pin;
  LCD_CTRL_GPIO_Port->BRR = LCD_RS_Pin;
  LCD_SetData(cmd);
  LCD_WR_Strobe();
  LCD_CTRL_GPIO_Port->BSRR = LCD_CS_Pin;
}

static void LCD_WriteData(uint16_t data)
{
  LCD_CTRL_GPIO_Port->BRR = LCD_CS_Pin;
  LCD_CTRL_GPIO_Port->BSRR = LCD_RS_Pin;
  LCD_SetData(data);
  LCD_WR_Strobe();
  LCD_CTRL_GPIO_Port->BSRR = LCD_CS_Pin;
}

static void LCD_WriteData8(uint8_t data)
{
  LCD_WriteData((uint16_t)data);
}

static void LCD_WriteData16(uint16_t data)
{
  LCD_WriteData(data);
}

static uint16_t LCD_ReadData16(void)
{
  uint16_t data;

  LCD_CTRL_GPIO_Port->BRR = LCD_CS_Pin;
  LCD_CTRL_GPIO_Port->BSRR = LCD_RS_Pin;
  LCD_DataBusToInput();

  LCD_CTRL_GPIO_Port->BRR = LCD_RD_Pin;
  data = (uint16_t)LCD_DATA_GPIO_Port->IDR;
  LCD_CTRL_GPIO_Port->BSRR = LCD_RD_Pin;

  LCD_DataBusToOutput();
  LCD_CTRL_GPIO_Port->BSRR = LCD_CS_Pin;

  return data;
}

static uint16_t LCD_ReadReg16(uint16_t reg)
{
  LCD_WriteCmd(reg);
  return LCD_ReadData16();
}

static uint32_t LCD_ReadID9341(void)
{
  uint8_t b1;
  uint8_t b2;
  uint8_t b3;
  uint8_t b4;

  LCD_WriteCmd(0xD3U);
  (void)LCD_ReadData16();
  b1 = (uint8_t)(LCD_ReadData16() & 0x00FFU);
  b2 = (uint8_t)(LCD_ReadData16() & 0x00FFU);
  b3 = (uint8_t)(LCD_ReadData16() & 0x00FFU);
  b4 = (uint8_t)(LCD_ReadData16() & 0x00FFU);

  return ((uint32_t)b1 << 24) | ((uint32_t)b2 << 16) | ((uint32_t)b3 << 8) | (uint32_t)b4;
}

static void LCD_SetWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1)
{
  if (g_lcd_is_ili9341 != 0U)
  {
    LCD_WriteCmd(0x2AU);
    LCD_WriteData8((uint8_t)(x0 >> 8));
    LCD_WriteData8((uint8_t)(x0 & 0x00FFU));
    LCD_WriteData8((uint8_t)(x1 >> 8));
    LCD_WriteData8((uint8_t)(x1 & 0x00FFU));

    LCD_WriteCmd(0x2BU);
    LCD_WriteData8((uint8_t)(y0 >> 8));
    LCD_WriteData8((uint8_t)(y0 & 0x00FFU));
    LCD_WriteData8((uint8_t)(y1 >> 8));
    LCD_WriteData8((uint8_t)(y1 & 0x00FFU));

    LCD_WriteCmd(0x2CU);
  }
  else
  {
#if (LCD_932X_MIRROR_X == 1U)
    uint16_t sx0 = (uint16_t)((LCD_WIDTH - 1U) - x1);
    uint16_t sx1 = (uint16_t)((LCD_WIDTH - 1U) - x0);
#else
    uint16_t sx0 = x0;
    uint16_t sx1 = x1;
#endif
    LCD_WriteCmd(0x50U);
    LCD_WriteData16(sx0);
    LCD_WriteCmd(0x51U);
    LCD_WriteData16(sx1);
    LCD_WriteCmd(0x52U);
    LCD_WriteData16(y0);
    LCD_WriteCmd(0x53U);
    LCD_WriteData16(y1);
    LCD_WriteCmd(0x20U);
    LCD_WriteData16(sx0);
    LCD_WriteCmd(0x21U);
    LCD_WriteData16(y0);
    LCD_WriteCmd(0x22U);
  }
}

static void LCD_GPIO_Init(void)
{
  GPIO_InitTypeDef gpio = {0};

  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();

  gpio.Pin = GPIO_PIN_All;
  gpio.Mode = GPIO_MODE_OUTPUT_PP;
  gpio.Pull = GPIO_NOPULL;
  gpio.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(LCD_DATA_GPIO_Port, &gpio);

  gpio.Pin = LCD_BL_Pin | LCD_CS_Pin | LCD_RS_Pin | LCD_WR_Pin | LCD_RD_Pin;
  HAL_GPIO_Init(LCD_CTRL_GPIO_Port, &gpio);

  LCD_DataBusToOutput();

  LCD_CTRL_GPIO_Port->BSRR = LCD_CS_Pin | LCD_RS_Pin | LCD_WR_Pin | LCD_RD_Pin;
#if (LCD_BL_ACTIVE_HIGH == 1U)
  LCD_CTRL_GPIO_Port->BSRR = LCD_BL_Pin;
#else
  LCD_CTRL_GPIO_Port->BRR = LCD_BL_Pin;
#endif
}

static void LCD_Init_ILI9341(void)
{
  LCD_DelayMs(20U);

  LCD_WriteCmd(0x01U);
  LCD_DelayMs(120U);

  LCD_WriteCmd(0x28U);

  LCD_WriteCmd(0xCFU);
  LCD_WriteData8(0x00U);
  LCD_WriteData8(0x83U);
  LCD_WriteData8(0x30U);

  LCD_WriteCmd(0xEDU);
  LCD_WriteData8(0x64U);
  LCD_WriteData8(0x03U);
  LCD_WriteData8(0x12U);
  LCD_WriteData8(0x81U);

  LCD_WriteCmd(0xE8U);
  LCD_WriteData8(0x85U);
  LCD_WriteData8(0x01U);
  LCD_WriteData8(0x79U);

  LCD_WriteCmd(0xCBU);
  LCD_WriteData8(0x39U);
  LCD_WriteData8(0x2CU);
  LCD_WriteData8(0x00U);
  LCD_WriteData8(0x34U);
  LCD_WriteData8(0x02U);

  LCD_WriteCmd(0xF7U);
  LCD_WriteData8(0x20U);

  LCD_WriteCmd(0xEAU);
  LCD_WriteData8(0x00U);
  LCD_WriteData8(0x00U);

  LCD_WriteCmd(0xC0U);
  LCD_WriteData8(0x26U);

  LCD_WriteCmd(0xC1U);
  LCD_WriteData8(0x11U);

  LCD_WriteCmd(0xC5U);
  LCD_WriteData8(0x35U);
  LCD_WriteData8(0x3EU);

  LCD_WriteCmd(0xC7U);
  LCD_WriteData8(0xBEU);

  LCD_WriteCmd(0x36U);
  LCD_WriteData8(0x48U);

  LCD_WriteCmd(0x3AU);
  LCD_WriteData8(0x55U);

  LCD_WriteCmd(0xB1U);
  LCD_WriteData8(0x00U);
  LCD_WriteData8(0x1BU);

  LCD_WriteCmd(0xB6U);
  LCD_WriteData8(0x0AU);
  LCD_WriteData8(0xA2U);

  LCD_WriteCmd(0xF2U);
  LCD_WriteData8(0x00U);

  LCD_WriteCmd(0x26U);
  LCD_WriteData8(0x01U);

  LCD_WriteCmd(0xE0U);
  LCD_WriteData8(0x1FU);
  LCD_WriteData8(0x1AU);
  LCD_WriteData8(0x18U);
  LCD_WriteData8(0x0AU);
  LCD_WriteData8(0x0FU);
  LCD_WriteData8(0x06U);
  LCD_WriteData8(0x45U);
  LCD_WriteData8(0x87U);
  LCD_WriteData8(0x32U);
  LCD_WriteData8(0x0AU);
  LCD_WriteData8(0x07U);
  LCD_WriteData8(0x02U);
  LCD_WriteData8(0x07U);
  LCD_WriteData8(0x05U);
  LCD_WriteData8(0x00U);

  LCD_WriteCmd(0xE1U);
  LCD_WriteData8(0x00U);
  LCD_WriteData8(0x25U);
  LCD_WriteData8(0x27U);
  LCD_WriteData8(0x05U);
  LCD_WriteData8(0x10U);
  LCD_WriteData8(0x09U);
  LCD_WriteData8(0x3AU);
  LCD_WriteData8(0x78U);
  LCD_WriteData8(0x4DU);
  LCD_WriteData8(0x05U);
  LCD_WriteData8(0x18U);
  LCD_WriteData8(0x0DU);
  LCD_WriteData8(0x38U);
  LCD_WriteData8(0x3AU);
  LCD_WriteData8(0x1FU);

  LCD_WriteCmd(0x11U);
  LCD_DelayMs(120U);
  LCD_WriteCmd(0x29U);
  LCD_DelayMs(20U);
}

static void LCD_Init_ILI9325(void)
{
  LCD_DelayMs(50U);

  LCD_WriteCmd(0x00E5U); LCD_WriteData16(0x8000U);
  LCD_WriteCmd(0x0000U); LCD_WriteData16(0x0001U);
  LCD_WriteCmd(0x0001U); LCD_WriteData16(0x0100U);
  LCD_WriteCmd(0x0002U); LCD_WriteData16(0x0700U);
  LCD_WriteCmd(0x0003U); LCD_WriteData16(LCD_932X_ENTRY_MODE);
  LCD_WriteCmd(0x0004U); LCD_WriteData16(0x0000U);
  LCD_WriteCmd(0x0008U); LCD_WriteData16(0x0202U);
  LCD_WriteCmd(0x0009U); LCD_WriteData16(0x0000U);
  LCD_WriteCmd(0x000AU); LCD_WriteData16(0x0000U);
  LCD_WriteCmd(0x000CU); LCD_WriteData16(0x0000U);
  LCD_WriteCmd(0x000DU); LCD_WriteData16(0x0000U);
  LCD_WriteCmd(0x000FU); LCD_WriteData16(0x0000U);
  LCD_WriteCmd(0x0010U); LCD_WriteData16(0x0000U);
  LCD_WriteCmd(0x0011U); LCD_WriteData16(0x0000U);
  LCD_WriteCmd(0x0012U); LCD_WriteData16(0x0000U);
  LCD_WriteCmd(0x0013U); LCD_WriteData16(0x0000U);
  LCD_DelayMs(20U);

  LCD_WriteCmd(0x0010U); LCD_WriteData16(0x17B0U);
  LCD_WriteCmd(0x0011U); LCD_WriteData16(0x0137U);
  LCD_DelayMs(5U);
  LCD_WriteCmd(0x0012U); LCD_WriteData16(0x0139U);
  LCD_DelayMs(5U);
  LCD_WriteCmd(0x0013U); LCD_WriteData16(0x1D00U);
  LCD_WriteCmd(0x0029U); LCD_WriteData16(0x0013U);
  LCD_DelayMs(5U);

  LCD_WriteCmd(0x0020U); LCD_WriteData16(0x0000U);
  LCD_WriteCmd(0x0021U); LCD_WriteData16(0x0000U);

  LCD_WriteCmd(0x0030U); LCD_WriteData16(0x0006U);
  LCD_WriteCmd(0x0031U); LCD_WriteData16(0x0101U);
  LCD_WriteCmd(0x0032U); LCD_WriteData16(0x0003U);
  LCD_WriteCmd(0x0035U); LCD_WriteData16(0x0106U);
  LCD_WriteCmd(0x0036U); LCD_WriteData16(0x0B02U);
  LCD_WriteCmd(0x0037U); LCD_WriteData16(0x0302U);
  LCD_WriteCmd(0x0038U); LCD_WriteData16(0x0707U);
  LCD_WriteCmd(0x0039U); LCD_WriteData16(0x0007U);
  LCD_WriteCmd(0x003CU); LCD_WriteData16(0x0600U);
  LCD_WriteCmd(0x003DU); LCD_WriteData16(0x020BU);

  LCD_WriteCmd(0x0050U); LCD_WriteData16(0x0000U);
  LCD_WriteCmd(0x0051U); LCD_WriteData16(0x00EFU);
  LCD_WriteCmd(0x0052U); LCD_WriteData16(0x0000U);
  LCD_WriteCmd(0x0053U); LCD_WriteData16(0x013FU);
  LCD_WriteCmd(0x0060U); LCD_WriteData16(0xA700U);
  LCD_WriteCmd(0x0061U); LCD_WriteData16(0x0001U);
  LCD_WriteCmd(0x006AU); LCD_WriteData16(0x0000U);
  LCD_WriteCmd(0x0090U); LCD_WriteData16(0x0010U);
  LCD_WriteCmd(0x0092U); LCD_WriteData16(0x0000U);
  LCD_WriteCmd(0x0093U); LCD_WriteData16(0x0003U);
  LCD_WriteCmd(0x0095U); LCD_WriteData16(0x0110U);
  LCD_WriteCmd(0x0097U); LCD_WriteData16(0x0000U);
  LCD_WriteCmd(0x0098U); LCD_WriteData16(0x0000U);
  LCD_WriteCmd(0x0003U); LCD_WriteData16(LCD_932X_ENTRY_MODE);

  LCD_WriteCmd(0x0007U); LCD_WriteData16(0x0173U);
}

static void LCD_Init_R61509_5408(void)
{
  LCD_DelayMs(50U);

  LCD_WriteCmd(0x0001U); LCD_WriteData16(0x0100U);
  LCD_WriteCmd(0x0002U); LCD_WriteData16(0x0700U);
  LCD_WriteCmd(0x0003U); LCD_WriteData16(LCD_932X_ENTRY_MODE);
  LCD_WriteCmd(0x0004U); LCD_WriteData16(0x0000U);
  LCD_WriteCmd(0x0008U); LCD_WriteData16(0x0202U);
  LCD_WriteCmd(0x0009U); LCD_WriteData16(0x0000U);
  LCD_WriteCmd(0x000AU); LCD_WriteData16(0x0000U);
  LCD_WriteCmd(0x000CU); LCD_WriteData16(0x0000U);
  LCD_WriteCmd(0x000DU); LCD_WriteData16(0x0000U);
  LCD_WriteCmd(0x000FU); LCD_WriteData16(0x0000U);
  LCD_WriteCmd(0x0010U); LCD_WriteData16(0x0000U);
  LCD_WriteCmd(0x0011U); LCD_WriteData16(0x0000U);
  LCD_WriteCmd(0x0012U); LCD_WriteData16(0x0000U);
  LCD_WriteCmd(0x0013U); LCD_WriteData16(0x0000U);
  LCD_DelayMs(20U);

  LCD_WriteCmd(0x0011U); LCD_WriteData16(0x0007U);
  LCD_DelayMs(5U);
  LCD_WriteCmd(0x0010U); LCD_WriteData16(0x12B0U);
  LCD_DelayMs(5U);
  LCD_WriteCmd(0x0012U); LCD_WriteData16(0x01BDU);
  LCD_DelayMs(5U);
  LCD_WriteCmd(0x0013U); LCD_WriteData16(0x1400U);
  LCD_WriteCmd(0x0029U); LCD_WriteData16(0x000EU);
  LCD_DelayMs(5U);

  LCD_WriteCmd(0x0020U); LCD_WriteData16(0x0000U);
  LCD_WriteCmd(0x0021U); LCD_WriteData16(0x013FU);

  LCD_WriteCmd(0x0030U); LCD_WriteData16(0x0B0DU);
  LCD_WriteCmd(0x0031U); LCD_WriteData16(0x1923U);
  LCD_WriteCmd(0x0032U); LCD_WriteData16(0x1C26U);
  LCD_WriteCmd(0x0035U); LCD_WriteData16(0x0D0BU);
  LCD_WriteCmd(0x0036U); LCD_WriteData16(0x1006U);
  LCD_WriteCmd(0x0037U); LCD_WriteData16(0x0610U);
  LCD_WriteCmd(0x0038U); LCD_WriteData16(0x0706U);
  LCD_WriteCmd(0x0039U); LCD_WriteData16(0x0304U);
  LCD_WriteCmd(0x003AU); LCD_WriteData16(0x0E05U);
  LCD_WriteCmd(0x003BU); LCD_WriteData16(0x0E01U);
  LCD_WriteCmd(0x003CU); LCD_WriteData16(0x010EU);
  LCD_WriteCmd(0x003DU); LCD_WriteData16(0x050EU);
  LCD_WriteCmd(0x003EU); LCD_WriteData16(0x0403U);
  LCD_WriteCmd(0x003FU); LCD_WriteData16(0x0607U);

  LCD_WriteCmd(0x0050U); LCD_WriteData16(0x0000U);
  LCD_WriteCmd(0x0051U); LCD_WriteData16(0x00EFU);
  LCD_WriteCmd(0x0052U); LCD_WriteData16(0x0000U);
  LCD_WriteCmd(0x0053U); LCD_WriteData16(0x013FU);
  LCD_WriteCmd(0x0060U); LCD_WriteData16(0xA700U);
  LCD_WriteCmd(0x0061U); LCD_WriteData16(0x0001U);
  LCD_WriteCmd(0x006AU); LCD_WriteData16(0x0000U);
  LCD_WriteCmd(0x0080U); LCD_WriteData16(0x0000U);
  LCD_WriteCmd(0x0081U); LCD_WriteData16(0x0000U);
  LCD_WriteCmd(0x0082U); LCD_WriteData16(0x0000U);
  LCD_WriteCmd(0x0083U); LCD_WriteData16(0x0000U);
  LCD_WriteCmd(0x0084U); LCD_WriteData16(0x0000U);
  LCD_WriteCmd(0x0085U); LCD_WriteData16(0x0000U);
  LCD_WriteCmd(0x0090U); LCD_WriteData16(0x0010U);
  LCD_WriteCmd(0x0092U); LCD_WriteData16(0x0000U);
  LCD_WriteCmd(0x0093U); LCD_WriteData16(0x0003U);
  LCD_WriteCmd(0x0095U); LCD_WriteData16(0x0110U);
  LCD_WriteCmd(0x0097U); LCD_WriteData16(0x0000U);
  LCD_WriteCmd(0x0098U); LCD_WriteData16(0x0000U);
  LCD_WriteCmd(0x0003U); LCD_WriteData16(LCD_932X_ENTRY_MODE);
  LCD_WriteCmd(0x0007U); LCD_WriteData16(0x0112U);
}

static void LCD_Controller_Init(void)
{
  uint16_t id16 = LCD_ReadReg16(0x0000U);
  uint32_t id9341 = LCD_ReadID9341();

  if ((id9341 & 0x0000FFFFUL) == 0x9341UL)
  {
    g_lcd_is_ili9341 = 1U;
    LCD_Init_ILI9341();
    return;
  }

  if ((id16 == 0x9325U) || (id16 == 0x9328U) || (id16 == 0x5408U) || (id16 == 0x8989U))
  {
    g_lcd_is_ili9341 = 0U;
    if (id16 == 0x5408U)
    {
      LCD_Init_R61509_5408();
    }
    else
    {
      LCD_Init_ILI9325();
    }
    return;
  }

  g_lcd_is_ili9341 = 0U;
  LCD_Init_ILI9325();
}

void LCD_Init(void)
{
  LCD_GPIO_Init();
  LCD_Controller_Init();
  LCD_FillScreen(LCD_COLOR_BLACK);
}

void LCD_DrawPixel(uint16_t x, uint16_t y, uint16_t color)
{
  if ((x >= LCD_WIDTH) || (y >= LCD_HEIGHT))
  {
    return;
  }

  LCD_SetWindow(x, y, x, y);
  LCD_WriteData(color);
}

void LCD_FillRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color)
{
  uint32_t i;
  uint32_t count;

  if ((x >= LCD_WIDTH) || (y >= LCD_HEIGHT) || (w == 0U) || (h == 0U))
  {
    return;
  }

  if ((x + w) > LCD_WIDTH)
  {
    w = LCD_WIDTH - x;
  }
  if ((y + h) > LCD_HEIGHT)
  {
    h = LCD_HEIGHT - y;
  }

  LCD_SetWindow(x, y, (uint16_t)(x + w - 1U), (uint16_t)(y + h - 1U));

  count = (uint32_t)w * (uint32_t)h;
  for (i = 0U; i < count; i++)
  {
    LCD_WriteData(color);
  }
}

void LCD_FillScreen(uint16_t color)
{
  LCD_FillRect(0U, 0U, LCD_WIDTH, LCD_HEIGHT, color);
}

void LCD_DrawCharASCII(uint16_t x, uint16_t y, char ch, const sFONT *font, uint16_t fg, uint16_t bg)
{
  uint16_t row;
  uint16_t col;
  uint16_t bytes_per_row;
  const uint8_t *glyph;

  if ((font == NULL) || (ch < 32) || (ch > 126))
  {
    return;
  }

  bytes_per_row = (uint16_t)((font->Width + 7U) / 8U);
  glyph = &font->table[(uint32_t)(ch - 32) * font->Height * bytes_per_row];

  for (row = 0U; row < font->Height; row++)
  {
    for (col = 0U; col < font->Width; col++)
    {
      uint8_t b = glyph[row * bytes_per_row + (col / 8U)];
      uint16_t mask = (uint16_t)(0x80U >> (col % 8U));
      LCD_DrawPixel((uint16_t)(x + col), (uint16_t)(y + row), ((b & mask) != 0U) ? fg : bg);
    }
  }
}

void LCD_DrawStringASCII(uint16_t x, uint16_t y, const char *str, const sFONT *font, uint16_t fg, uint16_t bg)
{
  uint16_t cursor_x = x;

  if ((str == NULL) || (font == NULL))
  {
    return;
  }

  while (*str != '\0')
  {
    LCD_DrawCharASCII(cursor_x, y, *str, font, fg, bg);
    cursor_x = (uint16_t)(cursor_x + font->Width);
    str++;
  }
}

void LCD_DrawChinese16x16(uint16_t x, uint16_t y, uint16_t unicode, uint16_t fg, uint16_t bg)
{
  const uint8_t *glyph = LCD_FontZH_Get16(unicode);
  uint16_t row;
  uint16_t col;

  if (glyph == NULL)
  {
    LCD_FillRect(x, y, 16U, 16U, bg);
    LCD_FillRect((uint16_t)(x + 1U), (uint16_t)(y + 1U), 14U, 14U, fg);
    LCD_FillRect((uint16_t)(x + 2U), (uint16_t)(y + 2U), 12U, 12U, bg);
    return;
  }

  for (row = 0U; row < 16U; row++)
  {
    for (col = 0U; col < 16U; col++)
    {
      uint8_t b = glyph[row * 2U + (col / 8U)];
      uint8_t mask = (uint8_t)(0x80U >> (col % 8U));
      LCD_DrawPixel((uint16_t)(x + col), (uint16_t)(y + row), ((b & mask) != 0U) ? fg : bg);
    }
  }
}

static uint16_t LCD_DecodeUtf8(const char **s)
{
  const uint8_t *p = (const uint8_t *)(*s);
  uint16_t code = 0U;

  if (p[0] < 0x80U)
  {
    code = p[0];
    (*s) += 1;
  }
  else if ((p[0] & 0xF0U) == 0xE0U)
  {
    code = (uint16_t)(((p[0] & 0x0FU) << 12) | ((p[1] & 0x3FU) << 6) | (p[2] & 0x3FU));
    (*s) += 3;
  }
  else
  {
    code = '?';
    (*s) += 1;
  }

  return code;
}

void LCD_DrawStringUTF8(uint16_t x, uint16_t y, const char *utf8, uint16_t fg, uint16_t bg)
{
  uint16_t cursor_x = x;

  if (utf8 == NULL)
  {
    return;
  }

  while (*utf8 != '\0')
  {
    uint16_t code = LCD_DecodeUtf8(&utf8);
    if (code < 0x80U)
    {
      LCD_DrawCharASCII(cursor_x, y, (char)code, &Font16, fg, bg);
      cursor_x = (uint16_t)(cursor_x + Font16.Width);
    }
    else
    {
      LCD_DrawChinese16x16(cursor_x, y, code, fg, bg);
      cursor_x = (uint16_t)(cursor_x + 16U);
    }
  }
}

void LCD_TestSolidColors(void)
{
  static const uint16_t colors[] = {
    LCD_COLOR_RED, LCD_COLOR_GREEN, LCD_COLOR_BLUE,
    LCD_COLOR_WHITE, LCD_COLOR_BLACK, LCD_COLOR_YELLOW,
    LCD_COLOR_CYAN
  };
  uint32_t i;

  for (i = 0U; i < (sizeof(colors) / sizeof(colors[0])); i++)
  {
    LCD_FillScreen(colors[i]);
    LCD_DelayMs(250U);
  }
}

void LCD_TestSlider(void)
{
  uint16_t x;

  LCD_FillScreen(LCD_COLOR_BLACK);
  LCD_DrawStringASCII(8U, 8U, "Slider Test", &Font16, LCD_COLOR_WHITE, LCD_COLOR_BLACK);
  LCD_DrawChinese16x16(8U, 32U, 0x6ED1U, LCD_COLOR_YELLOW, LCD_COLOR_BLACK);
  LCD_DrawChinese16x16(24U, 32U, 0x5757U, LCD_COLOR_YELLOW, LCD_COLOR_BLACK);
  LCD_DrawChinese16x16(40U, 32U, 0x6D4BU, LCD_COLOR_YELLOW, LCD_COLOR_BLACK);
  LCD_DrawChinese16x16(56U, 32U, 0x8BD5U, LCD_COLOR_YELLOW, LCD_COLOR_BLACK);
  LCD_FillRect(10U, 160U, 220U, 8U, LCD_COLOR_GRAY);

  for (x = 10U; x < 210U; x++)
  {
    LCD_FillRect(10U, 150U, 220U, 28U, LCD_COLOR_BLACK);
    LCD_FillRect(10U, 160U, 220U, 8U, LCD_COLOR_GRAY);
    LCD_FillRect(x, 152U, 20U, 24U, LCD_COLOR_CYAN);
    LCD_DelayMs(8U);
  }
}

void LCD_TestFonts(void)
{
  LCD_FillScreen(LCD_COLOR_BLACK);
  LCD_DrawStringASCII(8U, 8U, "English Font16/20", &Font16, LCD_COLOR_GREEN, LCD_COLOR_BLACK);
  LCD_DrawStringASCII(8U, 32U, "PB0-15  8080 LCD", &Font16, LCD_COLOR_WHITE, LCD_COLOR_BLACK);
  LCD_DrawChinese16x16(8U, 72U, 0x4E2DU, LCD_COLOR_YELLOW, LCD_COLOR_BLACK);
  LCD_DrawChinese16x16(24U, 72U, 0x6587U, LCD_COLOR_YELLOW, LCD_COLOR_BLACK);
  LCD_DrawChinese16x16(40U, 72U, 0x5B57U, LCD_COLOR_YELLOW, LCD_COLOR_BLACK);
  LCD_DrawChinese16x16(56U, 72U, 0x5E93U, LCD_COLOR_YELLOW, LCD_COLOR_BLACK);

  LCD_DrawChinese16x16(8U, 96U, 0x4E2DU, LCD_COLOR_CYAN, LCD_COLOR_BLACK);
  LCD_DrawChinese16x16(24U, 96U, 0x82F1U, LCD_COLOR_CYAN, LCD_COLOR_BLACK);
  LCD_DrawChinese16x16(40U, 96U, 0x6587U, LCD_COLOR_CYAN, LCD_COLOR_BLACK);
  LCD_DrawChinese16x16(56U, 96U, 0x5B57U, LCD_COLOR_CYAN, LCD_COLOR_BLACK);
  LCD_DrawChinese16x16(72U, 96U, 0x5E93U, LCD_COLOR_CYAN, LCD_COLOR_BLACK);

  LCD_DrawChinese16x16(8U, 120U, 0x5B8CU, LCD_COLOR_GREEN, LCD_COLOR_BLACK);
  LCD_DrawChinese16x16(24U, 120U, 0x6210U, LCD_COLOR_GREEN, LCD_COLOR_BLACK);
}
