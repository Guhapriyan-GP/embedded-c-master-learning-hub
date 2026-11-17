# Lab 06: General-Purpose Timer Periodic Interrupts (TIM2)

## 1. Learning Objectives
Configure 32-bit hardware timer TIM2 to generate periodic update interrupts at 10 Hz without polling loops.

## 2. Hardware Requirements & Wiring
- **Hardware:** NUCLEO-G474RE board.
- **Wiring:** Internal timer peripheral; Green LED on PA5.

## 3. Peripheral Registers & Official Documentation
- RCC->APB1ENR1 (TIM2EN)
- TIM2->PSC (Prescaler)
- TIM2->ARR (Auto-reload)
- TIM2->DIER (UIE update interrupt enable)
- TIM2->SR (UIF update flag)
- TIM2->CR1 (CEN counter enable)

**Reference Documentation:** RM0440 Chapter 22 (General-purpose timers TIM2/TIM3/TIM4).

## 4. Mathematical Calculations & Bitfield Settings
f_TIM = 170 MHz. Let PSC = 16999 (divide by 17,000 -> 10,000 Hz count clock). For 10 Hz: ARR = (10,000 / 10) - 1 = 999.

## 5. Complete Bare-Metal C Source Code
```c
#include "stm32g474xx.h"

void TIM2_IRQHandler(void) {
    if (TIM2->SR & TIM_SR_UIF) {
        TIM2->SR &= ~TIM_SR_UIF; /* Clear update flag */
        GPIOA->ODR ^= (1U << 5);  /* Toggle LED at 10 Hz */
    }
}

void TIM2_Init_10Hz(void) {
    RCC->APB1ENR1 |= RCC_APB1ENR1_TIM2EN;

    TIM2->PSC = 16999U; /* 170 MHz / 17000 = 10 kHz */
    TIM2->ARR = 999U;   /* 10 kHz / 1000 = 10 Hz */

    TIM2->DIER |= TIM_DIER_UIE; /* Enable update interrupt */
    
    NVIC_SetPriority(TIM2_IRQn, 4);
    NVIC_EnableIRQ(TIM2_IRQn);

    TIM2->CR1 |= TIM_CR1_CEN;   /* Start timer */
}

int main(void) {
    RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN;
    GPIOA->MODER &= ~(3U << 10);
    GPIOA->MODER |=  (1U << 10);

    TIM2_Init_10Hz();

    while (1) {
        __WFI();
    }
}
```

## 6. Debugging & Register Verification Procedure
In GDB, monitor `TIM2->CNT` and verify that it counts up to 999 and resets, firing `TIM2_IRQHandler` exactly 10 times per second.

## 7. Common Beginner Mistakes & Diagnostic Guidance
1. Forgetting to clear `TIM2->SR` bit `UIF` inside the ISR.
2. Incorrect APB timer clock assumption when APB prescaler is not 1.

## 8. Expected Output & Verification
The green LED blinks rapidly at 10 Hz (toggling every 100 ms).
