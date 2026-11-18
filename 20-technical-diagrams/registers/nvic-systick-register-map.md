# ARM Cortex-M4 Core Peripherals Register Map (NVIC, SysTick, SCB, MPU & DWT)

**Core Architecture:** **ARMv7E-M** (ARM Cortex-M4F r0p1: `__CM4_REV = 0x0001U`, `__FPU_PRESENT = 1U`, `__MPU_PRESENT = 1U`)  
**STM32G474RE Silicon Instantiation:** **102 external IRQs** (`IRQ 0..101`, IRQ 85 reserved) and **4 priority bits** (`__NVIC_PRIO_BITS = 4U` → 16 levels) *(Generic Cortex-M4 supports up to 240 IRQs and 3–8 priority bits)*  
**Base Addresses (PPB):** `SCS` `0xE000E000` | `SysTick` `0xE000E010` | `NVIC` `0xE000E100` | `SCB` `0xE000ED00` | `MPU` `0xE000ED90` | `CoreDebug` `0xE000EDF0` | `FPU` `0xE000EF30` | `DWT` `0xE0001000`  
**Reference Manuals:** ARM DDI 0403E.e (*ARMv7-M Architecture RM*), ST PM0214, `core_cm4.h`, `mpu_armv7.h`  
**Concept IDs:** `ARM-NVIC-001`, `ARM-SYST-001`, `ARM-FLT-001`, `ARM-MPU-001`, `ARM-FPU-001`, `ARM-DBG-001`, `PROF-WCET-001`  
**Verification Status:** Every register offset verified directly against `core_cm4.h`; code snippets Statically Reviewed (Not Tested on Physical Hardware).

---

### 1. SysTick (24-Bit System Timer) — Base `0xE000E010` (`SysTick_Type`)

| Register | Offset | Reset Value | Access | Description |
|---|---|---|---|---|
| `SysTick->CTRL` | `0x000` | `0x00000000` | RW | `ENABLE` (b0), `TICKINT` (b1), `CLKSOURCE` (b2: `0`=HCLK/8, `1`=HCLK), `COUNTFLAG` (b16, cleared on read or write to `VAL`) |
| `SysTick->LOAD` | `0x004` | Unknown | RW | 24-bit reload value (`0x00000001..0x00FFFFFF`; max period at 170 MHz = 98.69 ms) |
| `SysTick->VAL` | `0x008` | Unknown | RW | 24-bit current value (writing any value clears counter & `COUNTFLAG`) |
| `SysTick->CALIB` | `0x00C` | Impl-defined | RO | `TENMS[23:0]` (10 ms reload value), `SKEW` (b30), `NOREF` (b31) |

---

### 2. NVIC (Nested Vectored Interrupt Controller) — Base `0xE000E100` (`NVIC_Type`)

| Register Group | Offset | Access | Description |
|---|---|---|---|
| `NVIC->ISER[0..7]` | `0x000` | RW | Interrupt Set-Enable (Write `1` to enable IRQ; `0` ignored) |
| `NVIC->ICER[0..7]` | `0x080` | RW | Interrupt Clear-Enable (Write `1` to disable IRQ) |
| `NVIC->ISPR[0..7]` | `0x100` | RW | Interrupt Set-Pending (Trigger IRQ in software) |
| `NVIC->ICPR[0..7]` | `0x180` | RW | Interrupt Clear-Pending |
| `NVIC->IABR[0..7]` | `0x200` | RO | Interrupt Active Bit (Indicates currently executing or preempted active ISR) |
| `NVIC->IP[0..239]` | `0x300` | RW | Interrupt Priority byte array (maps 60 × 32-bit hardware `IPR[0..59]`; on STM32G474RE bits `[7:4]` active, `[3:0]` tied 0) |
| `NVIC->STIR` | `0xE00` | WO | Software Trigger Interrupt Register (Base `0xE000EF00`; write IRQ number `0..101`) |

> **FreeRTOS Requirement:** Always call `NVIC_SetPriorityGrouping(0U)` (or `3U` — all 4 bits preemption, 0 bits subpriority) before `vTaskStartScheduler()`, and keep `BASEPRI` (`configMAX_SYSCALL_INTERRUPT_PRIORITY`) aligned to bits `[7:4]`.

---

### 3. SCB (System Control Block — Faults, Vector Table & FPU) — Base `0xE000ED00` (`SCB_Type`)

