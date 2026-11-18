/**
 * @file    tim_g4.h
 * @brief   Bare-metal CMSIS General-Purpose & Basic Timer driver (TIM2/3/6/7)
 *          for periodic interrupts and glitch-free hardware PWM on STM32G474RE.
 *
 * Handles STM32G4-specific silicon details:
 *   - Dynamically derives timer kernel clock from SystemCoreClock + RCC->CFGR PPREx
 *     (including STM32 x2 timer clock multiplier when APB prescaler > /1).
 *   - Forces shadow-register preload via `TIMx->EGR = TIM_EGR_UG` and clears stale
 *     UIF before enabling UIE/CEN so the first period never glitches with PSC=0.
 *   - Clears rc_w0 status flags using direct write `TIMx->SR = ~TIM_SR_UIF` (NEVER `&= ~`),
 *     followed by dummy readback `(void)TIMx->SR;` (ARM AN321 §4.9: APB posted-write commit)
 *     and `__DSB()` (write-buffer drain + ARM Erratum 838869 / ST ES0430 §2.1.3).
 *   - Enables ARPE + OCxPE for glitch-free runtime PWM duty-cycle updates.
 *
 * Reference: RM0440 Ch. 29/30 (GP Timers TIM2-5), Ch. 31/33 (Basic Timers TIM6-7), ARM AN321 §4.9, ES0430 §2.1.3.
 * Concepts:  PERIPH-TIM-001, PERIPH-TIM-002, PERIPH-PWM-001
 */

#ifndef TIM_G4_H
#define TIM_G4_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#if !defined(DRIVER_HOST_TEST)
#include "stm32g474xx.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Timer driver status/error codes. */
typedef enum {
    TIM_OK                    = 0,  /**< Operation succeeded                          */
    TIM_ERR_INVALID_ARG       = -1, /**< NULL pointer, bad channel (not 1..4), freq=0 */
    TIM_ERR_UNSUPPORTED_TIM   = -2, /**< Timer instance not supported for this op     */
    TIM_ERR_FREQ_OUT_OF_RANGE = -3  /**< Target frequency unreachable in 16-bit PSC/ARR */
} tim_status_t;

/**
 * @brief Pure host-testable helper: calculate optimal 16-bit prescaler (PSC) and
 *        16-bit-compatible auto-reload (ARR) register values for @p target_hz.
 *
 * Maximizes timer resolution (smallest valid PSC so ARR is as large as possible
 * while staying within [0..65535], compatible with both 16-bit TIM3/6/7 and 32-bit TIM2).
 *
 * Formula: f_update = clk_hz / ((PSC + 1) * (ARR + 1))
 *
 * @param clk_hz     Timer kernel clock frequency in Hz (e.g. 170000000 or 160000000).
 * @param target_hz  Desired update / PWM frequency in Hz (1 .. clk_hz).
 * @param out_psc    Output PSC register value (0..65535).
 * @param out_arr    Output ARR register value (0..65535).
 * @return true on success, false if arguments are invalid or frequency is out of range.
 */
bool tim_calc_psc_arr(uint32_t clk_hz, uint32_t target_hz,
                      uint16_t *out_psc, uint32_t *out_arr);

/**
 * @brief Pure host-testable helper: compute PWM Mode 1 compare value (CCR) from
 *        ARR and duty cycle in permille (0..1000 = 0.0%..100.0%).
 *
 * @note  Clamps @p duty_permille > 1000 to 1000. For 100.0% duty, returns `arr + 1`
 *        (true solid HIGH in PWM Mode 1), except when `arr == 0xFFFFU` where it
 *        clamps to `0xFFFFU` to prevent 16-bit CCR register wraparound to 0%!
 *
 * @param arr            Timer auto-reload register value.
 * @param duty_permille  Duty cycle in tenths of a percent (0..1000).
 * @return Calculated CCR register value.
 */
uint32_t tim_calc_pwm_ccr(uint32_t arr, uint16_t duty_permille);

