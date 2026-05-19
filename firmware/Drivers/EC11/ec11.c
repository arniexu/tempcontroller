#include "ec11.h"

static uint32_t ec11_irq_lock(void)
{
    uint32_t primask = __get_PRIMASK();
    __disable_irq();
    return primask;
}

static void ec11_irq_unlock(uint32_t primask)
{
    if (primask == 0U) {
        __enable_irq();
    }
}

void ec11_init(ec11_t *dev)
{
    dev->accumulated = 0;
    dev->key_pressed = false;
}

void ec11_on_exti(ec11_t *dev, uint16_t gpio_pin)
{
    if (gpio_pin == dev->pin_a || gpio_pin == dev->pin_b) {
        GPIO_PinState a = HAL_GPIO_ReadPin(dev->port_a, dev->pin_a);
        GPIO_PinState b = HAL_GPIO_ReadPin(dev->port_b, dev->pin_b);
        dev->accumulated += (a == b) ? 1 : -1;
    }

    if (gpio_pin == dev->pin_key) {
        if (HAL_GPIO_ReadPin(dev->port_key, dev->pin_key) == GPIO_PIN_RESET) {
            dev->key_pressed = true;
        }
    }
}

int8_t ec11_take_step(ec11_t *dev)
{
    uint32_t primask = ec11_irq_lock();
    int32_t acc = dev->accumulated;
    int8_t step = 0;
    if (acc >= 4) {
        step = 1;
        dev->accumulated = 0;
    } else if (acc <= -4) {
        step = -1;
        dev->accumulated = 0;
    }
    ec11_irq_unlock(primask);
    return step;
}

bool ec11_take_key_press(ec11_t *dev)
{
    uint32_t primask = ec11_irq_lock();
    bool pressed = dev->key_pressed;
    dev->key_pressed = false;
    ec11_irq_unlock(primask);
    return pressed;
}