| Register | Offset | Access | Key Bitfields & Engineering Purpose |
|---|---|---|---|
| `SCB->CPUID` | `0x000` | RO | Reads `0x410FC241` on STM32G474RE (ARM Cortex-M4 r0p1) |
| `SCB->ICSR` | `0x004` | RW | `PENDSVSET` (b28), `PENDSVCLR` (b27), `PENDSTSET` (b26), `VECTACTIVE[8:0]` |
| `SCB->VTOR` | `0x008` | RW | Vector Table Offset (**512-byte `0x200` aligned** on STM32G474RE: 118 vectors × 4 B = 472 B) |
| `SCB->AIRCR` | `0x00C` | RW | `VECTKEY` (`0x05FA` bits `[31:16]`), `PRIGROUP[10:8]`, `SYSRESETREQ` (b2) |
| `SCB->SCR` | `0x010` | RW | `SLEEPONEXIT` (b1), `SLEEPDEEP` (b2), `SEVONPEND` (b4) |
| `SCB->CCR` | `0x014` | RW | `UNALIGN_TRP` (b3), `DIV_0_TRP` (b4), `STKALIGN` (b9 = 1 default) |
| `SCB->SHP[0..11]` | `0x018` | RW | System Handler Priorities (MemManage, BusFault, UsageFault, SVCall, PendSV `[10]`, SysTick `[11]`) |
| **`SCB->SHCSR`** | `0x024` | RW | **`MEMFAULTENA` (b16), `BUSFAULTENA` (b17), `USGFAULTENA` (b18)** — enable configurable faults so they don't escalate to HardFault! |
| **`SCB->CFSR`** | `0x028` | rc_w1 | Combined Configurable Fault Status: **`MMFSR[7:0]`** (`IACCVIOL`, `DACCVIOL`, `MMARVALID` b7), **`BFSR[15:8]`** (`IBUSERR`, `PRECISERR`, `IMPRECISERR`, `BFARVALID` b15), **`UFSR[31:16]`** (`UNDEFINSTR`, `INVSTATE`, `NOCP`, `UNALIGNED`, `DIVBYZERO`) |
| **`SCB->HFSR`** | `0x02C` | rc_w1 | HardFault Status: `VECTTBL` (b1), **`FORCED` (b30** = escalated configurable fault), `DEBUGEVT` (b31) |
| **`SCB->MMFAR`** | `0x034` | RW | MemManage Fault Address (valid **only** when `CFSR.MMARVALID == 1`) |
| **`SCB->BFAR`** | `0x038` | RW | BusFault Address (valid **only** when `CFSR.BFARVALID == 1`) |
| **`SCB->CPACR`** | `0x088` | RW | Coprocessor Access Control: set **`CP10`/`CP11 = 0b11` (`0x00F00000UL`) + `__DSB(); __ISB();`** to enable FPv4-SP FPU! |

---

### 4. MPU (8-Region Memory Protection Unit) — Base `0xE000ED90` (`MPU_Type`)

| Register | Offset | Access | Key Bitfields |
|---|---|---|---|
| `MPU->TYPE` | `0x000` | RO | `DREGION[15:8]` reads `8` on STM32G474RE |
| `MPU->CTRL` | `0x004` | RW | `ENABLE` (b0), `HFNMIENA` (b1), **`PRIVDEFENA` (b2** = default memory map for privileged code) |
| **`MPU->RNR`** | `0x008` | RW | Region Number selector (`0..7`) |
| **`MPU->RBAR`** | `0x00C` | RW | `ADDR[31:5]` (size-aligned base address), `VALID` (b4), `REGION[3:0]` |
| **`MPU->RASR`** | `0x010` | RW | `ENABLE` (b0), `SIZE[5:1]` ($\text{Region Bytes} = 2^{\text{SIZE}+1}$, min `4` = 32 B), `SRD[15:8]` (subregion disable), `B/C/S/TEX` `[21:16]`, `AP[26:24]`, **`XN` (b28 Execute-Never)** |

---

### 5. DWT (Data Watchpoint & Trace Cycle Counter) — Base `0xE0001000` (`DWT_Type`)

| Register | Address | Access | Purpose |
|---|---|---|---|
| `CoreDebug->DEMCR` | `0xE000EDFC` | RW | Set **`TRCENA` (bit 24)** first to power up DWT/ITM trace unit |
| `DWT->CTRL` | `0xE0001000` | RW | Set **`CYCCNTENA` (bit 0)** to start free-running 32-bit CPU cycle counter |
| **`DWT->CYCCNT`** | `0xE0001004` | RW | 32-bit exact CPU clock cycle counter (5.882 ns/tick @ 170 MHz; wraps every 25.26 s) |
| `DWT->COMP0..3` | `0xE0001020..` | RW | 4 hardware data watchpoint comparators (used by GDB `watch`) |

---

### 6. CMSIS Code Examples: DWT Cycle Profiling & MPU 32-Byte Stack Guard

> **Status:** Statically Reviewed (Not Tested on Physical Hardware).

```c
#include "stm32g474xx.h"

/* 1. Initialize 32-bit DWT->CYCCNT cycle counter (5.882 ns/cycle @ 170 MHz) */
void DWT_CycleCounter_Init(void) {
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CYCCNT       = 0U;
    DWT->CTRL        |= DWT_CTRL_CYCCNTENA_Msk;
}

/* 2. Enable Configurable Faults + 32-Byte No-Access MPU Stack Guard at guard_addr (32-byte aligned) */
void MPU_Setup_StackGuard(uint32_t guard_addr) {
    SCB->SHCSR |= SCB_SHCSR_MEMFAULTENA_Msk | SCB_SHCSR_BUSFAULTENA_Msk | SCB_SHCSR_USGFAULTENA_Msk;

    __DMB();
    MPU->CTRL = 0U; /* Disable MPU during configuration */
    MPU->RNR  = 0U; /* Region 0 */
    MPU->RBAR = guard_addr & MPU_RBAR_ADDR_Msk;
    /* Size = 32 B (SIZE=4 -> 2^(4+1)), AP = 000 (No Access), XN = 1, Normal Non-Cacheable, Enable */
    MPU->RASR = MPU_RASR_XN_Msk | (0U << MPU_RASR_AP_Pos) | (1U << MPU_RASR_TEX_Pos) |
                (4U << MPU_RASR_SIZE_Pos) | MPU_RASR_ENABLE_Msk;
    MPU->CTRL = MPU_CTRL_PRIVDEFENA_Msk | MPU_CTRL_ENABLE_Msk;
    __DSB();
    __ISB();
}
```
