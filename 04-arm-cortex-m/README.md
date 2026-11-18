# Stage 04: ARM Cortex-M4F (ARMv7E-M) Microarchitecture & Core Peripherals

> **Curriculum Track:** Level 1–3 Core Processor Architecture (`ARM-*`, all 26 concepts)  
> **Architecture Layer Precision:** This module explicitly separates **Generic Cortex-M (ARMv6-M/v7-M)**, **Cortex-M4 Specific (ARMv7E-M)**, and **STM32G474RE Exact Silicon** (`__CM4_REV = 0x0001U` r0p1, `__MPU_PRESENT = 1U`, `__NVIC_PRIO_BITS = 4U`, `__FPU_PRESENT = 1U` verified in `stm32g474xx.h`).  
> **Code Verification Policy:** All C and assembly snippets in Section 8 are **Compile-Verified (`arm-none-eabi-gcc 14.3.1`, `-O2 -Wall -Wextra -Werror -Wconversion -Wdouble-promotion -Wvla`)** against `core_cm4.h`, `mpu_armv7.h`, `cmsis_gcc.h`, and `stm32g474xx.h` (Not Tested on Physical Hardware).

---

## 1. Learning Objectives

Upon completing Stage 04, you will master every ARM Cortex-M4 / ARMv7E-M architectural subsystem:

| Concept ID | Architectural Scope | Level | Mastery Objective |
|---|---|---|---|
| `ARM-CORE-001` | `[CM-v7M / CM4 / G474]` | L1 | Explain the 3-stage pipeline with branch speculation and Harvard buses (I-Code, D-Code, System, PPB), and decode `SCB->CPUID` (`0x410FC241` = Cortex-M4 r0p1). |
| `ARM-CORE-002..005` | `[CM-Gen / CM-v7M / CM4]` | L1–L2 | Master `R0–R15` (exact `PC` = `current+4`, `bit[0]=0`), `APSR` (`N,Z,C,V,Q,GE[3:0]`), `IPSR`, `EPSR.T`, `xPSR`, `PRIMASK`/`FAULTMASK`/`BASEPRI`/`CONTROL`, banked `MSP`/`PSP`, Thread/Handler & Privilege levels + `SVC`. |
| `ARM-ISA-001..002` | `[CM-Gen / CM-v7M]` | L2 | Read Thumb-2 16/32-bit disassembly (`IT`, `BFI/UBFX`, `CLZ`, `RBIT`, `REV`, `CBZ`, `TBB/TBH`) and Load/Store (`LDR`/`STR`/`LDM`/`STM`/`PUSH`/`POP`) alignment rules. |
| `ARM-MEM-001..002` | `[CM-v7M / G474]` | L1–L2 | Navigate the 4 GB architectural memory map, default memory types/XN rules, CCM-SRAM dual-mapping, and **exact STM32G474RE bit-banding rules (including AHB2 GPIO/ADC exclusion!)**. |
| `ARM-ABI-001..002`, `ARM-EXC-001..003` | `[CM-Gen / CM4 / G474]` | L2 | Apply AAPCS (`R0–R3`, `R4–R11`, `STKALIGN`, hard-float `S0–S15`), trace 8-word (32 B) vs 26-word (104 B) FPU lazy-stacked frames (`FPCCR.LSPEN`), all 6 `EXC_RETURN` values, 12-cycle latency / 6-cycle tail-chaining / late arrival, and 512-byte aligned `SCB->VTOR`. |
| `ARM-NVIC-001`, `ARM-SYST-001` | `[CM-v7M / G474]` | L1–L2 | Distinguish generic NVIC (240 IRQs, 3–8 bits) from STM32G474RE (102 IRQs, 4 bits), configure `PRIGROUP` & `BASEPRI`, and master the 24-bit SysTick timer (`CTRL`, `LOAD`, `VAL`, `CALIB`). |
| `ARM-FLT-001`, `ARM-SYNC-001..002` | `[CM-v7M / CM4]` | L3 | Diagnose faults using `CFSR` (`MMFSR`/`BFSR`/`UFSR`), `HFSR`, `MMFAR`, `BFAR`, `SHCSR`, `ACTLR.DISDEFWBUF`, and apply `LDREX`/`STREX`/`CLREX` and `DMB`/`DSB`/`ISB`. |
| `ARM-MPU-001`, `ARM-FPU-001`, `ARM-DSP-001` | `[CM-v7M / CM4 / G474]` | L2–L3 | Configure the 8-region ARMv7-M MPU (`RNR`/`RBAR`/`RASR`), enable `FPv4-SP` FPU (`SCB->CPACR`), and leverage ARMv7E-M DSP/SIMD (`MLA`, `SADD16`, `SMLAD`, `SSAT`). |
| `ARM-DBG-001`, `ARM-PWR-001`, `ARM-SEC-001..002` | `[CM-v7M / CM4 / G474]` | L2–L3 | Use CoreSight (`SWD`, 6 FPB, 4 DWT + `CYCCNT`, ITM/SWO on `PB3`), `WFI`/`WFE`/`SEV`/`SLEEPDEEP`/`SLEEPONEXIT`, and STM32G474RE security (explicitly distinguishing why **ARMv8-M TrustZone-M is `NOT_APPLICABLE`** to ARMv7E-M!). |

---

## 2. Prerequisites

- [Stage 02 — C Language Foundations](../02-c-fundamentals/README.md) (`C-PTR-*`, `C-ALIGN-*`, `C-MATH-003`).
- [Stage 03 — Embedded C & Concurrency](../03-embedded-c/README.md) (`EMB-MMIO-*`, `EMB-VOL-003`, `EMB-ATOM-001`).

---

## 3. Detailed Technical Theory

### 3.1 Core Microarchitecture, ISA Identity & Harvard Buses (`ARM-CORE-001`)

> **Critical Architectural Distinction:** Cortex-M3 implements baseline **ARMv7-M**. The STM32G474RE's **Cortex-M4F (r0p1)** implements **ARMv7E-M** — the `E` adds **DSP, SIMD, and single-cycle MAC/saturating instructions**, plus an optional single-precision **FPv4-SP** Floating-Point Unit (`__FPU_PRESENT = 1U`).

