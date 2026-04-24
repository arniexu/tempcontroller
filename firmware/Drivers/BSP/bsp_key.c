#include "bsp_key.h"

#if defined(USE_STDPERIPH_DRIVER)
#include "stm32f10x_gpio.h"
#include "stm32f10x_rcc.h"
#endif

static bool g_mock_pressed[BSP_KEY_COUNT] = {false, false, false, false};

void bsp_key_init(void)
{
#if defined(USE_STDPERIPH_DRIVER)
    GPIO_InitTypeDef gpio;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

    gpio.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_2 | GPIO_Pin_3;
    gpio.GPIO_Mode = GPIO_Mode_IPU;
    gpio.GPIO_Speed = GPIO_Speed_2MHz;
    GPIO_Init(GPIOA, &gpio);
#endif
}

bool bsp_key_get_state(bsp_key_id_t key)
{
    if ((unsigned int)key >= (unsigned int)BSP_KEY_COUNT)
    {
        return false;
    }

#if defined(USE_STDPERIPH_DRIVER)
    {
        uint16_t pin = GPIO_Pin_0;

        switch (key)
        {
        case BSP_KEY_SET:
            pin = GPIO_Pin_0;
            break;
        case BSP_KEY_UP:
            pin = GPIO_Pin_1;
            break;
        case BSP_KEY_DOWN:
            pin = GPIO_Pin_2;
            break;
        case BSP_KEY_BACK:
            pin = GPIO_Pin_3;
            break;
        default:
            return false;
        }

        /* Active-low keys with pull-up input. */
        return GPIO_ReadInputDataBit(GPIOA, pin) == Bit_RESET;
    }
#else
    return g_mock_pressed[(unsigned int)key];
#endif
}

void bsp_key_mock_set_state(bsp_key_id_t key, bool pressed)
{
    if ((unsigned int)key >= (unsigned int)BSP_KEY_COUNT)
    {
        return;
    }

    g_mock_pressed[(unsigned int)key] = pressed;
}
