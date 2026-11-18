/**
 * @file    adc_g4.h
 * @brief   Bare-metal CMSIS STM32G474RE ADC1 / ADC2 driver with safe clock limits,
 *          calibration, VREFINT VDDA measurement, and calibrated die temperature.
 *
 * Handles STM32G4-specific silicon details:
 *   - Uses synchronous `CKMODE = 0b11` (`HCLK / 4` = 42.5 MHz @ 170 MHz or
 *     40.0 MHz @ 160 MHz), strictly honoring DS12288 max f_ADC <= 60 MHz!
 *   - Executes full RM0440 §21.4.6 startup sequence:
 *     `DEEPPWD = 0` -> `ADVREGEN = 1` -> wait >= 20 us -> `ADCAL` -> `ISR = ADRDY`
 *     (direct rc_w1 write, NEVER `|=`) -> `ADEN = 1` -> wait `ADRDY`.
 *   - Enables `VSENSESEL` + waits >= 120 us t_START + uses 640.5-cycle sampling
 *     time (`SMP16 = 0b111`, >= 5 us DS12288 min) for internal temperature sensor.
 *   - Uses STM32G474RE factory calibration ROM words (`TS_CAL1`, `TS_CAL2`,
 *     `VREFINT_CAL`) with pure host-testable integer conversion functions.
 *
 * Reference: RM0440 Chapter 21 (ADC), DS12288 §3.14 & §5.3.19.
 * Concepts:  PERIPH-ADC-001
 */

#ifndef ADC_G4_H
#define ADC_G4_H

#include <stdbool.h>
#include <stdint.h>

#if !defined(DRIVER_HOST_TEST)
#include "stm32g474xx.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

/** @brief STM32G474RE System Memory factory calibration addresses (DS12288 §3.14). */
#define ADC_TS_CAL1_ADDR     0x1FFF75A8UL /**< 12-bit temp cal at 30 C,  VDDA = 3.0 V */
#define ADC_VREFINT_CAL_ADDR 0x1FFF75AAUL /**< 12-bit VREFINT cal at 30 C, VDDA = 3.0 V */
#define ADC_TS_CAL2_ADDR     0x1FFF75CAUL /**< 12-bit temp cal at 130 C, VDDA = 3.0 V */

#define ADC_CAL_VREF_MV      3000U        /**< Factory calibration reference voltage (mV) */
#define ADC_TS_CAL1_MDEGC    30000        /**< TS_CAL1 calibration temperature (30.000 C) */
#define ADC_TS_CAL2_MDEGC    130000       /**< TS_CAL2 calibration temperature (130.000 C) */

/** @brief Internal ADC1 channel assignments on STM32G474RE (RM0440 Table 155). */
#define ADC1_CH_TEMP_SENSOR  16U          /**< Internal temperature sensor (ADC1_IN16) */
#define ADC1_CH_VBAT_DIV3    17U          /**< Internal VBAT / 3 divider   (ADC1_IN17) */
#define ADC1_CH_VREFINT      18U          /**< Internal VREFINT (~1.212 V) (ADC1_IN18) */

/** @brief STM32G4 ADC sampling time codes (SMPR1 / SMPR2). */
typedef enum {
    ADC_SMP_2_5_CYCLES   = 0U,
    ADC_SMP_6_5_CYCLES   = 1U,
    ADC_SMP_12_5_CYCLES  = 2U,
    ADC_SMP_24_5_CYCLES  = 3U,
    ADC_SMP_47_5_CYCLES  = 4U,
    ADC_SMP_92_5_CYCLES  = 5U,
    ADC_SMP_247_5_CYCLES = 6U,
    ADC_SMP_640_5_CYCLES = 7U
} adc_smp_t;

/** @brief ADC driver status/error codes. */
typedef enum {
    ADC_OK              = 0,  /**< Operation succeeded                             */
    ADC_ERR_INVALID_ARG = -1, /**< NULL pointer, unsupported ADC, bad channel/smp  */
    ADC_ERR_TIMEOUT     = -2, /**< Bounded calibration / ADRDY / EOC wait expired  */
    ADC_ERR_NOT_READY   = -3  /**< ADC is not enabled (ADEN == 0)                  */
} adc_status_t;

/**
 * @brief Pure host-testable helper: convert 12-bit ADC raw code (0..4095) to
 *        millivolts with integer half-LSB rounding.
 * @param raw      12-bit ADC sample (values > 4095 are clamped to 4095).
 * @param vref_mv  Reference voltage VDDA / VREF+ in millivolts (e.g. 3300).
 * @return Voltage in millivolts.
 */
uint32_t adc_raw_to_mv(uint16_t raw, uint32_t vref_mv);