- **3-Stage Pipeline:** **Fetch → Decode → Execute** with branch speculation (target prefetch). Single-cycle 32×32→32/64 multiply-accumulate (`MAC`) and hardware 2–12 cycle `SDIV`/`UDIV`.
- **Harvard Bus Interfaces (Generic ARMv7-M):**
  - **I-Code Bus (`0x00000000–0x1FFFFFFF`):** Instruction fetches from Flash and **CCM-SRAM (`0x10000000`)**.
  - **D-Code Bus (`0x00000000–0x1FFFFFFF`):** Literal pool / constant / data reads from Flash and **CCM-SRAM (`0x10000000`)**.
  - **System Bus (`S-Bus`, `0x20000000–0xDFFFFFFF`):** Accesses SRAM1/SRAM2, CCM-SRAM alias (`0x20018000`), and AHB/APB peripherals concurrently with Flash instruction fetches!
  - **Private Peripheral Bus (`PPB`, `0xE0000000–0xE00FFFFF`):** Internal core peripherals (`SysTick`, `NVIC`, `SCB`, `MPU`, `FPU`, `DWT`, `ITM`, `FPB`, `DBGMCU`).

---

### 3.2 Core Registers, Exact `R15 (PC)` Semantics & `xPSR` (`ARM-CORE-002`, `ARM-CORE-003`)

- **`R0–R12`:** General-purpose 32-bit registers (`R0–R7` "Low registers" fit compact 16-bit Thumb encodings; `R8–R12` "High registers" typically require 32-bit Thumb-2 encodings).
- **`R13 (SP)`:** Banked Stack Pointer (`MSP` and `PSP`, bits `[1:0]` always forced `00` = 4-byte aligned in hardware; 8-byte aligned by AAPCS `STKALIGN`).
- **`R14 (LR)`:** Link Register — stores function return address (with `bit[0] = 1` for Thumb) or an **`EXC_RETURN`** magic code on exception entry.
- **`R15 (PC)` — Program Counter (EXACT ARMv7-M Specification, DDI 0403E.e §B1.4.7 & PM0214 §2.1.3):**
  1. Reading `PC` returns the address of the **currently executing instruction `+ 4`** (due to the 3-stage pipeline).
  2. Reading `PC` **ALWAYS returns a halfword-aligned value with `bit[0] = 0`**! Thumb execution state is **NOT** stored in `PC[0]` — it is stored in **`EPSR.T` (bit 24)**. `bit[0] = 1` is only set on interworking branch targets (`BX`/`BLX`), `LR`, and vector table entries so hardware loads `EPSR.T = 1` (clearing it causes immediate `INVSTATE` UsageFault!).
- **Combined Program Status Register (`xPSR`, `core_cm4.h`):**
  - **`APSR` (Application PSR):** `N` [31] Negative, `Z` [30] Zero, `C` [29] Carry, `V` [28] Overflow, **`Q` [27]** Sticky Saturation (ARMv7-M/E-M), and **`GE[3:0]` [19:16]** Greater-than-or-Equal SIMD byte-lane flags (**Cortex-M4 ARMv7E-M specific!**).
  - **`IPSR` (Interrupt PSR):** `ISR_NUMBER[8:0]` (`0` = Thread mode, `1..15` = Core exceptions, `16 + IRQn` = External IRQs; e.g., `15` inside `SysTick_Handler`).
  - **`EPSR` (Execution PSR):** `T` [24] Thumb bit (must remain `1`), `ICI/IT` [26:25, 15:10] If-Then / Interruptible-Continuable state. *(Reads as `0` via `MRS`!)*
- **Special Masking & Control Registers:**
  - `PRIMASK` (Generic Cortex-M): Bit 0 = `1` masks all configurable-priority exceptions (leaves NMI, HardFault, Reset unmasked).
  - `FAULTMASK` (ARMv7-M only — absent on M0/M0+): Bit 0 = `1` masks everything including HardFault (only NMI unmasked; auto-clears on exception return).
  - `BASEPRI` (ARMv7-M only — absent on M0/M0+): Non-zero value masks all exceptions with numeric priority $\ge \text{BASEPRI}$ (lower urgency).
  - `CONTROL`: Bit 0 `nPRIV` (`0` Privileged, `1` Unprivileged Thread mode), Bit 1 `SPSEL` (`0` MSP, `1` PSP in Thread mode), Bit 2 `FPCA` (Cortex-M4F specific: `1` = FPU context active).

---

### 3.3 Banked Stacks (`MSP`/`PSP`), Modes, Privilege & `SVC` (`ARM-CORE-004`, `ARM-CORE-005`)

- **Thread Mode:** Normal application execution (resets to Privileged + `MSP`). Can switch to `PSP` (`CONTROL.SPSEL = 1`) and drop to Unprivileged (`CONTROL.nPRIV = 1`) — always execute **`__ISB()`** immediately after writing `CONTROL`!
- **Handler Mode:** Entered automatically on any exception/ISR. **ALWAYS Privileged** and **ALWAYS uses `MSP`**.
- **One-Way Privilege Drop & `SVC` Escalation:** Unprivileged Thread code cannot clear `CONTROL.nPRIV` directly (writes are ignored) and faults if accessing PPB (`NVIC`/`SCB`/`MPU`). To request OS/kernel services or re-elevate privilege, it executes **`SVC #imm`**. Inside `SVC_Handler`, the kernel extracts the 8-bit immediate directly from the instruction stream at `((uint8_t *)stacked_pc)[-2]`.

---

### 3.4 Thumb-2 ISA & Load/Store Architecture (`ARM-ISA-001`, `ARM-ISA-002`)

- **Thumb-2 (ARMv7-M / v7E-M):** Seamlessly mixes 16-bit and 32-bit instructions without mode switching (unlike ARMv6-M Cortex-M0/M0+, which lacks most 32-bit Thumb-2 instructions). Key embedded instructions:
  - `IT{x{y{z}}} cond`: If-Then conditional block predicating up to 4 following instructions without branch pipeline flush!
  - `BFI`/`BFC`/`UBFX`/`SBFX`: Single-cycle bit-field insert, clear, and unsigned/signed extract.
  - `CLZ` (count leading zeros — used by RTOS priority bitmaps!), `RBIT` (reverse 32 bits), `REV`/`REV16`/`REVSH` (single-cycle endian swap), `CBZ`/`CBNZ` (compare & branch), `TBB`/`TBH` (compact `switch` jump tables).
