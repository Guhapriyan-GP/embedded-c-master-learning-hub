# Stage 03: Embedded C, Concurrency Primitives & Production Firmware Architecture

> **Curriculum Track:** Level 1–3 Bare-Metal Systems & Firmware Architecture (`EMB-*`, `FW-*`)  
> **Target Platform:** STM32G474RET6 (ARM Cortex-M4F, CMSIS-Core V5.x, `stm32g474xx.h`)  
> **Code Verification Policy:** All C snippets below are **Statically Reviewed against `stm32g474xx.h` & `core_cm4.h` (Not Tested on Physical Hardware)** unless explicitly tagged **Compile-Verified (`arm-none-eabi-gcc 14.3.1`)**.

---

## 1. Learning Objectives

Upon completing Stage 03, you will master both **low-level hardware-facing Embedded C (`EMB-*`)** and **scalable bare-metal Firmware Architecture (`FW-*`)**:

| Domain | Concept IDs | Level | Core Mastery Outcomes |
|---|---|---|---|
| MMIO & CMSIS | `EMB-MMIO-001..003`, `EMB-CMSIS-001..003` | L1–L2 | Derive peripheral base addresses via CMSIS macro chains (`PERIPH_BASE -> GPIOA`), build `volatile` struct overlays (`__I`/`__O`/`__IO`), obey bus access widths & RCC clock-enable sync delays, and use CMSIS-Core/Device/SVD/DSP. |
| Register Semantics | `EMB-REG-001..004` | L1 | Eliminate RMW preemption races, classify and handle `rw`, `r`, `w`, `rc_w1`, `rc_w0`, `rc_r`, `rs`, `t` registers without flag loss, preserve reserved bits, use single-store `MODIFY_REG`, and apply atomic `BSRR`/`BRR`. |
| `volatile` & Barriers | `EMB-VOL-001..003` | L1–L2 | Explain **what `volatile` guarantees and what it does NOT guarantee**, combine compiler barriers with ARMv7-M `__DMB()`/`__DSB()`/`__ISB()` for DMA and ISR synchronization. |
| Atomics & Concurrency | `EMB-ATOM-001..004`, `EMB-ISR-001..002`, `EMB-NUM-001` | L2–L3 | Implement reentrant `PRIMASK` and Cortex-M4 `BASEPRI` critical sections, lock-free `LDREX`/`STREX` & C11 `<stdatomic.h>`, tear-free multi-word snapshots, lock-free SPSC ring buffers, and reentrant/deferred ISR architectures. |
| Layered Drivers & Execution | `FW-LAYER-001..003`, `FW-EXEC-001..003`, `FW-DRV-001..002`, `FW-MOD-001..002` | L1–L3 | Compare Direct Register vs CMSIS vs ST LL vs ST HAL objectively, quantify Polling vs Interrupt vs DMA, build race-free `WFI` superloops and wraparound-safe TTC schedulers, design opaque-handle async drivers with bounded timeouts and dependency injection for host testing. |
| FSMs, Patterns, Safety & Standards | `FW-FSM-001..002`, `FW-ERR-001..003`, `FW-VER-001`, `FW-PAT-001..009`, `EMB-STD-001..003` | L2–L3 | Implement Switch/Table FSMs, HSMs & Active Objects, apply 9 core embedded design patterns, enforce defensive assertions & crash telemetry, and apply MISRA C:2012/2023, SEI CERT C, Barr-C, and formal deviation records. |

---

## 2. Prerequisites

- [Stage 01b — Digital Electronics & Hardware Integration](../01b-digital-electronics-and-hw-integration/README.md) (`DIG-ELEC-001..006`, `HWSW-PCB-001`).
- [Stage 02 — C Language Foundations](../02-c-fundamentals/README.md) and [Advanced C Deep-Dive](../02-c-fundamentals/advanced-c-ub-memory-model.md) (`C-BIT-*`, `C-PTR-*`, `C-TYPE-*`, `C-STOR-*`, `C-QUAL-*`, `C-ALIGN-*`, `C-ATOM-*`).

---

## 3. Detailed Technical Theory

### 3.1 Memory-Mapped I/O (MMIO), CMSIS Chains & Bus Synchronization (`EMB-MMIO-001..003`, `EMB-CMSIS-001..002`)

