/**
 * @file main.c
 * @brief Bare-Metal Embedded C Blinky & Button Input for STM32G474RE (NUCLEO-G474RE).
 *
 * Hardware connections on NUCLEO-G474RE:
 * - User LED (LD2, Green) : Pin PA5 (Active HIGH)
 * - User Button (B1, Blue) : Pin PC13 (Active LOW with external pull-up)
 *
 * Direct register-level programming: NO HAL, NO peripheral libraries.
 */

#include "stm32g474xx.h"

/* Software delay loop */
static void delay(volatile uint32_t count) {
    while (count--) {
        __asm__("nop");
    }
}

int main(void) {
    /* -------------------------------------------------------------
     * 1. Enable AHB2 Peripheral Clocks:
     *    - Bit 0: GPIOAEN (for PA5 / Green User LED LD2)
     *    - Bit 2: GPIOCEN (for PC13 / Blue User Button B1)
     * ------------------------------------------------------------- */
    RCC->AHB2ENR |= (RCC_AHB2ENR_GPIOAEN | RCC_AHB2ENR_GPIOCEN);

    /* Small delay to allow peripheral clock to stabilize */
    delay(10);

    /* -------------------------------------------------------------
     * 2. Configure PA5 as General Purpose Output:
     *    - GPIOA->MODER bits [11:10]:
     *      00: Input, 01: Output, 10: Alternate Function, 11: Analog
     * ------------------------------------------------------------- */
    GPIOA->MODER &= ~(0x3UL << (5 * 2)); // Clear bits 11:10
    GPIOA->MODER |=  (0x1UL << (5 * 2)); // Set bit 10 (Output mode)

    /* Output type: Push-Pull (Bit 5 = 0) */
    GPIOA->OTYPER &= ~(1UL << 5);

    /* Output speed: Low speed (Bits 11:10 = 00) */
    GPIOA->OSPEEDR &= ~(0x3UL << (5 * 2));

    /* No pull-up / pull-down (Bits 11:10 = 00) */
    GPIOA->PUPDR &= ~(0x3UL << (5 * 2));

    /* -------------------------------------------------------------
     * 3. Configure PC13 as Digital Input:
     *    - GPIOC->MODER bits [27:26]:
     *      00: Input mode
     * ------------------------------------------------------------- */
    GPIOC->MODER &= ~(0x3UL << (13 * 2)); // Bits 27:26 = 00 (Input)
    GPIOC->PUPDR &= ~(0x3UL << (13 * 2)); // Floating (board has hardware pull-up)

    /* -------------------------------------------------------------
     * 4. Super Loop: Blink LED and check Button
     * ------------------------------------------------------------- */
    while (1) {
        /* Check if User Button B1 (PC13) is pressed (Active LOW) */
        uint32_t is_button_pressed = !(GPIOC->IDR & (1UL << 13));

        uint32_t blink_delay = is_button_pressed ? 100000UL : 500000UL;

        /* Turn LED ON (Set PA5 via BSRR bit 5) */
        GPIOA->BSRR = (1UL << 5);
        delay(blink_delay);

        /* Turn LED OFF (Reset PA5 via BSRR bit 21 [5 + 16]) */
        GPIOA->BSRR = (1UL << (5 + 16));
        delay(blink_delay);
    }

    return 0;
}
