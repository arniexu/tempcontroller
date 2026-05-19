#include "ads1220.h"

#include <string.h>

#define ADS1220_CMD_RESET      0x06U
#define ADS1220_CMD_START_SYNC 0x08U
#define ADS1220_CMD_POWERDOWN  0x02U
#define ADS1220_CMD_RDATA      0x10U
#define ADS1220_CMD_RREG       0x20U
#define ADS1220_CMD_WREG       0x40U

#define ADS1220_REGISTER_COUNT 4U
#define ADS1220_DEFAULT_RETRIES 2U
#define ADS1220_DEFAULT_SPI_TIMEOUT_MS 20U
#define ADS1220_DEFAULT_DRDY_TIMEOUT_MS 100U
/* ADS1220 internal temperature sensor output LSB is 1/32°C in temp mode (datasheet transfer function). */
#define ADS1220_DEFAULT_TEMP_SCALE_C_PER_LSB 0.03125f
#define ADS1220_DEFAULT_TEMP_OFFSET_C 0.0f

/* REG1.TS=1 selects the internal temperature sensor. */
#define ADS1220_DEFAULT_REG0 0x00U
#define ADS1220_DEFAULT_REG1 0x24U
#define ADS1220_DEFAULT_REG2 0x10U
#define ADS1220_DEFAULT_REG3 0x00U

#define ADS1220_SIGN_BIT_MASK 0x800000L
#define ADS1220_SIGN_EXTEND_MASK 0xFF000000L

static bool ads1220_is_valid_dev(const ads1220_t *dev)
{
    return (dev != NULL) &&
           (dev->hspi != NULL) &&
           (dev->cs_port != NULL) &&
           (dev->drdy_port != NULL);
}

static inline void ads1220_cs(ads1220_t *dev, GPIO_PinState state)
{
    HAL_GPIO_WritePin(dev->cs_port, dev->cs_pin, state);
}

static HAL_StatusTypeDef ads1220_send_cmd(ads1220_t *dev, uint8_t cmd)
{
    ads1220_cs(dev, GPIO_PIN_RESET);
    HAL_StatusTypeDef rc = HAL_SPI_Transmit(dev->hspi, &cmd, 1U, dev->config.spi_timeout_ms);
    ads1220_cs(dev, GPIO_PIN_SET);
    return rc;
}

static HAL_StatusTypeDef ads1220_wait_drdy(ads1220_t *dev, uint32_t timeout_ms)
{
    uint32_t t0 = HAL_GetTick();
    while (HAL_GPIO_ReadPin(dev->drdy_port, dev->drdy_pin) == GPIO_PIN_SET) {
        if ((HAL_GetTick() - t0) > timeout_ms) {
            return HAL_TIMEOUT;
        }
    }
    return HAL_OK;
}

void ads1220_get_default_config(ads1220_config_t *config)
{
    if (config == NULL) {
        return;
    }

    config->reg0 = ADS1220_DEFAULT_REG0;
    config->reg1 = ADS1220_DEFAULT_REG1;
    config->reg2 = ADS1220_DEFAULT_REG2;
    config->reg3 = ADS1220_DEFAULT_REG3;
    config->spi_timeout_ms = ADS1220_DEFAULT_SPI_TIMEOUT_MS;
    config->drdy_timeout_ms = ADS1220_DEFAULT_DRDY_TIMEOUT_MS;
    config->io_retries = ADS1220_DEFAULT_RETRIES;
    config->temp_scale_c_per_lsb = ADS1220_DEFAULT_TEMP_SCALE_C_PER_LSB;
    config->temp_offset_c = ADS1220_DEFAULT_TEMP_OFFSET_C;
}

