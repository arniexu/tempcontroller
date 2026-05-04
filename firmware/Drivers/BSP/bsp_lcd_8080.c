#include "app_config.h"

#if defined(APP_DISPLAY_DRIVER_LCD_8080)

#include "bsp_lcd_8080.h"

#include <string.h>

#include "tiny_graphics.h"

#if defined(USE_HAL_DRIVER)
#include "stm32f1xx_hal.h"

#define TFT_DATA_PORT               GPIOB
#define TFT_CTRL_PORT               GPIOC

#define TFT_PIN_RD                  GPIO_PIN_6
#define TFT_PIN_WR                  GPIO_PIN_7
#define TFT_PIN_RS                  GPIO_PIN_8
#define TFT_PIN_CS                  GPIO_PIN_9
#define TFT_PIN_BL                  GPIO_PIN_10

#define TFT_GPIO_SPEED              GPIO_SPEED_FREQ_LOW
#define TFT_BUS_SETTLE_NOP          (1U)
#define TFT_WR_LOW_NOP              (6U)
#define TFT_WR_HIGH_NOP             (6U)
#define TFT_MIRROR_X_AROUND_CENTER  (0U)
#define TFT_LCD_ENTRY_MODE          (0x1018U)
#define TFT_DRAW_ROW_FROM_END       (0U)
#define TFT_TEXT_LOAD_Y_REVERSED    (1U)

#define OLED_COLOR_BG               (BSP_OLED_COLOR_BLACK)
#define OLED_COLOR_FG               (BSP_OLED_COLOR_WHITE)

#define LCD_TEXT_SCALE              (1U)
#define LCD_GLYPH_WIDTH             (5U)
#define LCD_GLYPH_HEIGHT            (7U)
#define LCD_CHAR_WIDTH              ((LCD_GLYPH_WIDTH * LCD_TEXT_SCALE) + 1U)
#define LCD_CHAR_HEIGHT             (LCD_GLYPH_HEIGHT * LCD_TEXT_SCALE)
#define LCD_TEXT_START_X            (1U)
#define LCD_TEXT_CLEAR_MARGIN_Y     (1U)
#define TFT_PHYSICAL_WIDTH          (240U)
#define TFT_PHYSICAL_HEIGHT         (320U)

#define LCD_REG_0                   (0x00U)
#define LCD_REG_1                   (0x01U)
#define LCD_REG_2                   (0x02U)
#define LCD_REG_3                   (0x03U)
#define LCD_REG_4                   (0x04U)
#define LCD_REG_7                   (0x07U)
#define LCD_REG_8                   (0x08U)
#define LCD_REG_9                   (0x09U)
#define LCD_REG_10                  (0x0AU)
#define LCD_REG_12                  (0x0CU)
#define LCD_REG_13                  (0x0DU)
#define LCD_REG_15                  (0x0FU)
#define LCD_REG_16                  (0x10U)
#define LCD_REG_17                  (0x11U)
#define LCD_REG_18                  (0x12U)
#define LCD_REG_19                  (0x13U)
#define LCD_REG_32                  (0x20U)
#define LCD_REG_33                  (0x21U)
#define LCD_REG_34                  (0x22U)
#define LCD_REG_41                  (0x29U)
#define LCD_REG_48                  (0x30U)
#define LCD_REG_49                  (0x31U)
#define LCD_REG_50                  (0x32U)
#define LCD_REG_53                  (0x35U)
#define LCD_REG_54                  (0x36U)
#define LCD_REG_55                  (0x37U)
#define LCD_REG_56                  (0x38U)
#define LCD_REG_57                  (0x39U)
#define LCD_REG_58                  (0x3AU)
#define LCD_REG_59                  (0x3BU)
#define LCD_REG_60                  (0x3CU)
#define LCD_REG_61                  (0x3DU)
#define LCD_REG_62                  (0x3EU)
#define LCD_REG_63                  (0x3FU)
#define LCD_REG_80                  (0x50U)
#define LCD_REG_81                  (0x51U)
#define LCD_REG_82                  (0x52U)
#define LCD_REG_83                  (0x53U)
#define LCD_REG_96                  (0x60U)
#define LCD_REG_97                  (0x61U)
#define LCD_REG_106                 (0x6AU)
#define LCD_REG_128                 (0x80U)
#define LCD_REG_129                 (0x81U)
#define LCD_REG_130                 (0x82U)
#define LCD_REG_131                 (0x83U)
#define LCD_REG_132                 (0x84U)
#define LCD_REG_133                 (0x85U)
#define LCD_REG_144                 (0x90U)
#define LCD_REG_146                 (0x92U)
#define LCD_REG_147                 (0x93U)
#define LCD_REG_149                 (0x95U)
#define LCD_REG_151                 (0x97U)
#define LCD_REG_152                 (0x98U)
#define LCD_REG_229                 (0xE5U)

static uint16_t g_lcd_id = 0U;
static uint8_t g_lcd_ready = 0U;
#endif

