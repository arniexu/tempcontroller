#include "oled96x96_i2c.h"

#include <stdio.h>
#include <string.h>

#define OLED_WIDTH 96U
#define OLED_HEIGHT 96U
#define OLED_PAGE_COUNT (OLED_HEIGHT / 8U)

#define OLED_CTRL_CMD 0x00U
#define OLED_CTRL_DATA 0x40U

#define OLED_DATA_CHUNK 16U
#define OLED_FRAME_SIZE (1U + OLED_DATA_CHUNK)

#define OLED_DEFAULT_I2C_TIMEOUT_MS 100U
#define OLED_DEFAULT_RETRIES 2U

static bool oled_is_valid_dev(const oled96x96_t *dev)
{
    return (dev != NULL) && (dev->hi2c != NULL);
}

static HAL_StatusTypeDef oled_tx(oled96x96_t *dev, uint8_t ctrl, const uint8_t *data, uint16_t len)
{
    if (!oled_is_valid_dev(dev) || ((len > 0U) && (data == NULL))) {
        return HAL_ERROR;
    }

    uint8_t frame[OLED_FRAME_SIZE];
    frame[0] = ctrl;

    uint16_t offset = 0U;
    while (offset < len) {
        uint16_t chunk = (uint16_t)((len - offset) > OLED_DATA_CHUNK ? OLED_DATA_CHUNK : (len - offset));
        memcpy(&frame[1], &data[offset], chunk);

        HAL_StatusTypeDef rc = HAL_ERROR;
        for (uint8_t attempt = 0U; attempt <= dev->config.io_retries; ++attempt) {
            rc = HAL_I2C_Master_Transmit(
                dev->hi2c,
                (uint16_t)(dev->addr_7bit << 1),
                frame,
                (uint16_t)(1U + chunk),
                dev->config.i2c_timeout_ms);
            if (rc == HAL_OK) {
                break;
            }
            HAL_Delay(1U);
        }

        if (rc != HAL_OK) {
            return rc;
        }

        offset += chunk;
    }

    return HAL_OK;
}

static HAL_StatusTypeDef oled_send_cmd1(oled96x96_t *dev, uint8_t cmd)
{
    return oled_tx(dev, OLED_CTRL_CMD, &cmd, 1U);
}

static HAL_StatusTypeDef oled_set_window(oled96x96_t *dev, uint8_t col_start, uint8_t col_end, uint8_t page_start, uint8_t page_end)
{
    HAL_StatusTypeDef rc = oled_send_cmd1(dev, 0x21U);
    if (rc != HAL_OK) {
        return rc;
    }
    rc = oled_send_cmd1(dev, col_start);
    if (rc != HAL_OK) {
        return rc;
    }
    rc = oled_send_cmd1(dev, col_end);
    if (rc != HAL_OK) {
        return rc;
    }
    rc = oled_send_cmd1(dev, 0x22U);
    if (rc != HAL_OK) {
        return rc;
    }
    rc = oled_send_cmd1(dev, page_start);
    if (rc != HAL_OK) {
        return rc;
    }
    return oled_send_cmd1(dev, page_end);
}

void oled96x96_get_default_config(oled96x96_config_t *config)
{
    if (config == NULL) {
        return;
    }
    config->i2c_timeout_ms = OLED_DEFAULT_I2C_TIMEOUT_MS;
    config->io_retries = OLED_DEFAULT_RETRIES;
}

