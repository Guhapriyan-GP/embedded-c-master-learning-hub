/**
 * @file    main.c
 * @brief   NUCLEO-G474RE Safe Onboard Hardware Self-Test Suite (9 Tests).
 *
 * Self-contained physical silicon verification using ONLY onboard peripherals:
 *   - PA5  (LD2 Green LED)
 *   - PC13 (B1 Blue Pushbutton, Active-HIGH idle read + EXTI13 SWIER)
 *   - USART2 ST-LINK VCP (PA2/PA3 AF7, 115200 8N1)
 *   - Internal silicon: RCC/PWR/FLASH (160 MHz & 170 MHz), SysTick, DWT->CYCCNT,
 *     TIM2, EXTI13, DMA1 + DMAMUX1, ADC1 (VREFINT + Temp Sensor), CORDIC, FMAC,
 *     CRC-32, FDCAN1 (Internal Loopback), Cortex-M4 MPU + .noinit UsageFault
 *     persistence across NVIC_SystemReset(), and IWDG safe kick verification.
 *
 * Zero external wires, zero Flash option-byte writes, zero SWD reconfiguration.
 */

#include "stm32g474xx.h"

#include "adc_g4.h"
#include "clock_g4.h"
#include "dma_g4.h"
#include "exti_g4.h"
#include "fault_handler.h"
#include "gpio_g4.h"
#include "iwdg_g4.h"
#include "tim_g4.h"
#include "usart_g4.h"

#include <math.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

/* -------------------------------------------------------------------------- */
/* Constants & Fixed Hardware Offsets (verified against stm32g474xx.h/RM0440) */
/* -------------------------------------------------------------------------- */
#define UART_BAUD_RATE          115200U
#define UART_TX_TIMEOUT         500000U
#define PERIPH_TIMEOUT          250000U

#define FDCAN1_RAM_BASE         (SRAMCAN_BASE)            /* 0x4000A400 */
#define FDCAN1_RX_FIFO0_BASE    (FDCAN1_RAM_BASE + 0x0B0UL)
#define FDCAN1_TX_BUF_BASE      (FDCAN1_RAM_BASE + 0x278UL)
#define FDCAN_ELEM_STRIDE_U32   (18U)                     /* 72 B = 18 words */

#define CORDIC_BENCH_N          (256U)
#define FMAC_FIR_TAPS           (8U)
#define FMAC_FIR_SAMPLES        (32U)
#define ADC_DMA_SAMPLES         (32U)

#define SELFTEST_P1_MAGIC       (0x53543031UL)

/* -------------------------------------------------------------------------- */
/* Shared Volatile / .noinit State                                            */
/* -------------------------------------------------------------------------- */
__attribute__((section(".noinit"), used))
static volatile uint32_t g_p1_magic;

__attribute__((section(".noinit"), used))
static volatile uint32_t g_p1_failures;

__attribute__((aligned(32)))
static volatile uint8_t g_mpu_guard_zone[32];

static volatile uint32_t g_mpu_valid_var     = 0x12345678UL;
static volatile uint32_t g_tim2_irq_count    = 0U;
static volatile uint32_t g_exti13_cb_count   = 0U;
static volatile uint8_t  g_exti13_last_line  = 0xFFU;

static volatile uint16_t g_adc_dma_buf[ADC_DMA_SAMPLES];
static volatile uint32_t g_dma_ht_count      = 0U;
static volatile uint32_t g_dma_tc_count      = 0U;
static volatile uint32_t g_dma_te_count      = 0U;

/* -------------------------------------------------------------------------- */
/* Formatted UART Helper (Integer-only snprintf, zero heap / zero float fmt)  */
/* -------------------------------------------------------------------------- */
static void uart_printf(const char *fmt, ...) {
    char buf[256];
    va_list ap;
    va_start(ap, fmt);
    int n = vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);

    if (n > 0) {
        size_t len = (size_t)n;
        if (len >= sizeof(buf)) {
            len = sizeof(buf) - 1U;
        }
        (void)usart_write_buf(USART2, (const uint8_t *)buf, len, UART_TX_TIMEOUT);
    }
}

static void uart2_pins_and_init(void) {
    GPIO_EnableClock(GPIOA);
    (void)GPIO_ConfigAF(GPIOA, 2U, 7U, GPIO_OTYPE_PUSHPULL, GPIO_SPEED_MEDIUM, GPIO_PULL_NONE);
    (void)GPIO_ConfigAF(GPIOA, 3U, 7U, GPIO_OTYPE_PUSHPULL, GPIO_SPEED_MEDIUM, GPIO_PULL_UP);
    (void)usart_init(USART2, UART_BAUD_RATE, false, 0U);
}

/* -------------------------------------------------------------------------- */
/* Interrupt Handlers (TIM2, EXTI15_10, DMA1_Channel1)                        */
/* -------------------------------------------------------------------------- */
void TIM2_IRQHandler(void) {
    if ((TIM2->SR & TIM_SR_UIF) != 0U) {
        TIM_ClearUpdateFlag(TIM2);
        g_tim2_irq_count++;
    }
}

static void on_exti13_callback(uint8_t line, void *ctx) {
    (void)ctx;
    g_exti13_last_line = line;
    g_exti13_cb_count++;
}

void EXTI15_10_IRQHandler(void) {
    EXTI_DispatchRange(10U, 15U);
}

void DMA1_Channel1_IRQHandler(void) {
    uint32_t isr = DMA1->ISR;
    bool ht = ((isr & DMA_ISR_HTIF1) != 0U);
    bool tc = ((isr & DMA_ISR_TCIF1) != 0U);
    bool te = ((isr & DMA_ISR_TEIF1) != 0U);

    if (ht) {
        g_dma_ht_count++;
    }
    if (tc) {
        g_dma_tc_count++;
    }
    if (te) {
        g_dma_te_count++;
    }
    /* ES0430 §2.3.1: clear ONLY individual flags via dma_clear_flags() */
    dma_clear_flags(DMA1, 1U, tc, ht, te);
}

/* -------------------------------------------------------------------------- */
/* Controlled UsageFault Handler Override for Automated Test 8 Reboot Cycle   */
/* Overrides weak UsageFault_Handler in startup_stm32g474re.c so Test 8's     */
/* deliberate DIV0 persists into fault_handler.c's .noinit g_crash_log and    */
/* performs NVIC_SystemReset() (HardFault_Handler_C in fault_handler.c stays  */
/* unmodified for post-mortem debugger halt).                                 */
/* -------------------------------------------------------------------------- */
static bool is_valid_sram_sp(uintptr_t sp_addr) {
    if ((sp_addr & 3UL) != 0UL) {
        return false;
    }
    bool in_sram = (sp_addr >= (uintptr_t)SRAM1_BASE) &&
                   (sp_addr <= (uintptr_t)(SRAM2_BASE + SRAM2_SIZE - 32UL));
    bool in_ccm  = (sp_addr >= (uintptr_t)CCMSRAM_BASE) &&
                   (sp_addr <= (uintptr_t)(CCMSRAM_BASE + CCMSRAM_SIZE - 32UL));
    return (in_sram || in_ccm);
}

