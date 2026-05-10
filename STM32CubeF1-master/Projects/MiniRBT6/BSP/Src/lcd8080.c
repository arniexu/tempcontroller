#include "lcd8080.h"
#include "lcd_font_zh.h"

#include <stddef.h>

static uint8_t g_lcd_is_ili9341 = 0U;
static uint8_t g_lcd_bare_mode = 1U;
static uint16_t g_lcd_ramwr_cmd = 0x2CU;
static uint8_t g_lcd_pixel_mode = LCD_PIXEL_MODE_16BIT_NORMAL;

#define LCD_NOP_PER_US     72U
#define LCD_TDS_NOP_COUNT  2U
#define LCD_TWL_NOP_COUNT  8U
#define LCD_TDH_NOP_COUNT  2U
#define LCD_CTRL_SETTLE_NOP_COUNT 2U
#define LCD_REG_PAIR_COUNT(arr) ((uint32_t)((sizeof(arr) / sizeof((arr)[0])) / 2U))

static inline void LCD_DelayNop(uint32_t nops)
{
  while (nops-- > 0U)
  {
    __NOP();
  }
}

static void LCD_DelayUs(uint32_t us)
{
  while (us-- > 0U)
  {
    LCD_DelayNop(LCD_NOP_PER_US);
  }
}

static void LCD_DelayMs(uint32_t ms)
{
  while (ms-- > 0U)
  {
    LCD_DelayUs(1000U);
  }
}

static inline void LCD_WriteBus16_Critical(uint16_t data)
{
  uint32_t primask = __get_PRIMASK();

  __disable_irq();

  LCD_DATA_GPIO_Port->ODR = data;
  LCD_DelayNop(LCD_TDS_NOP_COUNT); /* tDS ~= 14 ns */

  LCD_CTRL_GPIO_Port->BRR = LCD_WR_Pin;
  LCD_DelayNop(LCD_TWL_NOP_COUNT); /* tWL ~= 56 ns */

  LCD_CTRL_GPIO_Port->BSRR = LCD_WR_Pin;
  LCD_DelayNop(LCD_TDH_NOP_COUNT); /* tDH ~= 14 ns */

  if (primask == 0U)
  {
    __enable_irq();
  }
}

static inline void LCD_BusSelectCmd(void)
{
  /* CS=0, RS=0 in one atomic write. */
  LCD_CTRL_GPIO_Port->BSRR = ((uint32_t)LCD_CS_Pin << 16) | ((uint32_t)LCD_RS_Pin << 16);
  LCD_DelayNop(LCD_CTRL_SETTLE_NOP_COUNT);
}

static inline void LCD_BusSelectData(void)
{
  /* CS=0, RS=1 in one atomic write. */
  LCD_CTRL_GPIO_Port->BSRR = ((uint32_t)LCD_CS_Pin << 16) | (uint32_t)LCD_RS_Pin;
  LCD_DelayNop(LCD_CTRL_SETTLE_NOP_COUNT);
}

static inline void LCD_BusDeselect(void)
{
  LCD_CTRL_GPIO_Port->BSRR = LCD_CS_Pin;
  LCD_DelayNop(LCD_CTRL_SETTLE_NOP_COUNT);
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
  LCD_BusSelectCmd();
  LCD_WriteBus16_Critical(cmd);
  LCD_BusDeselect();
}

static void LCD_WriteData(uint16_t data)
{
  LCD_BusSelectData();
  LCD_WriteBus16_Critical(data);
  LCD_BusDeselect();
}

static inline void LCD_WriteDataStreamBegin(void)
{
  LCD_BusSelectData();
}

static inline void LCD_WriteDataStream16(uint16_t data)
{
  LCD_WriteBus16_Critical(data);
}

static inline void LCD_WritePixelData(uint16_t color)
{
  switch (g_lcd_pixel_mode)
  {
    case LCD_PIXEL_MODE_16BIT_SWAP_BYTES:
      LCD_WriteDataStream16((uint16_t)((color << 8) | (color >> 8)));
      break;

    case LCD_PIXEL_MODE_8BIT_LOW_LANE:
      LCD_WriteDataStream16((uint16_t)(color >> 8));
      LCD_WriteDataStream16((uint16_t)(color & 0x00FFU));
      break;

    case LCD_PIXEL_MODE_8BIT_HIGH_LANE:
      LCD_WriteDataStream16((uint16_t)((color >> 8) << 8));
      LCD_WriteDataStream16((uint16_t)((color & 0x00FFU) << 8));
      break;

    case LCD_PIXEL_MODE_16BIT_NORMAL:
    default:
      LCD_WriteDataStream16(color);
      break;
  }
}

