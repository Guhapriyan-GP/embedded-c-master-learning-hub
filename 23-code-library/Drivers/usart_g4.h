/**
 * @file    usart_g4.h
 * @brief   Bare-metal CMSIS USART1 / USART2 / LPUART1 driver for STM32G474RE.
 *
 * Handles STM32G4-specific silicon details:
 *   - Dynamically computes BRR from live SystemCoreClock + RCC->CFGR PPREx
 *     so 16 MHz, 160 MHz, and 170 MHz profiles all yield exact baud rates.
 *   - Correctly distinguishes standard USART BRR (`f_ck / baud`) from LPUART1
 *     BRR (`256 * f_ck / baud`, valid range 0x300..0xFFFFF per RM0440 §40.5.4).
 *   - Bounded-timeout blocking TX prevents infinite hangs on misconfigured hardware.
 *   - Non-blocking RX ISR hook clears ORE/FE/NE/PE flags via ICR before draining
 *     RDR into a lock-free C11 SPSC `ring_buffer_t`.
 *
 * Reference: RM0440 Chapter 40 (USART / LPUART), UM2505 (USART2 PA2/PA3 STLINK VCP).
 * Concepts:  PERIPH-UART-001, PERIPH-UART-002, PERIPH-UART-004
 */

#ifndef USART_G4_H
#define USART_G4_H

#include "ring_buffer.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#if !defined(DRIVER_HOST_TEST)
#include "stm32g474xx.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

/** @brief USART driver status/error codes. */
typedef enum {
    USART_OK              = 0,  /**< Operation succeeded                             */
    USART_ERR_INVALID_ARG = -1, /**< NULL pointer or unsupported USART instance      */
    USART_ERR_BAUD_RANGE  = -2, /**< Computed BRR outside valid hardware limits      */
    USART_ERR_TIMEOUT     = -3  /**< Bounded polling wait expired before TXE / TC    */
} usart_status_t;

/** @brief Error bitmask returned by usart_clear_errors(). */
typedef enum {
    USART_ERRFLAG_NONE = 0U,
    USART_ERRFLAG_PE   = (1U << 0), /**< Parity error  */
    USART_ERRFLAG_FE   = (1U << 1), /**< Framing error */
    USART_ERRFLAG_NE   = (1U << 2), /**< Noise error   */
    USART_ERRFLAG_ORE  = (1U << 3)  /**< Overrun error */
} usart_errflags_t;

/**
 * @brief Pure host-testable helper: compute standard USART BRR register value
 *        for 16x oversampling (OVER8 = 0, RM0440 §40.5.4).
 *
 * Formula: BRR = round(pclk_hz / baud) = (pclk_hz + baud/2) / baud.
 *
 * @param pclk_hz  Peripheral kernel clock in Hz (e.g., 170000000 or 160000000).
 * @param baud     Desired baud rate in bps (e.g., 115200, 921600).
 * @return Valid 16-bit BRR in [0x10..0xFFFF], or 0 if invalid / out of range.
 */
uint32_t usart_calc_brr(uint32_t pclk_hz, uint32_t baud);

/**
 * @brief Pure host-testable helper: compute LPUART1 BRR register value (RM0440 §40.5.4).
 *
 * Formula: BRR = round((256 * pclk_hz) / baud), valid in [0x300..0xFFFFF].
 *
 * @param pclk_hz  LPUART1 kernel clock in Hz.
 * @param baud     Desired baud rate in bps.
 * @return Valid 20-bit LPUART BRR in [0x300..0xFFFFF], or 0 if out of range.
 */
uint32_t lpuart_calc_brr(uint32_t pclk_hz, uint32_t baud);

#if !defined(DRIVER_HOST_TEST)

/**
 * @brief Initialize USART1, USART2, or LPUART1 in 8N1 mode computing BRR dynamically
 *        from CMSIS SystemCoreClock + APB prescaler.
 *
 * @note  Caller must configure TX/RX GPIO pins to Alternate Function first
 *        (e.g., NUCLEO-G474RE VCP: PA2/PA3 AF7 for USART2, pull-up on RX).
 *
 * @param usart          USART1, USART2, or LPUART1.
 * @param baud           Desired baud rate (e.g., 115200U).
 * @param enable_rx_irq  If true, enables RXNEIE and the corresponding NVIC IRQ.
 * @param nvic_priority  Encoded NVIC priority (ignored when enable_rx_irq is false).
 * @return USART_OK or error status.
 */
usart_status_t usart_init(USART_TypeDef *usart, uint32_t baud,
                          bool enable_rx_irq, uint32_t nvic_priority);

/**
 * @brief Transmit a single byte with bounded TXE polling timeout.
 * @param usart          USART1, USART2, or LPUART1.
 * @param byte           Byte to transmit.
 * @param timeout_iters  Maximum polling loop iterations (must be > 0).
 * @return USART_OK, USART_ERR_INVALID_ARG, or USART_ERR_TIMEOUT.
 */
usart_status_t usart_write_byte(USART_TypeDef *usart, uint8_t byte,
                                uint32_t timeout_iters);

/**
 * @brief Transmit a raw byte buffer with bounded per-byte timeout.
 */
usart_status_t usart_write_buf(USART_TypeDef *usart, const uint8_t *data,
                               size_t len, uint32_t timeout_iters);

/**
 * @brief Transmit a NUL-terminated C string with bounded per-byte timeout.
 */
usart_status_t usart_write_str(USART_TypeDef *usart, const char *str,
                               uint32_t timeout_iters);

/**
 * @brief Wait (bounded) until the final stop bit has shifted out (USART_ISR_TC).
 */
usart_status_t usart_flush_tx(const USART_TypeDef *usart, uint32_t timeout_iters);

/**
 * @brief Check and clear hardware receive error flags (ORE, FE, NE, PE) via ICR.
 * @return Bitmask of usart_errflags_t that were set prior to clearing.
 */
uint32_t usart_clear_errors(USART_TypeDef *usart);

/**
 * @brief Non-blocking RX ISR hook: clears ORE/FE/NE/PE flags and drains available
 *        bytes from RDR into the lock-free SPSC ring buffer @p rx_rb.
 *
 * Call this directly from USART1_IRQHandler / USART2_IRQHandler / LPUART1_IRQHandler.
 *
 * @param usart  USART1, USART2, or LPUART1.
 * @param rx_rb  Initialized ring_buffer_t (if NULL, RDR is still drained to clear RXNE).
 */
void usart_rx_irq_handler(USART_TypeDef *usart, ring_buffer_t *rx_rb);

#endif /* !defined(DRIVER_HOST_TEST) */

#ifdef __cplusplus
}
#endif

#endif /* USART_G4_H */
