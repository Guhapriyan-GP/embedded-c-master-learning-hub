/**
 * @file    fault_handler.c
 * @brief   ARM Cortex-M4 HardFault decoder and persistent .noinit crash logger.
 *
 * Verified against CMSIS core_cm4.h (SCB_Type) and PM0214 Section 4.4.10..4.4.14.
 */

#include "fault_handler.h"
#include "stm32g474xx.h"

__attribute__((section(".noinit"), used))
volatile crash_log_t g_crash_log;

void FaultHandler_EnableDetailedFaults(bool trap_div_by_zero) {
    /* Enable MemManage, BusFault, and UsageFault handlers so they do not
     * escalate to HardFault unless disabled or occurring at equal/higher priority */
    SCB->SHCSR |= (SCB_SHCSR_MEMFAULTENA_Msk |
                   SCB_SHCSR_BUSFAULTENA_Msk |
                   SCB_SHCSR_USGFAULTENA_Msk);

    if (trap_div_by_zero) {
        SCB->CCR |= SCB_CCR_DIV_0_TRP_Msk;
    }
    __DSB();
    __ISB();
}

bool FaultHandler_HasValidCrashLog(void) {
    return (g_crash_log.magic == CRASH_LOG_MAGIC);
}

void FaultHandler_ClearCrashLog(void) {
    g_crash_log.magic = 0U;
}

#define FAULT_INVALID_STACK_SENTINEL 0xDEAD57ACUL

static bool fault_is_valid_stack_frame(uintptr_t sp_addr) {
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
void HardFault_Handler_C(uint32_t *stacked_frame) {
    uint32_t cfsr  = SCB->CFSR;
    uint32_t hfsr  = SCB->HFSR;
    uint32_t flags = FAULT_FLAG_NONE;
    uintptr_t sp_addr = (uintptr_t)stacked_frame;

    /* Capture SCB fault status/address registers FIRST before touching stacked_frame */
    g_crash_log.cfsr       = cfsr;
    g_crash_log.hfsr       = hfsr;
    g_crash_log.dfsr       = SCB->DFSR;
    g_crash_log.afsr       = SCB->AFSR;
    g_crash_log.mmfar      = SCB->MMFAR;
    g_crash_log.bfar       = SCB->BFAR;
    g_crash_log.stacked_sp = (uint32_t)sp_addr;

    /* Validate stacked_frame lies inside STM32G474RE SRAM1+SRAM2 or CCM-SRAM
     * so an MSP stack overflow below 0x20000000 never causes a secondary BusFault -> Lockup */
    if (fault_is_valid_stack_frame(sp_addr)) {
        g_crash_log.r0   = stacked_frame[0];
        g_crash_log.r1   = stacked_frame[1];
        g_crash_log.r2   = stacked_frame[2];
        g_crash_log.r3   = stacked_frame[3];
        g_crash_log.r12  = stacked_frame[4];
        g_crash_log.lr   = stacked_frame[5];
        g_crash_log.pc   = stacked_frame[6];
        g_crash_log.xpsr = stacked_frame[7];
    } else {
        g_crash_log.r0   = FAULT_INVALID_STACK_SENTINEL;
        g_crash_log.r1   = FAULT_INVALID_STACK_SENTINEL;
        g_crash_log.r2   = FAULT_INVALID_STACK_SENTINEL;
        g_crash_log.r3   = FAULT_INVALID_STACK_SENTINEL;
        g_crash_log.r12  = FAULT_INVALID_STACK_SENTINEL;
        g_crash_log.lr   = FAULT_INVALID_STACK_SENTINEL;
        g_crash_log.pc   = FAULT_INVALID_STACK_SENTINEL;
        g_crash_log.xpsr = FAULT_INVALID_STACK_SENTINEL;
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

    g_crash_log.fault_flags = flags;
    __DSB();
    g_crash_log.magic = CRASH_LOG_MAGIC;
    __DSB();

    /* Halt debugger if attached; otherwise spin safely for post-mortem inspection */
    if ((CoreDebug->DHCSR & CoreDebug_DHCSR_C_DEBUGEN_Msk) != 0U) {
        __asm__ volatile("bkpt #0");
    }

    while (1) {
        __asm__ volatile("nop");
    }
}
