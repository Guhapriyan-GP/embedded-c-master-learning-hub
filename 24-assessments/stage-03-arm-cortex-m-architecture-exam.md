# Stage 03 Mastery Exam: ARM Cortex-M4F Architecture, Linker, Faults & MPU

**Aligned Stages:** Stages 03–05 (`04-arm-cortex-m/`, `05-toolchain-and-debugging/`, `06-stm32-fundamentals/`)  
**Concept IDs:** `ARM-CORE-001..005`, `ARM-ABI-001..002`, `ARM-EXC-001..003`, `ARM-FLT-001`, `ARM-MPU-001`, `ARM-FPU-001`, `ARM-SYNC-001`, `ARM-DBG-001`, `TOOL-LD-001`, `STM32-ARCH-001`, `STM32-SRAM-001`  
**Total Points:** 100 | **Pass Threshold:** $\ge 85$ Points | **Answer Key:** [`answer-keys-and-rubrics.md`](answer-keys-and-rubrics.md#stage-03-answer-key--rubric)

---

### Questions (100 Points Total)

1. **ARMv7E-M Core Registers, PC Semantics & Banked Stacks (15 Points):**
   (a) Why is Cortex-M4 specifically **ARMv7E-M** rather than baseline ARMv7-M?
   (b) When reading `R15 (PC)` during execution, what is the value of `PC[0]` and what pipeline offset applies? Where is Thumb execution state actually stored?
   (c) Contrast `MSP` and `PSP`, stating which is active at reset, in Handler mode, and in FreeRTOS tasks (`CONTROL.SPSEL`).

2. **Exception Stacking, `EXC_RETURN` & FPU Lazy Stacking (15 Points):**
   (a) List the 8 registers (in exact stack offset order `SP+0` to `SP+28`) pushed by hardware on exception entry when FPU is inactive.
   (b) How does the frame size change when FPU is active (`FPCA = 1`)? Explain how `FPU->FPCCR.LSPEN` (lazy stacking) avoids wasting cycles in integer-only ISRs and contrast `EXC_RETURN = 0xFFFFFFFD` vs. `0xFFFFFFED`.

3. **Fault Register Decoding & Naked-Assembly Unwind (20 Points):**
   (a) Why do MemManage, BusFault, and UsageFault escalate to `HardFault` (`HFSR.FORCED = 1`) by default, and which `SCB` register enables them individually?
   (b) A FreeRTOS task crashes into `HardFault_Handler`. Write the 4-instruction naked assembly prologue that inspects `EXC_RETURN` bit 2 to pass the active stack frame pointer (`MSP` or `PSP`) in `R0` to a C decoder.
   (c) Given `SCB->CFSR = 0x00008200` and `SCB->BFAR = 0x60000000`, decode the exact fault bit names and root cause. What does `SCB->CFSR = 0x02000000` indicate (when `SCB->CCR.DIV_0_TRP = 1`)?

4. **8-Region MPU Stack-Overflow Guard Configuration (15 Points):**
   Write a CMSIS function configuring **MPU Region 0** as a **32-byte Execute-Never (`XN=1`), No-Access (`AP=000`) stack-overflow guard** at a 32-byte-aligned address `guard_addr`, while keeping the default memory map active for privileged code (`PRIVDEFENA = 1`). Which mandatory barrier instructions must follow `MPU->CTRL` enable?

5. **Linker Scripts, CCM-SRAM Relocation & `VTOR` Alignment (20 Points):**
   (a) Contrast `VMA` and `LMA` for `.data` and `.ccmram`. Why does a linker script declaring `CCMRAM (rw)` without `(xrw)` and without a `_siccmram` copy loop in `Reset_Handler` fail at runtime?
   (b) STM32G474RE has 16 core exceptions + 102 external IRQs (118 vectors = 472 bytes). What is the minimum legal alignment of `SCB->VTOR`?
   (c) Explain STM32G474RE's dual-mapped CCM-SRAM (`0x10000000` vs. `0x20018000`): which address must CPU code use for zero-wait-state execution, and which address must DMA1/DMA2 use?

6. **Hardware Concurrency, Profiling & Bus Architecture (15 Points):**
   (a) How do `LDREX`/`STREX` implement a lock-free atomic increment without disabling interrupts?
   (b) Which two CoreSight registers must be enabled to measure exact cycle counts via `DWT->CYCCNT`? At 170 MHz, what is the duration of 1 cycle and when does the 32-bit counter wrap?
   (c) On STM32G474RE, which bus hosts `DAC1–DAC4`, and which silicon peripherals in `stm32g474xx.h` are unavailable on the LQFP64 package (`STM32G474RET6`)?