On ARM Cortex-M, peripheral hardware registers live inside the unified 32-bit address space (`0x40000000–0x5FFFFFFF` and `0xA0000000–0xA00FFFFF`). CMSIS-Device (`stm32g474xx.h`) derives typed peripheral pointers from layered base macros:
```c
#define PERIPH_BASE           (0x40000000UL)
#define AHB2PERIPH_BASE       (PERIPH_BASE + 0x08000000UL) /* 0x48000000UL */
#define GPIOA_BASE            (AHB2PERIPH_BASE + 0x0000UL) /* 0x48000000UL */
#define GPIOA                 ((GPIO_TypeDef *)GPIOA_BASE)
```
Every member of `GPIO_TypeDef` is qualified using CMSIS-Core access macros (`core_cm4.h`):
- `__I` / `__IM` → `volatile const`: Read-only hardware register (e.g., `IDR`, `ISR`). Attempting to write fails at compile time!
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
2. **Peripheral Bus Access Widths:** Most STM32G4 peripheral registers require 32-bit aligned word accesses (`STR`/`LDR`). Notable exception: **SPI `SPIx->DR`** uses 8-bit `*(volatile uint8_t *)&SPI1->DR` writes to push 1 byte into the TX FIFO vs 16-bit writes to pack 2 bytes!

---

### 3.2 Complete Register Access Taxonomy & Eliminating RMW Hazards (`EMB-REG-001..004`)

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

**Reserved Bits & Atomic GPIO (`EMB-REG-002`, `EMB-REG-004`):**
- Always preserve reserved bits at their reset value (RM0440 §1.1) using `MODIFY_REG(REG, CLEARMSK, SETMASK)` (defined in `stm32g4xx.h`).
- Never use `GPIOA->ODR ^= (1U << 5)` or `GPIOA->ODR |= (1U << 5)` in concurrent main/ISR code (`LDR` → `ORR` → `STR` loses ISR updates!). Use single-store atomic **`GPIOA->BSRR`** (bits `[15:0]` set, bits `[31:16]` reset; if both `BSy` and `BRy` are written `1` simultaneously, **`BSy` set wins** per RM0440 §9.4.7) or **`GPIOA->BRR`** (offset `0x28`, bits `[15:0]` reset).

---

### 3.3 What `volatile` DOES and DOES NOT Guarantee (`EMB-VOL-001..003`)

This is one of the most misunderstood topics in embedded C:

| Property | Guaranteed by `volatile`? | Engineering Truth on ARM Cortex-M4 (`arm-none-eabi-gcc -O2`) |
|---|---|---|
| Forces fresh load/store on every access (no register caching / loop elimination) | **YES** | Prevents `while (!(USART2->ISR & RXNE));` from turning into an infinite `b .` loop, and ensures CPU re-reads DMA-written `volatile uint16_t adc_buf[]`. |
| Preserves program order **between two `volatile` accesses** | **YES** | Compiler cannot reorder `vol_a = 1; vol_b = 2;` relative to each other. |
| Prevents **non-volatile** accesses from reordering across a `volatile` access | **NO!** | GCC at `-O2` CAN legally hoist or sink `payload_buf[0] = 42;` *past* `volatile ready_flag = 1;`! **Requires `__COMPILER_BARRIER()` / `__DMB()`!** |
| Guarantees **atomicity** of RMW (`v++`) or 64-bit reads (`volatile uint64_t`) | **NO!** | `v++` still compiles to non-atomic `LDR`/`ADD`/`STR`; `uint64_t` still tears across two 32-bit loads! |
| Guarantees **hardware bus visibility** to DMA1/DMA2 or completion of buffered writes | **NO!** | Requires ARMv7-M hardware barriers: **`__DMB()`** (DMA buffer publish/consume), **`__DSB()`** (wait for peripheral flag clear before ISR exit or `__WFI()`), **`__ISB()`** (pipeline flush after `SCB->VTOR`/`CPACR`/`MPU`). |

---

### 3.4 Critical Sections, Lock-Free Atomics & ISR Discipline (`EMB-ATOM-001..004`, `EMB-ISR-001..002`)

1. **Reentrant `PRIMASK` Critical Section (`EMB-ATOM-001`):**
   Never pair bare `__disable_irq()` with bare `__enable_irq()` inside helper functions — if called from a context that *already* had interrupts disabled, the inner `__enable_irq()` prematurely re-enables interrupts! Always **save and restore `PRIMASK`** with compiler barriers:
   ```c
   static inline uint32_t crit_enter(void)
   {
       uint32_t primask = __get_PRIMASK();
       __disable_irq();
       __asm__ volatile ("" ::: "memory");
       return primask;
   }

   static inline void crit_exit(uint32_t primask)
   {
       __asm__ volatile ("" ::: "memory");
       __set_PRIMASK(primask);
   }
   ```