__attribute__((used))
void Selftest_UsageFault_C(uint32_t *stacked_frame) {
    uint32_t  cfsr    = SCB->CFSR;
    uint32_t  hfsr    = SCB->HFSR;
    uint32_t  flags   = FAULT_FLAG_NONE;
    uintptr_t sp_addr = (uintptr_t)stacked_frame;

    g_crash_log.cfsr       = cfsr;
    g_crash_log.hfsr       = hfsr;
    g_crash_log.dfsr       = SCB->DFSR;
    g_crash_log.afsr       = SCB->AFSR;
    g_crash_log.mmfar      = SCB->MMFAR;
    g_crash_log.bfar       = SCB->BFAR;
    g_crash_log.stacked_sp = (uint32_t)sp_addr;

    if (is_valid_sram_sp(sp_addr)) {
        g_crash_log.r0   = stacked_frame[0];
        g_crash_log.r1   = stacked_frame[1];
        g_crash_log.r2   = stacked_frame[2];
        g_crash_log.r3   = stacked_frame[3];
        g_crash_log.r12  = stacked_frame[4];
        g_crash_log.lr   = stacked_frame[5];
        g_crash_log.pc   = stacked_frame[6];
        g_crash_log.xpsr = stacked_frame[7];
    } else {
        g_crash_log.pc = 0xDEAD57ACUL;
    }

    if ((cfsr & SCB_CFSR_MEMFAULTSR_Msk) != 0U) {
        flags |= FAULT_FLAG_MEMMANAGE;
        if ((cfsr & SCB_CFSR_MMARVALID_Msk) != 0U) {
            flags |= FAULT_FLAG_MMFAR_VALID;
        }
    }
    if ((cfsr & SCB_CFSR_BUSFAULTSR_Msk) != 0U) {
        flags |= FAULT_FLAG_BUSFAULT;
        if ((cfsr & SCB_CFSR_BFARVALID_Msk) != 0U) {
            flags |= FAULT_FLAG_BFAR_VALID;
        }
    }
    if ((cfsr & SCB_CFSR_USGFAULTSR_Msk) != 0U) {
        flags |= FAULT_FLAG_USAGEFAULT;
        if ((cfsr & SCB_CFSR_DIVBYZERO_Msk) != 0U) {
            flags |= FAULT_FLAG_DIVBYZERO;
        }
    }
    if ((hfsr & SCB_HFSR_FORCED_Msk) != 0U) {
        flags |= FAULT_FLAG_FORCED_HF;
    }

    g_crash_log.fault_flags = flags;
    __DSB();
    g_crash_log.magic = CRASH_LOG_MAGIC;
    __DSB();

    /* Clean software reset to verify .noinit persistence on Phase 2 reboot */
    NVIC_SystemReset();
    for (;;) {
        __asm__ volatile("nop");
    }
}

__attribute__((naked))
void UsageFault_Handler(void) {
    __asm__ volatile(
        "tst   lr, #4                \n\t"
        "ite   eq                    \n\t"
        "mrseq r0, msp               \n\t"
        "mrsne r0, psp               \n\t"
        "b     Selftest_UsageFault_C \n\t"
    );
}

/* -------------------------------------------------------------------------- */
/* Helper: Measure 10 ms SysTick ticks against DWT->CYCCNT                    */
/* -------------------------------------------------------------------------- */
static uint32_t measure_10ms_systick_dwt_cycles(uint32_t sysclk_hz) {
    uint32_t reload = (sysclk_hz / 1000U) - 1U;
    SysTick->CTRL = 0U;
    SysTick->LOAD = reload;
    SysTick->VAL  = 0U;
    SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_ENABLE_Msk;

    /* Align on first tick boundary */
    while ((SysTick->CTRL & SysTick_CTRL_COUNTFLAG_Msk) == 0U) {}

    uint32_t t0 = Clock_GetCycles();
    for (uint32_t ms = 0U; ms < 10U; ++ms) {
        while ((SysTick->CTRL & SysTick_CTRL_COUNTFLAG_Msk) == 0U) {}
    }
    uint32_t elapsed = Clock_GetCycles() - t0;

    SysTick->CTRL = 0U;
    return elapsed;
}

/* -------------------------------------------------------------------------- */
/* Helper: Hardware IEEE 802.3 CRC-32                                         */
/* -------------------------------------------------------------------------- */
static uint32_t crc32_hw_ieee8023(const uint8_t *data, uint32_t len) {
    RCC->AHB1ENR |= RCC_AHB1ENR_CRCEN;
    (void)RCC->AHB1ENR;

    CRC->POL  = 0x04C11DB7UL;
    CRC->INIT = 0xFFFFFFFFUL;
    CRC->CR   = CRC_CR_REV_IN_0 | CRC_CR_REV_OUT | CRC_CR_RESET;

    for (uint32_t i = 0U; i < len; ++i) {
        *(__IO uint8_t *)&CRC->DR = data[i];
    }
    return CRC->DR ^ 0xFFFFFFFFUL;
}

/* -------------------------------------------------------------------------- */
/* Individual Onboard Tests (1 .. 9)                                          */
/* -------------------------------------------------------------------------- */
static bool run_test1_mcu_id(void) {
    uint32_t idcode   = DBGMCU->IDCODE;
    uint32_t dev_id   = idcode & DBGMCU_IDCODE_DEV_ID_Msk;
    uint32_t rev_id   = (idcode & DBGMCU_IDCODE_REV_ID_Msk) >> DBGMCU_IDCODE_REV_ID_Pos;
    uint32_t cpuid    = SCB->CPUID;
    uint16_t flash_kb = *(const volatile uint16_t *)(uintptr_t)FLASHSIZE_BASE;
    uint32_t rcc_csr  = RCC->CSR;

    bool ok = (dev_id == 0x469UL) && ((cpuid & 0xFF0FFFF0UL) == 0x410FC240UL) && (flash_kb == 512U);
    uart_printf("[%s] TEST1_MCU_ID: DBGMCU_IDCODE=0x%08lX (DEV=0x%03lX REV=0x%04lX) "
                "SCB_CPUID=0x%08lX FLASH_SIZE=%uKB RCC_CSR=0x%08lX\r\n",
                ok ? "PASS" : "FAIL",
                (unsigned long)idcode, (unsigned long)dev_id, (unsigned long)rev_id,
                (unsigned long)cpuid, (unsigned)flash_kb, (unsigned long)rcc_csr);
    return ok;
}

