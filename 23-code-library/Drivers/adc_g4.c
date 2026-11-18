/**
 * @file    adc_g4.c
 * @brief   Bare-metal CMSIS STM32G474RE ADC1 / ADC2 driver implementation.
 */

#include "adc_g4.h"
#include <limits.h>
#include <stddef.h>

#if !defined(DRIVER_HOST_TEST)
#include "clock_g4.h"
#endif

uint32_t adc_raw_to_mv(uint16_t raw, uint32_t vref_mv) {
    uint32_t clamped = (raw > 4095U) ? 4095U : (uint32_t)raw;
    return (clamped * vref_mv + 2047U) / 4095U;
}

uint32_t adc_calc_vdda_mv(uint16_t vrefint_raw, uint16_t vrefint_cal) {
    if (vrefint_raw == 0U) {
        return 0U;
    }
    return (((uint32_t)vrefint_cal * ADC_CAL_VREF_MV) + ((uint32_t)vrefint_raw / 2U)) /
           (uint32_t)vrefint_raw;
}

int32_t adc_temp_mdegc(uint16_t raw, uint16_t ts_cal1, uint16_t ts_cal2,
                       uint32_t vdda_mv) {
    if ((ts_cal2 <= ts_cal1) || (vdda_mv == 0U)) {
        return INT32_MIN;
    }

    int64_t scaled_raw = (int64_t)raw * (int64_t)vdda_mv;
    int64_t cal1_3v0   = (int64_t)ts_cal1 * (int64_t)ADC_CAL_VREF_MV;
    int64_t span_mdegc = (int64_t)(ADC_TS_CAL2_MDEGC - ADC_TS_CAL1_MDEGC);
    int64_t den        = (int64_t)(ts_cal2 - ts_cal1) * (int64_t)ADC_CAL_VREF_MV;
    int64_t delta      = ((scaled_raw - cal1_3v0) * span_mdegc) / den;

    return (int32_t)((int64_t)ADC_TS_CAL1_MDEGC + delta);
}

#if !defined(DRIVER_HOST_TEST)

static bool adc_is_supported(const ADC_TypeDef *adc) {
    return (adc == ADC1) || (adc == ADC2);
}

adc_status_t adc_init_single_ended(ADC_TypeDef *adc, uint32_t timeout_iters) {
    if ((adc == NULL) || !adc_is_supported(adc) || (timeout_iters == 0U)) {
        return ADC_ERR_INVALID_ARG;
    }

    /* 1. Enable ADC12 AHB2 clock (+ readback sync) */
    RCC->AHB2ENR |= RCC_AHB2ENR_ADC12EN;
    (void)RCC->AHB2ENR;

    /* 2. If ADC is currently enabled, disable cleanly via ADDIS (writing ADEN=0 has no effect!) */
    if ((adc->CR & ADC_CR_ADEN) != 0U) {
        adc->CR |= ADC_CR_ADDIS;
        uint32_t rem = timeout_iters;
        while ((adc->CR & ADC_CR_ADEN) != 0U) {
            if (--rem == 0U) {
                return ADC_ERR_TIMEOUT;
            }
        }
    }

    /* 3. Select synchronous clock CKMODE = 0b11 (HCLK / 4):
     *    At 170 MHz -> 42.5 MHz; at 160 MHz -> 40.0 MHz (strictly <= 60 MHz DS12288 max!).
     *    RM0440 §21.7.17: Shared ADC12_COMMON->CCR.CKMODE may only be written when
     *    BOTH ADC1 and ADC2 are disabled (ADEN == 0). */
    if (((ADC1->CR & ADC_CR_ADEN) == 0U) && ((ADC2->CR & ADC_CR_ADEN) == 0U)) {
        ADC12_COMMON->CCR = (ADC12_COMMON->CCR & ~ADC_CCR_CKMODE) |
                            (3UL << ADC_CCR_CKMODE_Pos);
    } else if ((ADC12_COMMON->CCR & ADC_CCR_CKMODE) != (3UL << ADC_CCR_CKMODE_Pos)) {
        return ADC_ERR_NOT_READY;
    } else {
        /* Peer ADC is active with CKMODE = 0b11; safe to reuse shared clock */
    }

    /* 4. Exit Deep Power-Down (STM32G4 reset default: ADC_CR = 0x20000000 -> DEEPPWD=1) */
    adc->CR &= ~ADC_CR_DEEPPWD;

    /* 5. Enable internal ADC voltage regulator and wait >= 20 us (t_ADCVREG_STUP) */
    adc->CR |= ADC_CR_ADVREGEN;
    Clock_DelayUs(25U);

    /* 6. Perform single-ended calibration (ADCALDIF = 0 -> ADCAL = 1 -> wait ADCAL == 0) */
    adc->CR &= ~ADC_CR_ADCALDIF;
    adc->CR |= ADC_CR_ADCAL;
    uint32_t rem = timeout_iters;
    while ((adc->CR & ADC_CR_ADCAL) != 0U) {
        if (--rem == 0U) {
            return ADC_ERR_TIMEOUT;
        }
    }

    /* 7. Clear ADRDY via direct rc_w1 write (NEVER `|=`) then set ADEN = 1 */
    adc->ISR = ADC_ISR_ADRDY;
    adc->CR |= ADC_CR_ADEN;
    rem = timeout_iters;
    while ((adc->ISR & ADC_ISR_ADRDY) == 0U) {
        if (--rem == 0U) {
            return ADC_ERR_TIMEOUT;
        }
    }

    /* 8. Clear ADRDY and configure default 12-bit right-aligned single polling mode */
    adc->ISR  = ADC_ISR_ADRDY;
    adc->CFGR &= ~(ADC_CFGR_CONT | ADC_CFGR_ALIGN | ADC_CFGR_RES |
                   ADC_CFGR_EXTEN | ADC_CFGR_DMAEN | ADC_CFGR_DMACFG);

    return ADC_OK;
}

