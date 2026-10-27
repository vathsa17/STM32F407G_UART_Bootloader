/*
 * stm32f407xx_bootloader.h
 *
 *  Created on: 27 Oct 2026
 *      Author: shriv
 */

#ifndef STM32F407XX_BOOTLOADER_H_
#define STM32F407XX_BOOTLOADER_H_

#include "stm32f407xx.h"

#define PROTOCOL_SOF            0xAAU

#define CMD_BEGIN               0x01U
#define CMD_DATA                0x02U
#define CMD_END                 0x03U

#define CMD_ACK                 0x80U
#define CMD_NAK                 0x81U

#define PROTOCOL_HEADER_SIZE    4U
#define PROTOCOL_CRC_SIZE       2U
#define PROTOCOL_MAX_PAYLOAD    256U

#define APP_ADDRESS             0x08020000UL
#define APP_END_ADDRESS         0x08100000UL
#define APP_MAX_SIZE            (APP_END_ADDRESS - APP_ADDRESS)
#define SRAM1_START       0x20000000UL
#define SRAM1_END         0x20020000UL



typedef struct
{
    uint8_t command;
    uint16_t length;
    uint8_t payload[PROTOCOL_MAX_PAYLOAD];
    uint16_t crc;
} BootPacket_t;


typedef enum
{
    METADATA_STATUS_OK,
    METADATA_STATUS_ERROR
} MetadataStatus_t;


typedef enum
{
    PROTOCOL_INCOMPLETE,
    PROTOCOL_PACKET_READY,
    PROTOCOL_INVALID
} Protocol_Result_t;


typedef enum
{
    RX_WAIT_SOF,
    RX_COMMAND,
    RX_LENGTH_LOW,
    RX_LENGTH_HIGH,
    RX_PAYLOAD,
    RX_CRC_LOW,
    RX_CRC_HIGH
} ProtocolRxState_t;

typedef struct
{
    uint32_t imageSize;
    uint32_t expectedImageCrc32;
    uint32_t expectedOffset;
    uint8_t metadataValid;
    uint8_t active;
} FirmwareTransfer_t;

typedef enum
{
    TRANSFER_OK,
    TRANSFER_ERR_METADATA,
    TRANSFER_ERR_FLASH
} TransferStatus_t;


#define SCB_VTOR \
    (*(volatile uint32_t *)0xE000ED08UL)

typedef void (*ApplicationEntry_t)(void);


bool Bootloader_IsApplicationValid(void);
void Bootloader_JumpToApplication(void);
Protocol_Result_t Protocol_ProcessByte(uint8_t byte);
MetadataStatus_t Protocol_ValidatePacket(const BootPacket_t *packet);
void Protocol_Init(void);
BootPacket_t *Protocol_GetPacket(void);
MetadataStatus_t Bootloader_ProcessMetadata(const BootPacket_t *packet);
#endif /* STM32F407XX_BOOTLOADER_H_ */