static inline void LCD_WriteDataStreamEnd(void)
{
  LCD_BusDeselect();
}

static void LCD_WriteCmdData8Array(uint16_t cmd, const uint8_t *data, uint32_t count)
{
  uint32_t i;

  LCD_BusSelectCmd();
  LCD_WriteBus16_Critical(cmd);

  if ((data != NULL) && (count > 0U))
  {
    LCD_CTRL_GPIO_Port->BSRR = LCD_RS_Pin;
    for (i = 0U; i < count; i++)
    {
      LCD_WriteBus16_Critical((uint16_t)data[i]);
    }
  }

  LCD_BusDeselect();
}

static inline void LCD_WriteCmdData8(uint16_t cmd, uint8_t data)
{
  LCD_WriteCmdData8Array(cmd, &data, 1U);
}

static inline void LCD_WriteCmdData16(uint16_t cmd, uint16_t data)
{
  LCD_BusSelectCmd();
  LCD_WriteBus16_Critical(cmd);
  LCD_CTRL_GPIO_Port->BSRR = LCD_RS_Pin;
  LCD_WriteBus16_Critical(data);
  LCD_BusDeselect();
}

static void LCD_WriteReg16Pairs(const uint16_t *pairs, uint32_t pair_count)
{
  uint32_t i;

  if (pairs == NULL)
  {
    return;
  }

  for (i = 0U; i < pair_count; i++)
  {
    LCD_WriteCmdData16(pairs[i * 2U], pairs[i * 2U + 1U]);
  }
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
#if (LCD_MIRROR_Y == 1U)
  uint16_t sy0 = (uint16_t)((LCD_HEIGHT - 1U) - y1);
  uint16_t sy1 = (uint16_t)((LCD_HEIGHT - 1U) - y0);
#else
  uint16_t sy0 = y0;
  uint16_t sy1 = y1;
#endif

  if (g_lcd_bare_mode != 0U)
  {
    uint16_t sx0;
    uint16_t sx1;

#if (LCD_932X_MIRROR_X == 1U)
    sx0 = (uint16_t)((LCD_WIDTH - 1U) - x1);
    sx1 = (uint16_t)((LCD_WIDTH - 1U) - x0);
#else
    sx0 = x0;
    sx1 = x1;
#endif

    /* 9341-style window + memory write. */
    LCD_WriteCmd(0x2AU);
    LCD_WriteData8((uint8_t)(x0 >> 8));
    LCD_WriteData8((uint8_t)(x0 & 0x00FFU));
    LCD_WriteData8((uint8_t)(x1 >> 8));
    LCD_WriteData8((uint8_t)(x1 & 0x00FFU));

    LCD_WriteCmd(0x2BU);
    LCD_WriteData8((uint8_t)(sy0 >> 8));
    LCD_WriteData8((uint8_t)(sy0 & 0x00FFU));
    LCD_WriteData8((uint8_t)(sy1 >> 8));
    LCD_WriteData8((uint8_t)(sy1 & 0x00FFU));

    /* 932x-style GRAM window and cursor. */
    LCD_WriteCmd(0x50U);
    LCD_WriteData16(sx0);
    LCD_WriteCmd(0x51U);
    LCD_WriteData16(sx1);
    LCD_WriteCmd(0x52U);
    LCD_WriteData16(sy0);
    LCD_WriteCmd(0x53U);
    LCD_WriteData16(sy1);
    LCD_WriteCmd(0x20U);
    LCD_WriteData16(sx0);
    LCD_WriteCmd(0x21U);
    LCD_WriteData16(sy0);

    LCD_WriteCmd(g_lcd_ramwr_cmd);
  }
  else if (g_lcd_is_ili9341 != 0U)
  {
    LCD_WriteCmd(0x2AU);
    LCD_WriteData8((uint8_t)(x0 >> 8));
    LCD_WriteData8((uint8_t)(x0 & 0x00FFU));
    LCD_WriteData8((uint8_t)(x1 >> 8));
    LCD_WriteData8((uint8_t)(x1 & 0x00FFU));

    LCD_WriteCmd(0x2BU);
    LCD_WriteData8((uint8_t)(sy0 >> 8));
    LCD_WriteData8((uint8_t)(sy0 & 0x00FFU));
    LCD_WriteData8((uint8_t)(sy1 >> 8));
    LCD_WriteData8((uint8_t)(sy1 & 0x00FFU));

    LCD_WriteCmd(g_lcd_ramwr_cmd);
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
    LCD_WriteData16(sy0);
    LCD_WriteCmd(0x53U);
    LCD_WriteData16(sy1);
    LCD_WriteCmd(0x20U);
    LCD_WriteData16(sx0);
    LCD_WriteCmd(0x21U);
    LCD_WriteData16(sy0);
    LCD_WriteCmd(g_lcd_ramwr_cmd);
  }
}

