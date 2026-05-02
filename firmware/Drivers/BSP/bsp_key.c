#include "bsp_key.h"

#if defined(USE_HAL_DRIVER)
#include "stm32f1xx_hal.h"
#endif

#if !defined(USE_HAL_DRIVER)
static bool g_mock_pressed[BSP_KEY_COUNT] = {false, false, false, false};
#endif

#if defined(USE_HAL_DRIVER)
static GPIO_TypeDef *key_gpio_port(bsp_key_id_t key)
{
    switch (key)
    {
    case BSP_KEY_SET:
        return GPIOA;
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
        return GPIO_PIN_13;
    case BSP_KEY_UP:
        return GPIO_PIN_0;
    case BSP_KEY_DOWN:
        return GPIO_PIN_15;
    default:
        return 0U;
    }
}
#endif

void bsp_key_init(void)
{
#if defined(USE_HAL_DRIVER)
    GPIO_InitTypeDef gpio = {0};

    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_AFIO_CLK_ENABLE();
    /* PA13 is SWDIO by default; disable SWJ to use it as SET key input. */
    __HAL_AFIO_REMAP_SWJ_DISABLE();

    gpio.Pin = GPIO_PIN_0 | GPIO_PIN_13 | GPIO_PIN_15;
    gpio.Mode = GPIO_MODE_INPUT;
    gpio.Pull = GPIO_PULLUP;
    gpio.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOA, &gpio);
#endif
}

bool bsp_key_get_state(bsp_key_id_t key)
{
    if ((unsigned int)key >= (unsigned int)BSP_KEY_COUNT)
    {
        return false;
    }

#if defined(USE_HAL_DRIVER)
    {
        GPIO_TypeDef *port = key_gpio_port(key);
        uint16_t pin = key_to_pin(key);
        GPIO_PinState level;

        if ((port == 0) || (pin == 0U))
        {
            return false;
        }

        level = HAL_GPIO_ReadPin(port, pin);
        if (key == BSP_KEY_UP)
        {
            /* ALIENTEK WK_UP key on PA0 is active-high. */
            return (level == GPIO_PIN_SET);
        }
        return (level == GPIO_PIN_RESET);
    }
#else
    return g_mock_pressed[(unsigned int)key];
#endif
}

void bsp_key_mock_set_state(bsp_key_id_t key, bool pressed)
{
#if defined(USE_HAL_DRIVER)
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