static char g_lines[BSP_OLED_LINE_COUNT][BSP_OLED_LINE_CHARS + 1U];
static char g_drawn_lines[BSP_OLED_LINE_COUNT][BSP_OLED_LINE_CHARS + 1U];
static uint8_t g_refresh_pending = 0U;
static uint8_t g_refresh_line = 0U;
static uint8_t g_dirty_mask = 0U;

#if defined(USE_HAL_DRIVER)
static tg_canvas_t g_tg_canvas;
static void lcd_fill_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color);
#endif

#if defined(USE_HAL_DRIVER)
static void glyph_5x7(char c, uint8_t out[5])
{
    uint8_t i;

    for (i = 0U; i < 5U; ++i)
    {
        out[i] = 0x00U;
    }

    if ((c >= '0') && (c <= '9'))
    {
        static const uint8_t digit[10][5] = {
            {0x3EU, 0x51U, 0x49U, 0x45U, 0x3EU},
            {0x00U, 0x42U, 0x7FU, 0x40U, 0x00U},
            {0x62U, 0x51U, 0x49U, 0x49U, 0x46U},
            {0x22U, 0x49U, 0x49U, 0x49U, 0x36U},
            {0x18U, 0x14U, 0x12U, 0x7FU, 0x10U},
            {0x2FU, 0x49U, 0x49U, 0x49U, 0x31U},
            {0x3EU, 0x49U, 0x49U, 0x49U, 0x32U},
            {0x01U, 0x71U, 0x09U, 0x05U, 0x03U},
            {0x36U, 0x49U, 0x49U, 0x49U, 0x36U},
            {0x26U, 0x49U, 0x49U, 0x49U, 0x3EU}
        };
        memcpy(out, digit[(unsigned int)(c - '0')], 5U);
        return;
    }

    switch (c)
    {
    case 'A': out[0]=0x7EU; out[1]=0x11U; out[2]=0x11U; out[3]=0x11U; out[4]=0x7EU; break;
    case 'B': out[0]=0x7FU; out[1]=0x49U; out[2]=0x49U; out[3]=0x49U; out[4]=0x36U; break;
    case 'C': out[0]=0x3EU; out[1]=0x41U; out[2]=0x41U; out[3]=0x41U; out[4]=0x22U; break;
    case 'D': out[0]=0x7FU; out[1]=0x41U; out[2]=0x41U; out[3]=0x22U; out[4]=0x1CU; break;
    case 'E': out[0]=0x7FU; out[1]=0x49U; out[2]=0x49U; out[3]=0x49U; out[4]=0x41U; break;
    case 'F': out[0]=0x7FU; out[1]=0x09U; out[2]=0x09U; out[3]=0x09U; out[4]=0x01U; break;
    case 'G': out[0]=0x3EU; out[1]=0x41U; out[2]=0x49U; out[3]=0x49U; out[4]=0x3AU; break;
    case 'H': out[0]=0x7FU; out[1]=0x08U; out[2]=0x08U; out[3]=0x08U; out[4]=0x7FU; break;
    case 'I': out[0]=0x00U; out[1]=0x41U; out[2]=0x7FU; out[3]=0x41U; out[4]=0x00U; break;
    case 'K': out[0]=0x7FU; out[1]=0x08U; out[2]=0x14U; out[3]=0x22U; out[4]=0x41U; break;
    case 'L': out[0]=0x7FU; out[1]=0x40U; out[2]=0x40U; out[3]=0x40U; out[4]=0x40U; break;
    case 'M': out[0]=0x7FU; out[1]=0x02U; out[2]=0x0CU; out[3]=0x02U; out[4]=0x7FU; break;
    case 'N': out[0]=0x7FU; out[1]=0x04U; out[2]=0x08U; out[3]=0x10U; out[4]=0x7FU; break;
    case 'O': out[0]=0x3EU; out[1]=0x41U; out[2]=0x41U; out[3]=0x41U; out[4]=0x3EU; break;
    case 'P': out[0]=0x7FU; out[1]=0x09U; out[2]=0x09U; out[3]=0x09U; out[4]=0x06U; break;
    case 'R': out[0]=0x7FU; out[1]=0x09U; out[2]=0x19U; out[3]=0x29U; out[4]=0x46U; break;
    case 'S': out[0]=0x46U; out[1]=0x49U; out[2]=0x49U; out[3]=0x49U; out[4]=0x31U; break;
    case 'T': out[0]=0x01U; out[1]=0x01U; out[2]=0x7FU; out[3]=0x01U; out[4]=0x01U; break;
    case 'U': out[0]=0x3FU; out[1]=0x40U; out[2]=0x40U; out[3]=0x40U; out[4]=0x3FU; break;
    case 'V': out[0]=0x1FU; out[1]=0x20U; out[2]=0x40U; out[3]=0x20U; out[4]=0x1FU; break;
    case 'W': out[0]=0x3FU; out[1]=0x40U; out[2]=0x38U; out[3]=0x40U; out[4]=0x3FU; break;
    case 'X': out[0]=0x63U; out[1]=0x14U; out[2]=0x08U; out[3]=0x14U; out[4]=0x63U; break;
    case 'Y': out[0]=0x07U; out[1]=0x08U; out[2]=0x70U; out[3]=0x08U; out[4]=0x07U; break;
    case 'Z': out[0]=0x61U; out[1]=0x51U; out[2]=0x49U; out[3]=0x45U; out[4]=0x43U; break;
    case ' ': break;
    case '+': out[0]=0x08U; out[1]=0x08U; out[2]=0x3EU; out[3]=0x08U; out[4]=0x08U; break;
    case '.': out[2]=0x60U; break;
    case ':': out[1]=0x36U; out[3]=0x36U; break;
    case '-': out[1]=0x08U; out[2]=0x08U; out[3]=0x08U; break;
    case '?': out[0]=0x02U; out[1]=0x01U; out[2]=0x59U; out[3]=0x09U; out[4]=0x06U; break;
    case '>': out[0]=0x00U; out[1]=0x41U; out[2]=0x22U; out[3]=0x14U; out[4]=0x08U; break;
    case '/': out[0]=0x20U; out[1]=0x10U; out[2]=0x08U; out[3]=0x04U; out[4]=0x02U; break;
    case '*': out[0]=0x14U; out[1]=0x08U; out[2]=0x3EU; out[3]=0x08U; out[4]=0x14U; break;
    default:
        out[0]=0x7FU; out[1]=0x41U; out[2]=0x5DU; out[3]=0x41U; out[4]=0x7FU;
        break;
    }
}