- **Load/Store Model:** Only `LDR`/`STR` variants touch memory (pre-indexed `[Rn, #imm]!`, post-indexed `[Rn], #imm`, shifted `[Rn, Rm, LSL #2]`, multi-register `LDMIA`/`STMDB`/`PUSH`/`POP`). Single `LDR`/`STR` support unaligned addresses when `SCB->CCR.UNALIGN_TRP == 0`, whereas `LDRD`/`STRD`/`LDM`/`STM`/`LDREX`/`STREX`/`VLDR`/`VSTR` **always** require natural alignment!

---

### 3.5 Architectural 4 GB Memory Map & STM32G474RE Bit-Banding Reality (`ARM-MEM-001`, `ARM-MEM-002`)

| Address Range | ARMv7-M Architectural Region | Default Memory Type & XN | STM32G474RE Exact Mapping (`stm32g474xx.h`) |
|---|---|---|---|
| `0x00000000–0x1FFFFFFF` | Code (512 MB, I/D-Code) | Normal, Executable | Boot Alias (`0x00000000`), Main Flash 512 KB (`0x08000000`), **CCM-SRAM 32 KB (`0x10000000`, 0 WS!)**, System ROM (`0x1FFF0000`) |
| `0x20000000–0x3FFFFFFF` | SRAM (512 MB, S-Bus) | Normal, Executable (mark XN via MPU!) | SRAM1 80 KB (`0x20000000`), SRAM2 16 KB (`0x20014000`), **CCM-SRAM DMA Alias 32 KB (`0x20018000`)**, SRAM Bit-Band Alias (`0x22000000–0x23FFFFFF`) |
| `0x40000000–0x5FFFFFFF` | Peripheral (512 MB, S-Bus) | Device, **XN** | APB1 (`0x40000000`), APB2 (`0x40010000`), AHB1 (`0x40020000`), Periph Bit-Band Alias (`0x42000000–0x43FFFFFF`), **AHB2 GPIO (`0x48000000`)**, **AHB2 ADC/DAC (`0x50000000`)**, RNG (`0x50060800`) |
| `0x60000000–0x9FFFFFFF` | External RAM (1 GB) | Normal, Executable | QUADSPI XIP Memory-Mapped (`0x90000000–0x9FFFFFFF`) |
| `0xA0000000–0xDFFFFFFF` | External Device (1 GB) | Device, **XN** | QUADSPI Control Regs (`0xA0001000`); FMC (`0xA0000000`, unbonded on LQFP64) |
| `0xE0000000–0xE00FFFFF` | Private Peripheral Bus (PPB) | Strongly-Ordered, **XN** | `SCS`/`SysTick`/`NVIC`/`SCB`/`MPU`/`FPU`/`DWT`/`ITM`/`DBGMCU` |

**Exact STM32G474RE Bit-Banding Rules (`ARM-MEM-002`, RM0440 §2.3 & `stm32g474xx.h`):**
- Bit-banding is an optional ARMv7-M feature (present on M3/M4, **absent on M0/M0+/M7**). **STM32G474RE DOES implement bit-banding** for two **1 MB** windows:
  1. **SRAM window (`0x20000000–0x200FFFFF`) → Alias (`0x22000000–0x23FFFFFF`):** Covers SRAM1 (`SRAM1_BB_BASE = 0x22000000`), SRAM2 (`SRAM2_BB_BASE = 0x22280000`), and the S-Bus CCM-SRAM alias (`CCMSRAM_BB_BASE = 0x22300000`).
  2. **Peripheral window (`0x40000000–0x400FFFFF`) → Alias (`0x42000000–0x43FFFFFF`):** Covers APB1 (`0x40000000`), APB2 (`0x40010000`), and AHB1 (`0x40020000`, e.g., `RCC`).
- **TWO CRITICAL STM32G474RE TRAPS:**
  1. **AHB2 GPIO (`0x48000000`) and AHB2 ADC/DAC (`0x50000000`) are OUTSIDE the 1 MB `0x40000000–0x400FFFFF` bit-band window!** You **CANNOT** bit-band `GPIOA->ODR` on STM32G4 — use `GPIOA->BSRR`!
  2. **CPU-Only:** Bit-banding is performed inside the Cortex-M4 bus interface; **DMA1/DMA2 cannot access bit-band alias addresses**.

---

### 3.6 AAPCS, Exception Stack Frames, `EXC_RETURN`, Lazy Stacking & Interrupt Timing (`ARM-ABI-001..002`, `ARM-EXC-001..002`)

- **AAPCS Calling Convention:** `R0–R3` pass arguments 1–4 and return values (`R0:R1` for 64-bit); `R4–R11` are callee-saved; `R12` is intra-call scratch; public interfaces enforce **8-byte stack alignment** (`SCB->CCR.STKALIGN = 1` default on r0p1). Under `-mfloat-abi=hard`, single-precision floats pass in `S0–S15` (caller-saved), while `S16–S31` are callee-saved.
- **Hardware Exception Stack Frames:**
  - **Basic 8-Word Frame (32 Bytes, `SP+0..SP+28`):** Hardware pushes `[R0, R1, R2, R3, R12, LR, ReturnAddr, xPSR]` automatically in **12 cycles** (zero-wait-state memory). If `SP` was only 4-byte aligned, hardware inserts a 4-byte pad and sets stacked `xPSR[9] = 1`.
  - **Extended 26-Word FPU Frame (104 Bytes, Cortex-M4F Specific):** When `CONTROL.FPCA == 1`, hardware reserves an extra **18 words (+72 bytes)**: `S0–S15` (`SP+32..SP+92`), `FPSCR` (`SP+96`), and 1 reserved alignment word (`SP+100`).
  - **Lazy Stacking (`FPU->FPCCR`: `ASPEN=1`, `LSPEN=1` default, ARM AN298):** To preserve 12-cycle interrupt latency, the CPU allocates the 104-byte frame space and records `FPU->FPCAR` (`LSPACT = 1`), **skipping the 17-cycle FPU register push** unless the ISR actually executes an FPU instruction!
