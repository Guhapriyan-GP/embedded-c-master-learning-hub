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

bool GPIO_ConfigInput(GPIO_TypeDef *port, uint8_t pin, gpio_pull_t pull) {
    const gpio_pin_config_t cfg = {
        .pin   = pin,
        .mode  = GPIO_MODE_INPUT,
        .otype = GPIO_OTYPE_PUSHPULL,
        .speed = GPIO_SPEED_LOW,
        .pull  = pull,
        .af    = 0U
    };
    return GPIO_ConfigPin(port, &cfg);
}

bool GPIO_ConfigOutput(GPIO_TypeDef *port, uint8_t pin, gpio_otype_t otype,
                       gpio_speed_t speed, gpio_pull_t pull) {
    const gpio_pin_config_t cfg = {
        .pin   = pin,
        .mode  = GPIO_MODE_OUTPUT,
        .otype = otype,
        .speed = speed,
        .pull  = pull,
        .af    = 0U
    };
    return GPIO_ConfigPin(port, &cfg);
}

bool GPIO_ConfigAF(GPIO_TypeDef *port, uint8_t pin, uint8_t af,
                   gpio_otype_t otype, gpio_speed_t speed, gpio_pull_t pull) {
    const gpio_pin_config_t cfg = {
        .pin   = pin,
        .mode  = GPIO_MODE_AF,
        .otype = otype,
        .speed = speed,
        .pull  = pull,
        .af    = af
    };
    return GPIO_ConfigPin(port, &cfg);
}

bool GPIO_ConfigAnalog(GPIO_TypeDef *port, uint8_t pin) {
    const gpio_pin_config_t cfg = {
        .pin   = pin,
        .mode  = GPIO_MODE_ANALOG,
        .otype = GPIO_OTYPE_PUSHPULL,
        .speed = GPIO_SPEED_LOW,
        .pull  = GPIO_PULL_NONE,
        .af    = 0U
    };
    return GPIO_ConfigPin(port, &cfg);
}

bool GPIO_LockPin(GPIO_TypeDef *port, uint8_t pin) {
    if ((port == NULL) || (pin > 15U)) {
        return false;
    }

    uint32_t mask = (1UL << pin);

    /* Per RM0440 §9.4.8, once LCKK is latched 1, subsequent LCKR writes are
     * ignored until MCU reset. Return true ONLY if this pin is already locked. */
    if ((port->LCKR & GPIO_LCKR_LCKK) != 0U) {
        return ((port->LCKR & mask) == mask);
    }

    /* RM0440 §9.4.8 mandatory 4-step LCKR key write sequence */
    port->LCKR = GPIO_LCKR_LCKK | mask;
    port->LCKR = mask;
    port->LCKR = GPIO_LCKR_LCKK | mask;
    (void)port->LCKR;

    /* Verify BOTH LCKK and target pin bit latched 1 */
    uint32_t expected = GPIO_LCKR_LCKK | mask;
    return ((port->LCKR & expected) == expected);
}
