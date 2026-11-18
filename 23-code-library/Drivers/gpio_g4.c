/**
 * @file    gpio_g4.c
 * @brief   Bare-metal CMSIS GPIO driver implementation for STM32G474RE.
 */

#include "gpio_g4.h"
#include <stddef.h>

void GPIO_EnableClock(const GPIO_TypeDef *port) {
    uint32_t en_bit = 0U;

    if (port == GPIOA) {
        en_bit = RCC_AHB2ENR_GPIOAEN;
    } else if (port == GPIOB) {
        en_bit = RCC_AHB2ENR_GPIOBEN;
    } else if (port == GPIOC) {
        en_bit = RCC_AHB2ENR_GPIOCEN;
    } else if (port == GPIOD) {
        en_bit = RCC_AHB2ENR_GPIODEN;
    } else if (port == GPIOE) {
        en_bit = RCC_AHB2ENR_GPIOEEN;
    } else if (port == GPIOF) {
        en_bit = RCC_AHB2ENR_GPIOFEN;
    } else if (port == GPIOG) {
        en_bit = RCC_AHB2ENR_GPIOGEN;
    } else {
        return;
    }

    RCC->AHB2ENR |= en_bit;
    /* Readback guarantees AHB2 clock enable propagates before GPIO register access */
    (void)RCC->AHB2ENR;
}

bool GPIO_ConfigPin(GPIO_TypeDef *port, const gpio_pin_config_t *cfg) {
    if ((port == NULL) || (cfg == NULL) || (cfg->pin > 15U) || (cfg->af > 15U)) {
        return false;
    }

    uint32_t pin   = (uint32_t)cfg->pin;
    uint32_t shift = pin * 2U;

    /* 1. Output type (1 bit per pin) */
    if (cfg->otype == GPIO_OTYPE_OPENDRAIN) {
        port->OTYPER |= (1UL << pin);
    } else {
        port->OTYPER &= ~(1UL << pin);
    }

    /* 2. Output speed (2 bits per pin) */
    port->OSPEEDR = (port->OSPEEDR & ~(0x3UL << shift)) |
                    (((uint32_t)cfg->speed & 0x3UL) << shift);

    /* 3. Pull-up / pull-down (2 bits per pin) */
    port->PUPDR = (port->PUPDR & ~(0x3UL << shift)) |
                  (((uint32_t)cfg->pull & 0x3UL) << shift);

    /* 4. Alternate Function (4 bits per pin across AFR[0] = AFRL, AFR[1] = AFRH) */
    if (cfg->mode == GPIO_MODE_AF) {
        uint32_t afr_idx   = pin >> 3U;
        uint32_t afr_shift = (pin & 0x7U) * 4U;
        port->AFR[afr_idx] = (port->AFR[afr_idx] & ~(0xFUL << afr_shift)) |
                             (((uint32_t)cfg->af & 0xFUL) << afr_shift);
    }

    /* 5. Mode (2 bits per pin) written LAST to prevent transient glitches.
     *    Explicitly clears STM32G4 Analog (0b11) reset default when selecting Input (0b00). */
    port->MODER = (port->MODER & ~(0x3UL << shift)) |
                  (((uint32_t)cfg->mode & 0x3UL) << shift);

    return true;
}
