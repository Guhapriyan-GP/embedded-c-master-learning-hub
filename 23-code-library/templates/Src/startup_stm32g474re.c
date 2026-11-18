/**
 * @file    startup_stm32g474re.c
 * @brief   Bare-metal C startup code and complete 118-entry NVIC vector table
 *          for STM32G474RET6 (ARM Cortex-M4F, NUCLEO-G474RE).
 *
 * Vector table layout verified 1:1 against CMSIS stm32g474xx.h (IRQn_Type 0..101)
 * and RM0440 Table 111:
 *   - 16 ARMv7-M core exception slots (indices 0..15)
 *   - 102 STM32G474RE external IRQ slots (IRQn 0..101; IRQn 85 reserved)
 *   - Total: 118 entries = 472 bytes (0x1D8), aligned to 512 bytes (0x200) for VTOR.
 *
 * Boot sequence in Reset_Handler:
 *   1. Copy .ccmram from Flash LMA (_siccmram) to CCM-SRAM VMA (_sccmram.._eccmram)
 *   2. Copy .data   from Flash LMA (_sidata)   to SRAM VMA     (_sdata.._edata)
 *   3. Zero .bss    in SRAM        (_sbss.._ebss) [leaves .noinit untouched]
 *   4. Call CMSIS SystemInit() (enables FPv4-SP-D16 FPU with DSB/ISB barriers)
 *   5. Call __libc_init_array() (C/newlib constructors)
 *   6. Branch to main()
 */

#include <stdbool.h>
#include <stdint.h>

/* Linker-exported memory boundary symbols */
extern uint32_t _estack;
extern uint32_t _sidata;
extern uint32_t _sdata;
extern uint32_t _edata;
extern uint32_t _siccmram;
extern uint32_t _sccmram;
extern uint32_t _eccmram;
extern uint32_t _sbss;
extern uint32_t _ebss;

/* External startup hooks */
extern void SystemInit(void);
extern void __libc_init_array(void);
extern int  main(void);

/* ISO C11 pedantic-clean vector table entry (zero void*-to-fn-ptr casts) */
typedef void (*isr_fn_t)(void);

typedef union {
    void    *stack_top;
    isr_fn_t handler;
} vector_entry_t;

/* Cortex-M4 CoreDebug DHCSR address & C_DEBUGEN bit (ARMv7-M Architecture Reference Manual) */
#define COREDEBUG_DHCSR_ADDR    (0xE000EDF0UL)
#define COREDEBUG_DHCSR_DEBUGEN (1UL << 0U)

/* Valid STM32G474RE RAM bounds for 8-word (32-byte) exception stack frames */
#define STARTUP_SRAM_BASE       (0x20000000UL)
#define STARTUP_SRAM_END        (0x20018000UL)
#define STARTUP_CCMRAM_BASE     (0x10000000UL)
#define STARTUP_CCMRAM_END      (0x10008000UL)

/* Default trap handler for unhandled interrupts.
 * Only executes bkpt #0 when a hardware debugger is attached (C_DEBUGEN == 1)
 * so standalone firmware never escalates into a spurious HardFault. */
void Default_Handler(void) {
    if ((*(volatile const uint32_t *)COREDEBUG_DHCSR_ADDR & COREDEBUG_DHCSR_DEBUGEN) != 0UL) {
        __asm__ volatile("bkpt #0");
    }
    while (1) {
        __asm__ volatile("nop");
    }
}

/* Weak C HardFault/Fault handler (overridden by Drivers/fault_handler.c when linked) */
void HardFault_Handler_C(uint32_t *stacked_frame) __attribute__((weak, used));

