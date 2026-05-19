#ifndef TEMPCONTROLLER_BOARD_PINS_H
#define TEMPCONTROLLER_BOARD_PINS_H

#include "stm32f1xx_hal.h"

#ifdef __cplusplus
extern "C" {
#endif

#define OLED_I2C_HANDLE            hi2c1
#define OLED_I2C_ADDR_7BIT         0x3C
#define OLED_I2C_TIMEOUT_MS        100U

#define ADS1220_SPI_HANDLE         hspi1
#define ADS1220_CS_PORT            GPIOA
#define ADS1220_CS_PIN             GPIO_PIN_4
#define ADS1220_DRDY_PORT          GPIOB
#define ADS1220_DRDY_PIN           GPIO_PIN_0
#define ADS1220_RST_PORT           GPIOB
#define ADS1220_RST_PIN            GPIO_PIN_1

#define EC11_A_PORT                GPIOA
#define EC11_A_PIN                 GPIO_PIN_0
#define EC11_B_PORT                GPIOA
#define EC11_B_PIN                 GPIO_PIN_1
#define EC11_KEY_PORT              GPIOA
#define EC11_KEY_PIN               GPIO_PIN_2

#define EC11_EXTI_IRQ_A            EXTI0_IRQn
#define EC11_EXTI_IRQ_B            EXTI1_IRQn
#define EC11_EXTI_IRQ_KEY          EXTI2_IRQn

#ifdef __cplusplus
}
#endif

#endif
