#include "bsp_relay.h"

#if defined(USE_STDPERIPH_DRIVER)
#include "stm32f10x_gpio.h"
#include "stm32f10x_rcc.h"
#endif

static bool g_relay_on = false;

void bsp_relay_init(void)
{
#if defined(USE_STDPERIPH_DRIVER)
    GPIO_InitTypeDef gpio;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOD, ENABLE);

    gpio.GPIO_Pin = GPIO_Pin_2;
    gpio.GPIO_Speed = GPIO_Speed_2MHz;
    gpio.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_Init(GPIOD, &gpio);
    GPIO_ResetBits(GPIOD, GPIO_Pin_2);
#endif

    g_relay_on = false;
}

void bsp_relay_set(bool on)
{
    g_relay_on = on;

#if defined(USE_STDPERIPH_DRIVER)
    if (on)
    {
        GPIO_SetBits(GPIOD, GPIO_Pin_2);
    }
    else
    {
        GPIO_ResetBits(GPIOD, GPIO_Pin_2);
    }
#endif
}

bool bsp_relay_get(void)
{
    return g_relay_on;
}