void HardFault_Handler_C(uint32_t *stacked_frame) {
    /* Minimal standalone fallback persistent capture in .noinit (overridden by fault_handler.c) */
    static volatile uint32_t s_fallback_frame[8] __attribute__((section(".noinit.fallback")));
    uintptr_t sp_addr = (uintptr_t)stacked_frame;
    bool in_sram = (sp_addr >= STARTUP_SRAM_BASE) && (sp_addr <= (STARTUP_SRAM_END - 32UL));
    bool in_ccm  = (sp_addr >= STARTUP_CCMRAM_BASE) && (sp_addr <= (STARTUP_CCMRAM_END - 32UL));
    if (((sp_addr & 3UL) == 0UL) && (in_sram || in_ccm)) {
        for (uint32_t i = 0U; i < 8U; ++i) {
            s_fallback_frame[i] = stacked_frame[i];
        }
    } else {
        for (uint32_t i = 0U; i < 8U; ++i) {
            s_fallback_frame[i] = 0xDEAD57ACUL;
        }
    }
    (void)s_fallback_frame;
    if ((*(volatile const uint32_t *)COREDEBUG_DHCSR_ADDR & COREDEBUG_DHCSR_DEBUGEN) != 0UL) {
        __asm__ volatile("bkpt #0");
    }
    while (1) {
        __asm__ volatile("nop");
    }
}

/* Shared naked-asm fault trampoline: selects MSP or PSP via EXC_RETURN bit 2,
 * passes stacked exception frame pointer in R0 to HardFault_Handler_C(). */
void Fault_Trampoline(void)             __attribute__((naked));

void Fault_Trampoline(void) {
    __asm__ volatile(
        "tst   lr, #4              \n\t"
        "ite   eq                  \n\t"
        "mrseq r0, msp             \n\t"
        "mrsne r0, psp             \n\t"
        "b     HardFault_Handler_C \n\t"
    );
}

/* Cortex-M4 core exception prototypes.
 * HardFault, MemManage, BusFault, and UsageFault all route through Fault_Trampoline
 * so enabling detailed faults in SCB->SHCSR always captures the true faulting frame
 * in g_crash_log instead of halting in Default_Handler. */
void Reset_Handler(void);
void NMI_Handler(void)                  __attribute__((weak, alias("Default_Handler")));
void HardFault_Handler(void)            __attribute__((weak, alias("Fault_Trampoline")));
void MemManage_Handler(void)            __attribute__((weak, alias("Fault_Trampoline")));
void BusFault_Handler(void)             __attribute__((weak, alias("Fault_Trampoline")));
void UsageFault_Handler(void)           __attribute__((weak, alias("Fault_Trampoline")));
void SVC_Handler(void)                  __attribute__((weak, alias("Default_Handler")));
void DebugMon_Handler(void)             __attribute__((weak, alias("Default_Handler")));
void PendSV_Handler(void)               __attribute__((weak, alias("Default_Handler")));
void SysTick_Handler(void)              __attribute__((weak, alias("Default_Handler")));

