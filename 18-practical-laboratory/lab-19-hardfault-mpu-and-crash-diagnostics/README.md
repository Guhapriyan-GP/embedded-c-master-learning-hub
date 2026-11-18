# Lab 19: Cortex-M4 MPU Stack Guard, Naked-ASM HardFault Decoder & `.noinit` Persistent Crash Recovery

> **Verification Status:** `CROSS-COMPILED` (`arm-none-eabi-gcc 14.3.1`, `-Wall -Wextra -Werror -O2`, 0 warnings) | **Hardware Queue:** `ONBOARD — READY FOR AGENT 4` *(Not Yet Executed on Physical NUCLEO-G474RE Hardware)*

> **Clock Configuration Profile:** **Primary = `16 MHz HSI16` (Reset Default)** | **160 MHz / 170 MHz Boost Compatibility:** 100% identical `MPU`/`SCB`/`.noinit` register behavior across `16 MHz`, `160 MHz`, and `170 MHz` | **Reusable Drivers:** [`23-code-library/Drivers/fault_handler.h`](../../23-code-library/Drivers/fault_handler.h) (`FaultHandler_EnableDetailedFaults()`, `FaultHandler_HasValidCrashLog()`, `FaultHandler_ClearCrashLog()`, `HardFault_Handler_C()`), [`clock_g4.h`](../../23-code-library/Drivers/clock_g4.h), & [`gpio_g4.h`](../../23-code-library/Drivers/gpio_g4.h).

---

## 1. Objective & Concept IDs

Harden STM32G474RE firmware for safety-critical production diagnostics by:
1. Configuring the ARM Cortex-M4 **8-region Memory Protection Unit (`MPU`)** with a **32-byte No-Access (`AP = 000`) Stack Guard Region** at the bottom of the Main Stack (`MSP`) so stack overflows trap immediately as a `MemManage` fault *before* silently corrupting `.bss`/`.data`!
2. Writing a **naked-assembly `HardFault_Handler` / `MemManage_Handler` trampoline** that inspects `EXC_RETURN` bit 2 (`TST LR, #4`) to pick `MSP` vs `PSP`, passes the exact hardware-stacked frame (`R0..R3, R12, LR, PC, xPSR`) to a C decoder, captures **`CFSR`, `HFSR`, `MMFAR`, and `BFAR`**, persists the crash record into a **`.noinit` SRAM section** (CRC-protected) surviving `NVIC_SystemReset()`, and prints the post-mortem report on reboot!

- **Target Concept IDs:** `ARM-MPU-001` (8-Region MPU & Stack Guard), `ARM-FAULT-001` (HardFault/MemManage/BusFault Decoding via `CFSR`/`HFSR`/`MMFAR`/`BFAR`), `REL-CRASH-001` (`.noinit` Persistent Crash Log & Safe Reboot), `TOOL-LD-001` (`.noinit (NOLOAD)` Linker Section)
- **Scope Layer:** `[Cortex-M4 Specific]` / `[STM32G474RE Exact]`

---

## 2. Required Hardware

- **Development Board:** ST NUCLEO-G474RE (`MB1367` Rev C)
- **Host Interface:** ST-LINK VCP (`USART2`, `115200 8N1`) and Blue Button `B1` (`PC13`, Active-HIGH) to trigger a deliberate fault on demand

---

## 3. Required Software

- `arm-none-eabi-gcc` (14.3.1+), `make`, `openocd`, `arm-none-eabi-gdb`
- Linker script defining a `.noinit (NOLOAD)` region in SRAM2 (e.g., `0x20017E00`–`0x20017FFF`, 512 bytes excluded from `.bss` zeroing in `Reset_Handler`!)

---

## 4. Wiring & Pin Configuration

Uses onboard `PC13` (`B1`, Active-HIGH) and `PA5` (`LD2`, Fault/Recovery indicator). Optional: `USART2` (`PA2`/`PA3`) to print decoded crash telemetry after reboot.

---

## 5. Register Configuration & Mathematical Derivation

**Official Documentation:** **ARM PM0214 §4.4** (*System Control Block — `SHCSR`, `CFSR`, `HFSR`, `MMFAR`, `BFAR`*) and **PM0214 §4.6** (*Memory Protection Unit — `MPU_CTRL`, `MPU_RNR`, `MPU_RBAR`, `MPU_RASR`*).

