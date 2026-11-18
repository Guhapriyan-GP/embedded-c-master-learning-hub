/**
 * @file    gpio_g4.h
 * @brief   Bare-metal CMSIS GPIO driver for STM32G474RE (AHB2 ports A-D, F-G).
 *
 * Handles STM32G4-specific details:
 *   - Reset default MODER is 0b11 (Analog), NOT 0b00 (Input).
 *   - Configures OTYPER/OSPEEDR/PUPDR/AFR BEFORE writing MODER to prevent glitches.
 *   - Uses atomic BSRR writes for ISR-safe pin set/reset operations.
 *
 * Reference: RM0440 Chapter 9 (GPIO), DS12288 Table 12-13 (LQFP64 pinout).
 * Concepts:  PERIPH-GPIO-001, EMB-MMIO-001
 */

#ifndef GPIO_G4_H
#define GPIO_G4_H

#include "stm32g474xx.h"
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    GPIO_MODE_INPUT  = 0x0U,
    GPIO_MODE_OUTPUT = 0x1U,
    GPIO_MODE_AF     = 0x2U,
    GPIO_MODE_ANALOG = 0x3U
} gpio_mode_t;

typedef enum {
    GPIO_OTYPE_PUSHPULL  = 0x0U,
    GPIO_OTYPE_OPENDRAIN = 0x1U
} gpio_otype_t;

typedef enum {
    GPIO_SPEED_LOW       = 0x0U,
    GPIO_SPEED_MEDIUM    = 0x1U,
    GPIO_SPEED_HIGH      = 0x2U,
    GPIO_SPEED_VERY_HIGH = 0x3U
} gpio_speed_t;

typedef enum {
    GPIO_PULL_NONE = 0x0U,
    GPIO_PULL_UP   = 0x1U,
    GPIO_PULL_DOWN = 0x2U
} gpio_pull_t;

typedef struct {
    uint8_t      pin;      /**< Pin number 0..15                              */
    gpio_mode_t  mode;     /**< Input / Output / Alternate Function / Analog  */
    gpio_otype_t otype;    /**< Push-Pull or Open-Drain                       */
    gpio_speed_t speed;    /**< Output slew-rate control                      */
    gpio_pull_t  pull;     /**< Internal pull resistor (NONE/UP/DOWN)         */
    uint8_t      af;       /**< Alternate function 0..15 (used if mode == AF) */
} gpio_pin_config_t;

/**
 * @brief Enable the AHB2 peripheral clock for the given GPIO port (A..G).
 *        Performs a readback barrier to satisfy the 2-cycle AHB clock sync delay.
 */
void GPIO_EnableClock(const GPIO_TypeDef *port);

/**
 * @brief Configure a single GPIO pin cleanly (sets OTYPER/OSPEEDR/PUPDR/AFR before MODER).
 * @return true on success, false if port is NULL or pin > 15 / af > 15.
 */
bool GPIO_ConfigPin(GPIO_TypeDef *port, const gpio_pin_config_t *cfg);

/**
 * @brief Atomically drive pin HIGH via BSRR[15:0].
 */
static inline void GPIO_SetPin(GPIO_TypeDef *port, uint8_t pin) {
    port->BSRR = (1UL << pin);
}

/**
 * @brief Atomically drive pin LOW via BSRR[31:16].
 */
static inline void GPIO_ResetPin(GPIO_TypeDef *port, uint8_t pin) {
    port->BSRR = (1UL << (pin + 16U));
}

/**
 * @brief Atomically write logic level @p high to pin via BSRR.
 */
static inline void GPIO_WritePin(GPIO_TypeDef *port, uint8_t pin, bool high) {
    port->BSRR = high ? (1UL << pin) : (1UL << (pin + 16U));
}

/**
 * @brief Toggle an output pin using BSRR based on current ODR state.
 */
static inline void GPIO_TogglePin(GPIO_TypeDef *port, uint8_t pin) {
    uint32_t mask = (1UL << pin);
    if ((port->ODR & mask) != 0U) {
        port->BSRR = (1UL << (pin + 16U));
    } else {
        port->BSRR = mask;
    }
}

/**
 * @brief Read current digital input level from IDR.
 */
static inline bool GPIO_ReadPin(const GPIO_TypeDef *port, uint8_t pin) {
    return ((port->IDR & (1UL << pin)) != 0U);
}

#ifdef __cplusplus
}
#endif

#endif /* GPIO_G4_H */
