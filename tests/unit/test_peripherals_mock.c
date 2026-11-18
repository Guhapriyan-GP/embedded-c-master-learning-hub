/**
 * @file    test_peripherals_mock.c
 * @brief   Comprehensive Register-Level Functional Host Unit Test Suite for ALL 8
 *          STM32G474RE Bare-Metal Peripheral Drivers (`exti_g4`, `tim_g4`, `usart_g4`,
 *          `spi_g4`, `i2c_g4`, `adc_g4`, `dma_g4`, `iwdg_g4`) + `gpio_g4` & `ring_buffer`.
 *
 * Links DIRECTLY against the real `.c` driver implementations in
 * `23-code-library/Drivers/` WITHOUT `-DDRIVER_HOST_TEST`, using the CMSIS-verified
 * fake register backing header `tests/mocks/stm32g474xx.h`.
 *
 * Concept IDs: PERIPH-EXTI-001, PERIPH-SYSCFG-001, PERIPH-TIM-001, PERIPH-PWM-001,
 *              PERIPH-UART-001, PERIPH-SPI-001, PERIPH-I2C-001, PERIPH-I2C-003,
 *              PERIPH-ADC-001, PERIPH-DMA-001, STM32-ERRATA-001, PERIPH-WDG-001,
 *              TOOL-QA-001
 * Verification Status: Host-Compiled & Executed (GCC host compiler, fake CMSIS registers).
 */

#include "adc_g4.h"
#include "clock_g4.h"
#include "dma_g4.h"
#include "exti_g4.h"
#include "gpio_g4.h"
#include "i2c_g4.h"
#include "iwdg_g4.h"
#include "ring_buffer.h"
#include "spi_g4.h"
#include "tim_g4.h"
#include "usart_g4.h"

#include <limits.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

/* ============================================================================
 * Backing Fake CMSIS Peripheral Register Instances & Core State
 * ========================================================================== */
mock_core_state_t      g_mock_core;
uint32_t               SystemCoreClock = 170000000UL;
const uint8_t          APBPrescTable[8] = { 0U, 0U, 0U, 0U, 1U, 2U, 3U, 4U };

GPIO_TypeDef           g_mock_gpioa;
GPIO_TypeDef           g_mock_gpiob;
GPIO_TypeDef           g_mock_gpioc;
GPIO_TypeDef           g_mock_gpiod;
GPIO_TypeDef           g_mock_gpioe;
GPIO_TypeDef           g_mock_gpiof;
GPIO_TypeDef           g_mock_gpiog;
RCC_TypeDef            g_mock_rcc;
SYSCFG_TypeDef         g_mock_syscfg;
EXTI_TypeDef           g_mock_exti;
TIM_TypeDef            g_mock_tim2;
TIM_TypeDef            g_mock_tim3;
TIM_TypeDef            g_mock_tim6;
TIM_TypeDef            g_mock_tim7;
USART_TypeDef          g_mock_usart1;
USART_TypeDef          g_mock_usart2;
USART_TypeDef          g_mock_lpuart1;
SPI_TypeDef            g_mock_spi1;
SPI_TypeDef            g_mock_spi2;
SPI_TypeDef            g_mock_spi3;
I2C_TypeDef            g_mock_i2c1;
I2C_TypeDef            g_mock_i2c2;
I2C_TypeDef            g_mock_i2c3;
ADC_TypeDef            g_mock_adc1;
ADC_TypeDef            g_mock_adc2;
ADC_Common_TypeDef     g_mock_adc12_common;
DMA_TypeDef            g_mock_dma1;
DMA_TypeDef            g_mock_dma2;
DMA_Channel_TypeDef    g_mock_dma1_ch[8];
DMA_Channel_TypeDef    g_mock_dma2_ch[8];
DMAMUX_Channel_TypeDef g_mock_dmamux1_ch[16];
IWDG_TypeDef           g_mock_iwdg;
DBGMCU_TypeDef         g_mock_dbgmcu;

/* Stub instrumentation for Clock_DelayUs() called by adc_g4.c & i2c_g4.c */
static uint32_t g_mock_delay_us_total = 0U;
static uint32_t g_mock_delay_us_calls = 0U;

void Clock_DelayUs(uint32_t us) {
    g_mock_delay_us_total += us;
    g_mock_delay_us_calls++;
}