2. **Cortex-M4 Selective `BASEPRI` Critical Section (`EMB-ATOM-001`):**
   On STM32G474RE (`__NVIC_PRIO_BITS = 4U`, priority stored in top bits `[7:4]`), `PRIMASK` blocks *every* maskable IRQ (including 100 kHz digital power fault ISRs!). Instead, `__set_BASEPRI(5U << (8U - __NVIC_PRIO_BITS))` masks only IRQs with numeric priority $\ge 5$, letting ultra-critical priority `0..4` ISRs execute with **zero added latency** (exact mechanism used by FreeRTOS `configMAX_SYSCALL_INTERRUPT_PRIORITY`).
3. **`LDREX`/`STREX` & Lock-Free SPSC Ring Buffers (`EMB-ATOM-002`, `EMB-ATOM-004`):**
   - CMSIS/ST `ATOMIC_SET_BIT` (`stm32g4xx.h`) and C11 `<stdatomic.h>` use Cortex-M4 `__LDREXW`/`__STREXW` for lock-free RMW without masking interrupts (CPU-vs-ISR only; NOT DMA!).
   - A **Single-Producer Single-Consumer (SPSC) Ring Buffer** (e.g., UART RX ISR pushing, `main()` popping) needs **zero interrupt disabling** if: (a) buffer size $N$ is a power of 2, (b) `head` is modified *only* by producer and `tail` *only* by consumer as free-running unsigned indices (`used = (uint32_t)(head - tail)`), and (c) `__DMB()` separates writing data from publishing `head` (release) and reading `head` from reading data (acquire).
4. **ISR Reentrancy & Newlib Hazards (`EMB-ISR-001`, `EMB-ISR-002`):**
   - Never call `printf`, `malloc`, `free`, `strtok`, or non-reentrant libc functions inside an ISR (newlib `_reent` global buffers and `__malloc_lock` will corrupt heap/stdio or deadlock!).
   - Keep Top-Half ISRs strictly **$< 1\text{--}5\ \mu\text{s}$** (verified via `DWT->CYCCNT`): read hardware register, push into SPSC ring buffer / post event flag, clear interrupt flag + `__DSB()`, and defer parsing/formatting to the Bottom-Half (`main()` or RTOS task).

---

### 3.5 Polling vs Interrupt vs DMA & Abstraction Layer Trade-Offs (`FW-EXEC-001`, `FW-LAYER-001..002`)

#### Quantitative Execution Model Comparison (`FW-EXEC-001`)

| Metric | Blocking Polling | Interrupt-Driven (Per Byte/Event) | DMA + Half/Full Complete ISR |
|---|---|---|---|
| **CPU Utilization (1 Mbps SPI)** | ~100% (busy-wait) | ~35–70% (125,000 IRQs/sec = interrupt storm!) | **< 1%** (CPU sleeps in `WFI`, 1 IRQ per buffer!) |
| **Worst-Case Latency** | Unbounded (blocked by other tasks) | ~12 cycles (70 ns @ 170 MHz) | Hardware-deterministic (0 CPU cycles/sample) |
| **Power Consumption** | Highest (CPU active 100%) | Moderate | **Lowest** (CPU in Sleep mode during transfer) |
| **Best Use Case** | One-shot startup (`PLL` lock, `VOSF` wait with timeout!) | Low-rate async events (`EXTI` button, UART CLI, <20 kHz timers) | High-rate streams (ADC multi-channel, SPI displays, 921 kbps UART, DAC/CORDIC/FMAC) |

#### Objective Firmware Abstraction Matrix (`FW-LAYER-002`)

| Dimension | Direct Raw Addresses | CMSIS Register Access (`GPIOA->BSRR`) | ST Low-Layer (LL) (`LL_GPIO_SetOutputPin`) | ST HAL (`HAL_GPIO_WritePin`) |
|---|---|---|---|---|
| **Flash / RAM Overhead** | Minimal | Minimal (0 B overhead) | Very Low (`static inline`) | High (10–40 KB Flash + handle structs) |
| **Execution Latency** | 1–2 cycles | 1–2 cycles | 1–3 cycles | 15–80+ cycles (null/lock/assert checks) |
| **Silicon Errata (ES0430) Handled?** | Manual | Manual | Partial | Broadly patched by ST |
| **STM32 Cross-Family Portability** | None | Low | Medium | High (G4 ↔ H7 ↔ U5) |
| **Real-World Industry Practice** | Learning only | Bootloaders, ultra-tight ISRs | High-speed control loops (`HRTIM`, `ADC`, `DMA`) | Complex stacks (`USB`, `FDCAN`, bring-up) — **Hybrid HAL + LL/CMSIS** is the industry gold standard! |

