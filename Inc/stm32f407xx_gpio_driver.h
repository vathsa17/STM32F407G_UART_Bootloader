/*
 * stm32f407xx_gpio_driver.h
 *
 *  Created on: Sep 22, 2026
 *      Author: udupas
 */

#ifndef STM32F407XX_GPIO_DRIVER_H_
#define STM32F407XX_GPIO_DRIVER_H_

#include "stm32f407xx.h"

typedef struct{

	uint8_t GPIO_PinNumber; /*Pin Config*/
	uint8_t GPIO_PinMode; /*PinMode*/
	uint8_t GPIO_OutType;
	uint8_t GPIO_OutSpeed;
	uint8_t GPIO_PUPD;
	uint8_t GPIO_AltFnc;
	uint8_t GPIO_EdgeTrigger;
}GPIO_PinConf_t;

#define GPIO_IT_EDGE_FT 0U
#define GPIO_IT_EDGE_RT 1U
#define GPIO_IT_EDGE_RFT 2U


#define GPIO_PIN_NUM_0 0U
#define GPIO_PIN_NUM_1 1U
#define GPIO_PIN_NUM_2 2U
#define GPIO_PIN_NUM_3 3U
#define GPIO_PIN_NUM_4 4U
#define GPIO_PIN_NUM_5 5U
#define GPIO_PIN_NUM_6 6U
#define GPIO_PIN_NUM_7 7U
#define GPIO_PIN_NUM_8 8U
#define GPIO_PIN_NUM_9 9U
#define GPIO_PIN_NUM_10 10U
#define GPIO_PIN_NUM_11 11U
#define GPIO_PIN_NUM_12 12U
#define GPIO_PIN_NUM_13 13U
#define GPIO_PIN_NUM_14 14U
#define GPIO_PIN_NUM_15 15U

#define GPIO_MODE_INPUT 0U
#define GPIO_MODE_OUTPUT 1U
#define GPIO_MODE_ALT 2U
#define GPIO_Mode_Analog 3U




#define GPIO_OutType_PP 0U
#define GPIO_Output_OutDrain 1U

#define GPIO_OutSpeed_Low 0U
#define GPIO_OutSpeed_Medium 1U
#define GPIO_OutSpeed_High 2U
#define GPIO_OutSpeed_Fast 3U

#define GPIO_NO_PUPD 0U
#define GPIO_PU 1U
#define GPIO_PD 2U


#define GPIO_AF0 0U
#define GPIO_AF1 1U
#define GPIO_AF2 2U
#define GPIO_AF3 3U
#define GPIO_AF4 4U
#define GPIO_AF5 5U
#define GPIO_AF6 6U
#define GPIO_AF7 7U
#define GPIO_AF8 8U
#define GPIO_AF9 9U
#define GPIO_AF10 10U
#define GPIO_AF11 11U
#define GPIO_AF12 12U
#define GPIO_AF13 13U
#define GPIO_AF14 14U
#define GPIO_AF15 15U




typedef enum
{
	GPIO_PIN_LOW=0,
	GPIO_PIN_HIGH=1
}GPIO_PinState_e;
#define SYSCFG_EXTICR_PORTCODE(GPIOx)\
		((GPIOx==GPIOA) ?0U:\
		(GPIOx==GPIOB) ?1U:\
		(GPIOx==GPIOC) ?2U:\
		(GPIOx==GPIOD) ?3U:\
		(GPIOx==GPIOE) ?4U:\
		(GPIOx==GPIOF) ?5U:\
		(GPIOx==GPIOG) ?6U:\
		(GPIOx==GPIOH) ?7U:\
		(GPIOx==GPIOI) ?8U:0U)


#define GPIO_PIN_TO_IRQ(PinNumber) \
	((PinNumber==0)?EXTI_NO_0 : \
			(PinNumber==1)?EXTI_NO_1 : \
					(PinNumber==2)?EXTI_NO_2 : \
							(PinNumber==3)?EXTI_NO_3 : \
									(PinNumber==4)?EXTI_NO_4 : \
											((PinNumber>=5U)&&(PinNumber<=9U))?EXTI_NO_9_5 :\
													((PinNumber>=10U)&&(PinNumber<=15U))?EXTI_NO_10_15 :EXTI_NO_10_15)

void GPIO_Init(GPIO_RegDef_t * GPIOx, GPIO_PinConf_t GPIOPinConf);
void GPIO_WritePin(GPIO_RegDef_t * GPIOx,uint8_t PinNumber, GPIO_PinState_e PinState);
void GPIO_TogglePin(GPIO_RegDef_t * GPIOx,uint8_t PinNumber);
uint8_t GPIO_ReadPin(GPIO_RegDef_t * GPIOx,uint8_t PinNumber);
void GPIO_WritePinBit(GPIO_RegDef_t * GPIOx,uint8_t PinNumber, GPIO_PinState_e PinState);
void GPIO_LockPinConf(GPIO_RegDef_t * GPIOx,uint8_t PinNumber);
void GPIO_IT_Init(GPIO_RegDef_t * GPIOx, GPIO_PinConf_t GPIOPinConf,uint8_t Priority);


#endif



