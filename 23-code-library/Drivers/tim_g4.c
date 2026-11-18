/**
 * @file    tim_g4.c
 * @brief   Bare-metal CMSIS Timer & PWM driver implementation for STM32G474RE.
 */

#include "tim_g4.h"
#include <stddef.h>

bool tim_calc_psc_arr(uint32_t clk_hz, uint32_t target_hz,
                      uint16_t *out_psc, uint32_t *out_arr) {
    if ((clk_hz == 0U) || (target_hz == 0U) || (target_hz > clk_hz) ||
        (out_psc == NULL) || (out_arr == NULL)) {
        return false;
    }

    /* Rounded total timer clock ticks per update period */
    uint64_t total_ticks = ((uint64_t)clk_hz + ((uint64_t)target_hz / 2ULL)) / (uint64_t)target_hz;
    if (total_ticks == 0ULL) {
        total_ticks = 1ULL;
    }

    /* Smallest prescaler divider so arr_ticks fits within 16-bit ARR + 1 (65536) */
    uint64_t psc_div = (total_ticks + 65535ULL) / 65536ULL;
    if (psc_div == 0ULL) {
        psc_div = 1ULL;
    }
    if (psc_div > 65536ULL) {
        return false;
    }

    uint64_t arr_ticks = (total_ticks + (psc_div / 2ULL)) / psc_div;
    if (arr_ticks == 0ULL) {
        arr_ticks = 1ULL;
    }
    if (arr_ticks > 65536ULL) {
        if (psc_div >= 65536ULL) {
            return false;
        }
        psc_div++;
        arr_ticks = (total_ticks + (psc_div / 2ULL)) / psc_div;
        if (arr_ticks > 65536ULL) {
            arr_ticks = 65536ULL;
        }
    }

    *out_psc = (uint16_t)(psc_div - 1ULL);
    *out_arr = (uint32_t)(arr_ticks - 1ULL);
    return true;
}

uint32_t tim_calc_pwm_ccr(uint32_t arr, uint16_t duty_permille) {
    if (duty_permille == 0U) {
        return 0U;
    }
    if (duty_permille >= 1000U) {
        /* Guard both 16-bit (TIM3) and 32-bit (TIM2) CCR wraparound at max ARR */
        if ((arr == 0xFFFFU) || (arr == 0xFFFFFFFFU)) {
            return arr;
        }
        return arr + 1U;
    }

    uint64_t period = (uint64_t)arr + 1ULL;
    uint64_t ccr    = (period * (uint64_t)duty_permille + 500ULL) / 1000ULL;
    if ((arr == 0xFFFFU) && (ccr > 0xFFFFULL)) {
        ccr = 0xFFFFULL;
    }
    return (uint32_t)ccr;
}

#if !defined(DRIVER_HOST_TEST)

static bool tim_is_supported(const TIM_TypeDef *tim) {
    return (tim == TIM2) || (tim == TIM3) || (tim == TIM6) || (tim == TIM7);
}

static bool tim_enable_clock(const TIM_TypeDef *tim) {
    uint32_t bit = 0U;
    if (tim == TIM2) {
        bit = RCC_APB1ENR1_TIM2EN;
    } else if (tim == TIM3) {
        bit = RCC_APB1ENR1_TIM3EN;
    } else if (tim == TIM6) {
        bit = RCC_APB1ENR1_TIM6EN;
    } else if (tim == TIM7) {
        bit = RCC_APB1ENR1_TIM7EN;
    } else {
        return false;
    }

    RCC->APB1ENR1 |= bit;
    (void)RCC->APB1ENR1;
    return true;
}

static IRQn_Type tim_to_irqn(const TIM_TypeDef *tim) {
    if (tim == TIM2) {
        return TIM2_IRQn;
    }
    if (tim == TIM3) {
        return TIM3_IRQn;
    }
    if (tim == TIM6) {
        return TIM6_DAC_IRQn;
    }
    return TIM7_DAC_IRQn;
}

uint32_t TIM_GetKernelClockHz(const TIM_TypeDef *tim) {
    if (!tim_is_supported(tim)) {
        return 0U;
    }

    /* TIM2, TIM3, TIM6, TIM7 all reside on APB1.
     * RM0440 §7.2.8: If APB1 prescaler == /1, TIMCLK = PCLK1 = HCLK.
     *                If APB1 prescaler  > /1, TIMCLK = 2 * PCLK1. */
    uint32_t ppre1 = (RCC->CFGR & RCC_CFGR_PPRE1) >> RCC_CFGR_PPRE1_Pos;
    if (ppre1 < 4U) {
        return SystemCoreClock;
    }
    uint32_t apb1_shift = APBPrescTable[ppre1];
    uint32_t pclk1      = SystemCoreClock >> apb1_shift;
    return pclk1 * 2U;
}

