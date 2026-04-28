#include "bsp_buzzer.h"

#if defined(USE_STDPERIPH_DRIVER)
#include "stm32f10x_gpio.h"
#include "stm32f10x_rcc.h"
#endif

static bool g_buzzer_on = false;

void bsp_buzzer_init(void)
{
#if defined(USE_STDPERIPH_DRIVER)
    GPIO_InitTypeDef gpio;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

    gpio.GPIO_Pin = GPIO_Pin_8;
    gpio.GPIO_Speed = GPIO_Speed_2MHz;
    gpio.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_Init(GPIOA, &gpio);
    GPIO_ResetBits(GPIOA, GPIO_Pin_8);
#endif

    g_buzzer_on = false;
}

void bsp_buzzer_set(bool on)
{
    g_buzzer_on = on;

#if defined(USE_STDPERIPH_DRIVER)
    if (on)
    {
        GPIO_SetBits(GPIOA, GPIO_Pin_8);
    }
    else
    {
        GPIO_ResetBits(GPIOA, GPIO_Pin_8);
    }
#endif
}

bool bsp_buzzer_get(void)
{
    return g_buzzer_on;
}
