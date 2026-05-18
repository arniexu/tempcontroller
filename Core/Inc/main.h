#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f1xx_hal.h"

#define LCD_CS_Pin GPIO_PIN_12
#define LCD_CS_GPIO_Port GPIOG
#define LCD_RS_A12_Pin GPIO_PIN_2
#define LCD_RS_A12_GPIO_Port GPIOG
#define LCD_BL_Pin GPIO_PIN_10
#define LCD_BL_GPIO_Port GPIOF

#define TOUCH_PEN_Pin GPIO_PIN_11
#define TOUCH_PEN_GPIO_Port GPIOF
#define TOUCH_CS_Pin GPIO_PIN_0
#define TOUCH_CS_GPIO_Port GPIOB
#define TOUCH_MOSI_Pin GPIO_PIN_1
#define TOUCH_MOSI_GPIO_Port GPIOB
#define TOUCH_MISO_Pin GPIO_PIN_2
#define TOUCH_MISO_GPIO_Port GPIOB
#define TOUCH_CLK_Pin GPIO_PIN_5
#define TOUCH_CLK_GPIO_Port GPIOA

#define SRAM_BANK3_BASE ((uint32_t)0x68000000U)
#define LCD_BANK4_BASE  ((uint32_t)0x6C000000U)
#define LCD_REG16       (*((volatile uint16_t *)(LCD_BANK4_BASE)))
#define LCD_RAM16       (*((volatile uint16_t *)(LCD_BANK4_BASE | (1U << (12 + 1))))) /* A12=1 for LCD data in 16-bit FSMC mode */

void Error_Handler(void);

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
