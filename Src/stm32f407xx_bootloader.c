/*
 * stm32f407xx_bootloader.c
 *
 *  Created on: 27 Oct 2026
 *      Author: shriv
 */

#include "stm32f407xx.h"



static BootPacket_t rxPacket;
static ProtocolRxState_t rxState = RX_WAIT_SOF;
FirmwareTransfer_t transfer = {0};


static uint16_t payloadIndex;
static uint8_t lengthLow;
static uint8_t crcLow;

bool Bootloader_IsApplicationValid(void)
{
    uint32_t appStackPointer =
        *(volatile uint32_t *)APP_ADDRESS;

    uint32_t appResetHandler =
        *(volatile uint32_t *)(APP_ADDRESS + 4U);

    /* Check that the initial stack pointer is within SRAM */
    if ((appStackPointer < SRAM1_START) ||
        (appStackPointer > SRAM1_END) ||
        ((appStackPointer & 0x3U) != 0U))
    {
        return false;
    }

    /* The reset handler must point into the application region */
    uint32_t resetAddress = appResetHandler & ~1UL;

    if (((appResetHandler & 1U) == 0U) ||
        (resetAddress < APP_ADDRESS) ||
        (resetAddress >= 0x08100000UL))
    {
        return false;
    }

    return true;
}

static inline void __disable_irq(void)
{
    __asm volatile ("cpsid i" ::: "memory");
}

static inline void __set_MSP(uint32_t address)
{
    __asm volatile ("msr msp, %0" :: "r" (address) : "memory");
}

static inline void DSB(void)
{
    __asm volatile ("dsb 0xF" ::: "memory");
}

static inline void ISB(void)
{
    __asm volatile ("isb 0xF" ::: "memory");
}


void Bootloader_JumpToApplication(void)
{
    uint32_t app_stack;
    uint32_t app_reset;
    ApplicationEntry_t app_entry;

    /* Read the first two entries of the application vector table */
    app_stack = *(volatile uint32_t *)APP_ADDRESS;
    app_reset = *(volatile uint32_t *)(APP_ADDRESS + 4U);

    /* Validate the initial stack pointer (top of SRAM1 is allowed) */
    if ((app_stack < SRAM1_START) ||
        (app_stack > SRAM1_END) ||
        ((app_stack & 0x7U) != 0U))
    {
        return;
    }

    /* Validate that the reset handler points into application Flash */
    if ((app_reset < APP_ADDRESS) ||
        (app_reset >= APP_END_ADDRESS) ||
        ((app_reset & 1U) == 0U))
    {
        return;
    }

    app_entry = (ApplicationEntry_t)app_reset;

    /*
     * Stop bootloader interrupts before handing over control.
     * This initial implementation assumes the bootloader uses SysTick
     * and USART2, but no other active peripherals or DMA channels.
     */
    __disable_irq();

    SysTick->CTRL = 0U;
    SysTick->LOAD = 0U;
    SysTick->VAL  = 0U;

    /* Disable and clear pending external interrupts */
    for (uint32_t i = 0U; i < 8U; i++)
    {
        NVIC->ICER[i] = 0xFFFFFFFFU;
        NVIC->ICPR[i] = 0xFFFFFFFFU;
    }

    /* Relocate the interrupt vector table to the application */
    SCB_VTOR  = APP_ADDRESS;
    DSB();
    ISB();

    /* Set the application's initial stack pointer */
    __set_MSP(app_stack);

    /* The application's startup code will initialize its environment */
    app_entry();

    /* The application should never return */
    while (1)
    {
    }
}

void Protocol_Init(void)
{
    rxState = RX_WAIT_SOF;

    payloadIndex = 0U;
    lengthLow = 0U;
    crcLow = 0U;

    rxPacket.command = 0U;
    rxPacket.length = 0U;
    rxPacket.crc = 0U;
}



volatile uint32_t protocolTrace[32];
volatile uint32_t protocolTraceIndex = 0U;


