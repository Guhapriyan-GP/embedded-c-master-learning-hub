# Lab 14: Low-Power Stop Mode with EXTI Wakeup

## 1. Learning Objectives
Configure the STM32G4 power architecture into low-power Stop 0 mode, reducing power consumption, and wake up back into full 170 MHz operation via an external button interrupt.

## 2. Hardware Requirements & Wiring
- **Hardware:** NUCLEO-G474RE board, digital multimeter to measure JP5 current.
- **Wiring:** Current meter connected across JP5 jumper (IDD measurement jumper).

## 3. Peripheral Registers & Official Documentation
- PWR->CR1 (LPMS bits: Stop mode)
- SCB->SCR (SLEEPDEEP bit)
- EXTI->IMR1, EXTI->FTSR1
- SystemClock_Config()

**Reference Documentation:** RM0440 Chapter 5 (Power control PWR) and PM0214 (System Control Register).

## 4. Mathematical Calculations & Bitfield Settings
Stop mode disables HSI/HSE/PLL. On wakeup, MCU defaults to HSI16; clocks must be re-initialized to 170 MHz.

## 5. Complete Bare-Metal C Source Code
```c
#include "stm32g474xx.h"

extern void SystemClock_Config_170MHz(void);

void Enter_Stop0_Mode(void) {
    /* 1. Select Stop 0 mode in PWR_CR1 (LPMS = 000b) */
    PWR->CR1 &= ~PWR_CR1_LPMS;

    /* 2. Set SLEEPDEEP in Cortex-M4 SCB->SCR */
    SCB->SCR |= SCB_SCR_SLEEPDEEP_Msk;

    /* 3. Wait for Interrupt to enter Stop mode */
    __WFI();

    /* 4. On wakeup, core runs on HSI16. Re-enable 170 MHz PLL! */
    SystemClock_Config_170MHz();
}

int main(void) {
    /* Configure button on PC13 for EXTI wakeup */
    RCC->AHB2ENR |= RCC_AHB2ENR_GPIOCEN;
    RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN;
    SYSCFG->EXTICR[3] |= (2U << 4);
    EXTI->FTSR1 |= (1U << 13);
    EXTI->IMR1  |= (1U << 13);
    NVIC_EnableIRQ(EXTI15_10_IRQn);

    while (1) {
        Enter_Stop0_Mode();
        /* Wakeup occurred: execute task, then return to sleep */
    }
}
```

## 6. Debugging & Register Verification Procedure
Measure current on JP5: Drops from ~35 mA down to microamp levels in Stop mode, jumping back upon button press.

## 7. Common Beginner Mistakes & Diagnostic Guidance
1. Forgetting to re-configure the PLL clock tree after waking up from Stop mode (MCU continues running at 16 MHz HSI).
2. Leaving unnecessary floating I/O pins consuming static leakage current.

## 8. Expected Output & Verification
Power consumption drops dramatically while retaining complete SRAM state.
