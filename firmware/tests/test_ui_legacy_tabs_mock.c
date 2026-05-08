#include "bsp_lcd_8080.h"

#include <string.h>

#include "test_framework.h"

#define TAB_BAR_X           (24U)
#define TAB_BAR_Y           (84U)
#define TAB_BAR_W           (192U)
#define TAB_BAR_H           (30U)
#define TAB_W               (64U)
#define TAB_TXT_Y           (93U)
#define TAB0_TXT_X          (36U)
#define TAB1_TXT_X          (100U)
#define TAB2_TXT_X          (164U)

#define COLOR_BG            (0x0000U)
#define COLOR_PANEL         (0x10A2U)
#define COLOR_BORDER        (0x18C3U)
#define COLOR_TEXT          (0xC638U)
#define COLOR_ACTIVE        (0x0596U)

static void draw_pid_tabs_mock(unsigned int selected)
{
    uint16_t active_x = (uint16_t)(TAB_BAR_X + ((selected % 3U) * TAB_W));

    bsp_oled_fill_rect(TAB_BAR_X, TAB_BAR_Y, TAB_BAR_W, TAB_BAR_H, COLOR_BG);
    bsp_oled_draw_rect(TAB_BAR_X, TAB_BAR_Y, TAB_BAR_W, TAB_BAR_H, COLOR_BORDER);
    bsp_oled_fill_rect(active_x, TAB_BAR_Y, TAB_W, TAB_BAR_H, COLOR_ACTIVE);
    bsp_oled_draw_line((uint16_t)(TAB_BAR_X + TAB_W), TAB_BAR_Y,
                       (uint16_t)(TAB_BAR_X + TAB_W), (uint16_t)(TAB_BAR_Y + TAB_BAR_H - 1U), COLOR_BORDER);
    bsp_oled_draw_line((uint16_t)(TAB_BAR_X + (2U * TAB_W)), TAB_BAR_Y,
                       (uint16_t)(TAB_BAR_X + (2U * TAB_W)), (uint16_t)(TAB_BAR_Y + TAB_BAR_H - 1U), COLOR_BORDER);

    bsp_oled_draw_text_xy(TAB0_TXT_X, TAB_TXT_Y, "NOR", 1U, COLOR_TEXT);
    bsp_oled_draw_text_xy(TAB1_TXT_X, TAB_TXT_Y, "GUD", 1U, COLOR_TEXT);
    bsp_oled_draw_text_xy(TAB2_TXT_X, TAB_TXT_Y, "EXP", 1U, COLOR_TEXT);
}

static int assert_op_fill_rect(uint16_t idx, uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color)
{
    bsp_oled_mock_op_t op;
    TEST_ASSERT_TRUE(bsp_oled_mock_get_draw_op(idx, &op) == 1);
    TEST_ASSERT_EQ_INT(op.kind, BSP_OLED_OP_FILL_RECT);
    TEST_ASSERT_EQ_INT(op.x0, x);
    TEST_ASSERT_EQ_INT(op.y0, y);
    TEST_ASSERT_EQ_INT(op.x1, w);
    TEST_ASSERT_EQ_INT(op.y1, h);
    TEST_ASSERT_EQ_INT(op.color, color);
    return 0;
}

static int assert_op_draw_rect(uint16_t idx, uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color)
{
    bsp_oled_mock_op_t op;
    TEST_ASSERT_TRUE(bsp_oled_mock_get_draw_op(idx, &op) == 1);
    TEST_ASSERT_EQ_INT(op.kind, BSP_OLED_OP_DRAW_RECT);
    TEST_ASSERT_EQ_INT(op.x0, x);
    TEST_ASSERT_EQ_INT(op.y0, y);
    TEST_ASSERT_EQ_INT(op.x1, w);
    TEST_ASSERT_EQ_INT(op.y1, h);
    TEST_ASSERT_EQ_INT(op.color, color);
    return 0;
}