Protocol_Result_t Protocol_ProcessByte(uint8_t byte)
{

    if (protocolTraceIndex < 32U)
    {
        protocolTrace[protocolTraceIndex] = byte;
        protocolTraceIndex++;
    }
    switch (rxState)
    {
        case RX_WAIT_SOF:
        {
            if (byte == PROTOCOL_SOF)
            {
                rxState = RX_COMMAND;
            }
            break;
        }

        case RX_COMMAND:
        {
            rxPacket.command = byte;
            rxState = RX_LENGTH_LOW;
            break;
        }

        case RX_LENGTH_LOW:
        {
            lengthLow = byte;
            rxState = RX_LENGTH_HIGH;
            break;
        }

        case RX_LENGTH_HIGH:
        {
            rxPacket.length =
                (uint16_t)lengthLow |
                ((uint16_t)byte << 8U);

            /* Prevent writing beyond the payload array. */
            if (rxPacket.length > PROTOCOL_MAX_PAYLOAD)
            {
                rxState = RX_WAIT_SOF;
                payloadIndex = 0U;

                return PROTOCOL_INVALID;
            }

            payloadIndex = 0U;

            if (rxPacket.length == 0U)
            {
                rxState = RX_CRC_LOW;
            }
            else
            {
                rxState = RX_PAYLOAD;
            }

            break;
        }

        case RX_PAYLOAD:
        {
            rxPacket.payload[payloadIndex] = byte;
            payloadIndex++;

            if (payloadIndex >= rxPacket.length)
            {
                rxState = RX_CRC_LOW;
            }

            break;
        }

        case RX_CRC_LOW:
        {
            crcLow = byte;
            rxState = RX_CRC_HIGH;
            break;
        }

        case RX_CRC_HIGH:
        {
            rxPacket.crc =
                (uint16_t)crcLow |
                ((uint16_t)byte << 8U);

            /* Complete frame assembled. */
            rxState = RX_WAIT_SOF;
            payloadIndex = 0U;

            return PROTOCOL_PACKET_READY;
        }

        default:
        {
            /* Recover from an unexpected parser state. */
            rxState = RX_WAIT_SOF;
            payloadIndex = 0U;

            return PROTOCOL_INVALID;
        }
    }

    return PROTOCOL_INCOMPLETE;
}


BootPacket_t *Protocol_GetPacket(void)
{
    return &rxPacket;
}



MetadataStatus_t Protocol_ValidatePacket(const BootPacket_t *packet)
{
    uint8_t crcInput[3U + PROTOCOL_MAX_PAYLOAD];
    uint32_t crcInputLength;
    uint16_t calculatedCrc;

    if (packet == 0)
    {
        return METADATA_STATUS_ERROR;
    }

    /* Reject an invalid payload length before accessing the payload. */
    if (packet->length > PROTOCOL_MAX_PAYLOAD)
    {
        return METADATA_STATUS_ERROR;
    }

    /* Reconstruct CMD + LEN in wire format. */
    crcInput[0] = packet->command;
    crcInput[1] = (uint8_t)(packet->length & 0xFFU);
    crcInput[2] = (uint8_t)((packet->length >> 8U) & 0xFFU);

    /* Append the payload. */
    for (uint16_t i = 0U; i < packet->length; i++)
    {
        crcInput[3U + i] = packet->payload[i];
    }

    crcInputLength = 3U + packet->length;

    /* Calculate CRC over CMD + LEN + PAYLOAD. */
    calculatedCrc = CRC16_Calculate(crcInput, crcInputLength);

    /* Compare against the CRC received from UART. */
    if (calculatedCrc == packet->crc) 
    { 
        
        return METADATA_STATUS_OK;
     }
     else
     { 
        Bootloader_SendString("CRC Not Matching\r\n");
        return METADATA_STATUS_ERROR;
        
     }
}


MetadataStatus_t Bootloader_ProcessMetadata(const BootPacket_t *packet)
{
    uint32_t imageSize;
    uint32_t imageCrc32;

    if (packet == 0)
    {
        return METADATA_STATUS_ERROR;
    }

    if (GetBootloaderMode() != BOOTLOADER_MODE_METADATA)
    {
        return METADATA_STATUS_ERROR;
    }

    if (packet->command != CMD_METADATA)
    {
        return METADATA_STATUS_ERROR;
    }

    if (packet->length != 8U)
    {
        return METADATA_STATUS_ERROR;
    }

    /* Decode image size: bytes 0–3, little-endian. */
    imageSize =
        ((uint32_t)packet->payload[0])       |
        ((uint32_t)packet->payload[1] << 8U)  |
        ((uint32_t)packet->payload[2] << 16U) |
        ((uint32_t)packet->payload[3] << 24U);

    /* Decode image CRC-32: bytes 4–7, little-endian. */
    imageCrc32 =
        ((uint32_t)packet->payload[4])       |
        ((uint32_t)packet->payload[5] << 8U)  |
        ((uint32_t)packet->payload[6] << 16U) |
        ((uint32_t)packet->payload[7] << 24U);

    /* Validate image size before any Flash operation. */
    if ((imageSize == 0U) || (imageSize > APP_MAX_SIZE))
    {
        return METADATA_STATUS_ERROR;
    }

    /* Store validated metadata for the upcoming transfer. */
    transfer.imageSize = imageSize;
    transfer.expectedImageCrc32 = imageCrc32;
    transfer.expectedOffset = 0U;
    transfer.metadataValid = 1U;
    transfer.lastPacketOffset = 0U;
    transfer.lastPacketLength = 0U;
    transfer.lastPacketValid = 0U;
    transfer.active = 0U;

    return METADATA_STATUS_OK;
}


