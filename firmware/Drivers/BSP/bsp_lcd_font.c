#include "bsp_lcd_font.h"

#include <string.h>

typedef struct
{
    uint16_t codepoint;
    uint16_t rows[LCD_FONT_CJK_HEIGHT_16X16];
} lcd_cjk_glyph_t;

static const lcd_cjk_glyph_t g_cjk_16x16[] = {
    {0x6E29U, {0x0100U, 0x1100U, 0x1FF0U, 0x1100U, 0x17F0U, 0x1450U, 0x27F8U, 0x2450U,
               0x2FF8U, 0x2450U, 0x2450U, 0x4450U, 0x47F0U, 0x8000U, 0x0000U, 0x0000U}},
    {0x5EA6U, {0x07F0U, 0x0400U, 0x7FC0U, 0x0440U, 0x07F0U, 0x4448U, 0x7FF8U, 0x4448U,
               0x4FF8U, 0x4888U, 0x5088U, 0x6888U, 0x47F0U, 0x0000U, 0x0000U, 0x0000U}},
    {0x6C34U, {0x0100U, 0x0120U, 0x0140U, 0x0180U, 0x1FF0U, 0x0300U, 0x0500U, 0x08C0U,
               0x1040U, 0x2020U, 0x4010U, 0x8008U, 0x0000U, 0x0000U, 0x0000U, 0x0000U}},
    {0x70EDU, {0x2108U, 0x3DF8U, 0x2108U, 0x3DF8U, 0x2108U, 0x7FF8U, 0x4448U, 0x4FF8U,
               0x4888U, 0x7FF8U, 0x4448U, 0x4208U, 0x81F0U, 0x0000U, 0x0000U, 0x0000U}},
    {0x8BBEU, {0x0108U, 0x7DF8U, 0x4448U, 0x7DF8U, 0x4448U, 0x0110U, 0x7FF8U, 0x0100U,
               0x1FF0U, 0x1100U, 0x1FF0U, 0x1100U, 0x1FF0U, 0x0000U, 0x0000U, 0x0000U}},
    {0x5B9AU, {0x0200U, 0x3FF0U, 0x2220U, 0x3FF0U, 0x0220U, 0x1FF0U, 0x1100U, 0x1FF0U,
               0x0100U, 0x7FF8U, 0x0900U, 0x1100U, 0x60F8U, 0x0000U, 0x0000U, 0x0000U}}
};

void bsp_lcd_font_get_ascii_5x7(char c, uint8_t out[5], uint8_t *width, uint8_t *height)
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
    }
    else
    {
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

    if (width != 0)
    {
        *width = LCD_FONT_ASCII_WIDTH_5X7;
    }
    if (height != 0)
    {
        *height = LCD_FONT_ASCII_HEIGHT_5X7;
    }
}

int bsp_lcd_font_utf8_decode_one(const char *text, uint16_t *codepoint, uint8_t *consumed)
{
    uint8_t c0;

    if ((text == 0) || (codepoint == 0) || (consumed == 0))
    {
        return 0;
    }

    c0 = (uint8_t)text[0];
    if (c0 < 0x80U)
    {
        *codepoint = c0;
        *consumed = 1U;
        return 1;
    }

    if ((c0 & 0xF0U) == 0xE0U)
    {
        uint8_t c1 = (uint8_t)text[1];
        uint8_t c2 = (uint8_t)text[2];

        if (((c1 & 0xC0U) == 0x80U) && ((c2 & 0xC0U) == 0x80U))
        {
            *codepoint = (uint16_t)(((uint16_t)(c0 & 0x0FU) << 12) |
                                    ((uint16_t)(c1 & 0x3FU) << 6) |
                                    (uint16_t)(c2 & 0x3FU));
            *consumed = 3U;
            return 1;
        }
    }

    *codepoint = (uint16_t)'?';
    *consumed = 1U;
    return 1;
}

int bsp_lcd_font_get_cjk_16x16(uint16_t codepoint, uint16_t out[LCD_FONT_CJK_HEIGHT_16X16])
{
    uint16_t i;

    if (out == 0)
    {
        return 0;
    }

    for (i = 0U; i < (uint16_t)(sizeof(g_cjk_16x16) / sizeof(g_cjk_16x16[0])); ++i)
    {
        if (g_cjk_16x16[i].codepoint == codepoint)
        {
            memcpy(out, g_cjk_16x16[i].rows, sizeof(g_cjk_16x16[i].rows));
            return 1;
        }
    }

    return 0;
}
