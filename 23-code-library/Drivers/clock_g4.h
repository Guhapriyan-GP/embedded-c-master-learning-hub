/**
 * @file    clock_g4.h
 * @brief   STM32G474RE 170 MHz Range 1 Boost Mode clock initialization and
 *          Cortex-M4 DWT->CYCCNT cycle-accurate microsecond delay driver.
 *
 * Verified against RM0440 Section 6.1.5 (Transition to > 150 MHz),
 * RM0440 Table 9 (Flash wait states), DS12288 Section 5.3.9..5.3.10,
 * and CMSIS stm32g474xx.h.
 *
 * Concepts: STM32-RCC-001, STM32-PWR-001, STM32-FLASH-001, TOOL-DBG-003
 */

#ifndef CLOCK_G4_H
#define CLOCK_G4_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Configure STM32G474RE to 170 MHz SYSCLK/HCLK/PCLK1/PCLK2 from HSI16
 *        (16 MHz / M=4 * N=85 / R=2 = 170 MHz) using RM0440 Section 6.1.5
 *        Range 1 Boost Mode and mandatory two-step HPRE=/2 -> >=1 us -> HPRE=/1.
 *
 * Also configures PLL_P = 340/6 = 56.67 MHz (<= 60 MHz max ADC clock) and
 * PLL_Q = 340/8 = 42.5 MHz. Updates CMSIS SystemCoreClock to 170000000 Hz.
 *
 * @return true on success, false if an oscillator/PLL timeout occurs.
 */
bool Clock_Init_170MHz(void);

/**
 * @brief Configure STM32G474RE to 170 MHz SYSCLK/HCLK/PCLK1/PCLK2 from the
 *        NUCLEO-G474RE onboard 24 MHz HSE crystal X3 (UM2505)
 *        (24 MHz / M=6 * N=85 / R=2 = 170 MHz).
 *
 * @return true on success, false if HSE or PLL fails to stabilize within timeout.
 */
bool Clock_Init_170MHz_HSE24(void);

/**
 * @brief Enable the ARM Cortex-M4 DWT->CYCCNT 32-bit cycle counter for
 *        deterministic cycle timing and microsecond delays.
 */
void Clock_DWT_Init(void);

/**
 * @brief Return current 32-bit CPU cycle count (wraps every ~25.26 s at 170 MHz).
 */
uint32_t Clock_GetCycles(void);

/**
 * @brief Blocking cycle-accurate microsecond delay using DWT->CYCCNT.
 * @note  Automatically enables DWT->CYCCNT via Clock_DWT_Init() if not yet active,
 *        and chunks delays > 1,000,000 us to prevent 32-bit cycle-counter wrap overflow.
 */
void Clock_DelayUs(uint32_t us);

/**
 * @brief Blocking millisecond delay built on Clock_DelayUs(1000U).
 */
void Clock_DelayMs(uint32_t ms);

#ifdef __cplusplus
}
#endif

#endif /* CLOCK_G4_H */
