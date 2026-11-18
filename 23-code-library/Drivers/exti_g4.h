/**
 * @file    exti_g4.h
 * @brief   Bare-metal CMSIS EXTI & SYSCFG GPIO interrupt driver for STM32G474RE.
 *
 * Handles STM32G4-specific silicon details:
 *   - Enables RCC->APB2ENR.SYSCFGEN (+ readback barrier) before writing SYSCFG->EXTICR.
 *   - Clears EXTI->PR1 (rc_w1) using direct assignment `=` (NEVER `|=`) followed by
 *     dummy readback `(void)EXTI->PR1;` (ARM AN321 §4.9: AHB-to-APB2 posted-write commit)
 *     and Cortex-M4 `__DSB()` (write-buffer drain + ARM Erratum 838869 / ST ES0430 §2.1.3).
 *   - Supports multi-pin shared IRQ dispatch (EXTI9_5_IRQn, EXTI15_10_IRQn) via callbacks.
 *
 * Reference: RM0440 Chapter 10 (SYSCFG), Chapter 15 (EXTI), PM0214 (NVIC), ARM AN321 §4.9, ES0430 §2.1.3.
 * Concepts:  PERIPH-EXTI-001, PERIPH-EXTI-002, PERIPH-EXTI-003, PERIPH-SYSCFG-001, ARM-NVIC-001
 */

#ifndef EXTI_G4_H
#define EXTI_G4_H

#include <stdbool.h>
#include <stdint.h>

#if !defined(DRIVER_HOST_TEST)
#include "stm32g474xx.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

/** @brief EXTI driver status/error codes. */
typedef enum {
    EXTI_OK              = 0,  /**< Operation succeeded                             */
    EXTI_ERR_INVALID_ARG = -1  /**< Invalid GPIO port, pin (>15), or NULL parameter */
} exti_status_t;

/** @brief EXTI hardware edge trigger selection (RTSR1 / FTSR1). */
typedef enum {
    EXTI_EDGE_NONE    = 0U, /**< Neither rising nor falling edge                 */
    EXTI_EDGE_RISING  = 1U, /**< Rising edge trigger (e.g. NUCLEO PC13 Active-HIGH press) */
    EXTI_EDGE_FALLING = 2U, /**< Falling edge trigger                            */
    EXTI_EDGE_BOTH    = 3U  /**< Both rising and falling edges                   */
} exti_edge_t;

/**
 * @brief User callback invoked from EXTI_DispatchRange() after PR1 is cleared.
 * @param line  EXTI line index 0..15 that triggered.
 * @param ctx   Opaque caller context pointer supplied at registration.
 */
typedef void (*exti_callback_t)(uint8_t line, void *ctx);

/**
 * @brief Pure host-testable helper: compute SYSCFG->EXTICR register index (0..3)
 *        and 4-bit nibble shift (0, 4, 8, 12) for GPIO EXTI line @p pin (0..15).
 *
 * @param pin            GPIO pin / EXTI line number (0..15).
 * @param out_reg_idx    Output EXTICR array index (0..3).
 * @param out_bit_shift  Output bit shift within EXTICR[index] (0, 4, 8, or 12).
 * @return true on success, false if pin > 15 or output pointers are NULL.
 */
bool exti_calc_exticr_index_shift(uint8_t pin, uint8_t *out_reg_idx, uint8_t *out_bit_shift);

#if !defined(DRIVER_HOST_TEST)

/**
 * @brief Route GPIO @p port pin @p pin to EXTI line @p pin, configure edge triggers,
 *        unmask EXTI->IMR1, and enable the corresponding NVIC IRQ line.
 *
 * @note  Caller must configure the GPIO pin as Digital Input (MODER = 0b00) prior
 *        to enabling EXTI — STM32G4 resets GPIO pins to Analog (0b11)!
 *
 * @param port           GPIOA..GPIOG.
 * @param pin            Pin / EXTI line number 0..15.
 * @param edge           Rising, falling, both, or none.
 * @param nvic_priority  Encoded NVIC priority value passed to NVIC_SetPriority().
 * @return EXTI_OK on success, EXTI_ERR_INVALID_ARG on bad port/pin.
 */
exti_status_t EXTI_ConfigLine(const GPIO_TypeDef *port, uint8_t pin,
                              exti_edge_t edge, uint32_t nvic_priority);

/**
 * @brief Mask EXTI line @p pin in EXTI->IMR1 and clear any pending trigger flag.
 * @param pin  EXTI line number 0..15.
 * @return EXTI_OK or EXTI_ERR_INVALID_ARG.
 */
exti_status_t EXTI_DisableLine(uint8_t pin);

/**
 * @brief Register (or clear with cb=NULL) a callback for EXTI line @p pin (0..15).
 * @param pin  EXTI line number 0..15.
 * @param cb   Callback function pointer (or NULL to unregister).
 * @param ctx  Optional user context passed to callback.
 * @return EXTI_OK or EXTI_ERR_INVALID_ARG.
 */
exti_status_t EXTI_RegisterCallback(uint8_t pin, exti_callback_t cb, void *ctx);

/**
 * @brief Trigger an EXTI line in software via EXTI->SWIER1 (useful for self-tests).
 * @param pin  EXTI line number 0..15.
 */
void EXTI_TriggerSoftware(uint8_t pin);

/**
 * @brief Clear EXTI pending flag in EXTI->PR1 (rc_w1 direct assignment, NEVER `|=`),
 *        read back EXTI->PR1 to flush the AHB-to-APB2 posted-write bridge, and issue
 *        Cortex-M4 `__DSB()` (drains CPU write buffer & guards ARM Erratum 838869 / ES0430 §2.1.3).
 * @param pin  EXTI line number 0..15.
 */
static inline void EXTI_ClearPending(uint8_t pin) {
    if (pin <= 15U) {
        EXTI->PR1 = (1UL << pin);
        (void)EXTI->PR1;
        __DSB();
    }
}

/**
 * @brief Dispatch all unmasked pending EXTI lines in [min_line..max_line] (0..15).
 *
 * Intended to be called directly from EXTI0_IRQHandler..EXTI15_10_IRQHandler:
 *   - Reads `pending = EXTI->PR1 & EXTI->IMR1` so masked lines are never disturbed.
 *   - Clears each pending line via EXTI_ClearPending() before invoking its callback.
 *
 * @param min_line  Lowest line in the IRQ range (e.g., 10 for EXTI15_10_IRQHandler).
 * @param max_line  Highest line in the IRQ range (e.g., 15 for EXTI15_10_IRQHandler).
 */
void EXTI_DispatchRange(uint8_t min_line, uint8_t max_line);

#endif /* !defined(DRIVER_HOST_TEST) */

#ifdef __cplusplus
}
#endif

#endif /* EXTI_G4_H */
