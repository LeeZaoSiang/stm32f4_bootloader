/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : Main program body
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

/* Includes ------------------------------------------------------------------*/
#include "all.h"

/* Private includes ----------------------------------------------------------*/

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

/* Private macro -------------------------------------------------------------*/



/* Private variables ---------------------------------------------------------*/
volatile uint8_t USB_Drive_Ready_Flag = 0;
CRC_HandleTypeDef hcrc;

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
void MX_USB_HOST_Process(void);
static void StopUSBH(void);
static void Launch_Application(void);
static void MX_CRC_Init(void);

/* Private user code ---------------------------------------------------------*/

/**
 * @brief  The application entry point.
 * @retval int
 */
int main(void)
{
  uint32_t initial_msp = *(__IO uint32_t *)APPLICATION_ADDRESS;

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* Configure the system clock */
  SystemClock_Config();

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_FATFS_Init();
  MX_USB_HOST_Init();
  MX_CRC_Init();
  
  // Check if user button has not been pressed
  if (HAL_GPIO_ReadPin(USER_PUSH_BUTTON_GPIO_Port, USER_PUSH_BUTTON_Pin) == GPIO_PIN_RESET)
  {
    // Check if the main program has been programmed
    // Explicitly verify if the Stack Pointer sits between 0x20000000 and 0x20020000
    if (initial_msp >= 0x20000000 && initial_msp <= 0x20020000)
    {
      StopUSBH();
      Launch_Application(); // Go to the main application directly
    }
  }

  while (1)
  {
    MX_USB_HOST_Process();
    if (USB_Drive_Ready_Flag == 1U)
    {
      if (USB_IAP_Process() < IAP_FLASH_ERROR)  // If there is a failure and it's not related to flashing, continue to boot
      {
        // HAL_GPIO_WritePin(GPIOD, GPIO_PIN_15, GPIO_PIN_SET);
        // Check if the main program has been programmed
        // Explicitly verify if the Stack Pointer sits between 0x20000000 and 0x20020000
        if (initial_msp >= 0x20000000 && initial_msp <= 0x20020000)
        {
          StopUSBH();
		      // HAL_GPIO_WritePin(GPIOD, GPIO_PIN_15, GPIO_PIN_SET);  // Blue LED (for testing only)
          Launch_Application(); // Go to the main application directly
        }
      }
      else
      {
        StopUSBH();
        HAL_GPIO_WritePin(GPIOD, GPIO_PIN_14, GPIO_PIN_SET);  // Turn on RED LED to indicate IAP Process failure
        // Main application flashing failure, implement next action here
      }
      USB_Drive_Ready_Flag = 0U;
    }
    // Flash the orange LED here to indicate that the bootloader is running
    HAL_GPIO_TogglePin(GPIOD, GPIO_PIN_13);
    HAL_Delay(100);
  }
  return 0U;
}

/**
 * @brief Leave bootloader and jump to actual application
 * @param None
 * @retval None
 */
static void Launch_Application(void)
{
  // Define the function pointer to the reset handler
  typedef void (*pFunction)(void);
  pFunction JumpToApplication = (pFunction) * (__IO uint32_t *)(APPLICATION_ADDRESS + 4);

  // Shut down Systick Timer to prevent unexpected interrupts
  SysTick->CTRL = 0;
  SysTick->LOAD = 0;
  SysTick->VAL = 0;

  // De-initialize all peripherals to default states before jumping
  HAL_RCC_DeInit();
  HAL_DeInit();

  // 3. Set the Main Stack Pointer to the application's stack pointer
  __set_MSP(*(__IO uint32_t *)APPLICATION_ADDRESS);

  // 4. Make the jump
  JumpToApplication();
}

/**
 * @brief Stop USBH
 * @param None
 * @retval None
 */
static void StopUSBH(void)
{
  // De-initialize the USB Host
  USBH_DeInit(&hUsbHostFS);

  // Disable the global USB OTG Interrupt
  HAL_NVIC_DisableIRQ(OTG_FS_IRQn); // Use OTG_HS_IRQn if using High Speed

  // Turn off VBus
  USBH_Stop(&hUsbHostFS);
}

/**
 * @brief System Clock Configuration
 * @retval None
 */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
   */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
   * in the RCC_OscInitTypeDef structure.
   */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 4;
  RCC_OscInitStruct.PLL.PLLN = 72;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 3;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
   */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
 * @brief GPIO Initialization Function
 * @param None
 * @retval None
 */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(USB_VBUS_GPIO_Port, USB_VBUS_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOD, GREEN_LED_Pin | ORANGE_LED_Pin | RED_LED_Pin | BLUE_LED_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : USB_VBUS_Pin */
  GPIO_InitStruct.Pin = USB_VBUS_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(USB_VBUS_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : USER_PUSH_BUTTON_Pin */
  GPIO_InitStruct.Pin = USER_PUSH_BUTTON_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(USER_PUSH_BUTTON_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : GREEN_LED_Pin ORANGE_LED_Pin RED_LED_Pin BLUE_LED_Pin */
  GPIO_InitStruct.Pin = GREEN_LED_Pin | ORANGE_LED_Pin | RED_LED_Pin | BLUE_LED_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);
}

/**
 * @brief CRC Initialization Function
 * @param None
 * @retval None
 */
static void MX_CRC_Init(void)
{
  hcrc.Instance = CRC;
  if (HAL_CRC_Init(&hcrc) != HAL_OK)
  {
    Error_Handler();
  }
}


/**
 * @brief  This function is executed in case of error occurrence.
 * @retval None
 */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
 * @brief  Reports the name of the source file and the source line number
 *         where the assert_param error has occurred.
 * @param  file: pointer to the source file name
 * @param  line: assert_param error line source number
 * @retval None
 */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