/* Complete STM32G474RE peripheral IRQ handler prototypes (IRQn 0..101) */
void WWDG_IRQHandler(void)                  __attribute__((weak, alias("Default_Handler"))); /*   0 */
void PVD_PVM_IRQHandler(void)               __attribute__((weak, alias("Default_Handler"))); /*   1 */
void RTC_TAMP_LSECSS_IRQHandler(void)       __attribute__((weak, alias("Default_Handler"))); /*   2 */
void RTC_WKUP_IRQHandler(void)              __attribute__((weak, alias("Default_Handler"))); /*   3 */
void FLASH_IRQHandler(void)                 __attribute__((weak, alias("Default_Handler"))); /*   4 */
void RCC_IRQHandler(void)                   __attribute__((weak, alias("Default_Handler"))); /*   5 */
void EXTI0_IRQHandler(void)                 __attribute__((weak, alias("Default_Handler"))); /*   6 */
void EXTI1_IRQHandler(void)                 __attribute__((weak, alias("Default_Handler"))); /*   7 */
void EXTI2_IRQHandler(void)                 __attribute__((weak, alias("Default_Handler"))); /*   8 */
void EXTI3_IRQHandler(void)                 __attribute__((weak, alias("Default_Handler"))); /*   9 */
void EXTI4_IRQHandler(void)                 __attribute__((weak, alias("Default_Handler"))); /*  10 */
void DMA1_Channel1_IRQHandler(void)         __attribute__((weak, alias("Default_Handler"))); /*  11 */
void DMA1_Channel2_IRQHandler(void)         __attribute__((weak, alias("Default_Handler"))); /*  12 */
void DMA1_Channel3_IRQHandler(void)         __attribute__((weak, alias("Default_Handler"))); /*  13 */
void DMA1_Channel4_IRQHandler(void)         __attribute__((weak, alias("Default_Handler"))); /*  14 */
void DMA1_Channel5_IRQHandler(void)         __attribute__((weak, alias("Default_Handler"))); /*  15 */
void DMA1_Channel6_IRQHandler(void)         __attribute__((weak, alias("Default_Handler"))); /*  16 */
void DMA1_Channel7_IRQHandler(void)         __attribute__((weak, alias("Default_Handler"))); /*  17 */
void ADC1_2_IRQHandler(void)                __attribute__((weak, alias("Default_Handler"))); /*  18 */
void USB_HP_IRQHandler(void)                __attribute__((weak, alias("Default_Handler"))); /*  19 */
void USB_LP_IRQHandler(void)                __attribute__((weak, alias("Default_Handler"))); /*  20 */
void FDCAN1_IT0_IRQHandler(void)            __attribute__((weak, alias("Default_Handler"))); /*  21 */
void FDCAN1_IT1_IRQHandler(void)            __attribute__((weak, alias("Default_Handler"))); /*  22 */
void EXTI9_5_IRQHandler(void)               __attribute__((weak, alias("Default_Handler"))); /*  23 */
void TIM1_BRK_TIM15_IRQHandler(void)        __attribute__((weak, alias("Default_Handler"))); /*  24 */
void TIM1_UP_TIM16_IRQHandler(void)         __attribute__((weak, alias("Default_Handler"))); /*  25 */
void TIM1_TRG_COM_TIM17_IRQHandler(void)    __attribute__((weak, alias("Default_Handler"))); /*  26 */
void TIM1_CC_IRQHandler(void)               __attribute__((weak, alias("Default_Handler"))); /*  27 */
void TIM2_IRQHandler(void)                  __attribute__((weak, alias("Default_Handler"))); /*  28 */
void TIM3_IRQHandler(void)                  __attribute__((weak, alias("Default_Handler"))); /*  29 */
void TIM4_IRQHandler(void)                  __attribute__((weak, alias("Default_Handler"))); /*  30 */
void I2C1_EV_IRQHandler(void)               __attribute__((weak, alias("Default_Handler"))); /*  31 */
void I2C1_ER_IRQHandler(void)               __attribute__((weak, alias("Default_Handler"))); /*  32 */
void I2C2_EV_IRQHandler(void)               __attribute__((weak, alias("Default_Handler"))); /*  33 */
void I2C2_ER_IRQHandler(void)               __attribute__((weak, alias("Default_Handler"))); /*  34 */
void SPI1_IRQHandler(void)                  __attribute__((weak, alias("Default_Handler"))); /*  35 */
void SPI2_IRQHandler(void)                  __attribute__((weak, alias("Default_Handler"))); /*  36 */
void USART1_IRQHandler(void)                __attribute__((weak, alias("Default_Handler"))); /*  37 */
void USART2_IRQHandler(void)                __attribute__((weak, alias("Default_Handler"))); /*  38 */
void USART3_IRQHandler(void)                __attribute__((weak, alias("Default_Handler"))); /*  39 */
void EXTI15_10_IRQHandler(void)             __attribute__((weak, alias("Default_Handler"))); /*  40 */
void RTC_Alarm_IRQHandler(void)             __attribute__((weak, alias("Default_Handler"))); /*  41 */
void USBWakeUp_IRQHandler(void)             __attribute__((weak, alias("Default_Handler"))); /*  42 */
void TIM8_BRK_IRQHandler(void)              __attribute__((weak, alias("Default_Handler"))); /*  43 */
void TIM8_UP_IRQHandler(void)               __attribute__((weak, alias("Default_Handler"))); /*  44 */
void TIM8_TRG_COM_IRQHandler(void)          __attribute__((weak, alias("Default_Handler"))); /*  45 */
void TIM8_CC_IRQHandler(void)               __attribute__((weak, alias("Default_Handler"))); /*  46 */
void ADC3_IRQHandler(void)                  __attribute__((weak, alias("Default_Handler"))); /*  47 */
void FMC_IRQHandler(void)                   __attribute__((weak, alias("Default_Handler"))); /*  48 */
void LPTIM1_IRQHandler(void)                __attribute__((weak, alias("Default_Handler"))); /*  49 */
void TIM5_IRQHandler(void)                  __attribute__((weak, alias("Default_Handler"))); /*  50 */
void SPI3_IRQHandler(void)                  __attribute__((weak, alias("Default_Handler"))); /*  51 */
void UART4_IRQHandler(void)                 __attribute__((weak, alias("Default_Handler"))); /*  52 */
void UART5_IRQHandler(void)                 __attribute__((weak, alias("Default_Handler"))); /*  53 */
void TIM6_DAC_IRQHandler(void)              __attribute__((weak, alias("Default_Handler"))); /*  54 */
void TIM7_DAC_IRQHandler(void)              __attribute__((weak, alias("Default_Handler"))); /*  55 */
void DMA2_Channel1_IRQHandler(void)         __attribute__((weak, alias("Default_Handler"))); /*  56 */
void DMA2_Channel2_IRQHandler(void)         __attribute__((weak, alias("Default_Handler"))); /*  57 */
void DMA2_Channel3_IRQHandler(void)         __attribute__((weak, alias("Default_Handler"))); /*  58 */
void DMA2_Channel4_IRQHandler(void)         __attribute__((weak, alias("Default_Handler"))); /*  59 */
void DMA2_Channel5_IRQHandler(void)         __attribute__((weak, alias("Default_Handler"))); /*  60 */
void ADC4_IRQHandler(void)                  __attribute__((weak, alias("Default_Handler"))); /*  61 */
void ADC5_IRQHandler(void)                  __attribute__((weak, alias("Default_Handler"))); /*  62 */
void UCPD1_IRQHandler(void)                 __attribute__((weak, alias("Default_Handler"))); /*  63 */
void COMP1_2_3_IRQHandler(void)             __attribute__((weak, alias("Default_Handler"))); /*  64 */
void COMP4_5_6_IRQHandler(void)             __attribute__((weak, alias("Default_Handler"))); /*  65 */
void COMP7_IRQHandler(void)                 __attribute__((weak, alias("Default_Handler"))); /*  66 */
void HRTIM1_Master_IRQHandler(void)         __attribute__((weak, alias("Default_Handler"))); /*  67 */
void HRTIM1_TIMA_IRQHandler(void)           __attribute__((weak, alias("Default_Handler"))); /*  68 */
void HRTIM1_TIMB_IRQHandler(void)           __attribute__((weak, alias("Default_Handler"))); /*  69 */
void HRTIM1_TIMC_IRQHandler(void)           __attribute__((weak, alias("Default_Handler"))); /*  70 */
void HRTIM1_TIMD_IRQHandler(void)           __attribute__((weak, alias("Default_Handler"))); /*  71 */
void HRTIM1_TIME_IRQHandler(void)           __attribute__((weak, alias("Default_Handler"))); /*  72 */
void HRTIM1_FLT_IRQHandler(void)            __attribute__((weak, alias("Default_Handler"))); /*  73 */
void HRTIM1_TIMF_IRQHandler(void)           __attribute__((weak, alias("Default_Handler"))); /*  74 */
void CRS_IRQHandler(void)                   __attribute__((weak, alias("Default_Handler"))); /*  75 */
void SAI1_IRQHandler(void)                  __attribute__((weak, alias("Default_Handler"))); /*  76 */
void TIM20_BRK_IRQHandler(void)             __attribute__((weak, alias("Default_Handler"))); /*  77 */
void TIM20_UP_IRQHandler(void)              __attribute__((weak, alias("Default_Handler"))); /*  78 */
void TIM20_TRG_COM_IRQHandler(void)         __attribute__((weak, alias("Default_Handler"))); /*  79 */
void TIM20_CC_IRQHandler(void)              __attribute__((weak, alias("Default_Handler"))); /*  80 */
void FPU_IRQHandler(void)                   __attribute__((weak, alias("Default_Handler"))); /*  81 */
void I2C4_EV_IRQHandler(void)               __attribute__((weak, alias("Default_Handler"))); /*  82 */
void I2C4_ER_IRQHandler(void)               __attribute__((weak, alias("Default_Handler"))); /*  83 */
void SPI4_IRQHandler(void)                  __attribute__((weak, alias("Default_Handler"))); /*  84 */
/* IRQn 85 is Reserved on STM32G474xx (AES exists on STM32G483/484 only) */
void FDCAN2_IT0_IRQHandler(void)            __attribute__((weak, alias("Default_Handler"))); /*  86 */
void FDCAN2_IT1_IRQHandler(void)            __attribute__((weak, alias("Default_Handler"))); /*  87 */
void FDCAN3_IT0_IRQHandler(void)            __attribute__((weak, alias("Default_Handler"))); /*  88 */
void FDCAN3_IT1_IRQHandler(void)            __attribute__((weak, alias("Default_Handler"))); /*  89 */
void RNG_IRQHandler(void)                   __attribute__((weak, alias("Default_Handler"))); /*  90 */
void LPUART1_IRQHandler(void)               __attribute__((weak, alias("Default_Handler"))); /*  91 */
void I2C3_EV_IRQHandler(void)               __attribute__((weak, alias("Default_Handler"))); /*  92 */
void I2C3_ER_IRQHandler(void)               __attribute__((weak, alias("Default_Handler"))); /*  93 */
void DMAMUX_OVR_IRQHandler(void)            __attribute__((weak, alias("Default_Handler"))); /*  94 */
void QUADSPI_IRQHandler(void)               __attribute__((weak, alias("Default_Handler"))); /*  95 */
void DMA1_Channel8_IRQHandler(void)         __attribute__((weak, alias("Default_Handler"))); /*  96 */
void DMA2_Channel6_IRQHandler(void)         __attribute__((weak, alias("Default_Handler"))); /*  97 */
void DMA2_Channel7_IRQHandler(void)         __attribute__((weak, alias("Default_Handler"))); /*  98 */
void DMA2_Channel8_IRQHandler(void)         __attribute__((weak, alias("Default_Handler"))); /*  99 */
void CORDIC_IRQHandler(void)                __attribute__((weak, alias("Default_Handler"))); /* 100 */
void FMAC_IRQHandler(void)                  __attribute__((weak, alias("Default_Handler"))); /* 101 */

