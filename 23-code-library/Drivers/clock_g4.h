/**
 * @file    clock_g4.h
 * @brief   STM32G474RE Range 1 Boost Mode clock initialization (170 MHz & 160 MHz)
 *          and Cortex-M4 DWT->CYCCNT cycle-accurate microsecond delay driver.
 *
 * Verified against RM0440 Section 6.1.5 (Transition to > 150 MHz),
 * RM0440 Table 9 (Flash wait states), DS12288 Section 5.3.9..5.3.10,
 * UM2505 (NUCLEO-G474RE 24 MHz HSE crystal X3), and CMSIS stm32g474xx.h.
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
 * @brief Active STM32G474RE system clock profile tracked by clock_g4.
 */
typedef enum {
    CLOCK_PROFILE_HSI16      = 0U, /**< Reset default: 16 MHz HSI16                    */
    CLOCK_PROFILE_160MHZ_HSI = 1U, /**< 160 MHz Range 1 Boost from HSI16 (M=4,N=80,R=2) */
    CLOCK_PROFILE_170MHZ_HSI = 2U, /**< 170 MHz Range 1 Boost from HSI16 (M=4,N=85,R=2) */
    CLOCK_PROFILE_170MHZ_HSE = 3U  /**< 170 MHz Range 1 Boost from 24 MHz HSE X3       */
} clock_profile_t;

/**
 * @brief Configure STM32G474RE to 170 MHz SYSCLK/HCLK/PCLK1/PCLK2 from HSI16
 *        (16 MHz / M=4 * N=85 / R=2 = 170 MHz) using RM0440 Section 6.1.5
 *        Range 1 Boost Mode (R1MODE = 0), 4 Flash Wait States, and mandatory
 *        two-step HPRE=/2 -> >=1 us -> HPRE=/1 transition.
 *
 * Also configures PLL_P = 340/6 = 56.67 MHz (<= 60 MHz max ADC clock) and
 * PLL_Q = 340/8 = 42.5 MHz. Updates CMSIS SystemCoreClock to 170000000 Hz.
 *
 * @return true on success, false if an oscillator/PLL/PWR timeout occurs.
 */
bool Clock_Init_170MHz(void);

/**
 * @brief Configure STM32G474RE to 160 MHz SYSCLK/HCLK/PCLK1/PCLK2 from HSI16
 *        (16 MHz / M=4 * N=80 / R=2 = 160 MHz) using RM0440 Section 6.1.5
 *        Range 1 Boost Mode (R1MODE = 0, required for > 150 MHz), 4 Flash Wait
 *        States (RM0440 Table 9: > 136 MHz..170 MHz), and mandatory two-step
 *        HPRE=/2 (80 MHz) -> SW=PLL -> >=1 us -> HPRE=/1 (160 MHz) transition.
 *
 * Also configures PLL_P = 320/6 = 53.33 MHz (<= 60 MHz max ADC clock) and
 * PLL_Q = 320/8 = 40.0 MHz. Updates CMSIS SystemCoreClock to 160000000 Hz.
 *
 * @return true on success, false if an oscillator/PLL/PWR timeout occurs.
 */
bool Clock_Init_160MHz(void);

/**
 * @brief Configure STM32G474RE to 170 MHz SYSCLK/HCLK/PCLK1/PCLK2 from the
 *        NUCLEO-G474RE onboard 24 MHz HSE crystal X3 (UM2505)
 *        (24 MHz / M=6 * N=85 / R=2 = 170 MHz).
 *
 * @return true on success, false if HSE or PLL fails to stabilize within timeout.
 */
bool Clock_Init_170MHz_HSE24(void);

/**
 * @brief Return the current active SYSCLK/HCLK frequency in Hz (CMSIS SystemCoreClock).
 *
 * All peripheral drivers (USART, TIM, SPI, I2C) query this value dynamically so
 * baud rates and prescalers remain exact across 16 MHz, 160 MHz, and 170 MHz profiles.
 */
uint32_t Clock_GetSysClkHz(void);

/**
 * @brief Return the active clock profile configured via clock_g4.
 */
clock_profile_t Clock_GetProfile(void);

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
