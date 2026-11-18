# Stage 03: Embedded C, Concurrency Primitives & Production Firmware Architecture

> **Sequential Navigation (Step 3 of 17 — Canonical Acyclic Order):**  
> **Home:** [`START_HERE.md`](../START_HERE.md) | **Roadmap:** [`learning-sequence.md`](../01-master-concept-map/learning-sequence.md) | **Prev:** [Stage 02 — C Language Foundations (`02-c-fundamentals/`)](../02-c-fundamentals/README.md) | **Next:** [Stage 04 — ARM Cortex-M4F Core (`04-arm-cortex-m/`)](../04-arm-cortex-m/README.md)  
> **Prerequisites:** [Stage 01b (`DIG-ELEC-*`)](../01b-digital-electronics-and-hw-integration/README.md), [Stage 02 (`C-*`)](../02-c-fundamentals/README.md)

> **Curriculum Track:** Level 1–3 Bare-Metal Systems & Firmware Architecture (`EMB-*`, `FW-*`)  
> **Target Platform:** STM32G474RET6 (ARM Cortex-M4F, CMSIS-Core V5.x, `stm32g474xx.h`, `__NVIC_PRIO_BITS = 4U`)  
> **Code Verification Policy:** All C11 implementation blocks in Section 8 are **Compile-Verified (`arm-none-eabi-gcc 14.3.1`, `-std=c11 -mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard -DSTM32G474xx -O2 -Wall -Wextra -Werror -Wdouble-promotion -Wvla`)** against vendored `stm32g474xx.h`, `core_cm4.h`, and `cmsis_gcc.h` (**Not Tested on Physical Hardware**).

---

## 1. Learning Objectives

Upon completing Stage 03, you will master both **low-level hardware-facing Embedded C (`EMB-*`)** and **scalable bare-metal Firmware Architecture (`FW-*`)**:

| Domain | Concept IDs | Level | Prerequisites | Core Mastery Outcomes |
|---|---|---|---|---|
| MMIO & CMSIS | `EMB-MMIO-001..003`, `EMB-CMSIS-001..003` | L1–L2 | `C-PTR-*`, `C-TYPE-001` | Derive peripheral base addresses via CMSIS macro chains (`PERIPH_BASE -> GPIOA`), build `volatile` struct overlays (`__I`/`__O`/`__IO`), obey bus access widths & RCC clock-enable sync delays, and use CMSIS-Core/Device/SVD/DSP. |
| Register Semantics | `EMB-REG-001..004` | L1 | `EMB-MMIO-001`, `C-BIT-*` | Eliminate RMW preemption races, classify and handle `rw`, `r`, `w`, `rc_w1`, `rc_w0`, `rc_r`, `rs`, `t` registers without flag loss, preserve reserved bits, use single-store `MODIFY_REG`, and apply atomic `BSRR`/`BRR` (plus STM32G4 bit-banding bounds). |
| `volatile` & Barriers | `EMB-VOL-001..003` | L1–L2 | `EMB-MMIO-001`, `C-QUAL-002` | Explain **what `volatile` guarantees and what it does NOT guarantee**, and combine compiler barriers with ARMv7-M `__DMB()`/`__DSB()`/`__ISB()` for DMA and ISR synchronization. |
| Atomics & Concurrency | `EMB-ATOM-001..004`, `EMB-ISR-001..002`, `EMB-NUM-001` | L2–L3 | `EMB-VOL-002..003`, `C-ATOM-002` | Implement nested `PRIMASK` and Cortex-M4 `BASEPRI` (`__set_BASEPRI_MAX`) critical sections, lock-free `__LDREXW`/`__STREXW`/`__CLREX` & C11 `<stdatomic.h>`, tear-free 64-bit double-reads & multi-field seqlocks, lock-free SPSC ring buffers, and reentrant/deferred ISR architectures. |
| Layered Drivers & Static vs Dynamic Dispatch | `FW-LAYER-001..003`, `FW-EXEC-001..003`, `FW-DRV-001..003`, `FW-MOD-001..002` | L1–L3 | `EMB-CMSIS-002`, `C-QUAL-003`, `C-PREP-003` | Compare Direct Register vs CMSIS vs ST LL vs ST HAL objectively, quantify Polling vs Interrupt vs DMA, build race-free `WFI` superloops and wraparound-safe TTC schedulers, design opaque-handle async drivers with bounded timeouts, and engineer **Zero-Overhead Static HAL Dispatch (X-Macros, `static inline`, `_Generic`, link-time seams) vs Runtime Vtables (`FW-DRV-003`)**. |
| FSMs, HSMs, Patterns & Standards | `FW-FSM-001..002`, `FW-ERR-001..003`, `FW-VER-001`, `FW-PAT-001..009`, `EMB-STD-001..003` | L2–L3 | `FW-DRV-001`, `C-FPTR-001..003` | Implement Switch/Table FSMs, Hierarchical State Machines (HSMs) & Active Objects, apply all 9 embedded design patterns (Observer, Hardware Proxy/Adapter, Debouncer, Ring Buffer, $O(1)$ Memory Pool, DMA Ping-Pong, Command Pattern, Singleton Ownership, SW Timer Mux), enforce assertions & crash telemetry, and apply MISRA C:2023, SEI CERT C, and formal deviation records. |

---

## 2. Prerequisites

- [Stage 01b — Digital Electronics & Hardware Integration](../01b-digital-electronics-and-hw-integration/README.md) (`DIG-ELEC-001..006`, `HWSW-PCB-001`).
- [Stage 02 — C Language Foundations](../02-c-fundamentals/README.md) and [Advanced C Deep-Dive](../02-c-fundamentals/advanced-c-ub-memory-model.md) (`C-BIT-*`, `C-PTR-*`, `C-TYPE-*`, `C-STOR-*`, `C-QUAL-*`, `C-PREP-003`, `C-ALIGN-*`, `C-ATOM-*`, `C-MATH-*`, `C-LIB-001`).

---

## 3. Detailed Technical Theory

### 3.1 Memory-Mapped I/O (MMIO), CMSIS Architecture, SVD & DSP (`EMB-MMIO-001..003`, `EMB-CMSIS-001..003`)

*Prerequisites: `C-PTR-001..003`, `C-TYPE-001`, `C-ALIGN-001`.*

On ARM Cortex-M, peripheral hardware registers live inside the unified 32-bit address space (`0x40000000–0x5FFFFFFF` and `0xA0000000–0xA00FFFFF`). CMSIS-Device (`stm32g474xx.h`) derives typed peripheral pointers from layered base macros:
```c
#define PERIPH_BASE           (0x40000000UL)
#define AHB2PERIPH_BASE       (PERIPH_BASE + 0x08000000UL) /* 0x48000000UL */
#define GPIOA_BASE            (AHB2PERIPH_BASE + 0x0000UL) /* 0x48000000UL */
#define GPIOA                 ((GPIO_TypeDef *)GPIOA_BASE)
```
Every member of `GPIO_TypeDef` is qualified using CMSIS-Core access macros (`core_cm4.h`):
- `__I` / `__IM` → `volatile const`: Read-only hardware register (e.g., `IDR`, `ISR`). Attempting to write fails at compile time.
- `__O` / `__OM` → `volatile`: Write-only hardware register (e.g., `BSRR`, `EGR`, `ICR`).
- `__IO` / `__IOM` → `volatile`: Read/write hardware register (e.g., `MODER`, `CR1`).

**Two Critical Bus-Interface Rules (`EMB-MMIO-003`):**
1. **RCC Peripheral Clock-Enable Synchronization Delay (RM0440 §6.2.17):** After writing `RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN;`, the peripheral clock takes up to 2 AHB/APB cycles to gate ON. Writing `GPIOA->MODER` on the very next cycle can be **silently dropped**! Perform a dummy read-back + barrier:
   ```c
   RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN;
   volatile uint32_t tmpreg = RCC->AHB2ENR;
   (void)tmpreg;
   __DSB();
   ```
2. **Peripheral Bus Access Widths:** Most STM32G4 peripheral registers require 32-bit aligned word accesses (`STR`/`LDR`). Notable exception: **SPI `SPIx->DR`** uses 8-bit `*(volatile uint8_t *)&SPI1->DR` writes to push 1 byte into the TX FIFO vs 16-bit writes to pack 2 bytes (RM0440 §40.4.9).

**CMSIS-SVD Peripheral Inspection & CMSIS-DSP on Cortex-M4F (`EMB-CMSIS-003`):**
- **CMSIS-SVD (System View Description):** Vendor-supplied XML schema (`STM32G474.svd`) enumerating every peripheral base address, register offset, bitfield position, access permission (`read-only`, `write-only`, `read-write`), and reset value. Loading `"svdFile": "${workspaceFolder}/STM32G474.svd"` in VS Code Cortex-Debug (`launch.json`) lets you inspect live decoded bitfields (`TIM2->CR1.CEN`, `ADC1->ISR.ADRDY`, `USART2->ISR.RXNE`) in GDB without manually decoding hex words.
- **CMSIS-DSP on ARMv7E-M (Cortex-M4F):** Naive scalar C FIR/biquad loops fail to exploit Cortex-M4's DSP extensions. Linking ARM **CMSIS-DSP** (`arm_fir_f32`, `arm_biquad_cascade_df1_q15`, `arm_rfft_fast_f32`) delivers 3×–8× cycle reductions (measurable via `DWT->CYCCNT`) by combining:
  - **Dual-MAC 16-bit SIMD (`SMLAD` / `SMLALD`):** Executes two Q15 multiply-accumulates per single cycle on packed `int16_t` pairs.
  - **Hardware Saturation (`SSAT` / `USAT`):** Single-cycle clamping (`__SSAT(acc, 16)`) with zero branch misprediction penalty.
  - **FPv4-SP Fused Multiply-Accumulate (`VFMA.F32`) & Loop Unrolling (4×):** Maximizes single-precision floating-point pipeline throughput.