static void tg_glyph_5x7(char c, uint8_t *columns, uint8_t *width, uint8_t *height)
{
    glyph_5x7(c, columns);
    *width = LCD_GLYPH_WIDTH;
    *height = LCD_GLYPH_HEIGHT;
}
#endif

static void oled_build_frame(void)
{
}

static uint16_t oled_line_y(uint8_t line)
{
    static const uint16_t y_pos[BSP_OLED_LINE_COUNT] = {4U, 20U, 36U, 52U};

    if (line >= BSP_OLED_LINE_COUNT)
    {
        return 0U;
    }

    return y_pos[line];
}

#if defined(USE_HAL_DRIVER)
static void tg_lcd_fill_rect(void *ctx,
                             uint16_t x,
                             uint16_t y,
                             uint16_t w,
                             uint16_t h,
                             tg_color_t color)
{
    (void)ctx;
    lcd_fill_rect(x, y, w, h, color);
}

static void lcd_delay_cycles(unsigned int n)
{
    volatile unsigned int i;

    for (i = 0U; i < n; ++i)
    {
        __NOP();
    }
}

static void lcd_delay(unsigned int n)
{
    uint32_t start_tick;
    uint32_t spin_guard;

    if (n == 0U)
    {
        return;
    }

    /* Prefer the SysTick-backed HAL tick so Release optimization does not shrink
     * LCD power-on delays below the controller's timing requirements. */
    start_tick = HAL_GetTick();
    spin_guard = n * 72000U;
    while ((HAL_GetTick() - start_tick) < (uint32_t)n)
    {
        __NOP();
        if (spin_guard == 0U)
        {
            break;
        }
        spin_guard--;
    }

    if ((HAL_GetTick() - start_tick) >= (uint32_t)n)
    {
        return;
    }

    /* Fallback for cases where the tick is not advancing yet. */
    lcd_delay_cycles(n * 6000U);
}

