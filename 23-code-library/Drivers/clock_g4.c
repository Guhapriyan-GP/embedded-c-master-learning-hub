/**
 * @file    clock_g4.c
 * @brief   STM32G474RE 170 MHz / 160 MHz Range 1 Boost Mode clock setup
 *          (RM0440 §6.1.5) and DWT cycle-counter delay implementation.
 */

#include "clock_g4.h"
#include "stm32g474xx.h"

#define CLOCK_READY_TIMEOUT 1000000U

static clock_profile_t s_active_profile = CLOCK_PROFILE_HSI16;

/* Minimum >= 1 us stabilization delay at intermediate HCLK (80..85 MHz -> ~3+ us actual) */
static void clock_stabilize_1us_intermediate(void) {
    for (uint32_t i = 0U; i < 120U; ++i) {
        __asm__ volatile("nop");
    }
}

/**
 * @brief Internal shared Range 1 Boost Mode PLL configuration (> 150 MHz .. 170 MHz).
 * @param pll_src    RCC_PLLCFGR_PLLSRC_HSI or RCC_PLLCFGR_PLLSRC_HSE.
 * @param pll_m_div  Input divider M (1..16); yields f_PLL_IN = 4 MHz here.
 * @param pll_n_mul  VCO multiplier N (80 -> 320 MHz / 160 MHz SYSCLK; 85 -> 340 MHz / 170 MHz).
 */
