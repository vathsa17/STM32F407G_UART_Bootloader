/*
 * stm32f407xx_driver.c
 *
 *  Created on: Sep 25, 2026
 *      Author: udupas
 */

#include "stm32f407xx.h"
#include "math.h"


// Lookup tables according to STM32F4 Reference Manual (RM0090)
static const uint16_t AHB_PrescalerTable[16] = {
    1, 1, 1, 1, 1, 1, 1, 1, // 0..7   -> SYSCLK not divided
    2, 4, 8, 16,            // 8..11  -> SYSCLK / 2, 4, 8, 16
    64, 128, 256, 512       // 12..15 -> SYSCLK / 64, 128, 256, 512
};

static const uint8_t APB_PrescalerTable[8] = {
    1, 1, 1, 1,             // 0..3 -> HCLK not divided
    2, 4, 8, 16             // 4..7 -> HCLK / 2, 4, 8, 16
};

uint32_t Get_PLLClk()
{

	uint32_t pllcfgr = RCC->PLLCFGR;

	uint32_t pll_m = pllcfgr & 0x3FU;
	uint32_t pll_n = (pllcfgr >> 6U) & 0x1FFU;
	uint32_t pll_p_bits = (pllcfgr >> 16U) & 0x03U;
	uint32_t pll_src = (pllcfgr >> 22U) & 0x01U;

	uint32_t pll_p = (pll_p_bits + 1U) * 2U;

	uint32_t pll_input =
	    (pll_src == 0U) ? 16000000U : 8000000U;

	uint32_t pll_clk =
	    (pll_input * pll_n) / (pll_m * pll_p);

	if (pll_m == 0U)
    {
        return 0U;
    }
	return pll_clk;
}
uint32_t RCC_GetPCLK1Val(void)
{
    uint32_t sys_clk = 0;
    uint8_t clk_src = (RCC->CFGR >> 2U) & 0x03U;

    switch (clk_src)
    {
        case 0:
            sys_clk = 16000000U; // HSI (16 MHz)
            break;
        case 1:
            sys_clk = 8000000U;  // HSE (8 MHz external crystal)
            break;
        case 2:
        	sys_clk =Get_PLLClk();
            break;
        default:
            sys_clk = 16000000U;
            break;
    }

    // Extract Prescaler Register Index Bits
    uint8_t ahb_idx  = (RCC->CFGR >> 4U)  & 0x0FU;
    uint8_t apb1_idx = (RCC->CFGR >> 10U) & 0x07U;

    uint32_t ahb_pre  = AHB_PrescalerTable[ahb_idx];
    uint32_t apb1_pre = APB_PrescalerTable[apb1_idx];

    return (sys_clk / ahb_pre) / apb1_pre;
}

uint32_t RCC_GetPCLK2Val(void)
{
    uint32_t sys_clk = 0;
    uint8_t clk_src = (RCC->CFGR >> 2U) & 0x03U;

    switch (clk_src)
    {
        case 0:
            sys_clk = 16000000U; // HSI (16 MHz)
            break;
        case 1:
            sys_clk = 8000000U;  // HSE (8 MHz external crystal)
            break;
        case 2:
        	sys_clk =Get_PLLClk();
            // Optional: Add PLL clock retrieval if PLL is configured
            break;
        default:
            sys_clk = 16000000U;
            break;
    }

    // Extract Prescaler Register Index Bits
    uint8_t ahb_idx  = (RCC->CFGR >> 4U)  & 0x0FU;
    uint8_t apb2_idx = (RCC->CFGR >> 13U) & 0x07U;

    uint32_t ahb_pre  = AHB_PrescalerTable[ahb_idx];
    uint32_t apb2_pre = APB_PrescalerTable[apb2_idx];

    return (sys_clk / ahb_pre) / apb2_pre;
}
