# Lab 02: GPIO Pushbutton Input with State Machine Debouncing

## 1. Learning Objectives
Read the Blue User Button (B1) on PC13 and implement an active-low software debouncing state machine.

## 2. Hardware Requirements & Wiring
- **Hardware:** NUCLEO-G474RE board.
- **Wiring:** Onboard Blue Pushbutton B1 is connected to pin PC13 with an external hardware RC filter and pull-up resistor. Pressing the button pulls PC13 to GND (Active LOW).

## 3. Peripheral Registers & Official Documentation
- RCC->AHB2ENR (Bit 2: GPIOCEN)
- GPIOC->MODER (Bits [27:26]: MODER13)
- GPIOC->PUPDR (Bits [27:26]: PUPDR13)
- GPIOC->IDR (Bit 13: IDR13)

**Reference Documentation:** RM0440 Section 9.4.5 (GPIOx_IDR) and UM2505 Section 6.5 (Push-buttons).

## 4. Mathematical Calculations & Bitfield Settings
PC13 input level is checked via `(GPIOC->IDR & (1U << 13))`. Button pressed = 0; button released = 1.

## 5. Complete Bare-Metal C Source Code
```c
#include "stm32g474xx.h"
#include <stdbool.h>

void delay_ms(volatile uint32_t ms) {
    while (ms--) {
        for (volatile int i = 0; i < 4000; i++) __NOP();
    }
}

bool button_is_pressed(void) {
    /* Active low: 0 indicates pressed */
    return (GPIOC->IDR & (1U << 13U)) == 0;
}

int main(void) {
    /* Enable GPIOA (LED) and GPIOC (Button) clocks */
    RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN | RCC_AHB2ENR_GPIOCEN;

    /* PA5 as Output */
    GPIOA->MODER &= ~(3U << (5U * 2U));
    GPIOA->MODER |=  (1U << (5U * 2U));

    /* PC13 as Digital Input (00b) */
    GPIOC->MODER &= ~(3U << (13U * 2U));
    GPIOC->PUPDR &= ~(3U << (13U * 2U)); /* Board has external pull-up */

    bool last_state = false;

    while (1) {
        bool current_state = button_is_pressed();

        /* Detect rising edge of press with debouncing delay */
        if (current_state && !last_state) {
            delay_ms(20); /* 20 ms debounce window */
            if (button_is_pressed()) {
                GPIOA->ODR ^= (1U << 5U); /* Toggle LED */
            }
        }
        last_state = current_state;
        delay_ms(10);
    }
}
```

## 6. Debugging & Register Verification Procedure
Place a watch expression on `GPIOC->IDR & (1 << 13)` in the debugger. Press and release the blue button to verify the bit changes between 0 and 1.

## 7. Common Beginner Mistakes & Diagnostic Guidance
1. Inverting logic: Assuming button press reads '1' (it reads '0' because of the active-low pull-up configuration).
2. Omitting debounce logic, resulting in multiple LED toggles per single mechanical press.

## 8. Expected Output & Verification
Every discrete press of the blue button toggles the green LED state cleanly exactly once.