static void LCD_GPIO_Init(void)
{
  /* Enable AFIO/GPIOB/GPIOC clocks via registers. */
  RCC->APB2ENR |= RCC_APB2ENR_AFIOEN | RCC_APB2ENR_IOPBEN | RCC_APB2ENR_IOPCEN;

  /* PB3/PB4 are JTAG pins by default on F103. Release them for D3/D4 while keeping SWD. */
  AFIO->MAPR &= ~AFIO_MAPR_SWJ_CFG;
  AFIO->MAPR |= AFIO_MAPR_SWJ_CFG_JTAGDISABLE;

  /* PB0..PB15: 50 MHz push-pull output for 8080 D0..D15. */
  LCD_DATA_GPIO_Port->CRL = 0x33333333U;
  LCD_DATA_GPIO_Port->CRH = 0x33333333U;

  /* PC6..PC10: 50 MHz push-pull output for RD/WR/RS/CS/BL. */
  LCD_CTRL_GPIO_Port->CRL = (LCD_CTRL_GPIO_Port->CRL & ~((0xFU << 24) | (0xFU << 28))) |
                            (0x3U << 24) | (0x3U << 28);
  LCD_CTRL_GPIO_Port->CRH = (LCD_CTRL_GPIO_Port->CRH & ~((0xFU << 0) | (0xFU << 4) | (0xFU << 8))) |
                            (0x3U << 0) | (0x3U << 4) | (0x3U << 8);

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
  static const uint8_t cmd_cf[] = {0x00U, 0x83U, 0x30U};
  static const uint8_t cmd_ed[] = {0x64U, 0x03U, 0x12U, 0x81U};
  static const uint8_t cmd_e8[] = {0x85U, 0x01U, 0x79U};
  static const uint8_t cmd_cb[] = {0x39U, 0x2CU, 0x00U, 0x34U, 0x02U};
  static const uint8_t cmd_ea[] = {0x00U, 0x00U};
  static const uint8_t cmd_c5[] = {0x35U, 0x3EU};
  static const uint8_t cmd_b1[] = {0x00U, 0x1BU};
  static const uint8_t cmd_b6[] = {0x0AU, 0xA2U};
  static const uint8_t cmd_e0[] = {
    0x1FU, 0x1AU, 0x18U, 0x0AU, 0x0FU, 0x06U, 0x45U, 0x87U,
    0x32U, 0x0AU, 0x07U, 0x02U, 0x07U, 0x05U, 0x00U
  };
  static const uint8_t cmd_e1[] = {
    0x00U, 0x25U, 0x27U, 0x05U, 0x10U, 0x09U, 0x3AU, 0x78U,
    0x4DU, 0x05U, 0x18U, 0x0DU, 0x38U, 0x3AU, 0x1FU
  };

  LCD_DelayMs(20U);
  g_lcd_ramwr_cmd = 0x2CU;

  LCD_WriteCmd(0x01U);
  LCD_DelayMs(120U);

  LCD_WriteCmd(0x28U);

  LCD_WriteCmdData8Array(0xCFU, cmd_cf, sizeof(cmd_cf));
  LCD_WriteCmdData8Array(0xEDU, cmd_ed, sizeof(cmd_ed));
  LCD_WriteCmdData8Array(0xE8U, cmd_e8, sizeof(cmd_e8));
  LCD_WriteCmdData8Array(0xCBU, cmd_cb, sizeof(cmd_cb));
  LCD_WriteCmdData8(0xF7U, 0x20U);
  LCD_WriteCmdData8Array(0xEAU, cmd_ea, sizeof(cmd_ea));
  LCD_WriteCmdData8(0xC0U, 0x26U);
  LCD_WriteCmdData8(0xC1U, 0x11U);
  LCD_WriteCmdData8Array(0xC5U, cmd_c5, sizeof(cmd_c5));
  LCD_WriteCmdData8(0xC7U, 0xBEU);
  LCD_WriteCmdData8(0x36U, 0x48U);
  LCD_WriteCmdData8(0x3AU, 0x55U);
  LCD_WriteCmdData8Array(0xB1U, cmd_b1, sizeof(cmd_b1));
  LCD_WriteCmdData8Array(0xB6U, cmd_b6, sizeof(cmd_b6));
  LCD_WriteCmdData8(0xF2U, 0x00U);
  LCD_WriteCmdData8(0x26U, 0x01U);
  LCD_WriteCmdData8Array(0xE0U, cmd_e0, sizeof(cmd_e0));
  LCD_WriteCmdData8Array(0xE1U, cmd_e1, sizeof(cmd_e1));

  LCD_WriteCmd(0x11U);
  LCD_DelayMs(120U);
  LCD_WriteCmd(0x29U);
  LCD_DelayMs(20U);
}