static bool run_test2_gpio(void) {
    GPIO_EnableClock(GPIOA);
    GPIO_EnableClock(GPIOC);

    bool cfg_a = GPIO_ConfigOutput(GPIOA, 5U, GPIO_OTYPE_PUSHPULL, GPIO_SPEED_LOW, GPIO_PULL_NONE);
    bool cfg_c = GPIO_ConfigInput(GPIOC, 13U, GPIO_PULL_NONE);

    GPIO_SetPin(GPIOA, 5U);
    __DSB();
    bool pa5_hi = GPIO_ReadPin(GPIOA, 5U);

    GPIO_ResetPin(GPIOA, 5U);
    __DSB();
    bool pa5_lo = GPIO_ReadPin(GPIOA, 5U);

    GPIO_TogglePin(GPIOA, 5U);
    __DSB();
    bool pa5_tog = GPIO_ReadPin(GPIOA, 5U);
    GPIO_ResetPin(GPIOA, 5U);

    uint32_t pc13_moder = (GPIOC->MODER >> 26U) & 0x3UL;
    bool pc13_idle = GPIO_ReadPin(GPIOC, 13U);

    bool ok = cfg_a && cfg_c && pa5_hi && !pa5_lo && pa5_tog && (pc13_moder == 0U) && !pc13_idle;
    uart_printf("[%s] TEST2_GPIO_LD2_B1: PA5(set=%u rst=%u tog=%u) "
                "GPIOA_MODER=0x%08lX PC13_MODER=%lu PC13_IDR=%u GPIOC_MODER=0x%08lX\r\n",
                ok ? "PASS" : "FAIL",
                pa5_hi ? 1U : 0U, pa5_lo ? 1U : 0U, pa5_tog ? 1U : 0U,
                (unsigned long)GPIOA->MODER, (unsigned long)pc13_moder,
                pc13_idle ? 1U : 0U, (unsigned long)GPIOC->MODER);
    return ok;
}

static bool run_test3_clocks(void) {
    /* --- 3A: 160 MHz Range 1 Boost --- */
    (void)usart_flush_tx(USART2, UART_TX_TIMEOUT);
    bool init160 = Clock_Init_160MHz();
    uart2_pins_and_init();
    Clock_DWT_Init();

    uint32_t cyc160 = measure_10ms_systick_dwt_cycles(160000000UL);
    bool ok160 = init160 &&
                 (Clock_GetSysClkHz() == 160000000UL) &&
                 ((PWR->CR5 & PWR_CR5_R1MODE) == 0U) &&
                 ((FLASH->ACR & FLASH_ACR_LATENCY) == FLASH_ACR_LATENCY_4WS) &&
                 (cyc160 >= 1590000UL) && (cyc160 <= 1610000UL);

    uart_printf("[%s] TEST3A_CLOCK_160MHZ: SYSCLK=%luHz RCC_CR=0x%08lX PLLCFGR=0x%08lX "
                "CFGR=0x%08lX PWR_CR1=0x%08lX PWR_CR5=0x%08lX PWR_SR2=0x%08lX "
                "FLASH_ACR=0x%08lX USART2_BRR=%lu DWT_10ms=%lu\r\n",
                ok160 ? "PASS" : "FAIL",
                (unsigned long)Clock_GetSysClkHz(),
                (unsigned long)RCC->CR, (unsigned long)RCC->PLLCFGR, (unsigned long)RCC->CFGR,
                (unsigned long)PWR->CR1, (unsigned long)PWR->CR5, (unsigned long)PWR->SR2,
                (unsigned long)FLASH->ACR, (unsigned long)USART2->BRR, (unsigned long)cyc160);

    /* --- 3B: 170 MHz Range 1 Boost --- */
    (void)usart_flush_tx(USART2, UART_TX_TIMEOUT);
    bool init170 = Clock_Init_170MHz();
    uart2_pins_and_init();
    Clock_DWT_Init();

    uint32_t cyc170 = measure_10ms_systick_dwt_cycles(170000000UL);
    bool ok170 = init170 &&
                 (Clock_GetSysClkHz() == 170000000UL) &&
                 ((PWR->CR5 & PWR_CR5_R1MODE) == 0U) &&
                 ((FLASH->ACR & FLASH_ACR_LATENCY) == FLASH_ACR_LATENCY_4WS) &&
                 (cyc170 >= 1690000UL) && (cyc170 <= 1710000UL);

    uart_printf("[%s] TEST3B_CLOCK_170MHZ: SYSCLK=%luHz RCC_CR=0x%08lX PLLCFGR=0x%08lX "
                "CFGR=0x%08lX PWR_CR1=0x%08lX PWR_CR5=0x%08lX PWR_SR2=0x%08lX "
                "FLASH_ACR=0x%08lX USART2_BRR=%lu DWT_10ms=%lu\r\n",
                ok170 ? "PASS" : "FAIL",
                (unsigned long)Clock_GetSysClkHz(),
                (unsigned long)RCC->CR, (unsigned long)RCC->PLLCFGR, (unsigned long)RCC->CFGR,
                (unsigned long)PWR->CR1, (unsigned long)PWR->CR5, (unsigned long)PWR->SR2,
                (unsigned long)FLASH->ACR, (unsigned long)USART2->BRR, (unsigned long)cyc170);

    return ok160 && ok170;
}

static bool run_test4_tim2_exti13(void) {
    g_tim2_irq_count   = 0U;
    g_exti13_cb_count  = 0U;
    g_exti13_last_line = 0xFFU;

    tim_status_t tst = TIM_InitPeriodic(TIM2, 100U, true, 4U);
    uint32_t tim2_psc = TIM2->PSC;
    uint32_t tim2_arr = TIM2->ARR;
    Clock_DelayMs(100U);
    uint32_t irq_cnt = g_tim2_irq_count;

    TIM2->CR1 &= ~TIM_CR1_CEN;
    TIM2->DIER &= ~TIM_DIER_UIE;
    NVIC_DisableIRQ(TIM2_IRQn);

    exti_status_t est1 = EXTI_RegisterCallback(13U, on_exti13_callback, NULL);
    exti_status_t est2 = EXTI_ConfigLine(GPIOC, 13U, EXTI_EDGE_RISING, 5U);
    for (uint32_t i = 0U; i < 3U; ++i) {
        EXTI_TriggerSoftware(13U);
        Clock_DelayMs(1U);
    }
    uint32_t exti_cnt  = g_exti13_cb_count;
    uint8_t  exti_line = g_exti13_last_line;
    (void)EXTI_DisableLine(13U);

    bool ok = (tst == TIM_OK) && (irq_cnt >= 9U) && (irq_cnt <= 11U) &&
              (est1 == EXTI_OK) && (est2 == EXTI_OK) &&
              (exti_cnt == 3U) && (exti_line == 13U);

    uart_printf("[%s] TEST4_TIM2_AND_EXTI13: TIM2(PSC=%lu ARR=%lu irqs_100ms=%lu) "
                "EXTI13(swier_cb=%lu last_line=%u SYSCFG_EXTICR4=0x%08lX)\r\n",
                ok ? "PASS" : "FAIL",
                (unsigned long)tim2_psc, (unsigned long)tim2_arr, (unsigned long)irq_cnt,
                (unsigned long)exti_cnt, (unsigned)exti_line,
                (unsigned long)SYSCFG->EXTICR[3]);
    return ok;
}