- **All 6 `EXC_RETURN` Magic Values (`core_cm4.h` L1636–1641):**

  | `EXC_RETURN` (Basic 8-Word, bit4=`1`) | `EXC_RETURN` (Extended FPU 26-Word, bit4=`0`) | Return Mode | Return Stack Pointer |
  |---|---|---|---|
  | `0xFFFFFFF1` | `0xFFFFFFE1` | Handler Mode (nested ISR) | `MSP` |
  | `0xFFFFFFF9` | `0xFFFFFFE9` | Thread Mode | `MSP` |
  | `0xFFFFFFFD` | `0xFFFFFFED` | Thread Mode (RTOS task) | `PSP` |

- **Zero-Overhead Hardware Optimizations (`ARM-EXC-002`):**
  - **Tail-Chaining (6 cycles):** If another IRQ is pending when an ISR exits, hardware skips unstacking and re-stacking and jumps directly to the next vector in **6 cycles**.
  - **Late Arrival:** If a higher-priority IRQ asserts *while* hardware is stacking for a lower-priority IRQ, the NVIC switches vector fetch mid-stacking and runs the higher-priority ISR first!

---

### 3.7 NVIC, `VTOR`, SysTick & Fault Diagnosis (`ARM-NVIC-001`, `ARM-EXC-003`, `ARM-SYST-001`, `ARM-FLT-001`)

- **Generic NVIC vs STM32G474RE NVIC (`ARM-NVIC-001`):**
  - **ARMv7-M Architectural Maximum:** Up to **240 external IRQs**, **3–8 priority bits** (8–256 levels).
  - **STM32G474RE Silicon Instantiation (`stm32g474xx.h`):** **102 external IRQs (`IRQn 0..101`, IRQ 85 reserved)**, **`__NVIC_PRIO_BITS = 4U`** (16 levels `0..15`, stored in upper bits `[7:4]` of each byte in `NVIC->IP[0..101]`).
  - Lower numeric priority = higher urgency (`0` is highest configurable). `SCB->AIRCR.PRIGROUP` (key `0x05FA`) splits the 4 bits between **Preemption (Group) Priority** and **Subpriority** (tie-breaker only, never preempts; `NVIC_SetPriorityGrouping(3)` = 4 preemption / 0 subpriority bits — mandatory for FreeRTOS!). `NVIC->STIR` allows privileged software-triggered IRQs.
- **`SCB->VTOR` Alignment (`ARM-EXC-003`):** Alignment must be the next power of 2 $\ge$ vector table size in bytes. On STM32G474RE: $(16 + 102) \times 4 = 472\text{ bytes} \implies$ **`SCB->VTOR` MUST be 512-byte (`0x200`) aligned!** Follow any `VTOR` update with `__DSB(); __ISB();`.
- **SysTick (`ARM-SYST-001`):** 24-bit down-counter at `0xE000E010` (`CTRL`, `LOAD`, `VAL`, `CALIB`), exception #15 (`SysTick_IRQn = -1`). On STM32G474RE, `CLKSOURCE = 1` selects `HCLK` (up to 170 MHz); `CLKSOURCE = 0` selects `HCLK/8`. Reading `CTRL` or writing `VAL` clears `COUNTFLAG`.
- **Fault Exceptions & Decoding (`ARM-FLT-001`):**
  ARMv6-M (Cortex-M0/M0+) has **only HardFault**. ARMv7-M (Cortex-M4) adds **MemManage**, **BusFault**, and **UsageFault** — disabled by default in `SCB->SHCSR` (bits 16–18), causing them to escalate to HardFault (`SCB->HFSR.FORCED = 1`).
  - **`SCB->CFSR` (`0xE000ED28`):** Packed into `MMFSR[7:0]` (`IACCVIOL`, `DACCVIOL`, `MMARVALID`), `BFSR[15:8]` (`IBUSERR`, `PRECISERR`, `IMPRECISERR`, `BFARVALID`), and `UFSR[31:16]` (`UNDEFINSTR`, `INVSTATE`, `NOCP`, `UNALIGNED`, `DIVBYZERO`).
  - Only trust `SCB->MMFAR` when `MMFSR.MMARVALID == 1`, and `SCB->BFAR` when `BFSR.BFARVALID == 1`.
  - **Imprecise BusFault Trick:** Buffered writes cause `IMPRECISERR` (where stacked `PC` is several instructions *past* the offending store!). During debugging, set `SCnSCB->ACTLR |= SCnSCB_ACTLR_DISDEFWBUF_Msk;` to disable the write buffer and convert imprecise BusFaults into **precise** faults with exact `PC` and `BFAR`!

---

### 3.8 Exclusives, Barriers, MPU, FPU, DSP/SIMD, CoreSight, Low-Power & Security (`ARM-SYNC-*`, `ARM-MPU-001`, `ARM-FPU-001`, `ARM-DSP-001`, `ARM-DBG-001`, `ARM-PWR-001`, `ARM-SEC-001..002`)