static void lcd_ctrl_write(uint16_t pin, int high)
{
    HAL_GPIO_WritePin(TFT_CTRL_PORT, pin, high ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

static void lcd_data_mode(uint32_t mode)
{
    GPIO_InitTypeDef gpio = {0};

    gpio.Pin = 0xFFFFU;
    gpio.Mode = mode;
    gpio.Pull = GPIO_NOPULL;
    gpio.Speed = TFT_GPIO_SPEED;
    HAL_GPIO_Init(TFT_DATA_PORT, &gpio);
}

static void lcd_write_timing_delay(unsigned int cycles)
{
    while (cycles-- > 0U)
    {
        __NOP();
    }
}

static void lcd_write_bus(uint16_t value)
{
    TFT_DATA_PORT->ODR = value;
    lcd_write_timing_delay(TFT_BUS_SETTLE_NOP);
}

static uint16_t lcd_read_bus(void)
{
    return (uint16_t)TFT_DATA_PORT->IDR;
}

static void lcd_write_strobe(void)
{
    lcd_ctrl_write(TFT_PIN_WR, 0);
    lcd_write_timing_delay(TFT_WR_LOW_NOP);
    lcd_ctrl_write(TFT_PIN_WR, 1);
    lcd_write_timing_delay(TFT_WR_HIGH_NOP);
}

static uint16_t lcd_read_strobe(void)
{
    uint16_t value;

    lcd_ctrl_write(TFT_PIN_RD, 0);
    value = lcd_read_bus();
    lcd_ctrl_write(TFT_PIN_RD, 1);
    return value;
}

static void lcd_write_index(uint16_t reg)
{
    lcd_ctrl_write(TFT_PIN_RS, 0);
    lcd_write_bus(reg);
    lcd_write_strobe();
}

static void lcd_write_data(uint16_t value)
{
    lcd_ctrl_write(TFT_PIN_RS, 1);
    lcd_write_bus(value);
    lcd_write_strobe();
}

static void lcd_write_reg(uint16_t reg, uint16_t value)
{
    lcd_write_index(reg);
    lcd_write_data(value);
}

static uint16_t lcd_read_reg(uint16_t reg)
{
    uint16_t value;

    lcd_write_index(reg);
    lcd_data_mode(GPIO_MODE_INPUT);
    lcd_ctrl_write(TFT_PIN_RS, 1);
    (void)lcd_read_strobe();
    value = lcd_read_strobe();
    lcd_data_mode(GPIO_MODE_OUTPUT_PP);
    return value;
}

static void lcd_set_cursor(uint16_t x, uint16_t y)
{
    lcd_write_reg(LCD_REG_32, x);
    lcd_write_reg(LCD_REG_33, y);
}

static void lcd_prepare_write_ram(void)
{
    lcd_write_index(LCD_REG_34);
}

static void lcd_set_window(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1)
{
    lcd_write_reg(LCD_REG_80, x0);
    lcd_write_reg(LCD_REG_81, x1);
    lcd_write_reg(LCD_REG_82, y0);
    lcd_write_reg(LCD_REG_83, y1);
#if (TFT_DRAW_ROW_FROM_END == 1U)
    lcd_set_cursor(x1, y0);
#else
    lcd_set_cursor(x0, y0);
#endif
}

static void lcd_fill_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color)
{
    uint16_t draw_x;
    uint32_t count;

    if ((w == 0U) || (h == 0U))
    {
        return;
    }

    draw_x = x;
#if (TFT_MIRROR_X_AROUND_CENTER == 1U)
    {
        uint16_t x_end = (uint16_t)(x + w - 1U);
        draw_x = (uint16_t)((BSP_OLED_WIDTH - 1U) - x_end);
    }
#endif

    lcd_set_window(draw_x,
                   y,
                   (uint16_t)(draw_x + w - 1U),
                   (uint16_t)(y + h - 1U));
    lcd_prepare_write_ram();
    for (count = 0UL; count < ((uint32_t)w * (uint32_t)h); ++count)
    {
        lcd_write_data(color);
    }
}

static void lcd_draw_text_dir(uint16_t x,
                              uint16_t y,
                              const char *text,
                              uint8_t scale,
                              uint16_t color,
                              uint8_t x_decreasing)
{
    uint16_t cursor_x;
    uint16_t advance;
    uint16_t i;

    if ((text == 0) || (scale == 0U))
    {
        return;
    }

    advance = (uint16_t)((LCD_GLYPH_WIDTH * scale) + 1U);
    cursor_x = x;

    for (i = 0U; text[i] != '\0'; ++i)
    {
        uint8_t glyph[8];
        uint8_t glyph_w = 0U;
        uint8_t glyph_h = 0U;
        uint8_t col;

        tg_glyph_5x7(text[i], glyph, &glyph_w, &glyph_h);
        for (col = 0U; col < glyph_w; ++col)
        {
            uint8_t bits = glyph[col];
#if (TFT_TEXT_LOAD_Y_REVERSED == 1U)
            int16_t row = (int16_t)glyph_h - 1;

            while (row >= 0)
            {
                if ((bits & (1U << (uint8_t)row)) != 0U)
                {
                    uint8_t run = 1U;

                    while (((int16_t)row - (int16_t)run) >= 0 &&
                           ((bits & (1U << (uint8_t)((int16_t)row - (int16_t)run))) != 0U))
                    {
                        ++run;
                    }

                    tg_fill_rect(&g_tg_canvas,
                                 cursor_x + ((uint16_t)col * scale),
                                 y + ((uint16_t)(((int16_t)row - (int16_t)run) + 1) * scale),
                                 scale,
                                 (uint16_t)run * scale,
                                 color);
                    row = (int16_t)(row - (int16_t)run);
                }
                else
                {
                    --row;
                }
            }
#else
            uint8_t row = 0U;

            while (row < glyph_h)
            {
                if ((bits & (1U << row)) != 0U)
                {
                    uint8_t run = 1U;

                    while (((uint8_t)(row + run) < glyph_h) && ((bits & (1U << (row + run))) != 0U))
                    {
                        ++run;
                    }

                    tg_fill_rect(&g_tg_canvas,
                                 cursor_x + ((uint16_t)col * scale),
                                 y + ((uint16_t)row * scale),
                                 scale,
                                 (uint16_t)run * scale,
                                 color);
                    row = (uint8_t)(row + run);
                }
                else
                {
                    ++row;
                }
            }
#endif
        }

        if (x_decreasing != 0U)
        {
            if (cursor_x < advance)
            {
                break;
            }
            cursor_x = (uint16_t)(cursor_x - advance);
        }
        else
        {
            if (cursor_x > (uint16_t)(BSP_OLED_WIDTH - advance))
            {
                break;
            }
            cursor_x = (uint16_t)(cursor_x + advance);
        }
    }
}

static void lcd_draw_text_line(uint8_t line)
{
    char line_buf[BSP_OLED_LINE_CHARS + 1U];
    uint16_t line_start_x;
    uint16_t line_y;

    if (line >= BSP_OLED_LINE_COUNT)
    {
        return;
    }

    line_y = oled_line_y(line);

    lcd_fill_rect(0U,
                  (uint16_t)(line_y - LCD_TEXT_CLEAR_MARGIN_Y),
                  BSP_OLED_WIDTH,
                  (uint16_t)(LCD_CHAR_HEIGHT + (LCD_TEXT_CLEAR_MARGIN_Y * 2U)),
                  OLED_COLOR_BG);
    strncpy(line_buf, g_lines[line], BSP_OLED_LINE_CHARS);
    line_buf[BSP_OLED_LINE_CHARS] = '\0';
    
#if (TFT_DRAW_ROW_FROM_END == 1U)
    line_start_x = (uint16_t)(BSP_OLED_WIDTH - LCD_TEXT_START_X - LCD_CHAR_WIDTH);
#else
    line_start_x = LCD_TEXT_START_X;
#endif

    lcd_draw_text_dir(line_start_x,
                      line_y,
                      line_buf,
                      LCD_TEXT_SCALE,
                      OLED_COLOR_FG,
                      TFT_DRAW_ROW_FROM_END);
}

static void lcd_init_932x(void)
{
    lcd_write_reg(LCD_REG_229, 0x8000U);
    lcd_write_reg(LCD_REG_0,   0x0001U);
    lcd_write_reg(LCD_REG_1,   0x0100U);
    lcd_write_reg(LCD_REG_2,   0x0700U);
    lcd_write_reg(LCD_REG_3,   TFT_LCD_ENTRY_MODE);
    lcd_write_reg(LCD_REG_4,   0x0000U);
    lcd_write_reg(LCD_REG_8,   0x0202U);
    lcd_write_reg(LCD_REG_9,   0x0000U);
    lcd_write_reg(LCD_REG_10,  0x0000U);
    lcd_write_reg(LCD_REG_12,  0x0000U);
    lcd_write_reg(LCD_REG_13,  0x0000U);
    lcd_write_reg(LCD_REG_15,  0x0000U);
    lcd_write_reg(LCD_REG_16,  0x0000U);
    lcd_write_reg(LCD_REG_17,  0x0000U);
    lcd_write_reg(LCD_REG_18,  0x0000U);
    lcd_write_reg(LCD_REG_19,  0x0000U);
    lcd_delay(20U);
    lcd_write_reg(LCD_REG_16,  0x17B0U);
    lcd_write_reg(LCD_REG_17,  0x0137U);
    lcd_delay(5U);
    lcd_write_reg(LCD_REG_18,  0x0139U);
    lcd_delay(5U);
    lcd_write_reg(LCD_REG_19,  0x1D00U);
    lcd_write_reg(LCD_REG_41,  0x0013U);
    lcd_delay(5U);
    lcd_write_reg(LCD_REG_32,  0x0000U);
    lcd_write_reg(LCD_REG_33,  0x0000U);
    lcd_write_reg(LCD_REG_48,  0x0006U);
    lcd_write_reg(LCD_REG_49,  0x0101U);
    lcd_write_reg(LCD_REG_50,  0x0003U);
    lcd_write_reg(LCD_REG_53,  0x0106U);
    lcd_write_reg(LCD_REG_54,  0x0B02U);
    lcd_write_reg(LCD_REG_55,  0x0302U);
    lcd_write_reg(LCD_REG_56,  0x0707U);
    lcd_write_reg(LCD_REG_57,  0x0007U);
    lcd_write_reg(LCD_REG_60,  0x0600U);
    lcd_write_reg(LCD_REG_61,  0x020BU);
    lcd_write_reg(LCD_REG_80,  0x0000U);
    lcd_write_reg(LCD_REG_81,  0x00EFU);
    lcd_write_reg(LCD_REG_82,  0x0000U);
    lcd_write_reg(LCD_REG_83,  0x013FU);
    lcd_write_reg(LCD_REG_96,  0x2700U);
    lcd_write_reg(LCD_REG_97,  0x0001U);
    lcd_write_reg(LCD_REG_106, 0x0000U);
    lcd_write_reg(LCD_REG_128, 0x0000U);
    lcd_write_reg(LCD_REG_129, 0x0000U);
    lcd_write_reg(LCD_REG_130, 0x0000U);
    lcd_write_reg(LCD_REG_131, 0x0000U);
    lcd_write_reg(LCD_REG_132, 0x0000U);
    lcd_write_reg(LCD_REG_133, 0x0000U);
    lcd_write_reg(LCD_REG_144, 0x0010U);
    lcd_write_reg(LCD_REG_146, 0x0000U);
    lcd_write_reg(LCD_REG_147, 0x0003U);
    lcd_write_reg(LCD_REG_149, 0x0110U);
    lcd_write_reg(LCD_REG_151, 0x0000U);
    lcd_write_reg(LCD_REG_152, 0x0000U);
    lcd_write_reg(LCD_REG_3,   TFT_LCD_ENTRY_MODE);
    lcd_write_reg(LCD_REG_7,   0x0173U);
}

static void lcd_init_5408(void)
{
    lcd_write_reg(LCD_REG_1,   0x0100U);
    lcd_write_reg(LCD_REG_2,   0x0700U);
    lcd_write_reg(LCD_REG_3,   TFT_LCD_ENTRY_MODE);
    lcd_write_reg(LCD_REG_4,   0x0000U);
    lcd_write_reg(LCD_REG_8,   0x0202U);
    lcd_write_reg(LCD_REG_9,   0x0000U);
    lcd_write_reg(LCD_REG_10,  0x0000U);
    lcd_write_reg(LCD_REG_12,  0x0000U);
    lcd_write_reg(LCD_REG_13,  0x0000U);
    lcd_write_reg(LCD_REG_15,  0x0000U);
    lcd_write_reg(LCD_REG_16,  0x0000U);
    lcd_write_reg(LCD_REG_17,  0x0000U);
    lcd_write_reg(LCD_REG_18,  0x0000U);
    lcd_write_reg(LCD_REG_19,  0x0000U);
    lcd_delay(20U);
    lcd_write_reg(LCD_REG_17,  0x0007U);
    lcd_delay(5U);
    lcd_write_reg(LCD_REG_16,  0x12B0U);
    lcd_delay(5U);
    lcd_write_reg(LCD_REG_18,  0x01BDU);
    lcd_delay(5U);
    lcd_write_reg(LCD_REG_19,  0x1400U);
    lcd_write_reg(LCD_REG_41,  0x000EU);
    lcd_delay(5U);
    lcd_write_reg(LCD_REG_32,  0x0000U);
    lcd_write_reg(LCD_REG_33,  0x013FU);
    lcd_write_reg(LCD_REG_48,  0x0B0DU);
    lcd_write_reg(LCD_REG_49,  0x1923U);
    lcd_write_reg(LCD_REG_50,  0x1C26U);
    lcd_write_reg(LCD_REG_53,  0x0D0BU);
    lcd_write_reg(LCD_REG_54,  0x1006U);
    lcd_write_reg(LCD_REG_55,  0x0610U);
    lcd_write_reg(LCD_REG_56,  0x0706U);
    lcd_write_reg(LCD_REG_57,  0x0304U);
    lcd_write_reg(LCD_REG_58,  0x0E05U);
    lcd_write_reg(LCD_REG_59,  0x0E01U);
    lcd_write_reg(LCD_REG_60,  0x010EU);
    lcd_write_reg(LCD_REG_61,  0x050EU);
    lcd_write_reg(LCD_REG_62,  0x0403U);
    lcd_write_reg(LCD_REG_63,  0x0607U);
    lcd_write_reg(LCD_REG_80,  0x0000U);
    lcd_write_reg(LCD_REG_81,  0x00EFU);
    lcd_write_reg(LCD_REG_82,  0x0000U);
    lcd_write_reg(LCD_REG_83,  0x013FU);
    lcd_write_reg(LCD_REG_96,  0xA700U);
    lcd_write_reg(LCD_REG_97,  0x0001U);
    lcd_write_reg(LCD_REG_106, 0x0000U);
    lcd_write_reg(LCD_REG_128, 0x0000U);
    lcd_write_reg(LCD_REG_129, 0x0000U);
    lcd_write_reg(LCD_REG_130, 0x0000U);
    lcd_write_reg(LCD_REG_131, 0x0000U);
    lcd_write_reg(LCD_REG_132, 0x0000U);
    lcd_write_reg(LCD_REG_133, 0x0000U);
    lcd_write_reg(LCD_REG_144, 0x0010U);
    lcd_write_reg(LCD_REG_146, 0x0000U);
    lcd_write_reg(LCD_REG_147, 0x0003U);
    lcd_write_reg(LCD_REG_149, 0x0110U);
    lcd_write_reg(LCD_REG_151, 0x0000U);
    lcd_write_reg(LCD_REG_152, 0x0000U);
    lcd_write_reg(LCD_REG_3,   TFT_LCD_ENTRY_MODE);
    lcd_write_reg(LCD_REG_7,   0x0112U);
}
#endif

void bsp_oled_init(void)
{
    unsigned int i;

    g_refresh_pending = 0U;
    g_refresh_line = 0U;

#if defined(USE_HAL_DRIVER)
    GPIO_InitTypeDef gpio = {0};

    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_AFIO_CLK_ENABLE();
    __HAL_AFIO_REMAP_SWJ_NOJTAG();

    lcd_data_mode(GPIO_MODE_OUTPUT_PP);

    gpio.Pin = TFT_PIN_RD | TFT_PIN_WR | TFT_PIN_RS | TFT_PIN_CS | TFT_PIN_BL;
    gpio.Mode = GPIO_MODE_OUTPUT_PP;
    gpio.Pull = GPIO_NOPULL;
    gpio.Speed = TFT_GPIO_SPEED;
    HAL_GPIO_Init(TFT_CTRL_PORT, &gpio);

    lcd_ctrl_write(TFT_PIN_CS, 0);
    lcd_ctrl_write(TFT_PIN_RD, 1);
    lcd_ctrl_write(TFT_PIN_WR, 1);
    lcd_ctrl_write(TFT_PIN_RS, 1);
    lcd_ctrl_write(TFT_PIN_BL, 0);

    lcd_delay(5U);
    g_lcd_id = lcd_read_reg(LCD_REG_0);
    if (g_lcd_id == 0x5408U)
    {
        lcd_init_5408();
    }
    else
    {
        lcd_init_932x();
    }
    lcd_fill_rect(0U, 0U, TFT_PHYSICAL_WIDTH, TFT_PHYSICAL_HEIGHT, OLED_COLOR_BG);
    lcd_ctrl_write(TFT_PIN_BL, 1);
    g_tg_canvas.width = BSP_OLED_WIDTH;
    g_tg_canvas.height = BSP_OLED_HEIGHT;
    g_tg_canvas.fill_rect = tg_lcd_fill_rect;
    g_tg_canvas.ctx = 0;
    g_lcd_ready = 1U;
#endif

    for (i = 0U; i < BSP_OLED_LINE_COUNT; ++i)
    {
        g_lines[i][0] = '\0';
        g_drawn_lines[i][0] = '\0';
    }

    g_dirty_mask = (uint8_t)((1U << BSP_OLED_LINE_COUNT) - 1U);
}

void bsp_oled_clear(void)
{
    unsigned int i;

    for (i = 0U; i < BSP_OLED_LINE_COUNT; ++i)
    {
        g_lines[i][0] = '\0';
        g_drawn_lines[i][0] = '\0';
    }

    g_dirty_mask = (uint8_t)((1U << BSP_OLED_LINE_COUNT) - 1U);

#if defined(USE_HAL_DRIVER)
    if (g_lcd_ready != 0U)
    {
        lcd_fill_rect(0U, 0U, TFT_PHYSICAL_WIDTH, TFT_PHYSICAL_HEIGHT, OLED_COLOR_BG);
    }
#endif
}

void bsp_oled_fill_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color)
{
#if defined(USE_HAL_DRIVER)
    if (g_lcd_ready == 0U)
    {
        return;
    }

    tg_fill_rect(&g_tg_canvas, (int)x, (int)y, (int)w, (int)h, color);
#else
    (void)x;
    (void)y;
    (void)w;
    (void)h;
    (void)color;
#endif
}

