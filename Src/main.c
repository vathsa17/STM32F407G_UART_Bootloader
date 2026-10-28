#include "stm32f407xx.h"
#include <string.h>

#define CMD_LEN 10


char txMsg[]="\r\nSTM32F407 UART Bootloader v0.1.0. Type HELP for commands\r\n";
volatile uint8_t RingBufferOverflow = 0;
RingBuffer_t RingBuffer;


char cmd_buffer[CMD_LEN];
uint8_t cmd_length=0;



/*UART Configuration*/
USART_Conf_t USART2_Conf;
/**
 * @brief Function to Initialize USART2 for communication with the GNSS-UART Device. 
 * This function configures the GPIO pins, sets up the USART parameters, and enables the necessary interrupts.
 * 
 */
void USART2_Init(void)
{
	GPIO_PinConf_t USART_Pin;
	USART_Pin.GPIO_PinMode=GPIO_MODE_ALT;
	USART_Pin.GPIO_PUPD=GPIO_NO_PUPD;
	USART_Pin.GPIO_OutType=GPIO_OutType_PP;
	USART_Pin.GPIO_OutSpeed=GPIO_OutSpeed_Fast;
	USART_Pin.GPIO_AltFnc=GPIO_AF7;
	GPIOA_CLK_ENB();

	USART_Pin.GPIO_PinNumber=GPIO_PIN_NUM_2;
	GPIO_Init(GPIOA,USART_Pin);
	USART_Pin.GPIO_PinNumber=GPIO_PIN_NUM_3;
	GPIO_Init(GPIOA,USART_Pin);

	USART2_Conf.Mode=USART_MODE_RX_TX;
	USART2_Conf.Parity=USART_PARITY_NONE;
	USART2_Conf.StopBits=USART_STOPBITS_1;
	USART2_Conf.WordLenght=USART_WORDLENGTH_8B;
	USART2_Conf.OverSampleing=0U; /*OverSampling by 16*/
	USART2_Conf.BaudRate=USART_BAUDRATE_9600;
	USART2_CLK_ENB();
	USART_Init(USART2,USART2_Conf);

	
	NVIC_SetPriority(IRQ_NO_USART2,0U);
	NVIC_EnableIRQ(IRQ_NO_USART2);
	USART2_RXNEIE_ENB();
	




}

/**
 * @brief Function to Initialize the Blue LED on the STM32F407G-DISCOVERY Board. 
 * This function configures the GPIO pin connected to the Blue LED as an output pin.
 * 
 */
void BlueLED_Init()
{
	GPIO_PinConf_t GPIOD_PinConf = {GPIO_PIN_NUM_15,GPIO_MODE_OUTPUT,GPIO_OutType_PP,GPIO_OutSpeed_Low,GPIO_NO_PUPD};

	// Enable clock for GPIOD
    //RCC_AHB1ENR |= (1 << 3);
	GPIOD_CLK_ENB();

	GPIO_Init(GPIOD,GPIOD_PinConf);


	GPIOD_PinConf.GPIO_PinNumber=GPIO_PIN_NUM_14;
	GPIO_Init(GPIOD,GPIOD_PinConf);
}

/**
 * @brief Function to enable the FPU.
 * This function enables the Floating Point Unit (FPU) for floating-point operations.
 */
static void FPU_Enable(void)
{
    *(volatile uint32_t *)0xE000ED88U |= (0xFU << 20U);
    __asm volatile ("dsb");
    __asm volatile ("isb");
}



/**
 * @brief The Main Function of the Driver
 *
 * @return int
 */

