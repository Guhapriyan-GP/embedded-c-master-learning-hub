# Lab 01: Bare-Metal GPIO LED Blinky (Direct Register Access & CMSIS)

## 1. Learning Objectives
Configure GPIOA Pin 5 (Green User LED LD2) in output push-pull mode and toggle it using direct register writes.

## 2. Hardware Requirements & Wiring
- **Hardware:** NUCLEO-G474RE board, USB-A to Micro-USB cable.
- **Wiring:** Onboard Green LED LD2 is internally connected to pin PA5. No external breadboard wiring required.

## 3. Peripheral Registers & Official Documentation
- RCC->AHB2ENR (Offset 0x4C, Bit 0: GPIOAEN)
- GPIOA->MODER (Offset 0x00, Bits [11:10]: MODER5)
- GPIOA->OTYPER (Offset 0x04, Bit 5: OT5)
- GPIOA->OSPEEDR (Offset 0x08, Bits [11:10]: OSPEEDR5)
- GPIOA->PUPDR (Offset 0x0C, Bits [11:10]: PUPDR5)
- GPIOA->BSRR (Offset 0x18, Bit 5: BS5, Bit 21: BR5)

**Reference Documentation:** RM0440 Section 6.4.14 (RCC_AHB2ENR) and Section 9.4 (GPIO registers).

## 4. Mathematical Calculations & Bitfield Settings
MODER5 requires setting bits [11:10] to 01b (Output). Mask = ~(3U << 10), Set = (1U << 10).

## 5. Complete Bare-Metal C Source Code
```c
#include "stm32g474xx.h"

void delay_cycles(volatile uint32_t count) {
    while (count--) {
        __NOP();
    }
}

int main(void) {
    /* 1. Enable GPIOA peripheral clock */
    RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN;

    /* 2. Configure PA5 as Output (01b) */
    GPIOA->MODER &= ~(3U << (5U * 2U));
    GPIOA->MODER |=  (1U << (5U * 2U));

    /* 3. Output push-pull, low speed, no pull-up/pull-down */
    GPIOA->OTYPER &= ~(1U << 5U);
    GPIOA->OSPEEDR &= ~(3U << (5U * 2U));
    GPIOA->PUPDR   &= ~(3U << (5U * 2U));

    while (1) {
        /* Atomic Pin Set via BSRR (Bit 5) */
        GPIOA->BSRR = (1U << 5U);
        delay_cycles(1000000);

        /* Atomic Pin Reset via BSRR (Bit 21 = 5 + 16) */
        GPIOA->BSRR = (1U << (5U + 16U));
        delay_cycles(1000000);
    }
}
```

## 6. Debugging & Register Verification Procedure
Set a breakpoint on `GPIOA->BSRR = (1U << 5U);`. In VS Code Cortex-Debug, open the XPERIPHERALS window, navigate to `GPIOA -> ODR`, and observe ODR5 toggle between 1 and 0 as you step.

## 7. Common Beginner Mistakes & Diagnostic Guidance
1. Forgetting to enable the GPIOA clock in `RCC->AHB2ENR` (results in a silent write that has no effect).
2. Using non-atomic `ODR ^= (1 << 5)` in interrupt-heavy code, risking race conditions.
3. Using non-volatile delay counter variables that get eliminated by `-O2` compiler optimization.

## 8. Expected Output & Verification
The green LED (LD2) on the NUCLEO board blinks at a steady ~1 Hz rate.