---

### 3.2 Complete Register Access Taxonomy & Eliminating RMW Hazards (`EMB-REG-001..004`)

*Prerequisites: `EMB-MMIO-001`, `C-BIT-001`.*

Never blindly use `|=` or `&=` on hardware registers! Always check the access column in **RM0440 §1.1**:

| RM0440 Code | Hardware Semantics | Safe C Pattern | Classic Bug to NEVER Write |
|---|---|---|---|
| `rw` | Software can read and write freely. | Local shadow single-store `MODIFY_REG(REG, CLR, SET)` | Two-step `REG &= ~M; REG |= V;` (causes transient glitch state!) |
| `r` | Read-only (writes ignored or fault). | `uint32_t v = REG;` | Writing to status/ID register |
| `w` | **Write-only** (reading returns 0 or undefined). | **Direct assignment:** `TIM2->EGR = TIM_EGR_UG;`, `DMA1->IFCR = DMA_IFCR_CTCIF1;`, `USART2->ICR = USART_ICR_ORECF;` | `TIM2->EGR |= TIM_EGR_UG;` (unnecessary bogus read!) |
| **`rc_w1`** | Read, **Write `1` to Clear** (`0` has no effect). | **Direct assignment:** `EXTI->PR1 = EXTI_PR1_PIF13;`, `ADC1->ISR = ADC_ISR_ADRDY;` | **`ADC1->ISR |= ADC_ISR_ADRDY;`** — writes `1` back to EVERY other already-set flag, silently destroying concurrent status bits! |
| **`rc_w0`** | Read, **Write `0` to Clear** (`1` has no effect). | **Direct assignment:** `TIM2->SR = ~TIM_SR_UIF;` | **`TIM2->SR &= ~TIM_SR_UIF;`** — RMW race can clear a newly asserted `CC1IF` flag that rose between `LDR` and `STR`! |
| `rc_r` | Read clears bit automatically (clear-on-read). | Read once into local `uint32_t sr = REG;` and test `sr`. | Reading `REG` twice in two `if` statements (second read sees `0`!) |
| `rs` / `t` | Read, write `1` sets / toggles hardware state. | Direct assignment `REG = BIT_MASK;` | RMW `|=` accidentally re-triggering other `rs`/`t` bits |

**Reserved Bits, Atomic GPIO & STM32G4 Bit-Banding Scope (`EMB-REG-002`, `EMB-REG-004`):**
- Always preserve reserved bits at their reset value (RM0440 §1.1) using `MODIFY_REG(REG, CLEARMSK, SETMASK)` (`stm32g4xx.h`).
- Never use `GPIOA->ODR ^= (1U << 5)` or `GPIOA->ODR |= (1U << 5)` in concurrent main/ISR code (`LDR` → `ORR` → `STR` loses ISR updates!). Use single-store atomic **`GPIOA->BSRR`** (offset `0x18`: bits `[15:0]` set, bits `[31:16]` reset; if both `BSy` and `BRy` are written `1` simultaneously, **`BSy` set wins** per RM0440 §9.4.7) or **`GPIOA->BRR`** (offset `0x28`, bits `[15:0]` reset).
- **STM32G474RE Bit-Banding Scope (`stm32g474xx.h`):** `SRAM1_BB_BASE = 0x22000000UL` covers `0x20000000–0x200FFFFF` and `PERIPH_BB_BASE = 0x42000000UL` covers `0x40000000–0x400FFFFF` (APB1/APB2/AHB1), which **excludes AHB2 GPIO registers at `0x48000000`!** That is why `BSRR`/`BRR` are mandatory for atomic GPIO updates.

---

### 3.3 What `volatile` DOES and DOES NOT Guarantee (`EMB-VOL-001..003`)

*Prerequisites: `EMB-MMIO-001`, `C-QUAL-002`, `C-OPT-001..002`.*

| Property | Guaranteed by `volatile`? | Engineering Truth on ARM Cortex-M4 (`arm-none-eabi-gcc -O2`) |
|---|---|---|
| Forces fresh load/store on every access (no register caching / loop elimination) | **YES** | Prevents `while (!(USART2->ISR & RXNE));` from turning into an infinite `b .` loop, and ensures CPU re-reads DMA-written `volatile uint16_t adc_buf[]`. |
| Preserves program order **between two `volatile` accesses** | **YES** | Compiler cannot reorder `vol_a = 1; vol_b = 2;` relative to each other. |
| Prevents **non-volatile** accesses from reordering across a `volatile` access | **NO!** | GCC at `-O2` CAN legally hoist or sink `payload_buf[0] = 42;` *past* `volatile ready_flag = 1;`! **Requires `__asm__ volatile ("" ::: "memory")` or `__DMB()`!** |
| Guarantees **atomicity** of RMW (`v++`) or 64-bit reads (`volatile uint64_t`) | **NO!** | `v++` still compiles to non-atomic `LDR`/`ADD`/`STR`; `uint64_t` still tears across two 32-bit loads! |
| Guarantees **hardware bus visibility** to DMA1/DMA2 or completion of buffered writes | **NO!** | Requires ARMv7-M hardware barriers: **`__DMB()`** (DMA publish/consume), **`__DSB()`** (peripheral flag clear before ISR exit or `__WFI()`), **`__ISB()`** (pipeline flush after `SCB->VTOR`/`CPACR`/`MPU`). |

---

### 3.4 Critical Sections, Lock-Free Atomics, Seqlocks & ISR Discipline (`EMB-ATOM-001..004`, `EMB-ISR-001..002`)

*Prerequisites: `EMB-VOL-002..003`, `C-ATOM-002`, `C-LIB-001`.*

1. **Reentrant `PRIMASK` & Selective Cortex-M4 `BASEPRI` Critical Sections (`EMB-ATOM-001`):**
   - Never pair bare `__disable_irq()` with bare `__enable_irq()` inside utility functions — nested calls prematurely re-enable interrupts on inner exit! Always **save and restore `PRIMASK`** with compiler memory barriers (`crit_enter_primask` / `crit_exit_primask` in Section 8.1).
   - On STM32G474RE (`__NVIC_PRIO_BITS = 4U`, priority encoded in top bits `[7:4]`), `PRIMASK` masks *every* interrupt (adding jitter to 100 kHz digital power fault ISRs!). Instead, **`__set_BASEPRI_MAX(min_masked_prio << 4U)`** masks only IRQs with numeric priority $\ge p_{\text{mask}}$ (`min_masked_prio`) while **never weakening** a stricter outer `BASEPRI` threshold (hardware ignores any write with a numerically larger non-zero priority). Priority `0..(min_masked_prio - 1)` ISRs execute with **zero added latency** (the exact mechanism behind FreeRTOS `configMAX_SYSCALL_INTERRUPT_PRIORITY`; Section 8.1).
2. **Exclusive Load/Store (`__LDREXW`/`__STREXW`/`__CLREX`) & C11 `<stdatomic.h>` (`EMB-ATOM-002`):**
   - Cortex-M4 provides a **local exclusive monitor** accessed via `LDREX` / `STREX` (exposed via `cmsis_gcc.h` `__LDREXW`/`__STREXW`/`__CLREX`, `stm32g4xx.h` `ATOMIC_SET_BIT`/`ATOMIC_CLEAR_BIT`/`ATOMIC_MODIFY_REG`, and ISO C11 `<stdatomic.h>`).
   - **Why It Is ISR-Safe Without Masking Interrupts:** `__LDREXW(addr)` loads `*addr` and tags the local monitor *Exclusive*. If an ISR preempts before `__STREXW(new_val, addr)`, exception entry/return (`ERET`) automatically clears the local monitor to *Open*, causing `__STREXW` to abort the write and return `1` so the `do { ... } while` loop cleanly retries with fresh data! Always issue `__CLREX()` on early loop exit (Section 8.2).
   - **Mandatory Hardware Limit:** Cortex-M4's local exclusive monitor tracks CPU execution **only** — it does **NOT** observe AHB writes from **DMA1/DMA2**! Never use `LDREX`/`STREX` to synchronize a memory word written concurrently by DMA.
3. **Eliminating Torn Multi-Word Reads & TOCTOU Races (`EMB-ATOM-003`):**
   - **64-Bit Monotonic Counter Double-Read:** When combining a 32-bit hardware counter (`TIM2->CNT`) with a 32-bit overflow ISR counter (`g_tim2_overflow_hi`), reading naively tears across rollover (`hi` increments right after `lo` reads `0xFFFFFFFF`, producing a +4.29 billion tick jump). Read `hi1 → lo → hi2` with compiler barriers and retry while `hi1 != hi2` (Section 8.2).
   - **Multi-Field Seqlock Pattern:** For multi-field ISR-produced telemetry structs (`{ bus_mv, bus_ma, sample_id }`), add a `volatile uint32_t seq` counter. The ISR increments `seq` to **odd** before writing fields (`__DMB()`) and increments `seq` to **even** after writing (`__DMB()`). Thread mode spins while `seq` is odd, copies fields between `__DMB()` barriers, and retries if `seq` changed — giving **zero-lock, tear-free multi-word snapshots** with zero ISR masking!
