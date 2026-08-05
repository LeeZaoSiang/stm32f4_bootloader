/**
 ******************************************************************************
 * @file           : flash_program.c
 * @brief          : This file implements the functions for the main application
 *                   flashing process
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
#include "all.h"
static ErrorStatus Erase_Sectors(uint8_t firstSector, uint8_t lastSector);
static ErrorStatus Erase_Sector(uint8_t sector);
static ErrorStatus Write_Program(uint32_t *flashAddress, uint32_t *data, uint32_t dataLength);
static ErrorStatus Unlock_Flash();
static ErrorStatus Lock_Flash();
static ErrorStatus Verify_Program_CRC(const uint32_t *flashPtr, uint32_t fileSizeInBytes, uint32_t expectedCRC);
static uint32_t Retrieve_Original_CRC(uint32_t appFlashBaseAddress, uint32_t totalFileSizeInBytes);
static uint32_t Byte_To_Word_Count(uint32_t byteCount);

uint32_t ramBuffer[512]; // Word-aligned buffer array

/**
 * @brief  Retrieve the main application bin file from USB and
 *         flash it into memory.
 * @param  None
 * @retval ErrorStatus
 */
IAPErrorStatus USB_IAP_Process(void)
{
    // FRESULT fileAccessStatus;
    uint32_t flashPointer = APPLICATION_ADDRESS;
    UINT bytesRead;
    uint32_t totalBytesFlashed;
    uint32_t wordLength;
    uint32_t expectedCRC;
    IAPErrorStatus FWLoadStatus = IAP_SUCCESS;

    if (f_mount(&USBHFatFS, (TCHAR const *)USBHPath, 0U) == FR_OK)
    {
        if (f_open(&USBHFile, "image.bin", FA_READ) == FR_OK)
        {
            if (Unlock_Flash() == SUCCESS)
            {
                if (Erase_Sectors(APPLICATION_FIRST_SECTOR, APPLICATION_LAST_SECTOR) == SUCCESS)
                {
                    totalBytesFlashed = 0U;
                    do
                    {
                        // Clear buffer to ensure safe 0xFF padding for non-word-aligned files
                        memset(ramBuffer, 0xFF, sizeof(ramBuffer));
						
						if (f_read(&USBHFile, ramBuffer, sizeof(ramBuffer), &bytesRead) != FR_OK)
                        {
                            // FLash area may have flashed with new value at this stage, so considered Flash Error
                            FWLoadStatus = IAP_FLASH_ERROR;  
                            break;
                        }
						
                        // Check for End of File (EOF) immediately before processing data
                        if (bytesRead == 0U)
                        {
                            break; // File completely read, exit successfully
                        }

                        // Record total bytes
                        totalBytesFlashed += bytesRead;

                        // Convert total of read byte to word
                        wordLength = Byte_To_Word_Count(bytesRead);

                        if (Write_Program(&flashPointer, (uint32_t *)ramBuffer, wordLength) != SUCCESS)
                        {
                            FWLoadStatus = IAP_FLASH_ERROR;
                            break;
                        }

                    } while (bytesRead > 0U);

					// CRC here
                    if (FWLoadStatus == IAP_SUCCESS)
                    {
                        expectedCRC = Retrieve_Original_CRC(APPLICATION_ADDRESS, totalBytesFlashed);
					    if (Verify_Program_CRC((uint32_t *)APPLICATION_ADDRESS, totalBytesFlashed, expectedCRC) == ERROR)
						{
							FWLoadStatus = IAP_CRC_ERROR;
						}
                    }
                }
                else
                {
                    FWLoadStatus = IAP_FLASH_ERROR;
                }
                Lock_Flash();
                f_close(&USBHFile);
                f_mount(NULL, (TCHAR const *)USBHPath, 0U);
            }
            else
            {
                f_close(&USBHFile);
                f_mount(NULL, (TCHAR const *)USBHPath, 0U);
                // Flash areas remains untouched yet, so considered USB Error
                FWLoadStatus = IAP_USB_ERROR;
            }
        }
        else
        {
            f_mount(NULL, (TCHAR const *)USBHPath, 0U);
            FWLoadStatus = IAP_USB_ERROR;
        }
    }
    else
    {
        FWLoadStatus = IAP_USB_ERROR;
    }

    // Erase all the application sectors if flashing unsuccessful to prevent the MCU boots into a 
    // currupted memory during the next power on
    if (FWLoadStatus >= IAP_FLASH_ERROR)
    {
        if (Unlock_Flash() == SUCCESS)
        {
            if (Erase_Sectors(APPLICATION_FIRST_SECTOR, APPLICATION_LAST_SECTOR) != SUCCESS)
            {
                Error_Handler();
            }
            Lock_Flash();
        }
        else
        {
            Error_Handler();
        }
    }
    return FWLoadStatus;
}

/**
 * @brief  Erase multiple sectors.
 * @param  firstSector: The first sector of the memory to be erased
 * @param  lastSector: The last sector of the memory to be erased
 * @retval ErrorStatus
 */
static ErrorStatus Erase_Sectors(uint8_t firstSector, uint8_t lastSector)
{
    for (uint8_t sectorCount = firstSector; sectorCount <= lastSector; sectorCount++)
    {
        if (Erase_Sector(sectorCount) == ERROR)
        {
            return ERROR;
        }
    }
    return SUCCESS;
}

