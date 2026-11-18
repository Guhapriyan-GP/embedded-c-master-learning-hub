/**
 * @file    gpio_g4.h
 * @brief   Bare-metal CMSIS GPIO driver for STM32G474RE (AHB2 ports A-D, F-G).
 *
 * Handles STM32G4-specific silicon details:
 *   - Reset default MODER is 0b11 (Analog), NOT 0b00 (Input) — except debug pins.
 *   - Configures OTYPER/OSPEEDR/PUPDR/AFR BEFORE writing MODER to prevent glitches.
 *   - Uses atomic BSRR writes for ISR-safe pin set/reset/toggle operations.
 *   - Implements RM0440 §9.4.8 4-step LCKR pin-configuration lock sequence.
 *
 * Reference: RM0440 Chapter 9 (GPIO), DS12288 Table 12-13 (LQFP64 pinout).
 * Concepts:  PERIPH-GPIO-001, PERIPH-GPIO-002, PERIPH-GPIO-003, PERIPH-GPIO-005, EMB-MMIO-001
 */

#ifndef GPIO_G4_H
#define GPIO_G4_H

#include "stm32g474xx.h"
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief GPIO pin mode (MODER[2n+1:2n]). */
typedef enum {
    GPIO_MODE_INPUT  = 0x0U, /**< Digital input (Schmitt trigger enabled)        */
    GPIO_MODE_OUTPUT = 0x1U, /**< General-purpose digital output                 */
    GPIO_MODE_AF     = 0x2U, /**< Alternate Function (muxed via AFR[0..1])       */
    GPIO_MODE_ANALOG = 0x3U  /**< Analog / lowest-power (STM32G4 reset default)  */
} gpio_mode_t;

/** @brief GPIO output driver topology (OTYPER[n]). */
typedef enum {
    GPIO_OTYPE_PUSHPULL  = 0x0U, /**< Active HIGH & active LOW drive             */
    GPIO_OTYPE_OPENDRAIN = 0x1U  /**< Active LOW / Hi-Z HIGH (required for I2C)  */
} gpio_otype_t;

/** @brief GPIO output slew-rate control (OSPEEDR[2n+1:2n]). */
typedef enum {
    GPIO_SPEED_LOW       = 0x0U, /**< Low slew rate (lowest EMI, LEDs/buttons)   */
    GPIO_SPEED_MEDIUM    = 0x1U, /**< Medium slew rate (UART, 100 kHz I2C)       */
    GPIO_SPEED_HIGH      = 0x2U, /**< High slew rate (400 kHz I2C, SPI <=20 MHz) */
    GPIO_SPEED_VERY_HIGH = 0x3U  /**< Very high slew rate (high-speed SPI/MCO)   */
} gpio_speed_t;

/** @brief GPIO internal weak pull resistor (~40 kOhm typical, PUPDR[2n+1:2n]). */
typedef enum {
    GPIO_PULL_NONE = 0x0U, /**< Floating / external pull resistor populated    */
    GPIO_PULL_UP   = 0x1U, /**< Internal weak pull-up                          */
    GPIO_PULL_DOWN = 0x2U  /**< Internal weak pull-down                        */
} gpio_pull_t;

/** @brief Full single-pin configuration descriptor. */
typedef struct {
    uint8_t      pin;      /**< Pin number 0..15                               */
    gpio_mode_t  mode;     /**< Input / Output / Alternate Function / Analog   */
    gpio_otype_t otype;    /**< Push-Pull or Open-Drain                        */
    gpio_speed_t speed;    /**< Output slew-rate control                       */
    gpio_pull_t  pull;     /**< Internal pull resistor (NONE/UP/DOWN)          */
    uint8_t      af;       /**< Alternate function 0..15 (used if mode == AF)  */
} gpio_pin_config_t;