/* ============================================================================
 * Minimal Unit Test Assertion Harness
 * ========================================================================== */
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
            fprintf(stderr, "  [FAIL] %s:%d: expected 0x%08X (%u), got 0x%08X (%u) (%s)\n", \
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

static void mock_reset_all(void) {
    memset(&g_mock_core, 0, sizeof(g_mock_core));
    memset(&g_mock_gpioa, 0, sizeof(g_mock_gpioa));
    memset(&g_mock_gpiob, 0, sizeof(g_mock_gpiob));
    memset(&g_mock_gpioc, 0, sizeof(g_mock_gpioc));
    memset(&g_mock_gpiod, 0, sizeof(g_mock_gpiod));
    memset(&g_mock_gpioe, 0, sizeof(g_mock_gpioe));
    memset(&g_mock_gpiof, 0, sizeof(g_mock_gpiof));
    memset(&g_mock_gpiog, 0, sizeof(g_mock_gpiog));
    memset(&g_mock_rcc, 0, sizeof(g_mock_rcc));
    memset(&g_mock_syscfg, 0, sizeof(g_mock_syscfg));
    memset(&g_mock_exti, 0, sizeof(g_mock_exti));
    memset(&g_mock_tim2, 0, sizeof(g_mock_tim2));
    memset(&g_mock_tim3, 0, sizeof(g_mock_tim3));
    memset(&g_mock_tim6, 0, sizeof(g_mock_tim6));
    memset(&g_mock_tim7, 0, sizeof(g_mock_tim7));
    memset(&g_mock_usart1, 0, sizeof(g_mock_usart1));
    memset(&g_mock_usart2, 0, sizeof(g_mock_usart2));
    memset(&g_mock_lpuart1, 0, sizeof(g_mock_lpuart1));
    memset(&g_mock_spi1, 0, sizeof(g_mock_spi1));
    memset(&g_mock_spi2, 0, sizeof(g_mock_spi2));
    memset(&g_mock_spi3, 0, sizeof(g_mock_spi3));
    memset(&g_mock_i2c1, 0, sizeof(g_mock_i2c1));
    memset(&g_mock_i2c2, 0, sizeof(g_mock_i2c2));
    memset(&g_mock_i2c3, 0, sizeof(g_mock_i2c3));
    memset(&g_mock_adc1, 0, sizeof(g_mock_adc1));
    memset(&g_mock_adc2, 0, sizeof(g_mock_adc2));
    memset(&g_mock_adc12_common, 0, sizeof(g_mock_adc12_common));
    memset(&g_mock_dma1, 0, sizeof(g_mock_dma1));
    memset(&g_mock_dma2, 0, sizeof(g_mock_dma2));
    memset(g_mock_dma1_ch, 0, sizeof(g_mock_dma1_ch));
    memset(g_mock_dma2_ch, 0, sizeof(g_mock_dma2_ch));
    memset(g_mock_dmamux1_ch, 0, sizeof(g_mock_dmamux1_ch));
    memset(&g_mock_iwdg, 0, sizeof(g_mock_iwdg));
    memset(&g_mock_dbgmcu, 0, sizeof(g_mock_dbgmcu));

    /* STM32G474RE reset defaults for ADC (DEEPPWD=1) and clock */
    g_mock_adc1.CR        = ADC_CR_DEEPPWD;
    g_mock_adc2.CR        = ADC_CR_DEEPPWD;
    SystemCoreClock       = 170000000UL;
    g_mock_delay_us_total = 0U;
    g_mock_delay_us_calls = 0U;
}

/* ============================================================================
 * 1. EXTI & SYSCFG Register-Level Functional Tests (`exti_g4.c`)
 * ========================================================================== */
typedef struct {
    uint32_t calls;
    uint8_t  last_line;
    uint32_t pr1_seen_in_cb;
} exti_cb_state_t;

static void mock_exti_cb(uint8_t line, void *ctx) {
    exti_cb_state_t *st = (exti_cb_state_t *)ctx;
    st->calls++;
    st->last_line      = line;
    st->pr1_seen_in_cb = EXTI->PR1;
}

static void test_exti_registers_and_dispatch(void) {
    mock_reset_all();

    /* PC13 Rising Edge (NUCLEO-G474RE Active-HIGH B1) -> EXTICR[3] nibble [7:4] = 2 */
    ASSERT_EQ_I32(EXTI_OK, EXTI_ConfigLine(GPIOC, 13U, EXTI_EDGE_RISING, 3U));
    ASSERT_EQ_U32(RCC_APB2ENR_SYSCFGEN, RCC->APB2ENR & RCC_APB2ENR_SYSCFGEN);
    ASSERT_EQ_U32(0x00000020UL, SYSCFG->EXTICR[3]);
    ASSERT_EQ_U32(1UL << 13U, EXTI->RTSR1);
    ASSERT_EQ_U32(0x00000000UL, EXTI->FTSR1);
    ASSERT_EQ_U32(1UL << 13U, EXTI->IMR1);
    ASSERT_EQ_U32(1UL << 13U, EXTI->PR1); /* Direct rc_w1 clear written during init */
    ASSERT_EQ_U32(1U, g_mock_core.enabled[EXTI15_10_IRQn]);
    ASSERT_EQ_U32(3U, g_mock_core.priority[EXTI15_10_IRQn]);
    ASSERT_TRUE(g_mock_core.clear_pending_cnt[EXTI15_10_IRQn] >= 1U);
    ASSERT_TRUE(g_mock_core.dsb_count >= 1U);

    /* PG15 Both Edges -> EXTICR[3] nibble [15:12] = 6 without clobbering PC13 nibble! */
    ASSERT_EQ_I32(EXTI_OK, EXTI_ConfigLine(GPIOG, 15U, EXTI_EDGE_BOTH, 4U));
    ASSERT_EQ_U32(0x00006020UL, SYSCFG->EXTICR[3]);
    ASSERT_EQ_U32((1UL << 13U) | (1UL << 15U), EXTI->RTSR1);
    ASSERT_EQ_U32(1UL << 15U, EXTI->FTSR1);

    /* PA0..PF5 port codes & individual EXTI0..4 / EXTI9_5 IRQ routing */
    ASSERT_EQ_I32(EXTI_OK, EXTI_ConfigLine(GPIOA, 0U, EXTI_EDGE_FALLING, 2U));
    ASSERT_EQ_I32(EXTI_OK, EXTI_ConfigLine(GPIOB, 1U, EXTI_EDGE_RISING, 2U));
    ASSERT_EQ_I32(EXTI_OK, EXTI_ConfigLine(GPIOD, 2U, EXTI_EDGE_RISING, 2U));
    ASSERT_EQ_I32(EXTI_OK, EXTI_ConfigLine(GPIOE, 3U, EXTI_EDGE_RISING, 2U));
    ASSERT_EQ_I32(EXTI_OK, EXTI_ConfigLine(GPIOF, 4U, EXTI_EDGE_RISING, 2U));
    ASSERT_EQ_I32(EXTI_OK, EXTI_ConfigLine(GPIOB, 7U, EXTI_EDGE_NONE, 5U));
    ASSERT_EQ_U32(0x00004310UL, SYSCFG->EXTICR[0]); /* PA0=0, PB1=1, PD2=3, PE3=4 */
    ASSERT_EQ_U32(0x00001005UL, SYSCFG->EXTICR[1]); /* PF4=5, PB7=1 */
    ASSERT_EQ_U32(1U, g_mock_core.enabled[EXTI0_IRQn]);
    ASSERT_EQ_U32(1U, g_mock_core.enabled[EXTI4_IRQn]);
    ASSERT_EQ_U32(0U, g_mock_core.enabled[EXTI9_5_IRQn]); /* EXTI_EDGE_NONE stays masked */
    ASSERT_EQ_U32(0U, EXTI->IMR1 & (1UL << 7U));

    /* Register callback, trigger SWIER1, and verify DispatchRange clears PR1 BEFORE callback */
    exti_cb_state_t cb_st = { 0U, 0xFFU, 0U };
    ASSERT_EQ_I32(EXTI_OK, EXTI_RegisterCallback(13U, mock_exti_cb, &cb_st));
    EXTI_TriggerSoftware(13U);
    ASSERT_EQ_U32(1UL << 13U, EXTI->SWIER1);

    /* Set PR1 for unmasked line 13 + masked line 12 -> only line 13 must dispatch */
    EXTI->PR1 = (1UL << 13U) | (1UL << 12U);
    uint32_t dsb_before = g_mock_core.dsb_count;
    EXTI_DispatchRange(10U, 15U);
    ASSERT_EQ_U32(1U, cb_st.calls);
    ASSERT_EQ_U32(13U, cb_st.last_line);
    /* Direct rc_w1 write `EXTI->PR1 = (1UL << 13)` overwrote PR1 before cb ran! */
    ASSERT_EQ_U32(1UL << 13U, cb_st.pr1_seen_in_cb);
    ASSERT_TRUE(g_mock_core.dsb_count >= (dsb_before + 2U)); /* per-line + tail DSB */

    /* DisableLine masks IMR1, clears edges, and clears PR1 */
    ASSERT_EQ_I32(EXTI_OK, EXTI_DisableLine(13U));
    ASSERT_EQ_U32(0U, EXTI->IMR1 & (1UL << 13U));
    ASSERT_EQ_U32(0U, EXTI->RTSR1 & (1UL << 13U));
    ASSERT_EQ_U32(0U, EXTI->FTSR1 & (1UL << 13U));

    /* Invalid argument guards */
    GPIO_TypeDef unknown_port;
    memset(&unknown_port, 0, sizeof(unknown_port));
    ASSERT_EQ_I32(EXTI_ERR_INVALID_ARG, EXTI_ConfigLine(NULL, 13U, EXTI_EDGE_RISING, 3U));
    ASSERT_EQ_I32(EXTI_ERR_INVALID_ARG, EXTI_ConfigLine(&unknown_port, 13U, EXTI_EDGE_RISING, 3U));
    ASSERT_EQ_I32(EXTI_ERR_INVALID_ARG, EXTI_ConfigLine(GPIOC, 16U, EXTI_EDGE_RISING, 3U));
    ASSERT_EQ_I32(EXTI_ERR_INVALID_ARG, EXTI_DisableLine(16U));
    ASSERT_EQ_I32(EXTI_ERR_INVALID_ARG, EXTI_RegisterCallback(16U, mock_exti_cb, &cb_st));
    EXTI_DispatchRange(15U, 10U); /* Invalid range no-op */
    EXTI_DispatchRange(0U, 16U);  /* Invalid range no-op */
    ASSERT_EQ_U32(1U, cb_st.calls);
}

/* ============================================================================
 * 2. TIM Periodic, PWM Preload & rc_w0 Flag Clear Tests (`tim_g4.c`)
 * ========================================================================== */
static void test_tim_registers_and_pwm(void) {
    mock_reset_all();

    /* APB1 timer x2 clock rule: PPRE1 = /1 -> 170M, /2 -> 170M, /4 -> 85M, /8 -> 42.5M */
    RCC->CFGR = 0U; /* PPRE1 = /1 */
    ASSERT_EQ_U32(170000000UL, TIM_GetKernelClockHz(TIM2));
    RCC->CFGR = (4UL << RCC_CFGR_PPRE1_Pos); /* PPRE1 = /2 -> TIMCLK = 2 * PCLK1 = 170M */
    ASSERT_EQ_U32(170000000UL, TIM_GetKernelClockHz(TIM2));
    RCC->CFGR = (5UL << RCC_CFGR_PPRE1_Pos); /* PPRE1 = /4 -> TIMCLK = 2 * 42.5M = 85M */
    ASSERT_EQ_U32(85000000UL, TIM_GetKernelClockHz(TIM3));
    RCC->CFGR = 0U;

    TIM_TypeDef foreign_tim;
    memset(&foreign_tim, 0, sizeof(foreign_tim));
    ASSERT_EQ_U32(0U, TIM_GetKernelClockHz(&foreign_tim));

    /* TIM2 Periodic 100 Hz @ 170 MHz with NVIC IRQ prio 5 */
    ASSERT_EQ_I32(TIM_OK, TIM_InitPeriodic(TIM2, 100U, true, 5U));
    ASSERT_EQ_U32(RCC_APB1ENR1_TIM2EN, RCC->APB1ENR1 & RCC_APB1ENR1_TIM2EN);
    ASSERT_EQ_U32(25U, TIM2->PSC);
    ASSERT_EQ_U32(65384U, TIM2->ARR);
    ASSERT_EQ_U32(TIM_EGR_UG, TIM2->EGR);
    ASSERT_EQ_U32(~TIM_SR_UIF, TIM2->SR); /* Direct rc_w0 ~UIF write */
    ASSERT_EQ_U32(TIM_DIER_UIE, TIM2->DIER);
    ASSERT_EQ_U32(TIM_CR1_ARPE | TIM_CR1_CEN, TIM2->CR1);
    ASSERT_EQ_U32(1U, g_mock_core.enabled[TIM2_IRQn]);
    ASSERT_EQ_U32(5U, g_mock_core.priority[TIM2_IRQn]);

    /* TIM6 & TIM7 basic timers periodic init + IRQ mapping */
    ASSERT_EQ_I32(TIM_OK, TIM_InitPeriodic(TIM6, 1000U, true, 6U));
    ASSERT_EQ_I32(TIM_OK, TIM_InitPeriodic(TIM7, 1000U, true, 7U));
    ASSERT_EQ_U32(1U, g_mock_core.enabled[TIM6_DAC_IRQn]);
    ASSERT_EQ_U32(1U, g_mock_core.enabled[TIM7_DAC_IRQn]);

    /* TIM_ClearUpdateFlag: rc_w0 direct write (~TIM_SR_UIF = 0xFFFFFFFE) + __DSB() */
    TIM2->SR = 0x00000001UL;
    uint32_t dsb_before = g_mock_core.dsb_count;
    TIM_ClearUpdateFlag(TIM2);
    ASSERT_EQ_U32(0xFFFFFFFEUL, TIM2->SR);
    ASSERT_EQ_U32(dsb_before + 1U, g_mock_core.dsb_count);

    /* TIM3 25 kHz PWM on all 4 channels (ARPE + OCxPE + PWM Mode 1 = 0b0110) */
    ASSERT_EQ_I32(TIM_OK, TIM_InitPwm(TIM3, 1U, 25000U, 250U)); /* 25.0% -> CCR1 = 1700 */
    ASSERT_EQ_I32(TIM_OK, TIM_InitPwm(TIM3, 2U, 25000U, 500U)); /* 50.0% -> CCR2 = 3400 */
    ASSERT_EQ_I32(TIM_OK, TIM_InitPwm(TIM3, 3U, 25000U, 750U)); /* 75.0% -> CCR3 = 5100 */
    ASSERT_EQ_I32(TIM_OK, TIM_InitPwm(TIM3, 4U, 25000U, 1000U));/* 100%  -> CCR4 = 6800 */
    ASSERT_EQ_U32(0U, TIM3->PSC);
    ASSERT_EQ_U32(6799U, TIM3->ARR);
    ASSERT_EQ_U32(1700U, TIM3->CCR1);
    ASSERT_EQ_U32(3400U, TIM3->CCR2);
    ASSERT_EQ_U32(5100U, TIM3->CCR3);
    ASSERT_EQ_U32(6800U, TIM3->CCR4);
    ASSERT_EQ_U32((6UL << TIM_CCMR1_OC1M_Pos) | TIM_CCMR1_OC1PE |
                  (6UL << TIM_CCMR1_OC2M_Pos) | TIM_CCMR1_OC2PE, TIM3->CCMR1);
    ASSERT_EQ_U32((6UL << TIM_CCMR2_OC3M_Pos) | TIM_CCMR2_OC3PE |
                  (6UL << TIM_CCMR2_OC4M_Pos) | TIM_CCMR2_OC4PE, TIM3->CCMR2);
    ASSERT_EQ_U32(TIM_CCER_CC1E | TIM_CCER_CC2E | TIM_CCER_CC3E | TIM_CCER_CC4E, TIM3->CCER);
    ASSERT_EQ_U32(TIM_CR1_ARPE | TIM_CR1_CEN, TIM3->CR1);

    /* Runtime duty update via TIM_SetPwmDutyPermille */
    ASSERT_EQ_I32(TIM_OK, TIM_SetPwmDutyPermille(TIM3, 1U, 500U));
    ASSERT_EQ_U32(3400U, TIM3->CCR1);

    /* Error path verification */
    ASSERT_EQ_I32(TIM_ERR_INVALID_ARG, TIM_InitPeriodic(NULL, 100U, false, 0U));
    ASSERT_EQ_I32(TIM_ERR_INVALID_ARG, TIM_InitPeriodic(TIM2, 0U, false, 0U));
    ASSERT_EQ_I32(TIM_ERR_UNSUPPORTED_TIM, TIM_InitPeriodic(&foreign_tim, 100U, false, 0U));
    ASSERT_EQ_I32(TIM_ERR_UNSUPPORTED_TIM, TIM_InitPwm(TIM6, 1U, 1000U, 500U)); /* Basic TIM no PWM */
    ASSERT_EQ_I32(TIM_ERR_INVALID_ARG, TIM_InitPwm(TIM3, 0U, 1000U, 500U));
    ASSERT_EQ_I32(TIM_ERR_INVALID_ARG, TIM_InitPwm(TIM3, 5U, 1000U, 500U));
    ASSERT_EQ_I32(TIM_ERR_FREQ_OUT_OF_RANGE, TIM_InitPwm(TIM3, 1U, 200000000U, 500U));
    ASSERT_EQ_I32(TIM_ERR_UNSUPPORTED_TIM, TIM_SetPwmDutyPermille(TIM7, 1U, 500U));
    ASSERT_EQ_I32(TIM_ERR_INVALID_ARG, TIM_SetPwmCompare(TIM2, 5U, 100U));
}

/* ============================================================================
 * 3. USART / LPUART Register, Error-Flag & Ring-Buffer Tests (`usart_g4.c`)
 * ========================================================================== */
static void test_usart_registers_errors_and_rx(void) {
    mock_reset_all();

    /* USART2 @ 170 MHz, 115200 8N1, RX IRQ enabled prio 6 */
    ASSERT_EQ_I32(USART_OK, usart_init(USART2, 115200U, true, 6U));
    ASSERT_EQ_U32(RCC_APB1ENR1_USART2EN, RCC->APB1ENR1 & RCC_APB1ENR1_USART2EN);
    ASSERT_EQ_U32(1476U, USART2->BRR);
    ASSERT_EQ_U32(USART_CR1_TE | USART_CR1_RE | USART_CR1_RXNEIE_RXFNEIE | USART_CR1_UE,
                  USART2->CR1);
    ASSERT_EQ_U32(USART_ICR_PECF | USART_ICR_FECF | USART_ICR_NECF | USART_ICR_ORECF,
                  USART2->ICR);
    ASSERT_EQ_U32(1U, g_mock_core.enabled[USART2_IRQn]);
    ASSERT_EQ_U32(6U, g_mock_core.priority[USART2_IRQn]);

    /* USART1 @ 160 MHz with PPRE2=/2 (80 MHz PCLK2) -> BRR = round(80M/115200) = 694 */
    SystemCoreClock = 160000000UL;
    RCC->CFGR       = (4UL << RCC_CFGR_PPRE2_Pos);
    ASSERT_EQ_I32(USART_OK, usart_init(USART1, 115200U, false, 0U));
    ASSERT_EQ_U32(RCC_APB2ENR_USART1EN, RCC->APB2ENR & RCC_APB2ENR_USART1EN);
    ASSERT_EQ_U32(694U, USART1->BRR);
    ASSERT_EQ_U32(USART_CR1_TE | USART_CR1_RE | USART_CR1_UE, USART1->CR1);
    ASSERT_EQ_U32(0U, g_mock_core.enabled[USART1_IRQn]);

    /* LPUART1 @ 16 MHz -> BRR = round(256 * 16M / 115200) = 35556 */
    SystemCoreClock = 16000000UL;
    RCC->CFGR       = 0U;
    ASSERT_EQ_I32(USART_OK, usart_init(LPUART1, 115200U, true, 7U));
    ASSERT_EQ_U32(RCC_APB1ENR2_LPUART1EN, RCC->APB1ENR2 & RCC_APB1ENR2_LPUART1EN);
    ASSERT_EQ_U32(35556U, LPUART1->BRR);
    ASSERT_EQ_U32(1U, g_mock_core.enabled[LPUART1_IRQn]);

    /* TX write_byte / write_buf / write_str / flush_tx: ready vs timeout */
    USART2->ISR = USART_ISR_TXE_TXFNF | USART_ISR_TC;
    ASSERT_EQ_I32(USART_OK, usart_write_byte(USART2, 0x5AU, 10U));
    ASSERT_EQ_U32(0x5AU, USART2->TDR);
    const uint8_t frame[3] = { 'G', '4', '!' };
    ASSERT_EQ_I32(USART_OK, usart_write_buf(USART2, frame, 3U, 10U));
    ASSERT_EQ_U32((uint32_t)'!', USART2->TDR);
    ASSERT_EQ_I32(USART_OK, usart_write_str(USART2, "OK", 10U));
    ASSERT_EQ_U32((uint32_t)'K', USART2->TDR);
    ASSERT_EQ_I32(USART_OK, usart_flush_tx(USART2, 10U));

    USART2->ISR = 0U;
    ASSERT_EQ_I32(USART_ERR_TIMEOUT, usart_write_byte(USART2, 0xAAU, 5U));
    ASSERT_EQ_I32(USART_ERR_TIMEOUT, usart_flush_tx(USART2, 5U));

    /* Error flag detection & ICR clearing (ORE, FE, NE, PE) */
    USART2->ICR = 0U;
    USART2->ISR = USART_ISR_ORE | USART_ISR_FE;
    ASSERT_EQ_U32(USART_ERRFLAG_ORE | USART_ERRFLAG_FE, usart_clear_errors(USART2));
    ASSERT_EQ_U32(USART_ICR_ORECF | USART_ICR_FECF, USART2->ICR);

    USART2->ICR = 0U;
    USART2->ISR = USART_ISR_PE | USART_ISR_NE;
    ASSERT_EQ_U32(USART_ERRFLAG_PE | USART_ERRFLAG_NE, usart_clear_errors(USART2));
    ASSERT_EQ_U32(USART_ICR_PECF | USART_ICR_NECF, USART2->ICR);

    /* usart_rx_irq_handler clears ORE/FE/NE/PE + issues __DSB(); integrate with ring_buffer_t */
    uint8_t       rb_mem[16];
    ring_buffer_t rx_rb;
    ASSERT_TRUE(rb_init(&rx_rb, rb_mem, 16U));
    USART2->ICR = 0U;
    USART2->ISR = USART_ISR_ORE | USART_ISR_PE; /* RXNE=0 so host loop terminates */
    uint32_t dsb_before = g_mock_core.dsb_count;
    usart_rx_irq_handler(USART2, &rx_rb);
    ASSERT_EQ_U32(USART_ICR_ORECF | USART_ICR_PECF, USART2->ICR);
    ASSERT_EQ_U32(dsb_before + 1U, g_mock_core.dsb_count);

    /* Simulate byte arrival into RDR -> SPSC ring_buffer_t consumer path */
    USART2->RDR = (uint32_t)'s';
    ASSERT_TRUE(rb_push(&rx_rb, (uint8_t)(USART2->RDR & 0xFFU)));
    uint8_t rx_out = 0U;
    ASSERT_TRUE(rb_pop(&rx_rb, &rx_out));
    ASSERT_EQ_U32((uint32_t)'s', rx_out);

    /* Invalid args & baud range error */
    USART_TypeDef foreign_usart;
    memset(&foreign_usart, 0, sizeof(foreign_usart));
    ASSERT_EQ_I32(USART_ERR_INVALID_ARG, usart_init(NULL, 115200U, false, 0U));
    ASSERT_EQ_I32(USART_ERR_INVALID_ARG, usart_init(&foreign_usart, 115200U, false, 0U));
    ASSERT_EQ_I32(USART_ERR_BAUD_RANGE, usart_init(USART2, 5000000U, false, 0U)); /* 16M/5M < 16 */
    ASSERT_EQ_I32(USART_ERR_INVALID_ARG, usart_write_buf(USART2, NULL, 4U, 10U));
    ASSERT_EQ_I32(USART_ERR_INVALID_ARG, usart_write_str(USART2, NULL, 10U));
    ASSERT_EQ_U32(USART_ERRFLAG_NONE, usart_clear_errors(NULL));
    usart_rx_irq_handler(NULL, &rx_rb); /* Safe no-op */
}

/* ============================================================================
 * 4. SPI Master FRXTH 8-Bit FIFO, Mode & Prescaler Tests (`spi_g4.c`)
 * ========================================================================== */
static void test_spi_registers_and_transfer(void) {
    mock_reset_all();
    SystemCoreClock = 170000000UL;

    /* SPI1 Mode 3 @ max 10 MHz (170M / 32 = 5.3125 MHz -> BR=4) */
    ASSERT_EQ_I32(SPI_OK, spi_init_master(SPI1, SPI_MODE_3, 10000000U));
    ASSERT_EQ_U32(RCC_APB2ENR_SPI1EN, RCC->APB2ENR & RCC_APB2ENR_SPI1EN);
    ASSERT_EQ_U32(SPI_CR1_MSTR | SPI_CR1_SSM | SPI_CR1_SSI | (4UL << SPI_CR1_BR_Pos) |
                  SPI_CR1_CPOL | SPI_CR1_CPHA | SPI_CR1_SPE, SPI1->CR1);
    /* Critical STM32G4 check: DS = 0b0111 (8-bit) AND FRXTH = 1 (8-bit RXNE threshold) */
    ASSERT_EQ_U32((7UL << SPI_CR2_DS_Pos) | SPI_CR2_FRXTH, SPI1->CR2);

    /* SPI2 Mode 0 & SPI3 Mode 1/2 clock enables and CPOL/CPHA encodings */
    ASSERT_EQ_I32(SPI_OK, spi_init_master(SPI2, SPI_MODE_0, 22000000U)); /* /8 -> BR=2 */
    ASSERT_EQ_U32(RCC_APB1ENR1_SPI2EN, RCC->APB1ENR1 & RCC_APB1ENR1_SPI2EN);
    ASSERT_EQ_U32(0U, SPI2->CR1 & (SPI_CR1_CPOL | SPI_CR1_CPHA));
    ASSERT_EQ_U32(2U, (SPI2->CR1 & SPI_CR1_BR) >> SPI_CR1_BR_Pos);

    ASSERT_EQ_I32(SPI_OK, spi_init_master(SPI3, SPI_MODE_1, 85000000U)); /* /2 -> BR=0 */
    ASSERT_EQ_U32(RCC_APB1ENR1_SPI3EN, RCC->APB1ENR1 & RCC_APB1ENR1_SPI3EN);
    ASSERT_EQ_U32(SPI_CR1_CPHA, SPI3->CR1 & (SPI_CR1_CPOL | SPI_CR1_CPHA));

    ASSERT_EQ_I32(SPI_OK, spi_init_master(SPI3, SPI_MODE_2, 85000000U));
    ASSERT_EQ_U32(SPI_CR1_CPOL, SPI3->CR1 & (SPI_CR1_CPOL | SPI_CR1_CPHA));

    /* Full-duplex 8-bit DR loopback transfer + dummy TX (tx_buf=NULL -> 0xFF) */
    SPI1->SR = SPI_SR_TXE | SPI_SR_RXNE;
    uint8_t rx = 0U;
    ASSERT_EQ_I32(SPI_OK, spi_transfer_byte(SPI1, 0xA5U, &rx, 10U));
    ASSERT_EQ_U32(0xA5U, rx);

    const uint8_t tx_buf[4] = { 0x11U, 0x22U, 0x33U, 0x44U };
    uint8_t       rx_buf[4] = { 0U, 0U, 0U, 0U };
    ASSERT_EQ_I32(SPI_OK, spi_transfer_buf(SPI1, tx_buf, rx_buf, 4U, 10U));
    ASSERT_EQ_U32(0x11U, rx_buf[0]);
    ASSERT_EQ_U32(0x44U, rx_buf[3]);

    ASSERT_EQ_I32(SPI_OK, spi_transfer_buf(SPI1, NULL, rx_buf, 2U, 10U));
    ASSERT_EQ_U32(0xFFU, rx_buf[0]); /* Dummy byte 0xFF clocked when tx_buf == NULL */

    /* TXE timeout, RXNE timeout, and spi_wait_idle BSY/FTLVL guards */
    SPI1->SR = 0U;
    ASSERT_EQ_I32(SPI_ERR_TIMEOUT, spi_transfer_byte(SPI1, 0x55U, &rx, 5U));
    SPI1->SR = SPI_SR_TXE;
    ASSERT_EQ_I32(SPI_ERR_TIMEOUT, spi_transfer_byte(SPI1, 0x55U, &rx, 5U));

    SPI1->SR = 0U;
    ASSERT_EQ_I32(SPI_OK, spi_wait_idle(SPI1, 5U));
    SPI1->SR = SPI_SR_BSY;
    ASSERT_EQ_I32(SPI_ERR_TIMEOUT, spi_wait_idle(SPI1, 5U));
    SPI1->SR = (1UL << SPI_SR_FTLVL_Pos);
    ASSERT_EQ_I32(SPI_ERR_TIMEOUT, spi_wait_idle(SPI1, 5U));

    /* Invalid arguments */
    SPI_TypeDef foreign_spi;
    memset(&foreign_spi, 0, sizeof(foreign_spi));
    ASSERT_EQ_I32(SPI_ERR_INVALID_ARG, spi_init_master(NULL, SPI_MODE_0, 1000000U));
    ASSERT_EQ_I32(SPI_ERR_INVALID_ARG, spi_init_master(&foreign_spi, SPI_MODE_0, 1000000U));
    ASSERT_EQ_I32(SPI_ERR_INVALID_ARG, spi_init_master(SPI1, (spi_mode_t)4U, 1000000U));
    ASSERT_EQ_I32(SPI_ERR_INVALID_ARG, spi_transfer_byte(SPI1, 0U, &rx, 0U));
    ASSERT_EQ_I32(SPI_ERR_INVALID_ARG, spi_wait_idle(NULL, 5U));
}

/* ============================================================================
 * 5. I2C v2 TIMINGR, NACKF+TXE Flush, Repeated START & Bus Recovery (`i2c_g4.c`)
 * ========================================================================== */
static void test_i2c_registers_nack_and_recovery(void) {
    mock_reset_all();

    /* TIMINGR selection across 170 MHz, 160 MHz, and 16 MHz */
    SystemCoreClock = 170000000UL;
    ASSERT_EQ_I32(I2C_OK, i2c_init_master(I2C1, I2C_SPEED_400KHZ));
    ASSERT_EQ_U32(RCC_APB1ENR1_I2C1EN, RCC->APB1ENR1 & RCC_APB1ENR1_I2C1EN);
    ASSERT_EQ_U32(I2C_TIMINGR_170MHZ_400KHZ, I2C1->TIMINGR);
    ASSERT_EQ_U32(I2C_CR1_PE, I2C1->CR1);

    SystemCoreClock = 160000000UL;
    ASSERT_EQ_I32(I2C_OK, i2c_init_master(I2C2, I2C_SPEED_100KHZ));
    ASSERT_EQ_U32(RCC_APB1ENR1_I2C2EN, RCC->APB1ENR1 & RCC_APB1ENR1_I2C2EN);
    ASSERT_EQ_U32(I2C_TIMINGR_160MHZ_100KHZ, I2C2->TIMINGR);

    SystemCoreClock = 16000000UL;
    ASSERT_EQ_I32(I2C_OK, i2c_init_master(I2C3, I2C_SPEED_100KHZ));
    ASSERT_EQ_U32(RCC_APB1ENR1_I2C3EN, RCC->APB1ENR1 & RCC_APB1ENR1_I2C3EN);
    ASSERT_EQ_U32(I2C_TIMINGR_16MHZ_100KHZ, I2C3->TIMINGR);

    /* Unsupported clock (e.g. 80 MHz) rejected with I2C_ERR_UNSUP_CLK */
    SystemCoreClock = 80000000UL;
    ASSERT_EQ_I32(I2C_ERR_UNSUP_CLK, i2c_init_master(I2C1, I2C_SPEED_100KHZ));
    SystemCoreClock = 170000000UL;

    /* Normal write & write_reg + >255 B RELOAD chunking */
    I2C1->ISR = I2C_ISR_TXIS | I2C_ISR_STOPF | I2C_ISR_TCR;
    const uint8_t wr_payload[2] = { 0xAEU, 0xAFU };
    ASSERT_EQ_I32(I2C_OK, i2c_write_reg(I2C1, 0x3CU, 0x00U, wr_payload, 2U, 20U));
    ASSERT_EQ_U32(((0x3CUL << 1U) | (3UL << I2C_CR2_NBYTES_Pos) | I2C_CR2_AUTOEND | I2C_CR2_START),
                  I2C1->CR2);
    ASSERT_EQ_U32(0xAFU, I2C1->TXDR);
    ASSERT_EQ_U32(I2C_ICR_STOPCF, I2C1->ICR);

    uint8_t big_buf[300];
    memset(big_buf, 0x5AU, sizeof(big_buf));
    ASSERT_EQ_I32(I2C_OK, i2c_write(I2C1, 0x3CU, big_buf, sizeof(big_buf), 20U));
    /* Final chunk is 300 - 255 = 45 bytes with AUTOEND=1 and START=0 */
    ASSERT_EQ_U32(((0x3CUL << 1U) | (45UL << I2C_CR2_NBYTES_Pos) | I2C_CR2_AUTOEND), I2C1->CR2);

    /* CRITICAL: NACKF detection -> clears NACKCF/STOPCF AND flushes TXDR via ISR = I2C_ISR_TXE */
    I2C1->ISR = I2C_ISR_NACKF | I2C_ISR_STOPF;
    ASSERT_EQ_I32(I2C_ERR_NACK, i2c_write(I2C1, 0x3CU, wr_payload, 2U, 20U));
    ASSERT_EQ_U32(I2C_ISR_TXE, I2C1->ISR);    /* TXDR flush confirmed (RM0440 §39.7.7) */
    ASSERT_EQ_U32(I2C_ICR_STOPCF, I2C1->ICR);

    /* BUSY, BERR, ARLO, TIMEOUT fault paths */
    I2C1->ISR = I2C_ISR_BUSY;
    ASSERT_EQ_I32(I2C_ERR_BUSY, i2c_write(I2C1, 0x3CU, wr_payload, 1U, 5U));

    I2C1->ISR = I2C_ISR_BERR;
    ASSERT_EQ_I32(I2C_ERR_BUS, i2c_write(I2C1, 0x3CU, wr_payload, 1U, 5U));
    ASSERT_EQ_U32(I2C_ICR_BERRCF, I2C1->ICR);

    I2C1->ISR = I2C_ISR_ARLO;
    ASSERT_EQ_I32(I2C_ERR_ARLO, i2c_write(I2C1, 0x3CU, wr_payload, 1U, 5U));
    ASSERT_EQ_U32(I2C_ICR_ARLOCF, I2C1->ICR);

    I2C1->ISR = 0U;
    ASSERT_EQ_I32(I2C_ERR_TIMEOUT, i2c_write(I2C1, 0x3CU, wr_payload, 1U, 5U));

    /* Hardware Repeated START i2c_read_reg() (normal + >255 B RELOAD) */
    I2C1->ISR  = I2C_ISR_TXIS | I2C_ISR_TC | I2C_ISR_RXNE | I2C_ISR_STOPF | I2C_ISR_TCR;
    I2C1->RXDR = 0xD4U;
    uint8_t rd_buf[4] = { 0U };
    ASSERT_EQ_I32(I2C_OK, i2c_read_reg(I2C1, 0x68U, 0x75U, rd_buf, 4U, 20U));
    ASSERT_EQ_U32(0x75U, I2C1->TXDR);
    ASSERT_EQ_U32(0xD4U, rd_buf[0]);
    ASSERT_EQ_U32(0xD4U, rd_buf[3]);
    ASSERT_EQ_U32(I2C_CR2_RD_WRN | I2C_CR2_START | I2C_CR2_AUTOEND,
                  I2C1->CR2 & (I2C_CR2_RD_WRN | I2C_CR2_START | I2C_CR2_AUTOEND));

    /* 9-clock bus recovery (UM10204 §3.1.16): released SDA -> true; stuck LOW -> false */
    g_mock_delay_us_total = 0U;
    GPIOB->IDR            = (1UL << 7U); /* SDA already HIGH */
    ASSERT_TRUE(i2c_recover_bus_9clks(GPIOA, 15U, GPIOB, 7U));
    ASSERT_EQ_U32(1UL << 15U, GPIOA->OTYPER & (1UL << 15U)); /* Open-drain SCL */
    ASSERT_EQ_U32(1UL << 7U, GPIOB->OTYPER & (1UL << 7U));   /* Open-drain SDA */

    g_mock_delay_us_total = 0U;
    GPIOB->IDR            = 0U; /* SDA stuck LOW through all 9 clocks */
    ASSERT_TRUE(!i2c_recover_bus_9clks(GPIOA, 15U, GPIOB, 7U));
    ASSERT_EQ_U32(115U, g_mock_delay_us_total); /* 5 + 9*10 + 4*5 = 115 us */
    ASSERT_TRUE(!i2c_recover_bus_9clks(NULL, 15U, GPIOB, 7U));
    ASSERT_EQ_I32(I2C_ERR_INVALID_ARG, i2c_read_reg(I2C1, 0x80U, 0U, rd_buf, 1U, 10U));
}

/* ============================================================================
 * 6. ADC CKMODE=11, Startup Sequence & VREFINT/Temp Conversion (`adc_g4.c`)
 * ========================================================================== */
static void test_adc_registers_and_calibration(void) {
    mock_reset_all();

    /* adc_init_single_ended: verifies ADC12EN, CKMODE=11, DEEPPWD=0, ADVREGEN=1, 25us delay, ADCAL */
    adc_status_t st = adc_init_single_ended(ADC1, 10U);
    ASSERT_EQ_I32(ADC_ERR_TIMEOUT, st); /* Plain RAM does not self-clear ADCAL -> bounded timeout */
    ASSERT_EQ_U32(RCC_AHB2ENR_ADC12EN, RCC->AHB2ENR & RCC_AHB2ENR_ADC12EN);
    ASSERT_EQ_U32(3UL << ADC_CCR_CKMODE_Pos, ADC12_COMMON->CCR & ADC_CCR_CKMODE);
    ASSERT_EQ_U32(0U, ADC1->CR & ADC_CR_DEEPPWD);
    ASSERT_EQ_U32(ADC_CR_ADVREGEN, ADC1->CR & ADC_CR_ADVREGEN);
    ASSERT_EQ_U32(0U, ADC1->CR & ADC_CR_ADCALDIF);
    ASSERT_EQ_U32(ADC_CR_ADCAL, ADC1->CR & ADC_CR_ADCAL);
    ASSERT_EQ_U32(25U, g_mock_delay_us_total);

    /* Peer ADC active with mismatched CKMODE -> ADC_ERR_NOT_READY (RM0440 §21.7.17 guard) */
    mock_reset_all();
    ADC2->CR          = ADC_CR_ADEN;
    ADC12_COMMON->CCR = 0U; /* CKMODE != 0b11 */
    ASSERT_EQ_I32(ADC_ERR_NOT_READY, adc_init_single_ended(ADC1, 10U));

    /* ADDIS sequence when ADC is already enabled */
    mock_reset_all();
    ADC1->CR = ADC_CR_ADEN;
    ASSERT_EQ_I32(ADC_ERR_TIMEOUT, adc_init_single_ended(ADC1, 5U));
    ASSERT_EQ_U32(ADC_CR_ADDIS, ADC1->CR & ADC_CR_ADDIS);

    /* adc_read_channel: NOT_READY if ADEN=0; succeeds when ADEN=1 and masks 12-bit DR */
    mock_reset_all();
    uint16_t raw = 0U;
    ASSERT_EQ_I32(ADC_ERR_NOT_READY, adc_read_channel(ADC1, 1U, ADC_SMP_47_5_CYCLES, &raw, 10U));

    ADC1->CR   = ADC_CR_ADEN;
    ADC1->CFGR = ADC_CFGR_DMAEN | ADC_CFGR_DMACFG;
    ADC1->DR   = 0xF5E9U; /* Upper nibble must be masked to 12-bit 0x05E9 */
    ASSERT_EQ_I32(ADC_OK, adc_read_channel(ADC1, 5U, ADC_SMP_47_5_CYCLES, &raw, 10U));
    ASSERT_EQ_U32(0x05E9U, raw);
    ASSERT_EQ_U32(0U, ADC1->CFGR & (ADC_CFGR_DMAEN | ADC_CFGR_DMACFG));
    ASSERT_EQ_U32((uint32_t)ADC_SMP_47_5_CYCLES, (ADC1->SMPR1 >> 15U) & 0x7U);
    ASSERT_EQ_U32(5UL << ADC_SQR1_SQ1_Pos, ADC1->SQR1);
    ASSERT_EQ_U32(ADC_CR_ADSTART, ADC1->CR & ADC_CR_ADSTART);

    /* Internal VREFINT (ch 18, VREFEN + 20 us) & Temp Sensor (ch 16, VSENSESEL + 125 us) */
    g_mock_delay_us_total = 0U;
    ADC1->DR              = 1513U;
    ASSERT_EQ_I32(ADC_OK, adc_read_vrefint_raw(&raw, 10U));
    ASSERT_EQ_U32(1513U, raw);
    ASSERT_EQ_U32(ADC_CCR_VREFEN, ADC12_COMMON->CCR & ADC_CCR_VREFEN);
    ASSERT_EQ_U32(20U, g_mock_delay_us_total);
    ASSERT_EQ_U32(7U, (ADC1->SMPR2 >> ((18U - 10U) * 3U)) & 0x7U); /* 640.5 cycles */

    g_mock_delay_us_total = 0U;
    ADC1->DR              = 956U;
    ASSERT_EQ_I32(ADC_OK, adc_read_temp_raw(&raw, 10U));
    ASSERT_EQ_U32(956U, raw);
    ASSERT_EQ_U32(ADC_CCR_VSENSESEL, ADC12_COMMON->CCR & ADC_CCR_VSENSESEL);
    ASSERT_EQ_U32(125U, g_mock_delay_us_total);
    ASSERT_EQ_U32(7U, (ADC1->SMPR2 >> ((16U - 10U) * 3U)) & 0x7U); /* 640.5 cycles */

    /* Real NUCLEO-G474RE silicon calibration verification (VREF_CAL=1657, TS_CAL1=1037, TS_CAL2=1370) */
    uint32_t vdda_mv = adc_calc_vdda_mv(1513U, 1657U);
    ASSERT_EQ_U32(3286U, vdda_mv);
    ASSERT_EQ_U32(3288U, adc_calc_vdda_mv(1512U, 1657U));
    ASSERT_EQ_I32(32906, adc_temp_mdegc(955U, 1037U, 1370U, 3288U));
    ASSERT_EQ_I32(33236, adc_temp_mdegc(956U, 1037U, 1370U, 3288U));

    /* Invalid args */
    ASSERT_EQ_I32(ADC_ERR_INVALID_ARG, adc_init_single_ended(NULL, 10U));
    ASSERT_EQ_I32(ADC_ERR_INVALID_ARG, adc_read_channel(ADC1, 19U, ADC_SMP_2_5_CYCLES, &raw, 10U));
    ASSERT_EQ_I32(ADC_ERR_INVALID_ARG, adc_read_vrefint_raw(NULL, 10U));
    ASSERT_EQ_I32(ADC_ERR_INVALID_ARG, adc_read_temp_raw(NULL, 10U));
}

/* ============================================================================
 * 7. DMA / DMAMUX1 Channel Indexing & ES0430 Flag Masks (`dma_g4.c`)
 * ========================================================================== */
static void test_dma_registers_dmamux_and_es0430(void) {
    mock_reset_all();

    /* Clock enables include DMAMUX1EN alongside DMA1EN / DMA2EN */
    dma_enable_clocks(DMA1);
    ASSERT_EQ_U32(RCC_AHB1ENR_DMA1EN | RCC_AHB1ENR_DMAMUX1EN, RCC->AHB1ENR);
    dma_enable_clocks(DMA2);
    ASSERT_EQ_U32(RCC_AHB1ENR_DMA1EN | RCC_AHB1ENR_DMA2EN | RCC_AHB1ENR_DMAMUX1EN, RCC->AHB1ENR);

    /* ES0430 §2.3.1: dma_clear_flags MUST ONLY write CTCIF(+1)/CHTIF(+2)/CTEIF(+3), NEVER CGIF(+0)! */
    uint32_t dsb_before = g_mock_core.dsb_count;
    dma_clear_flags(DMA1, 1U, true, false, false);
    ASSERT_EQ_U32(1UL << 1U, DMA1->IFCR);
    ASSERT_EQ_U32(0U, DMA1->IFCR & (1UL << 0U)); /* CGIF1 MUST be 0! */

    dma_clear_flags(DMA1, 3U, true, true, true);
    ASSERT_EQ_U32((1UL << 9U) | (1UL << 10U) | (1UL << 11U), DMA1->IFCR);
    ASSERT_EQ_U32(0U, DMA1->IFCR & (1UL << 8U)); /* CGIF3 MUST be 0! */

    dma_clear_flags(DMA2, 8U, false, true, true);
    ASSERT_EQ_U32((1UL << 30U) | (1UL << 31U), DMA2->IFCR);
    ASSERT_EQ_U32(0U, DMA2->IFCR & (1UL << 28U)); /* CGIF8 MUST be 0! */
    ASSERT_EQ_U32(dsb_before + 3U, g_mock_core.dsb_count);

    /* Circular P2M setup: DMA1 Ch1 -> DMAMUX1_Ch0 (ADC1 req=5) & DMA2 Ch8 -> DMAMUX1_Ch15 */
    uint16_t adc_buf[16];
    ASSERT_EQ_I32(DMA_OK, dma_config_p2m_circular(DMA1, 1U, DMAMUX_REQ_ADC1,
                                                  &ADC1->DR, adc_buf, 16U, 2U, true, 4U));
    ASSERT_EQ_U32(DMAMUX_REQ_ADC1, DMAMUX1_Channel0->CCR);
    ASSERT_EQ_U32(16U, DMA1_Channel1->CNDTR);
    ASSERT_EQ_U32(DMA_CCR_CIRC | DMA_CCR_MINC | (1UL << DMA_CCR_PSIZE_Pos) |
                  (1UL << DMA_CCR_MSIZE_Pos) | DMA_CCR_HTIE | DMA_CCR_TCIE |
                  DMA_CCR_TEIE | DMA_CCR_EN, DMA1_Channel1->CCR);
    ASSERT_EQ_U32(1U, g_mock_core.enabled[DMA1_Channel1_IRQn]);
    ASSERT_EQ_U32(4U, g_mock_core.priority[DMA1_Channel1_IRQn]);

    uint8_t uart_buf[32];
    ASSERT_EQ_I32(DMA_OK, dma_config_p2m_circular(DMA2, 8U, DMAMUX_REQ_USART2_RX,
                                                  &USART2->RDR, uart_buf, 32U, 1U, true, 5U));
    ASSERT_EQ_U32(DMAMUX_REQ_USART2_RX, DMAMUX1_Channel15->CCR);
    ASSERT_EQ_U32(1U, g_mock_core.enabled[DMA2_Channel8_IRQn]);

    /* MEM2MEM sync: TCIF -> DMA_OK, TEIF -> DMA_ERR_TRANSFER, ISR=0 -> DMA_ERR_TIMEOUT */
    uint32_t src32[4] = { 1U, 2U, 3U, 4U };
    uint32_t dst32[4] = { 0U, 0U, 0U, 0U };

    DMA1->ISR = (1UL << 5U); /* Ch2 TCIF2 */
    ASSERT_EQ_I32(DMA_OK, dma_mem2mem_sync(DMA1, 2U, src32, dst32, 4U, 4U, 10U));
    ASSERT_EQ_U32(0U, DMA1_Channel2->CCR);  /* Channel disabled cleanly after exit */
    ASSERT_EQ_U32(0xE0UL, DMA1->IFCR);      /* Individual CTCIF2|CHTIF2|CTEIF2, CGIF2=0! */

    DMA1->ISR = (1UL << 7U); /* Ch2 TEIF2 */
    ASSERT_EQ_I32(DMA_ERR_TRANSFER, dma_mem2mem_sync(DMA1, 2U, src32, dst32, 4U, 4U, 10U));

    DMA1->ISR = 0U;
    ASSERT_EQ_I32(DMA_ERR_TIMEOUT, dma_mem2mem_sync(DMA1, 2U, src32, dst32, 4U, 4U, 5U));

    /* Alignment & invalid argument rejection */
    uint8_t unaligned_buf[8];
    ASSERT_EQ_I32(DMA_ERR_INVALID_ARG,
                  dma_mem2mem_sync(DMA1, 2U, &unaligned_buf[1], dst32, 2U, 2U, 10U));
    ASSERT_EQ_I32(DMA_ERR_INVALID_ARG,
                  dma_config_p2m_circular(DMA1, 9U, DMAMUX_REQ_ADC1, &ADC1->DR, adc_buf, 4U, 2U, false, 0U));
    ASSERT_EQ_I32(DMA_ERR_INVALID_ARG,
                  dma_config_p2m_circular(DMA1, 1U, DMAMUX_REQ_ADC1, &ADC1->DR, adc_buf, 4U, 3U, false, 0U));
}

/* ============================================================================
 * 8. IWDG Key Sequence, Timeout & Reset Cause Tests (`iwdg_g4.c`)
 * ========================================================================== */
static void test_iwdg_registers_and_keys(void) {
    mock_reset_all();

    /* Normal 2000 ms init with DBGMCU freeze enabled */
    RCC->CSR = RCC_CSR_LSIRDY;
    IWDG->SR = 0U;
    ASSERT_EQ_I32(IWDG_OK, iwdg_init_ms(2000U, true, 100U));
    ASSERT_EQ_U32(DBGMCU_APB1FZR1_DBG_IWDG_STOP, DBGMCU->APB1FZR1);
    ASSERT_EQ_U32(RCC_CSR_LSION, RCC->CSR & RCC_CSR_LSION);
    ASSERT_EQ_U32(2U, IWDG->PR);     /* /16 */
    ASSERT_EQ_U32(3999U, IWDG->RLR); /* 4000 ticks @ 2 kHz = 2000 ms */
    ASSERT_EQ_U32(IWDG_KEY_RELOAD, IWDG->KR); /* Final reload key 0xAAAA */

    /* Init with freeze_in_debug = false leaves DBGMCU untouched */
    mock_reset_all();
    RCC->CSR = RCC_CSR_LSIRDY;
    ASSERT_EQ_I32(IWDG_OK, iwdg_init_ms(500U, false, 100U));
    ASSERT_EQ_U32(0U, DBGMCU->APB1FZR1);
    ASSERT_EQ_U32(0U, IWDG->PR);     /* /4 */
    ASSERT_EQ_U32(3999U, IWDG->RLR);

    /* Timeout waiting for PVU/RVU sync verifies unlock key 0x5555 was written */
    mock_reset_all();
    RCC->CSR = RCC_CSR_LSIRDY;
    IWDG->SR = IWDG_SR_PVU;
    ASSERT_EQ_I32(IWDG_ERR_TIMEOUT, iwdg_init_ms(1000U, false, 5U));
    ASSERT_EQ_U32(IWDG_KEY_WRITE_ACCESS, IWDG->KR); /* 0x5555 unlock verified */

    /* Timeout waiting for LSI ready */
    mock_reset_all();
    RCC->CSR = 0U;
    ASSERT_EQ_I32(IWDG_ERR_TIMEOUT, iwdg_init_ms(1000U, false, 5U));

    /* iwdg_kick(), iwdg_was_reset_cause(), iwdg_clear_reset_flags() */
    IWDG->KR = 0U;
    iwdg_kick();
    ASSERT_EQ_U32(IWDG_KEY_RELOAD, IWDG->KR);

    RCC->CSR = 0U;
    ASSERT_TRUE(!iwdg_was_reset_cause());
    RCC->CSR = RCC_CSR_IWDGRSTF;
    ASSERT_TRUE(iwdg_was_reset_cause());
    iwdg_clear_reset_flags();
    ASSERT_EQ_U32(0U, RCC->CSR & RCC_CSR_RMVF);

    /* Invalid arguments */
    ASSERT_EQ_I32(IWDG_ERR_INVALID_ARG, iwdg_init_ms(0U, true, 100U));
    ASSERT_EQ_I32(IWDG_ERR_INVALID_ARG, iwdg_init_ms(40000U, true, 100U));
    ASSERT_EQ_I32(IWDG_ERR_INVALID_ARG, iwdg_init_ms(1000U, true, 0U));
}

int main(void) {
    printf("Running STM32G474RE 8-Peripheral Register-Mock Functional Unit Tests...\n");
    test_exti_registers_and_dispatch();
    test_tim_registers_and_pwm();
    test_usart_registers_errors_and_rx();
    test_spi_registers_and_transfer();
    test_i2c_registers_nack_and_recovery();
    test_adc_registers_and_calibration();
    test_dma_registers_dmamux_and_es0430();
    test_iwdg_registers_and_keys();

    printf("Results: %d assertions executed, %d failed.\n", g_tests_run, g_tests_failed);
    return (g_tests_failed == 0) ? 0 : 1;
}
