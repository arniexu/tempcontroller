#include "bsp_buzzer.h"

#if defined(USE_HAL_DRIVER)
#include "stm32f1xx_hal.h"
#endif

static bool g_buzzer_on = false;

void bsp_buzzer_init(void)
{
#if defined(USE_HAL_DRIVER)
    GPIO_InitTypeDef gpio = {0};

    __HAL_RCC_GPIOA_CLK_ENABLE();

    gpio.Pin = GPIO_PIN_8;
    gpio.Speed = GPIO_SPEED_FREQ_LOW;
    gpio.Mode = GPIO_MODE_OUTPUT_PP;
    HAL_GPIO_Init(GPIOA, &gpio);
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_8, GPIO_PIN_RESET);
#endif

    g_buzzer_on = false;
}

void bsp_buzzer_set(bool on)
{
    g_buzzer_on = on;

#if defined(USE_HAL_DRIVER)
    if (on)
    {
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_8, GPIO_PIN_SET);
    }
    else
    {
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_8, GPIO_PIN_RESET);
    }
#endif
}

bool bsp_buzzer_get(void)
{
    return g_buzzer_on;
}
