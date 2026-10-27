/*
 * stm32f407_usart_driver.c
 *
 *  Created on: Sep 27, 2026
 *      Author: udupas
 */
#include "stm32f407xx.h"
void USART_Transmit(USART_RegDef_t * USARTx, uint8_t * Mess, uint8_t MessSize)
{
	uint8_t *pDataBit;
	uint16_t *pData16Bit;
	uint8_t TxCounter;

	TxCounter=MessSize;


	if((((USARTx->CR1>>12U)&0x01) == USART_WORDLENGTH_9B) \
			&& (((USARTx->CR1>>9U)&0x01) == USART_PARITY_NONE))
	{
		pDataBit=NULL;
		pData16Bit = (uint16_t *) Mess;
	}
	else
	{
		pDataBit=Mess;
		pData16Bit=NULL;


	}

	while(TxCounter > 0)
		{
			// 1. Wait until TXE (bit 7) becomes 1
			while(!((USARTx->SR >> 7U) & 0x01))
			{
				// Wait for transmit data register to empty
			}

			// 2. Write data to DR register
			if(pDataBit == NULL)
			{
				USARTx->DR = (uint16_t)(*pData16Bit & 0x01FF);
				pData16Bit++;
			}
			else
			{
				USARTx->DR = (uint8_t)(*pDataBit & 0x00FF);
				pDataBit++;
			}

			// 3. Decrement only after successful write
			TxCounter--;
		}
}


void USART_Recieve(USART_RegDef_t * USARTx, uint8_t * Mess, uint8_t MessSize)
{
	uint8_t RxBufferCounter = MessSize;
	uint8_t * pData8bit;
	uint16_t * pData16bit;

	if ((((USARTx->CR1 >> 12U) & 0x01) == USART_WORDLENGTH_9B) &&
	    (((USARTx->CR1 >> 9U) & 0x01) == USART_PARITY_NONE))
	{
		pData8bit = NULL;
		pData16bit = (uint16_t *)Mess;
	}
	else
	{
		pData8bit = Mess;
		pData16bit = NULL;
	}

	while (RxBufferCounter > 0)
	{
		// 1. BLOCKING WAIT: Pause here until RXNE (bit 5) is set by hardware
		while (!((USARTx->SR >> 5U) & 0x01))
		{
			// Waiting for incoming byte...
		}

		// 2. Read received byte from Data Register
		if (pData8bit == NULL)
		{
			*pData16bit = (uint16_t)(USARTx->DR & 0x01FF);
			pData16bit++;
		}
		else
		{
			if ((((USARTx->CR1 >> 12U) & 0x01) == USART_WORDLENGTH_9B) ||
			    ((((USARTx->CR1 >> 12U) & 0x01) == USART_WORDLENGTH_8B) && (((USARTx->CR1 >> 9U) & 0x01) == USART_PARITY_NONE)))
			{
				*pData8bit = (uint8_t)(USARTx->DR & 0x00FF);
			}
			else
			{
				*pData8bit = (uint8_t)(USARTx->DR & 0x007F);
			}
			pData8bit++;
		}

		// 3. Decrement counter ONLY after a byte has actually been read
		RxBufferCounter--;
	}
}


void USART_Init(USART_RegDef_t * USARTx, USART_Conf_t USART_Conf)
{
    uint32_t USARTDIV;
    uint32_t Mantissa, Fraction, Reminder, Scaling;
    uint32_t USARTx_Clk;
    uint32_t tempreg = 0;

    // 1. Disable USART peripheral during configuration (UE = 0)
    USARTx->CR1 &= ~(1U << 13U);

    // 2. Configure CR1 parameters (Word Length, Parity, Mode, Oversampling)
    tempreg |= (USART_Conf.WordLenght << 12U);
    tempreg |= (USART_Conf.Parity << 9U);
    tempreg |= (USART_Conf.Mode << 2U);
    tempreg |= (USART_Conf.OverSampleing << 15U);
    USARTx->CR1 = tempreg; // Write clean configuration to CR1

    // 3. Configure CR2 (Stop Bits)
    USARTx->CR2 &= ~(3U << 12U); // Clear Stop Bits
    USARTx->CR2 |= (USART_Conf.StopBits << 12U);

    // 4. Get Bus Clock Frequency
    if (USARTx == USART1 || USARTx == USART6)
    {
        USARTx_Clk = RCC_GetPCLK2Val();
    }
    else
    {
        USARTx_Clk = RCC_GetPCLK1Val();
    }

    // 5. Calculate Baud Rate Register (BRR) values
    // Scaling is 16 for OVER8=0, and 8 for OVER8=1
    Scaling = 8 * (2 - USART_Conf.OverSampleing);

    // Integer part (Mantissa)
    USARTDIV = USARTx_Clk / (Scaling * USART_Conf.BaudRate);
    Mantissa = USARTDIV;

    // Fractional part rounding
    Reminder = USARTx_Clk % (Scaling * USART_Conf.BaudRate);
    Fraction = ((Reminder * 100U) + ((Scaling * USART_Conf.BaudRate) / 2U)) / (Scaling * USART_Conf.BaudRate);

    if (USART_Conf.OverSampleing == 0) // 16x Oversampling
    {
        Fraction = ((Fraction * 16U) + 50U) / 100U;
    }
    else // 8x Oversampling
    {
        Fraction = (((Fraction * 8U) + 50U) / 100U) & 0x07U; // Only 3 bits used
    }

    // 6. Write to BRR cleanly
    USARTx->BRR = 0; // Clear BRR register completely
    USARTx->BRR |= (Mantissa << 4U);
    USARTx->BRR |= (Fraction & 0x0FU);

    // 7. Enable USART Peripheral (UE = 1)
    USARTx->CR1 |= (1U << 13U);
}