---

### 3.6 Bare-Metal Architectures, Drivers & Embedded Design Patterns (`FW-LAYER-001..003`, `FW-EXEC-002..003`, `FW-DRV-001..002`, `FW-FSM-001..002`, `FW-PAT-001..009`)

1. **Strict Layered Architecture & Dependency Injection (`FW-LAYER-001`, `FW-LAYER-003`, `FW-MOD-001`):**
   Dependencies point **strictly downward**: `App → Middleware → Device Drivers → BSP / MCU Drivers → CMSIS`. Application and middleware modules must have **zero `#include "stm32g474xx.h"`**! Inject bus interfaces via `const i2c_ops_t *` function-pointer vtable structs (or link-time seams) so device drivers compile and unit-test natively on host GCC/Unity with 100% mocked hardware!
2. **Production Multi-Instance Driver Rules (`FW-DRV-001`, `FW-DRV-002`):**
   - Expose **opaque handles** (`typedef struct uart_handle uart_handle_t;`), `const *_config_t *cfg` init structs, non-blocking async APIs (`drv_write_async(h, buf, len, cb, ctx)`), and **bounded tick timeouts** on *every* hardware polling loop (never `while (!(REG & FLAG));` without a timeout!).
3. **Race-Free `WFI` Superloop & Cooperative Time-Triggered Scheduler (`FW-EXEC-002`, `FW-EXEC-003`):**
   - **Lost-Wakeup `WFI` Bug:** If you check `if (!g_evt) { __WFI(); }`, an ISR firing *between* the check and `__WFI()` sets `g_evt = 1` and returns, and then the CPU goes to sleep forever!
   - **Cortex-M Race-Free `WFI` Pattern:** ARMv7-M wakes from `WFI` on any pending unmasked NVIC IRQ **even when `PRIMASK = 1`**! Therefore:
     ```c
     __disable_irq();
     if (!event_pending()) {
         __DSB();
         __WFI();
     }
     __enable_irq(); /* Pending ISR executes immediately here! */
     ```
   - **Cooperative TTC Scheduler:** Store static tasks `{ period_ms, last_run_ms, fn }` driven by a 1 ms `SysTick`, testing wraparound-safe `(uint32_t)(now - t->last_run_ms) >= t->period_ms`.
4. **Finite State Machines (FSM), HSMs & Event-Driven Active Objects (`FW-FSM-001`, `FW-FSM-002`):**
   - **Switch-Case FSM** for small 3–6 state protocols; **`const` Flash State-Transition Table** (`{ state, event, guard, action, next_state }`) for clean, unit-testable controllers with explicit Entry/Exit actions.
   - **Hierarchical State Machines (HSM):** Nest substates under a parent superstate (e.g., `FAULT` or `CANCEL` handled once in superstate) with Least-Common-Ancestor (LCA) exit/entry execution. Combine an HSM + private static event queue + Run-to-Completion (RTC) loop to build an **Active Object** (eliminating shared-state mutex spaghetti).
5. **Nine Production Embedded Design Patterns (`FW-PAT-001..009`):**
   - **Observer (`FW-PAT-001`):** Static fixed-size subscriber array `{ cb, void *ctx }` decoupling drivers from multiple consumers.
   - **Hardware Proxy / Adapter (`FW-PAT-002`):** Encapsulates registers behind domain calls (`motor_set_duty_pct()`).
   - **Non-Blocking Debouncer (`FW-PAT-003`):** 1 ms sampled shift register (`state = (state << 1) | raw | 0xE000U`, edge at `0xF000U`) or saturating integrator — zero `delay_ms()`!
   - **Ring Buffer (`FW-PAT-004`):** Circular FIFO with explicit drop-newest vs overwrite-oldest overrun policy and high-watermark counter.
   - **Fixed-Block Memory Pool (`FW-PAT-005`):** $O(1)$ intrusive free-list allocator replacing `malloc`/`free` with zero fragmentation.
   - **DMA Ping-Pong Double Buffer (`FW-PAT-006`):** `buf[2][N]` driven by circular DMA `HTIF`/`TCIF` ISRs publishing half-index via `__DMB()` with overrun detection.
   - **Command Pattern (`FW-PAT-007`):** `const cmd_entry_t[]` Flash CLI dispatch table (`name`, `fn`, `help`).
   - **Singleton Ownership (`FW-PAT-008`):** Static hardware registry guarding exclusive peripheral ownership (`USART2`, `I2C1`).
   - **Software Timer Multiplexer (`FW-PAT-009`):** Multiplexes one 1 ms hardware timer into $N$ one-shot/periodic virtual timers.