HAL_StatusTypeDef ads1220_reset(ads1220_t *dev)
{
    if (!ads1220_is_valid_dev(dev)) {
        return HAL_ERROR;
    }

    if (dev->rst_port != NULL) {
        HAL_GPIO_WritePin(dev->rst_port, dev->rst_pin, GPIO_PIN_RESET);
        HAL_Delay(2U);
        HAL_GPIO_WritePin(dev->rst_port, dev->rst_pin, GPIO_PIN_SET);
        HAL_Delay(2U);
    }

    HAL_StatusTypeDef rc = HAL_ERROR;
    for (uint8_t attempt = 0U; attempt <= dev->config.io_retries; ++attempt) {
        rc = ads1220_send_cmd(dev, ADS1220_CMD_RESET);
        if (rc == HAL_OK) {
            break;
        }
        HAL_Delay(1U);
    }
    HAL_Delay(2U);
    return rc;
}

HAL_StatusTypeDef ads1220_start(ads1220_t *dev)
{
    if (!ads1220_is_valid_dev(dev)) {
        return HAL_ERROR;
    }

    HAL_StatusTypeDef rc = HAL_ERROR;
    for (uint8_t attempt = 0U; attempt <= dev->config.io_retries; ++attempt) {
        rc = ads1220_send_cmd(dev, ADS1220_CMD_START_SYNC);
        if (rc == HAL_OK) {
            break;
        }
        HAL_Delay(1U);
    }
    return rc;
}

HAL_StatusTypeDef ads1220_stop(ads1220_t *dev)
{
    if (!ads1220_is_valid_dev(dev)) {
        return HAL_ERROR;
    }

    HAL_StatusTypeDef rc = HAL_ERROR;
    for (uint8_t attempt = 0U; attempt <= dev->config.io_retries; ++attempt) {
        rc = ads1220_send_cmd(dev, ADS1220_CMD_POWERDOWN);
        if (rc == HAL_OK) {
            break;
        }
        HAL_Delay(1U);
    }
    return rc;
}

HAL_StatusTypeDef ads1220_read_registers(ads1220_t *dev, uint8_t start_reg, uint8_t *data, uint8_t len)
{
    if (!ads1220_is_valid_dev(dev) || (data == NULL) || (len == 0U)) {
        return HAL_ERROR;
    }
    if ((start_reg >= ADS1220_REGISTER_COUNT) || ((start_reg + len) > ADS1220_REGISTER_COUNT)) {
        return HAL_ERROR;
    }

    uint8_t cmd[2];
    cmd[0] = (uint8_t)(ADS1220_CMD_RREG | ((start_reg & 0x03U) << 2));
    cmd[1] = (uint8_t)(len - 1U);

    ads1220_cs(dev, GPIO_PIN_RESET);
    HAL_StatusTypeDef rc = HAL_SPI_Transmit(dev->hspi, cmd, 2U, dev->config.spi_timeout_ms);
    if (rc == HAL_OK) {
        rc = HAL_SPI_Receive(dev->hspi, data, len, dev->config.spi_timeout_ms);
    }
    ads1220_cs(dev, GPIO_PIN_SET);
    return rc;
}

HAL_StatusTypeDef ads1220_write_registers(ads1220_t *dev, uint8_t start_reg, const uint8_t *data, uint8_t len)
{
    if (!ads1220_is_valid_dev(dev) || (data == NULL) || (len == 0U)) {
        return HAL_ERROR;
    }
    if ((start_reg >= ADS1220_REGISTER_COUNT) || ((start_reg + len) > ADS1220_REGISTER_COUNT)) {
        return HAL_ERROR;
    }

    uint8_t cmd[2];
    uint8_t tx_data[ADS1220_REGISTER_COUNT];
    cmd[0] = (uint8_t)(ADS1220_CMD_WREG | ((start_reg & 0x03U) << 2));
    cmd[1] = (uint8_t)(len - 1U);
    memcpy(tx_data, data, len);

    ads1220_cs(dev, GPIO_PIN_RESET);
    HAL_StatusTypeDef rc = HAL_SPI_Transmit(dev->hspi, cmd, 2U, dev->config.spi_timeout_ms);
    if (rc == HAL_OK) {
        rc = HAL_SPI_Transmit(dev->hspi, tx_data, len, dev->config.spi_timeout_ms);
    }
    ads1220_cs(dev, GPIO_PIN_SET);
    return rc;
}

