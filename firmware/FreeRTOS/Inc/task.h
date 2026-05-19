#ifndef TASK_H
#define TASK_H

#include "FreeRTOS.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*TaskFunction_t)(void *arg);
typedef void *TaskHandle_t;

#ifndef tskIDLE_PRIORITY
#define tskIDLE_PRIORITY ((UBaseType_t)0U)
#endif

BaseType_t xTaskCreate(TaskFunction_t pxTaskCode,
                       const char *const pcName,
                       const uint16_t usStackDepth,
                       void *const pvParameters,
                       UBaseType_t uxPriority,
                       TaskHandle_t *const pxCreatedTask);
TickType_t xTaskGetTickCount(void);
void vTaskDelay(const TickType_t xTicksToDelay);
void vTaskDelayUntil(TickType_t *const pxPreviousWakeTime, const TickType_t xTimeIncrement);

#ifdef __cplusplus
}
#endif

#endif
