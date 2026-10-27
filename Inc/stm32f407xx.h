/*
 * stm32f407xx.h
 *
 *  Created on: Sep 22, 2026
 *      Author: udupas
 */

#ifndef STM32F407XX_H_
#define STM32F407XX_H_
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "cortexM4.h"
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
/*GPIOD Reg Def*/


#define SYSTICK_BASE  0xE000E010UL

typedef struct
{
    volatile uint32_t CTRL;
    volatile uint32_t LOAD;
    volatile uint32_t VAL;
    volatile uint32_t CALIB;
} SysTick_RegDef_t;

#define SysTick ((SysTick_RegDef_t *)SYSTICK_BASE)


typedef struct
{
	volatile uint32_t MODER;
	volatile uint32_t OTYPER;
	volatile uint32_t OSPEEDR;
	volatile uint32_t PUPDR;
	volatile uint32_t IDR;
	volatile uint32_t ODR;
	volatile uint32_t BSRR;
	volatile uint32_t LCKR;
	volatile uint32_t AFRL;
	volatile uint32_t AFRH;
}GPIO_RegDef_t;



typedef struct
{
	volatile uint32_t CR;
	volatile uint32_t PLLCFGR;
	volatile uint32_t CFGR;
	volatile uint32_t CIR;
	volatile uint32_t AHB1RSTR;
	volatile uint32_t AHB2RSTR;
	volatile uint32_t AHB3RSTR;
	uint32_t RESERVED0;
	volatile uint32_t APB1RSTR;
	volatile uint32_t APB2RSTR;
	uint32_t RESERVED1[2];
	volatile uint32_t AHB1ENR;
	volatile uint32_t AHB2ENR;
	volatile uint32_t AHB3ENR;
	uint32_t RESERVED2;
	volatile uint32_t APB1ENR;
	volatile uint32_t APB2ENR;
	uint32_t RESERVED3[2];
	volatile uint32_t AHB1LPENR;
	volatile uint32_t AHB2LPENR;
	volatile uint32_t AHB3LPENR;
	uint32_t RESERVED4;
	volatile uint32_t APB1LPENR;
	volatile uint32_t APB2LPENR;
	uint32_t RESERVED5[2];
	volatile uint32_t BDCR;
	volatile uint32_t CSR;
	uint32_t RESERVED6[2];
	volatile uint32_t SSCGR;
	volatile uint32_t PLLI2SCFGR;


}RCC_RegDef_t;


typedef struct
{
	volatile uint32_t MEMRMP;
	volatile uint32_t PMC;
	volatile uint32_t EXTICR[4];
	volatile uint32_t CMRCR;

}SYSCFG_RegDef_t;

typedef struct
{
	volatile uint32_t IMR;
	volatile uint32_t EMR;
	volatile uint32_t RTSR;
	volatile uint32_t FTSR;
	volatile uint32_t SWIER;
	volatile uint32_t PR;

}EXTI_RegDef_t;


typedef struct
{
	volatile uint32_t SR;
	volatile uint32_t DR;
	volatile uint32_t BRR;
	volatile uint32_t CR1;
	volatile uint32_t CR2;
	volatile uint32_t CR3;
	volatile uint32_t GTPR;

}USART_RegDef_t;


typedef struct
{
	volatile uint32_t CR1;
	volatile uint32_t CR2;
	uint32_t Reserved0;
	volatile uint32_t DIER;
	volatile uint32_t SR;
	volatile uint32_t EGR;
	uint32_t Reserved1[3];
	volatile uint32_t CNT;
	volatile uint32_t PSC;
	volatile uint32_t ARR;
}TIM_RegDef_t;


#define AHB1_BASEADDR (0x40020000UL)
#define APB2_BASEADDR (0x40010000UL)
#define APB1_BASEADDR (0x40000000UL)

#define SYSCFG ((SYSCFG_RegDef_t *) (APB2_BASEADDR+0x0UL))
#define RCC ((RCC_RegDef_t *)(AHB1_BASEADDR+0x3800UL))
#define EXTI ((EXTI_RegDef_t *) (APB2_BASEADDR+0x3C00UL))


#define USART6 ((USART_RegDef_t *) (APB2_BASEADDR+0x1400UL) )
#define USART1 ((USART_RegDef_t *) (APB2_BASEADDR+0x1000UL) )
#define USART2 ((USART_RegDef_t *) (APB1_BASEADDR+0x4400UL) )
#define USART3 ((USART_RegDef_t *) (APB1_BASEADDR+0x4800UL) )
#define UART4  ((USART_RegDef_t *) (APB1_BASEADDR+0x4C00UL) )
#define UART5  ((USART_RegDef_t *) (APB1_BASEADDR+0x5000UL) )


#define GPIOA ((GPIO_RegDef_t *) (AHB1_BASEADDR+0x0000UL))
#define GPIOB ((GPIO_RegDef_t *) (AHB1_BASEADDR+0x0400UL))
#define GPIOC ((GPIO_RegDef_t *) (AHB1_BASEADDR+0x0800UL))
#define GPIOD ((GPIO_RegDef_t *) (AHB1_BASEADDR + 0x0C00UL))