void bsp_oled_draw_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color)
{
#if defined(USE_HAL_DRIVER)
    if (g_lcd_ready == 0U)
    {
        return;
    }

    tg_draw_rect(&g_tg_canvas, (int)x, (int)y, (int)w, (int)h, color);
#else
    (void)x;
    (void)y;
    (void)w;
    (void)h;
    (void)color;
#endif
}

void bsp_oled_fill_round_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t radius, uint16_t color)
{
#if defined(USE_HAL_DRIVER)
    if (g_lcd_ready == 0U)
    {
        return;
    }

    tg_fill_round_rect(&g_tg_canvas, (int)x, (int)y, (int)w, (int)h, (int)radius, color);
#else
    (void)x;
    (void)y;
    (void)w;
    (void)h;
    (void)radius;
    (void)color;
#endif
}

void bsp_oled_draw_line(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t color)
{
#if defined(USE_HAL_DRIVER)
    if (g_lcd_ready == 0U)
    {
        return;
    }

    tg_draw_line(&g_tg_canvas, (int)x0, (int)y0, (int)x1, (int)y1, color);
#else
    (void)x0;
    (void)y0;
    (void)x1;
    (void)y1;
    (void)color;
#endif
}

void bsp_oled_draw_circle(uint16_t cx, uint16_t cy, uint16_t radius, uint16_t color)
{
#if defined(USE_HAL_DRIVER)
    if (g_lcd_ready == 0U)
    {
        return;
    }

    tg_draw_circle(&g_tg_canvas, (int)cx, (int)cy, (int)radius, color);
#else
    (void)cx;
    (void)cy;
    (void)radius;
    (void)color;
#endif
}

