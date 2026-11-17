# Lab 05: EXTI Pushbutton Interrupt with NVIC Priority Configuration

## 1. Learning Objectives
Configure the PC13 Blue Pushbutton to trigger an external interrupt (EXTI13) through SYSCFG, set its NVIC preemption priority, and handle flag clearing in the ISR.

## 2. Hardware Requirements & Wiring
- **Hardware:** NUCLEO-G474RE board.
- **Wiring:** Blue Button on PC13, Green LED on PA5.

## 3. Peripheral Registers & Official Documentation
- RCC->AHB2ENR (GPIOCEN, GPIOAEN)
- RCC->APB2ENR (SYSCFGEN)
- SYSCFG->EXTICR[3] (EXTI13 Port C selection)
- EXTI->FTSR1 (Falling trigger enable)
- EXTI->IMR1 (Interrupt unmask)
- EXTI->PR1 (Pending register - rc_w1)
- NVIC ISER / IPR (EXTI15_10_IRQn)

**Reference Documentation:** RM0440 Section 13 (SYSCFG) & Section 14 (EXTI).

## 4. Mathematical Calculations & Bitfield Settings
PC13 routes to EXTI13. EXTICR[3] bits [7:4] set to 0010b (Port C).

## 5. Complete Bare-Metal C Source Code
```c
#include "stm32g474xx.h"

void EXTI15_10_IRQHandler(void) {
    if (EXTI->PR1 & (1U << 13)) {
        EXTI->PR1 = (1U << 13); /* Clear flag by writing 1 */
        GPIOA->ODR ^= (1U << 5); /* Toggle LED */
    }
}

int main(void) {
    RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN | RCC_AHB2ENR_GPIOCEN;
    RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN;

    /* PA5 Output */
    GPIOA->MODER &= ~(3U << 10);
    GPIOA->MODER |=  (1U << 10);

    /* PC13 Input */
    GPIOC->MODER &= ~(3U << 26);

    /* Route PC13 to EXTI13 */
    SYSCFG->EXTICR[3] &= ~(0xFU << 4);
    SYSCFG->EXTICR[3] |=  (2U   << 4);

    /* Falling edge trigger & unmask */
    EXTI->FTSR1 |= (1U << 13);
    EXTI->IMR1  |= (1U << 13);

    /* NVIC configuration */
    NVIC_SetPriority(EXTI15_10_IRQn, 3);
    NVIC_EnableIRQ(EXTI15_10_IRQn);

    while (1) {
        __WFI(); /* Sleep and wait for interrupt */
    }
}
```

## 6. Debugging & Register Verification Procedure
Place a breakpoint inside `EXTI15_10_IRQHandler`. Press the blue button and verify that the debugger halts inside the handler.

## 7. Common Beginner Mistakes & Diagnostic Guidance
1. Forgetting to clear `EXTI->PR1` bit 13 (causes the ISR to re-execute infinitely).
2. Forgetting to enable `SYSCFGEN` in `RCC->APB2ENR` (EXTI mapping fails silently).

## 8. Expected Output & Verification
The CPU remains asleep in low-power WFI state until the blue button is pressed, toggling the LED immediately.
