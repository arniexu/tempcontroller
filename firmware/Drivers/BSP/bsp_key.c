#include "bsp_key.h"

#if defined(USE_STDPERIPH_DRIVER)
#include "stm32f10x_gpio.h"
#include "stm32f10x_rcc.h"
#endif

#if !defined(USE_STDPERIPH_DRIVER)
static bool g_mock_pressed[BSP_KEY_COUNT] = {false, false, false, false};
#endif

#if defined(USE_STDPERIPH_DRIVER)
static GPIO_TypeDef *key_gpio_port(bsp_key_id_t key)
{
    switch (key)
    {
    case BSP_KEY_SET:
        return GPIOC;
    case BSP_KEY_UP:
        return GPIOA;
    case BSP_KEY_DOWN:
        return GPIOA;
    default:
        return (GPIO_TypeDef *)0;
    }
}

static uint16_t key_to_pin(bsp_key_id_t key)
{
    switch (key)
    {
    case BSP_KEY_SET:
        return GPIO_Pin_5;
    case BSP_KEY_UP:
        return GPIO_Pin_0;
    case BSP_KEY_DOWN:
        return GPIO_Pin_15;
    default:
        return 0U;
    }
}
#endif

void bsp_key_init(void)
{
#if defined(USE_STDPERIPH_DRIVER)
    GPIO_InitTypeDef gpio;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOC | RCC_APB2Periph_AFIO, ENABLE);
    GPIO_PinRemapConfig(GPIO_Remap_SWJ_JTAGDisable, ENABLE);

    gpio.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_15;
    gpio.GPIO_Mode = GPIO_Mode_IPU;
    gpio.GPIO_Speed = GPIO_Speed_2MHz;
    GPIO_Init(GPIOA, &gpio);

    gpio.GPIO_Pin = GPIO_Pin_5;
    gpio.GPIO_Mode = GPIO_Mode_IPU;
    gpio.GPIO_Speed = GPIO_Speed_2MHz;
    GPIO_Init(GPIOC, &gpio);
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
        GPIO_TypeDef *port = key_gpio_port(key);
        uint16_t pin = key_to_pin(key);

        if ((port == 0) || (pin == 0U))
        {
            return false;
        }

        return (GPIO_ReadInputDataBit(port, pin) == Bit_RESET);
    }
#else
    return g_mock_pressed[(unsigned int)key];
#endif
}

void bsp_key_mock_set_state(bsp_key_id_t key, bool pressed)
{
#if defined(USE_STDPERIPH_DRIVER)
    (void)key;
    (void)pressed;
#else
    if ((unsigned int)key >= (unsigned int)BSP_KEY_COUNT)
    {
        return;
    }

    g_mock_pressed[(unsigned int)key] = pressed;
#endif
}

