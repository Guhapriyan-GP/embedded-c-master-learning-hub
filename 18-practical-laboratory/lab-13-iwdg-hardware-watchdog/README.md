# Lab 13: Independent Watchdog (IWDG) System Health Monitoring

## 1. Learning Objectives
Implement the hardware Independent Watchdog (IWDG) running from the internal 32 kHz LSI clock to protect against software lockup, demonstrating both successful refresh and deliberate timeout reset.

## 2. Hardware Requirements & Wiring
- **Hardware:** NUCLEO-G474RE board.
- **Wiring:** Pushbutton B1 on PC13 used to trigger an intentional blocking loop; LED on PA5 indicates reset.

## 3. Peripheral Registers & Official Documentation
- IWDG->KR (Key register: 0x5555, 0xAAAA, 0xCCCC)
- IWDG->PR (Prescaler)
- IWDG->RLR (Reload register)
- RCC->CSR (IWDGRSTF reset flag)

**Reference Documentation:** RM0440 Chapter 30 (IWDG) and Section 6.4.21 (RCC_CSR).

## 4. Mathematical Calculations & Bitfield Settings
LSI = 32 kHz. Prescaler /64 -> 500 Hz (2 ms/tick). Target: 1.0 s -> RLR = 499.

## 5. Complete Bare-Metal C Source Code
```c
#include "stm32g474xx.h"

void IWDG_Init_1s(void) {
    IWDG->KR = 0xCCCC; /* Start watchdog */
    IWDG->KR = 0x5555; /* Unlock PR and RLR */
    IWDG->PR = 4U;     /* /64 */
    IWDG->RLR = 499U;  /* 1 second */
    while (IWDG->SR);  /* Wait for sync */
    IWDG->KR = 0xAAAA; /* Initial kick */
}

int main(void) {
    RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN | RCC_AHB2ENR_GPIOCEN;
    GPIOA->MODER &= ~(3U << 10);
    GPIOA->MODER |=  (1U << 10);

    /* Check if system reset was caused by IWDG */
    if (RCC->CSR & RCC_CSR_IWDGRSTF) {
        RCC->CSR |= RCC_CSR_RMVF; /* Clear reset flags */
        /* Rapidly blink LED 5 times to indicate watchdog reset */
        for (int i = 0; i < 10; i++) {
            GPIOA->ODR ^= (1U << 5);
            for (volatile int d = 0; d < 200000; d++);
        }
    }

    IWDG_Init_1s();

    while (1) {
        /* Normal operation: refresh watchdog */
        IWDG->KR = 0xAAAA;

        /* If button pressed, simulate crash by entering infinite loop */
        if ((GPIOC->IDR & (1U << 13)) == 0) {
            while (1); /* Watchdog will reset MCU after 1 second */
        }
        for (volatile int d = 0; d < 1000000; d++);
    }
}
```

## 6. Debugging & Register Verification Procedure
Run under GDB. Press the blue button. After 1 second, observe GDB report that the target was reset.

## 7. Common Beginner Mistakes & Diagnostic Guidance
1. Refreshing the watchdog inside an ISR (masks thread-level deadlocks).
2. Trying to stop the watchdog in software (IWDG cannot be stopped once started).

## 8. Expected Output & Verification
The microcontroller recovers automatically from an induced lockup, rebooting safely.
