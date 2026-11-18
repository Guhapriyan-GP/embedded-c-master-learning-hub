/**
 * @file    test_gpio_mock.c
 * @brief   Register-level Host Unit Test Suite for STM32G474RE GPIO Driver
 *          (`23-code-library/Drivers/gpio_g4.c`) and Fault Frame SRAM Validator.
 *
 * Links DIRECTLY against the real `23-code-library/Drivers/gpio_g4.c` using
 * `tests/mocks/stm32g474xx.h` to verify:
 *   1. STM32G4 Analog reset state (`GPIOA=0xABFFFFFF`, `GPIOB=0xFFFFFEBF`, `GPIOC=0xFFFFFFFF`)
 *   2. RCC->AHB2ENR clock enable for all ports A..G and safe NULL/invalid port ignore
 *   3. Bitfield isolation across MODER/OTYPER/OSPEEDR/PUPDR/AFRL/AFRH (SWD PA13/PA14 preserved!)
 *   4. Atomic BSRR set/reset/write, sibling-safe BSRR toggle (single-writer ODR->BSRR), & BSRR->ODR hardware model (BS wins over BR)
 *   5. IDR digital read helper & LCKR 4-step RM0440 §9.4.8 sequence verification
 *   6. HardFault 8-word exception stack-frame SRAM1+SRAM2 & CCM-SRAM bounds validator
 *      and Cortex-M4 SCB CFSR/HFSR fault-flag decoder (matching `fault_handler.c`)
 *
 * Concept IDs: PERIPH-GPIO-001, PERIPH-GPIO-002, PERIPH-GPIO-003, EMB-MMIO-001,
 *              ARM-EXCP-001, REL-FAULT-001, TOOL-QA-001
 * Verification Status: Host-Compiled & Executed (MSYS2 gcc 15.2.0, fake CMSIS registers).
 */

#include "fault_handler.h"
#include "gpio_g4.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

/* Backing fake CMSIS peripheral registers */
GPIO_TypeDef g_mock_gpioa;
GPIO_TypeDef g_mock_gpiob;
GPIO_TypeDef g_mock_gpioc;
GPIO_TypeDef g_mock_gpiod;
GPIO_TypeDef g_mock_gpioe;
GPIO_TypeDef g_mock_gpiof;
GPIO_TypeDef g_mock_gpiog;
RCC_TypeDef  g_mock_rcc;

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
            fprintf(stderr, "  [FAIL] %s:%d: expected 0x%08X, got 0x%08X (%s)\n",       \
                    __FILE__, __LINE__, _e, _a, #actual);                               \
        }                                                                               \
    } while (0)

/** Reset all fake GPIO ports to exact STM32G474RE silicon reset values (RM0440 §9.4). */
static void mock_reset_silicon(void) {
    memset(&g_mock_gpioa, 0, sizeof(g_mock_gpioa));
    memset(&g_mock_gpiob, 0, sizeof(g_mock_gpiob));
    memset(&g_mock_gpioc, 0, sizeof(g_mock_gpioc));
    memset(&g_mock_gpiod, 0, sizeof(g_mock_gpiod));
    memset(&g_mock_gpioe, 0, sizeof(g_mock_gpioe));
    memset(&g_mock_gpiof, 0, sizeof(g_mock_gpiof));
    memset(&g_mock_gpiog, 0, sizeof(g_mock_gpiog));
    memset(&g_mock_rcc, 0, sizeof(g_mock_rcc));

    /* RM0440 §9.4.1..9.4.4 STM32G474RE reset defaults */
    g_mock_gpioa.MODER   = 0xABFFFFFFUL; /* PA13/14/15 = AF0 (SWD/JTAG), others Analog */
    g_mock_gpioa.OSPEEDR = 0x0C000000UL; /* PA13 Very-High speed */
    g_mock_gpioa.PUPDR   = 0x64000000UL; /* PA13 PU, PA14 PD, PA15 PU */

    g_mock_gpiob.MODER   = 0xFFFFFEBFUL; /* PB3/PB4 = AF0 (SWO/NJTRST), others Analog */
    g_mock_gpiob.PUPDR   = 0x00000100UL; /* PB4 PU */

    g_mock_gpioc.MODER   = 0xFFFFFFFFUL; /* All pins Analog (0b11) */
    g_mock_gpiod.MODER   = 0xFFFFFFFFUL;
    g_mock_gpioe.MODER   = 0xFFFFFFFFUL;
    g_mock_gpiof.MODER   = 0xFFFFFFFFUL;
    g_mock_gpiog.MODER   = 0xFFFFFFFFUL;
}