- **Exclusive Accesses (`ARM-SYNC-001`) & Barriers (`ARM-SYNC-002`):** ARMv7-M provides `LDREX`/`STREX`/`CLREX` with an internal local exclusive monitor automatically cleared on exception entry/return (absent on Cortex-M0/M0+). Use `DMB`, `DSB`, and `ISB` per ARM AN321.
- **8-Region ARMv7-M MPU (`ARM-MPU-001`, `mpu_armv7.h`):** STM32G474RE implements **8 unified regions** (`MPU->TYPE.DREGION = 8`), configured via `MPU->RNR`, `MPU->RBAR`, and `MPU->RASR` (`SIZE` = power of 2 $\ge 32\text{ B}$, naturally aligned; 8 subregions `SRD` for regions $\ge 256\text{ B}$; `AP[2:0]`, `XN`, `TEX/S/C/B`). Always enable `MPU_CTRL_PRIVDEFENA_Msk` (default background map for privileged code) and guard `MPU->CTRL` updates with `__DSB(); __ISB();`. Classic safety pattern: place a **32-byte `ARM_MPU_AP_NONE` / `XN=1` stack-guard region** at the bottom of `MSP` (and mark SRAM `XN=1`) so stack overflow traps deterministically into `MemManage_Handler`!
- **FPv4-SP FPU (`ARM-FPU-001`):** Single-precision hardware IEEE-754 (`S0–S31`, `FPSCR`, `VMUL`/`VMLA`/`VFMA`/`VDIV`/`VSQRT`; `double` falls back to software!). Enable in `SystemInit()` via `SCB->CPACR |= (3UL << 20U) | (3UL << 22U); __DSB(); __ISB();` (else `NOCP` UsageFault). FPU cumulative exception IRQ routes to **`FPU_IRQn = 81`** on STM32G474RE.
- **ARMv7E-M DSP & SIMD (`ARM-DSP-001`):** Single-cycle 32×32 MAC (`MLA`/`MLS`/`SMLAL`), dual 16-bit / quad 8-bit parallel SIMD (`SADD16`, `QADD16`, `SMLAD`, `SEL` with `APSR.GE[3:0]`), and hardware saturating clamps (`SSAT`, `USAT` setting `APSR.Q`). *(Distinct from STM32G474RE's external AHB1 `CORDIC`/`FMAC` peripherals!)*
- **CoreSight Debug & Trace (`ARM-DBG-001`):**
  - **SWJ-DP (`PA13` SWDIO / `PA14` SWCLK)**: 2-wire Serial Wire Debug interface.
  - **FPB (Flash Patch & Breakpoint, `0xE0002000`):** **6 hardware code breakpoints** + 2 literal comparators on Cortex-M4.
  - **DWT (Data Watchpoint & Trace, `0xE0001000`):** **4 hardware data watchpoints** (`DWT->COMP0..3`) + free-running 32-bit cycle counter **`DWT->CYCCNT`** (enabled via `CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk; DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;`).
  - **ITM (`0xE0000000`) & TPIU/TPI (`0xE0040000`):** 32 stimulus ports (`ITM->PORT[0..31]`, unlocked via `ITM->LAR = 0xC5ACCE55UL`) framed by TPIU (`TPI->SPPR = 2` NRZ, `TPI->ACPR = f_SYSCLK / f_SWO - 1`) and streamed non-blockingly over single-wire **`PB3` (`TRACESWO`, AF0)** to STLINK-V3E! *(See Stage 05 `TOOL-DBG-004` for complete 170 MHz & 160 MHz `TPI->ACPR` prescaler math and OpenOCD capture commands.)*
  - **`DBGMCU` (`0xE0042000`):** Controls `TRACE_IOEN`/`TRACE_MODE` (`DBGMCU->CR`), debug-in-low-power (`DBG_SLEEP`/`DBG_STOP`/`DBG_STANDBY`), and freezes `TIMx`, `IWDG`, `WWDG`, `RTC` during GDB halts via `DBGMCU->APB1FZR1/2` & `APB2FZ`.
- **Core Low-Power Mechanisms (`ARM-PWR-001`):**
  - `WFI` sleeps until an unmasked IRQ pends (~**11–12 CPU cycles** Sleep-to-Run wakeup on STM32G474RE per DS12288, NOT 4 cycles!).
  - `WFE` sleeps on the 1-bit Event Register (`SEV` sets it; `SCB->SCR.SEVONPEND = 1` lets even disabled pending IRQs wake `WFE` without entering an ISR!).
  - `SCB->SCR.SLEEPONEXIT = 1` auto-re-enters Sleep upon returning from the last ISR (zero thread-mode wake overhead).
  - `SCB->SCR.SLEEPDEEP = 1` enters STM32G4 Stop 0/1, Standby, or Shutdown (per `PWR->CR1.LPMS`).
- **Security Architecture & Explicit TrustZone-M Boundary (`ARM-SEC-001`, `ARM-SEC-002`):**
  - **`ARM-SEC-002` (TrustZone-M): `NOT_APPLICABLE` to ARMv7E-M Cortex-M4 / STM32G474RE!** TrustZone-M (`SAU`/`IDAU`, `SG`/`BXNS`, `MSPLIM`/`PSPLIM`) exists **exclusively on ARMv8-M** (Cortex-M23/M33/M55/M85, e.g., STM32L5/U5/H5).
  - **`ARM-SEC-001` on STM32G474RE:** Combine Cortex-M4 **Unprivileged Thread Mode + 8-region MPU + `SVC` gate** with STM32G4 silicon Flash protections: **RDP** (Level 0 `0xAA` open, Level 1 debug blocked/mass-erase regression, **irreversible Level 2 `0xCC` permanently bricks SWD!**), **WRP**, **PCROP** (execute-only Flash), and **Securable Memory Area** (`FLASH->SEC1R`/`SEC2R` boot hide-protection).

---

## 4. Important Terminology

| Term | Engineering Definition |
|---|---|
| **ARMv7E-M** | Enhanced ARMv7-M instruction set architecture implemented by Cortex-M4/M7 adding DSP, SIMD, and saturating math extensions over Cortex-M3's baseline ARMv7-M. |
| **`EXC_RETURN`** | Special `0xFFFFFFxx` sentinel loaded into `LR` on exception entry telling hardware how to unstack (8-word vs 26-word FPU, MSP vs PSP, Thread vs Handler) upon `BX LR`. |
| **Lazy Stacking (`FPCCR.LSPEN`)** | Cortex-M4F mechanism that reserves stack space for `S0–S15`/`FPSCR` on ISR entry but defers physically storing them unless the ISR actually uses the FPU. |
| **Tail-Chaining** | Back-to-back ISR transition in 6 cycles without redundant unstacking and re-stacking. |
| **Late Arrival** | NVIC optimization allowing a higher-priority IRQ to hijack an in-progress exception stacking sequence and execute first. |
| **Bit-Banding** | ARMv7-M atomic single-bit alias mapping where each bit in a 1 MB region maps to a full 32-bit word in a 32 MB alias region (CPU-only; excludes STM32G4 AHB2 GPIO/ADC!). |
| **CoreSight (`FPB`/`DWT`/`ITM`)** | ARM on-chip debug and real-time trace hardware providing 6 breakpoints, 4 watchpoints, `CYCCNT` cycle counter, and non-blocking SWO trace. |

---

## 5. Register-Level Explanations

Verified directly against `core_cm4.h` (`SCS_BASE = 0xE000E000UL`):

| Register | Address | Key Bitfields (`core_cm4.h`) |
|---|---|---|
| `SCB->CPUID` | `0xE000ED00` | `0x410FC241` on STM32G474RE: Implementer `0x41` (ARM), PartNo `0xC24` (Cortex-M4), Rev `0x1` (r0p1) |
| `SCB->VTOR` | `0xE000ED08` | Vector Table Offset Register; bits `[8:0]` must be `0` (**512-byte aligned**) on STM32G474RE |
| `SCB->AIRCR` | `0xE000ED0C` | `VECTKEY[31:16] = 0x05FA`, `PRIGROUP[10:8]` (priority split), `SYSRESETREQ[2]` |
| `SCB->SCR` | `0xE000ED10` | `SLEEPONEXIT[1]`, `SLEEPDEEP[2]`, `SEVONPEND[4]` |
| `SCB->CCR` | `0xE000ED14` | `UNALIGN_TRP[3]`, `DIV_0_TRP[4]`, `STKALIGN[9]` (=1 default) |
| `SCB->SHCSR` | `0xE000ED24` | `MEMFAULTENA[16]`, `BUSFAULTENA[17]`, `USGFAULTENA[18]` |
| `SCB->CFSR` | `0xE000ED28` | `MMFSR[7:0]`, `BFSR[15:8]`, `UFSR[31:16]` (`rc_w1` — write `1` to clear!) |
| `SCB->HFSR` | `0xE000ED2C` | `VECTTBL[1]`, `FORCED[30]` (`rc_w1`) |
| `SCB->CPACR` | `0xE000ED88` | `CP10[21:20] = 11`, `CP11[23:22] = 11` (Full FPU access) |
| `FPU->FPCCR` | `0xE000EF34` | `LSPACT[0]`, `LSPEN[30]` (Lazy Stack Enable), `ASPEN[31]` (Auto State Preserve) |
| `SysTick->CTRL` | `0xE000E010` | `ENABLE[0]`, `TICKINT[1]`, `CLKSOURCE[2]` (`1`=HCLK), `COUNTFLAG[16]` |
| `DWT->CYCCNT` | `0xE0001004` | 32-bit cycle counter (requires `CoreDebug->DEMCR.TRCENA = 1` & `DWT->CTRL.CYCCNTENA = 1`) |
| `ITM->PORT[0]` / `TER` / `TCR` | `0xE0000000` / `0xE00` / `0xE80` | 32 SWO stimulus ports (unlock `ITM->LAR = 0xC5ACCE55UL`); non-blocking `printf`/trace |
| `TPI->ACPR` / `SPPR` (`TPIU`) | `0xE0040010` / `0xE00400F0` | Async SWO baud prescaler (`f_SYSCLK / f_SWO - 1`) & protocol (`SPPR=2` NRZ, `1` Manchester) |

---

## 6. Relevant Formulas & Calculations

### 6.1 ARMv7-M Bit-Band Alias Address Formula (`ARM-MEM-002`)
$$\text{alias\_addr} = \text{bit\_band\_base} + (\text{byte\_offset} \times 32) + (\text{bit\_number} \times 4)$$

**Worked Example (SRAM1 bit 5 at `0x20000100`):**
$$\text{alias\_addr} = 0\text{x}22000000 + (0\text{x}0100 \times 32) + (5 \times 4) = 0\text{x}22002014$$
Writing `1UL` to `*(volatile uint32_t *)0x22002014UL` atomically sets bit 5 of `0x20000100` in one CPU store! *(Remember: NEVER apply this formula to `0x48000000` AHB2 GPIO — outside the 1 MB window!)*

### 6.2 `SCB->VTOR` Minimum Alignment & SysTick Reload
$$\text{VTOR\_align} = 2^{\lceil \log_2((16 + N_{\text{IRQ}}) \times 4) \rceil} = 2^{\lceil \log_2(118 \times 4) \rceil} = 2^{\lceil \log_2(472) \rceil} = 512\text{ bytes (0x200)}$$
$$\text{SysTick\_LOAD} = \frac{f_{\text{HCLK}}}{f_{\text{tick}}} - 1 \implies \frac{170,000,000}{1000} - 1 = 169,999 \quad (1\text{ ms tick @ 170 MHz})$$

### 6.3 Worst-Case Nested ISR Stack Budget (With FPU Lazy Stacking)
$$S_{\text{MSP,max}} = \sum_{p=1}^{L_{\text{nest}}} \left( S_{\text{hw\_frame},p} + S_{\text{locals},p} \right), \qquad S_{\text{hw\_frame}} = \begin{cases} 32\text{ B} & \text{(no FPU active)} \\ 104\text{ B} & \text{(FPCA = 1)} \end{cases}$$

---

## 7. Official Documentation References

- **ARM DDI 0403E.e** — *ARMv7-M Architecture Reference Manual* (Registers, Thumb-2, Memory Map, Bit-Banding, Exceptions, Faults, Exclusives, Barriers, MPU, CoreSight).
- **ARM DDI 0439** — *ARM Cortex-M4 Technical Reference Manual* (Pipeline, 12-cycle latency, FPv4-SP FPU, 6 FPB, 4 DWT, ITM).
- **ST PM0214** — *STM32 Cortex-M4 MCUs and MPUs Programming Manual*.
- **ARM IHI 0042** — *Procedure Call Standard for the Arm Architecture (AAPCS)*.
- **ARM AN298** (*Cortex-M4F Lazy Stacking*) & **ARM AN321** (*Memory Barrier Instructions*).
- **ST RM0440** (§2.2–2.3 Memory Map & Bit-Banding, §3.5 Flash Security, Ch. 47 `DBGMCU`) & **ST DS12288** ($t_{\text{WUSLEEP}}$ = 11–12 cycles).

---

## 8. Verified Code Examples

> **Verification Status:** **Compile-Verified (`arm-none-eabi-gcc 14.3.1`, `-O2 -Wall -Wextra -Werror -Wconversion -Wdouble-promotion -Wvla`)** against `core_cm4.h`, `mpu_armv7.h`, `cmsis_gcc.h`, and `stm32g474xx.h` (Not Tested on Physical Hardware).

### 8.1 Naked-Assembly HardFault Unwinder (`ARM-FLT-001`) & `DWT->CYCCNT` Profiler (`ARM-DBG-001`)

```c
#include "stm32g474xx.h"
#include <stdint.h>

typedef struct {
    uint32_t r0, r1, r2, r3, r12, lr, pc, xpsr;
} exception_frame_t;

volatile uint32_t g_fault_pc, g_fault_cfsr, g_fault_hfsr, g_fault_bfar, g_fault_mmfar;

void hardfault_c_handler(const exception_frame_t *frame)
{
    g_fault_pc    = frame->pc;
    g_fault_cfsr  = SCB->CFSR;
    g_fault_hfsr  = SCB->HFSR;
    g_fault_bfar  = (g_fault_cfsr & SCB_CFSR_BFARVALID_Msk)  ? SCB->BFAR  : 0UL;
    g_fault_mmfar = (g_fault_cfsr & SCB_CFSR_MMARVALID_Msk)  ? SCB->MMFAR : 0UL;
    __BKPT(0);
    while (1) { }
}

__attribute__((naked)) void HardFault_Handler(void)
{
    /* Inspect EXC_RETURN bit 2 in LR: 0 -> MSP, 1 -> PSP */
    __asm volatile (
        "tst lr, #4                 \n"
        "ite eq                     \n"
        "mrseq r0, msp              \n"
        "mrsne r0, psp              \n"
        "b hardfault_c_handler      \n"
    );
}

/* Enable DWT->CYCCNT cycle-accurate profiler (5.882 ns/cycle @ 170 MHz) */
static inline void dwt_cyccnt_init(void)
{
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CYCCNT = 0UL;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
}
```

### 8.2 32-Byte MPU Stack Guard Setup (`ARM-MPU-001`)

```c
#include "stm32g474xx.h"
#include "mpu_armv7.h"

void mpu_configure_stack_guard(uint32_t guard_base_32b_aligned)
{
    __DMB();
    MPU->CTRL = 0UL; /* Disable MPU before reconfiguring */

    /* Region 0: 32-byte No-Access, Execute-Never guard at bottom of MSP */
    MPU->RNR  = 0UL;
    MPU->RBAR = (guard_base_32b_aligned & MPU_RBAR_ADDR_Msk);
    MPU->RASR = ARM_MPU_RASR(1UL,              /* XN = 1 (Execute Never) */
                             ARM_MPU_AP_NONE,  /* No privileged/unpriv access */
                             0UL, 0UL, 1UL, 0UL, /* Normal, non-shareable */
                             0x00UL,           /* All subregions enabled */
                             ARM_MPU_REGION_SIZE_32B);

    /* Enable MemManage Fault so stack overflow does not escalate blindly */
    SCB->SHCSR |= SCB_SHCSR_MEMFAULTENA_Msk;

    /* Enable MPU with Privileged Default Background Map (PRIVDEFENA = 1) */
    MPU->CTRL = MPU_CTRL_PRIVDEFENA_Msk | MPU_CTRL_ENABLE_Msk;
    __DSB();
    __ISB();
}
```

### 8.3 FPU Lazy Stacking (`ARM-FPU-001`), ARMv7E-M DSP `SMLAD` (`ARM-DSP-001`) & Lock-Free `LDREX`/`STREX` CAS (`ARM-SYNC-001`)

```c
#include "stm32g474xx.h"
#include <stdbool.h>
#include <stdint.h>

/* 1. Enable FPv4-SP FPU with mandatory barriers & lazy stacking (ARM AN298) */
void fpu_enable_with_lazy_stacking(void)
{
    SCB->CPACR |= (3UL << 20U) | (3UL << 22U); /* CP10 & CP11 Full Access */
    __DSB();
    __ISB();
    FPU->FPCCR |= FPU_FPCCR_ASPEN_Msk | FPU_FPCCR_LSPEN_Msk;
}

/* 2. ARMv7E-M single-cycle dual Q15 MAC (SMLAD) + hardware saturation (__SSAT) */
int32_t dsp_q15_dot_product_smlad(const int16_t *a, const int16_t *b, uint32_t pairs)
{
    int32_t acc = 0;
    for (uint32_t i = 0U; i < pairs; ++i) {
        uint32_t va = ((uint32_t)(uint16_t)a[2U * i]) |
                      (((uint32_t)(uint16_t)a[(2U * i) + 1U]) << 16U);
        uint32_t vb = ((uint32_t)(uint16_t)b[2U * i]) |
                      (((uint32_t)(uint16_t)b[(2U * i) + 1U]) << 16U);
        __asm__ volatile ("smlad %0, %1, %2, %0" : "+r"(acc) : "r"(va), "r"(vb));
    }
    return __SSAT(acc >> 15, 16); /* Clamp cleanly to Q15 [-32768, 32767] */
}

/* 3. Lock-free 32-bit Compare-And-Swap using Cortex-M4 exclusive monitor */
bool atomic_cas_u32(volatile uint32_t *addr, uint32_t expected, uint32_t desired)
{
    uint32_t current;
    do {
        current = __LDREXW(addr);
        if (current != expected) {
            __CLREX();
            return false;
        }
    } while (__STREXW(desired, addr) != 0U);
    __DMB();
    return true;
}
```

---

## 9. Practical STM32G474RE Experiments

1. **Core Register, `CPUID` & Exception Stack Frame Inspection (`ARM-CORE-001..004`, `ARM-ABI-002`):**
   - Pair with [Lab 04 — SysTick Delay](../18-practical-laboratory/lab-04-systick-accurate-delay-generator/README.md): halt in GDB, run `x/1wx 0xE000ED00` (confirm `0x410FC241`), break inside `SysTick_Handler`, verify `info reg pc` has `bit[0] == 0` while `xpsr` bit 24 (`T`) is `1`, `ipsr == 15`, `lr == 0xFFFFFFF9`, and dump `x/8wx $sp`.
2. **Fault Forensics Lab (`ARM-FLT-001`):**
   - Enable `SCB->SHCSR` configurable faults and `SCB->CCR |= SCB_CCR_DIV_0_TRP_Msk`; trigger an intentional divide-by-zero (`UFSR.DIVBYZERO`) and an invalid address read (`BFSR.PRECISERR` + `BFARVALID`), confirming exact `g_fault_pc` matches `arm-none-eabi-addr2line`.
3. **MPU Stack Guard & FPU `CPACR` Verification (`ARM-MPU-001`, `ARM-FPU-001`):**
   - Configure the 32-byte `ARM_MPU_AP_NONE` guard region, write into the guard address, and confirm `MemManage_Handler` fires with `MMFSR.DACCVIOL == 1` and exact `MMFAR`.
   - **Debugging Checkpoint:** Use `DWT->CYCCNT` in GDB to benchmark a 256-sample Q15 dot product in plain C vs ARMv7E-M `__SMLAD`, and compare `float` (`VMUL.F32`) vs `double` (`__aeabi_dmul`).

---

## 10. Common Mistakes & Anti-Patterns

1. **Claiming `PC` has `bit[0] = 1`:** False — reading `R15` returns `current + 4` with `bit[0] = 0`; Thumb state lives in `EPSR.T` (bit 24).
2. **Trying to bit-band STM32G474RE `GPIOA->ODR` (`0x48000014`):** AHB2 peripherals (`0x48000000`, `0x50000000`) are **outside** the 1 MB bit-band window (`0x40000000–0x400FFFFF`)!
3. **Omitting `__DSB(); __ISB();` after `SCB->CPACR`, `SCB->VTOR`, `MPU->CTRL`, or `MSR CONTROL`:** Pipelined instructions can execute under stale permissions or trigger intermittent `NOCP` UsageFaults.
4. **Assuming TrustZone-M (`SAU`/`MSPLIM`) exists on Cortex-M4:** TrustZone-M is ARMv8-M (Cortex-M23/M33) only!
5. **Relocating `SCB->VTOR` to an address not 512-byte aligned:** Low bits `[8:0]` are truncated/ignored on STM32G474RE (118 vectors = 472 B), silently dispatching wrong IRQs.
6. **Writing RDP Level 2 (`0xCC`) during experiments:** Permanently and **irreversibly** locks out SWD/JTAG and System ROM bootloader, bricking your NUCLEO-G474RE board!

---

## 11. Hands-On Exercises

1. **PSP Switching Exercise:** Allocate a 256-byte `_Alignas(8)` static array, switch Thread mode to `PSP` (`__set_PSP`, `__set_CONTROL(2)`, `__ISB()`), break in `SysTick_Handler`, and verify `$sp == $msp` in Handler mode while `LR == 0xFFFFFFFD`.
2. **SRAM1 Bit-Band Macro:** Write a C macro computing the bit-band alias address for any bit in `SRAM1` (`0x20000000–0x20013FFF`), verify it compiles to a single `STR`, and add a `_Static_assert` or runtime guard rejecting AHB2 addresses (`>= 0x40100000`).
3. **Lock-Free Atomic Increment:** Implement `atomic_inc_u32()` using `__LDREXW`/`__STREXW` (`cmsis_gcc.h`) and prove zero lost updates under a 50 kHz preempting timer ISR.
4. **Cortex-M4 vs Cortex-M33 Comparison Table:** Construct a 6-row table contrasting ARMv7E-M (Cortex-M4) vs ARMv8-M (Cortex-M33) across ISA, Security States, Stack Limit Registers, MPU architecture (power-of-2 vs base/limit), and Bit-Banding.

---

## 12. Assessment Questions

1. What exact value does `R15 (PC)` return when read by an instruction at `0x08000120`? Where is the Thumb execution state bit physically stored?
2. An exception occurs while Thread mode is running on `PSP` with `CONTROL.FPCA == 1`. What exact 32-bit `EXC_RETURN` value does hardware load into `LR`, and how many bytes are allocated on `PSP`?
3. Explain the difference between generic ARMv7-M NVIC limits and STM32G474RE's actual NVIC silicon implementation (`IRQ` count and priority bit width).
4. Why does an imprecise `BusFault` (`BFSR.IMPRECISERR = 1`) report an inaccurate stacked `PC`, and which Cortex-M4 register bit converts it into a precise fault during debugging?
5. Why can you bit-band a flag in STM32G474RE `SRAM1` (`0x20000040`) and `RCC` (`0x40021000`), but **cannot** bit-band `GPIOA->ODR` (`0x48000014`) or `ADC1->CR` (`0x50000008`)?
6. Is TrustZone-M available on the STM32G474RE? What hardware mechanisms DO you use on STM32G474RE for runtime memory isolation and Flash IP protection?

---

## 13. Measurable Completion Criteria

- [ ] **Architecture Precision Demonstrated:** Accurately distinguish Generic Cortex-M, Cortex-M4 ARMv7E-M, STM32G474RE silicon limits, and ARMv8-M (TrustZone-M `NOT_APPLICABLE`).
- [ ] **Live GDB Stack & Register Decoding:** Decode `SCB->CPUID` (`0x410FC241`), `xPSR` (`N,Z,C,V,Q,GE,IPSR,T`), all 6 `EXC_RETURN` codes, and both 32-byte and 104-byte exception frames in GDB.
- [ ] **Fault Forensics Mastery:** Successfully capture and decode `CFSR` (`MMFSR`/`BFSR`/`UFSR`), `HFSR`, `MMFAR`/`BFAR`, and stacked `PC` from an injected fault.
- [ ] **Hardware Primitives Verified:** Correctly configure an 8-region MPU stack guard, enable `FPv4-SP` with barriers, profile code with `DWT->CYCCNT`, and implement `LDREX`/`STREX` lock-free synchronization.