static void LCD_Init_ILI9325(void)
{
  static const uint16_t seq_9325_a[] = {
    0x00E5U, 0x8000U, 0x0000U, 0x0001U, 0x0001U, 0x0100U, 0x0002U, 0x0700U,
    0x0003U, LCD_932X_ENTRY_MODE, 0x0004U, 0x0000U, 0x0008U, 0x0202U, 0x0009U, 0x0000U,
    0x000AU, 0x0000U, 0x000CU, 0x0000U, 0x000DU, 0x0000U, 0x000FU, 0x0000U,
    0x0010U, 0x0000U, 0x0011U, 0x0000U, 0x0012U, 0x0000U, 0x0013U, 0x0000U
  };
  static const uint16_t seq_9325_b[] = {
    0x0010U, 0x17B0U, 0x0011U, 0x0137U
  };
  static const uint16_t seq_9325_c[] = {
    0x0012U, 0x0139U
  };
  static const uint16_t seq_9325_d[] = {
    0x0013U, 0x1D00U, 0x0029U, 0x0013U
  };
  static const uint16_t seq_9325_e[] = {
    0x0020U, 0x0000U, 0x0021U, 0x0000U
  };
  static const uint16_t seq_9325_f[] = {
    0x0030U, 0x0006U, 0x0031U, 0x0101U, 0x0032U, 0x0003U, 0x0035U, 0x0106U,
    0x0036U, 0x0B02U, 0x0037U, 0x0302U, 0x0038U, 0x0707U, 0x0039U, 0x0007U,
    0x003CU, 0x0600U, 0x003DU, 0x020BU
  };
  static const uint16_t seq_9325_g[] = {
    0x0050U, 0x0000U, 0x0051U, 0x00EFU, 0x0052U, 0x0000U, 0x0053U, 0x013FU,
    0x0060U, 0xA700U, 0x0061U, 0x0001U, 0x006AU, 0x0000U, 0x0090U, 0x0010U,
    0x0092U, 0x0000U, 0x0093U, 0x0003U, 0x0095U, 0x0110U, 0x0097U, 0x0000U,
    0x0098U, 0x0000U, 0x0003U, LCD_932X_ENTRY_MODE
  };

  LCD_DelayMs(50U);
  g_lcd_ramwr_cmd = 0x22U;

  LCD_WriteReg16Pairs(seq_9325_a, LCD_REG_PAIR_COUNT(seq_9325_a));
  LCD_DelayMs(20U);

  LCD_WriteReg16Pairs(seq_9325_b, LCD_REG_PAIR_COUNT(seq_9325_b));
  LCD_DelayMs(5U);
  LCD_WriteReg16Pairs(seq_9325_c, LCD_REG_PAIR_COUNT(seq_9325_c));
  LCD_DelayMs(5U);
  LCD_WriteReg16Pairs(seq_9325_d, LCD_REG_PAIR_COUNT(seq_9325_d));
  LCD_DelayMs(5U);

  LCD_WriteReg16Pairs(seq_9325_e, LCD_REG_PAIR_COUNT(seq_9325_e));
  LCD_WriteReg16Pairs(seq_9325_f, LCD_REG_PAIR_COUNT(seq_9325_f));
  LCD_WriteReg16Pairs(seq_9325_g, LCD_REG_PAIR_COUNT(seq_9325_g));

  LCD_WriteCmdData16(0x0007U, 0x0173U);
}