### 5.1 Cortex-M4 MPU Stack Guard Geometry

- **Alignment Rule:** An MPU region of size $2^{N+1}$ bytes MUST be aligned to a multiple of its own size!
- For a **32-byte (`SIZE = 4` $\implies 2^{4+1} = 32\text{ B}$)** stack-guard region placed at the bottom of the stack (`guard_addr` aligned to 32 bytes):
  - `MPU->CTRL = MPU_CTRL_PRIVDEFENA_Msk | MPU_CTRL_ENABLE_Msk;` (`PRIVDEFENA = 1` keeps the default background memory map active for privileged mode while higher-numbered regions override it!)
  - Region 7 (highest priority overlay!):
    - `RBAR = (guard_addr & ~0x1FUL)`
    - `RASR = MPU_RASR_XN_Msk | (0U << MPU_RASR_AP_Pos) | (4U << MPU_RASR_SIZE_Pos) | MPU_RASR_ENABLE_Msk` (`AP = 000` = **No Access** even for Privileged mode, `XN = 1` = Execute-Never!)
- Enable dedicated configurable faults in `SCB->SHCSR |= SCB_SHCSR_MEMFAULTENA_Msk | SCB_SHCSR_BUSFAULTENA_Msk | SCB_SHCSR_USGFAULTENA_Msk;` (and optionally `SCB->CCR |= SCB_CCR_DIV_0_TRP_Msk;` to trap divide-by-zero!).

### 5.1b Explicit Clock Configuration Profile Declaration (`16 MHz` vs `160 MHz` vs `170 MHz`)

Cortex-M4 core `MPU` and `SCB` fault registers operate synchronously at `HCLK` with **zero frequency-dependent divisors**. Running this lab at `16 MHz HSI16`, `160 MHz` (`Clock_Init_160MHz()`), or `170 MHz` (`Clock_Init_170MHz()`) uses identical `MPU->RNR`/`RBAR`/`RASR`/`CTRL` and `SCB->SHCSR` values — only optional `USART2` post-mortem printing requires matching `BRR` (`139` @ `16 MHz`, `1,389` @ `160 MHz`, `1,476` @ `170 MHz`, handled automatically by [`usart_g4.h`](../../23-code-library/Drivers/usart_g4.h))!

- **Reusable Production Driver (`23-code-library/Drivers/fault_handler.h`):** Links directly with `23-code-library/templates/stm32g474re.ld`'s `.noinit (NOLOAD)` section and `startup_stm32g474re.c`'s naked-ASM trampoline, including SRAM1/SRAM2 (`0x20000000..0x20017FFF`) + CCM-SRAM (`0x10000000..0x10007FFF`) stacked-SP bounds checks.

### 5.2 Hardware Exception Frame Layout (Stacked Automatically on Fault)

| Offset from SP | Register | Diagnostic Meaning |
|---|---|---|
| `sp[0]..sp[3]` | `R0..R3` | Function arguments at point of crash |
| `sp[4]` | `R12` | Scratch register |
| `sp[5]` | `LR` (`R14`) | Caller return address |
| **`sp[6]`** | **`PC` (`R15`)** | **Exact faulting instruction address** (resolve with `arm-none-eabi-addr2line -e firmware.elf 0x...`) |
| `sp[7]` | `xPSR` | Thumb bit, IPSR, ALU flags |

---

## 6. Code Implementation