void bsp_oled_draw_text_xy(uint16_t x, uint16_t y, const char *text, uint8_t scale, uint16_t color)
{
#if defined(USE_HAL_DRIVER)
    if ((g_lcd_ready == 0U) || (text == 0))
    {
        return;
    }

    /* UI coordinate system uses left-to-right text progression on X axis. */
    lcd_draw_text_dir(x, y, text, scale, color, 0U);
#else
    (void)x;
    (void)y;
    (void)text;
    (void)scale;
    (void)color;
#endif
}

void bsp_oled_write_area_rgb565(uint16_t x,
                                uint16_t y,
                                uint16_t w,
                                uint16_t h,
                                const uint16_t *pixels)
{
#if defined(USE_HAL_DRIVER)
    uint16_t clipped_w;
    uint16_t clipped_h;
    uint32_t count;

    if ((g_lcd_ready == 0U) || (pixels == 0) || (w == 0U) || (h == 0U))
    {
        return;
    }

    if ((x >= BSP_OLED_WIDTH) || (y >= BSP_OLED_HEIGHT))
    {
        return;
    }

    clipped_w = w;
    clipped_h = h;
    if ((uint32_t)x + (uint32_t)clipped_w > (uint32_t)BSP_OLED_WIDTH)
    {
        clipped_w = (uint16_t)(BSP_OLED_WIDTH - x);
    }
    if ((uint32_t)y + (uint32_t)clipped_h > (uint32_t)BSP_OLED_HEIGHT)
    {
        clipped_h = (uint16_t)(BSP_OLED_HEIGHT - y);
    }

    lcd_set_window(x,
                   y,
                   (uint16_t)(x + clipped_w - 1U),
                   (uint16_t)(y + clipped_h - 1U));
    lcd_prepare_write_ram();

    for (count = 0UL; count < ((uint32_t)clipped_w * (uint32_t)clipped_h); ++count)
    {
        lcd_write_data(pixels[count]);
    }
#else
    (void)x;
    (void)y;
    (void)w;
    (void)h;
    (void)pixels;
#endif
}

