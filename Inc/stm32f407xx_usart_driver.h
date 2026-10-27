/*
 * stm32f407xx_usart_driver.h
 *
 *  Created on: Sep 25, 2026
 *      Author: udupas
 */

#ifndef STM32F407XX_USART_DRIVER_H_
#define STM32F407XX_USART_DRIVER_H_

#include "stm32f407xx.h"

typedef struct
{
uint8_t Mode;
uint32_t BaudRate;
uint8_t WordLenght;
uint8_t StopBits;
uint8_t Parity;
uint8_t OverSampleing;
}
USART_Conf_t;

#define USART_MODE_TX 1U
#define USART_MODE_RX 2U
#define USART_MODE_RX_TX 3U

#define USART_BAUDRATE_115200 115200U
#define USART_BAUDRATE_57600 57600U
#define USART_BAUDRATE_38400 38400U
#define USART_BAUDRATE_19200 19200U
#define USART_BAUDRATE_9600 9600U
#define USART_BAUDRATE_4800 4800U

#define USART_WORDLENGTH_8B 0U
#define USART_WORDLENGTH_9B 1U

#define USART_STOPBITS_1 0U
#define USART_STOPBITS_0_5 1U
#define USART_STOPBITS_2 2U
#define USART_STOPBITS_1_5 3U

#define USART_PARITY_NONE 0U
#define USART_PARITY_EVEN 2U
#define USART_PARITY_ODD 3U



void USART_Init(USART_RegDef_t * USARTx,USART_Conf_t USART_Conf);
void USART_Transmit(USART_RegDef_t * USARTx, uint8_t * Mess, uint8_t MessSize);
void USART_Recieve(USART_RegDef_t * USARTx, uint8_t * Mess, uint8_t MessSize);
#endif /* STM32F407XX_USART_DRIVER_H_ */
    