HAL_StatusTypeDef oled96x96_init(oled96x96_t *dev)
{
    if (!oled_is_valid_dev(dev)) {
        return HAL_ERROR;
    }

    if (dev->config.i2c_timeout_ms == 0U) {
        oled96x96_get_default_config(&dev->config);
    }

    const uint8_t init_seq[] = {
        0xAE,       /* display off */
        0xD5, 0x80, /* clock divide */
        0xA8, 0x5F, /* multiplex = 95 */
        0xD3, 0x00, /* display offset */
        0x40,       /* start line */
        0x8D, 0x14, /* charge pump on */
        0x20, 0x00, /* horizontal addressing mode */
        0xA1,       /* segment remap */
        0xC8,       /* COM scan direction */
        0xDA, 0x12, /* COM pins */
        0x81, 0x7F, /* contrast */
        0xD9, 0xF1, /* pre-charge */
        0xDB, 0x40, /* VCOMH */
        0xA4,       /* display follows RAM */
        0xA6,       /* normal display */
        0x2E,       /* stop scroll */
        0xAF        /* display on */
    };

    HAL_StatusTypeDef rc = oled_tx(dev, OLED_CTRL_CMD, init_seq, (uint16_t)sizeof(init_seq));
    if (rc != HAL_OK) {
        dev->initialized = false;
        return rc;
    }

    memset(dev->framebuffer, 0x00, sizeof(dev->framebuffer));
    rc = oled96x96_flush(dev);
    dev->initialized = (rc == HAL_OK);
    return rc;
}

HAL_StatusTypeDef oled96x96_fill(oled96x96_t *dev, uint8_t value)
{
    if (!oled_is_valid_dev(dev) || !dev->initialized) {
        return HAL_ERROR;
    }
    memset(dev->framebuffer, value, sizeof(dev->framebuffer));
    return oled96x96_flush(dev);
}

HAL_StatusTypeDef oled96x96_flush(oled96x96_t *dev)
{
    if (!oled_is_valid_dev(dev)) {
        return HAL_ERROR;
    }

    HAL_StatusTypeDef rc = oled_set_window(dev, 0U, (uint8_t)(OLED_WIDTH - 1U), 0U, (uint8_t)(OLED_PAGE_COUNT - 1U));
    if (rc != HAL_OK) {
        return rc;
    }
    return oled_tx(dev, OLED_CTRL_DATA, dev->framebuffer, (uint16_t)sizeof(dev->framebuffer));
}

static void oled_draw_pixel(oled96x96_t *dev, uint8_t x, uint8_t y, bool on)
{
    if ((x >= OLED_WIDTH) || (y >= OLED_HEIGHT)) {
        return;
    }

    uint16_t index = (uint16_t)x + (uint16_t)((uint16_t)(y / 8U) * OLED_WIDTH);
    uint8_t mask = (uint8_t)(1U << (y & 0x07U));
    if (on) {
        dev->framebuffer[index] |= mask;
    } else {
        dev->framebuffer[index] &= (uint8_t)~mask;
    }
}

