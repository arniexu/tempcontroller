#include "bsp_oled.h"

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