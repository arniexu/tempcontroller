#ifndef PROJECT_CONFIG_SELECT_H
#define PROJECT_CONFIG_SELECT_H

#if defined(APP_PROJECT_EDGEGATEWAY)
#include "project_configs/project_edgegateway.h"
#elif defined(APP_PROJECT_STM32F103)
#include "project_configs/project_stm32f103.h"
#else
#include "project_configs/project_default.h"
#endif

#endif