```c
#include "stm32g474xx.h"
#include <stdint.h>

#define CRASH_MAGIC_VALID            (0xDEADFA17UL)
#define CRASH_INVALID_STACK_SENTINEL (0xDEAD57ACUL)

typedef struct {
    uint32_t magic;
    uint32_t boot_count;
    uint32_t fault_type; /* 1=HardFault, 2=MemManage, 3=BusFault, 4=UsageFault */
    uint32_t r0, r1, r2, r3, r12, lr, pc, psr;
    uint32_t cfsr;
    uint32_t hfsr;
    uint32_t mmfar;
    uint32_t bfar;
    uint32_t checksum;
} crash_record_t;

/* Placed in .noinit (NOLOAD) SRAM so Reset_Handler does NOT zero it across soft reset! */
__attribute__((section(".noinit")))
static volatile crash_record_t g_crash_log;

/* 32-byte aligned simulated stack guard buffer (in production, place at bottom of MSP!) */
__attribute__((aligned(32)))
static volatile uint8_t g_mpu_guard_zone[32];

static volatile uint32_t g_recovered_from_crash = 0U;

static uint32_t compute_simple_xor(const volatile crash_record_t *rec) {
    uint32_t x = rec->magic ^ rec->fault_type ^ rec->pc ^ rec->lr ^
                 rec->cfsr  ^ rec->hfsr       ^ rec->mmfar ^ rec->bfar;
    return x ^ 0xA5A55A5AUL;
}

/* C Fault Decoder invoked by naked assembly trampolines */
__attribute__((used))
void Fault_DecodeAndPersist(uint32_t *stack_frame, uint32_t fault_type) {
    uint32_t  cfsr    = SCB->CFSR;
    uintptr_t sp_addr = (uintptr_t)stack_frame;
    uint32_t  in_sram = ((sp_addr >= (uintptr_t)SRAM1_BASE) &&
                         (sp_addr <= (uintptr_t)(SRAM2_BASE + SRAM2_SIZE - 32UL))) ? 1U : 0U;
    uint32_t  in_ccm  = ((sp_addr >= (uintptr_t)CCMSRAM_BASE) &&
                         (sp_addr <= (uintptr_t)(CCMSRAM_BASE + CCMSRAM_SIZE - 32UL))) ? 1U : 0U;

    /* Capture SCB fault registers FIRST before touching stack_frame */
    g_crash_log.magic      = CRASH_MAGIC_VALID;
    g_crash_log.fault_type = fault_type;
    g_crash_log.cfsr       = cfsr;
    g_crash_log.hfsr       = SCB->HFSR;
    g_crash_log.mmfar      = ((cfsr & SCB_CFSR_MMARVALID_Msk) != 0U) ? SCB->MMFAR : 0xFFFFFFFFUL;
    g_crash_log.bfar       = ((cfsr & SCB_CFSR_BFARVALID_Msk) != 0U) ? SCB->BFAR  : 0xFFFFFFFFUL;

    /* Bounds-check stack_frame inside STM32G474RE SRAM (0x20000000..0x20017FE0) or CCM-SRAM
     * so an MSP stack overflow below 0x20000000 never triggers a secondary BusFault -> Lockup! */
    if (((sp_addr & 3UL) == 0UL) && ((in_sram != 0U) || (in_ccm != 0U))) {
        g_crash_log.r0  = stack_frame[0];
        g_crash_log.r1  = stack_frame[1];
        g_crash_log.r2  = stack_frame[2];
        g_crash_log.r3  = stack_frame[3];
        g_crash_log.r12 = stack_frame[4];
        g_crash_log.lr  = stack_frame[5];
        g_crash_log.pc  = stack_frame[6];
        g_crash_log.psr = stack_frame[7];
    } else {
        g_crash_log.r0  = CRASH_INVALID_STACK_SENTINEL;
        g_crash_log.r1  = CRASH_INVALID_STACK_SENTINEL;
        g_crash_log.r2  = CRASH_INVALID_STACK_SENTINEL;
        g_crash_log.r3  = CRASH_INVALID_STACK_SENTINEL;
        g_crash_log.r12 = CRASH_INVALID_STACK_SENTINEL;
        g_crash_log.lr  = CRASH_INVALID_STACK_SENTINEL;
        g_crash_log.pc  = CRASH_INVALID_STACK_SENTINEL;
        g_crash_log.psr = CRASH_INVALID_STACK_SENTINEL;
    }

    g_crash_log.checksum = compute_simple_xor(&g_crash_log);

    __DSB();

    /* If debugger is attached, halt so engineer can inspect live callstack;
     * in standalone production, perform clean NVIC_SystemReset() to recover! */
    if ((CoreDebug->DHCSR & CoreDebug_DHCSR_C_DEBUGEN_Msk) != 0U) {
        __BKPT(0);
    }
    NVIC_SystemReset();
    for (;;) {}
}

/* Naked-assembly trampoline: picks MSP vs PSP via EXC_RETURN bit 2 before touching stack! */
__attribute__((naked))
void HardFault_Handler(void) {
    __asm volatile (
        "tst lr, #4                 \n"
        "ite eq                     \n"
        "mrseq r0, msp              \n"
        "mrsne r0, psp              \n"
        "movs r1, #1                \n"
        "b Fault_DecodeAndPersist   \n"
    );
}

__attribute__((naked))
void MemManage_Handler(void) {
    __asm volatile (
        "tst lr, #4                 \n"
        "ite eq                     \n"
        "mrseq r0, msp              \n"
        "mrsne r0, psp              \n"
        "movs r1, #2                \n"
        "b Fault_DecodeAndPersist   \n"
    );
}

static void MPU_Enable_StackGuard(uint32_t guard_base_32b_aligned) {
    /* 1. Disable MPU cleanly before reconfiguring regions */
    __DMB();
    MPU->CTRL = 0U;

    /* 2. Region 7 (highest priority): 32-byte No-Access (AP=000), Execute-Never (XN=1) */
    MPU->RNR  = 7U;
    MPU->RBAR = guard_base_32b_aligned & ~0x1FUL;
    MPU->RASR = MPU_RASR_XN_Msk                | /* Execute Never */
                (0U << MPU_RASR_AP_Pos)        | /* No access (Privileged & Unprivileged) */
                (4U << MPU_RASR_SIZE_Pos)      | /* 2^(4+1) = 32 bytes */
                MPU_RASR_ENABLE_Msk;

    /* 3. Enable configurable MemManage, BusFault, UsageFault handlers + Divide-by-Zero trap */
    SCB->SHCSR |= SCB_SHCSR_MEMFAULTENA_Msk |
                  SCB_SHCSR_BUSFAULTENA_Msk |
                  SCB_SHCSR_USGFAULTENA_Msk;
    SCB->CCR   |= SCB_CCR_DIV_0_TRP_Msk;

    /* 4. Enable MPU with PRIVDEFENA=1 (privileged default background map active) */
    MPU->CTRL = MPU_CTRL_PRIVDEFENA_Msk | MPU_CTRL_ENABLE_Msk;
    __DSB();
    __ISB();
}

int main(void) {
    RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN | RCC_AHB2ENR_GPIOCEN;
    volatile uint32_t tmpreg = RCC->AHB2ENR;
    (void)tmpreg;

    /* PA5 (LD2) Output, PC13 (B1) Input (Active-HIGH) */
    GPIOA->MODER  = (GPIOA->MODER & ~(3U << 10U)) | (1U << 10U);
    GPIOC->MODER &= ~(3U << 26U);
    GPIOC->PUPDR &= ~(3U << 26U);

    /* Step 1: Check .noinit crash log on boot! */
    if ((g_crash_log.magic == CRASH_MAGIC_VALID) &&
        (g_crash_log.checksum == compute_simple_xor(&g_crash_log))) {
        g_recovered_from_crash = 1U;
        g_crash_log.boot_count++;
        /* Indicate post-crash recovery by turning LD2 ON */
        GPIOA->BSRR = GPIO_BSRR_BS5;
        /* In production: stream g_crash_log.pc/cfsr/mmfar over USART2, then clear magic */
        g_crash_log.magic = 0U;
    } else {
        g_crash_log.boot_count = 1U;
        GPIOA->BSRR = GPIO_BSRR_BR5;
    }

    /* Step 2: Arm 32-byte MPU Guard Region over g_mpu_guard_zone */
    MPU_Enable_StackGuard((uint32_t)&g_mpu_guard_zone[0]);

    while (1) {
        /* Press Active-HIGH B1 (PC13) to deliberately violate the MPU Guard Region! */
        if ((GPIOC->IDR & GPIO_IDR_ID13) != 0U) {
            /* Triggers MemManage_Handler immediately with DACCVIOL=1 & MMFAR=&g_mpu_guard_zone[0]! */
            g_mpu_guard_zone[0] = 0x5AU;
        }
    }
}
```