/** Hardware behavioral model of STM32G4 BSRR write -> ODR update (BS[15:0] wins over BR[31:16]). */
static void mock_gpio_commit_bsrr(GPIO_TypeDef *port) {
    uint32_t bs = port->BSRR & 0xFFFFUL;
    uint32_t br = (port->BSRR >> 16U) & 0xFFFFUL;
    port->ODR   = (port->ODR & ~br) | bs;
    port->BSRR  = 0U; /* Write-only register reads back 0 in silicon */
}

/** Exact replica of `fault_handler.c` STM32G474RE SRAM1+SRAM2 / CCM-SRAM frame validator. */
static bool fault_validate_stack_frame_addr(uintptr_t sp_addr) {
    if ((sp_addr & 3UL) != 0UL) {
        return false;
    }
    bool in_sram = (sp_addr >= (uintptr_t)SRAM1_BASE) &&
                   (sp_addr <= (uintptr_t)(SRAM2_BASE + SRAM2_SIZE - 32UL));
    bool in_ccm  = (sp_addr >= (uintptr_t)CCMSRAM_BASE) &&
                   (sp_addr <= (uintptr_t)(CCMSRAM_BASE + CCMSRAM_SIZE - 32UL));
    return (in_sram || in_ccm);
}

/** Exact replica of `fault_handler.c` SCB CFSR/HFSR bitfield decoder. */
static uint32_t fault_decode_scb_flags(uint32_t cfsr, uint32_t hfsr) {
    uint32_t flags = FAULT_FLAG_NONE;
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
        if ((cfsr & SCB_CFSR_UNALIGNED_Msk) != 0U) {
            flags |= FAULT_FLAG_UNALIGNED;
        }
        if ((cfsr & SCB_CFSR_UNDEFINSTR_Msk) != 0U) {
            flags |= FAULT_FLAG_UNDEFINSTR;
        }
        if ((cfsr & SCB_CFSR_INVSTATE_Msk) != 0U) {
            flags |= FAULT_FLAG_INVSTATE;
        }
    }
    if ((hfsr & SCB_HFSR_FORCED_Msk) != 0U) {
        flags |= FAULT_FLAG_FORCED_HF;
    }
    if ((hfsr & SCB_HFSR_VECTTBL_Msk) != 0U) {
        flags |= FAULT_FLAG_VECTTBL_HF;
    }
    return flags;
}

/* ============================================================================
 * 1. GPIO Clock Enable Tests (`GPIO_EnableClock`)
 * ========================================================================== */
static void test_gpio_enable_clock_all_ports(void) {
    mock_reset_silicon();
    ASSERT_EQ_U32(0x00000000UL, RCC->AHB2ENR);

    GPIO_EnableClock(GPIOA);
    ASSERT_EQ_U32(RCC_AHB2ENR_GPIOAEN, RCC->AHB2ENR);
    GPIO_EnableClock(GPIOB);
    GPIO_EnableClock(GPIOC);
    GPIO_EnableClock(GPIOD);
    GPIO_EnableClock(GPIOE);
    GPIO_EnableClock(GPIOF);
    GPIO_EnableClock(GPIOG);
    ASSERT_EQ_U32(0x0000007FUL, RCC->AHB2ENR);

    /* NULL and unknown port pointers must safely be no-ops */
    GPIO_TypeDef foreign_port;
    memset(&foreign_port, 0, sizeof(foreign_port));
    GPIO_EnableClock(NULL);
    GPIO_EnableClock(&foreign_port);
    ASSERT_EQ_U32(0x0000007FUL, RCC->AHB2ENR);
}

/* ============================================================================
 * 2. STM32G4 Analog Reset State & Bitfield Isolation (`MODER/OTYPER/OSPEEDR/PUPDR/AFR`)
 * ========================================================================== */