static bool run_test5_dma_and_adc(void) {
    /* --- 5A: DMA1 Mem-to-Mem 256 words --- */
    static uint32_t dma_src[256];
    static uint32_t dma_dst[256];
    for (uint32_t i = 0U; i < 256U; ++i) {
        dma_src[i] = 0xA5000000UL | (i * 0x0101UL);
        dma_dst[i] = 0UL;
    }

    uint32_t t0 = Clock_GetCycles();
    dma_status_t dst_m2m = dma_mem2mem_sync(DMA1, 2U, dma_src, dma_dst, 256U, 4U, PERIPH_TIMEOUT);
    uint32_t m2m_cycles = Clock_GetCycles() - t0;

    uint32_t match_cnt = 0U;
    for (uint32_t i = 0U; i < 256U; ++i) {
        if (dma_dst[i] == dma_src[i]) {
            match_cnt++;
        }
    }
    uint32_t dma_isr_after = DMA1->ISR;
    bool ok5a = (dst_m2m == DMA_OK) && (match_cnt == 256U) && (dma_isr_after == 0U);
    uart_printf("[%s] TEST5A_DMA1_MEM2MEM: status=%d match=%lu/256 cycles=%lu DMA1_ISR=0x%08lX\r\n",
                ok5a ? "PASS" : "FAIL",
                (int)dst_m2m, (unsigned long)match_cnt, (unsigned long)m2m_cycles,
                (unsigned long)dma_isr_after);

    /* --- 5B: Internal ADC1 VREFINT & Temp Sensor --- */
    adc_status_t ast_init = adc_init_single_ended(ADC1, PERIPH_TIMEOUT);
    uint16_t vref_raw = 0U;
    uint16_t temp_raw = 0U;
    int32_t  temp_mdegc = 0;
    uint32_t vdda_mv    = 0U;

    adc_status_t ast_vref = adc_read_vrefint_raw(&vref_raw, PERIPH_TIMEOUT);
    adc_status_t ast_tmp  = adc_read_temp_raw(&temp_raw, PERIPH_TIMEOUT);
    adc_status_t ast_cal  = adc_read_temp_mdegc(&temp_mdegc, &vdda_mv, PERIPH_TIMEOUT);

    uint16_t vref_cal = *(const volatile uint16_t *)(uintptr_t)ADC_VREFINT_CAL_ADDR;
    uint16_t ts_cal1  = *(const volatile uint16_t *)(uintptr_t)ADC_TS_CAL1_ADDR;
    uint16_t ts_cal2  = *(const volatile uint16_t *)(uintptr_t)ADC_TS_CAL2_ADDR;

    bool ok5b = (ast_init == ADC_OK) && (ast_vref == ADC_OK) && (ast_tmp == ADC_OK) &&
                (ast_cal == ADC_OK) &&
                (vdda_mv >= 3000U) && (vdda_mv <= 3600U) &&
                (temp_mdegc >= 10000) && (temp_mdegc <= 75000);

    uart_printf("[%s] TEST5B_ADC1_VREF_TEMP: ADC_CCR=0x%08lX CALFACT=0x%08lX "
                "VREF_CAL=%u VREF_RAW=%u VDDA=%lumV "
                "TS_CAL1=%u TS_CAL2=%u TEMP_RAW=%u DIE_TEMP=%ldmdegC\r\n",
                ok5b ? "PASS" : "FAIL",
                (unsigned long)ADC12_COMMON->CCR, (unsigned long)ADC1->CALFACT,
                (unsigned)vref_cal, (unsigned)vref_raw, (unsigned long)vdda_mv,
                (unsigned)ts_cal1, (unsigned)ts_cal2, (unsigned)temp_raw, (long)temp_mdegc);

    /* --- 5C: ADC1 -> DMA1_Channel1 (DMAMUX1 Req 5) Circular --- */
    g_dma_ht_count = 0U;
    g_dma_tc_count = 0U;
    g_dma_te_count = 0U;
    for (uint32_t i = 0U; i < ADC_DMA_SAMPLES; ++i) {
        g_adc_dma_buf[i] = 0U;
    }

    dma_status_t dst_circ = dma_config_p2m_circular(
        DMA1, 1U, (uint8_t)DMAMUX_REQ_ADC1,
        &ADC1->DR, g_adc_dma_buf,
        ADC_DMA_SAMPLES, 2U, true, 6U);

    /* Ensure SMPR2 ch16 & ch18 are 640.5 cycles, 2-channel scan SQ1=18 (VREFINT), SQ2=16 (TEMP) */
    ADC1->SMPR2 = (ADC1->SMPR2 & ~((7UL << ((16U - 10U) * 3U)) | (7UL << ((18U - 10U) * 3U)))) |
                  (7UL << ((16U - 10U) * 3U)) | (7UL << ((18U - 10U) * 3U));
    ADC1->SQR1  = (1UL << ADC_SQR1_L_Pos) |
                  ((uint32_t)ADC1_CH_VREFINT     << ADC_SQR1_SQ1_Pos) |
                  ((uint32_t)ADC1_CH_TEMP_SENSOR << ADC_SQR1_SQ2_Pos);
    ADC1->CFGR |= ADC_CFGR_CONT | ADC_CFGR_DMAEN | ADC_CFGR_DMACFG | ADC_CFGR_OVRMOD;
    ADC1->ISR   = ADC_ISR_EOC | ADC_ISR_EOS | ADC_ISR_OVR;
    ADC1->CR   |= ADC_CR_ADSTART;

    Clock_DelayMs(10U);

    /* Cleanly stop ADC1 continuous conversion and DMA1_Channel1 */
    ADC1->CR |= ADC_CR_ADSTP;
    uint32_t rem = PERIPH_TIMEOUT;
    while (((ADC1->CR & ADC_CR_ADSTP) != 0U) && (rem > 0U)) {
        --rem;
    }
    ADC1->CFGR &= ~(ADC_CFGR_CONT | ADC_CFGR_DMAEN | ADC_CFGR_DMACFG);
    DMA1_Channel1->CCR = 0U;
    NVIC_DisableIRQ(DMA1_Channel1_IRQn);
    dma_clear_flags(DMA1, 1U, true, true, true);

    uint32_t ht_cnt = g_dma_ht_count;
    uint32_t tc_cnt = g_dma_tc_count;
    uint32_t te_cnt = g_dma_te_count;
    uint16_t s0     = g_adc_dma_buf[0];
    uint16_t s1     = g_adc_dma_buf[1];

    bool ok5c = (dst_circ == DMA_OK) && (ht_cnt > 0U) && (tc_cnt > 0U) &&
                (te_cnt == 0U) && (s0 > 500U) && (s1 > 200U);

    uart_printf("[%s] TEST5C_ADC1_DMA1_CIRC: DMAMUX0_CCR=0x%08lX HT=%lu TC=%lu TE=%lu "
                "buf[0]_vref=%u buf[1]_temp=%u\r\n",
                ok5c ? "PASS" : "FAIL",
                (unsigned long)DMAMUX1_Channel0->CCR,
                (unsigned long)ht_cnt, (unsigned long)tc_cnt, (unsigned long)te_cnt,
                (unsigned)s0, (unsigned)s1);

    return ok5a && ok5b && ok5c;
}

