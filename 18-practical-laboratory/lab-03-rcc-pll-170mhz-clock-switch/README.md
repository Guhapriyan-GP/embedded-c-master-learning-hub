# Lab 03: RCC PLL 170 MHz Clock Configuration in Boost Mode

## 1. Learning Objectives
Configure the STM32G4 power regulator into Range 1 Boost Mode, set Flash wait states to 4 WS, and synthesize a maximum 170 MHz SYSCLK using the internal HSI16 oscillator and PLL.

## 2. Hardware Requirements & Wiring
- **Hardware:** NUCLEO-G474RE board.
- **Wiring:** Internal clock switching; output clock can optionally be measured on PA8 (MCO).

## 3. Peripheral Registers & Official Documentation
- PWR->CR1 (VOS bits)
- PWR->CR5 (R1MODE bit)
- FLASH->ACR (LATENCY, PRFTEN, ICEN, DCEN)
- RCC->CR (HSION, PLLON, HSIRDY, PLLRDY)
- RCC->PLLCFGR (PLLSRC, PLLM, PLLN, PLLR, PLLREN)
- RCC->CFGR (SW, SWS, HPRE, PPRE1, PPRE2)

**Reference Documentation:** RM0440 Section 5.4.1 (PWR_CR1), Section 5.4.5 (PWR_CR5), Section 3.3.3 (Flash latency), Section 6.4 (RCC).

## 4. Mathematical Calculations & Bitfield Settings
VCO_in = 16 MHz / 4 (M=4) = 4 MHz.
VCO_out = 4 MHz * 85 (N=85) = 340 MHz.
PLL_R = 340 MHz / 2 (R=2) = 170 MHz.
Flash Latency @ 170 MHz Range 1 Boost Mode = 4 Wait States.

## 5. Complete Bare-Metal C Source Code
```c
#include "stm32g474xx.h"

void SystemClock_Config_170MHz(void) {
    /* 1. Enable PWR peripheral clock */
    RCC->APB1ENR1 |= RCC_APB1ENR1_PWREN;

    /* 2. Set Voltage Scaling to Range 1 */
    PWR->CR1 &= ~PWR_CR1_VOS;
    PWR->CR1 |= (1U << PWR_CR1_VOS_Pos);

    /* 3. Enable Range 1 Boost Mode in PWR_CR5 (Clear R1MODE bit 8) */
    PWR->CR5 &= ~PWR_CR5_R1MODE;

    /* 4. Configure Flash Latency: 4 Wait States, enable Prefetch and Caches */
    FLASH->ACR &= ~FLASH_ACR_LATENCY;
    FLASH->ACR |= FLASH_ACR_LATENCY_4WS;
    FLASH->ACR |= (FLASH_ACR_PRFTEN | FLASH_ACR_ICEN | FLASH_ACR_DCEN);
    while ((FLASH->ACR & FLASH_ACR_LATENCY) != FLASH_ACR_LATENCY_4WS);

    /* 5. Enable HSI16 */
    RCC->CR |= RCC_CR_HSION;
    while (!(RCC->CR & RCC_CR_HSIRDY));

    /* 6. Configure PLL: Source=HSI16, M=4, N=85, R=2 */
    RCC->PLLCFGR = (2U << RCC_PLLCFGR_PLLSRC_Pos)   |
                   ((4U - 1U) << RCC_PLLCFGR_PLLM_Pos) |
                   (85U << RCC_PLLCFGR_PLLN_Pos)     |
                   (0U << RCC_PLLCFGR_PLLR_Pos)      |
                   RCC_PLLCFGR_PLLREN;

    /* 7. Enable PLL and wait for lock */
    RCC->CR |= RCC_CR_PLLON;
    while (!(RCC->CR & RCC_CR_PLLRDY));

    /* 8. Configure Bus Prescalers: AHB /1, APB1 /1, APB2 /1 */
    RCC->CFGR &= ~(RCC_CFGR_HPRE | RCC_CFGR_PPRE1 | RCC_CFGR_PPRE2);

    /* 9. Switch System Clock to PLL */
    RCC->CFGR &= ~RCC_CFGR_SW;
    RCC->CFGR |= RCC_CFGR_SW_PLL;
    while ((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_PLL);
}

int main(void) {
    SystemClock_Config_170MHz();
    
    /* Verify by toggling PA5 at 170 MHz core speed */
    RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN;
    GPIOA->MODER &= ~(3U << 10);
    GPIOA->MODER |=  (1U << 10);

    while (1) {
        GPIOA->BSRR = (1U << 5);
        GPIOA->BSRR = (1U << 21);
    }
}
```

## 6. Debugging & Register Verification Procedure
In GDB, read `RCC->CFGR` to confirm `SWS[1:0]` is `11b` (0x0C). Inspect `FLASH->ACR` to ensure `LATENCY` reads `0x04`.

## 7. Common Beginner Mistakes & Diagnostic Guidance
1. Switching to 170 MHz without first increasing Flash wait states (causes immediate HardFault).
2. Omitting PWR Boost mode enable in `PWR_CR5`, running the chip out-of-spec above 150 MHz.

## 8. Expected Output & Verification
The microcontroller executes without fault at 170 MHz; maximum GPIO toggle frequency increases substantially.
