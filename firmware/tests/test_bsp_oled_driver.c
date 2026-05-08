#include "bsp_lcd_8080.h"

#include <string.h>

#include "test_framework.h"

int test_bsp_oled_driver_run(void)
{
    char long_text[BSP_OLED_LINE_CHARS + 8U];
    unsigned int i;

    for (i = 0U; i < (unsigned int)(sizeof(long_text) - 1U); ++i)
    {
        long_text[i] = 'A';
    }
    long_text[sizeof(long_text) - 1U] = '\0';

    bsp_oled_init();

    for (i = 0U; i < BSP_OLED_LINE_COUNT; ++i)
    {
        TEST_ASSERT_TRUE(bsp_oled_mock_get_line((uint8_t)i)[0] == '\0');
    }
    TEST_ASSERT_EQ_INT(bsp_oled_is_busy(), 0);

    bsp_oled_draw_text(0U, "HELLO");
    TEST_ASSERT_TRUE(strcmp(bsp_oled_mock_get_line(0U), "HELLO") == 0);

    bsp_oled_draw_text(1U, long_text);
    TEST_ASSERT_EQ_INT((int)strlen(bsp_oled_mock_get_line(1U)), (int)BSP_OLED_LINE_CHARS);

    bsp_oled_draw_text(BSP_OLED_LINE_COUNT, "IGNORED");
    TEST_ASSERT_TRUE(bsp_oled_mock_get_line(BSP_OLED_LINE_COUNT)[0] == '\0');

    bsp_oled_draw_text(2U, 0);
    TEST_ASSERT_TRUE(bsp_oled_mock_get_line(2U)[0] == '\0');

    bsp_oled_refresh();
    TEST_ASSERT_EQ_INT(bsp_oled_is_busy(), 1);

    for (i = 0U; i < (BSP_OLED_LINE_COUNT - 1U); ++i)
    {
        TEST_ASSERT_EQ_INT(bsp_oled_process(), 1);
        TEST_ASSERT_EQ_INT(bsp_oled_is_busy(), 1);
    }

    TEST_ASSERT_EQ_INT(bsp_oled_process(), 0);
    TEST_ASSERT_EQ_INT(bsp_oled_is_busy(), 0);

    bsp_oled_clear();
    for (i = 0U; i < BSP_OLED_LINE_COUNT; ++i)
    {
        TEST_ASSERT_TRUE(bsp_oled_mock_get_line((uint8_t)i)[0] == '\0');
    }

    return 0;
}

int test_bsp_lcd_8080_driver_run(void)
{
    char long_text[BSP_LCD_LINE_CHARS + 8U];
    unsigned int i;

    for (i = 0U; i < (unsigned int)(sizeof(long_text) - 1U); ++i)
    {
        long_text[i] = 'L';
    }
    long_text[sizeof(long_text) - 1U] = '\0';

    /* Dedicated checks for LCD 8080 profile. */
    TEST_ASSERT_EQ_INT(BSP_LCD_WIDTH, 240);
    TEST_ASSERT_EQ_INT(BSP_LCD_HEIGHT, 320);
    TEST_ASSERT_EQ_INT(BSP_LCD_LINE_COUNT, 4);

    bsp_lcd_init();
    TEST_ASSERT_EQ_INT(bsp_lcd_mock_draw_op_count(), 0);

    for (i = 0U; i < BSP_LCD_LINE_COUNT; ++i)
    {
        TEST_ASSERT_TRUE(bsp_lcd_mock_get_line((uint8_t)i)[0] == '\0');
    }
    TEST_ASSERT_EQ_INT(bsp_lcd_is_busy(), 0);

    bsp_lcd_draw_text(0U, "LCD8080");
    TEST_ASSERT_TRUE(strcmp(bsp_lcd_mock_get_line(0U), "LCD8080") == 0);

    bsp_lcd_draw_text(1U, long_text);
    TEST_ASSERT_EQ_INT((int)strlen(bsp_lcd_mock_get_line(1U)), (int)BSP_LCD_LINE_CHARS);

    bsp_lcd_draw_text(BSP_LCD_LINE_COUNT, "IGNORED");
    TEST_ASSERT_TRUE(bsp_lcd_mock_get_line(BSP_LCD_LINE_COUNT)[0] == '\0');

    bsp_lcd_draw_text(2U, 0);
    TEST_ASSERT_TRUE(bsp_lcd_mock_get_line(2U)[0] == '\0');

    bsp_lcd_refresh();
    TEST_ASSERT_EQ_INT(bsp_lcd_is_busy(), 1);

    for (i = 0U; i < (BSP_LCD_LINE_COUNT - 1U); ++i)
    {
        TEST_ASSERT_EQ_INT(bsp_lcd_process(), 1);
        TEST_ASSERT_EQ_INT(bsp_lcd_is_busy(), 1);
    }

    TEST_ASSERT_EQ_INT(bsp_lcd_process(), 0);
    TEST_ASSERT_EQ_INT(bsp_lcd_is_busy(), 0);

    /* Draw APIs should be callable on host path even if HAL render path is disabled. */
    bsp_lcd_fill_rect(0U, 0U, 10U, 10U, BSP_LCD_COLOR_WHITE);
    bsp_lcd_draw_rect(2U, 2U, 8U, 8U, BSP_LCD_COLOR_WHITE);
    bsp_lcd_fill_round_rect(4U, 4U, 12U, 10U, 2U, BSP_LCD_COLOR_WHITE);
    bsp_lcd_draw_line(0U, 0U, 6U, 6U, BSP_LCD_COLOR_WHITE);
    bsp_lcd_draw_circle(8U, 8U, 4U, BSP_LCD_COLOR_WHITE);
    bsp_lcd_draw_text_xy(3U, 3U, "OK", 1U, BSP_LCD_COLOR_WHITE);

    {
        const uint16_t pixels[4] = {0x0000U, 0xFFFFU, 0xF800U, 0x07E0U};
        bsp_lcd_write_area_rgb565(0U, 0U, 2U, 2U, pixels);
        bsp_lcd_write_area_rgb565(0U, 0U, 0U, 2U, pixels);
        bsp_lcd_write_area_rgb565(0U, 0U, 2U, 0U, pixels);
        bsp_lcd_write_area_rgb565(0U, 0U, 2U, 2U, 0);
    }

    bsp_lcd_clear();
    for (i = 0U; i < BSP_LCD_LINE_COUNT; ++i)
    {
        TEST_ASSERT_TRUE(bsp_lcd_mock_get_line((uint8_t)i)[0] == '\0');
    }

    return 0;
}