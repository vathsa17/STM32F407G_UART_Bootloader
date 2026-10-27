#include "stm32f407xx.h"

void delay_ms(uint32_t ms)
{
	SYSTICK_LOAD= 16000 - 1;     // 1 ms @ 16 MHz
    SYSTICK_VAL  = 0;
    SYSTICK_CTRL = 5;             // Enable, processor clock, no interrupt

    for (uint32_t i = 0; i < ms; i++)
    {
        while ((SYSTICK_CTRL & (1U << 16)) == 0);
    }

    SYSTICK_CTRL = 0;
}

/**
 * @brief Function Introduses a Simulated Delay
 *
 */
void simDelay(void)
{
	uint32_t delayCounter;
	for(delayCounter=0;delayCounter<100000;delayCounter++)
	{

	}

}

void Bootloader_SendString(const char *str)
{
    USART_Transmit(USART2,
                   (uint8_t *)str,
                   (uint8_t)strlen(str));
}


void ProcessCommand(const char *cmd)
{
    if (strcmp(cmd, "PING") == 0)
    {
    	Bootloader_SendString("PONG\r\n");
    }
    else if (strcmp(cmd, "INFO") == 0)
    {
    	Bootloader_SendString(
            "\r\nSTM32F407G-DISC1\r\n"
            "Bootloader version: 0.1.0\r\n"
            "Transport: USART2\r\n"
            "Flash operations: disabled\r\n");
    }
    else if (strcmp(cmd, "HELP") == 0)
    {
        Bootloader_SendString("\r\nSupported commands:\r\n"
            "PING - Test UART communication\r\n"
            "INFO - Display bootloader information\r\n"
            "HELP - List supported commands\r\n");
    }
    else if (strcmp(cmd, "BOOT") == 0)
    {
        if (Bootloader_IsApplicationValid())
        {

            Bootloader_SendString("Booting application...\r\n");

            /* Wait until the final UART byte is transmitted */
            while ((USART2->SR & USART_SR_TC) == 0U)
            {
            }

            Bootloader_JumpToApplication();
            }
        else
        {
            Bootloader_SendString("ERR: Invalid application\r\n");
        }


    }
    else if (strcmp(cmd, "ERASE") == 0)
    {
        Bootloader_SendString("Erasing application...\r\n");
        Flash_Status_t status = Flash_EraseApplication();
        if (status == FLASH_OK)
        {
            Bootloader_SendString("ERASE OK\r\n");
        }
        else
        {
            Bootloader_SendString("ERASE FAILED\r\n");
        }
    }
    else if (strcmp(cmd, "FLASHTEST") == 0)
    {
        Flash_Status_t status = Flash_TestUnlockLock();

        if (status == FLASH_OK)
        {
        	Bootloader_SendString("FLASH TEST OK\r\n");
        }
        else
        {
        	Bootloader_SendString("FLASH TEST FAILED\r\n");

        }
    }
    else if(strcmp(cmd, "BEGIN") == 0)
    {
        Bootloader_SendString("OK\r\n");
        SetBootloaderMode(BOOTLOADER_MODE_METADATA);  
    }
    else if (strcmp(cmd, "WRITE") == 0)
    {
        Bootloader_SendString("Writing application...\r\n");
        //Bootloader_WriteApplication();
        Bootloader_SendString("Application written.\r\n");  
    }
    else if (strcmp(cmd, "VERIFY") == 0)
    {
        Bootloader_SendString("Verifying application...\r\n");
        //Bootloader_VerifyApplication();
        Bootloader_SendString("Application verified.\r\n");
    }
    else
    {
        Bootloader_SendString("ERR: Unknown command. Send HELP.\r\n");
    }
}


void SetBootloaderMode(Bootloader_Mode_t Mode)
{
    bootloaderMode=Mode;
}

Bootloader_Mode_t GetBootloaderMode(void)
{
    return bootloaderMode;
}
