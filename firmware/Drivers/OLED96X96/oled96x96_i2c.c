#include "oled96x96_i2c.h"

#include <stdio.h>
#include <string.h>

#define OLED_CTRL_CMD  0x00U
#define OLED_CTRL_DATA 0x40U

static HAL_StatusTypeDef oled_tx(oled96x96_t *dev, uint8_t ctrl, const uint8_t *data, uint16_t len)
{
    uint8_t frame[17];
    frame[0] = ctrl;

    uint16_t offset = 0U;
    while (offset < len) {
        uint16_t chunk = (uint16_t)((len - offset) > 16U ? 16U : (len - offset));
        memcpy(&frame[1], &data[offset], chunk);
        HAL_StatusTypeDef rc = HAL_I2C_Master_Transmit(dev->hi2c, (uint16_t)(dev->addr_7bit << 1), frame, (uint16_t)(1U + chunk), 100U);
        if (rc != HAL_OK) {
            return rc;
        }
        offset += chunk;
    }

    return HAL_OK;
}

HAL_StatusTypeDef oled96x96_init(oled96x96_t *dev)
{
    const uint8_t init_seq[] = {
        0xAE, 0xA1, 0xC8, 0xA8, 0x5F, 0xD3, 0x00, 0x40, 0xA6, 0xAF
    };
    return oled_tx(dev, OLED_CTRL_CMD, init_seq, (uint16_t)sizeof(init_seq));
}

HAL_StatusTypeDef oled96x96_fill(oled96x96_t *dev, uint8_t value)
{
    memset(dev->framebuffer, value, sizeof(dev->framebuffer));
    return oled_tx(dev, OLED_CTRL_DATA, dev->framebuffer, (uint16_t)sizeof(dev->framebuffer));
}

HAL_StatusTypeDef oled96x96_show_status(oled96x96_t *dev, bool heating, float current_temp, float target_temp, float tolerance)
{
    (void)heating;
    (void)current_temp;
    (void)target_temp;
    (void)tolerance;

    return oled96x96_fill(dev, 0x00U);
}
