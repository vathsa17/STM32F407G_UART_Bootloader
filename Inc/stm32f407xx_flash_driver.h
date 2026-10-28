
#ifndef FLASH_DRIVER_H
#define FLASH_DRIVER_H

#include <stm32f407xx.h>

/* STM32F407VG application Flash region */
#define APP_FLASH_START       0x08020000UL
#define APP_FLASH_END         0x08100000UL

#define FLASH_APP_FIRST_SECTOR  5U
#define FLASH_APP_LAST_SECTOR   11U

typedef enum
{
    FLASH_OK = 0,
    FLASH_ERROR_BUSY_TIMEOUT,
    FLASH_ERROR_OPERATION,
    FLASH_ERROR_INVALID_SECTOR,
    FLASH_ERROR_UNLOCK,
    FLASH_ERROR_INVALID_ADDRESS,
    FLASH_ERROR_INVALID_ARGUMENT
} Flash_Status_t;

Flash_Status_t Flash_EraseApplication(void);
Flash_Status_t Flash_TestUnlockLock(void);
Flash_Status_t Flash_ProgramBytes(uint32_t address,const uint8_t *data, uint32_t length);
#endif