---

### 3.7 Defensive Engineering, MISRA C:2012/2023, SEI CERT C & Deviations (`FW-ERR-001..003`, `EMB-STD-001..003`)

- **Assertions vs Error Codes (`FW-ERR-001`):** Use `FW_ASSERT(cond)` (Design by Contract) for **internal programming invariants/bugs** (null pointer passed to internal API, unreachable switch case) capturing module ID + line + `LR` into `.noinit` SRAM and halting via `__BKPT(0)` when attached to GDB. Return explicit `fw_status_t` error codes (annotated `__attribute__((warn_unused_result))`) for **recoverable external runtime faults** (I2C NACK, UART framing error, sensor timeout).
- **MISRA C:2012 / C:2023 (`EMB-STD-001`):** Categorizes rules & directives into **Mandatory** (never deviate), **Required** (must comply or file formal deviation), and **Advisory**. Key embedded rules: Essential Type Model (10.1–10.8), Integer-to-pointer casts (11.4), Shift bounds (12.2), Switch `default` (16.4), Recursion ban (17.2), Check return values (17.7), No VLAs (18.8), No `malloc`/`free` (21.3). **MISRA C:2023** integrates C11/C18 features (`_Generic`, `_Static_assert`, atomics/concurrency).
- **Formal MISRA Deviation Governance (`EMB-STD-002`, MISRA Compliance:2020):** Bare-metal MMIO casts (`(GPIO_TypeDef *)0x48000000UL`) inherently violate Required Rule 11.4. Never silence rules blindly — record a **Deviation Permit** containing: Deviation ID, Rule ID, Scope (exact macro/header), Technical Justification, Risk Assessment & Mitigation (verified against DS12288/RM0440 + `_Static_assert`), and Reviewer Sign-Off, paired with a scoped analyzer suppression comment (`/* cppcheck-suppress misra-c2012-11.4 */`).
- **SEI CERT C (`EMB-STD-003`):** Focuses on exploitable security & reliability defects: `INT30-C` (unsigned wrap), `INT32-C` (signed overflow), `ARR30-C` (array bounds), `STR31-C` (string null-termination space), `EXP33-C` (uninitialized reads), `CON40-C`/`CON43-C` (concurrency).

---

## 4. Important Terminology

| Term | Engineering Definition |
|---|---|
| **MMIO (Memory-Mapped I/O)** | Hardware architecture mapping peripheral control/status/data registers onto standard CPU memory addresses accessed via `LDR`/`STR`. |
| **Read-Modify-Write (RMW) Hazard** | Non-atomic 3-step update (`LDR` → modify → `STR`) vulnerable to lost updates if an ISR preempts mid-sequence, or unintended flag clearing on `rc_w1`/`rc_w0` registers. |
| **Compiler Memory Barrier** | `__asm__ volatile ("" ::: "memory")` directive emitting zero CPU instructions while forbidding GCC from reordering memory operations across that boundary. |
| **Top-Half / Bottom-Half** | Split interrupt design: ultra-fast ISR (Top-Half) captures data/clears HW flags and queues work for deferred processing in Thread mode (Bottom-Half). |
| **Opaque Handle** | Incomplete struct pointer (`typedef struct drv_ctx drv_ctx_t;`) hiding internal driver fields inside `.c` so callers cannot couple to private state. |
| **Run-to-Completion (RTC)** | Event-driven execution rule where each event handler finishes completely before the next queued event is dispatched (no blocking waits inside state handlers). |
| **Deviation Record** | Formal audited justification permitting a localized violation of a MISRA Required rule (e.g., Rule 11.4 for MMIO pointers) under MISRA Compliance:2020. |

---

## 5. Register-Level Explanations