static bool run_test6_cordic_fmac(void) {
    /* --- 6A: CORDIC cos/sin(pi/4) + 256-angle benchmark vs Cortex-M4F FPU --- */
    static int32_t angles_q31[CORDIC_BENCH_N];
    static float   angles_rad[CORDIC_BENCH_N];
    static int32_t cordic_cos[CORDIC_BENCH_N];
    static int32_t cordic_sin[CORDIC_BENCH_N];
    static float   fpu_cos[CORDIC_BENCH_N];
    static float   fpu_sin[CORDIC_BENCH_N];

    const float pi_f = 3.14159265358979f;
    for (uint32_t i = 0U; i < CORDIC_BENCH_N; ++i) {
        float norm = -0.95f + (1.90f * (float)i) / (float)CORDIC_BENCH_N;
        angles_rad[i] = norm * pi_f;
        angles_q31[i] = (int32_t)(norm * 2147483648.0f);
    }

    RCC->AHB1ENR |= RCC_AHB1ENR_CORDICEN;
    (void)RCC->AHB1ENR;

    /* FUNC=0 (Cos+Sin), PRECISION=6 (24 iter = 6 cyc), NRES=1 (2 outputs), 32-bit q1.31 */
    CORDIC->CSR = (0UL << CORDIC_CSR_FUNC_Pos) |
                  (6UL << CORDIC_CSR_PRECISION_Pos) |
                  CORDIC_CSR_NRES;

    /* Single exact check: theta = pi/4 -> norm = 0.25 -> q1.31 = 0x20000000 */
    uint32_t t0_one = Clock_GetCycles();
    CORDIC->WDATA = 0x20000000UL;
    int32_t cos_pi4_q31 = (int32_t)CORDIC->RDATA;
    int32_t sin_pi4_q31 = (int32_t)CORDIC->RDATA;
    uint32_t cordic_one_cyc = Clock_GetCycles() - t0_one;

    volatile float rad_pi4 = 0.78539816339745f;
    uint32_t t0_fpu_one = Clock_GetCycles();
    volatile float fpu_cos_pi4 = cosf(rad_pi4);
    volatile float fpu_sin_pi4 = sinf(rad_pi4);
    uint32_t fpu_one_cyc = Clock_GetCycles() - t0_fpu_one;
    (void)fpu_cos_pi4;
    (void)fpu_sin_pi4;

    /* 256-angle throughput benchmark */
    uint32_t t0 = Clock_GetCycles();
    for (uint32_t i = 0U; i < CORDIC_BENCH_N; ++i) {
        CORDIC->WDATA = (uint32_t)angles_q31[i];
        cordic_cos[i] = (int32_t)CORDIC->RDATA;
        cordic_sin[i] = (int32_t)CORDIC->RDATA;
    }
    uint32_t cordic_256_cyc = Clock_GetCycles() - t0;

    t0 = Clock_GetCycles();
    for (uint32_t i = 0U; i < CORDIC_BENCH_N; ++i) {
        fpu_cos[i] = cosf(angles_rad[i]);
        fpu_sin[i] = sinf(angles_rad[i]);
    }
    uint32_t fpu_256_cyc = Clock_GetCycles() - t0;

    float max_err = 0.0f;
    const float inv_q31 = 1.0f / 2147483648.0f;
    for (uint32_t i = 0U; i < CORDIC_BENCH_N; ++i) {
        float ec = fabsf(((float)cordic_cos[i] * inv_q31) - fpu_cos[i]);
        float es = fabsf(((float)cordic_sin[i] * inv_q31) - fpu_sin[i]);
        if (ec > max_err) { max_err = ec; }
        if (es > max_err) { max_err = es; }
    }

    int32_t cos_pi4_u6 = (int32_t)(((int64_t)cos_pi4_q31 * 1000000LL) / 2147483648LL);
    int32_t sin_pi4_u6 = (int32_t)(((int64_t)sin_pi4_q31 * 1000000LL) / 2147483648LL);
    uint32_t max_err_u6 = (uint32_t)(max_err * 1000000.0f + 0.5f);

    bool ok6a = (cos_pi4_u6 >= 707100) && (cos_pi4_u6 <= 707115) &&
                (sin_pi4_u6 >= 707100) && (sin_pi4_u6 <= 707115) &&
                (max_err_u6 <= 20U) && (cordic_256_cyc < fpu_256_cyc);

    uart_printf("[%s] TEST6A_CORDIC_VS_FPU: pi/4 cos=0.%06ld sin=0.%06ld "
                "(1x: CORDIC=%lu cyc vs FPU=%lu cyc; 256x: CORDIC=%lu vs FPU=%lu; max_err=%luE-6)\r\n",
                ok6a ? "PASS" : "FAIL",
                (long)cos_pi4_u6, (long)sin_pi4_u6,
                (unsigned long)cordic_one_cyc, (unsigned long)fpu_one_cyc,
                (unsigned long)cordic_256_cyc, (unsigned long)fpu_256_cyc,
                (unsigned long)max_err_u6);

    /* --- 6B: FMAC 8-tap q1.15 FIR on step input --- */
    static const int16_t coeffs_q15[FMAC_FIR_TAPS] = {
        4096, 4096, 4096, 4096, 4096, 4096, 4096, 4096
    };
    static int16_t step_in[FMAC_FIR_SAMPLES];
    static int16_t fmac_out[FMAC_FIR_SAMPLES];
    static int16_t sw_out[FMAC_FIR_SAMPLES];

    for (uint32_t n = 0U; n < FMAC_FIR_SAMPLES; ++n) {
        step_in[n] = 16000;
    }

    RCC->AHB1ENR |= RCC_AHB1ENR_FMACEN;
    (void)RCC->AHB1ENR;

    FMAC->CR |= FMAC_CR_RESET;
    while ((FMAC->CR & FMAC_CR_RESET) != 0U) {}

    FMAC->X2BUFCFG = (0UL  << FMAC_X2BUFCFG_X2_BASE_Pos) | ((uint32_t)FMAC_FIR_TAPS << FMAC_X2BUFCFG_X2_BUF_SIZE_Pos);
    FMAC->X1BUFCFG = (8UL  << FMAC_X1BUFCFG_X1_BASE_Pos) | (16UL << FMAC_X1BUFCFG_X1_BUF_SIZE_Pos);
    FMAC->YBUFCFG  = (24UL << FMAC_YBUFCFG_Y_BASE_Pos)   | (16UL << FMAC_YBUFCFG_Y_BUF_SIZE_Pos);

    /* Preload X2 coefficients (FUNC=2) */
    FMAC->PARAM = ((uint32_t)FMAC_FIR_TAPS << FMAC_PARAM_P_Pos) | (2UL << FMAC_PARAM_FUNC_Pos) | FMAC_PARAM_START;
    for (uint32_t k = 0U; k < FMAC_FIR_TAPS; ++k) {
        FMAC->WDATA = (uint32_t)(uint16_t)coeffs_q15[k];
    }
    while ((FMAC->PARAM & FMAC_PARAM_START) != 0U) {}

    /* Zero-preload X1 delay line (FUNC=1, P = TAPS - 1) */
    FMAC->PARAM = ((uint32_t)(FMAC_FIR_TAPS - 1U) << FMAC_PARAM_P_Pos) | (1UL << FMAC_PARAM_FUNC_Pos) | FMAC_PARAM_START;
    for (uint32_t k = 0U; k < (FMAC_FIR_TAPS - 1U); ++k) {
        FMAC->WDATA = 0UL;
    }
    while ((FMAC->PARAM & FMAC_PARAM_START) != 0U) {}

    /* Launch continuous 8-tap FIR (FUNC=8) */
    FMAC->CR    = FMAC_CR_CLIPEN;
    FMAC->PARAM = ((uint32_t)FMAC_FIR_TAPS << FMAC_PARAM_P_Pos) |
                  (0UL << FMAC_PARAM_Q_Pos) |
                  (0UL << FMAC_PARAM_R_Pos) |
                  (8UL << FMAC_PARAM_FUNC_Pos) |
                  FMAC_PARAM_START;

    t0 = Clock_GetCycles();
    for (uint32_t n = 0U; n < FMAC_FIR_SAMPLES; ++n) {
        while ((FMAC->SR & FMAC_SR_X1FULL) != 0U) {}
        FMAC->WDATA = (uint32_t)(uint16_t)step_in[n];
        while ((FMAC->SR & FMAC_SR_YEMPTY) != 0U) {}
        fmac_out[n] = (int16_t)(FMAC->RDATA & 0xFFFFU);
    }
    uint32_t fmac_cyc = Clock_GetCycles() - t0;

    t0 = Clock_GetCycles();
    for (uint32_t n = 0U; n < FMAC_FIR_SAMPLES; ++n) {
        int32_t acc = 0;
        for (uint32_t k = 0U; k < FMAC_FIR_TAPS; ++k) {
            int16_t x = (n >= k) ? step_in[n - k] : 0;
            acc += (int32_t)coeffs_q15[k] * (int32_t)x;
        }
        sw_out[n] = (int16_t)(acc >> 15);
    }
    uint32_t sw_fir_cyc = Clock_GetCycles() - t0;

    int32_t max_lsb_diff = 0;
    for (uint32_t n = 0U; n < FMAC_FIR_SAMPLES; ++n) {
        int32_t d = (int32_t)fmac_out[n] - (int32_t)sw_out[n];
        if (d < 0) { d = -d; }
        if (d > max_lsb_diff) { max_lsb_diff = d; }
    }
    uint32_t fmac_sr = FMAC->SR;
    uint32_t fmac_err_bits = fmac_sr & (FMAC_SR_OVFL | FMAC_SR_UNFL | FMAC_SR_SAT);

    bool ok6b = (fmac_out[0] == 2000) && (fmac_out[7] == 16000) && (fmac_out[31] == 16000) &&
                (max_lsb_diff <= 1) && (fmac_err_bits == 0U);

    uart_printf("[%s] TEST6B_FMAC_FIR: y[0]=%d y[7]=%d y[31]=%d sw[31]=%d max_diff=%ldLSB "
                "FMAC=%lu cyc vs SW=%lu cyc FMAC_SR=0x%08lX\r\n",
                ok6b ? "PASS" : "FAIL",
                (int)fmac_out[0], (int)fmac_out[7], (int)fmac_out[31], (int)sw_out[31],
                (long)max_lsb_diff, (unsigned long)fmac_cyc, (unsigned long)sw_fir_cyc,
                (unsigned long)fmac_sr);

    return ok6a && ok6b;
}

