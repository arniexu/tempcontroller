#include "bsp_relay.h"

#if defined(USE_HAL_DRIVER)
#include "stm32f1xx_hal.h"
#endif

static bool g_relay_on = false;

void bsp_relay_init(void)
{
#if defined(USE_HAL_DRIVER)
    GPIO_InitTypeDef gpio = {0};

    __HAL_RCC_GPIOD_CLK_ENABLE();

    gpio.Pin = GPIO_PIN_2;
    gpio.Speed = GPIO_SPEED_FREQ_LOW;
    gpio.Mode = GPIO_MODE_OUTPUT_PP;
    HAL_GPIO_Init(GPIOD, &gpio);
    HAL_GPIO_WritePin(GPIOD, GPIO_PIN_2, GPIO_PIN_RESET);
#endif

    g_relay_on = false;
}

void bsp_relay_set(bool on)
{
    g_relay_on = on;

#if defined(USE_HAL_DRIVER)
    if (on)
    {
        HAL_GPIO_WritePin(GPIOD, GPIO_PIN_2, GPIO_PIN_SET);
    }
    else
    {
        HAL_GPIO_WritePin(GPIOD, GPIO_PIN_2, GPIO_PIN_RESET);
    }
#endif
}

bool bsp_relay_get(void)
{
    return g_relay_on;
}
