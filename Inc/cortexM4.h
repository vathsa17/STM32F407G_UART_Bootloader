/*
 * cortexM4.h
 *
 *  Created on: Sep 24, 2026
 *      Author: udupas
 */

#ifndef CORTEXM4_H_
#define CORTEXM4_H_
#include <stdint.h>

/**
 * @brief NVIC Reg Definition
 * 
 */
typedef struct
{
	volatile uint32_t ISER[8];
	uint32_t RESERVED0[24];

	volatile uint32_t ICER[8];
	uint32_t RESERVED1[24];

	volatile uint32_t ISPR[8];
	uint32_t RESERVED2[24];

	volatile uint32_t ICPR[8];
	uint32_t RESERVED3[24];

	volatile uint32_t IABR[8];
	uint32_t RESERVED4[24];

	volatile uint32_t IPR[8];
	uint32_t RESERVED5[24];


	volatile uint32_t STIR;;
}NVIC_RegDef_t;

#define NVIC ((NVIC_RegDef_t *)(0xE000E100)) /**NVIC Memory Def */


#define EXTI_NO_0 6U
#define EXTI_NO_1 7U
#define EXTI_NO_2 8U
#define EXTI_NO_3 9U
#define EXTI_NO_4 10U
#define EXTI_NO_9_5 23U
#define EXTI_NO_10_15 40U

#define IRQ_NO_USART2 38U
#define IRQ_NO_TIM6 54U
#define IRQ_NO_TIM7 55U


#define SYSTICK_BASE_ADDR    0xE000E010U

#define SYSTICK_CTRL   (*(volatile uint32_t *)(SYSTICK_BASE_ADDR + 0x00U))
#define SYSTICK_LOAD   (*(volatile uint32_t *)(SYSTICK_BASE_ADDR + 0x04U))
#define SYSTICK_VAL    (*(volatile uint32_t *)(SYSTICK_BASE_ADDR + 0x08U))

#define NULL ((void *)0)

void NVIC_SetPriority(uint8_t IRQNumber,uint8_t IT_Priority);
void NVIC_EnableIRQ(uint8_t IRQNumber);
void NVIC_DisableIRQ(uint8_t IRQNumber);

#endif /* CORTEXM4_H_ */