tim_status_t TIM_InitPeriodic(TIM_TypeDef *tim, uint32_t freq_hz,
                              bool enable_irq, uint32_t nvic_priority) {
    if ((tim == NULL) || (freq_hz == 0U)) {
        return TIM_ERR_INVALID_ARG;
    }
    if (!tim_is_supported(tim)) {
        return TIM_ERR_UNSUPPORTED_TIM;
    }

    uint32_t tim_clk = TIM_GetKernelClockHz(tim);
    uint16_t psc     = 0U;
    uint32_t arr     = 0U;
    if (!tim_calc_psc_arr(tim_clk, freq_hz, &psc, &arr)) {
        return TIM_ERR_FREQ_OUT_OF_RANGE;
    }

    (void)tim_enable_clock(tim);

    /* Stop counter during configuration */
    tim->CR1 &= ~TIM_CR1_CEN;
    tim->DIER = 0U;

    tim->PSC = psc;
    tim->ARR = arr;
    tim->CNT = 0U;

    /* Enable ARR shadow preload (ARPE), force shadow transfer via UG, clear stale UIF */
    tim->CR1 |= TIM_CR1_ARPE;
    tim->EGR  = TIM_EGR_UG;
    tim->SR   = ~TIM_SR_UIF;

    if (enable_irq) {
        tim->DIER |= TIM_DIER_UIE;
        IRQn_Type irqn = tim_to_irqn(tim);
        NVIC_SetPriority(irqn, nvic_priority);
        NVIC_ClearPendingIRQ(irqn);
        NVIC_EnableIRQ(irqn);
    }

    tim->CR1 |= TIM_CR1_CEN;
    return TIM_OK;
}

tim_status_t TIM_SetPwmCompare(TIM_TypeDef *tim, uint8_t channel, uint32_t ccr_value) {
    if ((tim == NULL) || (channel < 1U) || (channel > 4U)) {
        return TIM_ERR_INVALID_ARG;
    }
    if ((tim != TIM2) && (tim != TIM3)) {
        return TIM_ERR_UNSUPPORTED_TIM;
    }

    switch (channel) {
        case 1U: tim->CCR1 = ccr_value; break;
        case 2U: tim->CCR2 = ccr_value; break;
        case 3U: tim->CCR3 = ccr_value; break;
        case 4U: tim->CCR4 = ccr_value; break;
        default: return TIM_ERR_INVALID_ARG;
    }
    return TIM_OK;
}

tim_status_t TIM_SetPwmDutyPermille(TIM_TypeDef *tim, uint8_t channel,
                                    uint16_t duty_permille) {
    if ((tim == NULL) || (channel < 1U) || (channel > 4U)) {
        return TIM_ERR_INVALID_ARG;
    }
    if ((tim != TIM2) && (tim != TIM3)) {
        return TIM_ERR_UNSUPPORTED_TIM;
    }

    uint32_t ccr = tim_calc_pwm_ccr(tim->ARR, duty_permille);
    return TIM_SetPwmCompare(tim, channel, ccr);
}

tim_status_t TIM_InitPwm(TIM_TypeDef *tim, uint8_t channel, uint32_t freq_hz,
                         uint16_t initial_duty_permille) {
    if ((tim == NULL) || (channel < 1U) || (channel > 4U) || (freq_hz == 0U)) {
        return TIM_ERR_INVALID_ARG;
    }
    if ((tim != TIM2) && (tim != TIM3)) {
        return TIM_ERR_UNSUPPORTED_TIM;
    }

    uint32_t tim_clk = TIM_GetKernelClockHz(tim);
    uint16_t psc     = 0U;
    uint32_t arr     = 0U;
    if (!tim_calc_psc_arr(tim_clk, freq_hz, &psc, &arr)) {
        return TIM_ERR_FREQ_OUT_OF_RANGE;
    }

    (void)tim_enable_clock(tim);

    tim->CR1 &= ~TIM_CR1_CEN;
    tim->PSC  = psc;
    tim->ARR  = arr;
    tim->CNT  = 0U;

    /* Configure PWM Mode 1 (OCxM = 0b0110) + Preload Enable (OCxPE = 1) */
    switch (channel) {
        case 1U:
            tim->CCMR1 = (tim->CCMR1 & ~(TIM_CCMR1_CC1S | TIM_CCMR1_OC1M)) |
                         (6UL << TIM_CCMR1_OC1M_Pos) | TIM_CCMR1_OC1PE;
            tim->CCER  = (tim->CCER & ~TIM_CCER_CC1P) | TIM_CCER_CC1E;
            break;
        case 2U:
            tim->CCMR1 = (tim->CCMR1 & ~(TIM_CCMR1_CC2S | TIM_CCMR1_OC2M)) |
                         (6UL << TIM_CCMR1_OC2M_Pos) | TIM_CCMR1_OC2PE;
            tim->CCER  = (tim->CCER & ~TIM_CCER_CC2P) | TIM_CCER_CC2E;
            break;
        case 3U:
            tim->CCMR2 = (tim->CCMR2 & ~(TIM_CCMR2_CC3S | TIM_CCMR2_OC3M)) |
                         (6UL << TIM_CCMR2_OC3M_Pos) | TIM_CCMR2_OC3PE;
            tim->CCER  = (tim->CCER & ~TIM_CCER_CC3P) | TIM_CCER_CC3E;
            break;
        case 4U:
            tim->CCMR2 = (tim->CCMR2 & ~(TIM_CCMR2_CC4S | TIM_CCMR2_OC4M)) |
                         (6UL << TIM_CCMR2_OC4M_Pos) | TIM_CCMR2_OC4PE;
            tim->CCER  = (tim->CCER & ~TIM_CCER_CC4P) | TIM_CCER_CC4E;
            break;
        default:
            return TIM_ERR_INVALID_ARG;
    }

    uint32_t ccr = tim_calc_pwm_ccr(arr, initial_duty_permille);
    (void)TIM_SetPwmCompare(tim, channel, ccr);

    /* Enable ARPE, latch PSC/ARR/CCRx shadows via UG, clear flags, start timer */
    tim->CR1 |= TIM_CR1_ARPE;
    tim->EGR  = TIM_EGR_UG;
    tim->SR   = 0U;
    tim->CR1 |= TIM_CR1_CEN;

    return TIM_OK;
}

#endif /* !defined(DRIVER_HOST_TEST) */
