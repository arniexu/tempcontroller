#ifndef EVENT_GROUPS_H
#define EVENT_GROUPS_H

#include "FreeRTOS.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef uint32_t EventBits_t;
typedef void *EventGroupHandle_t;

EventGroupHandle_t xEventGroupCreate(void);
EventBits_t xEventGroupSetBits(EventGroupHandle_t xEventGroup, const EventBits_t uxBitsToSet);
EventBits_t xEventGroupWaitBits(EventGroupHandle_t xEventGroup,
                                const EventBits_t uxBitsToWaitFor,
                                const BaseType_t xClearOnExit,
                                const BaseType_t xWaitForAllBits,
                                TickType_t xTicksToWait);

#ifdef __cplusplus
}
#endif

#endif
