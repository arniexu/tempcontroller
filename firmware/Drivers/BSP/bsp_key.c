#include "bsp_key.h"

#if defined(USE_HAL_DRIVER)
#include "../../ProjectConfig/bsp_config_select.h"
#include "stm32f1xx_hal.h"
#endif

#if !defined(USE_HAL_DRIVER)
static bool g_mock_pressed[BSP_KEY_COUNT] = {false, false, false, false};
#endif

#if defined(USE_HAL_DRIVER)
static uint16_t key_to_pin(bsp_key_id_t key)
{
    switch (key)
    {
    case BSP_KEY_SET:
        return BSP_KEY_PIN_SET;
    case BSP_KEY_UP:
        return BSP_KEY_PIN_UP;
    case BSP_KEY_DOWN:
        return BSP_KEY_PIN_DOWN;
    default:
        return 0U;
    }
}
#endif

void bsp_key_init(void)
{
#if defined(USE_HAL_DRIVER)
    GPIO_InitTypeDef gpio = {0};

    BSP_KEY_GPIO_CLK_ENABLE();
    BSP_KEY_AFIO_CLK_ENABLE();
    /* PA13 is SWDIO by default; disable SWJ to use it as SET key input. */
    BSP_KEY_SWJ_REMAP();

    gpio.Pin = BSP_KEY_PIN_SET | BSP_KEY_PIN_UP | BSP_KEY_PIN_DOWN;
    gpio.Mode = GPIO_MODE_INPUT;
    gpio.Pull = GPIO_PULLUP;
    gpio.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(BSP_KEY_GPIO_PORT, &gpio);
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
        uint16_t pin = key_to_pin(key);
        GPIO_PinState level;

        if (pin == 0U)
        {
            return false;
        }

        level = HAL_GPIO_ReadPin(BSP_KEY_GPIO_PORT, pin);
        if ((key == BSP_KEY_UP) && (BSP_KEY_UP_ACTIVE_HIGH == 1U))
        {
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

