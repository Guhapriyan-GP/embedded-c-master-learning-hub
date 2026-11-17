# Lab 04: SysTick Accurate Millisecond Delay Generator

## 1. Learning Objectives
Configure the 24-bit ARM Cortex-M4 SysTick timer to generate exact 1 millisecond system ticks and implement non-blocking and blocking delay APIs.

## 2. Hardware Requirements & Wiring
- **Hardware:** NUCLEO-G474RE board.
- **Wiring:** Internal SysTick core peripheral; Green LED on PA5.

## 3. Peripheral Registers & Official Documentation
- SysTick->CTRL (ENABLE, TICKINT, CLKSOURCE)
- SysTick->LOAD (24-bit reload value)
- SysTick->VAL (Current value)

**Reference Documentation:** PM0214 Section 4.5 (SysTick timer).

## 4. Mathematical Calculations & Bitfield Settings
f_HCLK = 170,000,000 Hz. For 1 ms tick: LOAD = (170,000,000 / 1000) - 1 = 169,999 (0x2980F).

## 5. Complete Bare-Metal C Source Code
```c
#include "stm32g474xx.h"

volatile uint32_t ms_ticks = 0;

void SysTick_Handler(void) {
    ms_ticks++;
}

void SysTick_Init(void) {
    SysTick->CTRL = 0;
    SysTick->LOAD = (170000000UL / 1000UL) - 1UL;
    SysTick->VAL  = 0;
    SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk |
                    SysTick_CTRL_TICKINT_Msk   |
                    SysTick_CTRL_ENABLE_Msk;
}

void delay_ms(uint32_t delay) {
    uint32_t start = ms_ticks;
    while ((ms_ticks - start) < delay);
}

int main(void) {
    /* Assume 170 MHz clock initialized */
    SysTick_Init();

    RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN;
    GPIOA->MODER &= ~(3U << 10);
    GPIOA->MODER |=  (1U << 10);

    while (1) {
        GPIOA->BSRR = (1U << 5);  /* LED ON */
        delay_ms(500);
        GPIOA->BSRR = (1U << 21); /* LED OFF */
        delay_ms(500);
    }
}
```

## 6. Debugging & Register Verification Procedure
Put a breakpoint in `SysTick_Handler`. Check that `ms_ticks` increments monotonically and that `SysTick->VAL` counts down from 169,999 to 0.

## 7. Common Beginner Mistakes & Diagnostic Guidance
1. Forgetting to qualify `ms_ticks` as `volatile` (causes delay loop to hang forever under compiler optimization).
2. Exceeding 24-bit maximum in `SysTick->LOAD` (values above 0x00FFFFFF get truncated).

## 8. Expected Output & Verification
The green LED toggles exactly once per second (500 ms ON, 500 ms OFF) verified against a stopwatch or logic analyzer.