static void LCD_Init_R61509_5408(void)
{
  static const uint16_t seq_5408_a[] = {
    0x0001U, 0x0100U, 0x0002U, 0x0700U, 0x0003U, LCD_932X_ENTRY_MODE, 0x0004U, 0x0000U,
    0x0008U, 0x0202U, 0x0009U, 0x0000U, 0x000AU, 0x0000U, 0x000CU, 0x0000U,
    0x000DU, 0x0000U, 0x000FU, 0x0000U, 0x0010U, 0x0000U, 0x0011U, 0x0000U,
    0x0012U, 0x0000U, 0x0013U, 0x0000U
  };
  static const uint16_t seq_5408_b[] = {
    0x0011U, 0x0007U
  };
  static const uint16_t seq_5408_c[] = {
    0x0010U, 0x12B0U
  };
  static const uint16_t seq_5408_d[] = {
    0x0012U, 0x01BDU
  };
  static const uint16_t seq_5408_e[] = {
    0x0013U, 0x1400U, 0x0029U, 0x000EU
  };
  static const uint16_t seq_5408_f[] = {
    0x0020U, 0x0000U, 0x0021U, 0x013FU
  };
  static const uint16_t seq_5408_g[] = {
    0x0030U, 0x0B0DU, 0x0031U, 0x1923U, 0x0032U, 0x1C26U, 0x0035U, 0x0D0BU,
    0x0036U, 0x1006U, 0x0037U, 0x0610U, 0x0038U, 0x0706U, 0x0039U, 0x0304U,
    0x003AU, 0x0E05U, 0x003BU, 0x0E01U, 0x003CU, 0x010EU, 0x003DU, 0x050EU,
    0x003EU, 0x0403U, 0x003FU, 0x0607U
  };
  static const uint16_t seq_5408_h[] = {
    0x0050U, 0x0000U, 0x0051U, 0x00EFU, 0x0052U, 0x0000U, 0x0053U, 0x013FU,
    0x0060U, 0xA700U, 0x0061U, 0x0001U, 0x006AU, 0x0000U, 0x0080U, 0x0000U,
    0x0081U, 0x0000U, 0x0082U, 0x0000U, 0x0083U, 0x0000U, 0x0084U, 0x0000U,
    0x0085U, 0x0000U, 0x0090U, 0x0010U, 0x0092U, 0x0000U, 0x0093U, 0x0003U,
    0x0095U, 0x0110U, 0x0097U, 0x0000U, 0x0098U, 0x0000U, 0x0003U, LCD_932X_ENTRY_MODE
  };

  LCD_DelayMs(50U);
  g_lcd_ramwr_cmd = 0x22U;

  LCD_WriteReg16Pairs(seq_5408_a, LCD_REG_PAIR_COUNT(seq_5408_a));
  LCD_DelayMs(20U);

  LCD_WriteReg16Pairs(seq_5408_b, LCD_REG_PAIR_COUNT(seq_5408_b));
  LCD_DelayMs(5U);
  LCD_WriteReg16Pairs(seq_5408_c, LCD_REG_PAIR_COUNT(seq_5408_c));
  LCD_DelayMs(5U);
  LCD_WriteReg16Pairs(seq_5408_d, LCD_REG_PAIR_COUNT(seq_5408_d));
  LCD_DelayMs(5U);
  LCD_WriteReg16Pairs(seq_5408_e, LCD_REG_PAIR_COUNT(seq_5408_e));
  LCD_DelayMs(5U);

  LCD_WriteReg16Pairs(seq_5408_f, LCD_REG_PAIR_COUNT(seq_5408_f));
  LCD_WriteReg16Pairs(seq_5408_g, LCD_REG_PAIR_COUNT(seq_5408_g));
  LCD_WriteReg16Pairs(seq_5408_h, LCD_REG_PAIR_COUNT(seq_5408_h));
  LCD_WriteCmdData16(0x0007U, 0x0112U);
}

