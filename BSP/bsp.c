#include "bsp.h"
#include "stm32f4xx.h"

/*
 * Clock target:
 *   HSI 16MHz → PLL → 180MHz SYSCLK
 *   AHB/1  = 180MHz
 *   APB1/4 = 45MHz
 *   APB2/2 = 90MHz
 *
 * PLL:  VCO = 16 * (180/8) = 360MHz
 *       SYSCLK = 360 / 2 = 180MHz
 */
void bsp_init(void)
{
    /* 1. Power: enable PWR clock, set voltage scale 1 */
    RCC->APB1ENR |= RCC_APB1ENR_PWREN;
    PWR->CR      |= PWR_CR_VOS;

    /* 2. Over-drive mode (required for 180MHz on F446) */
    PWR->CR |= PWR_CR_ODEN;
    while (!(PWR->CSR & PWR_CSR_ODRDY));
    PWR->CR |= PWR_CR_ODSWEN;
    while (!(PWR->CSR & PWR_CSR_ODSWRDY));

    /* 3. Flash latency: 5 wait states at 180MHz, 3.3V */
    FLASH->ACR = FLASH_ACR_PRFTEN
               | FLASH_ACR_ICEN
               | FLASH_ACR_DCEN
               | FLASH_ACR_LATENCY_5WS;

    /* 4. PLL config (source = HSI) */
    RCC->PLLCFGR = (8U   << RCC_PLLCFGR_PLLM_Pos)
                 | (180U << RCC_PLLCFGR_PLLN_Pos)
                 | (0U   << RCC_PLLCFGR_PLLP_Pos)   /* PLLP=2 */
                 | (0U   << RCC_PLLCFGR_PLLSRC_Pos) /* HSI */
                 | (7U   << RCC_PLLCFGR_PLLQ_Pos);

    /* 5. Bus prescalers */
    RCC->CFGR |= RCC_CFGR_HPRE_DIV1
              |  RCC_CFGR_PPRE1_DIV4
              |  RCC_CFGR_PPRE2_DIV2;

    /* 6. Enable PLL, wait for lock */
    RCC->CR |= RCC_CR_PLLON;
    while (!(RCC->CR & RCC_CR_PLLRDY));

    /* 7. Switch SYSCLK to PLL */
    RCC->CFGR |= RCC_CFGR_SW_PLL;
    while ((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_PLL);
}