/* Complete 118-entry Vector Table (16 Core + 102 External IRQs = 472 bytes, 512-byte aligned) */
__attribute__((section(".isr_vector"), used, aligned(512)))
const vector_entry_t g_pfnVectors[118] = {
    /* Core Exceptions (Indices 0..15) */
    { .stack_top = &_estack },                      /*   0: Initial MSP (0x20018000)       */
    { .handler   = Reset_Handler },                 /*   1: Reset                          */
    { .handler   = NMI_Handler },                   /*   2: NMI                            */
    { .handler   = HardFault_Handler },             /*   3: HardFault                      */
    { .handler   = MemManage_Handler },             /*   4: MemManage                      */
    { .handler   = BusFault_Handler },              /*   5: BusFault                       */
    { .handler   = UsageFault_Handler },            /*   6: UsageFault                     */
    { .handler   = 0 },                             /*   7: Reserved                       */
    { .handler   = 0 },                             /*   8: Reserved                       */
    { .handler   = 0 },                             /*   9: Reserved                       */
    { .handler   = 0 },                             /*  10: Reserved                       */
    { .handler   = SVC_Handler },                   /*  11: SVCall                         */
    { .handler   = DebugMon_Handler },              /*  12: DebugMonitor                   */
    { .handler   = 0 },                             /*  13: Reserved                       */
    { .handler   = PendSV_Handler },                /*  14: PendSV                         */
    { .handler   = SysTick_Handler },               /*  15: SysTick                        */

    /* External Interrupts (IRQn 0..101 -> Vector Indices 16..117) */
    { .handler   = WWDG_IRQHandler },               /*  16 (IRQn   0): WWDG                */
    { .handler   = PVD_PVM_IRQHandler },            /*  17 (IRQn   1): PVD_PVM             */
    { .handler   = RTC_TAMP_LSECSS_IRQHandler },    /*  18 (IRQn   2): RTC_TAMP_LSECSS     */
    { .handler   = RTC_WKUP_IRQHandler },           /*  19 (IRQn   3): RTC_WKUP            */
    { .handler   = FLASH_IRQHandler },              /*  20 (IRQn   4): FLASH               */
    { .handler   = RCC_IRQHandler },                /*  21 (IRQn   5): RCC                 */
    { .handler   = EXTI0_IRQHandler },              /*  22 (IRQn   6): EXTI0               */
    { .handler   = EXTI1_IRQHandler },              /*  23 (IRQn   7): EXTI1               */
    { .handler   = EXTI2_IRQHandler },              /*  24 (IRQn   8): EXTI2               */
    { .handler   = EXTI3_IRQHandler },              /*  25 (IRQn   9): EXTI3               */
    { .handler   = EXTI4_IRQHandler },              /*  26 (IRQn  10): EXTI4               */
    { .handler   = DMA1_Channel1_IRQHandler },      /*  27 (IRQn  11): DMA1_Channel1       */
    { .handler   = DMA1_Channel2_IRQHandler },      /*  28 (IRQn  12): DMA1_Channel2       */
    { .handler   = DMA1_Channel3_IRQHandler },      /*  29 (IRQn  13): DMA1_Channel3       */
    { .handler   = DMA1_Channel4_IRQHandler },      /*  30 (IRQn  14): DMA1_Channel4       */
    { .handler   = DMA1_Channel5_IRQHandler },      /*  31 (IRQn  15): DMA1_Channel5       */
    { .handler   = DMA1_Channel6_IRQHandler },      /*  32 (IRQn  16): DMA1_Channel6       */
    { .handler   = DMA1_Channel7_IRQHandler },      /*  33 (IRQn  17): DMA1_Channel7       */
    { .handler   = ADC1_2_IRQHandler },             /*  34 (IRQn  18): ADC1_2              */
    { .handler   = USB_HP_IRQHandler },             /*  35 (IRQn  19): USB_HP              */
    { .handler   = USB_LP_IRQHandler },             /*  36 (IRQn  20): USB_LP              */
    { .handler   = FDCAN1_IT0_IRQHandler },         /*  37 (IRQn  21): FDCAN1_IT0          */
    { .handler   = FDCAN1_IT1_IRQHandler },         /*  38 (IRQn  22): FDCAN1_IT1          */
    { .handler   = EXTI9_5_IRQHandler },            /*  39 (IRQn  23): EXTI9_5             */
    { .handler   = TIM1_BRK_TIM15_IRQHandler },     /*  40 (IRQn  24): TIM1_BRK_TIM15      */
    { .handler   = TIM1_UP_TIM16_IRQHandler },      /*  41 (IRQn  25): TIM1_UP_TIM16       */
    { .handler   = TIM1_TRG_COM_TIM17_IRQHandler }, /*  42 (IRQn  26): TIM1_TRG_COM_TIM17  */
    { .handler   = TIM1_CC_IRQHandler },            /*  43 (IRQn  27): TIM1_CC             */
    { .handler   = TIM2_IRQHandler },               /*  44 (IRQn  28): TIM2                */
    { .handler   = TIM3_IRQHandler },               /*  45 (IRQn  29): TIM3                */
    { .handler   = TIM4_IRQHandler },               /*  46 (IRQn  30): TIM4                */
    { .handler   = I2C1_EV_IRQHandler },            /*  47 (IRQn  31): I2C1_EV             */
    { .handler   = I2C1_ER_IRQHandler },            /*  48 (IRQn  32): I2C1_ER             */
    { .handler   = I2C2_EV_IRQHandler },            /*  49 (IRQn  33): I2C2_EV             */
    { .handler   = I2C2_ER_IRQHandler },            /*  50 (IRQn  34): I2C2_ER             */
    { .handler   = SPI1_IRQHandler },               /*  51 (IRQn  35): SPI1                */
    { .handler   = SPI2_IRQHandler },               /*  52 (IRQn  36): SPI2                */
    { .handler   = USART1_IRQHandler },             /*  53 (IRQn  37): USART1              */
    { .handler   = USART2_IRQHandler },             /*  54 (IRQn  38): USART2              */
    { .handler   = USART3_IRQHandler },             /*  55 (IRQn  39): USART3              */
    { .handler   = EXTI15_10_IRQHandler },          /*  56 (IRQn  40): EXTI15_10           */
    { .handler   = RTC_Alarm_IRQHandler },          /*  57 (IRQn  41): RTC_Alarm           */
    { .handler   = USBWakeUp_IRQHandler },          /*  58 (IRQn  42): USBWakeUp           */
    { .handler   = TIM8_BRK_IRQHandler },           /*  59 (IRQn  43): TIM8_BRK            */
    { .handler   = TIM8_UP_IRQHandler },            /*  60 (IRQn  44): TIM8_UP             */
    { .handler   = TIM8_TRG_COM_IRQHandler },       /*  61 (IRQn  45): TIM8_TRG_COM        */
    { .handler   = TIM8_CC_IRQHandler },            /*  62 (IRQn  46): TIM8_CC             */
    { .handler   = ADC3_IRQHandler },               /*  63 (IRQn  47): ADC3                */
    { .handler   = FMC_IRQHandler },                /*  64 (IRQn  48): FMC (unbonded RE)   */
    { .handler   = LPTIM1_IRQHandler },             /*  65 (IRQn  49): LPTIM1              */
    { .handler   = TIM5_IRQHandler },               /*  66 (IRQn  50): TIM5                */
    { .handler   = SPI3_IRQHandler },               /*  67 (IRQn  51): SPI3                */
    { .handler   = UART4_IRQHandler },              /*  68 (IRQn  52): UART4               */
    { .handler   = UART5_IRQHandler },              /*  69 (IRQn  53): UART5               */
    { .handler   = TIM6_DAC_IRQHandler },           /*  70 (IRQn  54): TIM6_DAC            */
    { .handler   = TIM7_DAC_IRQHandler },           /*  71 (IRQn  55): TIM7_DAC            */
    { .handler   = DMA2_Channel1_IRQHandler },      /*  72 (IRQn  56): DMA2_Channel1       */
    { .handler   = DMA2_Channel2_IRQHandler },      /*  73 (IRQn  57): DMA2_Channel2       */
    { .handler   = DMA2_Channel3_IRQHandler },      /*  74 (IRQn  58): DMA2_Channel3       */
    { .handler   = DMA2_Channel4_IRQHandler },      /*  75 (IRQn  59): DMA2_Channel4       */
    { .handler   = DMA2_Channel5_IRQHandler },      /*  76 (IRQn  60): DMA2_Channel5       */
    { .handler   = ADC4_IRQHandler },               /*  77 (IRQn  61): ADC4                */
    { .handler   = ADC5_IRQHandler },               /*  78 (IRQn  62): ADC5                */
    { .handler   = UCPD1_IRQHandler },              /*  79 (IRQn  63): UCPD1               */
    { .handler   = COMP1_2_3_IRQHandler },          /*  80 (IRQn  64): COMP1_2_3           */
    { .handler   = COMP4_5_6_IRQHandler },          /*  81 (IRQn  65): COMP4_5_6           */
    { .handler   = COMP7_IRQHandler },              /*  82 (IRQn  66): COMP7               */
    { .handler   = HRTIM1_Master_IRQHandler },      /*  83 (IRQn  67): HRTIM1_Master       */
    { .handler   = HRTIM1_TIMA_IRQHandler },        /*  84 (IRQn  68): HRTIM1_TIMA         */
    { .handler   = HRTIM1_TIMB_IRQHandler },        /*  85 (IRQn  69): HRTIM1_TIMB         */
    { .handler   = HRTIM1_TIMC_IRQHandler },        /*  86 (IRQn  70): HRTIM1_TIMC         */
    { .handler   = HRTIM1_TIMD_IRQHandler },        /*  87 (IRQn  71): HRTIM1_TIMD         */
    { .handler   = HRTIM1_TIME_IRQHandler },        /*  88 (IRQn  72): HRTIM1_TIME         */
    { .handler   = HRTIM1_FLT_IRQHandler },         /*  89 (IRQn  73): HRTIM1_FLT          */
    { .handler   = HRTIM1_TIMF_IRQHandler },        /*  90 (IRQn  74): HRTIM1_TIMF         */
    { .handler   = CRS_IRQHandler },                /*  91 (IRQn  75): CRS                 */
    { .handler   = SAI1_IRQHandler },               /*  92 (IRQn  76): SAI1                */
    { .handler   = TIM20_BRK_IRQHandler },          /*  93 (IRQn  77): TIM20_BRK           */
    { .handler   = TIM20_UP_IRQHandler },           /*  94 (IRQn  78): TIM20_UP            */
    { .handler   = TIM20_TRG_COM_IRQHandler },      /*  95 (IRQn  79): TIM20_TRG_COM       */
    { .handler   = TIM20_CC_IRQHandler },           /*  96 (IRQn  80): TIM20_CC            */
    { .handler   = FPU_IRQHandler },                /*  97 (IRQn  81): FPU                 */
    { .handler   = I2C4_EV_IRQHandler },            /*  98 (IRQn  82): I2C4_EV             */
    { .handler   = I2C4_ER_IRQHandler },            /*  99 (IRQn  83): I2C4_ER             */
    { .handler   = SPI4_IRQHandler },               /* 100 (IRQn  84): SPI4 (unbonded RE)  */
    { .handler   = 0 },                             /* 101 (IRQn  85): Reserved            */
    { .handler   = FDCAN2_IT0_IRQHandler },         /* 102 (IRQn  86): FDCAN2_IT0          */
    { .handler   = FDCAN2_IT1_IRQHandler },         /* 103 (IRQn  87): FDCAN2_IT1          */
    { .handler   = FDCAN3_IT0_IRQHandler },         /* 104 (IRQn  88): FDCAN3_IT0          */
    { .handler   = FDCAN3_IT1_IRQHandler },         /* 105 (IRQn  89): FDCAN3_IT1          */
    { .handler   = RNG_IRQHandler },                /* 106 (IRQn  90): RNG                 */
    { .handler   = LPUART1_IRQHandler },            /* 107 (IRQn  91): LPUART1             */
    { .handler   = I2C3_EV_IRQHandler },            /* 108 (IRQn  92): I2C3_EV             */
    { .handler   = I2C3_ER_IRQHandler },            /* 109 (IRQn  93): I2C3_ER             */
    { .handler   = DMAMUX_OVR_IRQHandler },         /* 110 (IRQn  94): DMAMUX_OVR          */
    { .handler   = QUADSPI_IRQHandler },            /* 111 (IRQn  95): QUADSPI             */
    { .handler   = DMA1_Channel8_IRQHandler },      /* 112 (IRQn  96): DMA1_Channel8       */
    { .handler   = DMA2_Channel6_IRQHandler },      /* 113 (IRQn  97): DMA2_Channel6       */
    { .handler   = DMA2_Channel7_IRQHandler },      /* 114 (IRQn  98): DMA2_Channel7       */
    { .handler   = DMA2_Channel8_IRQHandler },      /* 115 (IRQn  99): DMA2_Channel8       */
    { .handler   = CORDIC_IRQHandler },             /* 116 (IRQn 100): CORDIC              */
    { .handler   = FMAC_IRQHandler }                /* 117 (IRQn 101): FMAC                */
};

