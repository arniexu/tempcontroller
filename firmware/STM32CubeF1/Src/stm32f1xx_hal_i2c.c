#include "stm32f1xx_hal_i2c.h"
#include "stm32f1xx_hal.h"

#define I2C_CR1_PE    (1U << 0)
#define I2C_CR1_START (1U << 8)
#define I2C_CR1_STOP  (1U << 9)

#define I2C_SR1_SB    (1U << 0)
#define I2C_SR1_ADDR  (1U << 1)
#define I2C_SR1_BTF   (1U << 2)
#define I2C_SR1_TXE   (1U << 7)
#define I2C_SR1_AF    (1U << 10)

#define I2C_SR2_BUSY  (1U << 1)

static HAL_StatusTypeDef i2c_wait_flag(const __IO uint32_t *reg, uint32_t mask, uint32_t state, uint32_t timeout_ms)
{
    uint32_t t0 = HAL_GetTick();
    while (((*reg & mask) != 0U) != (state != 0U)) {
        if ((HAL_GetTick() - t0) > timeout_ms) {
            return HAL_TIMEOUT;
        }
    }
    return HAL_OK;
}

HAL_StatusTypeDef HAL_I2C_Master_Transmit(
    I2C_HandleTypeDef *hi2c,
    uint16_t DevAddress,
    uint8_t *pData,
    uint16_t Size,
    uint32_t Timeout)
{
    if (hi2c == NULL || hi2c->Instance == NULL || (pData == NULL && Size > 0U)) {
        return HAL_ERROR;
    }

    I2C_TypeDef *i2c = hi2c->Instance;
    i2c->CR1 |= I2C_CR1_PE;

    if (i2c_wait_flag(&i2c->SR2, I2C_SR2_BUSY, 0U, Timeout) != HAL_OK) {
        return HAL_BUSY;
    }

    i2c->CR1 |= I2C_CR1_START;
    if (i2c_wait_flag(&i2c->SR1, I2C_SR1_SB, 1U, Timeout) != HAL_OK) {
        return HAL_TIMEOUT;
    }

    *((__IO uint8_t *)&i2c->DR) = (uint8_t)(DevAddress & 0xFEU);
    if (i2c_wait_flag(&i2c->SR1, I2C_SR1_ADDR, 1U, Timeout) != HAL_OK) {
        i2c->CR1 |= I2C_CR1_STOP;
        return ((i2c->SR1 & I2C_SR1_AF) != 0U) ? HAL_ERROR : HAL_TIMEOUT;
    }

    (void)i2c->SR1;
    (void)i2c->SR2;

    for (uint16_t i = 0U; i < Size; i++) {
        if (i2c_wait_flag(&i2c->SR1, I2C_SR1_TXE, 1U, Timeout) != HAL_OK) {
            i2c->CR1 |= I2C_CR1_STOP;
            return HAL_TIMEOUT;
        }
        *((__IO uint8_t *)&i2c->DR) = pData[i];
    }

    if (i2c_wait_flag(&i2c->SR1, I2C_SR1_BTF, 1U, Timeout) != HAL_OK) {
        i2c->CR1 |= I2C_CR1_STOP;
        return HAL_TIMEOUT;
    }

    i2c->CR1 |= I2C_CR1_STOP;
    return HAL_OK;
}