static bool run_test7_fdcan1_crc32(void) {
    /* 7A: Standard IEEE 802.3 CRC-32 check vector */
    static const uint8_t check_vec[9] = {'1', '2', '3', '4', '5', '6', '7', '8', '9'};
    uint32_t crc_check = crc32_hw_ieee8023(check_vec, (uint32_t)sizeof(check_vec));

    /* 7B: FDCAN1 Internal Loopback (500 kbps nominal / 2.000 Mbps BRS @ 170 MHz PCLK1) */
    RCC->CCIPR     = (RCC->CCIPR & ~RCC_CCIPR_FDCANSEL) | RCC_CCIPR_FDCANSEL_1; /* PCLK1 */
    RCC->APB1ENR1 |= RCC_APB1ENR1_FDCANEN;
    (void)RCC->APB1ENR1;

    FDCAN_CONFIG->CKDIV = 0U;

    /* Zero Message RAM (212 words = 848 bytes) */
    volatile uint32_t *ram = (volatile uint32_t *)FDCAN1_RAM_BASE;
    for (uint32_t i = 0U; i < 212U; ++i) {
        ram[i] = 0U;
    }

    FDCAN1->CCCR |= FDCAN_CCCR_INIT;
    uint32_t rem = PERIPH_TIMEOUT;
    while (((FDCAN1->CCCR & FDCAN_CCCR_INIT) == 0U) && (rem > 0U)) { --rem; }
    if (rem == 0U) {
        uart_printf("[FAIL] TEST7_FDCAN1_AND_CRC32: timeout entering FDCAN INIT\r\n");
        return false;
    }

    FDCAN1->CCCR |= FDCAN_CCCR_CCE;
    FDCAN1->CCCR |= FDCAN_CCCR_FDOE | FDCAN_CCCR_BRSE | FDCAN_CCCR_MON | FDCAN_CCCR_TEST;
    FDCAN1->TEST |= FDCAN_TEST_LBCK;

    /* 500 kbps @ 170 MHz: NBRP=9 (/10=17MHz), NTSEG1=26 (27TQ), NTSEG2=5 (6TQ), NSJW=5 */
    FDCAN1->NBTP = (5UL  << FDCAN_NBTP_NSJW_Pos)   |
                   (9UL  << FDCAN_NBTP_NBRP_Pos)   |
                   (26UL << FDCAN_NBTP_NTSEG1_Pos) |
                   (5UL  << FDCAN_NBTP_NTSEG2_Pos);

    /* 2.000 Mbps @ 170 MHz: DBRP=4 (/5=34MHz), DTSEG1=11 (12TQ), DTSEG2=3 (4TQ), DSJW=3 */
    FDCAN1->DBTP = (4UL  << FDCAN_DBTP_DBRP_Pos)   |
                   (11UL << FDCAN_DBTP_DTSEG1_Pos) |
                   (3UL  << FDCAN_DBTP_DTSEG2_Pos) |
                   (3UL  << FDCAN_DBTP_DSJW_Pos);

    FDCAN1->RXGFC = 0U; /* Accept non-matching into RX FIFO 0 */

    FDCAN1->CCCR &= ~FDCAN_CCCR_INIT;
    rem = PERIPH_TIMEOUT;
    while (((FDCAN1->CCCR & FDCAN_CCCR_INIT) != 0U) && (rem > 0U)) { --rem; }
    if (rem == 0U) {
        uart_printf("[FAIL] TEST7_FDCAN1_AND_CRC32: timeout leaving FDCAN INIT\r\n");
        return false;
    }

    /* Build 16-byte CAN-FD payload (12 data bytes + 4-byte IEEE 802.3 CRC-32 trailer) */
    uint8_t tx_buf[16];
    for (uint32_t i = 0U; i < 12U; ++i) {
        tx_buf[i] = (uint8_t)(0xB0U + i);
    }
    uint32_t tx_crc = crc32_hw_ieee8023(tx_buf, 12U);
    tx_buf[12] = (uint8_t)(tx_crc & 0xFFU);
    tx_buf[13] = (uint8_t)((tx_crc >> 8U)  & 0xFFU);
    tx_buf[14] = (uint8_t)((tx_crc >> 16U) & 0xFFU);
    tx_buf[15] = (uint8_t)((tx_crc >> 24U) & 0xFFU);

    /* Write TX Buffer 0: Std ID 0x123, DLC=10 (16 bytes), FDF=1, BRS=1 */
    volatile uint32_t *tx_elem = (volatile uint32_t *)FDCAN1_TX_BUF_BASE;
    tx_elem[0] = (0x123UL << 18U);
    tx_elem[1] = (10UL << 16U) | (1UL << 21U) | (1UL << 20U);
    for (uint32_t w = 0U; w < 4U; ++w) {
        tx_elem[2U + w] = ((uint32_t)tx_buf[w * 4U + 0U])        |
                          ((uint32_t)tx_buf[w * 4U + 1U] << 8U)  |
                          ((uint32_t)tx_buf[w * 4U + 2U] << 16U) |
                          ((uint32_t)tx_buf[w * 4U + 3U] << 24U);
    }
    __DSB();
    FDCAN1->TXBAR = 1UL;

    rem = 500000UL;
    while (((FDCAN1->RXF0S & FDCAN_RXF0S_F0FL) == 0U) && (rem > 0U)) { --rem; }
    if (rem == 0U) {
        uart_printf("[FAIL] TEST7_FDCAN1_AND_CRC32: timeout waiting for RX FIFO 0\r\n");
        return false;
    }

    uint32_t get_idx = (FDCAN1->RXF0S & FDCAN_RXF0S_F0GI) >> FDCAN_RXF0S_F0GI_Pos;
    volatile uint32_t *rx_elem = (volatile uint32_t *)FDCAN1_RX_FIFO0_BASE +
                                 (get_idx * FDCAN_ELEM_STRIDE_U32);
    uint32_t rx_id  = (rx_elem[0] >> 18U) & 0x7FFUL;
    uint32_t rx_dlc = (rx_elem[1] >> 16U) & 0xFUL;
    uint32_t rx_fdf = (rx_elem[1] >> 21U) & 0x1UL;
    uint32_t rx_brs = (rx_elem[1] >> 20U) & 0x1UL;

    uint8_t rx_buf[16];
    for (uint32_t w = 0U; w < 4U; ++w) {
        uint32_t word = rx_elem[2U + w];
        rx_buf[w * 4U + 0U] = (uint8_t)(word & 0xFFU);
        rx_buf[w * 4U + 1U] = (uint8_t)((word >> 8U)  & 0xFFU);
        rx_buf[w * 4U + 2U] = (uint8_t)((word >> 16U) & 0xFFU);
        rx_buf[w * 4U + 3U] = (uint8_t)((word >> 24U) & 0xFFU);
    }
    FDCAN1->RXF0A = get_idx;

    uint32_t rx_crc_calc = crc32_hw_ieee8023(rx_buf, 12U);
    uint32_t rx_crc_recv = ((uint32_t)rx_buf[12])        |
                           ((uint32_t)rx_buf[13] << 8U)  |
                           ((uint32_t)rx_buf[14] << 16U) |
                           ((uint32_t)rx_buf[15] << 24U);
    bool bytes_eq = (memcmp(tx_buf, rx_buf, sizeof(tx_buf)) == 0);
    uint32_t ecr = FDCAN1->ECR;

    bool ok = (crc_check == 0xCBF43926UL) && (rx_id == 0x123UL) && (rx_dlc == 10UL) &&
              (rx_fdf == 1UL) && (rx_brs == 1UL) && bytes_eq &&
              (rx_crc_calc == rx_crc_recv) && (rx_crc_calc == tx_crc) && (ecr == 0UL);

    uart_printf("[%s] TEST7_FDCAN1_AND_CRC32: CRC_VEC=0x%08lX RX_ID=0x%03lX DLC=%lu(16B) "
                "FDF=%lu BRS=%lu PAYLOAD_CRC=0x%08lX MATCH=%u NBTP=0x%08lX DBTP=0x%08lX ECR=0x%08lX\r\n",
                ok ? "PASS" : "FAIL",
                (unsigned long)crc_check, (unsigned long)rx_id, (unsigned long)rx_dlc,
                (unsigned long)rx_fdf, (unsigned long)rx_brs, (unsigned long)rx_crc_calc,
                bytes_eq ? 1U : 0U, (unsigned long)FDCAN1->NBTP, (unsigned long)FDCAN1->DBTP,
                (unsigned long)ecr);
    return ok;
}