static void test_gpio_reset_state_and_bitfield_isolation(void) {
    mock_reset_silicon();

    /* Verify STM32G474RE GPIOA Analog + SWD reset state */
    ASSERT_EQ_U32(0xABFFFFFFUL, GPIOA->MODER);
    ASSERT_EQ_U32(0x2U, (GPIOA->MODER >> 26U) & 0x3U); /* PA13 SWDIO = AF (0b10) */
    ASSERT_EQ_U32(0x2U, (GPIOA->MODER >> 28U) & 0x3U); /* PA14 SWCLK = AF (0b10) */
    ASSERT_EQ_U32(0x3U, (GPIOA->MODER >> 10U) & 0x3U); /* PA5 starts Analog (0b11) */

    /* Configure PA5 (LD2 Green LED) = Push-Pull Output, Low speed, No pull */
    ASSERT_TRUE(GPIO_ConfigOutput(GPIOA, 5U, GPIO_OTYPE_PUSHPULL,
                                  GPIO_SPEED_LOW, GPIO_PULL_NONE));
    ASSERT_EQ_U32(0xABFFF7FFUL, GPIOA->MODER);         /* Only PA5[11:10] changed to 0b01 */
    ASSERT_EQ_U32(0x00000000UL, GPIOA->OTYPER);
    ASSERT_EQ_U32(0x0C000000UL, GPIOA->OSPEEDR);       /* PA13 Very-High speed untouched */
    ASSERT_EQ_U32(0x64000000UL, GPIOA->PUPDR);         /* SWD pulls untouched */

    /* Configure PC13 (B1 Active-HIGH User Button) = Digital Input, No pull */
    ASSERT_EQ_U32(0xFFFFFFFFUL, GPIOC->MODER);
    ASSERT_TRUE(GPIO_ConfigInput(GPIOC, 13U, GPIO_PULL_NONE));
    ASSERT_EQ_U32(0xF3FFFFFFUL, GPIOC->MODER);         /* PC13[27:26] cleared from 11 -> 00 */
    ASSERT_EQ_U32(0x00000000UL, GPIOC->PUPDR);

    /* Configure PA2/PA3 for USART2 AF7 (AFRL = AFR[0]) and PA9/PA10 for USART1 AF7 (AFRH = AFR[1]) */
    ASSERT_TRUE(GPIO_ConfigAF(GPIOA, 2U, 7U, GPIO_OTYPE_PUSHPULL,
                              GPIO_SPEED_HIGH, GPIO_PULL_NONE));
    ASSERT_TRUE(GPIO_ConfigAF(GPIOA, 3U, 7U, GPIO_OTYPE_PUSHPULL,
                              GPIO_SPEED_HIGH, GPIO_PULL_UP));
    ASSERT_EQ_U32(0x00007700UL, GPIOA->AFR[0]);        /* PA2=[11:8]=7, PA3=[15:12]=7 */
    ASSERT_EQ_U32(0x00000000UL, GPIOA->AFR[1]);        /* AFRH untouched */

    /* Configure PB7 (I2C1_SDA) AF4 Open-Drain Very-High speed Pull-Up, and PA15 (I2C1_SCL) AF4 */
    ASSERT_TRUE(GPIO_ConfigAF(GPIOB, 7U, 4U, GPIO_OTYPE_OPENDRAIN,
                              GPIO_SPEED_VERY_HIGH, GPIO_PULL_UP));
    ASSERT_EQ_U32(1UL << 7U, GPIOB->OTYPER);
    ASSERT_EQ_U32(0x3U, (GPIOB->OSPEEDR >> 14U) & 0x3U);
    ASSERT_EQ_U32(0x1U, (GPIOB->PUPDR >> 14U) & 0x3U);
    ASSERT_EQ_U32(0x40000000UL, GPIOB->AFR[0]);        /* PB7=[31:28]=4 */
    ASSERT_EQ_U32(0x2U, (GPIOB->MODER >> 14U) & 0x3U); /* PB7=AF */
    ASSERT_EQ_U32(0x2U, (GPIOB->MODER >> 6U) & 0x3U);  /* PB3 SWO still AF! */

    /* Configure PA15 (AFRH nibble [31:28]) and verify AFRL untouched & PA13/14 SWD intact */
    ASSERT_TRUE(GPIO_ConfigAF(GPIOA, 15U, 4U, GPIO_OTYPE_OPENDRAIN,
                              GPIO_SPEED_HIGH, GPIO_PULL_UP));
    ASSERT_EQ_U32(0x40000000UL, GPIOA->AFR[1]);
    ASSERT_EQ_U32(0x00007700UL, GPIOA->AFR[0]);
    ASSERT_EQ_U32(0x2U, (GPIOA->MODER >> 26U) & 0x3U); /* PA13 SWDIO preserved */
    ASSERT_EQ_U32(0x2U, (GPIOA->MODER >> 28U) & 0x3U); /* PA14 SWCLK preserved */

    /* Re-park PA5 in Analog mode (low-power Stop state: MODER=11, PUPDR=00) */
    ASSERT_TRUE(GPIO_ConfigAnalog(GPIOA, 5U));
    ASSERT_EQ_U32(0x3U, (GPIOA->MODER >> 10U) & 0x3U);
    ASSERT_EQ_U32(0x0U, (GPIOA->PUPDR >> 10U) & 0x3U);

    /* Invalid arguments must be rejected without modifying registers */
    uint32_t saved_moder = GPIOA->MODER;
    ASSERT_TRUE(!GPIO_ConfigPin(NULL, NULL));
    ASSERT_TRUE(!GPIO_ConfigInput(NULL, 0U, GPIO_PULL_NONE));
    ASSERT_TRUE(!GPIO_ConfigInput(GPIOA, 16U, GPIO_PULL_NONE));
    ASSERT_TRUE(!GPIO_ConfigAF(GPIOA, 2U, 16U, GPIO_OTYPE_PUSHPULL,
                               GPIO_SPEED_LOW, GPIO_PULL_NONE));
    ASSERT_EQ_U32(saved_moder, GPIOA->MODER);
}

