#ifndef STM32F1XX_H
#define STM32F1XX_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef __IO
#define __IO volatile
#endif

typedef enum {
    EXTI0_IRQn = 6,
    EXTI1_IRQn = 7,
    EXTI2_IRQn = 8
} IRQn_Type;

typedef struct {
    __IO uint32_t CRL;
    __IO uint32_t CRH;
    __IO uint32_t IDR;
    __IO uint32_t ODR;
    __IO uint32_t BSRR;
    __IO uint32_t BRR;
    __IO uint32_t LCKR;
} GPIO_TypeDef;

#define GPIOA ((GPIO_TypeDef *)0x40010800UL)
#define GPIOB ((GPIO_TypeDef *)0x40010C00UL)

#ifndef __get_PRIMASK
static inline uint32_t __get_PRIMASK(void)
{
    return 0U;
}
#endif

#ifndef __disable_irq
static inline void __disable_irq(void)
{
}
#endif

#ifndef __enable_irq
static inline void __enable_irq(void)
{
}
#endif

#ifdef __cplusplus
}
#endif

#endif