Verified directly against `stm32g474xx.h` and `core_cm4.h`:
- **`GPIOA->BSRR` (`0x48000018`, `w`):** Writing `1UL << n` sets pin $n$; writing `1UL << (n + 16U)` resets pin $n$ atomically in a single `STR`. Zero RMW!
- **`EXTI->PR1` (`0x40010414`, `rc_w1`):** Clear pending EXTI13 via `EXTI->PR1 = EXTI_PR1_PIF13;` (NEVER `|=`).
- **`TIM2->SR` (`0x40000010`, `rc_w0`):** Clear Update Interrupt Flag via `TIM2->SR = ~TIM_SR_UIF;` (NEVER `&= ~TIM_SR_UIF`).
- **`ADC1->ISR` (`0x50000000`, `rc_w1`):** Clear `ADRDY` via `ADC1->ISR = ADC_ISR_ADRDY;` (NEVER `|=`).
- **`PRIMASK` & `BASEPRI` (`cmsis_gcc.h`):** `__get_PRIMASK()`, `__set_PRIMASK()`, `__set_BASEPRI(prio << 4U)` control Cortex-M4 exception masking (`__NVIC_PRIO_BITS = 4U` on STM32G474RE).

---

## 6. Relevant Formulas & Calculations

### 6.1 Wraparound-Safe Cooperative Scheduler Condition
$$\left(\text{uint32\_t}\right)\left(t_{\text{now}} - t_{\text{last}}\right) \ge T_{\text{period}}$$

### 6.2 SPSC Power-of-Two Ring Buffer Indexing & Occupancy
With capacity $N = 2^k$ and free-running unsigned `uint32_t head, tail`:
$$\text{count} = \left(\text{uint32\_t}\right)(\text{head} - \text{tail}), \qquad \text{index}(i) = i \mathbin{\&} (N - 1)$$
$$\text{is\_empty} \iff (\text{head} == \text{tail}), \qquad \text{is\_full} \iff \left(\text{uint32\_t}\right)(\text{head} - \text{tail}) == N$$

### 6.3 Cortex-M4 `BASEPRI` Register Encoding on STM32G474RE
$$\text{BASEPRI\_REG} = \text{priority\_level} \ll (8 - \text{\_\_NVIC\_PRIO\_BITS}) = \text{priority\_level} \ll 4$$

---

## 7. Official Documentation References

- **ST RM0440** — *STM32G4 Series Reference Manual*: §1.1 (Register Access Abbreviations `rw`/`r`/`w`/`rc_w1`/`rc_w0`), §6.2.17 (Peripheral Clock-Enable Sync Delay), §9.4.7 (`GPIOx_BSRR`).
- **ARM AN321 (DAI 0321A)** — *ARM Cortex-M Programming Guide to Memory Barrier Instructions* (`DMB`, `DSB`, `ISB`).
- **ARM DDI 0403E.e** — *ARMv7-M Architecture Reference Manual*: §A3.4 (`LDREX`/`STREX`), §B1.4.3 (`PRIMASK`/`BASEPRI`), §B1.5.18 (`WFI` with `PRIMASK`).
- **ST PM0214** — *STM32 Cortex-M4 MCUs Programming Manual*.
- **MISRA C:2023**, **MISRA Compliance:2020**, **SEI CERT C**, **BARR-C:2018**.
- **Elecia White** — *Making Embedded Systems*; **Miro Samek** — *Practical UML Statecharts in C/C++ (PSiCC2)*; **James Grenning** — *TDD for Embedded C*.

---

## 8. Verified Code Examples

> **Verification Status:** **Compile-Verified (`arm-none-eabi-gcc 14.3.1`, `-O2 -Wall -Wextra -Werror -Wconversion -Wdouble-promotion -Wvla`)** against `stm32g474xx.h` & `cmsis_gcc.h` (Not Tested on Physical Hardware).

### 8.1 Lock-Free SPSC Ring Buffer with Hardware Memory Barriers (`EMB-ATOM-004`, `FW-PAT-004`)