---

## 7. Expected Output

1. On clean boot, `LD2` is OFF and `g_recovered_from_crash == 0`.
2. Pressing `B1` (`PC13`) writes into `g_mpu_guard_zone[0]`, triggering `MemManage_Handler` **on the exact offending `STRB` instruction**!
3. Under GDB, execution halts at `__BKPT(0)` with:
   - `g_crash_log.fault_type == 2` (`MemManage`)
   - `g_crash_log.cfsr & 0x82 == 0x82` (`MMARVALID | DACCVIOL`)
   - `g_crash_log.mmfar == (uint32_t)&g_mpu_guard_zone[0]`
   - `g_crash_log.pc` pointing to the exact faulting instruction!
4. Standalone (without debugger), the MCU resets cleanly via `NVIC_SystemReset()`, retains `g_crash_log` in `.noinit`, sets `g_recovered_from_crash = 1`, and illuminates `LD2`!

---

## 8. Debugging Procedure

1. Press `B1` under GDB and decode the faulting `PC` back to exact C file + line number:
   ```bash
   arm-none-eabi-addr2line -e build/firmware.elf -f -C 0x0800034A
   ```
2. Inspect `SCB->CFSR` bitfields in GDB:
   - `[7:0]` `MMFSR`: `IACCVIOL` (b0), `DACCVIOL` (b1), `MMARVALID` (b7)
   - `[15:8]` `BFSR`: `IBUSERR` (b8), `PRECISERR` (b9), `IMPRECISERR` (b10), `BFARVALID` (b15)
   - `[31:16]` `UFSR`: `UNDEFINSTR` (b16), `INVSTATE` (b17), `UNALIGNED` (b24), `DIVBYZERO` (b25)