#define GPIOE ((GPIO_RegDef_t *) (AHB1_BASEADDR+0x1000UL))
#define GPIOF ((GPIO_RegDef_t *) (AHB1_BASEADDR+0x1400UL))
#define GPIOG ((GPIO_RegDef_t *) (AHB1_BASEADDR+0x1800UL))
#define GPIOH ((GPIO_RegDef_t *) (AHB1_BASEADDR + 0x1C00UL))


#define GPIOI ((GPIO_RegDef_t *) (AHB1_BASEADDR+0x2000UL))
#define GPIOJ ((GPIO_RegDef_t *) (AHB1_BASEADDR+0x2400UL))
#define GPIOK ((GPIO_RegDef_t *) (AHB1_BASEADDR+0x2800UL))

#define TIM6 ((TIM_RegDef_t *) (APB1_BASEADDR+0x1000UL))
#define TIM7 ((TIM_RegDef_t *) (APB1_BASEADDR+0x1400UL))


#define GPIOA_CLK_ENB() (RCC->AHB1ENR |= 0x01U<<0U)
#define GPIOB_CLK_ENB() (RCC->AHB1ENR |= 0x01U<<1U)
#define GPIOC_CLK_ENB() (RCC->AHB1ENR |= 0x01U<<2U)
#define GPIOD_CLK_ENB() (RCC->AHB1ENR |= 0x01U<<3U)
#define GPIOE_CLK_ENB() (RCC->AHB1ENR |= 0x01U<<4U)
#define GPIOF_CLK_ENB() (RCC->AHB1ENR |= 0x01U<<5U)
#define GPIOG_CLK_ENB() (RCC->AHB1ENR |= 0x01U<<6U)
#define GPIOH_CLK_ENB() (RCC->AHB1ENR |= 0x01U<<7U)
#define GPIOI_CLK_ENB() (RCC->AHB1ENR |= 0x01U<<8U)


#define SYSCFG_CLK_ENB() (RCC->APB2ENR |=0x01U<<14U)

#define GPIOA_CLK_DIS() (RCC->AHB1ENR &= ~(0x01U<<0U))
#define GPIOB_CLK_DIS() (RCC->AHB1ENR &= ~(0x01U<<1U))
#define GPIOC_CLK_DIS() (RCC->AHB1ENR &= ~(0x01U<<2U))
#define GPIOD_CLK_DIS() (RCC->AHB1ENR &= ~(0x01U<<3U))
#define GPIOE_CLK_DIS() (RCC->AHB1ENR &= ~(0x01U<<4U))
#define GPIOF_CLK_DIS() (RCC->AHB1ENR &= ~(0x01U<<5U))
#define GPIOG_CLK_DIS() (RCC->AHB1ENR &= ~(0x01U<<6U))
#define GPIOH_CLK_DIS() (RCC->AHB1ENR &= ~(0x01U<<7U))
#define GPIOI_CLK_DIS() (RCC->AHB1ENR &= ~(0x01U<<8U))

#define SYSCFG_CLK_DIS() (RCC->APB2ENR &= ~(0x01U<<14U))


#define USART1_CLK_ENB()	(RCC->APB2ENR |= 0x01U<<4U)
#define USART2_CLK_ENB()	(RCC->APB1ENR |= 0x01U<<17U)
#define USART3_CLK_ENB()	(RCC->APB1ENR |= 0x01U<<18U)
#define USART6_CLK_ENB()	(RCC->APB2ENR |= 0x01U<<5U)
#define UART4_CLK_ENB()	(RCC->APB2ENR |= 0x01U<<19U)
#define UART5_CLK_ENB()	(RCC->APB2ENR |= 0x01U<<20U)

#define USART2_RXNEIE_ENB() (USART2->CR1 |= 0x01U<<5U)

#define CAN1_ENB()	(RCC->APB1ENR |= 0x01U<<25U)

#define TIM6_CLK_ENB()	(RCC->APB1ENR |= 0x01U<<4U)
#define TIM7_CLK_ENB()	(RCC->APB1ENR |= 0x01U<<5U)	



#define TRUE 1U
#define FALSE 0U

#define SET 1U
#define RESET 0U

#define ENABLE 1U
#define DISABLE 0U

#define RX_BUFFER_SIZE 8U
#define TX_BUFFER_SIZE 8U

#define UART_TRANSMIT_TIME 1000U
#define CAN_TRANSMIT_TIME 1000U
#define LED_TOGGLE_TIME 500U

#define USART_SR_TC (1U << 6) 


uint32_t Get_PLLClk();
uint32_t RCC_GetPCLK1Val();
uint32_t RCC_GetPCLK2Val();

#include "stm32f407xx_gpio_driver.h"
#include "stm32f407xx_usart_driver.h"
//#include "stm32f407xx_can_driver.h"
//#include "stm32f407xx_timer_driver.h"
//#include "stm32f407xx_gnss_driver.h"
#include "ringbuffer.h"
#include "utils.h"
#include "stm32f407xx_flash_driver.h"
#include "stm32f407xx_crc.h"
#include "stm32f407xx_bootloader.h"

#endif /* STM32F407XX_H_ */