_Static_assert(sizeof(g_pfnVectors) == (118U * 4U),
               "STM32G474RE vector table must be exactly 472 bytes (118 entries)");

/* Reset Handler: prevent GCC -O2 from synthesizing libc memcpy/memset before C runtime init */
__attribute__((optimize("no-tree-loop-distribute-patterns")))
void Reset_Handler(void) {
    /* 1. Copy .ccmram (zero-wait-state code/data) from FLASH LMA to CCM-SRAM (0x10000000) */
    uint32_t *pSrc  = &_siccmram;
    uint32_t *pDest = &_sccmram;
    while (pDest < &_eccmram) {
        *pDest++ = *pSrc++;
    }

    /* 2. Copy initialized .data from FLASH LMA to SRAM VMA */
    pSrc  = &_sidata;
    pDest = &_sdata;
    while (pDest < &_edata) {
        *pDest++ = *pSrc++;
    }

    /* 3. Zero-initialize .bss in SRAM (.noinit is intentionally preserved across reset) */
    pDest = &_sbss;
    while (pDest < &_ebss) {
        *pDest++ = 0UL;
    }

    /* 4. Initialize MCU core (FPU access with DSB/ISB barriers, VTOR if configured) */
    SystemInit();

    /* 5. Execute C/newlib static constructors (.preinit_array / .init_array) */
    __libc_init_array();

    /* 6. Enter application */
    (void)main();

    /* Trap if main() ever returns */
    while (1) {
        __asm__ volatile("bkpt #0");
    }
}
