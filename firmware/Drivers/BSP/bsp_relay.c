#include "bsp_relay.h"

#if defined(USE_HAL_DRIVER)
#include "../../ProjectConfig/bsp_config_select.h"
#include "stm32f1xx_hal.h"
#endif

static bool g_relay_on = false;

void bsp_relay_init(void)
{
#if defined(USE_HAL_DRIVER)
    GPIO_InitTypeDef gpio = {0};

    BSP_RELAY_GPIO_CLK_ENABLE();

    gpio.Pin = BSP_RELAY_PIN;
    gpio.Speed = GPIO_SPEED_FREQ_LOW;
    gpio.Mode = GPIO_MODE_OUTPUT_PP;
    HAL_GPIO_Init(BSP_RELAY_GPIO_PORT, &gpio);
    HAL_GPIO_WritePin(BSP_RELAY_GPIO_PORT, BSP_RELAY_PIN, GPIO_PIN_RESET);
#endif

    g_relay_on = false;
}

void bsp_relay_set(bool on)
{
    g_relay_on = on;

#if defined(USE_HAL_DRIVER)
    if (on)
    {
        HAL_GPIO_WritePin(BSP_RELAY_GPIO_PORT, BSP_RELAY_PIN, GPIO_PIN_SET);
    }
    else
    {
        HAL_GPIO_WritePin(BSP_RELAY_GPIO_PORT, BSP_RELAY_PIN, GPIO_PIN_RESET);
    }
#endif
}

bool bsp_relay_get(void)
{
    return g_relay_on;
}
