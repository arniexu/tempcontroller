#ifndef STM32F1XX_H
#define STM32F1XX_H

#include <stdint.h>
#include "stm32f1xx_hal_def.h"

#ifdef __cplusplus
extern "C" {
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

typedef struct {
    __IO uint32_t CR1;
    __IO uint32_t CR2;
    __IO uint32_t SR;
    __IO uint32_t DR;
    __IO uint32_t CRCPR;
    __IO uint32_t RXCRCR;
    __IO uint32_t TXCRCR;
    __IO uint32_t I2SCFGR;
    __IO uint32_t I2SPR;
} SPI_TypeDef;

typedef struct {
    __IO uint32_t CR1;
    __IO uint32_t CR2;
    __IO uint32_t OAR1;
    __IO uint32_t OAR2;
    __IO uint32_t DR;
    __IO uint32_t SR1;
    __IO uint32_t SR2;
    __IO uint32_t CCR;
    __IO uint32_t TRISE;
} I2C_TypeDef;

#define GPIOA ((GPIO_TypeDef *)0x40010800UL)
#define GPIOB ((GPIO_TypeDef *)0x40010C00UL)
#define GPIOC ((GPIO_TypeDef *)0x40011000UL)
#define GPIOD ((GPIO_TypeDef *)0x40011400UL)
#define GPIOE ((GPIO_TypeDef *)0x40011800UL)
#define SPI1  ((SPI_TypeDef *)0x40013000UL)
#define I2C1  ((I2C_TypeDef *)0x40005400UL)

#ifndef __get_PRIMASK
static inline uint32_t __get_PRIMASK(void)
{
#if defined(__arm__) || defined(__thumb__)
    uint32_t primask = 0U;
    __asm volatile("MRS %0, primask" : "=r"(primask));
    return primask;
#else
    /* Non-ARM fallback: report interrupt-enabled state (PRIMASK bit cleared). */
    return 0U;
#endif
}
#endif

#ifndef __disable_irq
static inline void __disable_irq(void)
{
#if defined(__arm__) || defined(__thumb__)
    __asm volatile("cpsid i" : : : "memory");
#else
    /* Non-ARM fallback: no-op. */
#endif
}
#endif

#ifndef __enable_irq
static inline void __enable_irq(void)
{
#if defined(__arm__) || defined(__thumb__)
    __asm volatile("cpsie i" : : : "memory");
#else
    /* Non-ARM fallback: no-op. */
#endif
}
#endif

#ifdef __cplusplus
}
#endif

#endif