static void run_test8_phase1_arm_and_fault(uint32_t p1_failures) {
    /* Configure 8-region MPU guard (Region 7 = 32-byte No-Access XN) + DIV_0_TRP */
    FaultHandler_EnableDetailedFaults(true);

    __DMB();
    MPU->CTRL = 0U;
    MPU->RNR  = 7U;
    MPU->RBAR = ((uint32_t)(uintptr_t)&g_mpu_guard_zone[0]) & ~0x1FUL;
    MPU->RASR = MPU_RASR_XN_Msk |
                (0UL << MPU_RASR_AP_Pos) |
                (4UL << MPU_RASR_SIZE_Pos) |
                MPU_RASR_ENABLE_Msk;
    MPU->CTRL = MPU_CTRL_PRIVDEFENA_Msk | MPU_CTRL_ENABLE_Msk;
    __DSB();
    __ISB();

    /* Benign privileged SRAM read must succeed cleanly under PRIVDEFENA=1 */
    uint32_t benign_val = g_mpu_valid_var;
    bool mpu_ok = (MPU->CTRL == (MPU_CTRL_PRIVDEFENA_Msk | MPU_CTRL_ENABLE_Msk)) &&
                  (benign_val == 0x12345678UL);
    if (!mpu_ok) {
        p1_failures++;
    }

    /* Persist Phase 1 failure count across controlled NVIC_SystemReset() */
    g_p1_failures = p1_failures;
    g_p1_magic    = SELFTEST_P1_MAGIC;
    __DSB();

    uart_printf("[%s] TEST8A_MPU_ARMED: MPU_CTRL=0x%08lX RBAR=0x%08lX RASR=0x%08lX "
                "SHCSR=0x%08lX CCR=0x%08lX benign_read=0x%08lX -> triggering DIV0 UsageFault...\r\n",
                mpu_ok ? "PASS" : "FAIL",
                (unsigned long)MPU->CTRL, (unsigned long)MPU->RBAR, (unsigned long)MPU->RASR,
                (unsigned long)SCB->SHCSR, (unsigned long)SCB->CCR, (unsigned long)benign_val);

    (void)usart_flush_tx(USART2, UART_TX_TIMEOUT);
    Clock_DelayMs(5U);

    /* Controlled divide-by-zero via Cortex-M4 SDIV instruction (trapped by DIV_0_TRP) */
    volatile int32_t dividend = 42;
    volatile int32_t divisor  = 0;
    volatile int32_t quotient = 0;
    __asm__ volatile("sdiv %0, %1, %2" : "=r"(quotient) : "r"(dividend), "r"(divisor));
    (void)quotient;

    /* Should never reach here */
    uart_printf("[FAIL] TEST8A_MPU_ARMED: DIV0 did not trap!\r\n");
}

