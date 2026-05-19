#ifndef EC11_H
#define EC11_H

#include "stm32f1xx_hal.h"
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    GPIO_TypeDef *port_a;
    uint16_t pin_a;
    GPIO_TypeDef *port_b;
    uint16_t pin_b;
    GPIO_TypeDef *port_key;
    uint16_t pin_key;
    volatile int32_t accumulated;
    volatile bool key_pressed;
} ec11_t;

void ec11_init(ec11_t *dev);
void ec11_on_exti(ec11_t *dev, uint16_t gpio_pin);
int8_t ec11_take_step(ec11_t *dev);
bool ec11_take_key_press(ec11_t *dev);

#ifdef __cplusplus
}
#endif

#endif
