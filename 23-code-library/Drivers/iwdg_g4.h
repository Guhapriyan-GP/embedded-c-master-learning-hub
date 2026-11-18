/**
 * @file    iwdg_g4.h
 * @brief   Bare-metal CMSIS Independent Watchdog (IWDG) driver for STM32G474RE.
 *
 * Handles STM32G4-specific silicon details:
 *   - Computes optimal prescaler (`PR = 0..6`, /4../256) and 12-bit reload
 *     (`RLR = 0..4095`) from desired timeout in milliseconds via a pure
 *     host-testable helper.
 *   - Supports `DBGMCU->APB1FZR1 |= DBGMCU_APB1FZR1_DBG_IWDG_STOP` so GDB
 *     breakpoints do not trigger unwanted watchdog resets during debugging.
 *   - Uses bounded polling waits on `IWDG->SR` (`PVU`, `RVU`) and `RCC->CSR` (`LSIRDY`).
 *   - Provides reset-cause inspection (`RCC->CSR.IWDGRSTF`) and cleanup (`RMVF`).
 *
 * Reference: RM0440 Chapter 35 (IWDG), Chapter 7 (RCC->CSR), Chapter 47 (DBGMCU).
 * Concepts:  PERIPH-WDG-001, STM32-RESET-001, STM32-DBGMCU-001
 */

#ifndef IWDG_G4_H
#define IWDG_G4_H

#include <stdbool.h>
#include <stdint.h>

#if !defined(DRIVER_HOST_TEST)
#include "stm32g474xx.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

/** @brief IWDG hardware key register magic values (RM0440 §35.4.1). */
#define IWDG_KEY_RELOAD       0xAAAAU /**< Reload counter from RLR (kick/feed)      */
#define IWDG_KEY_ENABLE       0xCCCCU /**< Start IWDG (irreversible until reset!)   */
#define IWDG_KEY_WRITE_ACCESS 0x5555U /**< Unlock writes to PR, RLR, and WINR       */

/** @brief Nominal STM32G474RE LSI oscillator frequency (DS12288: 32 kHz +/- 5%). */
#define IWDG_LSI_NOMINAL_HZ   32000U

/** @brief IWDG driver status/error codes. */
typedef enum {
    IWDG_OK              = 0,  /**< Operation succeeded                             */
    IWDG_ERR_INVALID_ARG = -1, /**< Timeout out of range (0 ms or > 32768 ms @ 32k) */
    IWDG_ERR_TIMEOUT     = -2  /**< Bounded wait on LSIRDY or IWDG->SR sync expired */
} iwdg_status_t;

/**
 * @brief Pure host-testable helper: compute IWDG prescaler code `PR` (0..6 -> /4../256)
 *        and 12-bit reload register `RLR` (0..4095) for desired @p timeout_ms.
 *
 * Formula: T_timeout = ((RLR + 1) * (4 << PR)) / lsi_hz
 *
 * Selects the smallest prescaler `PR` where the tick count fits in [1..4096]
 * to achieve the finest possible timeout resolution.
 *
 * @param timeout_ms  Desired watchdog timeout in milliseconds (>= 1).
 * @param lsi_hz      LSI clock frequency in Hz (typically 32000).
 * @param out_pr_reg  Output PR register value (0..6: /4, /8, /16, /32, /64, /128, /256).
 * @param out_rlr     Output 12-bit RLR register value (0..4095).
 * @return true on success, false if arguments are invalid or timeout exceeds max.
 */
bool iwdg_calc_prescaler_reload(uint32_t timeout_ms, uint32_t lsi_hz,
                                uint8_t *out_pr_reg, uint16_t *out_rlr);

#if !defined(DRIVER_HOST_TEST)

/**
 * @brief Start and configure the STM32G474RE Independent Watchdog for @p timeout_ms.
 *
 * @warning Once started (`IWDG_KEY_ENABLE`), IWDG CANNOT be stopped by software
 *          until a hardware reset! Call `iwdg_kick()` periodically well before
 *          @p timeout_ms (account for LSI +/-5% tolerance!).
 *
 * @param timeout_ms       Desired timeout in ms (1 .. 32768 ms at 32 kHz LSI).
 * @param freeze_in_debug  If true, sets `DBGMCU->APB1FZR1.DBG_IWDG_STOP` so IWDG
 *                         pauses when the Cortex-M4 core is halted in GDB.
 * @param timeout_iters    Bounded iteration limit for LSI ready and `IWDG->SR` sync.
 * @return IWDG_OK, IWDG_ERR_INVALID_ARG, or IWDG_ERR_TIMEOUT.
 */
iwdg_status_t iwdg_init_ms(uint32_t timeout_ms, bool freeze_in_debug,
                           uint32_t timeout_iters);

/**
 * @brief Feed / kick the watchdog by writing 0xAAAA to `IWDG->KR`.
 */
static inline void iwdg_kick(void) {
    IWDG->KR = IWDG_KEY_RELOAD;
}

/**
 * @brief Return true if the previous MCU reset was caused by IWDG (`RCC->CSR.IWDGRSTF`).
 */
bool iwdg_was_reset_cause(void);

/**
 * @brief Clear all latched reset-cause flags in `RCC->CSR` via `RCC_CSR_RMVF`.
 */
void iwdg_clear_reset_flags(void);

#endif /* !defined(DRIVER_HOST_TEST) */

#ifdef __cplusplus
}
#endif

#endif /* IWDG_G4_H */