void bsp_oled_draw_text(uint8_t line, const char *text)
{
    size_t old_len;

    if ((line >= BSP_OLED_LINE_COUNT) || (text == 0))
    {
        return;
    }

    old_len = strlen(g_lines[line]);
    strncpy(g_lines[line], text, BSP_OLED_LINE_CHARS);
    g_lines[line][BSP_OLED_LINE_CHARS] = '\0';

    if (strncmp(g_lines[line], g_drawn_lines[line], BSP_OLED_LINE_CHARS + 1U) != 0)
    {
        g_dirty_mask = (uint8_t)(g_dirty_mask | (uint8_t)(1U << line));
    }
    else if ((old_len == 0U) && (g_lines[line][0] == '\0'))
    {
        g_dirty_mask = (uint8_t)(g_dirty_mask & (uint8_t)(~(uint8_t)(1U << line)));
    }
}

void bsp_oled_refresh(void)
{
    if (g_dirty_mask == 0U)
    {
        g_refresh_pending = 0U;
        return;
    }

    oled_build_frame();
    g_refresh_line = 0U;
    g_refresh_pending = 1U;
}

int bsp_oled_process(void)
{
#if defined(USE_HAL_DRIVER)
    if ((!g_refresh_pending) || (!g_lcd_ready))
    {
        return 0;
    }

    while (g_refresh_line < BSP_OLED_LINE_COUNT)
    {
        uint8_t line_mask = (uint8_t)(1U << g_refresh_line);

        if ((g_dirty_mask & line_mask) != 0U)
        {
            lcd_draw_text_line(g_refresh_line);
            strncpy(g_drawn_lines[g_refresh_line], g_lines[g_refresh_line], BSP_OLED_LINE_CHARS + 1U);
            g_dirty_mask = (uint8_t)(g_dirty_mask & (uint8_t)(~line_mask));
            g_refresh_line++;
            break;
        }

        g_refresh_line++;
    }

    if (g_refresh_line >= BSP_OLED_LINE_COUNT)
    {
        g_refresh_pending = 0U;
    }
    return (g_refresh_pending != 0U) ? 1 : 0;
#else
    if (!g_refresh_pending)
    {
        return 0;
    }

    g_refresh_line++;
    if (g_refresh_line >= BSP_OLED_LINE_COUNT)
    {
        g_refresh_pending = 0U;
    }

    return (g_refresh_pending != 0U) ? 1 : 0;
#endif
}

int bsp_oled_is_busy(void)
{
    return (g_refresh_pending != 0U) ? 1 : 0;
}

const char *bsp_oled_mock_get_line(uint8_t line)
{
    if (line >= BSP_OLED_LINE_COUNT)
    {
        return "";
    }

    return g_lines[line];
}



#endif