TransferStatus_t Bootloader_PrepareFirmwareTransfer(void)
{
    Flash_Status_t flashStatus;

    if (transfer.metadataValid != 1U)
    {
        return TRANSFER_ERR_METADATA;
    }

    if (GetBootloaderMode() != BOOTLOADER_MODE_METADATA)
    {
        return TRANSFER_ERR_METADATA;
    }

    /*
     * Erase application sectors 5–11 only.
     * Flash_EraseApplication() must protect bootloader sectors 0–4.
     */
    flashStatus = Flash_EraseApplication();

    if (flashStatus != FLASH_OK)
    {
        transfer.metadataValid = 0U;
        transfer.active = 0U;

        return TRANSFER_ERR_FLASH;
    }

    /* Ready to receive firmware DATA packets. */
    transfer.expectedOffset = 0U;
    transfer.active = 1U;
    

    return TRANSFER_OK;
}

DataStatus_t Bootloader_ProcessData(const BootPacket_t *packet)
{
    uint32_t offset;
    uint32_t dataLength;
    uint32_t remaining;
    uint32_t destinationAddress;
    Flash_Status_t flashStatus;

    if (packet == 0)
    {
        return DATA_ERR_NULL;
    }

    if ((GetBootloaderMode() != BOOTLOADER_MODE_FIRMWARE) ||
        (transfer.active != 1U) ||
        (transfer.metadataValid != 1U))
    {
        return DATA_ERR_MODE;
    }

    if (packet->command != CMD_DATA)
    {
        return DATA_ERR_COMMAND;
    }

    /*
     * Payload:
     *   Bytes 0..3: firmware offset (little-endian)
     *   Bytes 4..N: firmware data
     *
     * Require at least one data byte.
     */
    if ((packet->length <= 4U) ||
        (packet->length > PROTOCOL_MAX_PAYLOAD))
    {
        return DATA_ERR_LENGTH;
    }

    offset =
        ((uint32_t)packet->payload[0])       |
        ((uint32_t)packet->payload[1] << 8U)  |
        ((uint32_t)packet->payload[2] << 16U) |
        ((uint32_t)packet->payload[3] << 24U);

    dataLength = (uint32_t)packet->length - 4U;

    if ((transfer.lastPacketValid == 1U) &&
    (offset == transfer.lastPacketOffset) &&
    (dataLength == transfer.lastPacketLength))
    {
        /*
        * This matches the previous packet's offset and length.
        * Do not program Flash again.
        */
        return DATA_OK;
    }

    /* Require strictly sequential DATA packets. */
    if (offset != transfer.expectedOffset)
    {
        return DATA_ERR_OFFSET;
    }




    /* Ensure the packet fits inside the declared image. */
    if (offset > transfer.imageSize)
    {
        return DATA_ERR_SIZE;
    }

    remaining = transfer.imageSize - offset;

    if (dataLength > remaining)
    {
        return DATA_ERR_SIZE;
    }

    destinationAddress = APP_FLASH_START + offset;

    /*
     * The Flash driver currently requires a 4-byte-aligned
     * destination address. Since APP_FLASH_START is aligned,
     * sequential offsets must also be aligned.
     */
    if ((destinationAddress & 0x3U) != 0U)
    {
        return DATA_ERR_OFFSET;
    }

    /*
     * Flash programming happens only after all packet checks.
     * Flash_ProgramBytes handles final partial-word padding.
     */
    flashStatus = Flash_ProgramBytes(
        destinationAddress,
        &packet->payload[4],
        dataLength);

    if (flashStatus != FLASH_OK)
    {
        return DATA_ERR_FLASH;
    }

    /* Advance only after successful programming. */
    transfer.lastPacketOffset = offset;
    transfer.lastPacketLength = dataLength;
    transfer.lastPacketValid = 1U;

    transfer.expectedOffset += dataLength;

    return DATA_OK;
}