HAL_StatusTypeDef ads1220_configure(ads1220_t *dev, const ads1220_config_t *config)
{
    if (!ads1220_is_valid_dev(dev) || (config == NULL)) {
        return HAL_ERROR;
    }

    uint8_t regs[ADS1220_REGISTER_COUNT];
    regs[0] = config->reg0;
    regs[1] = config->reg1;
    regs[2] = config->reg2;
    regs[3] = config->reg3;

    HAL_StatusTypeDef rc = HAL_ERROR;
    for (uint8_t attempt = 0U; attempt <= config->io_retries; ++attempt) {
        rc = ads1220_write_registers(dev, 0U, regs, ADS1220_REGISTER_COUNT);
        if (rc != HAL_OK) {
            HAL_Delay(1U);
            continue;
        }

        uint8_t verify[ADS1220_REGISTER_COUNT] = {0};
        rc = ads1220_read_registers(dev, 0U, verify, ADS1220_REGISTER_COUNT);
        if (rc == HAL_OK && (memcmp(verify, regs, ADS1220_REGISTER_COUNT) == 0)) {
            dev->config = *config;
            return HAL_OK;
        }
        rc = HAL_ERROR;
        HAL_Delay(1U);
    }
    return rc;
}

HAL_StatusTypeDef ads1220_init(ads1220_t *dev)
{
    if (!ads1220_is_valid_dev(dev)) {
        return HAL_ERROR;
    }

    if (dev->config.spi_timeout_ms == 0U) {
        ads1220_get_default_config(&dev->config);
    }

    HAL_StatusTypeDef rc = ads1220_reset(dev);
    if (rc != HAL_OK) {
        dev->initialized = false;
        return rc;
    }

    rc = ads1220_configure(dev, &dev->config);
    if (rc != HAL_OK) {
        dev->initialized = false;
        return rc;
    }

    rc = ads1220_start(dev);
    dev->initialized = (rc == HAL_OK);
    return rc;
}

HAL_StatusTypeDef ads1220_read_raw24(ads1220_t *dev, int32_t *raw, uint32_t timeout_ms)
{
    if (!ads1220_is_valid_dev(dev) || (raw == NULL) || !dev->initialized) {
        return HAL_ERROR;
    }

    const uint32_t effective_timeout = (timeout_ms == 0U) ? dev->config.drdy_timeout_ms : timeout_ms;
    HAL_StatusTypeDef rc = HAL_ERROR;
    uint8_t buf[3] = {0};

    for (uint8_t attempt = 0U; attempt <= dev->config.io_retries; ++attempt) {
        rc = ads1220_wait_drdy(dev, effective_timeout);
        if (rc != HAL_OK) {
            (void)ads1220_start(dev);
            continue;
        }

        uint8_t cmd = ADS1220_CMD_RDATA;
        ads1220_cs(dev, GPIO_PIN_RESET);
        rc = HAL_SPI_Transmit(dev->hspi, &cmd, 1U, dev->config.spi_timeout_ms);
        if (rc == HAL_OK) {
            rc = HAL_SPI_Receive(dev->hspi, buf, 3U, dev->config.spi_timeout_ms);
        }
        ads1220_cs(dev, GPIO_PIN_SET);

        if (rc == HAL_OK) {
            break;
        }
        HAL_Delay(1U);
    }

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

float ads1220_raw_to_celsius(const ads1220_t *dev, int32_t raw)
{
    if (dev == NULL) {
        return ((float)raw * ADS1220_DEFAULT_TEMP_SCALE_C_PER_LSB) + ADS1220_DEFAULT_TEMP_OFFSET_C;
    }
    return ((float)raw * dev->config.temp_scale_c_per_lsb) + dev->config.temp_offset_c;
}