/**
 * @brief  Erase single sectors.
 * @param  sector: The sector number of the sector to be erased
 * @retval ErrorStatus
 */
static ErrorStatus Erase_Sector(uint8_t sector)
{
    FLASH_EraseInitTypeDef EraseInitStruct;
    uint32_t sectorError = 0;

    // On F411, sectors 4-7 are 128KB each
    EraseInitStruct.TypeErase = FLASH_TYPEERASE_SECTORS;
    EraseInitStruct.VoltageRange = FLASH_VOLTAGE_RANGE_3; // Valid for 3.3V VCC
    EraseInitStruct.Sector = sector;
    EraseInitStruct.NbSectors = 1; // Erase sector by sector as file streams in

    if (HAL_FLASHEx_Erase(&EraseInitStruct, &sectorError) != HAL_OK)
    {
        return ERROR; // Error code
    }
    return SUCCESS;
}

/**
 * @brief  Flash the program into memory.
 * @param  flashAddress: The start address of the memory for the main application
 * @param  data: The main application bin
 * @param  dataLength:
 * @retval ErrorStatus
 */
static ErrorStatus Write_Program(uint32_t *flashAddress, uint32_t *data, uint32_t dataLength)
{
    // Validate null pointers before proceeding
    if ((flashAddress == NULL) || (data == NULL))
    {
        return ERROR;
    }

    for (uint32_t dataCount = 0U; dataCount < dataLength; dataCount++)
    {
        // Dereference flashAddress to get the actual 32-bit memory address value
        if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, *flashAddress, data[dataCount]) == HAL_OK)
        {
            // Incrementing a uint32_t pointer value by 1 advances it by 4 bytes
            *flashAddress += 4U;
        }
        else
        {
            return ERROR;
        }
    }
    return SUCCESS;
}

/**
 * @brief  Unlock the flash memory
 * @retval ErrorStatus
 */
static ErrorStatus Unlock_Flash()
{
    if (HAL_FLASH_Unlock() == HAL_ERROR)
    {
        return ERROR;
    }
    return SUCCESS;
}

/**
 * @brief  Lock the flash memory
 * @retval ErrorStatus
 */
static ErrorStatus Lock_Flash()
{
    if (HAL_FLASH_Lock() == HAL_ERROR)
    {
        return ERROR;
    }
    return SUCCESS;
}

/**
 * @brief  Verifies the CRC of the bin (compare the local calculated CRC with the supplied CRC)
 * @param  startAddress: The starting Flash address of the user application (e.g., 0x08010000)
 * @param  totalFileSizeInBytes: The exact size of the binary file read from USB (e.g., 21736)
 * @param  expectedCRC: The pre-calculated CRC32 supplied with the bin (the last 4 bytes)
 * @return ErrorStatus: SUCCESS if checksum matches, ERROR if it fails
 */
static ErrorStatus Verify_Program_CRC(const uint32_t *startAddress, uint32_t totalFileSizeInBytes, uint32_t expectedCRC)
{
    // 1. Ensure the file is at least large enough to hold a CRC
    if (totalFileSizeInBytes < 4U)
    {
        return ERROR;
    }

    // 2. Get the actual app size in byte by subtracting the CRC bytes (last 4 bytes)
    uint32_t appSizeInBytes = totalFileSizeInBytes - 4U;

    // 3. Convert only the application payload bytes to 32-bit hardware words 
    // (Rounding up if necessary to match the Python zero-padding rules)
    uint32_t totalWords = Byte_To_Word_Count(appSizeInBytes);

    #if 0
    // 4. Reset and stream data to the STM32F411 hardware
    __HAL_RCC_CRC_CLK_ENABLE();
    CRC->CR |= CRC_CR_RESET;

    for (uint32_t i = 0; i < totalWords; i++)
    {
        CRC->DR = startAddress[i];
    }

    uint32_t calculatedCRC = CRC->DR;
    
    #else
    // 4. Calculate the actual application CRC
    uint32_t calculatedCRC = HAL_CRC_Calculate(&hcrc, (uint32_t *)startAddress, totalWords);
    #endif

    // 5. Final verification comparison
    if (calculatedCRC == expectedCRC)
    {
        return SUCCESS; // Verification passed!
    }
    
    return ERROR; // Mismatch detected.
}

/**
 * @brief  Retrieves the original CRC came with the bin
 * @param  startAddress: The starting Flash address of the user application (e.g., 0x08010000)
 * @param  totalFileSizeInBytes: The exact size of the binary file read from USB (e.g., 21736)
 * @return The original CRC
 */
static uint32_t Retrieve_Original_CRC(uint32_t startAddress, uint32_t totalFileSizeInBytes)
{
    // 1. Convert the size of the bin in byte to word
    uint32_t totalWords = Byte_To_Word_Count(totalFileSizeInBytes);
    
    // 2. Cast the flash base address to a 32-bit pointer
    const uint32_t *flashPtr = (const uint32_t *)startAddress;
    
    // 3. Read the very last word (index totalWords - 1)
    // Note: The original CRC comes with the bin must be embedded in the last word (last 4 bytes) of the bin
    uint32_t originalCRC = flashPtr[totalWords - 1U];
    
    return originalCRC;
}

/**
 * @brief  Convert byte count to word count
 * @param  byteCount: Byte count
 * @return Word count
 */
static uint32_t Byte_To_Word_Count(uint32_t byteCount) 
{
    return (byteCount / 4U) + ((byteCount % 4U) != 0U);
}