/* ============================================================================
 * 3. Atomic BSRR Set/Reset/Write, Sibling-Safe BSRR Toggle, IDR Read, and LCKR Lock Tests
 * ========================================================================== */
static void test_gpio_atomic_bsrr_and_lock(void) {
    mock_reset_silicon();

    /* Atomic set PA5 HIGH */
    GPIO_SetPin(GPIOA, 5U);
    ASSERT_EQ_U32(1UL << 5U, GPIOA->BSRR);
    mock_gpio_commit_bsrr(GPIOA);
    ASSERT_EQ_U32(1UL << 5U, GPIOA->ODR);

    /* Atomic set PA0 HIGH without disturbing PA5 */
    GPIO_WritePin(GPIOA, 0U, true);
    ASSERT_EQ_U32(1UL << 0U, GPIOA->BSRR);
    mock_gpio_commit_bsrr(GPIOA);
    ASSERT_EQ_U32((1UL << 5U) | (1UL << 0U), GPIOA->ODR);

    /* Atomic reset PA5 LOW via BSRR[21] without disturbing PA0 */
    GPIO_ResetPin(GPIOA, 5U);
    ASSERT_EQ_U32(1UL << 21U, GPIOA->BSRR);
    mock_gpio_commit_bsrr(GPIOA);
    ASSERT_EQ_U32(1UL << 0U, GPIOA->ODR);

    /* Sibling-safe toggle PA5 LOW -> HIGH -> LOW via ODR read + BSRR write (single-writer per pin) */
    GPIO_TogglePin(GPIOA, 5U);
    ASSERT_EQ_U32(1UL << 5U, GPIOA->BSRR);
    mock_gpio_commit_bsrr(GPIOA);
    ASSERT_EQ_U32((1UL << 5U) | (1UL << 0U), GPIOA->ODR);

    GPIO_TogglePin(GPIOA, 5U);
    ASSERT_EQ_U32(1UL << 21U, GPIOA->BSRR);
    mock_gpio_commit_bsrr(GPIOA);
    ASSERT_EQ_U32(1UL << 0U, GPIOA->ODR);

    /* GPIO_WritePin(..., false) uses upper 16-bit reset half */
    GPIO_WritePin(GPIOA, 0U, false);
    ASSERT_EQ_U32(1UL << 16U, GPIOA->BSRR);
    mock_gpio_commit_bsrr(GPIOA);
    ASSERT_EQ_U32(0x00000000UL, GPIOA->ODR);

    /* IDR input reading (PC13 Active-HIGH pressed vs released) */
    GPIOC->IDR = 0U;
    ASSERT_TRUE(!GPIO_ReadPin(GPIOC, 13U));
    GPIOC->IDR = (1UL << 13U);
    ASSERT_TRUE(GPIO_ReadPin(GPIOC, 13U));

    /* LCKR 4-step lock sequence, 2nd-pin rejection on locked port, & invalid-arg guards */
    ASSERT_TRUE(!GPIO_LockPin(NULL, 5U));
    ASSERT_TRUE(!GPIO_LockPin(GPIOA, 16U));
    ASSERT_TRUE(GPIO_LockPin(GPIOA, 5U));
    ASSERT_EQ_U32(GPIO_LCKR_LCKK | (1UL << 5U), GPIOA->LCKR);
    /* Re-locking already-locked pin 5 succeeds; attempting 2nd unlocked pin 6 MUST return false! */
    ASSERT_TRUE(GPIO_LockPin(GPIOA, 5U));
    ASSERT_TRUE(!GPIO_LockPin(GPIOA, 6U));
    ASSERT_EQ_U32(GPIO_LCKR_LCKK | (1UL << 5U), GPIOA->LCKR);
}

/* ============================================================================
 * 4. Fault Frame SRAM1+SRAM2 & CCM-SRAM Bounds Validator & SCB Decoder Tests
 * ========================================================================== */