int main(void)
{
    FPU_Enable();
    RingBuffer_Init(&RingBuffer);
    USART2_Init();
    BlueLED_Init();

    USART_Transmit(USART2, (uint8_t *)txMsg,sizeof(txMsg) - 1U);
    Protocol_Init();
    while (1)
    {
        uint8_t data;
        if (RingBuffer_Get(&RingBuffer, &data))
        {   
            Bootloader_Mode_t bootloaderMode=GetBootloaderMode();
            switch(bootloaderMode)
            {
                case BOOTLOADER_MODE_CMD:
                                    if (data == '\r')
                                    {
                                        /* Ignore carriage return */
                                        continue;
                                    }
                                    if (data == '\n')
                                    {
                                        cmd_buffer[cmd_length] = '\0';
                                        ProcessCommand(cmd_buffer);
                                        cmd_length=0;
                                    }
                                    else if (cmd_length < (CMD_LEN - 1U))
                                    {
                                        cmd_buffer[cmd_length++] = (char)data;
                                    }
                                    else
                                    {
                                        /* Discard an oversized command */
                                        cmd_length = 0U;
                                        Bootloader_SendString("ERR: Command too long\r\n");
                                        SetBootloaderMode(BOOTLOADER_MODE_CMD);  
                                    }
                                    break;
                case BOOTLOADER_MODE_METADATA:
                                    Protocol_Result_t status = Protocol_ProcessByte(data);
                                    if(status==PROTOCOL_PACKET_READY)
                                    {
                                        BootPacket_t *packet = Protocol_GetPacket();
                                        
                                        if(Protocol_ValidatePacket(packet) == METADATA_STATUS_OK)
                                        {
                                            MetadataStatus_t MetaStatus=Bootloader_ProcessMetadata(packet);
                                            if(MetaStatus==METADATA_STATUS_OK)
                                            {
                                                TransferStatus_t transferStatus =Bootloader_PrepareFirmwareTransfer();
                                                if(transferStatus==TRANSFER_OK)
                                                {
                                                    Protocol_Init();
                                                    NVIC_DisableIRQ(IRQ_NO_USART2);
                                                    Protocol_Init();
                                                    SetBootloaderMode(BOOTLOADER_MODE_FIRMWARE);
                                                    RingBuffer_Init(&RingBuffer);
                                                    NVIC_EnableIRQ(IRQ_NO_USART2);


                                                    Bootloader_SendString("READY\r\n");

                                                }
                                                else
                                                {
                                                    Bootloader_SendString("FLASH ERROR\r\n");
                                                }
                                            } 
                                        }
                                        else
                                        {
                                            Bootloader_SendString("MetaData Validation Error\r\n");
                                            SetBootloaderMode(BOOTLOADER_MODE_CMD);  
                                        }
                                    }
                                    else if(status==PROTOCOL_INVALID)
                                    {
                                        Bootloader_SendString("ERR: Start Again\r\n");
                                        SetBootloaderMode(BOOTLOADER_MODE_CMD);  
                                    }
                                    break;
                case BOOTLOADER_MODE_FIRMWARE:
                                    Protocol_Result_t status_ProcessBytes = Protocol_ProcessByte(data);
                                    if(status_ProcessBytes==PROTOCOL_PACKET_READY)
                                    {
                                        BootPacket_t *Datapacket = Protocol_GetPacket();
                                        if(Protocol_ValidatePacket(Datapacket) == METADATA_STATUS_OK)
                                        {
                                            DataStatus_t DataStatus=Bootloader_ProcessData(Datapacket);
                                            if (DataStatus == DATA_OK)
                                            {
                                                Bootloader_SendString("ACK\r\n");
                                            }
                                            else
                                            {
                                                Bootloader_SendString("NAK DATA\r\n");
                                            }
                                        }
                                        else
                                        {
                                            Bootloader_SendString("Application Validation Error\r\n");
                                            SetBootloaderMode(BOOTLOADER_MODE_CMD);  
                                        }
                                    }
                                    else if(status_ProcessBytes==PROTOCOL_INVALID)
                                    {
                                        Bootloader_SendString("ERR: Start Again\r\n");
                                        SetBootloaderMode(BOOTLOADER_MODE_CMD);  
                                    }
                                    break;    
                default:
                                    break;
            }
        
        }
    }
}


/**
 * @brief Service Routine for USART2 Interrupts. This function is called when an interrupt occurs on USART2.
 * 
 */

void USART2_IRQHandler(void)
{

	if(USART2->SR & (1U<<5U)) //Check if RXNE Flag is Set
	{
		uint8_t RxData;
		RxData=USART2->DR; //Read the Data from DR Register
		if (!RingBuffer_Put(&RingBuffer, RxData))
        {
            RingBufferOverflow++;
        }
	}
}




