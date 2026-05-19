#include "stm32f1xx_hal_gpio.h"
#include <stddef.h>

GPIO_PinState HAL_GPIO_ReadPin(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin)
{
    if (GPIOx == NULL) {
        return GPIO_PIN_RESET;
    }

    return ((GPIOx->IDR & GPIO_Pin) != 0U) ? GPIO_PIN_SET : GPIO_PIN_RESET;
}

void HAL_GPIO_WritePin(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin, GPIO_PinState PinState)
{
    if (GPIOx == NULL) {
        return;
    }

    if (PinState == GPIO_PIN_SET) {
        GPIOx->BSRR = GPIO_Pin;
    } else {
        GPIOx->BRR = GPIO_Pin;
    }
}
