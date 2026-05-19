#include "ads1220.h"

#define ADS1220_CMD_RESET      0x06U
#define ADS1220_CMD_START_SYNC 0x08U
#define ADS1220_CMD_RDATA      0x10U
#define ADS1220_TEMP_SCALE_C_PER_LSB 0.001f
#define ADS1220_TEMP_OFFSET_C 0.0f
#define ADS1220_SIGN_BIT_MASK 0x800000L
#define ADS1220_SIGN_EXTEND_MASK 0xFF000000L

static inline void ads1220_cs(ads1220_t *dev, GPIO_PinState state)
{
    HAL_GPIO_WritePin(dev->cs_port, dev->cs_pin, state);
}

static HAL_StatusTypeDef ads1220_send_cmd(ads1220_t *dev, uint8_t cmd)
{
    ads1220_cs(dev, GPIO_PIN_RESET);
    HAL_StatusTypeDef rc = HAL_SPI_Transmit(dev->hspi, &cmd, 1U, 10U);
    ads1220_cs(dev, GPIO_PIN_SET);
    return rc;
}

HAL_StatusTypeDef ads1220_init(ads1220_t *dev)
{
    HAL_GPIO_WritePin(dev->rst_port, dev->rst_pin, GPIO_PIN_RESET);
    HAL_Delay(2U);
    HAL_GPIO_WritePin(dev->rst_port, dev->rst_pin, GPIO_PIN_SET);
    HAL_Delay(2U);

    HAL_StatusTypeDef rc = ads1220_send_cmd(dev, ADS1220_CMD_RESET);
    if (rc != HAL_OK) {
        return rc;
    }
    HAL_Delay(2U);

    rc = ads1220_send_cmd(dev, ADS1220_CMD_START_SYNC);
    return rc;
}

HAL_StatusTypeDef ads1220_read_raw24(ads1220_t *dev, int32_t *raw, uint32_t timeout_ms)
{
    uint32_t t0 = HAL_GetTick();
    while (HAL_GPIO_ReadPin(dev->drdy_port, dev->drdy_pin) == GPIO_PIN_SET) {
        if ((HAL_GetTick() - t0) > timeout_ms) {
            return HAL_TIMEOUT;
        }
    }

    uint8_t cmd = ADS1220_CMD_RDATA;
    uint8_t buf[3] = {0};

    ads1220_cs(dev, GPIO_PIN_RESET);
    HAL_StatusTypeDef rc = HAL_SPI_Transmit(dev->hspi, &cmd, 1U, 10U);
    if (rc == HAL_OK) {
        rc = HAL_SPI_Receive(dev->hspi, buf, 3U, 10U);
    }
    ads1220_cs(dev, GPIO_PIN_SET);

    if (rc != HAL_OK) {
        return rc;
    }

    int32_t value = ((int32_t)buf[0] << 16) | ((int32_t)buf[1] << 8) | (int32_t)buf[2];
    if (value & ADS1220_SIGN_BIT_MASK) {
        value |= (int32_t)ADS1220_SIGN_EXTEND_MASK;
    }
    *raw = value;
    return HAL_OK;
}

float ads1220_raw_to_celsius(int32_t raw)
{
    return ((float)raw * ADS1220_TEMP_SCALE_C_PER_LSB) + ADS1220_TEMP_OFFSET_C;
}
