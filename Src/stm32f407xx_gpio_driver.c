/*
 * stm32f407xx_gpio_driver.c
 *
 *  Created on: Sep 22, 2026
 *      Author: udupas
 */

#include "stm32f407xx.h"
#include "cortexM4.h"
/**
 * @brief This function initilizes the GPIO peripheral according to desiered configu
 *
 * @param GPIOx Desired GPIO Port
 * @param GPIOPinConf Desire GPIO Port Config
 */
void GPIO_Init(GPIO_RegDef_t * GPIOx, GPIO_PinConf_t GPIOPinConf)
{

	GPIOx->MODER &= ~(0x03<< (GPIOPinConf.GPIO_PinNumber * 2));
	GPIOx->MODER |= (GPIOPinConf.GPIO_PinMode << (GPIOPinConf.GPIO_PinNumber * 2));

	if ((GPIOPinConf.GPIO_PinMode == GPIO_MODE_OUTPUT) || (GPIOPinConf.GPIO_PinMode == GPIO_MODE_ALT))
	{
		GPIOx->OTYPER &= ~(0x01 << GPIOPinConf.GPIO_PinNumber);
		GPIOx->OTYPER |= (GPIOPinConf.GPIO_OutType << GPIOPinConf.GPIO_PinNumber);

		GPIOx->OSPEEDR &= ~(0x03 << GPIOPinConf.GPIO_PinNumber*2);
		GPIOx->OSPEEDR |= (GPIOPinConf.GPIO_OutSpeed << GPIOPinConf.GPIO_PinNumber * 2);
	}


	GPIOx->PUPDR &= ~(0x03 << GPIOPinConf.GPIO_PinNumber*2);
	GPIOx->PUPDR |= (GPIOPinConf.GPIO_PUPD << GPIOPinConf.GPIO_PinNumber * 2);

	if(GPIOPinConf.GPIO_PinMode == GPIO_MODE_ALT)
	{
		if (GPIOPinConf.GPIO_PinNumber <8)
		{
			GPIOx->AFRL &= ~(0x0F << GPIOPinConf.GPIO_PinNumber*4);
			GPIOx->AFRL |= (GPIOPinConf.GPIO_AltFnc  << GPIOPinConf.GPIO_PinNumber * 4);
		}
		else
		{
			GPIOx->AFRH &= ~(0x0F << (GPIOPinConf.GPIO_PinNumber-8)*4);
			GPIOx->AFRH |= (GPIOPinConf.GPIO_AltFnc << (GPIOPinConf.GPIO_PinNumber-8) * 4);
		}
	}
}

/**
 * @brief Function to Write the Particular GPIO Pin
 *
 * @param GPIOx Desired GPIO Port
 * @param PinNumber Desired GPIO Pin
 * @param PinState Desired GPIO Pin State
 */
void GPIO_WritePin(GPIO_RegDef_t * GPIOx,uint8_t PinNumber, GPIO_PinState_e PinState)
{

	if (PinState == GPIO_PIN_LOW)
	{
		GPIOx->ODR &= ~(0x01U<<PinNumber);
	}
	else
	{
		GPIOx->ODR |= (0x01U<<PinNumber);
	}
}

/**
 * @brief Function Read the Current Value from GPIO Pin
 *
 * @param GPIOx Desired GPIO Port
 * @param PinNumber The Pin Number in the Port
 * @return uint8_t Returns the current Pin Value
 */
uint8_t GPIO_ReadPin(GPIO_RegDef_t * GPIOx,uint8_t PinNumber)
{
	return  (GPIOx->IDR>>PinNumber)& 0x01;
}


/**
 * @brief Function to Toggle the corresponding GPIO Output Pin
 *
 * @param GPIOx Desired GPIO Port
 * @param PinNumber The Pin Number in the Port
 */
void GPIO_TogglePin(GPIO_RegDef_t * GPIOx,uint8_t PinNumber)
{


	GPIOx->ODR ^= (1U << PinNumber);

}


/**
 * @brief Function to Lock the GPIO Pin Configuration
 *
 * @param GPIOx Desired GPIO Port
 * @param PinNumber The Pin Number in the Port
 */
void GPIO_LockPinConf(GPIO_RegDef_t * GPIOx,uint8_t PinNumber)
{
	/*
	 * Steps to Lock you GPIO Pin Config
	 * */
	uint32_t temp1= 1<<16U;
	uint32_t temp2= 0x01<<PinNumber;
	temp1|=temp2;
	GPIOx->LCKR=temp1;
	GPIOx->LCKR=temp2;
	GPIOx->LCKR=temp1;
	temp1=GPIOx->LCKR;


}

/**
 * @brief Initilizes the interrupt for the specified GPIO Pin
 *
 */

void GPIO_IT_Init(GPIO_RegDef_t * GPIOx, GPIO_PinConf_t GPIOPinConf,uint8_t Priority)
{
	SYSCFG_CLK_ENB();

	uint8_t index,bitpos,portcode;

	index=GPIOPinConf.GPIO_PinNumber/4;
	bitpos=(GPIOPinConf.GPIO_PinNumber%4)*4;
	portcode=SYSCFG_EXTICR_PORTCODE(GPIOx);

	SYSCFG->EXTICR[index] &=~(0x0FU<<bitpos);
	SYSCFG->EXTICR[index] |=(portcode<<bitpos);

	switch (GPIOPinConf.GPIO_EdgeTrigger)
	{

		case GPIO_IT_EDGE_FT :
			{
				EXTI->RTSR &=~(0x01<<GPIOPinConf.GPIO_PinNumber);
				EXTI->FTSR |=(0x01<<GPIOPinConf.GPIO_PinNumber);
				break;
			}
		case GPIO_IT_EDGE_RT:
			{

				EXTI->FTSR &=~(0x01<<GPIOPinConf.GPIO_PinNumber);
				EXTI->RTSR |=(0x01<<GPIOPinConf.GPIO_PinNumber);
				break;
			}
		default:
		{
			EXTI->RTSR |=(0x01<<GPIOPinConf.GPIO_PinNumber);
			EXTI->FTSR |=(0x01<<GPIOPinConf.GPIO_PinNumber);
			break;
		}
	}

	EXTI->IMR |=(0x01<<GPIOPinConf.GPIO_PinNumber);
	NVIC_SetPriority(GPIO_PIN_TO_IRQ(GPIOPinConf.GPIO_PinNumber),Priority);
	NVIC_EnableIRQ(GPIO_PIN_TO_IRQ(GPIOPinConf.GPIO_PinNumber));
}
