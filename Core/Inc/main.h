
/**
 ******************************************************************************
 * @file           : main.h
 * @brief          : Header for main.c file.
 *                   This file contains the common defines of the application.
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2026 STMicroelectronics.
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

#ifdef __cplusplus
extern "C"
{
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_hal.h"

  /* Private includes ----------------------------------------------------------*/

  /* Exported types ------------------------------------------------------------*/

  /* Exported constants --------------------------------------------------------*/

  /* Exported macro ------------------------------------------------------------*/
  extern volatile uint8_t USB_Drive_Ready_Flag;
  extern CRC_HandleTypeDef hcrc;

  /* Exported functions prototypes ---------------------------------------------*/
  void Error_Handler(void);

/* Private defines -----------------------------------------------------------*/
#define USB_VBUS_Pin GPIO_PIN_0
#define USB_VBUS_GPIO_Port GPIOC
#define USER_PUSH_BUTTON_Pin GPIO_PIN_0
#define USER_PUSH_BUTTON_GPIO_Port GPIOA
#define GREEN_LED_Pin GPIO_PIN_12
#define GREEN_LED_GPIO_Port GPIOD
#define ORANGE_LED_Pin GPIO_PIN_13
#define ORANGE_LED_GPIO_Port GPIOD
#define RED_LED_Pin GPIO_PIN_14
#define RED_LED_GPIO_Port GPIOD
#define BLUE_LED_Pin GPIO_PIN_15
#define BLUE_LED_GPIO_Port GPIOD

  /* USER CODE BEGIN Private defines */

  /* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