static int assert_op_draw_line(uint16_t idx, uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t color)
{
    bsp_oled_mock_op_t op;
    TEST_ASSERT_TRUE(bsp_oled_mock_get_draw_op(idx, &op) == 1);
    TEST_ASSERT_EQ_INT(op.kind, BSP_OLED_OP_DRAW_LINE);
    TEST_ASSERT_EQ_INT(op.x0, x0);
    TEST_ASSERT_EQ_INT(op.y0, y0);
    TEST_ASSERT_EQ_INT(op.x1, x1);
    TEST_ASSERT_EQ_INT(op.y1, y1);
    TEST_ASSERT_EQ_INT(op.color, color);
    return 0;
}

static int assert_op_draw_text(uint16_t idx, uint16_t x, uint16_t y, const char *text, uint8_t scale, uint16_t color)
{
    bsp_oled_mock_op_t op;
    TEST_ASSERT_TRUE(bsp_oled_mock_get_draw_op(idx, &op) == 1);
    TEST_ASSERT_EQ_INT(op.kind, BSP_OLED_OP_DRAW_TEXT_XY);
    TEST_ASSERT_EQ_INT(op.x0, x);
    TEST_ASSERT_EQ_INT(op.y0, y);
    TEST_ASSERT_EQ_INT(op.scale, scale);
    TEST_ASSERT_EQ_INT(op.color, color);
    TEST_ASSERT_TRUE(strcmp(op.text, text) == 0);
    return 0;
}

int test_ui_legacy_tabs_mock_run(void)
{
    unsigned int selected;

    for (selected = 0U; selected < 3U; ++selected)
    {
        uint16_t active_x = (uint16_t)(TAB_BAR_X + (selected * TAB_W));

        bsp_oled_mock_reset_draw_ops();
        draw_pid_tabs_mock(selected);

        TEST_ASSERT_EQ_INT(bsp_oled_mock_draw_op_count(), 8);

        TEST_ASSERT_EQ_INT(assert_op_fill_rect(0U, TAB_BAR_X, TAB_BAR_Y, TAB_BAR_W, TAB_BAR_H, COLOR_BG), 0);
        TEST_ASSERT_EQ_INT(assert_op_draw_rect(1U, TAB_BAR_X, TAB_BAR_Y, TAB_BAR_W, TAB_BAR_H, COLOR_BORDER), 0);
        TEST_ASSERT_EQ_INT(assert_op_fill_rect(2U, active_x, TAB_BAR_Y, TAB_W, TAB_BAR_H, COLOR_ACTIVE), 0);
        TEST_ASSERT_EQ_INT(assert_op_draw_line(3U, (uint16_t)(TAB_BAR_X + TAB_W), TAB_BAR_Y,
                                               (uint16_t)(TAB_BAR_X + TAB_W), (uint16_t)(TAB_BAR_Y + TAB_BAR_H - 1U),
                                               COLOR_BORDER),
                           0);
        TEST_ASSERT_EQ_INT(assert_op_draw_line(4U, (uint16_t)(TAB_BAR_X + (2U * TAB_W)), TAB_BAR_Y,
                                               (uint16_t)(TAB_BAR_X + (2U * TAB_W)),
                                               (uint16_t)(TAB_BAR_Y + TAB_BAR_H - 1U),
                                               COLOR_BORDER),
                           0);
        TEST_ASSERT_EQ_INT(assert_op_draw_text(5U, TAB0_TXT_X, TAB_TXT_Y, "NOR", 1U, COLOR_TEXT), 0);
        TEST_ASSERT_EQ_INT(assert_op_draw_text(6U, TAB1_TXT_X, TAB_TXT_Y, "GUD", 1U, COLOR_TEXT), 0);
        TEST_ASSERT_EQ_INT(assert_op_draw_text(7U, TAB2_TXT_X, TAB_TXT_Y, "EXP", 1U, COLOR_TEXT), 0);
    }

    return 0;
}