static bool run_test8_phase2_verify_crashlog(void) {
    uint32_t magic  = g_crash_log.magic;
    uint32_t cfsr   = g_crash_log.cfsr;
    uint32_t hfsr   = g_crash_log.hfsr;
    uint32_t pc     = g_crash_log.pc;
    uint32_t lr     = g_crash_log.lr;
    uint32_t sp     = g_crash_log.stacked_sp;
    uint32_t flags  = g_crash_log.fault_flags;
    uint32_t csr    = RCC->CSR;

    bool ok = (magic == CRASH_LOG_MAGIC) &&
              ((cfsr & SCB_CFSR_DIVBYZERO_Msk) != 0U) &&
              ((flags & FAULT_FLAG_DIVBYZERO) != 0U) &&
              ((csr & RCC_CSR_SFTRSTF) != 0U) &&
              (pc >= 0x08000000UL) && (pc < 0x08080000UL) &&
              (sp >= 0x20000000UL) && (sp < 0x20018000UL);

    uart_printf("[%s] TEST8B_FAULT_PERSISTENCE_VERIFIED: magic=0x%08lX CFSR=0x%08lX "
                "HFSR=0x%08lX PC=0x%08lX LR=0x%08lX SP=0x%08lX flags=0x%08lX RCC_CSR=0x%08lX\r\n",
                ok ? "PASS" : "FAIL",
                (unsigned long)magic, (unsigned long)cfsr, (unsigned long)hfsr,
                (unsigned long)pc, (unsigned long)lr, (unsigned long)sp,
                (unsigned long)flags, (unsigned long)csr);

    FaultHandler_ClearCrashLog();
    iwdg_clear_reset_flags();
    return ok;
}

static bool run_test9_iwdg_safe(void) {
    iwdg_status_t ist = iwdg_init_ms(2000U, true, PERIPH_TIMEOUT);
    uint32_t pr       = IWDG->PR;
    uint32_t rlr      = IWDG->RLR;
    uint32_t fzr1     = DBGMCU->APB1FZR1;
    bool dbg_freeze   = ((fzr1 & DBGMCU_APB1FZR1_DBG_IWDG_STOP) != 0U);

    /* Kick cleanly every 50 ms for 300 ms while blinking LD2 */
    uint32_t kicks = 0U;
    if (ist == IWDG_OK) {
        for (uint32_t i = 0U; i < 6U; ++i) {
            Clock_DelayMs(50U);
            iwdg_kick();
            GPIO_TogglePin(GPIOA, 5U);
            kicks++;
        }
    }

    bool ok = (ist == IWDG_OK) && dbg_freeze && (kicks == 6U) && !iwdg_was_reset_cause();
    uart_printf("[%s] TEST9_IWDG_SAFE_KICK: status=%d PR=%lu RLR=%lu DBGMCU_APB1FZR1=0x%08lX "
                "kicks=%lu/6 (300ms no reset)\r\n",
                ok ? "PASS" : "FAIL",
                (int)ist, (unsigned long)pr, (unsigned long)rlr,
                (unsigned long)fzr1, (unsigned long)kicks);
    return ok;
}

/* -------------------------------------------------------------------------- */
/* Main Entry                                                                 */
/* -------------------------------------------------------------------------- */
int main(void) {
    if (FaultHandler_HasValidCrashLog() && (g_p1_magic == SELFTEST_P1_MAGIC)) {
        /* ==================== PHASE 2: POST-FAULT REBOOT ==================== */
        (void)Clock_Init_170MHz();
        Clock_DWT_Init();
        GPIO_EnableClock(GPIOA);
        (void)GPIO_ConfigOutput(GPIOA, 5U, GPIO_OTYPE_PUSHPULL, GPIO_SPEED_LOW, GPIO_PULL_NONE);
        uart2_pins_and_init();
        Clock_DelayMs(20U);

        uart_printf("\r\n=== NUCLEO-G474RE ONBOARD HARDWARE SELF-TEST (PHASE 2/2: POST-FAULT REBOOT) ===\r\n");

        uint32_t failures = g_p1_failures;
        g_p1_magic = 0U;

        if (!run_test8_phase2_verify_crashlog()) {
            failures++;
        }
        if (!run_test9_iwdg_safe()) {
            failures++;
        }

        if (failures == 0U) {
            uart_printf("[SUMMARY] ONBOARD_SELFTEST_ALL_PASSED (9/9 TESTS, 0 FAILURES)\r\n");
        } else {
            uart_printf("[SUMMARY] ONBOARD_SELFTEST_FAILED (failures=%lu)\r\n",
                        (unsigned long)failures);
        }
        (void)usart_flush_tx(USART2, UART_TX_TIMEOUT);

        /* Safe steady-state heartbeat loop keeping IWDG fed cleanly */
        for (;;) {
            iwdg_kick();
            GPIO_TogglePin(GPIOA, 5U);
            Clock_DelayMs(250U);
        }
    }

    /* ==================== PHASE 1: COLD BOOT TESTS 1..8A ==================== */
    /* Start at reset HSI16 -> bring up USART2 briefly for boot banner + Test 1/2,
     * then Test 3 exercises BOTH 160 MHz and 170 MHz Boost profiles cleanly! */
    Clock_DWT_Init();
    uart2_pins_and_init();
    Clock_DelayMs(80U);

    uart_printf("\r\n=== NUCLEO-G474RE ONBOARD HARDWARE SELF-TEST (PHASE 1/2) ===\r\n");

    uint32_t failures = 0U;
    if (!run_test1_mcu_id())       { failures++; }
    if (!run_test2_gpio())         { failures++; }
    if (!run_test3_clocks())       { failures++; }
    if (!run_test4_tim2_exti13())  { failures++; }
    if (!run_test5_dma_and_adc())  { failures++; }
    if (!run_test6_cordic_fmac())  { failures++; }
    if (!run_test7_fdcan1_crc32()) { failures++; }

    /* Test 8 Phase 1 triggers controlled DIV0 UsageFault -> .noinit -> NVIC_SystemReset() */
    run_test8_phase1_arm_and_fault(failures);

    /* Unreachable unless DIV0 trap failed */
    for (;;) {
        __asm__ volatile("nop");
    }
}