```c
#include <stdbool.h>
#include <stdint.h>
#include "cmsis_gcc.h"

#define SPSC_CAPACITY 64U
_Static_assert((SPSC_CAPACITY & (SPSC_CAPACITY - 1U)) == 0U, "Capacity must be power of 2");

typedef struct {
    uint8_t           buf[SPSC_CAPACITY];
    volatile uint32_t head;     /* Written ONLY by producer (e.g., UART RX ISR) */
    volatile uint32_t tail;     /* Written ONLY by consumer (e.g., main loop)   */
    volatile uint32_t overruns; /* Diagnostics counter */
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
    __DMB(); /* Ensure payload write is globally visible BEFORE publishing head */
    rb->head = head + 1U;
    return true;
}

static inline bool spsc_pop(spsc_ring_t *rb, uint8_t *out_byte)
{
    uint32_t tail = rb->tail;
    uint32_t head = rb->head;
    if (head == tail) {
        return false;
    }
    __DMB(); /* Ensure head read completes BEFORE reading payload */
    *out_byte = rb->buf[tail & (SPSC_CAPACITY - 1U)];
    __DMB(); /* Ensure payload read completes BEFORE freeing slot via tail */
    rb->tail = tail + 1U;
    return true;
}
```

### 8.2 Non-Blocking 1 ms Shift-Register Debouncer + Bounded Timeout Helper (`FW-PAT-003`, `FW-DRV-002`)

```c
#include <stdbool.h>
#include <stdint.h>

typedef struct {
    uint16_t history;
} debounce_t;

/* Call every 1 ms from cooperative TTC tick; returns true ONCE per clean rising press */
static inline bool debounce_update_rising(debounce_t *d, bool raw_pressed)
{
    d->history = (uint16_t)((d->history << 1U) | (raw_pressed ? 1U : 0U) | 0xE000U);
    return (d->history == 0xF000U); /* 12 consecutive HIGH samples after LOW */
}

/* Bounded hardware register flag wait: NEVER hangs infinitely on bus fault! */
static inline bool wait_flag_with_timeout(const volatile uint32_t *reg,
                                          uint32_t mask,
                                          uint32_t start_ms,
                                          uint32_t timeout_ms,
                                          uint32_t (*get_ms)(void))
{
    while ((*reg & mask) == 0UL) {
        if ((uint32_t)(get_ms() - start_ms) >= timeout_ms) {
            return false; /* Timeout reached cleanly */
        }
    }
    return true;
}
```

---

## 9. Practical STM32G474RE Experiments

1. **`volatile` & Compiler Barrier Disassembly Proof (`EMB-VOL-001`, `EMB-VOL-002`):**
   - Compile a non-`volatile` vs `volatile` `USART2->ISR` polling loop at `-O0` and `-O2`; inspect `arm-none-eabi-objdump -d` showing GCC hoisting the non-`volatile` read out of the loop into an infinite branch.
2. **Atomic `BSRR` vs `ODR` RMW Lab:**
   - Pair with [Lab 01 — Bare-Metal GPIO Blinky](../18-practical-laboratory/lab-01-baremetal-gpio-led-blinky/README.md) and [Lab 05 — EXTI Pushbutton Interrupt](../18-practical-laboratory/lab-05-exti-pushbutton-interrupt/README.md): confirm `GPIOA->BSRR` emits a single `STR` instruction in `objdump -d` while `GPIOA->ODR ^= ...` emits 3 instructions (`LDR`/`EOR`/`STR`).
3. **Non-Blocking Debouncer Upgrade:**
   - Refactor [Lab 02 — GPIO Debouncing](../18-practical-laboratory/lab-02-gpio-pushbutton-debouncing/README.md) using the 1 ms shift-register debouncer (`FW-PAT-003`) + cooperative TTC scheduler (`FW-EXEC-003`), eliminating all blocking `delay_ms()` calls.
   - **Debugging Checkpoint:** In GDB, step inside a critical section, verify `info reg primask` toggles `0 -> 1 -> 0` while preserving outer mask state, and verify `p/x &GPIOA->BSRR == 0x48000018`.

---

## 10. Common Mistakes & Anti-Patterns