/**
 * @brief Pure host-testable helper: calculate actual VDDA supply voltage in mV
 *        from a VREFINT sample and STM32G4 factory `VREFINT_CAL` (at 3.0 V).
 * @param vrefint_raw  Measured 12-bit ADC1_IN18 raw code.
 * @param vrefint_cal  Factory 16-bit calibration value from ADC_VREFINT_CAL_ADDR.
 * @return Computed VDDA in mV, or 0 if vrefint_raw == 0.
 */
uint32_t adc_calc_vdda_mv(uint16_t vrefint_raw, uint16_t vrefint_cal);

/**
 * @brief Pure host-testable helper: calculate STM32G4 die temperature in
 *        millidegrees Celsius (mdegC) using two-point factory linear interpolation
 *        scaled for actual supply voltage @p vdda_mv (RM0440 §21.4.32).
 *
 * @param raw      Measured 12-bit ADC1_IN16 temperature sensor raw code.
 * @param ts_cal1  Factory calibration word at 30 C / 3.0 V (ADC_TS_CAL1_ADDR).
 * @param ts_cal2  Factory calibration word at 130 C / 3.0 V (ADC_TS_CAL2_ADDR).
 * @param vdda_mv  Actual VDDA supply voltage in mV (from adc_calc_vdda_mv()).
 * @return Temperature in millidegrees C (e.g., 25450 = 25.450 C), or INT32_MIN
 *         if calibration inputs are invalid (ts_cal2 <= ts_cal1 or vdda_mv == 0).
 */
int32_t adc_temp_mdegc(uint16_t raw, uint16_t ts_cal1, uint16_t ts_cal2,
                       uint32_t vdda_mv);

#if !defined(DRIVER_HOST_TEST)

/**
 * @brief Safe STM32G474RE initialization for ADC1 or ADC2:
 *        RCC ADC12EN -> CKMODE=11 (HCLK/4 <= 60 MHz) -> DEEPPWD=0 -> ADVREGEN=1
 *        -> >=20 us wait -> single-ended ADCAL -> rc_w1 ADRDY clear -> ADEN=1.
 *
 * @param adc            ADC1 or ADC2.
 * @param timeout_iters  Bounded polling iteration budget for ADDIS/ADCAL/ADRDY.
 * @return ADC_OK or error status.
 */
adc_status_t adc_init_single_ended(ADC_TypeDef *adc, uint32_t timeout_iters);

/**
 * @brief Perform a single blocking 12-bit conversion on @p channel (0..18).
 * @param adc            Initialized ADC1 or ADC2.
 * @param channel        ADC channel index 0..18.
 * @param smp            Sampling time code (ADC_SMP_2_5_CYCLES .. ADC_SMP_640_5_CYCLES).
 * @param out_raw        Output 12-bit conversion result (0..4095).
 * @param timeout_iters  Bounded polling iteration budget for EOC.
 * @return ADC_OK or error status.
 */
adc_status_t adc_read_channel(ADC_TypeDef *adc, uint8_t channel, adc_smp_t smp,
                              uint16_t *out_raw, uint32_t timeout_iters);

/**
 * @brief Enable internal VREFINT path and sample ADC1_IN18 at 640.5 cycles.
 * @note  Requires ADC1 initialized via adc_init_single_ended(ADC1, ...).
 */
adc_status_t adc_read_vrefint_raw(uint16_t *out_raw, uint32_t timeout_iters);

/**
 * @brief Enable internal Temperature Sensor (`VSENSESEL`), wait >= 120 us t_START,
 *        and sample ADC1_IN16 at 640.5 cycles (>= 5 us DS12288 minimum).
 * @note  Requires ADC1 initialized via adc_init_single_ended(ADC1, ...).
 */
adc_status_t adc_read_temp_raw(uint16_t *out_raw, uint32_t timeout_iters);

/**
 * @brief Measure actual VDDA (via VREFINT) and calibrated die temperature in mdegC
 *        using STM32G474RE factory ROM constants (TS_CAL1, TS_CAL2, VREFINT_CAL).
 *
 * @param out_mdegc      Output die temperature in millidegrees C.
 * @param out_vdda_mv    Optional output actual VDDA in mV (may be NULL).
 * @param timeout_iters  Bounded polling iteration budget per conversion.
 * @return ADC_OK or error status.
 */
adc_status_t adc_read_temp_mdegc(int32_t *out_mdegc, uint32_t *out_vdda_mv,
                                 uint32_t timeout_iters);

#endif /* !defined(DRIVER_HOST_TEST) */

#ifdef __cplusplus
}
#endif

#endif /* ADC_G4_H */
