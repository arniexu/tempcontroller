#include "stm32f1xx_hal_spi.h"
#include "stm32f1xx_hal.h"

#define SPI_SR_RXNE (1U << 0)
#define SPI_SR_TXE  (1U << 1)
#define SPI_SR_BSY  (1U << 7)

static HAL_StatusTypeDef spi_wait_flag(const __IO uint32_t *reg, uint32_t mask, uint32_t state, uint32_t timeout_ms)
{
    uint32_t t0 = HAL_GetTick();
    while (((*reg & mask) != 0U) != (state != 0U)) {
        if ((HAL_GetTick() - t0) > timeout_ms) {
            return HAL_TIMEOUT;
        }
    }
    return HAL_OK;
}

HAL_StatusTypeDef HAL_SPI_Transmit(SPI_HandleTypeDef *hspi, uint8_t *pData, uint16_t Size, uint32_t Timeout)
{
    if (hspi == NULL || hspi->Instance == NULL || (pData == NULL && Size > 0U)) {
        return HAL_ERROR;
    }

    for (uint16_t i = 0U; i < Size; i++) {
        HAL_StatusTypeDef rc = spi_wait_flag(&hspi->Instance->SR, SPI_SR_TXE, 1U, Timeout);
        if (rc != HAL_OK) {
            return rc;
        }
        *((__IO uint8_t *)&hspi->Instance->DR) = pData[i];
    }

    if (spi_wait_flag(&hspi->Instance->SR, SPI_SR_TXE, 1U, Timeout) != HAL_OK) {
        return HAL_TIMEOUT;
    }
    if (spi_wait_flag(&hspi->Instance->SR, SPI_SR_BSY, 0U, Timeout) != HAL_OK) {
        return HAL_TIMEOUT;
    }

    return HAL_OK;
}

HAL_StatusTypeDef HAL_SPI_Receive(SPI_HandleTypeDef *hspi, uint8_t *pData, uint16_t Size, uint32_t Timeout)
{
    if (hspi == NULL || hspi->Instance == NULL || (pData == NULL && Size > 0U)) {
        return HAL_ERROR;
    }

    for (uint16_t i = 0U; i < Size; i++) {
        HAL_StatusTypeDef rc = spi_wait_flag(&hspi->Instance->SR, SPI_SR_TXE, 1U, Timeout);
        if (rc != HAL_OK) {
            return rc;
        }

        *((__IO uint8_t *)&hspi->Instance->DR) = 0xFFU;

        rc = spi_wait_flag(&hspi->Instance->SR, SPI_SR_RXNE, 1U, Timeout);
        if (rc != HAL_OK) {
            return rc;
        }

        pData[i] = *((__IO uint8_t *)&hspi->Instance->DR);
    }

    return HAL_OK;
}
