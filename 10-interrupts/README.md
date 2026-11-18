# Stage 08: Interrupts, Exceptions, NVIC, SysTick & EXTI

> **Navigation:** [← Stage 06B: RCC & Clocks](../07-rcc-and-clocks/README.md) | [Curriculum Map](../README.md) | [Stage 09: Hardware Timers & HRTIM1 →](../09-timers/README.md)  
> **Directory:** `10-interrupts/` *(directory name preserved for link stability; logically **Stage 08** in the dependency sequence — strictly preceding Timer Interrupts in `09-timers/`!)*  
> **Target Platform:** STM32G474RET6 (ARM Cortex-M4F r0p1, `__NVIC_PRIO_BITS = 4`, 102 External IRQs `0..101`) on NUCLEO-G474RE (MB1367)  
> **Code Verification Notice:** All bare-metal C snippets in Section 8 are **Compile-Verified (`arm-none-eabi-gcc 14.3.1`, `-O2 -Wall -Wextra -Werror`)** against `stm32g474xx.h`, `core_cm4.h`, and `cmsis_gcc.h` (Not Tested on Physical Hardware).

---

## 1. Learning Objectives & Concept Scope

| Concept ID | Concept Name | Architectural Scope | Mastery Tier |
|---|---|---|---|
| `RT-IRQ-001` | NVIC 4-Bit Priority Grouping (`AIRCR.PRIGROUP`), Preemption vs Subpriority, Tail-Chaining, Late-Arrival & Interrupt Latency/Jitter | `[ARM Cortex-M4F]` | Level 2 (Production) |
| `RT-IRQ-002` | ISR Re-Entrancy, Flag-Clearing Semantics (`rc_w1` / `rc_w0`), Peripheral Readback + `__DSB()` Barrier (ARM AN321 §4.9 & ES0430 §2.1.3 / Erratum 838869), & Deferred Processing | `[ARM Cortex-M4F]` / `[STM32 Generic]` | Level 2 (Production) |
| `RT-TIME-002` | Critical Sections (`PRIMASK` vs Selective `BASEPRI`), Race Conditions & Atomic Data Sharing | `[ARM Cortex-M4F]` | Level 2 (Production) |
| `PERIPH-SYSTICK-001` | Cortex-M4 24-Bit SysTick Timer (`CTRL`, `LOAD`, `VAL`, `CALIB`), 1 ms Timebase & Rollover-Safe Timing | `[ARM Cortex-M4F]` | Level 1 (Essential) |
| `PERIPH-EXTI-001` | EXTI Lines `0..43` (`IMR1/2`, `RTSR1/2`, `FTSR1/2`, `PR1/2`), `SYSCFG->EXTICR[0..3]` (`SYSCFGEN`), Active-HIGH `PC13` Rising Edge | `[STM32G474RE Exact]` | Level 1 (Essential) |
| `PERIPH-EXTI-002` | Shared EXTI IRQ Vectors (`EXTI9_5_IRQn = 23`, `EXTI15_10_IRQn = 40`) Multi-Pin Dispatch | `[STM32G474RE Exact]` | Level 2 (Production) |
| `PERIPH-EXTI-003` | EXTI Event Mode (`EMR1` + `__WFE()`) vs Interrupt Mode (`IMR1` + `__WFI()`) & Software Trigger (`SWIER1`) | `[STM32G474RE Exact]` | Level 2 (Production) |

---

## 2. Prerequisites & Curriculum Positioning

### Prerequisite Concept IDs
- **Stage 03 (`04-arm-cortex-m/`):** `ARM-EXC-001` (Exception entry/exit, 8-word basic stack frame vs 18-word FPU extended frame, `EXC_RETURN`, MSP vs PSP).
- **Stage 06 (`07-rcc-and-clocks/`):** `STM32-RCC-001`, `STM32-RCC-002` (170 MHz `HCLK`, `SystemCoreClock`, peripheral clock gating).
- **Stage 07 (`08-gpio/`):** `PERIPH-GPIO-001`, `PERIPH-GPIO-003` (Digital input `MODER = 00b`, NUCLEO-G474RE Active-HIGH `PC13`, atomic `BSRR`).

### Pedagogical Sequencing Note (Strictly Precedes Timer Interrupts!)
- **NVIC, SysTick, and EXTI (Labs 04–05)** are mastered **here** as a strict prerequisite for **Timer Interrupts (`Lab 06`)** in [`09-timers/`](../09-timers/README.md) (while polling-only Basic Timer exercises in `09-timers/` may precede interrupts). There are **zero** dependencies from this stage on `09-timers/`.