4. **Lock-Free SPSC Ring Buffer (`EMB-ATOM-004`, `FW-PAT-004`):**
   Requires **zero interrupt disabling** when: (a) capacity $N = 2^k$, (b) `head` is written *only* by producer and `tail` *only* by consumer as free-running unsigned `uint32_t` indices (`count = (uint32_t)(head - tail)`), and (c) `__DMB()` (or C11 `memory_order_acquire`/`memory_order_release`) orders payload access relative to index publication (Section 8.1).
5. **ISR Reentrancy, Newlib Hazards & Top-Half/Bottom-Half WCET (`EMB-ISR-001`, `EMB-ISR-002`):**
   - Never call `printf`, `malloc`, `free`, `strtok`, or non-reentrant libc functions inside an ISR (see [Stage 02 Companion — `C-LIB-001`](../02-c-fundamentals/advanced-c-ub-memory-model.md#6-newlib--newlib-nano-thread-safety--reentrancy-c-lib-001)).
   - Keep Top-Half ISRs strictly **$< 1\text{--}5\ \mu\text{s}$** (verified via `DWT->CYCCNT`): capture hardware register, push into SPSC ring buffer / post event flag, clear peripheral flag + `__DSB()`, and defer parsing/formatting to Thread mode (Bottom-Half).

---

### 3.5 Zero-Overhead Static HAL Dispatch vs Runtime Vtables (`FW-DRV-003`)

*Prerequisites: `FW-DRV-001`, `C-QUAL-003`, `C-PREP-003`, `C-FPTR-001`.*

A central architectural decision in Embedded C firmware is **how upper layers dispatch into hardware drivers**: via **Runtime Function-Pointer Vtables** (`ops->set_high(ctx)`) or via **Zero-Overhead Static Dispatch** resolved entirely at compile/link time.

#### Four Zero-Overhead Static Dispatch Mechanisms in ISO C11

1. **`static inline` Board Policy Headers:** Bind board-specific pins/peripherals directly in `board_policy.h` (`static inline void hal_led_high(void) { GPIOA->BSRR = (1UL << 5U); }`). At `-O2`, GCC folds the call directly into the caller's instruction stream with **zero call/return overhead**.
2. **X-Macro Pin & Peripheral Tables (`C-PREP-003`):** Define every board pin once in `#define BOARD_LED_PINS(X) X(LED_STATUS, GPIOA, 5U) ...`, then expand it to generate enum IDs, clock-enable init code, and `static inline` register wrappers (`hal_LED_STATUS_high()`, `hal_LED_STATUS_low()`) with zero copy-paste drift (Section 8.3).
3. **C11 `_Generic` Compile-Time Type Dispatch:** Route polymorphic API calls (`HAL_WRITE(dev, val)`) to distinct `static inline` functions based on the compile-time type of `dev` (`led_pin_tag_t` vs `pwm_ch1_tag_t`), achieving type-safe polymorphism with **zero runtime branching and zero function pointers** (Section 8.3).
4. **Link-Time Polymorphism (Link-Time Seams):** Declare a single pure C header `hal_gpio.h` (`void hal_gpio_set(pin_id_t id);`), then link `hal_gpio_stm32g4.c` in the target firmware build and `hal_gpio_mock.c` in the host PC unit-test build! Enabling **`-flto`** on target builds recovers cross-TU inlining, yielding near-zero overhead while retaining 100% host unit testability.

#### Empirically Verified Cortex-M4 `-O2` Disassembly & Trade-Off Comparison (`arm-none-eabi-gcc 14.3.1`)

The table below reports **exact compiled measurements** from `arm-none-eabi-gcc 14.3.1 -std=c11 -mcpu=cortex-m4 -mthumb -O2` (`arm-none-eabi-objdump -d`) on the Section 8.3 functions toggling `PA5` high then low:

| Architectural Metric | Runtime Function-Pointer Vtable (`bench_vtable_pulse`) | `static inline` + X-Macro Policy (`bench_static_pulse`) | C11 `_Generic` Static Dispatch (`bench_generic_dispatch`) | Link-Time Seam (`hal_gpio.c` + `-flto`) |
|---|---|---|---|---|
| **Verified Caller Machine Code (`-O2`)** | **9 instructions (24 B):** `push {r4,lr}`, `2× ldrd`, `2× ldr`, **`blx r3`**, `ldmia`, **`bx r3`** (+ 2 separate callee fns + 8 B `.rodata` vtable + 8 B handle) | **5 instructions (12 B):** `mov.w r3, #0x48000000`, `movs r2, #32`, `str r2, [r3, #24]`, `str r2, [r3, #40]`, `bx lr` | **6 instructions (16 B):** direct `str r1, [r2, #24]` (`BSRR`) + `str r0, [r3, #52]` (`TIM2->CCR1`) + `bx lr` | Identical to `static inline` under `-flto`; direct `BL` without `-flto` |
| **Indirect Branch / Pipeline Flush?** | **Yes** (`BLX r3` + `BX r3`; blocks compiler inlining & constant folding) | **No** (100% inlined; zero stack frame) | **No** (100% resolved at compile time) | **No** (direct branch or fully inlined with `-flto`) |
| **Runtime Multi-Instance Polymorphism?** | **Yes** (swap I2C/SPI/UART implementations dynamically at runtime; e.g., two different sensors on same bus interface) | **No** (hardware mapping fixed at compile time) | **No** (type fixed at compile time) | **No** (one implementation per linked binary) |
| **Host PC Unit Testing (`FW-LAYER-003`)** | **Easiest:** inject per-test mock vtable structs into driver init at runtime | Requires swapping `board_policy.h` include path (`-Itests/fakes`) for host builds | Requires swapping tag structs / inline targets via header seam | **Clean:** link `hal_mock.c` on host, `hal_stm32g4.c` on target |
| **Ideal Embedded Use Case** | Bus abstractions (`i2c_adapter_t`, `spi_ops_t`), swappable storage/comms backends, runtime DI | Ultra-tight 100 kHz+ control ISRs (`HRTIM`, `ADC`, GPIO bit-banging), Flash/RAM-constrained MCUs | Heterogeneous compile-time-known peripherals with unified API syntax | Board-level singletons (`hal_time`, `hal_flash`, `hal_watchdog`) |

---

### 3.6 Execution Models & Firmware Abstraction Layer Trade-Offs (`FW-EXEC-001`, `FW-LAYER-001..003`)

*Prerequisites: `EMB-CMSIS-002`, `EMB-ISR-002`.*

#### Quantitative Execution Model Comparison (`FW-EXEC-001`)

| Metric | Blocking Polling | Interrupt-Driven (Per Byte/Event) | DMA + Half/Full Complete ISR |
|---|---|---|---|
| **CPU Utilization (1 Mbps SPI)** | ~100% (busy-wait) | ~35–70% (125,000 IRQs/sec = interrupt storm!) | **< 1%** (CPU sleeps in `WFI`, 1 IRQ per buffer!) |
| **Worst-Case Latency** | Unbounded (blocked by other tasks) | ~12 cycles (70 ns @ 170 MHz) | Hardware-deterministic (0 CPU cycles/sample) |
| **Power Consumption** | Highest (CPU active 100%) | Moderate | **Lowest** (CPU in Sleep mode during transfer) |
| **Best Use Case** | One-shot startup (`PLL` lock, `VOSF` wait **with bounded timeout!**) | Low-rate async events (`EXTI` button, UART CLI, <20 kHz timers) | High-rate streams (ADC multi-channel, SPI displays, 921 kbps UART, DAC/CORDIC/FMAC) |

#### Objective Firmware Abstraction Matrix (`FW-LAYER-002`)

| Dimension | Direct Raw Addresses | CMSIS Register Access (`GPIOA->BSRR`) | ST Low-Layer (LL) (`LL_GPIO_SetOutputPin`) | ST HAL (`HAL_GPIO_WritePin`) |
|---|---|---|---|---|
| **Flash / RAM Overhead** | Minimal | Minimal (0 B overhead) | Very Low (`static inline`) | High (10–40 KB Flash + handle structs) |
| **Execution Latency** | 1–2 cycles | 1–2 cycles | 1–3 cycles | 15–80+ cycles (null/lock/assert checks) |
| **Silicon Errata (ES0430) Handled?** | Manual | Manual | Partial | Broadly patched by ST |
| **STM32 Cross-Family Portability** | None | Low | Medium | High (G4 ↔ H7 ↔ U5) |
| **Real-World Industry Practice** | Learning only | Bootloaders, ultra-tight ISRs | High-speed control loops (`HRTIM`, `ADC`, `DMA`) | Complex stacks (`USB`, `FDCAN`, bring-up) — **Hybrid HAL (init/USB/FDCAN) + LL/CMSIS (100 kHz ISRs)** is the industry gold standard! |

---

### 3.7 Bare-Metal Architectures, Async Drivers, HSMs & 9 Embedded Design Patterns (`FW-EXEC-002..003`, `FW-DRV-001..002`, `FW-FSM-001..002`, `FW-PAT-001..009`)

*Prerequisites: `FW-LAYER-001`, `C-FPTR-001..003`, `EMB-ATOM-001`.*

1. **Strict Layered Architecture & Dependency Injection (`FW-LAYER-001`, `FW-LAYER-003`, `FW-MOD-001`):**
   Dependencies point **strictly downward**: `App → Middleware → Device Drivers → BSP / MCU Drivers → CMSIS`. Application and middleware `.c` files must have **zero `#include "stm32g474xx.h"`**. Inject bus interfaces via `const i2c_adapter_t *` vtable structs (or link-time seams, Section 3.5) so sensor/protocol drivers compile and unit-test natively on host GCC/Unity with 100% mocked hardware!
2. **Production Multi-Instance & Asynchronous Driver Rules (`FW-DRV-001`, `FW-DRV-002`):**
   - Expose **opaque handles** (`typedef struct uart_handle uart_handle_t;`), `const *_config_t *cfg` init structs, non-blocking async APIs (`uart_write_async(h, buf, len, cb, user_ctx)` returning `__attribute__((warn_unused_result)) drv_status_t` — `DRV_OK`/`DRV_ERR_BUSY`/`DRV_ERR_PARAM`; Section 8.4), and **bounded tick timeouts** on *every* hardware polling loop (never unbounded `while (!(REG & FLAG));`!).
3. **Race-Free `WFI` Superloop & Cooperative Time-Triggered Scheduler (`FW-EXEC-002`, `FW-EXEC-003`):**
   - **Lost-Wakeup `WFI` Bug:** In `if (!g_evt) { __WFI(); }`, an ISR firing *between* the check and `__WFI()` sets `g_evt = 1` and returns, and the CPU then sleeps forever!
   - **Cortex-M Race-Free `WFI` Pattern (ARM DDI 0403E.e §B1.5.18):** ARMv7-M wakes from `WFI` on any pending unmasked NVIC IRQ **even when `PRIMASK = 1`**:
     ```c
     __disable_irq();
     if (!event_pending()) {
         __DSB();
         __WFI();
     }
     __enable_irq(); /* Pending ISR executes immediately here! */
     ```
   - **Cooperative TTC Scheduler:** Static task table `{ period_ms, last_run_ms, fn }` driven by 1 ms `SysTick`, testing wraparound-safe `(uint32_t)(now - t->last_run_ms) >= t->period_ms`.
4. **Finite State Machines (FSM), Hierarchical State Machines (HSM) & Active Objects (`FW-FSM-001`, `FW-FSM-002`):**
   - **Switch-Case / `.rodata` Table FSM (`FW-FSM-001`):** Clean flat state machines for 3–6 state protocols (see [Stage 02 Section 8.4](../02-c-fundamentals/README.md#84-callback-registration-void-ctx--flash-rodata-table-driven-fsm-c-fptr-002-c-fptr-003)).
   - **Hierarchical State Machines (HSM) & Active Objects (`FW-FSM-002`):** Eliminates state-transition explosion by nesting substates (`hsm_idle`, `hsm_running`) under a parent superstate (`hsm_operational`) that handles shared events like `HSM_EVT_FAULT` once! Unhandled events return `HSM_SUPER` to bubble up to the parent, while transitions execute explicit `HSM_EVT_EXIT` / `HSM_EVT_ENTRY` actions along the Least-Common-Ancestor (LCA) path (Section 8.5). Pairing an HSM with a private static SPSC event queue and a Run-to-Completion (RTC) dispatch loop creates an **Active Object**, eliminating shared-state mutex races.
5. **Nine Production Embedded Design Patterns (`FW-PAT-001..009`):**
   - **Observer (`FW-PAT-001`):** Static fixed-capacity subscriber registry `{ cb, void *ctx }` decoupling drivers from multiple independent consumers (Section 8.5).
   - **Hardware Proxy & Adapter (`FW-PAT-002`):** Proxy encapsulates hardware registers behind domain APIs (`temp_proxy_read_mdegc()`); Adapter translates a vendor-neutral `i2c_adapter_t` interface onto STM32G474RE I2C/SPI registers (Section 8.4).
   - **Non-Blocking Debouncer (`FW-PAT-003`):** 1 ms sampled shift register (`history = (history << 1) | raw | 0xE000U`, clean rising edge at `0xF000U`) — zero blocking `delay_ms()` (Section 8.1).
   - **Ring Buffer (`FW-PAT-004`):** Circular FIFO with explicit drop-newest vs overwrite-oldest overrun policy and diagnostic overrun counter (Section 8.1).
   - **$O(1)$ Fixed-Block Memory Pool (`FW-PAT-005`):** Intrusive free-list static pool allocator (`pool_alloc_o1` / `pool_free_o1`) replacing `malloc`/`free` with strictly constant-time allocation and **zero external fragmentation** (Section 8.5).
   - **DMA Ping-Pong Double Buffer (`FW-PAT-006`):** `volatile uint16_t buf[2][N]` driven by circular DMA `HTIF`/`TCIF` ISRs publishing half-index via `__DMB()` with overrun detection.
   - **Command Pattern (`FW-PAT-007`):** Flash `.rodata` CLI dispatch table `static const cli_cmd_t g_cli_cmds[]` mapping command names to handlers and help strings (Section 8.5).
   - **Singleton Ownership (`FW-PAT-008`):** Static peripheral registry guarding exclusive runtime ownership (`USART2`, `I2C1`) so two modules cannot silently reconfigure the same peripheral.
   - **Software Timer Multiplexer (`FW-PAT-009`):** Multiplexes one 1 ms hardware tick into $N$ one-shot/periodic virtual timers with wraparound-safe expiration checks.

---

### 3.8 Defensive Engineering, MISRA C:2012/2023, SEI CERT C & Deviations (`FW-ERR-001..003`, `EMB-STD-001..003`)

*Prerequisites: `C-UB-003`, `C-INT-004`, `C-QUAL-004`.*

- **Assertions vs Error Codes (`FW-ERR-001`):** Use `FW_ASSERT(cond)` (Design by Contract) for **internal programming invariants/bugs** (null pointer passed to internal helper, unreachable switch case) capturing module ID + line + `LR` into `.noinit` SRAM and halting via `__BKPT(0)` when attached to GDB. Return explicit `drv_status_t` error codes (annotated `__attribute__((warn_unused_result))`) for **recoverable external runtime faults** (I2C NACK, UART framing error, sensor timeout).
- **MISRA C:2012 / C:2023 (`EMB-STD-001`):** Classifies rules & directives as **Mandatory** (never deviate), **Required** (comply or file formal deviation), or **Advisory**. Key embedded rules: Essential Type Model (10.1–10.8), Integer-to-pointer casts (11.4), Shift bounds (12.2), Switch `default` (16.4), Recursion ban (17.2), Check return values (17.7), No VLAs (18.8), No `malloc`/`free` (21.3). **MISRA C:2023** integrates C11/C18 features (`_Generic`, `_Static_assert`, `<stdatomic.h>`).
- **Formal MISRA Deviation Governance (`EMB-STD-002`, MISRA Compliance:2020):** Bare-metal MMIO casts (`(GPIO_TypeDef *)0x48000000UL`) violate Required Rule 11.4. Record a **Deviation Permit** containing Deviation ID, Rule ID, Scope, Technical Justification, Risk Assessment & Mitigation (verified against DS12288/RM0440 + `_Static_assert`), and Reviewer Sign-Off, paired with a scoped analyzer comment (`/* cppcheck-suppress misra-c2012-11.4 */`).
- **SEI CERT C (`EMB-STD-003`):** Eliminates exploitable security & reliability defects: `INT30-C` (unsigned wrap), `INT32-C` (signed overflow), `ARR30-C` (array bounds), `STR31-C` (null-termination space), `EXP33-C` (uninitialized reads), `CON40-C`/`CON43-C` (concurrency).

---

## 4. Important Terminology

| Term | Engineering Definition |
|---|---|
| **MMIO (Memory-Mapped I/O)** | Hardware architecture mapping peripheral control/status/data registers onto standard CPU memory addresses accessed via `LDR`/`STR`. |
| **Read-Modify-Write (RMW) Hazard** | Non-atomic 3-step update (`LDR` → modify → `STR`) vulnerable to lost updates if an ISR preempts mid-sequence, or unintended flag clearing on `rc_w1`/`rc_w0` registers. |
| **Local Exclusive Monitor** | Cortex-M4 hardware state machine tracking `LDREX`/`STREX` pairs; automatically cleared on exception entry/return (`ERET`) to provide lock-free CPU-vs-ISR atomicity (not DMA). |
| **Seqlock** | Lock-free single-writer / multi-reader synchronization primitive using an odd/even sequence counter + `__DMB()` barriers for tear-free multi-word snapshots without masking interrupts. |
| **Static HAL Dispatch** | Compile/link-time hardware binding via `static inline`, X-Macros, C11 `_Generic`, or link-time seams eliminating indirect `BLX` calls and enabling full compiler inlining. |
| **Opaque Handle** | Incomplete struct pointer (`typedef struct drv_ctx drv_ctx_t;`) hiding private driver fields inside `.c` so callers cannot couple to internal state. |
| **Run-to-Completion (RTC)** | Event-driven execution rule where each event handler finishes completely before the next queued event is dispatched (zero blocking waits inside state handlers). |
| **Deviation Record** | Formal audited justification permitting a localized violation of a MISRA Required rule (e.g., Rule 11.4 for MMIO pointers) under MISRA Compliance:2020. |

---

## 5. Register-Level Explanations

Verified directly against `stm32g474xx.h`, `core_cm4.h`, and `cmsis_gcc.h`:
- **`GPIOA->BSRR` (`0x48000018`, `w`):** Writing `1UL << n` sets pin $n$; writing `1UL << (n + 16U)` resets pin $n$ atomically in a single `STR`. Zero RMW!
- **`GPIOA->BRR` (`0x48000028`, `w`):** Writing `1UL << n` resets pin $n$ atomically in a single `STR`.
- **`EXTI->PR1` (`0x40010414`, `rc_w1`):** Clear pending EXTI13 via `EXTI->PR1 = EXTI_PR1_PIF13;` (NEVER `|=`).
- **`TIM2->SR` (`0x40000010`, `rc_w0`):** Clear Update Interrupt Flag via `TIM2->SR = ~TIM_SR_UIF;` (NEVER `&= ~TIM_SR_UIF`).
- **`ADC1->ISR` (`0x50000000`, `rc_w1`):** Clear `ADRDY` via `ADC1->ISR = ADC_ISR_ADRDY;` (NEVER `|=`).
- **`USART2->CR1` / `ISR` / `TDR` / `RDR` (`0x40004400`):** Async TX enables `USART_CR1_TXEIE_TXFNFIE`, checks `USART_ISR_TXE_TXFNF`, writes `USART2->TDR`; reading `USART2->RDR` clears `RXNE` (`rc_r`).
- **`PRIMASK` & `BASEPRI` (`cmsis_gcc.h`):** `__get_PRIMASK()`, `__set_PRIMASK()`, `__get_BASEPRI()`, `__set_BASEPRI_MAX(prio << 4U)`, `__set_BASEPRI()` control Cortex-M4 exception masking (`__NVIC_PRIO_BITS = 4U` on STM32G474RE).

---

## 6. Relevant Formulas & Calculations

### 6.1 Wraparound-Safe Cooperative Scheduler Condition
$$\left(t_{\text{now}} - t_{\text{last}}\right) \bmod 2^{32} \ge T_{\text{period}}$$

### 6.2 SPSC Power-of-Two Ring Buffer Indexing & Occupancy
With capacity $N = 2^k$ and free-running unsigned `uint32_t head, tail`:
$$\text{count} = (\text{head} - \text{tail}) \bmod 2^{32}, \qquad \text{index}(i) = i \mathbin{\&} (N - 1)$$
$$\text{empty} \iff (\text{head} == \text{tail}), \qquad \text{full} \iff \left((\text{head} - \text{tail}) \bmod 2^{32}\right) == N$$

### 6.3 Cortex-M4 `BASEPRI` Register Encoding on STM32G474RE
With $N_{\text{prio}} = 4$ (`__NVIC_PRIO_BITS`) and numeric priority level $p \in [0..15]$:
$$\text{BASEPRI} = p \ll (8 - N_{\text{prio}}) = p \ll 4$$

---

## 7. Official Documentation References

- **ST RM0440** — *STM32G4 Series Reference Manual*: §1.1 (Register Access Abbreviations `rw`/`r`/`w`/`rc_w1`/`rc_w0`/`rc_r`), §2.3 (Bit-Banding Scope), §6.2.17 (Peripheral Clock-Enable Sync Delay), §9.4.7 (`GPIOx_BSRR`), §9.4.11 (`GPIOx_BRR`), §40.4.9 (SPI 8/16-bit FIFO Packing).
- **ARM AN321 (DAI 0321A)** — *ARM Cortex-M Programming Guide to Memory Barrier Instructions* (`DMB`, `DSB`, `ISB`).
- **ARM DDI 0403E.e** — *ARMv7-M Architecture Reference Manual*: §A3.4 (`LDREX`/`STREX`/`CLREX`), §B1.4.3 (`PRIMASK`/`BASEPRI`), §B1.5.18 (`WFI` with `PRIMASK`).
- **ST PM0214** — *STM32 Cortex-M4 MCUs Programming Manual* & **ST ES0430** — *STM32G474xB/xC/xE Errata Sheet*.
- **MISRA C:2023**, **MISRA Compliance:2020**, **SEI CERT C**, **BARR-C:2018**.
- **Elecia White** — *Making Embedded Systems*; **Miro Samek** — *Practical UML Statecharts in C/C++ (PSiCC2)*; **James Grenning** — *TDD for Embedded C*; **Bruce Powel Douglass** — *Design Patterns for Embedded Systems in C*.

---

## 8. Verified Code Examples

> **Verification Status:** All C11 snippets below are **Compile-Verified (`arm-none-eabi-gcc 14.3.1`, `-std=c11 -mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard -DSTM32G474xx -O2 -Wall -Wextra -Werror -Wdouble-promotion -Wvla`)** against vendored `stm32g474xx.h`, `core_cm4.h`, and `cmsis_gcc.h` (**Not Tested on Physical Hardware**).

### 8.1 Nested `PRIMASK`/`BASEPRI` Critical Sections, Lock-Free SPSC Ring Buffer & Non-Blocking Debouncer (`EMB-ATOM-001`, `EMB-ATOM-004`, `FW-PAT-003..004`)

```c
#include <stdbool.h>
#include <stdint.h>
#include "stm32g474xx.h"

/* EMB-ATOM-001: Nesting-safe PRIMASK critical section with compiler memory barriers */
static inline uint32_t crit_enter_primask(void)
{
    uint32_t prev = __get_PRIMASK();
    __disable_irq();
    __asm__ volatile ("" ::: "memory");
    return prev;
}

static inline void crit_exit_primask(uint32_t prev_primask)
{
    __asm__ volatile ("" ::: "memory");
    __set_PRIMASK(prev_primask);
}

/* EMB-ATOM-001: Nesting-safe Cortex-M4 BASEPRI critical section (keeps prio 0..N-1 zero-latency!) */
static inline uint32_t crit_enter_basepri(uint8_t min_masked_prio)
{
    uint32_t prev    = __get_BASEPRI();
    uint32_t encoded = ((uint32_t)min_masked_prio << (8U - __NVIC_PRIO_BITS)) & 0xFFUL;
    /* __set_BASEPRI_MAX raises mask threshold without ever weakening a stricter outer mask */
    __set_BASEPRI_MAX(encoded);
    __asm__ volatile ("" ::: "memory");
    return prev;
}

static inline void crit_exit_basepri(uint32_t prev_basepri)
{
    __asm__ volatile ("" ::: "memory");
    __set_BASEPRI(prev_basepri);
}

/* EMB-ATOM-004 / FW-PAT-004: Lock-free SPSC Ring Buffer with __DMB() barriers */
#define SPSC_CAPACITY 64U
_Static_assert((SPSC_CAPACITY & (SPSC_CAPACITY - 1U)) == 0U, "Capacity must be power of 2");

typedef struct {
    uint8_t           buf[SPSC_CAPACITY];
    volatile uint32_t head;     /* Written ONLY by producer (e.g., UART RX ISR) */
    volatile uint32_t tail;     /* Written ONLY by consumer (e.g., main loop)   */
    volatile uint32_t overruns; /* Overrun diagnostic counter                   */
} spsc_ring_t;

static inline bool spsc_push(spsc_ring_t *rb, uint8_t byte)
{
    uint32_t head = rb->head;
    uint32_t tail = rb->tail;
    if ((uint32_t)(head - tail) >= SPSC_CAPACITY) {
        rb->overruns++;
        return false;
    }
    rb->buf[head & (SPSC_CAPACITY - 1U)] = byte;
    __DMB(); /* Payload write globally visible BEFORE publishing head */
    rb->head = head + 1U;
    return true;
}

static inline bool spsc_pop(spsc_ring_t *rb, uint8_t *out_byte)
{
    uint32_t tail = rb->tail;
    uint32_t head = rb->head;
    if (head == tail) { return false; }
    __DMB(); /* head load completes BEFORE reading payload */
    *out_byte = rb->buf[tail & (SPSC_CAPACITY - 1U)];
    __DMB(); /* payload read completes BEFORE freeing slot via tail */
    rb->tail = tail + 1U;
    return true;
}

/* FW-PAT-003: Non-blocking 1 ms shift-register debouncer */
typedef struct { uint16_t history; } debounce_t;

static inline bool debounce_update_rising(debounce_t *d, bool raw_pressed)
{
    d->history = (uint16_t)((d->history << 1U) | (raw_pressed ? 1U : 0U) | 0xE000U);
    return (d->history == 0xF000U); /* 12 consecutive HIGH samples after LOW */
}
```

### 8.2 `LDREX`/`STREX` Atomics, 64-Bit Double-Read & Multi-Field Seqlock (`EMB-ATOM-002`, `EMB-ATOM-003`)

```c
#include <stdbool.h>
#include <stdint.h>
#include "stm32g474xx.h"

/* EMB-ATOM-002: Lock-free Cortex-M4 __LDREXW/__STREXW bitmask OR (CPU vs ISR only; NOT DMA!) */
uint32_t ldrex_atomic_or(volatile uint32_t *addr, uint32_t mask)
{
    uint32_t old_val;
    uint32_t new_val;
    do {
        old_val = __LDREXW(addr);
        new_val = old_val | mask;
    } while (__STREXW(new_val, addr) != 0UL);
    __DMB();
    return old_val;
}

/* EMB-ATOM-002: Conditional refcount decrement with clean __CLREX() on early exit */
bool ldrex_refcount_dec_if_nonzero(volatile uint32_t *refcnt)
{
    uint32_t val;
    do {
        val = __LDREXW(refcnt);
        if (val == 0UL) {
            __CLREX();
            return false;
        }
    } while (__STREXW(val - 1UL, refcnt) != 0UL);
    __DMB();
    return true;
}

/* EMB-ATOM-003: Tear-free 64-bit TIM2 monotonic counter double-read */
static volatile uint32_t g_tim2_overflow_hi = 0UL;

uint64_t tim2_read_u64_tearfree(void)
{
    uint32_t hi1, lo, hi2;
    do {
        hi1 = g_tim2_overflow_hi;
        __asm__ volatile ("" ::: "memory");
        lo  = TIM2->CNT;
        __asm__ volatile ("" ::: "memory");
        hi2 = g_tim2_overflow_hi;
    } while (hi1 != hi2);
    return ((uint64_t)hi1 << 32U) | (uint64_t)lo;
}

/* EMB-ATOM-003: Zero-lock Multi-Field Seqlock (ISR writer, Thread-mode reader) */
typedef struct {
    volatile uint32_t seq; /* Odd = write active; Even = consistent snapshot */
    int32_t  bus_mv;
    int32_t  bus_ma;
    uint32_t sample_id;
} power_seqlock_t;

void seqlock_write_isr(power_seqlock_t *sl, int32_t mv, int32_t ma, uint32_t id)
{
    sl->seq = sl->seq + 1UL; /* Mark odd */
    __DMB();
    sl->bus_mv    = mv;
    sl->bus_ma    = ma;
    sl->sample_id = id;
    __DMB();
    sl->seq = sl->seq + 1UL; /* Mark even */
}

void seqlock_read_thread(const power_seqlock_t *sl, int32_t *mv, int32_t *ma, uint32_t *id)
{
    uint32_t s1, s2;
    do {
        do { s1 = sl->seq; } while ((s1 & 1UL) != 0UL);
        __DMB();
        *mv = sl->bus_mv;
        *ma = sl->bus_ma;
        *id = sl->sample_id;
        __DMB();
        s2 = sl->seq;
    } while (s1 != s2);
}
```

### 8.3 Zero-Overhead Static HAL Dispatch (X-Macros, `static inline`, `_Generic`) vs Runtime Vtable (`FW-DRV-003`)

```c
#include <stdint.h>
#include "stm32g474xx.h"

/* 1) Runtime Function-Pointer Vtable (9 caller insns / 24 B + BLX r3 + 2 callees at -O2) */
typedef struct {
    void (*set_high)(void *ctx);
    void (*set_low)(void *ctx);
} gpio_vtable_t;

typedef struct {
    const gpio_vtable_t *ops;
    void                *ctx;
} gpio_dyn_handle_t;

typedef struct {
    GPIO_TypeDef *port;
    uint16_t      pin_mask;
} stm32_gpio_ctx_t;

static void stm32_vtable_set_high(void *ctx)
{
    const stm32_gpio_ctx_t *g = (const stm32_gpio_ctx_t *)ctx;
    g->port->BSRR = (uint32_t)g->pin_mask;
}

static void stm32_vtable_set_low(void *ctx)
{
    const stm32_gpio_ctx_t *g = (const stm32_gpio_ctx_t *)ctx;
    g->port->BRR = (uint32_t)g->pin_mask;
}

const gpio_vtable_t g_stm32_gpio_ops = {
    .set_high = stm32_vtable_set_high,
    .set_low  = stm32_vtable_set_low,
};

void bench_vtable_pulse(const gpio_dyn_handle_t *h)
{
    h->ops->set_high(h->ctx);
    h->ops->set_low(h->ctx);
}

/* 2) Zero-Overhead X-Macro + static inline Board Policy (5 insns / 12 B, direct STR at -O2) */
#define BOARD_LED_PINS(X)     \
    X(LED_STATUS, GPIOA, 5U)  \
    X(LED_FAULT,  GPIOB, 0U)

#define X_PIN_WRAPPERS(name, port, pin)                          \
    static inline void hal_##name##_high(void)                   \
    {                                                            \
        (port)->BSRR = (1UL << (pin));                           \
    }                                                            \
    static inline void hal_##name##_low(void)                    \
    {                                                            \
        (port)->BRR = (1UL << (pin));                            \
    }

BOARD_LED_PINS(X_PIN_WRAPPERS)
#undef X_PIN_WRAPPERS

void bench_static_pulse(void)
{
    hal_LED_STATUS_high();
    hal_LED_STATUS_low();
}

/* 3) C11 _Generic Compile-Time Type Dispatch (6 insns / 16 B, zero runtime branch at -O2) */
typedef struct { GPIO_TypeDef *port; uint32_t mask; } led_pin_tag_t;
typedef struct { TIM_TypeDef  *tim;                 } pwm_ch1_tag_t;

static inline void dev_write_led(led_pin_tag_t d, uint32_t on)
{
    if (on != 0UL) { d.port->BSRR = d.mask; }
    else           { d.port->BRR  = d.mask; }
}

static inline void dev_write_pwm(pwm_ch1_tag_t d, uint32_t ccr)
{
    d.tim->CCR1 = ccr;
}

#define HAL_WRITE(dev, val) _Generic((dev), \
    led_pin_tag_t: dev_write_led,           \
    pwm_ch1_tag_t: dev_write_pwm            \
)((dev), (val))

void bench_generic_dispatch(uint32_t duty)
{
    const led_pin_tag_t led = { .port = GPIOA, .mask = (1UL << 5U) };
    const pwm_ch1_tag_t pwm = { .tim  = TIM2 };
    HAL_WRITE(led, 1UL);
    HAL_WRITE(pwm, duty);
}
```

### 8.4 Non-Blocking Async Driver API & Hardware Proxy/Adapter (`FW-DRV-002`, `FW-PAT-002`)

```c
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "stm32g474xx.h"

typedef enum {
    DRV_OK = 0,
    DRV_ERR_PARAM,
    DRV_ERR_BUSY,
    DRV_ERR_TIMEOUT,
    DRV_ERR_HW
} drv_status_t;

/* FW-DRV-002: Non-blocking asynchronous UART TX driver with void *ctx callback */
typedef enum { UART_ST_IDLE = 0, UART_ST_TX_BUSY, UART_ST_ERROR } uart_state_t;
typedef void (*uart_tx_cb_t)(drv_status_t status, void *user_ctx);

typedef struct {
    USART_TypeDef        *inst;
    const uint8_t        *tx_ptr;
    size_t                tx_rem;
    volatile uart_state_t state;
    uart_tx_cb_t          cb;
    void                 *cb_ctx;
} uart_async_t;

__attribute__((warn_unused_result))
drv_status_t uart_write_async(uart_async_t *h, const uint8_t *buf, size_t len,
                              uart_tx_cb_t cb, void *user_ctx)
{
    if ((h == NULL) || (h->inst == NULL) || (buf == NULL) || (len == 0U)) {
        return DRV_ERR_PARAM;
    }
    uint32_t pm = __get_PRIMASK();
    __disable_irq();
    __asm__ volatile ("" ::: "memory");
    if (h->state != UART_ST_IDLE) {
        __set_PRIMASK(pm);
        return DRV_ERR_BUSY;
    }
    h->tx_ptr = buf;
    h->tx_rem = len;
    h->cb     = cb;
    h->cb_ctx = user_ctx;
    h->state  = UART_ST_TX_BUSY;
    h->inst->CR1 |= USART_CR1_TXEIE_TXFNFIE;
    __asm__ volatile ("" ::: "memory");
    __set_PRIMASK(pm);
    return DRV_OK;
}

void uart_async_isr(uart_async_t *h)
{
    if ((h == NULL) || (h->inst == NULL)) { return; }
    if ((h->inst->ISR & USART_ISR_TXE_TXFNF) != 0UL) {
        if (h->tx_rem > 0U) {
            h->inst->TDR = *h->tx_ptr++;
            h->tx_rem--;
        }
        if (h->tx_rem == 0U) {
            h->inst->CR1 &= ~USART_CR1_TXEIE_TXFNFIE;
            h->state = UART_ST_IDLE;
            if (h->cb != NULL) { h->cb(DRV_OK, h->cb_ctx); }
        }
    }
}

/* FW-PAT-002: Hardware Proxy + Vendor-Neutral I2C Adapter (100% host unit-testable!) */
typedef struct {
    drv_status_t (*read_reg)(void *bus_ctx, uint8_t dev_addr, uint8_t reg, uint8_t *dst, size_t n);
    void *bus_ctx;
} i2c_adapter_t;

typedef struct {
    const i2c_adapter_t *bus;
    uint8_t              i2c_addr;
} temp_sensor_proxy_t;

__attribute__((warn_unused_result))
drv_status_t temp_proxy_read_mdegc(const temp_sensor_proxy_t *s, int32_t *out_mdegc)
{
    if ((s == NULL) || (s->bus == NULL) || (s->bus->read_reg == NULL) || (out_mdegc == NULL)) {
        return DRV_ERR_PARAM;
    }
    uint8_t raw[2] = { 0U, 0U };
    drv_status_t st = s->bus->read_reg(s->bus->bus_ctx, s->i2c_addr, 0x00U, raw, sizeof(raw));
    if (st != DRV_OK) { return st; }
    int16_t counts = (int16_t)(((uint16_t)raw[0] << 8U) | raw[1]);
    *out_mdegc = ((int32_t)(counts >> 4) * 625L) / 10L; /* 0.0625 degC/LSB -> milli-degC */
    return DRV_OK;
}
```

### 8.5 HSM, O(1) Memory Pool, Observer & Command Patterns (`FW-PAT-001`, `005`, `007`, `FW-FSM-002`)

```c
#include <stdalign.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include "stm32g474xx.h"

/* FW-FSM-002: Minimal Hierarchical State Machine (superstate FAULT handling + ENTRY/EXIT) */
typedef enum { HSM_EVT_ENTRY = 0, HSM_EVT_EXIT, HSM_EVT_START, HSM_EVT_STOP, HSM_EVT_FAULT } hsm_evt_t;
typedef enum { HSM_HANDLED = 0, HSM_SUPER, HSM_TRAN } hsm_ret_t;

struct hsm_s;
typedef hsm_ret_t (*hsm_state_fn_t)(struct hsm_s *me, hsm_evt_t evt);

typedef struct hsm_s {
    hsm_state_fn_t state;
    hsm_state_fn_t next_state;
    uint32_t       fault_count;
    bool           motor_en;
} hsm_t;

static hsm_ret_t hsm_top(hsm_t *me, hsm_evt_t evt) { (void)me; (void)evt; return HSM_HANDLED; }
static hsm_ret_t hsm_fault(hsm_t *me, hsm_evt_t evt);
static hsm_ret_t hsm_operational(hsm_t *me, hsm_evt_t evt);
static hsm_ret_t hsm_idle(hsm_t *me, hsm_evt_t evt);
static hsm_ret_t hsm_running(hsm_t *me, hsm_evt_t evt);

static hsm_ret_t hsm_operational(hsm_t *me, hsm_evt_t evt)
{
    if (evt == HSM_EVT_FAULT) { me->fault_count++; me->next_state = hsm_fault; return HSM_TRAN; }
    if ((evt == HSM_EVT_ENTRY) || (evt == HSM_EVT_EXIT)) { return HSM_HANDLED; }
    me->next_state = hsm_top;
    return HSM_SUPER;
}

static hsm_ret_t hsm_idle(hsm_t *me, hsm_evt_t evt)
{
    if (evt == HSM_EVT_ENTRY) { me->motor_en = false; return HSM_HANDLED; }
    if (evt == HSM_EVT_START) { me->next_state = hsm_running; return HSM_TRAN; }
    me->next_state = hsm_operational; /* Delegates HSM_EVT_FAULT to superstate */
    return HSM_SUPER;
}

static hsm_ret_t hsm_running(hsm_t *me, hsm_evt_t evt)
{
    if (evt == HSM_EVT_ENTRY) { me->motor_en = true;  return HSM_HANDLED; }
    if (evt == HSM_EVT_EXIT)  { me->motor_en = false; return HSM_HANDLED; }
    if (evt == HSM_EVT_STOP)  { me->next_state = hsm_idle; return HSM_TRAN; }
    me->next_state = hsm_operational; /* Inherits FAULT transition without duplication */
    return HSM_SUPER;
}

static hsm_ret_t hsm_fault(hsm_t *me, hsm_evt_t evt)
{
    if (evt == HSM_EVT_ENTRY) { me->motor_en = false; return HSM_HANDLED; }
    if (evt == HSM_EVT_STOP)  { me->next_state = hsm_idle; return HSM_TRAN; }
    me->next_state = hsm_top;
    return HSM_SUPER;
}

void hsm_dispatch(hsm_t *me, hsm_evt_t evt)
{
    hsm_state_fn_t cur = me->state;
    hsm_ret_t r;
    do {
        r = cur(me, evt);
        if (r == HSM_SUPER) { cur = me->next_state; }
    } while ((r == HSM_SUPER) && (cur != hsm_top));

    if (r == HSM_TRAN) {
        hsm_state_fn_t target = me->next_state;
        (void)me->state(me, HSM_EVT_EXIT);
        me->state = target;
        (void)me->state(me, HSM_EVT_ENTRY);
    }
}

/* FW-PAT-005: O(1) Deterministic Fixed-Block Memory Pool (zero fragmentation, zero malloc) */
#define POOL_BLOCK_SIZE  32U
#define POOL_BLOCK_COUNT 16U
_Static_assert(POOL_BLOCK_SIZE >= sizeof(void *), "Block must hold freelist pointer");
_Static_assert((POOL_BLOCK_SIZE % sizeof(void *)) == 0U, "Block size must be pointer-aligned");

typedef struct pool_node { struct pool_node *next; } pool_node_t;

typedef struct {
    _Alignas(max_align_t) uint8_t storage[POOL_BLOCK_COUNT][POOL_BLOCK_SIZE];
    pool_node_t *free_head;
    size_t       free_count;
} fixed_pool_t;

void pool_init(fixed_pool_t *p)
{
    p->free_head = NULL;
    for (size_t i = 0U; i < POOL_BLOCK_COUNT; ++i) {
        pool_node_t *node = (pool_node_t *)(void *)p->storage[i];
        node->next   = p->free_head;
        p->free_head = node;
    }
    p->free_count = POOL_BLOCK_COUNT;
}

void *pool_alloc_o1(fixed_pool_t *p)
{
    uint32_t pm = __get_PRIMASK();
    __disable_irq();
    __asm__ volatile ("" ::: "memory");
    pool_node_t *node = p->free_head;
    if (node != NULL) { p->free_head = node->next; p->free_count--; }
    __asm__ volatile ("" ::: "memory");
    __set_PRIMASK(pm);
    return (void *)node;
}

void pool_free_o1(fixed_pool_t *p, void *blk)
{
    if (blk == NULL) { return; }
    uint32_t pm = __get_PRIMASK();
    __disable_irq();
    __asm__ volatile ("" ::: "memory");
    pool_node_t *node = (pool_node_t *)blk;
    node->next   = p->free_head;
    p->free_head = node;
    p->free_count++;
    __asm__ volatile ("" ::: "memory");
    __set_PRIMASK(pm);
}

/* FW-PAT-001 (Observer) & FW-PAT-007 (Flash .rodata Command Pattern CLI) */
#define OBS_MAX_SUBS 4U
typedef void (*obs_cb_t)(uint32_t event_id, void *ctx);
typedef struct { obs_cb_t cb; void *ctx; } obs_entry_t;
typedef struct { obs_entry_t subs[OBS_MAX_SUBS]; size_t count; } observer_bus_t;

bool obs_subscribe(observer_bus_t *bus, obs_cb_t cb, void *ctx)
{
    if ((bus == NULL) || (cb == NULL) || (bus->count >= OBS_MAX_SUBS)) { return false; }
    bus->subs[bus->count].cb  = cb;
    bus->subs[bus->count].ctx = ctx;
    bus->count++;
    return true;
}

void obs_notify(const observer_bus_t *bus, uint32_t event_id)
{
    if (bus == NULL) { return; }
    for (size_t i = 0U; i < bus->count; ++i) { bus->subs[i].cb(event_id, bus->subs[i].ctx); }
}

typedef int (*cli_cmd_fn_t)(int argc, const char *const argv[]);
typedef struct { const char *name; cli_cmd_fn_t handler; const char *help; } cli_cmd_t;

static int cmd_ver(int argc, const char *const argv[])  { (void)argc; (void)argv; return 0; }
static int cmd_stat(int argc, const char *const argv[]) { (void)argc; (void)argv; return 1; }

static const cli_cmd_t g_cli_cmds[] = {
    { "ver",  cmd_ver,  "Print firmware version" },
    { "stat", cmd_stat, "Print telemetry status" },
};

int cli_dispatch(int argc, const char *const argv[])
{
    if ((argc <= 0) || (argv == NULL) || (argv[0] == NULL)) { return -1; }
    for (size_t i = 0U; i < (sizeof(g_cli_cmds) / sizeof(g_cli_cmds[0])); ++i) {
        if (strcmp(argv[0], g_cli_cmds[i].name) == 0) {
            return g_cli_cmds[i].handler(argc, argv);
        }
    }
    return -2;
}
```

---

## 9. Practical STM32G474RE Experiments

1. **Static HAL Dispatch vs Runtime Vtable Disassembly Lab (`FW-DRV-003`):**
   - Compile Section 8.3 with `arm-none-eabi-gcc -std=c11 -mcpu=cortex-m4 -mthumb -O2 -c` and run `arm-none-eabi-objdump -d` to verify `bench_static_pulse` emits 5 instructions (direct `STR` to `BSRR`/`BRR`) vs `bench_vtable_pulse` emitting `BLX r3` / `BX r3`.
2. **`volatile`, `LDREX`/`STREX` & Atomic `BSRR` Disassembly Proof (`EMB-VOL-001`, `EMB-ATOM-002`, `EMB-REG-002`):**
   - Pair with [Lab 01 — Bare-Metal GPIO Blinky](../18-practical-laboratory/lab-01-baremetal-gpio-led-blinky/README.md) and [Lab 05 — EXTI Pushbutton Interrupt](../18-practical-laboratory/lab-05-exti-pushbutton-interrupt/README.md): confirm `GPIOA->BSRR` emits a single `STR`, `ldrex_atomic_or` emits `ldrex`/`orr.w`/`strex`/`dmb sy`, and non-`volatile` polling loops hoist out at `-O2`.
3. **Non-Blocking Debouncer & Async Driver Upgrade (`FW-PAT-003`, `FW-DRV-002`):**
   - Refactor [Lab 02 — GPIO Debouncing](../18-practical-laboratory/lab-02-gpio-pushbutton-debouncing/README.md) using the 1 ms shift-register debouncer + cooperative TTC scheduler, eliminating all blocking `delay_ms()` calls.
   - **Debugging Checkpoint:** In GDB with `STM32G474.svd` loaded (`EMB-CMSIS-003`), step inside nested `crit_enter_primask()` / `crit_enter_basepri(5)` calls, verify `info reg primask basepri` preserves outer state, and confirm `p/x &GPIOA->BSRR == 0x48000018`.

---

## 10. Common Mistakes & Anti-Patterns

1. **Using `|=` on `rc_w1` (`EXTI->PR1`, `ADC1->ISR`) or `&=` on `rc_w0` (`TIM2->SR`):** Silently clears unrelated concurrent hardware flags! Always use direct `=` assignment.
2. **Using `GPIOA->ODR ^= (1U << 5)` inside an ISR:** Non-atomic RMW races with main-loop GPIO updates. Use `BSRR`/`BRR`.
3. **Assuming `volatile` makes ISR/DMA data sharing thread-safe:** `volatile` provides zero atomicity and allows non-volatile payload reordering across a volatile flag without `__DMB()` / compiler barriers.
4. **Assuming `LDREX`/`STREX` synchronizes with DMA1/DMA2:** Cortex-M4's local exclusive monitor only tracks CPU exceptions, never DMA AHB transfers!
5. **Unbounded `while (!(REG & FLAG));` polling loops:** A shorted I2C bus freezes the entire MCU permanently. Always bound waits with a wraparound-safe timeout.
6. **Naive `__disable_irq()` / `__enable_irq()` inside utility functions:** Breaks nested critical sections by prematurely re-enabling interrupts. Always save/restore `PRIMASK` or use `__set_BASEPRI_MAX()` + restore.
7. **Checking `if (!flag) __WFI();` with interrupts enabled:** Creates the classic lost-wakeup race condition. Guard `WFI` with `PRIMASK` + `__DSB()`.
8. **Using runtime vtables inside 100 kHz+ control ISRs when static inline dispatch fits:** Indirect `BLX` prevents inlining and adds unnecessary call/stack overhead (`FW-DRV-003`).

---

## 11. Hands-On Exercises

1. **CMSIS Struct & Offset Verification (`EMB-MMIO-002`):** Write a standalone `MY_GPIO_TypeDef` matching RM0440 §9.4 (`MODER`..`BRR`), annotate with `__I`/`__O`/`__IO`, and prove every offset (`0x00`..`0x28`) via `_Static_assert(offsetof(...))`.
2. **Reentrant `PRIMASK` & `BASEPRI` Critical Sections (`EMB-ATOM-001`):** Test nested `crit_enter_basepri(3)` inside `crit_enter_basepri(5)` and verify `__set_BASEPRI_MAX()` keeps priority 3 masked until exit while priority 0–2 ISRs still preempt cleanly.
3. **Static Policy vs Vtable Cycle Benchmark (`FW-DRV-003`):** Measure 10,000 GPIO toggle iterations via `DWT->CYCCNT` using `bench_static_pulse()` vs `bench_vtable_pulse()` at `-O2` and tabulate exact cycle & `.text` byte differences.
4. **Host-Testable Hardware Proxy + HSM Test (`FW-PAT-002`, `FW-FSM-002`):** Compile `temp_proxy_read_mdegc()` and `hsm_dispatch()` on host GCC with a mock `i2c_adapter_t`, verifying `DRV_ERR_HW` propagation and superstate `HSM_EVT_FAULT` transitions from both `hsm_idle` and `hsm_running`.

---

## 12. Assessment Questions

1. **(`EMB-MMIO-001`, `EMB-REG-002`)** Trace the CMSIS macro expansion chain from `PERIPH_BASE` (`0x40000000UL`) to `GPIOA->BSRR` (`0x48000018UL`) in `stm32g474xx.h`, and explain why STM32G474RE's peripheral bit-band alias (`0x42000000`) cannot address `GPIOA`.
2. **(`EMB-REG-003`)** Why is `TIM2->SR &= ~TIM_SR_UIF;` a latent bug on an `rc_w0` register, and why is `ADC1->ISR |= ADC_ISR_ADRDY;` a bug on an `rc_w1` register? Write the exact bug-free statement for each.
3. **(`EMB-VOL-002..003`, `EMB-ATOM-002..003`)** Give two concrete concurrency bugs that **`volatile` alone does NOT fix**; explain how Cortex-M4 `LDREX`/`STREX` stays atomic against ISRs without masking interrupts yet fails against DMA, and explain how a **Seqlock** prevents torn multi-field telemetry reads.
4. **(`EMB-ATOM-001`, `FW-EXEC-002`)** Explain why `__set_BASEPRI_MAX(5U << 4U)` is superior to `PRIMASK` for motor-control firmware on STM32G474RE (`__NVIC_PRIO_BITS = 4U`), and explain why ARM Cortex-M still wakes from `__WFI()` when `PRIMASK = 1` guards the event check.
5. **(`FW-DRV-003`)** Compare **Zero-Overhead Static HAL Dispatch** (X-Macros + `static inline` + C11 `_Generic` + link-time seams) against **Runtime Function-Pointer Vtables** across `-O2` Thumb-2 code size, indirect `BLX` overhead, inlining, runtime polymorphism, and host unit testability.
6. **(`FW-LAYER-002`, `EMB-CMSIS-003`)** Compare Direct CMSIS Register Access vs ST LL vs ST HAL across latency, Flash size, ES0430 errata coverage, and portability; then explain what CMSIS-SVD and CMSIS-DSP (`SMLAD`, `SSAT`, `VFMA.F32`) provide on Cortex-M4F.
7. **(`FW-FSM-002`, `FW-PAT-002`, `FW-PAT-005`)** How does an HSM superstate eliminate state explosion compared to a flat FSM? Contrast a Hardware Proxy/Adapter with direct `stm32g474xx.h` calls in application code, and explain why `fixed_pool_t` guarantees $O(1)$ allocation with zero external fragmentation.
8. **(`FW-ERR-001`, `EMB-STD-002`)** Contrast `FW_ASSERT(ptr != NULL)` vs returning `DRV_ERR_TIMEOUT`, and list the required elements of a formal MISRA Compliance:2020 Deviation Record for Rule 11.4 MMIO pointer casts.

---

## 13. Measurable Completion Criteria

- [ ] **Zero Register Access Semantic Bugs:** Correctly write single-store `MODIFY_REG`, atomic `BSRR`/`BRR`, and direct-assignment `rc_w1`/`rc_w0`/`w` clears with zero RMW hazards.
- [ ] **Optimization-Proof Code:** Firmware compiles cleanly at **both `-O0` and `-O2`** with `-std=c11 -Wall -Wextra -Werror -Wconversion -Wdouble-promotion -Wvla` without polling-loop hangs or reordering bugs.
- [ ] **Concurrency Primitives Verified:** Nested `PRIMASK`/`BASEPRI` critical sections, `__LDREXW`/`__STREXW`/`__CLREX`, 64-bit double-read / Seqlock, and lock-free SPSC ring buffer with `__DMB()` verified via disassembly and host boundary tests.
- [ ] **Static vs Dynamic HAL & Design Patterns Applied:** Quantify static-inline/X-Macro/`_Generic` dispatch vs runtime vtables via `objdump -d`, implement a decoupled Hardware Proxy + Async Driver + HSM + $O(1)$ Fixed-Block Pool, and replace blocking `delay()` loops with a race-free `WFI` + TTC architecture.
- [ ] **Standards Fluency:** Correctly classify MISRA Mandatory/Required/Advisory rules, author a valid Rule 11.4 Deviation Record, and apply SEI CERT C bounds/overflow checks.

---

> **Sequential Navigation (Step 3 of 17):**  
> **Prev:** [Stage 02 — C Language Foundations (`02-c-fundamentals/`)](../02-c-fundamentals/README.md) | **Home:** [`START_HERE.md`](../START_HERE.md) | **Progress:** [`LEARNER_PROGRESS_DASHBOARD.md`](../LEARNERS/LEARNER_PROGRESS_DASHBOARD.md) | **Next:** [Stage 04 — ARM Cortex-M4F Core (`04-arm-cortex-m/`)](../04-arm-cortex-m/README.md)