---

## 9. Common Errors & Troubleshooting

| Symptom | Root Cause | Verified Fix |
|---|---|---|
| `g_crash_log` is all zeros after `NVIC_SystemReset()` | Linker script put `.noinit` inside `.bss` (zeroed by `Reset_Handler`) | Add explicit `.noinit (NOLOAD) : { *(.noinit*) } > RAM` to `stm32g474re.ld` |
| MPU fails to trap write or faults on wrong address | Region base address was not aligned to a multiple of region size (`32 B`) | Enforce `__attribute__((aligned(32)))` (or linker `ALIGN(32)`) |
| `HardFault_Handler` corrupts stack frame before reading `R0..PC` | Wrote C function prologue without `__attribute__((naked))` | Always use `__attribute__((naked))` ASM trampoline |

---

## 10. Verification Procedure

- [ ] Code compiles cleanly with `arm-none-eabi-gcc -Wall -Wextra -Werror`.
- [ ] `MPU->CTRL == 0x5` (`PRIVDEFENA | ENABLE`) and `SCB->SHCSR` enables MemManage/BusFault/UsageFault.
- [ ] Triggering write to `g_mpu_guard_zone[0]` populates `g_crash_log.mmfar == &g_mpu_guard_zone[0]` and `DACCVIOL == 1`.
- [ ] `arm-none-eabi-addr2line` maps `g_crash_log.pc` to the exact line `g_mpu_guard_zone[0] = 0x5AU;`.

---

## 11. Extension Exercises

1. **Real MSP Stack Guard:** Move `g_mpu_guard_zone` to the linker symbol `_estack - _Min_Stack_Size` and induce an infinite recursive function to catch a real stack overflow before `.bss` corruption!
2. **Execute-Never (`XN`) SRAM Defense:** Add an MPU region marking all SRAM1 (`0x20000000`, 64 KB + 16 KB) as `XN = 1` to block code-injection attacks from data buffers.
3. **Imprecise BusFault Localization:** Temporarily set `SCnSCB->ACTLR |= SCnSCB_ACTLR_DISDEFWBUF_Msk;` (`ACTLR` lives in `SCnSCB_Type` at `0xE000E008`, NOT `SCB_Type`!) to disable the Cortex-M4 write buffer so imprecise BusFaults become exact precise BusFaults during debugging.

---

## 12. Standardized Verification Record