static const uint8_t *oled_get_glyph(char c)
{
    static const uint8_t glyph_space[5] = {0x00, 0x00, 0x00, 0x00, 0x00};
    static const uint8_t glyph_dash[5] = {0x08, 0x08, 0x08, 0x08, 0x08};
    static const uint8_t glyph_dot[5] = {0x00, 0x60, 0x60, 0x00, 0x00};
    static const uint8_t glyph_colon[5] = {0x00, 0x36, 0x36, 0x00, 0x00};
    static const uint8_t glyph_0[5] = {0x3E, 0x51, 0x49, 0x45, 0x3E};
    static const uint8_t glyph_1[5] = {0x00, 0x42, 0x7F, 0x40, 0x00};
    static const uint8_t glyph_2[5] = {0x62, 0x51, 0x49, 0x49, 0x46};
    static const uint8_t glyph_3[5] = {0x22, 0x49, 0x49, 0x49, 0x36};
    static const uint8_t glyph_4[5] = {0x18, 0x14, 0x12, 0x7F, 0x10};
    static const uint8_t glyph_5[5] = {0x2F, 0x49, 0x49, 0x49, 0x31};
    static const uint8_t glyph_6[5] = {0x3E, 0x49, 0x49, 0x49, 0x32};
    static const uint8_t glyph_7[5] = {0x01, 0x71, 0x09, 0x05, 0x03};
    static const uint8_t glyph_8[5] = {0x36, 0x49, 0x49, 0x49, 0x36};
    static const uint8_t glyph_9[5] = {0x26, 0x49, 0x49, 0x49, 0x3E};
    static const uint8_t glyph_A[5] = {0x7E, 0x11, 0x11, 0x11, 0x7E};
    static const uint8_t glyph_C[5] = {0x3E, 0x41, 0x41, 0x41, 0x22};
    static const uint8_t glyph_E[5] = {0x7F, 0x49, 0x49, 0x49, 0x41};
    static const uint8_t glyph_F[5] = {0x7F, 0x09, 0x09, 0x09, 0x01};
    static const uint8_t glyph_G[5] = {0x3E, 0x41, 0x49, 0x49, 0x3A};
    static const uint8_t glyph_H[5] = {0x7F, 0x08, 0x08, 0x08, 0x7F};
    static const uint8_t glyph_L[5] = {0x7F, 0x40, 0x40, 0x40, 0x40};
    static const uint8_t glyph_N[5] = {0x7F, 0x06, 0x18, 0x60, 0x7F};
    static const uint8_t glyph_O[5] = {0x3E, 0x41, 0x41, 0x41, 0x3E};
    static const uint8_t glyph_R[5] = {0x7F, 0x09, 0x19, 0x29, 0x46};
    static const uint8_t glyph_T[5] = {0x01, 0x01, 0x7F, 0x01, 0x01};
    static const uint8_t glyph_U[5] = {0x3F, 0x40, 0x40, 0x40, 0x3F};

    switch (c) {
    case ' ': return glyph_space;
    case '-': return glyph_dash;
    case '.': return glyph_dot;
    case ':': return glyph_colon;
    case '0': return glyph_0;
    case '1': return glyph_1;
    case '2': return glyph_2;
    case '3': return glyph_3;
    case '4': return glyph_4;
    case '5': return glyph_5;
    case '6': return glyph_6;
    case '7': return glyph_7;
    case '8': return glyph_8;
    case '9': return glyph_9;
    case 'A': return glyph_A;
    case 'C': return glyph_C;
    case 'E': return glyph_E;
    case 'F': return glyph_F;
    case 'G': return glyph_G;
    case 'H': return glyph_H;
    case 'L': return glyph_L;
    case 'N': return glyph_N;
    case 'O': return glyph_O;
    case 'R': return glyph_R;
    case 'T': return glyph_T;
    case 'U': return glyph_U;
    default: return glyph_space;
    }
}

static void oled_draw_char(oled96x96_t *dev, uint8_t x, uint8_t y, char c)
{
    const uint8_t *glyph = oled_get_glyph(c);
    for (uint8_t col = 0U; col < 5U; ++col) {
        uint8_t bits = glyph[col];
        for (uint8_t row = 0U; row < 7U; ++row) {
            bool on = ((bits >> row) & 0x01U) != 0U;
            oled_draw_pixel(dev, (uint8_t)(x + col), (uint8_t)(y + row), on);
        }
    }
}

static void oled_draw_text(oled96x96_t *dev, uint8_t x, uint8_t y, const char *text)
{
    if (text == NULL) {
        return;
    }

    uint8_t cursor_x = x;
    while (*text != '\0') {
        if ((cursor_x + 5U) >= OLED_WIDTH) {
            break;
        }
        oled_draw_char(dev, cursor_x, y, *text);
        cursor_x = (uint8_t)(cursor_x + 6U);
        ++text;
    }
}

HAL_StatusTypeDef oled96x96_show_status(oled96x96_t *dev, bool heating, float current_temp, float target_temp, float tolerance)
{
    if (!oled_is_valid_dev(dev) || !dev->initialized) {
        return HAL_ERROR;
    }

    memset(dev->framebuffer, 0x00, sizeof(dev->framebuffer));

    char line0[20];
    char line1[20];
    char line2[20];
    char line3[20];

    (void)snprintf(line0, sizeof(line0), "HEAT:%s", heating ? "ON" : "OFF");
    (void)snprintf(line1, sizeof(line1), "CUR:%5.1fC", (double)current_temp);
    (void)snprintf(line2, sizeof(line2), "TGT:%5.1fC", (double)target_temp);
    (void)snprintf(line3, sizeof(line3), "TOL:%5.1fC", (double)tolerance);

    oled_draw_text(dev, 0U, 0U, line0);
    oled_draw_text(dev, 0U, 14U, line1);
    oled_draw_text(dev, 0U, 28U, line2);
    oled_draw_text(dev, 0U, 42U, line3);

    return oled96x96_flush(dev);
}