static bool clock_configure_pll_boost(uint32_t pll_src, uint32_t pll_m_div, uint32_t pll_n_mul) {
    uint32_t timeout;

    /* 1. Enable PWR clock on APB1 and read back to synchronize bus */
    RCC->APB1ENR1 |= RCC_APB1ENR1_PWREN;
    (void)RCC->APB1ENR1;

    /* 2. Select Core Voltage Scaling Range 1 (VOS = 0b01) */
    PWR->CR1 = (PWR->CR1 & ~PWR_CR1_VOS) | PWR_CR1_VOS_0;

    /* 3. Wait until voltage scaling transition completes (VOSF == 0) */
    timeout = CLOCK_READY_TIMEOUT;
    while ((PWR->SR2 & PWR_SR2_VOSF) != 0U) {
        if (--timeout == 0U) {
            return false;
        }
    }

    /* 4. Enable Range 1 Boost Mode (R1MODE = 0 in PWR_CR5 @ offset 0x80) for > 150 MHz */
    PWR->CR5 &= ~PWR_CR5_R1MODE;

    /* 5. Set Flash latency to 4 Wait States (RM0440 Table 9: > 136 MHz..170 MHz in Boost Mode)
     *    and enable ART Prefetch, Instruction Cache, and Data Cache */
    FLASH->ACR = (FLASH->ACR & ~FLASH_ACR_LATENCY) |
                 FLASH_ACR_LATENCY_4WS |
                 FLASH_ACR_PRFTEN |
                 FLASH_ACR_ICEN |
                 FLASH_ACR_DCEN;

    timeout = CLOCK_READY_TIMEOUT;
    while ((FLASH->ACR & FLASH_ACR_LATENCY) != FLASH_ACR_LATENCY_4WS) {
        if (--timeout == 0U) {
            return false;
        }
    }

    /* 6. If PLL is currently active as SYSCLK, switch safely back to HSI16 first */
    if ((RCC->CFGR & RCC_CFGR_SWS) == RCC_CFGR_SWS_PLL) {
        RCC->CR |= RCC_CR_HSION;
        timeout = CLOCK_READY_TIMEOUT;
        while ((RCC->CR & RCC_CR_HSIRDY) == 0U) {
            if (--timeout == 0U) {
                return false;
            }
        }
        RCC->CFGR = (RCC->CFGR & ~RCC_CFGR_SW) | RCC_CFGR_SW_HSI;
        timeout = CLOCK_READY_TIMEOUT;
        while ((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_HSI) {
            if (--timeout == 0U) {
                return false;
            }
        }
    }

    /* 7. Disable PLL and wait until PLLRDY clears before modifying PLLCFGR */
    RCC->CR &= ~RCC_CR_PLLON;
    timeout = CLOCK_READY_TIMEOUT;
    while ((RCC->CR & RCC_CR_PLLRDY) != 0U) {
        if (--timeout == 0U) {
            return false;
        }
    }

    /* 8. Configure Main PLL:
     *    f_PLL_IN = 4 MHz (HSI16/4 or HSE24/6), within [2.66, 16] MHz (DS12288)
     *    f_VCO    = 4 MHz * N (320 MHz for N=80; 340 MHz for N=85), within [96, 344] MHz
     *    f_PLL_R  = f_VCO / 2 (160 MHz or 170 MHz; PLLR = 0b00 -> /2, PLLREN = 1)
     *    f_PLL_Q  = f_VCO / 8 (40.0 MHz or 42.5 MHz; PLLQ = 0b11 -> /8, PLLQEN = 1)
     *    f_PLL_P  = f_VCO / 6 (53.33 MHz or 56.67 MHz <= 60 MHz ADC max; PLLPDIV = 6) */
    uint32_t pllcfgr = pll_src |
                       ((pll_m_div - 1U) << RCC_PLLCFGR_PLLM_Pos) |
                       (pll_n_mul << RCC_PLLCFGR_PLLN_Pos) |
                       (0U << RCC_PLLCFGR_PLLR_Pos) |
                       RCC_PLLCFGR_PLLREN |
                       (3U << RCC_PLLCFGR_PLLQ_Pos) |
                       RCC_PLLCFGR_PLLQEN |
                       (6U << RCC_PLLCFGR_PLLPDIV_Pos) |
                       RCC_PLLCFGR_PLLPEN;
    RCC->PLLCFGR = pllcfgr;

    /* 9. Enable PLL and wait for PLL lock */
    RCC->CR |= RCC_CR_PLLON;
    timeout = CLOCK_READY_TIMEOUT;
    while ((RCC->CR & RCC_CR_PLLRDY) == 0U) {
        if (--timeout == 0U) {
            return false;
        }
    }

    /* 10. RM0440 Section 6.1.5 mandatory two-step switch for SYSCLK > 150 MHz:
     *     Step A: Set AHB prescaler HPRE = /2 (80 MHz or 85 MHz intermediate HCLK) */
    RCC->CFGR = (RCC->CFGR & ~(RCC_CFGR_HPRE | RCC_CFGR_PPRE1 | RCC_CFGR_PPRE2)) |
                RCC_CFGR_HPRE_DIV2 |
                RCC_CFGR_PPRE1_DIV1 |
                RCC_CFGR_PPRE2_DIV1;

    /*     Step B: Switch SYSCLK to PLL and verify SWS == PLL */
    RCC->CFGR = (RCC->CFGR & ~RCC_CFGR_SW) | RCC_CFGR_SW_PLL;
    timeout = CLOCK_READY_TIMEOUT;
    while ((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_PLL) {
        if (--timeout == 0U) {
            return false;
        }
    }

    /*     Step C: Wait >= 1 us at intermediate HCLK before stepping to full frequency */
    clock_stabilize_1us_intermediate();

    /*     Step D: Switch AHB prescaler HPRE = /1 -> full 160 MHz or 170 MHz HCLK/PCLK1/PCLK2 */
    RCC->CFGR = (RCC->CFGR & ~RCC_CFGR_HPRE) | RCC_CFGR_HPRE_DIV1;

    /* 11. Refresh CMSIS SystemCoreClock global variable */
    SystemCoreClockUpdate();
    return true;
}

bool Clock_Init_170MHz(void) {
    uint32_t timeout = CLOCK_READY_TIMEOUT;

    /* Ensure HSI16 is enabled and stable */
    RCC->CR |= RCC_CR_HSION;
    while ((RCC->CR & RCC_CR_HSIRDY) == 0U) {
        if (--timeout == 0U) {
            return false;
        }
    }

    /* HSI16 (16 MHz) / M=4 * N=85 / R=2 = 170 MHz */
    if (!clock_configure_pll_boost(RCC_PLLCFGR_PLLSRC_HSI, 4U, 85U)) {
        return false;
    }

    s_active_profile = CLOCK_PROFILE_170MHZ_HSI;
    return true;
}

bool Clock_Init_160MHz(void) {
    uint32_t timeout = CLOCK_READY_TIMEOUT;

    /* Ensure HSI16 is enabled and stable */
    RCC->CR |= RCC_CR_HSION;
    while ((RCC->CR & RCC_CR_HSIRDY) == 0U) {
        if (--timeout == 0U) {
            return false;
        }
    }

    /* HSI16 (16 MHz) / M=4 * N=80 / R=2 = 160 MHz */
    if (!clock_configure_pll_boost(RCC_PLLCFGR_PLLSRC_HSI, 4U, 80U)) {
        return false;
    }

    s_active_profile = CLOCK_PROFILE_160MHZ_HSI;
    return true;
}

bool Clock_Init_170MHz_HSE24(void) {
    uint32_t timeout = CLOCK_READY_TIMEOUT;

    /* Enable 24 MHz crystal oscillator HSE (X3 on NUCLEO-G474RE MB1367 Rev C) */
    RCC->CR &= ~RCC_CR_HSEBYP;
    RCC->CR |= RCC_CR_HSEON;
    while ((RCC->CR & RCC_CR_HSERDY) == 0U) {
        if (--timeout == 0U) {
            return false;
        }
    }

    /* HSE (24 MHz) / M=6 * N=85 / R=2 = 170 MHz */
    if (!clock_configure_pll_boost(RCC_PLLCFGR_PLLSRC_HSE, 6U, 85U)) {
        return false;
    }

    s_active_profile = CLOCK_PROFILE_170MHZ_HSE;
    return true;
}

uint32_t Clock_GetSysClkHz(void) {
    return SystemCoreClock;
}

clock_profile_t Clock_GetProfile(void) {
    return s_active_profile;
}

void Clock_DWT_Init(void) {
    /* Enable Trace (TRCENA) in CoreDebug->DEMCR so DWT registers are accessible */
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    __DSB();
    DWT->CYCCNT = 0U;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
    __DSB();
}

uint32_t Clock_GetCycles(void) {
    return DWT->CYCCNT;
}

void Clock_DelayUs(uint32_t us) {
    /* Guard against uninitialized DWT->CYCCNT so Clock_DelayUs() can never hang */
    if (((CoreDebug->DEMCR & CoreDebug_DEMCR_TRCENA_Msk) == 0U) ||
        ((DWT->CTRL & DWT_CTRL_CYCCNTENA_Msk) == 0U)) {
        Clock_DWT_Init();
    }

    uint32_t cycles_per_us = SystemCoreClock / 1000000U;
    if (cycles_per_us == 0U) {
        cycles_per_us = 1U;
    }

    /* Chunk delays into <= 1,000,000 us slices to prevent 32-bit cycle-counter wrap overflow */
    while (us > 0U) {
        uint32_t chunk_us    = (us > 1000000U) ? 1000000U : us;
        uint32_t start       = DWT->CYCCNT;
        uint32_t wait_cycles = chunk_us * cycles_per_us;
        while ((uint32_t)(DWT->CYCCNT - start) < wait_cycles) {
            __asm__ volatile("nop");
        }
        us -= chunk_us;
    }
}

void Clock_DelayMs(uint32_t ms) {
    for (uint32_t i = 0U; i < ms; ++i) {
        Clock_DelayUs(1000U);
    }
}