static void test_fault_frame_bounds_and_scb_decode(void) {
    /* Valid SRAM1 + SRAM2 window: [0x20000000 .. 0x20017FE0] (4-byte aligned, >=32B headroom) */
    ASSERT_TRUE(fault_validate_stack_frame_addr(0x20000000UL));  /* Bottom of SRAM1 */
    ASSERT_TRUE(fault_validate_stack_frame_addr(0x20010000UL));  /* Mid SRAM1 */
    ASSERT_TRUE(fault_validate_stack_frame_addr(0x20014000UL));  /* Start of SRAM2 */
    ASSERT_TRUE(fault_validate_stack_frame_addr(0x20017FE0UL));  /* Exact top minus 32B frame */

    /* Valid CCM-SRAM window: [0x10000000 .. 0x10007FE0] */
    ASSERT_TRUE(fault_validate_stack_frame_addr(0x10000000UL));  /* Bottom of CCM-SRAM */
    ASSERT_TRUE(fault_validate_stack_frame_addr(0x10007FE0UL));  /* Exact top minus 32B frame */

    /* Invalid / overflowed / unaligned stack pointers (must be rejected to prevent Lockup!) */
    ASSERT_TRUE(!fault_validate_stack_frame_addr(0x1FFFFFFCUL)); /* MSP stack overflow below SRAM1 */
    ASSERT_TRUE(!fault_validate_stack_frame_addr(0x20017FE4UL)); /* Only 28B left (< 32B frame) */
    ASSERT_TRUE(!fault_validate_stack_frame_addr(0x20018000UL)); /* _estack top (0B headroom) */
    ASSERT_TRUE(!fault_validate_stack_frame_addr(0x10007FE4UL)); /* CCM partial overflow */
    ASSERT_TRUE(!fault_validate_stack_frame_addr(0x10008000UL)); /* End of CCM-SRAM */
    ASSERT_TRUE(!fault_validate_stack_frame_addr(0x20000002UL)); /* Unaligned (mod 4 != 0) */
    ASSERT_TRUE(!fault_validate_stack_frame_addr(0x08000000UL)); /* Flash memory */
    ASSERT_TRUE(!fault_validate_stack_frame_addr(0x40000000UL)); /* Peripheral APB space */
    ASSERT_TRUE(!fault_validate_stack_frame_addr(0x00000000UL)); /* NULL pointer */

    /* SCB CFSR / HFSR bitfield decoding (matching fault_handler.c) */
    ASSERT_EQ_U32(FAULT_FLAG_NONE, fault_decode_scb_flags(0U, 0U));

    uint32_t div0_flags = fault_decode_scb_flags(SCB_CFSR_DIVBYZERO_Msk, SCB_HFSR_FORCED_Msk);
    ASSERT_EQ_U32(FAULT_FLAG_USAGEFAULT | FAULT_FLAG_DIVBYZERO | FAULT_FLAG_FORCED_HF, div0_flags);

    uint32_t bus_flags = fault_decode_scb_flags(SCB_CFSR_BFARVALID_Msk | (1UL << 9U), 0U);
    ASSERT_EQ_U32(FAULT_FLAG_BUSFAULT | FAULT_FLAG_BFAR_VALID, bus_flags);

    uint32_t mem_flags = fault_decode_scb_flags(SCB_CFSR_MMARVALID_Msk | (1UL << 1U),
                                                SCB_HFSR_VECTTBL_Msk);
    ASSERT_EQ_U32(FAULT_FLAG_MEMMANAGE | FAULT_FLAG_MMFAR_VALID | FAULT_FLAG_VECTTBL_HF,
                  mem_flags);

    uint32_t usg_all = fault_decode_scb_flags(
        SCB_CFSR_UNALIGNED_Msk | SCB_CFSR_UNDEFINSTR_Msk | SCB_CFSR_INVSTATE_Msk, 0U);
    ASSERT_EQ_U32(FAULT_FLAG_USAGEFAULT | FAULT_FLAG_UNALIGNED |
                  FAULT_FLAG_UNDEFINSTR | FAULT_FLAG_INVSTATE, usg_all);
}

int main(void) {
    printf("Running STM32G474RE GPIO Mock & Fault Bounds Unit Tests...\n");
    test_gpio_enable_clock_all_ports();
    test_gpio_reset_state_and_bitfield_isolation();
    test_gpio_atomic_bsrr_and_lock();
    test_fault_frame_bounds_and_scb_decode();

    printf("Results: %d assertions executed, %d failed.\n", g_tests_run, g_tests_failed);
    return (g_tests_failed == 0) ? 0 : 1;
}
