# Lab 07: Hardware PWM Output & LED Fading (TIM3 Channel 1)

## 1. Learning Objectives
Configure TIM3 Channel 1 in PWM Mode 1 to output a 25 kHz variable duty-cycle signal on PA6 to smoothly fade an external LED.

## 2. Hardware Requirements & Wiring
- **Hardware:** NUCLEO-G474RE, external LED with 330 ohm resistor connected to PA6.
- **Wiring:** PA6 -> 330 ohm resistor -> LED anode; LED cathode -> GND.

## 3. Peripheral Registers & Official Documentation
- TIM3->CCMR1 (OC1M, OC1PE)
- TIM3->CCER (CC1E)
- TIM3->CCR1 (Duty cycle)
- TIM3->ARR (Period)
- TIM3->CR1 (ARPE, CEN)
- GPIOA->AFR[0] (AF2 on PA6)

**Reference Documentation:** RM0440 Chapter 22 and Datasheet DS12288 Table 14.

## 4. Mathematical Calculations & Bitfield Settings
f_TIM = 170 MHz. 25 kHz target: ARR = (170,000,000 / 25,000) - 1 = 6799. Duty cycle 0 to 100% maps to CCR1 = 0 to 6799.

## 5. Complete Bare-Metal C Source Code
```c
#include "stm32g474xx.h"

void delay_ms(volatile uint32_t ms) {
    while (ms--) {
        for (volatile int i = 0; i < 4000; i++) __NOP();
    }
}

int main(void) {
    RCC->APB1ENR1 |= RCC_APB1ENR1_TIM3EN;
    RCC->AHB2ENR  |= RCC_AHB2ENR_GPIOAEN;

    /* PA6 to Alternate Function AF2 (TIM3_CH1) */
    GPIOA->MODER &= ~(3U << 12);
    GPIOA->MODER |=  (2U << 12);
    GPIOA->AFR[0] &= ~(0xFU << 24);
    GPIOA->AFR[0] |=  (2U   << 24); /* AF2 = TIM3_CH1 */

    TIM3->PSC = 0;
    TIM3->ARR = 6799; /* 25 kHz */
    TIM3->CCR1 = 0;

    /* PWM Mode 1 with preload */
    TIM3->CCMR1 |= (6U << TIM_CCMR1_OC1M_Pos) | TIM_CCMR1_OC1PE;
    TIM3->CCER  |= TIM_CCER_CC1E;
    TIM3->CR1   |= TIM_CR1_ARPE | TIM_CR1_CEN;

    while (1) {
        /* Fade in */
        for (uint32_t duty = 0; duty <= 6799; duty += 50) {
            TIM3->CCR1 = duty;
            delay_ms(5);
        }
        /* Fade out */
        for (uint32_t duty = 6799; duty > 0; duty -= 50) {
            TIM3->CCR1 = duty;
            delay_ms(5);
        }
    }
}
```

## 6. Debugging & Register Verification Procedure
Probe PA6 with an oscilloscope. Verify a clean 25.00 kHz square wave whose duty cycle expands from 0% to 100% and back.

## 7. Common Beginner Mistakes & Diagnostic Guidance
1. Forgetting to set `CC1E` in `TIM3->CCER` (pin remains disconnected from timer output).
2. Omitting `OC1PE` preload enable, risking glitching when modifying `CCR1`.

## 8. Expected Output & Verification
The LED smoothly breathes brighter and dimmer without any visible flicker.