adc_status_t adc_read_channel(ADC_TypeDef *adc, uint8_t channel, adc_smp_t smp,
                              uint16_t *out_raw, uint32_t timeout_iters) {
    if ((adc == NULL) || !adc_is_supported(adc) || (channel > 18U) ||
        ((uint32_t)smp > (uint32_t)ADC_SMP_640_5_CYCLES) ||
        (out_raw == NULL) || (timeout_iters == 0U)) {
        return ADC_ERR_INVALID_ARG;
    }

    if ((adc->CR & ADC_CR_ADEN) == 0U) {
        return ADC_ERR_NOT_READY;
    }

    /* Ensure DMA requests are disabled before single CPU polling conversion */
    adc->CFGR &= ~(ADC_CFGR_DMAEN | ADC_CFGR_DMACFG);

    /* Configure per-channel sampling time (SMPR1: ch 0..9, SMPR2: ch 10..18) */
    if (channel <= 9U) {
        uint32_t shift = (uint32_t)channel * 3U;
        adc->SMPR1 = (adc->SMPR1 & ~(0x7UL << shift)) |
                     (((uint32_t)smp & 0x7UL) << shift);
    } else {
        uint32_t shift = ((uint32_t)channel - 10U) * 3U;
        adc->SMPR2 = (adc->SMPR2 & ~(0x7UL << shift)) |
                     (((uint32_t)smp & 0x7UL) << shift);
    }

    /* Single-conversion regular sequence: L = 0 (1 conversion), SQ1 = channel */
    adc->SQR1 = ((uint32_t)channel << ADC_SQR1_SQ1_Pos);

    /* Clear stale EOC/EOS flags (direct rc_w1 write) and trigger conversion */
    adc->ISR = ADC_ISR_EOC | ADC_ISR_EOS;
    adc->CR |= ADC_CR_ADSTART;

    uint32_t rem = timeout_iters;
    while ((adc->ISR & ADC_ISR_EOC) == 0U) {
        if (--rem == 0U) {
            return ADC_ERR_TIMEOUT;
        }
    }

    *out_raw = (uint16_t)(adc->DR & 0x0FFFU);
    return ADC_OK;
}

adc_status_t adc_read_vrefint_raw(uint16_t *out_raw, uint32_t timeout_iters) {
    if ((out_raw == NULL) || (timeout_iters == 0U)) {
        return ADC_ERR_INVALID_ARG;
    }

    /* Enable internal VREFINT path and allow startup stabilization */
    if ((ADC12_COMMON->CCR & ADC_CCR_VREFEN) == 0U) {
        ADC12_COMMON->CCR |= ADC_CCR_VREFEN;
        Clock_DelayUs(20U);
    }

    /* 640.5 cycles @ 42.5 MHz = 15.1 us (>= 4 us DS12288 minimum for VREFINT) */
    return adc_read_channel(ADC1, ADC1_CH_VREFINT, ADC_SMP_640_5_CYCLES,
                            out_raw, timeout_iters);
}

adc_status_t adc_read_temp_raw(uint16_t *out_raw, uint32_t timeout_iters) {
    if ((out_raw == NULL) || (timeout_iters == 0U)) {
        return ADC_ERR_INVALID_ARG;
    }

    /* Enable internal Temperature Sensor path and wait >= 120 us t_START (DS12288) */
    if ((ADC12_COMMON->CCR & ADC_CCR_VSENSESEL) == 0U) {
        ADC12_COMMON->CCR |= ADC_CCR_VSENSESEL;
        Clock_DelayUs(125U);
    }

    /* 640.5 cycles @ 42.5 MHz = 15.1 us (>= 5 us DS12288 minimum for Temp Sensor) */
    return adc_read_channel(ADC1, ADC1_CH_TEMP_SENSOR, ADC_SMP_640_5_CYCLES,
                            out_raw, timeout_iters);
}

adc_status_t adc_read_temp_mdegc(int32_t *out_mdegc, uint32_t *out_vdda_mv,
                                 uint32_t timeout_iters) {
    if ((out_mdegc == NULL) || (timeout_iters == 0U)) {
        return ADC_ERR_INVALID_ARG;
    }

    uint16_t vref_raw = 0U;
    adc_status_t st = adc_read_vrefint_raw(&vref_raw, timeout_iters);
    if (st != ADC_OK) {
        return st;
    }

    uint16_t vref_cal = *(const volatile uint16_t *)(uintptr_t)ADC_VREFINT_CAL_ADDR;
    uint32_t vdda_mv  = adc_calc_vdda_mv(vref_raw, vref_cal);
    if (out_vdda_mv != NULL) {
        *out_vdda_mv = vdda_mv;
    }

    uint16_t temp_raw = 0U;
    st = adc_read_temp_raw(&temp_raw, timeout_iters);
    if (st != ADC_OK) {
        return st;
    }

    uint16_t ts_cal1 = *(const volatile uint16_t *)(uintptr_t)ADC_TS_CAL1_ADDR;
    uint16_t ts_cal2 = *(const volatile uint16_t *)(uintptr_t)ADC_TS_CAL2_ADDR;

    *out_mdegc = adc_temp_mdegc(temp_raw, ts_cal1, ts_cal2, vdda_mv);
    return ADC_OK;
}

#endif /* !defined(DRIVER_HOST_TEST) */
