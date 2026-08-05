/**
 ******************************************************************************
 * @file           : flash_program.h
 * @brief          : Header for flash_program.c file.
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

#ifndef __FLASH_IF_H
#define __FLASH_IF_H

#ifdef __cplusplus
extern "C"
{
#endif
#include "stm32f4xx_hal.h"

// For STM32F411 devices, sector 0-3 are normally reserved for bootloader
// Adjust these values according to the specific STM32F4 device used
#define APPLICATION_ADDRESS           0x08010000         // Application start address (start in Sector 4)
#define APPLICATION_FIRST_SECTOR      FLASH_SECTOR_4     // The first sector that contains the application binary
#define APPLICATION_LAST_SECTOR       FLASH_SECTOR_7     // The last sector that contains the application binary

/* Exported types ------------------------------------------------------------*/
typedef void (*pFunction)(void);

typedef enum
{
  IAP_SUCCESS = 0U,
  IAP_USB_ERROR,
  IAP_FLASH_ERROR,
  IAP_CRC_ERROR
} IAPErrorStatus;
 

/* Exported macros -----------------------------------------------------------*/
/* Exported functions ------------------------------------------------------- */
IAPErrorStatus USB_IAP_Process(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // _FLASH_IF_H