1. **Using `|=` on `rc_w1` (`EXTI->PR1`, `ADC1->ISR`) or `&=` on `rc_w0` (`TIM2->SR`):** Silently clears unrelated concurrent hardware flags! Always use direct `=` assignment.
2. **Using `GPIOA->ODR ^= (1U << 5)` inside an ISR:** Non-atomic RMW races with main-loop GPIO updates. Use `BSRR`/`BRR` or read `ODR` inside a `PRIMASK` critical section and write `BSRR`.
3. **Assuming `volatile` makes ISR/DMA data sharing thread-safe:** `volatile` provides zero atomicity and allows non-volatile payload reordering across a volatile flag without `__DMB()` / compiler barriers.
4. **Forgetting `volatile` on DMA destination buffers (`adc_dma_buffer`):** Allows GCC `-O2` to cache stale RAM values in CPU registers instead of re-reading fresh DMA samples.
5. **Unbounded `while (!(REG & FLAG));` polling loops:** A shorted I2C line or disconnected SPI slave freezes the entire MCU permanently. Always bound waits with a wraparound-safe timeout.
6. **Naive `__disable_irq()` / `__enable_irq()` inside utility functions:** Breaks nested critical sections by prematurely re-enabling interrupts. Always save/restore `PRIMASK` (or `BASEPRI`).
7. **Checking `if (!flag) __WFI();` with interrupts enabled:** Creates the classic lost-wakeup race condition. Always guard `WFI` with `PRIMASK` + `__DSB()`.
8. **Calling `printf` or `malloc` inside an ISR:** Corrupts newlib internal state or deadlocks on locks.

---

## 11. Hands-On Exercises

1. **CMSIS Struct & Offset Verification:** Write a minimal standalone `MY_GPIO_TypeDef` matching RM0440 §9.4 (`MODER`..`BRR`), annotate with `__I`/`__O`/`__IO`, and prove every offset (`0x00`..`0x28`) using `_Static_assert(offsetof(...))`.
2. **Reentrant `PRIMASK` & `BASEPRI` Critical Sections:** Implement both `crit_enter_primask()` and `crit_enter_basepri(uint8_t max_prio)` for STM32G474RE (`__NVIC_PRIO_BITS = 4U`), including nested call tests.
3. **Lock-Free SPSC Ring Buffer Stress Test:** Write a host unit test pushing and popping 100,000 sequential bytes through `spsc_ring_t`, verifying exact ordering, wraparound across `UINT32_MAX` (`head = 0xFFFFFFF8UL`), and overrun counting.
4. **MISRA Deviation Permit:** Write a formal MISRA Compliance:2020 Deviation Record for Rule 11.4 covering `#define GPIOA ((GPIO_TypeDef *)0x48000000UL)`.

---

## 12. Assessment Questions

1. Trace the exact CMSIS macro expansion chain from `PERIPH_BASE` (`0x40000000UL`) to `GPIOA->BSRR` (`0x48000018UL`) in `stm32g474xx.h`.
2. Why is `TIM2->SR &= ~TIM_SR_UIF;` a latent bug on an `rc_w0` register, and why is `ADC1->ISR |= ADC_ISR_ADRDY;` a bug on an `rc_w1` register? Write the exact bug-free statement for each.
3. Give two concrete examples of concurrency bugs that **`volatile` alone does NOT fix**, and explain the exact barrier or synchronization primitive required to fix each.
4. Explain why `if (!g_event) { __WFI(); }` can sleep forever, and explain why ARM Cortex-M still wakes up when `PRIMASK = 1` wraps the check and `__WFI()`.
5. Compare Direct CMSIS Register Access vs ST LL vs ST HAL across code size, latency, ES0430 errata coverage, and cross-family portability. When is a hybrid HAL + LL/CMSIS architecture preferred?
6. Contrast `FW_ASSERT(ptr != NULL)` vs returning `DRV_ERR_TIMEOUT`: when should firmware assert/halt versus return an error status code?

---

## 13. Measurable Completion Criteria

- [ ] **Zero Register Access Semantic Bugs:** Correctly write single-store `MODIFY_REG`, atomic `BSRR`/`BRR`, and direct-assignment `rc_w1`/`rc_w0`/`w` clears with zero RMW hazards.
- [ ] **Optimization-Proof Code:** Firmware compiles cleanly at **both `-O0` and `-O2`** with `-Wall -Wextra -Werror -Wconversion -Wshadow` without polling-loop hangs or reordering bugs.
- [ ] **Concurrency Primitives Verified:** Reentrant `PRIMASK`/`BASEPRI` critical sections and lock-free SPSC ring buffer with `__DMB()` pass boundary/wraparound verification.
- [ ] **Non-Blocking Architecture Applied:** Replace blocking `delay()` loops with a cooperative wraparound-safe TTC scheduler, non-blocking debouncer, and race-free `WFI` sleep loop.
- [ ] **Standards Fluency:** Correctly classify MISRA Mandatory/Required/Advisory rules, author a valid Rule 11.4 Deviation Record, and apply SEI CERT C bounds/overflow checks.
