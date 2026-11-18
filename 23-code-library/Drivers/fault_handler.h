/**
 * @file    fault_handler.h
 * @brief   ARM Cortex-M4 HardFault post-mortem crash record in persistent .noinit SRAM.
 *
 * Captures hardware-stacked exception frame (R0-R3, R12, LR, PC, xPSR) along with
 * SCB fault status/address registers (CFSR, HFSR, DFSR, AFSR, MMFAR, BFAR) and
 * decoded fault category flags into `g_crash_log`, surviving soft/watchdog resets.
 *
 * Reference: PM0214 Section 4.4.10..4.4.14 (SCB Fault Status Registers).
 * Concepts:  ARM-EXCP-001, TOOL-DBG-002, REL-FAULT-001
 */

#ifndef FAULT_HANDLER_H
#define FAULT_HANDLER_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define CRASH_LOG_MAGIC 0xFA017C0DU

/** Decoded high-level fault flags extracted from SCB->CFSR & SCB->HFSR */
typedef enum {
    FAULT_FLAG_NONE        = 0U,
    FAULT_FLAG_MEMMANAGE   = (1U << 0),
    FAULT_FLAG_BUSFAULT    = (1U << 1),
    FAULT_FLAG_USAGEFAULT  = (1U << 2),
    FAULT_FLAG_FORCED_HF   = (1U << 3),
    FAULT_FLAG_VECTTBL_HF  = (1U << 4),
    FAULT_FLAG_MMFAR_VALID = (1U << 5),
    FAULT_FLAG_BFAR_VALID  = (1U << 6),
    FAULT_FLAG_DIVBYZERO   = (1U << 7),
    FAULT_FLAG_UNALIGNED   = (1U << 8),
    FAULT_FLAG_UNDEFINSTR  = (1U << 9),
    FAULT_FLAG_INVSTATE    = (1U << 10)
} fault_flags_t;

/** Persistent crash log placed in .noinit SRAM (survives non-POR reset) */
typedef struct {
    uint32_t magic;       /**< Set to CRASH_LOG_MAGIC when record is valid */
    uint32_t r0;
    uint32_t r1;
    uint32_t r2;
    uint32_t r3;
    uint32_t r12;
    uint32_t lr;          /**< Link Register at fault entry                */
    uint32_t pc;          /**< Faulting instruction address (Return PC)    */
    uint32_t xpsr;        /**< Program Status Register                     */
    uint32_t cfsr;        /**< SCB->CFSR (MMFSR[7:0]|BFSR[15:8]|UFSR[31:16]) */
    uint32_t hfsr;        /**< SCB->HFSR (HardFault Status Register)       */
    uint32_t dfsr;        /**< SCB->DFSR (Debug Fault Status Register)     */
    uint32_t afsr;        /**< SCB->AFSR (Auxiliary Fault Status Register) */
    uint32_t mmfar;       /**< SCB->MMFAR (valid if MMARVALID set in CFSR) */
    uint32_t bfar;        /**< SCB->BFAR  (valid if BFARVALID set in CFSR) */
    uint32_t fault_flags; /**< Bitmask of fault_flags_t                    */
    uint32_t stacked_sp;  /**< Captured MSP or PSP frame base address      */
} crash_log_t;

/** Global persistent crash log stored in .noinit SRAM */
extern volatile crash_log_t g_crash_log;

/**
 * @brief Enable MemManage, BusFault, UsageFault, and optionally divide-by-zero trapping.
 * @param trap_div_by_zero  If true, sets SCB->CCR.DIV_0_TRP so integer /0 triggers UsageFault.
 */
void FaultHandler_EnableDetailedFaults(bool trap_div_by_zero);

/**
 * @brief Returns true if a valid crash record from a previous HardFault is stored in .noinit.
 */
bool FaultHandler_HasValidCrashLog(void);

/**
 * @brief Invalidate the persistent crash log after inspecting/reporting it on boot.
 */
void FaultHandler_ClearCrashLog(void);

/**
 * @brief C HardFault entry called from startup_stm32g474re.c naked trampoline.
 * @param stacked_frame Pointer to the 8-word Cortex-M4 hardware exception stack frame.
 */
void HardFault_Handler_C(uint32_t *stacked_frame);

#ifdef __cplusplus
}
#endif

#endif /* FAULT_HANDLER_H */
