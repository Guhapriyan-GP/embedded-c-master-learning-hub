/**
 * @file    test_drivers_math.c
 * @brief   Host Unit Test Suite for ALL 8 STM32G474RE Bare-Metal Driver Pure Helpers.
 *
 * Compiled directly against the real driver `.c` implementations in
 * `23-code-library/Drivers/` with `-DDRIVER_HOST_TEST`:
 *   - exti_g4.c   (`exti_calc_exticr_index_shift`)
 *   - tim_g4.c    (`tim_calc_psc_arr`, `tim_calc_pwm_ccr`)
 *   - usart_g4.c  (`usart_calc_brr`, `lpuart_calc_brr`)
 *   - spi_g4.c    (`spi_calc_br_div`)
 *   - i2c_g4.c    (`i2c_select_timingr` + UM10204 t_LOW/t_HIGH verification)
 *   - adc_g4.c    (`adc_raw_to_mv`, `adc_calc_vdda_mv`, `adc_temp_mdegc`)
 *   - dma_g4.c    (`dma_dmamux_channel_index` + RM0440 Table 91 Request IDs)
 *   - iwdg_g4.c   (`iwdg_calc_prescaler_reload`)
 *
 * Concept IDs: PERIPH-EXTI-001, PERIPH-TIM-001, PERIPH-PWM-001, PERIPH-UART-001,
 *              PERIPH-SPI-001, PERIPH-I2C-001, PERIPH-ADC-001, PERIPH-DMA-001,
 *              PERIPH-WDG-001, TOOL-QA-001
 * Verification Status: Host-Compiled & Executed (MSYS2 gcc 15.2.0, zero external deps).
 */

#include "adc_g4.h"
#include "dma_g4.h"
#include "exti_g4.h"
#include "i2c_g4.h"
#include "iwdg_g4.h"
#include "spi_g4.h"
#include "tim_g4.h"
#include "usart_g4.h"

#include <limits.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

static int g_tests_run    = 0;
static int g_tests_failed = 0;

