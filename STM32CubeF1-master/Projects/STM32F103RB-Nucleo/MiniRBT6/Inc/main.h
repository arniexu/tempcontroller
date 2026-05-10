/**
  ******************************************************************************
  * @file    Templates/Inc/main.h 
  * @author  MCD Application Team
  * @brief   Header for main.c module
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2016 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
  
/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

/* Includes ------------------------------------------------------------------*/
#include "stm32f1xx_hal.h"

/* User board LED pins */
#define LED0_Pin        GPIO_PIN_8
#define LED0_GPIO_Port  GPIOA

#define LED1_Pin        GPIO_PIN_2
#define LED1_GPIO_Port  GPIOD

/* 8080 LCD: PB0..PB15 are D0..D15, PC10..PC6 are BL/CS/RS/WR/RD */
#define LCD_DATA_GPIO_Port GPIOB

#define LCD_BL_Pin       GPIO_PIN_10
#define LCD_CS_Pin       GPIO_PIN_9
#define LCD_RS_Pin       GPIO_PIN_8
#define LCD_WR_Pin       GPIO_PIN_7
#define LCD_RD_Pin       GPIO_PIN_6
#define LCD_CTRL_GPIO_Port GPIOC
/* Exported types ------------------------------------------------------------*/
/* Exported constants --------------------------------------------------------*/
/* Exported macro ------------------------------------------------------------*/
/* Exported functions ------------------------------------------------------- */

#endif /* __MAIN_H */