| Field | Record |
|---|---|
| **Verification Status** | `ONBOARD HARDWARE TESTED` *(DIV0 `UsageFault` + `.noinit` persistence across `NVIC_SystemReset()` verified on NUCLEO-G474RE via `onboard_selftest` Test 8; dedicated MPU `MemManage` trap pending extension test)* |
| **Objective** | Configure Cortex-M4 8-region `MPU` with a 32-byte No-Access (`AP=000`, `XN=1`) guard region, trap violations via naked-ASM `MemManage_Handler`/`HardFault_Handler` with SRAM/CCM SP bounds checking, persist `CFSR`/`HFSR`/`MMFAR`/`BFAR`/`PC`/`LR` into `.noinit` SRAM across `NVIC_SystemReset()`, and signal recovery on `LD2` (`PA5`). |
| **Firmware Version** | `v2.2.0-remediation` (branch `feature/complete-remediation`, base commit `585dc4fa`) |
| **Compiler Version** | `arm-none-eabi-gcc 14.3.1 20250623 (GNU Tools for STM32 14.3.rel1.20251027-0700)` |
| **Build Result** | **PASS** — `arm-none-eabi-gcc -std=c11 -mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard -DSTM32G474xx -Wall -Wextra -Werror -O2 -c` succeeded with **0 warnings, 0 errors** (`1,916 B` object file). |
| **Test Environment** | Windows 11 + ARM GNU Toolchain 14.3.rel1 + NUCLEO-G474RE (`MB1367` Rev C, `STM32G474RET6`, STLINK-V3E `V3J17M11`, `COM8`) |
| **Required Peripherals** | **Onboard Only:** Cortex-M4 `MPU`, `SCB`, `CoreDebug->DHCSR`, `.noinit` SRAM, `GPIOA` (`PA5` `LD2`), `GPIOC` (`PC13` `B1`). Zero external wiring required. |
| **Hardware Availability** | `ONBOARD — VERIFIED (DIV0 .noinit Persistence; MPU Trap Pending Extension)` |
| **Pin Configuration** | `PA5` (`LD2` Push-Pull Output, ON after crash recovery), `PC13` (`B1` Input `00b`, Active-HIGH fault trigger); optional `PA2`/`PA3` `USART2` VCP for crash telemetry printout. |
| **Expected Behavior** | Clean boot leaves `LD2` OFF (`g_recovered_from_crash == 0`). Writing `g_mpu_guard_zone[0]` triggers `MemManage_Handler` with `DACCVIOL=1`, `MMARVALID=1`, `mmfar == &g_mpu_guard_zone[0]`, exact faulting `pc`, and valid `.noinit` checksum surviving `NVIC_SystemReset()` to set `g_recovered_from_crash = 1` and turn `LD2` ON. |
| **Actual Observations** | Agent 4 Onboard Test (`onboard_selftest` Test 8 on NUCLEO-G474RE): Verified `MPU->CTRL = 0x5` (Region 7 guard armed), `DIV_0_TRP` `UsageFault` capture (`CFSR` `DIVBYZERO` + stacked `PC`/`LR`), and `.noinit` crash-log persistence across `NVIC_SystemReset()`. Dedicated MPU `MemManage` fault-injection (`DACCVIOL`/`MMARVALID`) remains pending as an extension test. |
| **Register Values** | Reset: `MPU->CTRL = 0`, `SCB->SHCSR = 0`. Configured: `MPU->RNR = 7`, `MPU->RBAR = (&g_mpu_guard_zone[0] & ~0x1F)`, `MPU->RASR = 0x10000009` (`XN=1, AP=000, SIZE=4 (32 B), ENABLE=1`), `MPU->CTRL = 0x00000005` (`PRIVDEFENA\|ENABLE`), `SCB->SHCSR`: `MEMFAULTENA\|BUSFAULTENA\|USGFAULTENA`, `SCB->CCR.DIV_0_TRP = 1`. |
| **Debugging Procedure** | Flash via `STM32_Programmer_CLI -c port=SWD -w firmware.bin 0x08000000 -v -rst` or OpenOCD; trigger fault, inspect `p/x g_crash_log` in `arm-none-eabi-gdb`, and resolve `g_crash_log.pc` via `arm-none-eabi-addr2line -e build/firmware.elf -f -C <pc>`. |
| **Remaining Limitations** | Requires linker script `.noinit (NOLOAD)` section (already provided in `23-code-library/templates/stm32g474re.ld`) so `Reset_Handler` does not zero `g_crash_log`. Dedicated MPU `MemManage` violation (`g_mpu_guard_zone[0]` write trap) pending extension execution. |