---

## 3. Core Technical Theory & Engineering Principles

### 3.1 NVIC Architecture & 4-Bit Priority Grouping (`__NVIC_PRIO_BITS = 4`) `[ARM Cortex-M4F]`
STM32G474RE implements **4 priority bits** (`__NVIC_PRIO_BITS = 4U` in `stm32g474xx.h`) located in the **upper 4 bits `[7:4]`** of each 8-bit `NVIC->IP[n]` byte, yielding **16 programmable priority levels (`0` = highest urgency, `15` = lowest urgency)**. Lower numeric value ALWAYS means higher priority!

The **Priority Grouping** field `SCB->AIRCR.PRIGROUP[10:8]` splits those 4 bits into **Preemption Priority** (can interrupt a currently running lower-priority ISR) and **Subpriority** (tie-breaker only when two IRQs with identical preemption priority are pending simultaneously — NEVER preempts an active ISR!):

| `NVIC_SetPriorityGrouping(val)` | `PRIGROUP[2:0]` | Preemption Bits | Subpriority Bits | Recommended Use Case |
|---|---|---|---|---|
| `NVIC_SetPriorityGrouping(3)` *(or `0`–`3`)* | `0b011` (`3`) | **4 bits (`0..15`)** | **0 bits (`0`)** | **Mandatory for FreeRTOS (`ARM_CM4F`) and recommended for all STM32G4 firmware!** |
| `NVIC_SetPriorityGrouping(4)` | `0b100` (`4`) | 3 bits (`0..7`) | 1 bit (`0..1`) | Custom bare-metal tie-breaking |
| `NVIC_SetPriorityGrouping(5)` | `0b101` (`5`) | 2 bits (`0..3`) | 2 bits (`0..3`) | Legacy grouped preemption |
| `NVIC_SetPriorityGrouping(7)` | `0b111` (`7`) | 0 bits | 4 bits (`0..15`) | Cooperative non-nested ISRs only |

> **Important CMSIS Detail:** `NVIC_SetPriority(IRQn, prio)` automatically shifts `prio << (8 - __NVIC_PRIO_BITS)` (`prio << 4`) into hardware bits `[7:4]`. Always call `NVIC_SetPriorityGrouping(3)` (or `0` — both map to 4 preemption / 0 subpriority bits on a 4-bit implementation!) once at boot before enabling IRQs.

### 3.2 Interrupt Masking: `PRIMASK` vs `BASEPRI` vs `FAULTMASK` `[ARM Cortex-M4F]`
| Core Register | CMSIS Access | Behavior | Engineering Rule |
|---|---|---|---|
| **`PRIMASK`** | `__disable_irq()` / `__enable_irq()` (`CPSID I` / `CPSIE I`) | Masks **ALL** configurable interrupts (priorities `0..15`); only Reset, NMI (`-2`), HardFault (`-1`) execute | Use ONLY for ultra-short (< 20 cycles) atomic sequences; save/restore via `__get_PRIMASK()` / `__set_PRIMASK()` for safe nesting! |
| **`BASEPRI`** | `__set_BASEPRI(prio << 4)` | Masks only interrupts with **numeric priority $\ge$ `prio`** (`BASEPRI = 0` disables masking) | **Preferred for real-time & FreeRTOS:** keeps zero-latency motor/power-fault ISRs (e.g., prio `0..4`) running while protecting shared data against prio `5..15` ISRs! |
| **`FAULTMASK`** | `__disable_fault_irq()` | Masks everything including HardFault (only NMI remains); auto-clears on exception return | Rarely used; reserved for fault-handling recovery routines. |

