#ifndef SEMPHR_H
#define SEMPHR_H

#include "queue.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef QueueHandle_t SemaphoreHandle_t;

SemaphoreHandle_t xSemaphoreCreateMutex(void);
BaseType_t xSemaphoreTake(SemaphoreHandle_t xSemaphore, TickType_t xTicksToWait);
BaseType_t xSemaphoreGive(SemaphoreHandle_t xSemaphore);

#ifdef __cplusplus
}
#endif

#endif
