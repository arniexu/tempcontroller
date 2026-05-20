#ifndef FREERTOS_H
#define FREERTOS_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef int32_t BaseType_t;
typedef uint32_t UBaseType_t;
typedef uint32_t TickType_t;

#ifndef pdTRUE
#define pdTRUE  ((BaseType_t)1)
#endif

#ifndef pdFALSE
#define pdFALSE ((BaseType_t)0)
#endif

#ifndef pdPASS
#define pdPASS  pdTRUE
#endif

#ifndef pdFAIL
#define pdFAIL  pdFALSE
#endif

#ifndef configTICK_RATE_HZ
#define configTICK_RATE_HZ 1000U
#endif

#if (configTICK_RATE_HZ == 0U)
#error "configTICK_RATE_HZ must be greater than 0"
#endif

#if (configTICK_RATE_HZ > 1000U)
#error "configTICK_RATE_HZ above 1000 is not supported by this shim"
#endif

#ifndef portMAX_DELAY
#define portMAX_DELAY ((TickType_t)0xFFFFFFFFUL)
#endif

#ifndef portTICK_PERIOD_MS
#define portTICK_PERIOD_MS ((TickType_t)(1000U / (uint32_t)configTICK_RATE_HZ))
#endif

#define FREERTOS_MS_PER_SECOND 1000ULL
#define FREERTOS_TICK_ROUND_UP_BIAS (FREERTOS_MS_PER_SECOND - 1ULL)

#ifndef pdMS_TO_TICKS
#define pdMS_TO_TICKS(xTimeInMs)                                                                                              \
    ((TickType_t)((((uint64_t)(xTimeInMs) * (uint64_t)configTICK_RATE_HZ) >= ((uint64_t)portMAX_DELAY * FREERTOS_MS_PER_SECOND)) \
                     ? (uint64_t)portMAX_DELAY                                                                                \
                     : ((((uint64_t)(xTimeInMs) * (uint64_t)configTICK_RATE_HZ) + FREERTOS_TICK_ROUND_UP_BIAS) / FREERTOS_MS_PER_SECOND)))
#endif

#ifndef configASSERT
#define configASSERT(x) ((void)0)
#endif

#ifdef __cplusplus
}
#endif

#endif
