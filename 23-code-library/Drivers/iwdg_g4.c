/**
 * @file    iwdg_g4.c
 * @brief   Bare-metal CMSIS Independent Watchdog (IWDG) driver implementation.
 */

#include "iwdg_g4.h"
#include <stddef.h>

bool iwdg_calc_prescaler_reload(uint32_t timeout_ms, uint32_t lsi_hz,
                                uint8_t *out_pr_reg, uint16_t *out_rlr) {
    if ((timeout_ms == 0U) || (lsi_hz == 0U) ||
        (out_pr_reg == NULL) || (out_rlr == NULL)) {
        return false;
    }

    for (uint8_t pr = 0U; pr <= 6U; ++pr) {
        uint64_t div   = 4ULL << pr;
        uint64_t denom = div * 1000ULL;
        uint64_t ticks = ((uint64_t)timeout_ms * (uint64_t)lsi_hz + (denom / 2ULL)) / denom;

        if (ticks == 0ULL) {
            ticks = 1ULL;
        }
        if (ticks <= 4096ULL) {
            *out_pr_reg = pr;
            *out_rlr    = (uint16_t)(ticks - 1ULL);
            return true;
        }
    }

    return false;
}

#if !defined(DRIVER_HOST_TEST)

iwdg_status_t iwdg_init_ms(uint32_t timeout_ms, bool freeze_in_debug,
                           uint32_t timeout_iters) {
    uint8_t  pr  = 0U;
    uint16_t rlr = 0U;

    if ((timeout_iters == 0U) ||
        !iwdg_calc_prescaler_reload(timeout_ms, IWDG_LSI_NOMINAL_HZ, &pr, &rlr)) {
        return IWDG_ERR_INVALID_ARG;
    }

    /* 1. Optional debug freeze so GDB breakpoints do not trigger watchdog resets */
    if (freeze_in_debug) {
        DBGMCU->APB1FZR1 |= DBGMCU_APB1FZR1_DBG_IWDG_STOP;
    }

    /* 2. Ensure LSI (32 kHz) is running and stable */
    RCC->CSR |= RCC_CSR_LSION;
    uint32_t rem = timeout_iters;
    while ((RCC->CSR & RCC_CSR_LSIRDY) == 0U) {
        if (--rem == 0U) {
            return IWDG_ERR_TIMEOUT;
        }
    }

    /* 3. Enable IWDG and unlock PR / RLR write access */
    IWDG->KR = IWDG_KEY_ENABLE;
    IWDG->KR = IWDG_KEY_WRITE_ACCESS;

    /* 4. Wait bounded until any prior PR/RLR update finishes syncing to LSI domain */
    rem = timeout_iters;
    while ((IWDG->SR & (IWDG_SR_PVU | IWDG_SR_RVU)) != 0U) {
        if (--rem == 0U) {
            return IWDG_ERR_TIMEOUT;
        }
    }

    /* 5. Program prescaler and 12-bit reload register */
    IWDG->PR  = pr;
    IWDG->RLR = rlr;

    /* 6. Wait bounded for hardware sync across APB -> LSI clock domain */
    rem = timeout_iters;
    while ((IWDG->SR & (IWDG_SR_PVU | IWDG_SR_RVU)) != 0U) {
        if (--rem == 0U) {
            return IWDG_ERR_TIMEOUT;
        }
    }

    /* 7. Reload down-counter immediately to start full timeout window */
    IWDG->KR = IWDG_KEY_RELOAD;
    return IWDG_OK;
}

bool iwdg_was_reset_cause(void) {
    return ((RCC->CSR & RCC_CSR_IWDGRSTF) != 0U);
}

void iwdg_clear_reset_flags(void) {
    RCC->CSR |= RCC_CSR_RMVF;
    (void)RCC->CSR;
    RCC->CSR &= ~RCC_CSR_RMVF;
}

#endif /* !defined(DRIVER_HOST_TEST) */