### 3.3 Hardware Latency Optimization: Tail-Chaining & Late-Arrival `[ARM Cortex-M4F]`
- **Standard Interrupt Entry (PM0214 §2.3.7 / ARM DDI 0439):** **12 clock cycles** (~70.6 ns @ 170 MHz, ~75.0 ns @ 160 MHz, zero-wait-state SRAM/CCM stack) to automatically stack `{R0, R1, R2, R3, R12, LR, PC, xPSR}` and fetch the ISR vector.
- **Tail-Chaining:** Whenever any eligible exception is already pending at the NVIC input upon exception return from ISR A, Cortex-M4 skips the 12-cycle unstack + 12-cycle re-stack and branches directly to the pending ISR in **6 cycles** (~35.3 ns @ 170 MHz). *(Note: this includes **re-entering the same ISR** in 6 cycles if a peripheral's IRQ signal has not yet deasserted across the AHB-to-APB bridge when `BX LR` / `POP {PC}` executes!)*
- **Late-Arrival:** If a higher-priority IRQ arrives *during* the stacking phase of a lower-priority IRQ, the NVIC switches the vector fetch to the higher-priority ISR immediately without re-stacking.
- **FPU Lazy Stacking (`FPU->FPCCR.ASPEN | LSPEN`):** When Thread Mode uses FPU instructions, hardware reserves 18 extra words (72 bytes, total 104-byte frame) on the stack without actually pushing `S0–S15` unless the ISR itself executes an FPU instruction — keeping non-FPU ISR entry latency at 12 cycles!

### 3.4 Cortex-M4 24-Bit SysTick Timer (`PERIPH-SYSTICK-001`) `[ARM Cortex-M4F]`
Integrated inside the Cortex-M4 Private Peripheral Bus (`0xE000E010`), SysTick is a 24-bit down-counter reloading from `LOAD` when transitioning `1 -> 0` (setting `CTRL.COUNTFLAG` and pending `SysTick_IRQn = -1`).
- Clocked by `HCLK` (`CLKSOURCE = 1`, max 170 MHz $\implies$ max single period $2^{24} / 170\text{ MHz} = 98.69\text{ ms}$) or `HCLK / 8` (`CLKSOURCE = 0`, max `789.5 ms`).
- **Rollover-Safe Timing:** Always compare unsigned differences `(uint32_t)(g_ms_ticks - start_ms) >= period_ms` — immune to 32-bit rollover (every 49.7 days at 1 kHz)!

### 3.5 EXTI (44 Lines: `0..43`) & `SYSCFG` Multiplexing `[STM32G474RE Exact]`
STM32G474RE's External Interrupt/Event Controller (`EXTI` @ `0x40010400`) handles **44 lines**:
- **Lines `0..15` (GPIO Configurable):** Each line `N` maps to **one** port (`PA[N]..PG[N]`) via 4-bit nibbles inside **`SYSCFG->EXTICR[0..3]`** (`0000=PA, 0001=PB, 0010=PC, 0011=PD, 0101=PF, 0110=PG`). **CRITICAL:** `SYSCFG` requires **`RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN`** (bit 0) before writing `EXTICR`!
  - Cannot map `PA5` and `PB5` to EXTI5 simultaneously (same pin number shares one EXTI line).
- **Lines `16..43` (Internal Peripherals & Stop Wakeup):** Line 16 = PVD, 17 = RTC Alarm, 18 = USB Wakeup, 19 = RTC TAMP/LSECSS, 20 = RTC Wakeup, 21–22 & 29–33 = COMP1–7, plus I2C1–4, USART1–5, LPUART1, LPTIM1, PVM1/3/4, and UCPD1 wakeup lines (`IMR2`/`PR2` handle lines `32..43`).
- **Interrupt (`IMR1/2`) vs Event (`EMR1/2`) Mode:**
  - `IMR1`: Routes edge latch to NVIC, executing an ISR (wakes `__WFI()`).
  - `EMR1`: Sends a hardware pulse directly to the CPU event input — wakes `__WFE()` **without entering an ISR** (0-cycle stacking overhead!).

### 3.6 Peripheral Flag-Clear Synchronization: Dummy Readback + `__DSB()` (ARM AN321 §4.9 & ES0430 §2.1.3 / Erratum 838869) `[ARM Cortex-M4F]`
Two **distinct** hardware mechanisms must be addressed when clearing a peripheral interrupt flag (`EXTI->PR1`, `TIMx->SR`, `DMAx->IFCR`, etc.) inside a fast Cortex-M4 ISR at 160/170 MHz:

1. **AHB-to-APB Bus Bridge Propagation & Spurious Re-Entry (ARM AN321 / DAI0321A §4.9 — *Disabling interrupts at peripherals*):**
   - Writing `EXTI->PR1 = EXTI_PR1_PIF13;` posts a Device-memory store through the Cortex-M4 internal write buffer and across the STM32 **AHB-to-APB2 bridge**.
   - **Important Architectural Nuance:** **`__DSB()` alone** is *not* strictly guaranteed by the ARMv7-M architecture to wait for a downstream APB slave to latch the clear and deassert its IRQ wire back to NVIC — `DSB` only drains the **processor's internal write buffer** onto the bus interface. If the flag clear is the very last instruction of a short ISR (especially at 160/170 MHz or with an APB prescaler > `/1`), exception return (`BX LR` / `POP {PC}`) can execute while the IRQ line is still asserted at the NVIC input, causing an immediate 6-cycle tail-chained **duplicate ISR re-entry**.
   - **Why Dummy Readback Works:** Because Device-memory reads on the same peripheral slave cannot bypass prior writes across the AHB-to-APB bridge, reading the register back (`(void)EXTI->PR1;`) stalls the CPU until the clear write has physically committed at the peripheral slave and the read response returns! Note that per ARM AN321 §4.9, peripherals operating in slower APB prescaler domains (`> /1`) or asynchronous wakeup clock domains can still incur 1–2 cycles of IRQ-wire synchronizer latency back to NVIC beyond the register commit — which is why clearing **early** in the ISR, checking the status flag on entry, and finishing with `(void)REG;` + `__DSB();` is the complete production pattern.
2. **ARM Cortex-M4 r0p1 Erratum 838869 (ST ES0430 §2.1.3 — *Store immediate overlapping exception return operation might vector to incorrect interrupt*):**
   - Distinct from APB double-triggering, on Cortex-M4 r0p0/r0p1 (`__CM4_REV = 0x0001U` on STM32G474RE), if a **store-immediate** instruction (such as writing a peripheral register) stalls on the bus while immediately followed by an exception return (`BX LR` / `POP {PC}`), a pipeline/bus overlap with a late interrupt change can cause the core to **vector to an incorrect interrupt handler**.
   - ARM's official software workaround is placing a **`__DSB();`** (or ensuring a non-store-immediate instruction separates the store from the exception return) before ISR exit.

- **Authoritative 3-Step Production Pattern:**
  1. Clear the peripheral interrupt flag **early** in the ISR using **direct write** (`EXTI->PR1 = EXTI_PR1_PIF13;` — NEVER `|=` on an `rc_w1` register!).
  2. Perform a **dummy readback** of the same peripheral register (`(void)EXTI->PR1;`) so the clear traverses the AHB-to-APB bridge and commits at the peripheral slave before exit (**ARM AN321 §4.9**).
  3. Execute **`__DSB();`** before returning from the ISR as a defensive architectural barrier draining the core write buffer/pipeline and mitigating **ARM Erratum 838869 / ST ES0430 §2.1.3**.

---

## 4. Core Terminology & Acronyms

| Term | Engineering Definition |
|---|---|
| **NVIC** | Nested Vectored Interrupt Controller — Cortex-M hardware prioritizing, nesting, and vectoring up to 102 external STM32G474RE IRQs. |
| **Preemption vs Subpriority** | Preemption allows interrupting an active ISR; subpriority only resolves simultaneous pending ties. |
| **`rc_w1` (Write-1-to-Clear)** | Register bit cleared by writing `1` and unaffected by writing `0` (e.g., `EXTI->PR1/PR2`). Must use `=`, NEVER `|=`. |
| **`PRIMASK` / `BASEPRI`** | Core interrupt-masking registers (all-or-nothing vs priority-threshold selective masking). |
| **Tail-Chaining / Late-Arrival** | Cortex-M4 hardware optimizations eliminating redundant stacking between back-to-back or preempting IRQs. |
| **`__DSB()` + Dummy Readback** | `DSB` drains the Cortex-M4 internal write buffer and guards against ARM Erratum 838869 (ES0430 §2.1.3); a dummy read `(void)REG;` forces completion across downstream AHB-to-APB bus bridges (ARM AN321 §4.9). |
| **Deferred Interrupt Processing** | Minimal ISR clears flag, captures data, and notifies `main()`/RTOS task to execute heavy processing outside interrupt context. |

---

## 5. Hardware Register Architecture & Bitfield Reference

Verified directly against `core_cm4.h` and `stm32g474xx.h`:

### 5.1 Cortex-M4 Core NVIC, SCB & SysTick Registers
| Register | Address | Function |
|---|---|---|
| **`SysTick->CTRL`** | `0xE000E010` | `ENABLE` (b0), `TICKINT` (b1), `CLKSOURCE` (b2: `1=HCLK`), `COUNTFLAG` (b16, cleared on read) |
| **`SysTick->LOAD`** | `0xE000E014` | 24-bit reload value (`0x00000001..0x00FFFFFF`); write `N - 1` for `N` cycles |
| **`SysTick->VAL`** | `0xE000E018` | Current 24-bit counter; writing any value clears `VAL` to `0` and clears `COUNTFLAG` |
| **`NVIC->ISER[0..3]` / `ICER[0..3]`** | `0xE000E100` / `0xE000E180` | Enable / Disable external IRQs `0..101` (`NVIC_EnableIRQ()` / `NVIC_DisableIRQ()`) |
| **`NVIC->ISPR[0..3]` / `ICPR[0..3]`** | `0xE000E200` / `0xE000E280` | Set / Clear Pending IRQ bits |
| **`NVIC->IP[0..101]`** | `0xE000E400` | Byte-accessible 8-bit priority registers (STM32G4 implements upper bits `[7:4]`) |
| **`SCB->AIRCR`** | `0xE000ED0C` | `VECTKEY = 0x05FA` (bits `[31:16]`), `PRIGROUP[10:8]` |

### 5.2 `SYSCFG` (`0x40010000`, APB2) & `EXTI` (`0x40010400`, APB2)
| Register | Offset | Access | Key Bitfields |
|---|---|---|---|
| **`SYSCFG->EXTICR[0..3]`** | `0x08..0x14` | R/W | 4 pins per register (`EXTICR[3]` bits `[7:4]` = `EXTI13`: `0x2` = Port C!). Requires `RCC->APB2ENR.SYSCFGEN`! |
| **`EXTI->IMR1` / `IMR2`** | `0x00` / `0x20` | R/W | Interrupt mask for lines `0..31` / `32..43` (`1` = unmasked) |
| **`EXTI->EMR1` / `EMR2`** | `0x04` / `0x24` | R/W | Event mask (`__WFE()` wake without ISR) for lines `0..31` / `32..43` |
| **`EXTI->RTSR1` / `RTSR2`** | `0x08` / `0x28` | R/W | Rising-edge trigger enable |
| **`EXTI->FTSR1` / `FTSR2`** | `0x0C` / `0x2C` | R/W | Falling-edge trigger enable |
| **`EXTI->SWIER1` / `SWIER2`** | `0x10` / `0x30` | R/W | Software interrupt/event trigger (`1` sets `PR1/2`) |
| **`EXTI->PR1` / `PR2`** | `0x14` / `0x34` | **`rc_w1`** | Pending bit (`1` = triggered); **clear with direct `EXTI->PR1 = mask;`** |

**STM32G474RE GPIO EXTI IRQ Numbers (`stm32g474xx.h`):** `EXTI0_IRQn = 6`, `EXTI1_IRQn = 7`, `EXTI2_IRQn = 8`, `EXTI3_IRQn = 9`, `EXTI4_IRQn = 10`, **`EXTI9_5_IRQn = 23`**, **`EXTI15_10_IRQn = 40`**.

---

## 6. Engineering Mathematics & Worked Formulas

### 6.1 SysTick Reload Equation
$$\text{LOAD} = \frac{f_{\text{HCLK}}}{f_{\text{tick}}} - 1$$
At $f_{\text{HCLK}} = 170\text{ MHz}$ (Profile A) and $f_{\text{tick}} = 1000\text{ Hz}$ ($1.000\text{ ms}$):
$$\text{LOAD} = \frac{170{,}000{,}000}{1{,}000} - 1 = 169{,}999 \le 16{,}777{,}215\ (2^{24}-1) \quad \checkmark$$
*(At $f_{\text{HCLK}} = 160\text{ MHz}$ Profile B: $\text{LOAD} = 159{,}999$.)*

### 6.2 Worst-Case Interrupt Response Latency & Jitter Budget
$$t_{\text{response,max}} = t_{\text{crit}} + t_{\text{higher ISRs}} + \frac{12\text{ cycles} + N_{\text{wait}}}{f_{\text{HCLK}}}$$
At $170\text{ MHz}$ ($5.882\text{ ns/cycle}$), hardware stacking of 12 cycles takes **$70.6\text{ ns}$** (zero contention). Any `PRIMASK` critical section of $200\text{ cycles}$ adds **$1.18\ \mu\text{s}$** of jitter to every interrupt — proving why selective **`BASEPRI`** masking is essential for hard-real-time control loops!

---

## 7. Official Documentation & Standards References

| Document | Exact Sections |
|---|---|
| **ST PM0214** (*STM32 Cortex-M4 Programming Manual*) | §2.1.3 (`PRIMASK`/`BASEPRI`), §2.3 (Exception Model & §2.3.7 Tail-Chaining/Late-Arrival), §4.3 (`NVIC`), §4.4 (`SCB->AIRCR`), §4.5 (`SysTick`) |
| **ST RM0440** (*STM32G4 Reference Manual*) | Ch. 10 (*SYSCFG*), Ch. 15 (*EXTI* — lines `0..43`) |
| **ST UM2505** (*NUCLEO-G474RE User Manual*) | §6.6 (`B1` on `PC13` is Active-HIGH with external pull-down) |
| **ARM AN321 (DAI0321A)** | §4.2 (*Device Access*), §4.6 (*Disabling Interrupts via NVIC*), §4.8 (*`CPS`/`MSR` Self-Synchronization*), **§4.9 (*Disabling/Clearing Interrupts at Peripherals — Readback Across Bus Bridges*)** |
| **ST ES0430 / ARM Cortex-M4 TRM (DDI 0439)** | **ES0430 §2.1.3** (*ARM Erratum 838869: Store immediate overlapping exception return might vector to incorrect interrupt — `__DSB()` workaround*) |

---

## 8. Production-Grade Bare-Metal C Code Examples

> **Verification Status:** **Compile-Verified (`arm-none-eabi-gcc 14.3.1`, `-O2 -Wall -Wextra -Werror`)** against `stm32g474xx.h`, `core_cm4.h`, and `cmsis_gcc.h` (Not Tested on Physical Hardware).

### 8.1 Rollover-Safe SysTick 1 ms Timebase & `BASEPRI` Critical Section
```c
#include "stm32g474xx.h"
#include <stdint.h>

static volatile uint32_t g_ms_ticks = 0U;

void SysTick_Init_1ms(uint32_t hclk_hz)
{
    SysTick->CTRL = 0U;
    SysTick->LOAD = (hclk_hz / 1000U) - 1U;
    SysTick->VAL  = 0U;
    NVIC_SetPriority(SysTick_IRQn, 15U); /* Lowest priority */
    SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk
                  | SysTick_CTRL_TICKINT_Msk
                  | SysTick_CTRL_ENABLE_Msk;
}

void SysTick_Handler(void)
{
    g_ms_ticks++;
}

uint32_t millis(void)
{
    return g_ms_ticks; /* Atomic 32-bit aligned read on Cortex-M4 */
}

/* Selective BASEPRI critical section: masks IRQs with priority >= threshold_prio (e.g. 5..15),
 * leaving ultra-urgent priorities 0..4 completely unmasked! */
static inline uint32_t enter_basepri_critical(uint32_t threshold_prio)
{
    uint32_t prev = __get_BASEPRI();
    __set_BASEPRI(threshold_prio << (8U - __NVIC_PRIO_BITS));
    __DSB();
    __ISB();
    return prev;
}

static inline void exit_basepri_critical(uint32_t prev_basepri)
{
    __set_BASEPRI(prev_basepri);
}
```

### 8.2 Shared `EXTI15_10` Multi-Pin Dispatch (`PC13` Active-HIGH Rising Edge + Readback & `__DSB()`)
```c
static volatile uint32_t g_b1_press_count = 0U;
static volatile uint32_t g_pb10_edge_count = 0U;

void EXTI_PC13_Init(void)
{
    /* 1. Establish 4 preemption bits, 0 subpriority bits once at system boot */
    NVIC_SetPriorityGrouping(3U);

    /* 2. Enable GPIOC and MANDATORY SYSCFG clocks + sync readback */
    RCC->AHB2ENR  |= RCC_AHB2ENR_GPIOCEN;
    RCC->APB2ENR  |= RCC_APB2ENR_SYSCFGEN;
    (void)RCC->APB2ENR;

    /* 3. PC13 = Digital Input (clear Analog reset 11b -> 00b), No Pull (external PD on MB1367) */
    GPIOC->PUPDR &= ~GPIO_PUPDR_PUPD13_Msk;
    GPIOC->MODER &= ~GPIO_MODER_MODE13_Msk;

    /* 4. Route PC13 to EXTI13: SYSCFG->EXTICR[3] bits [7:4] = 0b0010 (Port C) */
    SYSCFG->EXTICR[3] = (SYSCFG->EXTICR[3] & ~SYSCFG_EXTICR4_EXTI13_Msk)
                      | SYSCFG_EXTICR4_EXTI13_PC;

    /* 5. NUCLEO-G474RE B1 is Active-HIGH -> Rising Edge trigger on press! */
    EXTI->RTSR1 |= EXTI_RTSR1_RT13;
    EXTI->FTSR1 &= ~EXTI_FTSR1_FT13;

    /* 6. Clear stale pending flag (direct write '='), then unmask EXTI13 */
    EXTI->PR1  = EXTI_PR1_PIF13;
    EXTI->IMR1 |= EXTI_IMR1_IM13;

    /* 7. Configure NVIC priority and enable shared EXTI15_10_IRQn (IRQ 40) */
    NVIC_SetPriority(EXTI15_10_IRQn, 6U);
    NVIC_ClearPendingIRQ(EXTI15_10_IRQn);
    NVIC_EnableIRQ(EXTI15_10_IRQn);
}

void EXTI15_10_IRQHandler(void)
{
    /* Mask against IMR1 so we only service enabled lines */
    uint32_t pending = EXTI->PR1 & EXTI->IMR1;

    if ((pending & EXTI_PR1_PIF13) != 0U) {
        EXTI->PR1 = EXTI_PR1_PIF13; /* Direct '=' clear (NEVER '|='!) */
        g_b1_press_count++;
        GPIOA->BSRR = ((GPIOA->ODR & GPIO_ODR_OD5) != 0U) ? GPIO_BSRR_BR5 : GPIO_BSRR_BS5;
    }

    if ((pending & EXTI_PR1_PIF10) != 0U) {
        EXTI->PR1 = EXTI_PR1_PIF10;
        g_pb10_edge_count++;
    }

    /* 1. Dummy readback guarantees APB2 write commits at EXTI before exit (ARM AN321 §4.9).
     * 2. __DSB() drains write buffer & mitigates ARM Erratum 838869 (ST ES0430 §2.1.3). */
    (void)EXTI->PR1;
    __DSB();
}
```

---

## 9. Practical Laboratory Experiments & Debugging Checkpoints

- **Lab 04:** [`lab-04-systick-accurate-delay-generator`](../18-practical-laboratory/lab-04-systick-accurate-delay-generator/README.md) — 170 MHz SysTick 1 ms timebase & non-blocking `millis()`.
- **Lab 05:** [`lab-05-exti-pushbutton-interrupt`](../18-practical-laboratory/lab-05-exti-pushbutton-interrupt/README.md) — `SYSCFGEN`, Active-HIGH `PC13` rising-edge EXTI13, `PRIGROUP`, and `__DSB()`.
- **GDB Verification Checkpoint:**
  ```text
  (gdb) x/1wx 0x40010014   # SYSCFG->EXTICR[3]: verify bits [7:4] == 0x2 (Port C)
  (gdb) x/6wx 0x40010400   # EXTI IMR1, EMR1, RTSR1, FTSR1, SWIER1, PR1
  (gdb) set *(unsigned int*)0x40010410 = (1<<13)  # Trigger EXTI13 via SWIER1 in GDB!
  (gdb) x/1wx 0xE000ED0C   # Verify SCB->AIRCR PRIGROUP[10:8] == 3
  ```

---

## 10. Common Engineering Mistakes, Hardware Traps & Silicon Errata (ES0430)

1. **Forgetting `RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN`:** Writes to `SYSCFG->EXTICR[3]` are silently ignored, leaving `EXTI13` routed to `PA13` (SWDIO!) instead of `PC13`.
2. **Using `EXTI->PR1 |= EXTI_PR1_PIF13;` (`rc_w1` Bug):** Reading `PR1` and ORing writes `1` back to EVERY currently pending EXTI line, silently wiping unrelated interrupts (`EXTI10`..`EXTI15`)! Always use `EXTI->PR1 = EXTI_PR1_PIF13;`.
3. **Omitting Peripheral Readback and `__DSB()` Before ISR Return (ARM AN321 §4.9 & ES0430 §2.1.3 / ARM 838869):** Omitting `(void)EXTI->PR1;` after a late APB2 flag clear can cause 6-cycle tail-chained phantom re-entry on fast 160/170 MHz ISRs; omitting `__DSB()` leaves Cortex-M4 r0p1 exposed to Erratum 838869 (store-immediate overlapping exception return vectoring to an incorrect interrupt).
4. **Using Falling Edge (`FTSR1`) on NUCLEO-G474RE `B1` (`PC13`):** B1 is Active-HIGH (`UM2505`); falling edge triggers on button *release*, not press.
5. **Forgetting `SystemClock_Config_170MHz()` Before `SysTick->LOAD = 169999`:** Leaves SysTick running 10.625× too slow on 16 MHz HSI16.
6. **Executing `printf()` or Blocking Loops Inside an ISR:** Causes massive latency, non-reentrant newlib heap corruption, and priority starvation.

---

## 11. Progressive Coding Exercises & Design Challenges

1. **[Level 1]** Write a rollover-safe `delay_us()` function combining `g_ms_ticks` and live `SysTick->VAL` down-counter reads, handling the wrap boundary atomically.
2. **[Level 2]** Configure two shared lines (`PC13` EXTI13 and `PB10` EXTI10) inside `EXTI15_10_IRQHandler` and trigger both simultaneously via `EXTI->SWIER1 = (1U<<13) | (1U<<10);` verifying zero lost edges.
3. **[Level 2]** Reproduce a torn 64-bit read (`volatile uint64_t`) shared between `main()` and a high-rate ISR, then eliminate 100% of corruptions using `BASEPRI` selective masking while proving a Priority-1 ISR remains unmasked.
4. **[Level 3]** Configure `EXTI13` in **Event Mode (`EMR1`, `IMR1 = 0`)** to wake the Cortex-M4 from `__WFE()` with zero ISR invocation and measure wake latency via oscilloscope GPIO toggle.

---

## 12. Self-Assessment Mastery Questions

1. **Q:** Why does `EXTI->PR1 |= (1U << 13)` introduce a subtle concurrency bug on shared IRQ lines?  
   *Answer Check:* `PR1` is `rc_w1`; the read-modify-write writes `1` back to every *other* bit that happened to be set (e.g., bit 10), accidentally acknowledging and dropping that interrupt.
2. **Q:** With `PRIGROUP = 3` (4 preemption bits, 0 subpriority bits), can an IRQ at priority `2` preempt an active ISR at priority `5`? What if both are priority `5`?  
   *Answer Check:* Priority `2` preempts priority `5` (lower number = higher urgency). Two IRQs at priority `5` never preempt each other; the lower hardware `IRQn` executes first.
3. **Q:** Why must `BASEPRI` be written with `prio << 4` on STM32G474RE?  
   *Answer Check:* `__NVIC_PRIO_BITS = 4` occupies the most significant nibble `[7:4]` of the 8-bit priority byte; writing unshifted `5` places it in unimplemented bits `[3:0]` (effective `0` = disables masking!).
4. **Q:** Why is `EXTI` able to wake STM32G4 from Stop 0/1 mode when APB2 clocks are stopped?  
   *Answer Check:* Configurable EXTI edge-detection latches are asynchronous combinational/edge circuits powered in the main domain that operate without a running APB2 clock.

---

## 13. Measurable Mastery & Completion Criteria

| Concept ID | Pass / Fail Verification Criterion | Measurement Tool |
|---|---|---|
| `PERIPH-SYSTICK-001` | Verify $1.000\text{ ms} \pm 0.01\%$ SysTick period at 170 MHz and rollover-safe timeout across `0xFFFFFFFF -> 0x00000000` | Oscilloscope + Unit Test |
| `PERIPH-EXTI-001` / `002` | Trigger `EXTI15_10_IRQHandler` on `PC13` rising edge and simultaneous `SWIER1` multi-pin trigger with `0` lost or double-counted events | GDB + Logic Analyzer |
| `RT-IRQ-001` / `002` | Verify `PRIGROUP = 3`, nested preemption trace, `< 150 ns` ISR entry latency at 170 MHz, and `0` double triggers using `(void)EXTI->PR1; __DSB();` | Oscilloscope + GDB |
| `RT-TIME-002` | Prove `0` torn 64-bit reads over 10M iterations using `BASEPRI` while Priority-1 ISR experiences `0` added jitter | Logic Analyzer + `DWT->CYCCNT` |

---

> **Navigation:** [← Stage 06B: RCC & Clocks](../07-rcc-and-clocks/README.md) | [Curriculum Map](../README.md) | [Stage 09: Hardware Timers & HRTIM1 →](../09-timers/README.md)