/**
 * @brief Enable the AHB2 peripheral clock for the given GPIO port (A..G).
 *        Performs a readback barrier to satisfy the 2-cycle AHB clock sync delay.
 * @param port  Pointer to GPIOA..GPIOG (NULL or invalid pointer is safely ignored).
 */
void GPIO_EnableClock(const GPIO_TypeDef *port);

/**
 * @brief Configure a single GPIO pin cleanly (sets OTYPER/OSPEEDR/PUPDR/AFR before MODER).
 * @param port  Pointer to GPIOA..GPIOG.
 * @param cfg   Non-NULL pin configuration descriptor (pin <= 15, af <= 15).
 * @return true on success, false if arguments are invalid.
 */
bool GPIO_ConfigPin(GPIO_TypeDef *port, const gpio_pin_config_t *cfg);

/**
 * @brief Convenience helper: configure a pin as Digital Input (explicitly clears
 *        STM32G4's Analog 0b11 reset default).
 * @param port  Pointer to GPIOA..GPIOG.
 * @param pin   Pin index 0..15.
 * @param pull  Internal pull configuration (use GPIO_PULL_NONE for NUCLEO PC13 B1).
 * @return true on success, false if arguments are invalid.
 */
bool GPIO_ConfigInput(GPIO_TypeDef *port, uint8_t pin, gpio_pull_t pull);

/**
 * @brief Convenience helper: configure a pin as General-Purpose Digital Output.
 * @param port   Pointer to GPIOA..GPIOG.
 * @param pin    Pin index 0..15.
 * @param otype  Push-pull or open-drain output stage.
 * @param speed  Output slew rate.
 * @param pull   Internal pull resistor.
 * @return true on success, false if arguments are invalid.
 */
bool GPIO_ConfigOutput(GPIO_TypeDef *port, uint8_t pin, gpio_otype_t otype,
                       gpio_speed_t speed, gpio_pull_t pull);

/**
 * @brief Convenience helper: configure a pin in Alternate Function mode (AF0..AF15).
 * @param port   Pointer to GPIOA..GPIOG.
 * @param pin    Pin index 0..15.
 * @param af     Alternate function number 0..15 (verified against DS12288 Table 13).
 * @param otype  Push-pull or open-drain (use GPIO_OTYPE_OPENDRAIN for I2C).
 * @param speed  Output slew rate.
 * @param pull   Internal pull resistor.
 * @return true on success, false if arguments are invalid.
 */
bool GPIO_ConfigAF(GPIO_TypeDef *port, uint8_t pin, uint8_t af,
                   gpio_otype_t otype, gpio_speed_t speed, gpio_pull_t pull);

/**
 * @brief Convenience helper: park a pin in Analog mode (MODER=0b11, PUPDR=0b00)
 *        for ADC/DAC/OPAMP/COMP inputs or minimum Stop-mode leakage current.
 * @param port  Pointer to GPIOA..GPIOG.
 * @param pin   Pin index 0..15.
 * @return true on success, false if arguments are invalid.
 */
bool GPIO_ConfigAnalog(GPIO_TypeDef *port, uint8_t pin);

/**
 * @brief Lock a pin's configuration registers until the next MCU reset using
 *        the RM0440 §9.4.8 4-step LCKR write sequence.
 * @param port  Pointer to GPIOA..GPIOG.
 * @param pin   Pin index 0..15.
 * @return true if LCKR[16] (LCKK) verified locked, false otherwise.
 */
bool GPIO_LockPin(GPIO_TypeDef *port, uint8_t pin);

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
 * @brief Toggle an output pin using a single branchless atomic BSRR mask write.
 * @note  Unlike `port->ODR ^= mask`, writing `BSRR` affects ONLY @p pin and
 *        cannot corrupt concurrent ISR updates to other pins on the same port.
 */
static inline void GPIO_TogglePin(GPIO_TypeDef *port, uint8_t pin) {
    uint32_t mask = (1UL << pin);
    uint32_t odr  = port->ODR;
    port->BSRR    = ((odr & mask) << 16U) | (~odr & mask);
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