#define ASSERT_TRUE(expr)                                                               \
    do {                                                                                \
        g_tests_run++;                                                                  \
        if (!(expr)) {                                                                  \
            g_tests_failed++;                                                           \
            fprintf(stderr, "  [FAIL] %s:%d: %s\n", __FILE__, __LINE__, #expr);         \
        }                                                                               \
    } while (0)

#define ASSERT_EQ_U32(expected, actual)                                                 \
    do {                                                                                \
        uint32_t _e = (uint32_t)(expected);                                             \
        uint32_t _a = (uint32_t)(actual);                                               \
        g_tests_run++;                                                                  \
        if (_e != _a) {                                                                 \
            g_tests_failed++;                                                           \
            fprintf(stderr, "  [FAIL] %s:%d: expected %u (0x%08X), got %u (0x%08X) (%s)\n", \
                    __FILE__, __LINE__, _e, _e, _a, _a, #actual);                       \
        }                                                                               \
    } while (0)

#define ASSERT_EQ_I32(expected, actual)                                                 \
    do {                                                                                \
        int32_t _e = (int32_t)(expected);                                               \
        int32_t _a = (int32_t)(actual);                                                 \
        g_tests_run++;                                                                  \
        if (_e != _a) {                                                                 \
            g_tests_failed++;                                                           \
            fprintf(stderr, "  [FAIL] %s:%d: expected %d, got %d (%s)\n",               \
                    __FILE__, __LINE__, _e, _a, #actual);                               \
        }                                                                               \
    } while (0)

/* ============================================================================
 * 1. EXTI Driver Pure Helper Tests (`exti_g4.c`)
 * ========================================================================== */
static void test_exti_exticr_mapping(void) {
    uint8_t idx   = 0xFFU;
    uint8_t shift = 0xFFU;

    /* Exhaustively verify all 16 GPIO EXTI lines (pins 0..15) */
    for (uint8_t pin = 0U; pin < 16U; ++pin) {
        ASSERT_TRUE(exti_calc_exticr_index_shift(pin, &idx, &shift));
        ASSERT_EQ_U32((uint32_t)(pin / 4U), idx);
        ASSERT_EQ_U32((uint32_t)((pin % 4U) * 4U), shift);
    }

    /* Spot-check NUCLEO-G474RE User Button B1 on PC13: EXTICR[3], nibble shift 4 */
    ASSERT_TRUE(exti_calc_exticr_index_shift(13U, &idx, &shift));
    ASSERT_EQ_U32(3U, idx);
    ASSERT_EQ_U32(4U, shift);

    /* Invalid pin and NULL output pointer rejection */
    ASSERT_TRUE(!exti_calc_exticr_index_shift(16U, &idx, &shift));
    ASSERT_TRUE(!exti_calc_exticr_index_shift(255U, &idx, &shift));
    ASSERT_TRUE(!exti_calc_exticr_index_shift(0U, NULL, &shift));
    ASSERT_TRUE(!exti_calc_exticr_index_shift(0U, &idx, NULL));
}

/* ============================================================================
 * 2. Timer & PWM Driver Pure Helper Tests (`tim_g4.c`)
 * ========================================================================== */
static void verify_tim_freq_tolerance(uint32_t clk_hz, uint32_t target_hz,
                                      uint32_t max_err_ppm) {
    uint16_t psc = 0U;
    uint32_t arr = 0U;
    ASSERT_TRUE(tim_calc_psc_arr(clk_hz, target_hz, &psc, &arr));
    ASSERT_TRUE(arr <= 65535U);

    uint64_t denom     = ((uint64_t)psc + 1ULL) * ((uint64_t)arr + 1ULL);
    uint64_t target_d  = (uint64_t)clk_hz;
    uint64_t actual_d  = denom * (uint64_t)target_hz;
    uint64_t diff      = (target_d > actual_d) ? (target_d - actual_d) : (actual_d - target_d);
    uint64_t err_ppm   = (diff * 1000000ULL) / target_d;
    ASSERT_TRUE(err_ppm <= max_err_ppm);
}

static void test_tim_psc_arr_and_pwm(void) {
    uint16_t psc = 0U;
    uint32_t arr = 0U;

    /* 170 MHz Range 1 Boost Profile (1 Hz, 10 Hz, 1 kHz, 25 kHz, 170 MHz) */
    verify_tim_freq_tolerance(170000000U, 1U, 100U);
    verify_tim_freq_tolerance(170000000U, 10U, 100U);
    verify_tim_freq_tolerance(170000000U, 1000U, 100U);
    verify_tim_freq_tolerance(170000000U, 25000U, 0U);
    verify_tim_freq_tolerance(170000000U, 170000000U, 0U);

    /* Exact register expectations for 25 kHz PWM @ 170 MHz: PSC=0, ARR=6799 */
    ASSERT_TRUE(tim_calc_psc_arr(170000000U, 25000U, &psc, &arr));
    ASSERT_EQ_U32(0U, psc);
    ASSERT_EQ_U32(6799U, arr);

    /* 160 MHz Range 1 Boost Profile (1 Hz, 10 Hz, 1 kHz, 25 kHz, 160 MHz) */
    verify_tim_freq_tolerance(160000000U, 1U, 100U);
    verify_tim_freq_tolerance(160000000U, 10U, 100U);
    verify_tim_freq_tolerance(160000000U, 1000U, 100U);
    verify_tim_freq_tolerance(160000000U, 25000U, 0U);
    verify_tim_freq_tolerance(160000000U, 160000000U, 0U);

    /* Exact register expectations for 25 kHz PWM @ 160 MHz: PSC=0, ARR=6399 */
    ASSERT_TRUE(tim_calc_psc_arr(160000000U, 25000U, &psc, &arr));
    ASSERT_EQ_U32(0U, psc);
    ASSERT_EQ_U32(6399U, arr);

    /* 16 MHz HSI16 Reset Profile */
    verify_tim_freq_tolerance(16000000U, 1U, 100U);
    verify_tim_freq_tolerance(16000000U, 1000U, 0U);

    /* Maximum 32-bit clk_hz / 1 Hz boundary */
    ASSERT_TRUE(tim_calc_psc_arr(UINT32_MAX, 1U, &psc, &arr));
    ASSERT_EQ_U32(65535U, psc);
    ASSERT_EQ_U32(65535U, arr);

    /* Error / invalid argument rejection */
    ASSERT_TRUE(!tim_calc_psc_arr(0U, 1000U, &psc, &arr));
    ASSERT_TRUE(!tim_calc_psc_arr(170000000U, 0U, &psc, &arr));
    ASSERT_TRUE(!tim_calc_psc_arr(170000000U, 170000001U, &psc, &arr));
    ASSERT_TRUE(!tim_calc_psc_arr(170000000U, 1000U, NULL, &arr));
    ASSERT_TRUE(!tim_calc_psc_arr(170000000U, 1000U, &psc, NULL));

    /* PWM CCR duty cycle calculations (0%, 25%, 50%, 75%, 100%, >100% clamp, 16-bit & 32-bit ARR guards) */
    ASSERT_EQ_U32(0U, tim_calc_pwm_ccr(6799U, 0U));
    ASSERT_EQ_U32(3400U, tim_calc_pwm_ccr(6799U, 500U));
    ASSERT_EQ_U32(1600U, tim_calc_pwm_ccr(6399U, 250U));
    ASSERT_EQ_U32(750U, tim_calc_pwm_ccr(999U, 750U));
    ASSERT_EQ_U32(1000U, tim_calc_pwm_ccr(999U, 1000U));  /* arr + 1 = solid HIGH */
    ASSERT_EQ_U32(1000U, tim_calc_pwm_ccr(999U, 1500U));  /* >1000 permille clamped */
    ASSERT_EQ_U32(0xFFFFU, tim_calc_pwm_ccr(0xFFFFU, 1000U)); /* 16-bit wraparound guard */
    ASSERT_TRUE(tim_calc_pwm_ccr(0xFFFFU, 999U) <= 0xFFFFU);
    ASSERT_EQ_U32(0xFFFFFFFFU, tim_calc_pwm_ccr(0xFFFFFFFFU, 1000U)); /* 32-bit TIM2 wrap guard */
    ASSERT_EQ_U32(0xFFFFFFFFU, tim_calc_pwm_ccr(0xFFFFFFFFU, 1500U));
    ASSERT_EQ_U32(0x80000000U, tim_calc_pwm_ccr(0xFFFFFFFFU, 500U));
    ASSERT_TRUE(tim_calc_pwm_ccr(0xFFFFFFFFU, 999U) < 0xFFFFFFFFU);
}

/* ============================================================================
 * 3. USART & LPUART Baud Rate Driver Pure Helper Tests (`usart_g4.c`)
 * ========================================================================== */
static void test_usart_and_lpuart_brr(void) {
    /* Standard USART (OVER8=0): 170 MHz */
    ASSERT_EQ_U32(17708U, usart_calc_brr(170000000U, 9600U));
    ASSERT_EQ_U32(1476U,  usart_calc_brr(170000000U, 115200U));
    ASSERT_EQ_U32(184U,   usart_calc_brr(170000000U, 921600U));

    /* Standard USART (OVER8=0): 160 MHz */
    ASSERT_EQ_U32(16667U, usart_calc_brr(160000000U, 9600U));
    ASSERT_EQ_U32(1389U,  usart_calc_brr(160000000U, 115200U));
    ASSERT_EQ_U32(174U,   usart_calc_brr(160000000U, 921600U));

    /* Standard USART (OVER8=0): 16 MHz HSI16 */
    ASSERT_EQ_U32(1667U,  usart_calc_brr(16000000U, 9600U));
    ASSERT_EQ_U32(139U,   usart_calc_brr(16000000U, 115200U));
    ASSERT_EQ_U32(17U,    usart_calc_brr(16000000U, 921600U));

    /* Exact min/max valid 16-bit USART BRR boundaries [0x10..0xFFFF] */
    ASSERT_EQ_U32(16U,     usart_calc_brr(16000000U, 1000000U));
    ASSERT_EQ_U32(0xFFFFU, usart_calc_brr(65535000U, 1000U));

    /* Out-of-range / invalid USART arguments */
    ASSERT_EQ_U32(0U, usart_calc_brr(0U, 115200U));
    ASSERT_EQ_U32(0U, usart_calc_brr(170000000U, 0U));
    ASSERT_EQ_U32(0U, usart_calc_brr(16000000U, 1500000U));  /* BRR = 11 < 16 */
    ASSERT_EQ_U32(0U, usart_calc_brr(170000000U, 1200U));    /* BRR > 0xFFFF */

    /* LPUART1 (256 * f_ck / baud, valid [0x300..0xFFFFF]) across 170/160/16 MHz & LSE */
    ASSERT_EQ_U32(377778U, lpuart_calc_brr(170000000U, 115200U));
    ASSERT_EQ_U32(47222U,  lpuart_calc_brr(170000000U, 921600U));
    ASSERT_EQ_U32(355556U, lpuart_calc_brr(160000000U, 115200U));
    ASSERT_EQ_U32(44444U,  lpuart_calc_brr(160000000U, 921600U));
    ASSERT_EQ_U32(35556U,  lpuart_calc_brr(16000000U,  115200U));
    ASSERT_EQ_U32(426667U, lpuart_calc_brr(16000000U,  9600U));
    ASSERT_EQ_U32(874U,    lpuart_calc_brr(32768U,     9600U)); /* 0x36A >= 0x300 */

    /* Out-of-range / invalid LPUART arguments */
    ASSERT_EQ_U32(0U, lpuart_calc_brr(0U, 115200U));
    ASSERT_EQ_U32(0U, lpuart_calc_brr(170000000U, 0U));
    ASSERT_EQ_U32(0U, lpuart_calc_brr(32768U, 115200U));    /* BRR = 73 < 0x300 */
    ASSERT_EQ_U32(0U, lpuart_calc_brr(170000000U, 9600U));  /* BRR = 4.53M > 0xFFFFF */
}

/* ============================================================================
 * 4. SPI Prescaler Driver Pure Helper Tests (`spi_g4.c`)
 * ========================================================================== */
static void test_spi_prescaler_boundaries(void) {
    /* 170 MHz APB clock: /2=85M, /4=42.5M, /8=21.25M, /16=10.625M, /32=5.3125M, /64=2.65625M, /128=1.328125M, /256=664062 */
    ASSERT_EQ_U32(0U, spi_calc_br_div(170000000U, 85000000U));
    ASSERT_EQ_U32(1U, spi_calc_br_div(170000000U, 84999999U));
    ASSERT_EQ_U32(1U, spi_calc_br_div(170000000U, 42500000U));
    ASSERT_EQ_U32(2U, spi_calc_br_div(170000000U, 42499999U));
    ASSERT_EQ_U32(2U, spi_calc_br_div(170000000U, 21250000U));
    ASSERT_EQ_U32(3U, spi_calc_br_div(170000000U, 20000000U));
    ASSERT_EQ_U32(4U, spi_calc_br_div(170000000U, 10000000U));
    ASSERT_EQ_U32(5U, spi_calc_br_div(170000000U, 5000000U));
    ASSERT_EQ_U32(6U, spi_calc_br_div(170000000U, 2000000U));
    ASSERT_EQ_U32(7U, spi_calc_br_div(170000000U, 1000000U));

    /* 160 MHz APB clock: exact powers-of-two boundaries (/2=80M .. /256=625k) */
    ASSERT_EQ_U32(0U, spi_calc_br_div(160000000U, 80000000U));
    ASSERT_EQ_U32(1U, spi_calc_br_div(160000000U, 40000000U));
    ASSERT_EQ_U32(2U, spi_calc_br_div(160000000U, 20000000U));
    ASSERT_EQ_U32(3U, spi_calc_br_div(160000000U, 10000000U));
    ASSERT_EQ_U32(4U, spi_calc_br_div(160000000U, 5000000U));
    ASSERT_EQ_U32(5U, spi_calc_br_div(160000000U, 2500000U));
    ASSERT_EQ_U32(6U, spi_calc_br_div(160000000U, 1250000U));
    ASSERT_EQ_U32(7U, spi_calc_br_div(160000000U, 625000U));

    /* Exact ceiling check: 170000001 / 2 = 85000000.5 > 85000000 -> must pick /4 (BR=1) */
    ASSERT_EQ_U32(1U, spi_calc_br_div(170000001U, 85000000U));

    /* Defensive zero & clamp bounds -> fallback to slowest /256 (BR=7) */
    ASSERT_EQ_U32(7U, spi_calc_br_div(0U, 1000000U));
    ASSERT_EQ_U32(7U, spi_calc_br_div(170000000U, 0U));
    ASSERT_EQ_U32(7U, spi_calc_br_div(170000000U, 100000U));
}

/* ============================================================================
 * 5. I2C TIMINGR Selector Pure Helper Tests (`i2c_g4.c`)
 * ========================================================================== */
static void verify_i2c_um10204_bounds(uint32_t clk_hz, uint32_t timingr,
                                      uint32_t min_tlow_ns, uint32_t min_thigh_ns,
                                      uint32_t min_scldel_ns, uint32_t max_fscl_hz) {
    uint32_t presc  = ((timingr >> 28U) & 0xFU) + 1U;
    uint32_t scldel = ((timingr >> 20U) & 0xFU) + 1U;
    uint32_t sclh   = ((timingr >> 8U) & 0xFFU) + 1U;
    uint32_t scll   = (timingr & 0xFFU) + 1U;

    uint64_t t_low_ns    = ((uint64_t)scll   * (uint64_t)presc * 1000000000ULL) / (uint64_t)clk_hz;
    uint64_t t_high_ns   = ((uint64_t)sclh   * (uint64_t)presc * 1000000000ULL) / (uint64_t)clk_hz;
    uint64_t t_scldel_ns = ((uint64_t)scldel * (uint64_t)presc * 1000000000ULL) / (uint64_t)clk_hz;

    ASSERT_TRUE(t_low_ns >= min_tlow_ns);
    ASSERT_TRUE(t_high_ns >= min_thigh_ns);
    ASSERT_TRUE(t_scldel_ns >= min_scldel_ns);

    if (max_fscl_hz > 0U) {
        /* RM0440 §39.4.9 fast-edge lower bound: t_SYNC1+2(min) = 4*t_I2CCLK + 2*50 ns */
        uint64_t t_sync_min_ns = (4000000000ULL / (uint64_t)clk_hz) + 100ULL;
        uint64_t t_scl_min_ns  = t_low_ns + t_high_ns + t_sync_min_ns;
        uint64_t f_scl_max_hz  = 1000000000ULL / t_scl_min_ns;
        ASSERT_TRUE(f_scl_max_hz <= (uint64_t)max_fscl_hz);
    }
}

static void test_i2c_timingr_profiles(void) {
    uint32_t t_170_100 = i2c_select_timingr(170000000U, I2C_SPEED_100KHZ);
    uint32_t t_160_100 = i2c_select_timingr(160000000U, I2C_SPEED_100KHZ);
    uint32_t t_16_100  = i2c_select_timingr(16000000U,  I2C_SPEED_100KHZ);
    uint32_t t_170_400 = i2c_select_timingr(170000000U, I2C_SPEED_400KHZ);
    uint32_t t_160_400 = i2c_select_timingr(160000000U, I2C_SPEED_400KHZ);
    uint32_t t_16_400  = i2c_select_timingr(16000000U,  I2C_SPEED_400KHZ);

    ASSERT_EQ_U32(I2C_TIMINGR_170MHZ_100KHZ, t_170_100);
    ASSERT_EQ_U32(I2C_TIMINGR_160MHZ_100KHZ, t_160_100);
    ASSERT_EQ_U32(I2C_TIMINGR_16MHZ_100KHZ,  t_16_100);
    ASSERT_EQ_U32(I2C_TIMINGR_170MHZ_400KHZ, t_170_400);
    ASSERT_EQ_U32(I2C_TIMINGR_160MHZ_400KHZ, t_160_400);
    ASSERT_EQ_U32(I2C_TIMINGR_16MHZ_400KHZ,  t_16_400);

    /* Exact regression locks on RM0440-remediated profiles */
    ASSERT_EQ_U32(0x70811021UL, t_170_400);
    ASSERT_EQ_U32(0x70710F1FUL, t_160_400);
    ASSERT_EQ_U32(0x30420F13UL, t_16_100);

    /* Verify NXP UM10204 minimum t_LOW, t_HIGH, t_SCLDEL, and max FM frequency */
    verify_i2c_um10204_bounds(170000000U, t_170_100, 4700U, 4000U,  600U,      0U);
    verify_i2c_um10204_bounds(160000000U, t_160_100, 4700U, 4000U,  600U,      0U);
    verify_i2c_um10204_bounds(16000000U,  t_16_100,  4700U, 4000U, 1250U,      0U);
    verify_i2c_um10204_bounds(170000000U, t_170_400, 1300U,  600U,  400U, 400000U);
    verify_i2c_um10204_bounds(160000000U, t_160_400, 1300U,  600U,  400U, 400000U);
    verify_i2c_um10204_bounds(16000000U,  t_16_400,  1300U,  600U,  250U, 400000U);

    /* Unsupported clocks and invalid speed enum rejection */
    ASSERT_EQ_U32(0U, i2c_select_timingr(0U, I2C_SPEED_100KHZ));
    ASSERT_EQ_U32(0U, i2c_select_timingr(80000000U, I2C_SPEED_100KHZ));
    ASSERT_EQ_U32(0U, i2c_select_timingr(80000000U, I2C_SPEED_400KHZ));
    ASSERT_EQ_U32(0U, i2c_select_timingr(170000000U, (i2c_speed_t)99));
}

/* ============================================================================
 * 6. ADC Voltage, VREFINT VDDA, and Die Temperature Pure Helpers (`adc_g4.c`)
 * ========================================================================== */
static void test_adc_conversions_and_temp(void) {
    /* adc_raw_to_mv: 0, mid-scale, full-scale, clamping >4095, zero VREF */
    ASSERT_EQ_U32(0U,    adc_raw_to_mv(0U, 3300U));
    ASSERT_EQ_U32(1650U, adc_raw_to_mv(2048U, 3300U));
    ASSERT_EQ_U32(3300U, adc_raw_to_mv(4095U, 3300U));
    ASSERT_EQ_U32(3000U, adc_raw_to_mv(4095U, 3000U));
    ASSERT_EQ_U32(3300U, adc_raw_to_mv(5000U, 3300U)); /* Clamped to 4095 */
    ASSERT_EQ_U32(0U,    adc_raw_to_mv(2048U, 0U));

    /* adc_calc_vdda_mv: divide-by-zero guard + nominal 3.0 V / 3.3 V / 2.76 V */
    ASSERT_EQ_U32(0U,    adc_calc_vdda_mv(0U, 1656U));
    ASSERT_EQ_U32(3000U, adc_calc_vdda_mv(1656U, 1656U));
    ASSERT_EQ_U32(3301U, adc_calc_vdda_mv(1505U, 1656U));
    ASSERT_EQ_U32(2760U, adc_calc_vdda_mv(1800U, 1656U));

    /* adc_temp_mdegc: two-point factory TS_CAL1 (30 C) / TS_CAL2 (130 C) interpolation */
    const uint16_t cal1 = 1030U; /* Representative 30 C @ 3.0 V code */
    const uint16_t cal2 = 1380U; /* Representative 130 C @ 3.0 V code */
    ASSERT_EQ_I32(30000,  adc_temp_mdegc(cal1, cal1, cal2, 3000U));
    ASSERT_EQ_I32(130000, adc_temp_mdegc(cal2, cal1, cal2, 3000U));
    ASSERT_EQ_I32(80000,  adc_temp_mdegc(1205U, cal1, cal2, 3000U)); /* Exact midpoint */
    ASSERT_TRUE(adc_temp_mdegc(890U, cal1, cal2, 3000U) < 0);        /* Sub-zero Celsius */

    /* Supply-voltage scaling check: 30 C sensor code at VDDA = 3.3 V is ~1030 * 3000/3300 = 936 */
    int32_t t_3v3 = adc_temp_mdegc(936U, cal1, cal2, 3300U);
    ASSERT_TRUE((t_3v3 >= 29800) && (t_3v3 <= 30200));

    /* Invalid calibration & divide-by-zero guards -> INT32_MIN */
    ASSERT_EQ_I32(INT32_MIN, adc_temp_mdegc(1100U, 1000U, 1000U, 3000U)); /* cal2 == cal1 */
    ASSERT_EQ_I32(INT32_MIN, adc_temp_mdegc(1100U, 1400U, 1000U, 3000U)); /* cal2 < cal1 */
    ASSERT_EQ_I32(INT32_MIN, adc_temp_mdegc(1100U, cal1, cal2, 0U));      /* vdda == 0 */
}

/* ============================================================================
 * 7. DMA / DMAMUX1 Channel Mapping & Request ID Tests (`dma_g4.c`)
 * ========================================================================== */
static void test_dma_dmamux_channel_mapping(void) {
    /* DMA1 Ch1..8 -> DMAMUX1 Ch0..7 */
    for (uint8_t ch = 1U; ch <= 8U; ++ch) {
        ASSERT_EQ_I32((int32_t)(ch - 1U), dma_dmamux_channel_index(1U, ch));
    }

    /* DMA2 Ch1..8 -> DMAMUX1 Ch8..15 */
    for (uint8_t ch = 1U; ch <= 8U; ++ch) {
        ASSERT_EQ_I32((int32_t)(8U + ch - 1U), dma_dmamux_channel_index(2U, ch));
    }

    /* Out-of-range controller & channel rejection */
    ASSERT_EQ_I32(-1, dma_dmamux_channel_index(0U, 1U));
    ASSERT_EQ_I32(-1, dma_dmamux_channel_index(3U, 1U));
    ASSERT_EQ_I32(-1, dma_dmamux_channel_index(1U, 0U));
    ASSERT_EQ_I32(-1, dma_dmamux_channel_index(1U, 9U));
    ASSERT_EQ_I32(-1, dma_dmamux_channel_index(2U, 0U));
    ASSERT_EQ_I32(-1, dma_dmamux_channel_index(2U, 9U));

    /* RM0440 Table 91 critical DMAMUX1 Request ID sanity checks (RX even, TX odd) */
    ASSERT_EQ_U32(5U,  DMAMUX_REQ_ADC1);
    ASSERT_EQ_U32(24U, DMAMUX_REQ_USART1_RX);
    ASSERT_EQ_U32(25U, DMAMUX_REQ_USART1_TX);
    ASSERT_EQ_U32(26U, DMAMUX_REQ_USART2_RX);
    ASSERT_EQ_U32(27U, DMAMUX_REQ_USART2_TX);
    ASSERT_EQ_U32(36U, DMAMUX_REQ_ADC2);
}

/* ============================================================================
 * 8. IWDG Prescaler & Reload Pure Helper Tests (`iwdg_g4.c`)
 * ========================================================================== */
static void verify_iwdg_timeout(uint32_t timeout_ms, uint32_t lsi_hz,
                                uint8_t expected_pr, uint16_t expected_rlr) {
    uint8_t  pr  = 0xFFU;
    uint16_t rlr = 0xFFFFU;
    ASSERT_TRUE(iwdg_calc_prescaler_reload(timeout_ms, lsi_hz, &pr, &rlr));
    ASSERT_EQ_U32(expected_pr, pr);
    ASSERT_EQ_U32(expected_rlr, rlr);
    ASSERT_TRUE(pr <= 6U);
    ASSERT_TRUE(rlr <= 4095U);
}

static void test_iwdg_prescaler_and_reload(void) {
    /* Standard 32 kHz nominal LSI timeouts */
    verify_iwdg_timeout(10U,    32000U, 0U, 79U);    /* /4,   80 ticks */
    verify_iwdg_timeout(500U,   32000U, 0U, 3999U);  /* /4, 4000 ticks */
    verify_iwdg_timeout(1000U,  32000U, 1U, 3999U);  /* /8, 4000 ticks */
    verify_iwdg_timeout(3000U,  32000U, 3U, 2999U);  /* /32, 3000 ticks */
    verify_iwdg_timeout(32000U, 32000U, 6U, 3999U);  /* /256, 4000 ticks */
    verify_iwdg_timeout(32768U, 32000U, 6U, 4095U);  /* Exact max: 32.768 s */

    /* Minimum 1-tick clamp branch (very slow clock + 1 ms -> ticks rounds to 0 -> clamped to 1) */
    verify_iwdg_timeout(1U, 100U, 0U, 0U);

    /* DS12288 LSI +/-5% tolerance bounds (30.4 kHz & 33.6 kHz) */
    uint8_t  pr  = 0U;
    uint16_t rlr = 0U;
    ASSERT_TRUE(iwdg_calc_prescaler_reload(1000U, 30400U, &pr, &rlr));
    ASSERT_EQ_U32(1U, pr);
    ASSERT_EQ_U32(3799U, rlr);

    ASSERT_TRUE(iwdg_calc_prescaler_reload(1000U, 33600U, &pr, &rlr));
    ASSERT_EQ_U32(2U, pr);
    ASSERT_EQ_U32(2099U, rlr);

    /* Saturation overflow & invalid argument rejection */
    ASSERT_TRUE(!iwdg_calc_prescaler_reload(0U, 32000U, &pr, &rlr));
    ASSERT_TRUE(!iwdg_calc_prescaler_reload(1000U, 0U, &pr, &rlr));
    ASSERT_TRUE(!iwdg_calc_prescaler_reload(1000U, 32000U, NULL, &rlr));
    ASSERT_TRUE(!iwdg_calc_prescaler_reload(1000U, 32000U, &pr, NULL));
    ASSERT_TRUE(!iwdg_calc_prescaler_reload(32776U, 32000U, &pr, &rlr)); /* > max 32768 ms */
    ASSERT_TRUE(!iwdg_calc_prescaler_reload(60000U, 32000U, &pr, &rlr));
}

int main(void) {
    printf("Running STM32G474RE 8-Driver Pure Helper Unit Tests (-DDRIVER_HOST_TEST)...\n");
    test_exti_exticr_mapping();
    test_tim_psc_arr_and_pwm();
    test_usart_and_lpuart_brr();
    test_spi_prescaler_boundaries();
    test_i2c_timingr_profiles();
    test_adc_conversions_and_temp();
    test_dma_dmamux_channel_mapping();
    test_iwdg_prescaler_and_reload();

    printf("Results: %d assertions executed, %d failed.\n", g_tests_run, g_tests_failed);
    return (g_tests_failed == 0) ? 0 : 1;
}