static void LCD_Controller_Init(void)
{
  uint16_t id16 = LCD_ReadReg16(0x0000U);
  uint32_t id9341 = LCD_ReadID9341();

  if ((id9341 & 0x0000FFFFUL) == 0x9341UL)
  {
    g_lcd_is_ili9341 = 1U;
    g_lcd_ramwr_cmd = 0x2CU;
    LCD_Init_ILI9341();
    return;
  }

  if ((id16 == 0x9325U) || (id16 == 0x9328U) || (id16 == 0x5408U) || (id16 == 0x8989U))
  {
    g_lcd_is_ili9341 = 0U;
    g_lcd_ramwr_cmd = 0x22U;
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
  g_lcd_ramwr_cmd = 0x22U;
  LCD_Init_ILI9325();
}

static void LCD_Init_Bare8080(void)
{
  g_lcd_bare_mode = 1U;
  g_lcd_is_ili9341 = 0U;
  g_lcd_ramwr_cmd = 0x2CU;

  LCD_DelayMs(20U);

  /* Common command superset for many 8080 TFT controllers. */
  LCD_WriteCmd(0x01U);
  LCD_DelayMs(120U);
  LCD_WriteCmd(0x11U);
  LCD_DelayMs(120U);
  LCD_WriteCmd(0x13U);
  LCD_WriteCmd(0x29U);

  /* Try 16-bit color mode and typical scan direction in a controller-agnostic way. */
  LCD_WriteCmdData8(0x3AU, 0x55U);
  LCD_WriteCmdData8(0x36U, 0x48U);

  LCD_WriteCmdData16(0x0003U, LCD_932X_ENTRY_MODE);
  LCD_WriteCmdData16(0x0007U, 0x0173U);
}

void LCD_SetBacklightRaw(uint8_t high_level)
{
  if (high_level != 0U)
  {
    LCD_CTRL_GPIO_Port->BSRR = LCD_BL_Pin;
  }
  else
  {
    LCD_CTRL_GPIO_Port->BRR = LCD_BL_Pin;
  }
}

void LCD_SetRamWriteCmd(uint16_t cmd)
{
  if ((cmd == 0x2CU) || (cmd == 0x22U))
  {
    g_lcd_ramwr_cmd = cmd;
  }
}

uint16_t LCD_GetRamWriteCmd(void)
{
  return g_lcd_ramwr_cmd;
}

void LCD_SetPixelWriteMode(uint8_t mode)
{
  if (mode <= LCD_PIXEL_MODE_8BIT_HIGH_LANE)
  {
    g_lcd_pixel_mode = mode;
  }
}

uint8_t LCD_GetPixelWriteMode(void)
{
  return g_lcd_pixel_mode;
}

void LCD_InitWithProfile(uint8_t profile)
{
  LCD_GPIO_Init();
  g_lcd_bare_mode = 0U;

  switch (profile)
  {
    case 1U:
      g_lcd_is_ili9341 = 1U;
      LCD_Init_ILI9341();
      break;
    case 2U:
      g_lcd_is_ili9341 = 0U;
      LCD_Init_R61509_5408();
      break;
    case 3U:
      g_lcd_is_ili9341 = 0U;
      LCD_Init_ILI9325();
      break;
    case 0U:
    default:
      LCD_Controller_Init();
      break;
  }

  LCD_FillScreen(LCD_COLOR_BLACK);
}

void LCD_Init(void)
{
  LCD_GPIO_Init();
  LCD_Init_Bare8080();
  LCD_FillScreen(LCD_COLOR_BLACK);
}

void LCD_DrawPixel(uint16_t x, uint16_t y, uint16_t color)
{
  if ((x >= LCD_WIDTH) || (y >= LCD_HEIGHT))
  {
    return;
  }

  LCD_SetWindow(x, y, x, y);
  LCD_WriteDataStreamBegin();
  LCD_WritePixelData(color);
  LCD_WriteDataStreamEnd();
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
  LCD_WriteDataStreamBegin();
  for (i = 0U; i < count; i++)
  {
    LCD_WritePixelData(color);
  }
  LCD_WriteDataStreamEnd();
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
