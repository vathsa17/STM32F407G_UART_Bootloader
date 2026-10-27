#ifndef UTILS_H
#define UTILS_H

#include "stm32f407xx.h"


typedef enum
{
    BOOTLOADER_MODE_CMD,
    BOOTLOADER_MODE_METADATA,
    BOOTLOADER_MODE_FIRMWARE
} Bootloader_Mode_t;

static Bootloader_Mode_t bootloaderMode = BOOTLOADER_MODE_CMD;

Bootloader_Mode_t GetBootloaderMode(void);
void SetBootloaderMode(Bootloader_Mode_t Mode);
void delay_ms(uint32_t ms);
void simDelay(void);
void Bootloader_SendString(const char *str);
void ProcessCommand(const char *cmd);





#endif /* UTILS_H */