#if !defined(DRIVER_HOST_TEST)

/**
 * @brief Compute active timer kernel clock frequency in Hz from CMSIS SystemCoreClock
 *        and RCC->CFGR APB prescaler (applying the STM32 x2 rule when PPREx > /1).
 * @param tim  Supported timer instance (TIM2, TIM3, TIM6, TIM7).
 * @return Timer clock frequency in Hz, or 0 if @p tim is unsupported.
 */
uint32_t TIM_GetKernelClockHz(const TIM_TypeDef *tim);

/**
 * @brief Configure TIM2, TIM3, TIM6, or TIM7 as a periodic update timer at @p freq_hz.
 *
 * Performs: RCC enable + readback -> PSC/ARR computation -> ARPE -> `EGR = UG`
 * shadow load -> `SR = ~UIF` clear -> optional UIE + NVIC enable -> `CR1 |= CEN`.
 *
 * @param tim            TIM2, TIM3, TIM6, or TIM7.
 * @param freq_hz        Desired update frequency in Hz.
 * @param enable_irq     If true, enables TIM_DIER_UIE and the matching NVIC IRQ.
 * @param nvic_priority  Encoded NVIC priority value (ignored if enable_irq is false).
 * @return TIM_OK or error status.
 */
tim_status_t TIM_InitPeriodic(TIM_TypeDef *tim, uint32_t freq_hz,
                              bool enable_irq, uint32_t nvic_priority);

/**
 * @brief Configure TIM2 or TIM3 channel @p channel (1..4) for edge-aligned PWM Mode 1
 *        with ARPE + OCxPE preload enabled for glitch-free duty updates.
 *
 * @note  Caller must configure the corresponding GPIO pin to the timer's Alternate
 *        Function (e.g., PA6 AF2 = TIM3_CH1, PA5 AF1 = TIM2_CH1) via GPIO_ConfigAF().
 *
 * @param tim                    TIM2 or TIM3 (TIM6/TIM7 have no PWM channels).
 * @param channel                PWM channel 1..4.
 * @param freq_hz                Desired PWM carrier frequency in Hz.
 * @param initial_duty_permille  Initial duty cycle 0..1000 (0.0%..100.0%).
 * @return TIM_OK or error status.
 */
tim_status_t TIM_InitPwm(TIM_TypeDef *tim, uint8_t channel, uint32_t freq_hz,
                         uint16_t initial_duty_permille);

/**
 * @brief Glitch-free runtime PWM duty update via preloaded CCRx register.
 * @param tim            TIM2 or TIM3.
 * @param channel        Channel 1..4.
 * @param duty_permille  Duty cycle 0..1000 (0.0%..100.0%).
 * @return TIM_OK or error status.
 */
tim_status_t TIM_SetPwmDutyPermille(TIM_TypeDef *tim, uint8_t channel,
                                    uint16_t duty_permille);

/**
 * @brief Direct raw compare register write (CCRx) for channels 1..4.
 * @param tim        TIM2 or TIM3.
 * @param channel    Channel 1..4.
 * @param ccr_value  Raw CCR value.
 * @return TIM_OK or error status.
 */
tim_status_t TIM_SetPwmCompare(TIM_TypeDef *tim, uint8_t channel, uint32_t ccr_value);

/**
 * @brief Clear TIM update interrupt flag (UIF) safely on STM32 rc_w0 SR register
 *        (direct write `~TIM_SR_UIF`, NEVER RMW `&=`), read back SR to flush the
 *        AHB-to-APB1 posted-write bridge, and issue Cortex-M4 `__DSB()`.
 */
static inline void TIM_ClearUpdateFlag(TIM_TypeDef *tim) {
    if (tim != NULL) {
        tim->SR = ~TIM_SR_UIF;
        (void)tim->SR;
        __DSB();
    }
}

#endif /* !defined(DRIVER_HOST_TEST) */

#ifdef __cplusplus
}
#endif

#endif /* TIM_G4_H */
