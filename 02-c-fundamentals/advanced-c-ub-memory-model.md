# Stage 02 Companion Deep-Dive: Alignment, Undefined Behavior, C11 Memory Model, Fixed/Floating-Point Math & Newlib Reentrancy

> **Sequential Navigation (Step 2B of 17 (Companion Deep-Dive) — Canonical Acyclic Order):**  
> **Home:** [`START_HERE.md`](../START_HERE.md) | **Roadmap:** [`learning-sequence.md`](../01-master-concept-map/learning-sequence.md) | **Prev:** [Stage 02 — C Language Foundations (`02-c-fundamentals/README.md`)](./README.md) | **Next:** [Stage 03 — Embedded C & MMIO (`03-embedded-c/`)](../03-embedded-c/README.md)  
> **Prerequisites:** [Stage 02 (`C-INT-*`, `C-PTR-*`, `C-TYPE-*`, `C-QUAL-*`)](./README.md)

> **Parent Module:** [Stage 02 — C Language Foundations](./README.md)  
> **Concept IDs Covered:** `C-ALIGN-001..003`, `C-UB-001..003`, `C-SEQ-001..002`, `C-ATOM-001..004`, `C-MATH-001..003`, `C-PTR-004`, `C-MEM-003`, `C-OPT-001..003`, `C-LIB-001` (Levels 2–3)  
> **Target ABI:** ARM Cortex-M4F (`arm-none-eabi-gcc 14.3.1 -std=c11 -mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard -DSTM32G474xx -O2 -Wall -Wextra -Werror`)  
> **Code Verification Policy:** All C11 implementation blocks below are **Compile-Verified (`arm-none-eabi-gcc 14.3.1`, `-O2 -Wall -Wextra -Werror -Wdouble-promotion -Wvla`, Not Tested on Physical Hardware)**. Deliberately broken anti-pattern snippets are labeled `/* BUG */` for educational contrast.

---

## 0. Prerequisites

| Prerequisite Concept ID | Required Mastery Before This Guide |
|---|---|
| `C-INT-001..004`, `C-PTR-001..003`, `C-TYPE-001` | Fixed-width integers, integer promotions, pointer arithmetic, struct layout ([Stage 02 README](./README.md)). |
| `C-STOR-001..003`, `C-QUAL-001..003` | Static/automatic storage duration, `const`, `volatile`, `static inline`. |
| `TOOL-GCC-003`, `RTOS-MEM-001` *(for Section 6 `C-LIB-001`)* | Newlib / Newlib-Nano `--specs=nano.specs` linking ([Stage 05](../05-toolchain-and-debugging/README.md)) and FreeRTOS task heap/TCB concepts ([Stage 16](../16-rtos/README.md)). |

---

## 1. Alignment, Padding, `__attribute__((packed))` & Cortex-M4 Unaligned Faults (`C-ALIGN-001..003`)

*Prerequisites: `C-TYPE-001`, `C-PTR-001`.*

### 1.1 Natural Alignment, Padding Rules & `_Alignof` / `_Alignas` (`C-ALIGN-001`)

Under ARM AAPCS (IHI 0042 §5.1, §7.1.8), every scalar type has a natural alignment equal to its size:
- `uint8_t`: 1-byte aligned (`addr % 1 == 0`)
- `uint16_t`: 2-byte aligned (`addr % 2 == 0`)
- `uint32_t`, `float`, pointers: 4-byte aligned (`addr % 4 == 0`)
- `uint64_t`, `double`: **8-byte aligned (`addr % 8 == 0`)**

Each struct's alignment equals the strictest member alignment (`max_align`), and the compiler appends **trailing padding** so consecutive elements in an array `S arr[N]` all satisfy `max_align`. Ordering members largest-to-smallest alignment eliminates wasted padding without sacrificing hardware alignment safety.

Use C11 `_Alignof(T)` to query alignment, `offsetof(S, m)` (`<stddef.h>`) to verify member byte offsets, and **`_Alignas(N)`** (`<stdalign.h>`) to enforce hardware alignment constraints (e.g., STM32G474RE relocated SRAM vector tables require **512-byte alignment** for `SCB->VTOR`, because 118 vectors × 4 B = 472 B rounds up to the next power of 2 = 512 B):

```c
#include <stdalign.h>
#include <stdint.h>

/* Relocated SRAM vector table for STM32G474RE (118 vectors = 472 B -> 512 B aligned!) */
_Alignas(512) __attribute__((used)) static uintptr_t sram_vector_table[118];
```

### 1.2 The `__attribute__((packed))` Hazard & Cortex-M4 `UNALIGN_TRP` (`C-ALIGN-002`)

Overlaying `__attribute__((packed))` structs directly onto raw UART/SPI/CAN byte buffers is a pervasive embedded anti-pattern. Understand the **exact ARMv7-M hardware reality** (ARM DDI 0403E.e §A3.2.1; `core_cm4.h` `SCB->CCR` at `0xE000ED14`, bit 3 `SCB_CCR_UNALIGN_TRP_Msk`):

| Cortex-M4 Instruction Class | `SCB->CCR.UNALIGN_TRP == 0` (Reset Default) | `SCB->CCR.UNALIGN_TRP == 1` |
|---|---|---|
| Single `LDR`, `STR`, `LDRH`, `STRH`, `LDRSH` | Hardware splits into multiple bus cycles (slower, **non-atomic!**) | **Immediate `UsageFault` (`CFSR.UNALIGNED = 1`)** |
| Multi-word `LDRD`, `STRD`, `LDM`, `STM`, `PUSH`, `POP` | **ALWAYS `UsageFault` (`CFSR.UNALIGNED = 1`)!** | **ALWAYS `UsageFault`!** |
| Exclusive `LDREX`, `STREX` & FPU `VLDR`/`VSTR` | **ALWAYS `UsageFault`!** | **ALWAYS `UsageFault`!** |

**Why Taking the Address of a Packed Member Crashes:**
Given `struct __attribute__((packed)) Frame { uint8_t hdr; uint32_t val; };`, member `val` sits at byte offset `1` (misaligned!). Passing `&f.val` to `void process(uint32_t *p)` strips the `packed` attribute from the pointer type (flagged by GCC `-Waddress-of-packed-member`). Inside `process()`, GCC legally assumes 4-byte alignment and may emit `LDRD`/`STRD`/`LDM` or FPU `VLDR` — triggering an immediate **UsageFault / HardFault**, even when `UNALIGN_TRP == 0`!

**Compile-Verified Production-Safe Serialization & `__REV` (`C-ALIGN-002`, `C-ALIGN-003`):**
STM32G474RE (ARM Cortex-M4) is **Little-Endian**: `uint32_t x = 0x12345678UL` stores `[0x78, 0x56, 0x34, 0x12]` from lowest to highest address, whereas network/Modbus/many sensor protocols transmit **Big-Endian** (MSB first). Use explicit byte-shift deserialization or fixed-size `memcpy` plus single-cycle CMSIS `__REV(x)` / `__REV16(x)`:

```c
#include <stdint.h>
#include <string.h>
#include "stm32g474xx.h"

/* 100% alignment-safe, endianness-explicit Big-Endian wire deserialization */
static inline uint32_t read_be32_safe(const uint8_t *p)
{
    return ((uint32_t)p[0] << 24U) |
           ((uint32_t)p[1] << 16U) |
           ((uint32_t)p[2] <<  8U) |
           ((uint32_t)p[3]);
}

/* 100% alignment-safe Little-Endian deserialization (zero UB, zero UNALIGN_TRP fault) */
static inline uint32_t read_le32_safe(const uint8_t *p)
{
    uint32_t v = 0UL;
    memcpy(&v, p, sizeof(v));
    return v;
}

/* Single-cycle Cortex-M4 hardware byte swap (emits: rev r0, r0) */
static inline uint32_t swap_be32_hw(uint32_t le_word)
{
    return __REV(le_word);
}
```

---

## 2. Strict Aliasing, Safe Type Punning, Provenance & UB Taxonomy (`C-UB-001..003`, `C-PTR-004`, `C-MEM-003`)

*Prerequisites: `C-PTR-001`, `C-INT-003`, `C-STOR-001`.*

### 2.1 Strict Aliasing Rule & Effective Types (`C-UB-001`)

At `-O2`, `-O3`, and `-Os`, GCC enables **`-fstrict-aliasing`** by default. Under ISO C11 §6.5p7, an object's stored value may only be accessed by an lvalue whose type is compatible with the object's **effective type** (or a `const`/`volatile`-qualified or signed/unsigned variant thereof), **or a character type (`char`, `unsigned char`, `signed char`)**.

Consequently, GCC assumes two pointers of **incompatible types** (e.g., `uint32_t *` and `uint16_t *`, or `uint32_t *` and `float *`) **never alias the same address**:
```c
/* BUG: Violates ISO C11 §6.5p7 strict aliasing! */
uint32_t alias_miscompile(uint32_t *u32, uint16_t *u16)
{
    *u32 = 0x11223344UL;
    *u16 = 0xAABB;   /* GCC -O2 assumes u16 CANNOT point inside *u32! */
    return *u32;     /* GCC legally caches and returns constant 0x11223344UL! */
}

/* BUG: Violates strict aliasing AND may violate alignment! */
uint32_t float_bits_broken(float f)
{
    return *(uint32_t *)&f; /* UB under -O2 -fstrict-aliasing */
}
```
The **only** universally legal byte-inspection pointer is `unsigned char *` / `char *` (and `uint8_t *` where `typedef unsigned char uint8_t`).

### 2.2 Safe Type Punning with `memcpy` (`C-UB-002`)

The **100% ISO C11 (and C++20) conformant, alignment-safe, strict-aliasing-safe** way to reinterpret bit patterns is fixed-size `memcpy` (Compile-Verified with `arm-none-eabi-gcc 14.3.1 -O2 -mfpu=fpv4-sp-d16 -mfloat-abi=hard`):
```c
#include <stdint.h>
#include <string.h>

uint32_t float_to_u32_bits(float f)
{
    uint32_t u = 0UL;
    _Static_assert(sizeof(u) == sizeof(f), "IEEE-754 binary32 required");
    memcpy(&u, &f, sizeof(u));
    return u;
}
```
**Verified `arm-none-eabi-gcc 14.3.1 -O2` Disassembly (`objdump -d`):** GCC recognizes the 4-byte `memcpy` and completely eliminates any `libc` function call, emitting inlined instructions (`sub sp, #8; vstr s0, [sp, #4]; ldr r0, [sp, #4]; add sp, #8; bx lr` — or direct `vmov r0, s0` depending on GCC register-move heuristics) with **zero branch to `memcpy`**.
*(Note: C99/C11 Footnote 95 allows type punning via a `union` in C, but it is UB in C++ and violates MISRA C Rule 19.2; `memcpy` is portable across both C and C++.)*

### 2.3 Pointer Provenance, Dangling Pointers & UB-Driven Check Deletion (`C-MEM-003`, `C-UB-003`)

In ISO C11, a pointer is not merely a raw integer address — it carries **object provenance** tied to the lifetime of the exact object (array, struct, static variable, or heap block) from which it was created (§6.2.4p2, §6.5.6p8):
- Once a stack frame exits, every pointer to its automatic locals becomes **indeterminate/dangling**; reading it is UB, and on Cortex-M4 the next call or hardware ISR frame push (`R0–R3, R12, LR, PC, xPSR`) overwrites that address.
- **UB Time-Travel / Dead-Check Deletion:** Because dereferencing `NULL` is UB, GCC `-O2` assumes any pointer already dereferenced *cannot* be `NULL`:
  ```c
  /* BUG: GCC -O2 sees *p dereferenced first -> proves p != NULL -> DELETES the if-check! */
  int read_cfg_bug(const int *p)
  {
      int v = *p;           /* Dereference BEFORE null check! */
      if (p == NULL) {      /* Optimized away completely at -O2! */
          return -1;
      }
      return v;
  }
  ```
  Always validate preconditions (`if (p == NULL) { return -1; }`) **before** any dereference.

### 2.4 The `restrict` Non-Aliasing Contract (`C-PTR-004`)

When two pointers have the *same* type (`const float *src`, `float *dst`), GCC must assume `dst[i]` might overwrite `src[i+1]`, forcing conservative memory ordering. Adding C99/C11 **`restrict`** promises the regions do not overlap, allowing tight `VLDMIA`/`VMUL.F32`/`VSTMIA` loops (Compile-Verified `arm-none-eabi-gcc 14.3.1 -O2` emits a 6-instruction inner loop with zero redundant reloads):
```c
#include <stddef.h>

void vec_scale_f32(const float * restrict src, float * restrict dst, float gain, size_t n)
{
    for (size_t i = 0U; i < n; ++i) {
        dst[i] = src[i] * gain;
    }
}
```
*(Passing overlapping buffers to `restrict`-qualified parameters is Undefined Behavior!)*

### 2.5 Behavioral Taxonomy: UB vs Unspecified vs Implementation-Defined (`C-UB-003`)

| Category | ISO C11 Definition | Compiler Obligation | 10 Concrete Embedded Examples (ISO C11 Annex J) |
|---|---|---|---|
| **Undefined Behavior (UB)** | Erroneous construct; standard imposes **no requirements** (§3.4.3, Annex J.2) | Compiler assumes UB **never happens** and may delete checks or emit arbitrary code | (1) Signed overflow `INT32_MAX + 1`, (2) `1 << 31`, (3) `x << 32`, (4) Strict-aliasing cast `*(uint32_t *)&f`, (5) Reading uninitialized stack local, (6) Dereferencing `NULL` or `&arr[N]`, (7) Returning/using dangling `&local`, (8) Writing to string literal in `.rodata`, (9) Unsequenced `i = i++ + 1`, (10) Passing overlapping buffers to `restrict` pointers. |
| **Unspecified Behavior** | Standard allows 2+ valid choices without requiring documentation (§3.4.4, Annex J.1) | Choice may silently flip between `-O0` and `-O2` or compiler versions | (1) **Function argument evaluation order:** `combine(spi_rx(), spi_rx())`, (2) Subexpression evaluation order `f() + g()`, (3) Value of struct padding bytes after member writes. |
| **Implementation-Defined** | Standard allows 2+ choices, but toolchain **must document** its choice (§3.4.1, Annex J.3) | Stable for a specific target ABI (`arm-none-eabi-gcc` AAPCS) | (1) Plain `char` signedness (`unsigned` on ARM AAPCS), (2) `sizeof(int)` = 4, (3) Right-shift of negative `int` (`ASR` arithmetic shift on ARM), (4) C bit-field bit order & alignment, (5) Underlying integer size of `enum` (4 B default vs `-fshort-enums`). |

---

## 3. Evaluation Order, C11 `sequenced-before` & Short-Circuiting (`C-SEQ-001..002`)

*Prerequisites: `C-UB-003`, `C-QUAL-002`.*

- **Unsequenced Side-Effect UB (`C-SEQ-001`):** Modifying a scalar object twice (or both reading and modifying it without a value-computation dependency) without an intervening `sequenced-before` edge (ISO C11 §6.5p2) is **UB**: `i = i++ + 1;`, `buf[i] = i++;`. Always enable **`-Wsequence-point -Werror`**.
- **Unspecified Function Argument Evaluation Hazard:** Comma separators between function arguments are **NOT** the comma operator — arguments may evaluate right-to-left or left-to-right! Never read a hardware FIFO twice inside one call:
  ```c
  /* BUG: Unspecified which SPI1->DR read happens first (MSB/LSB swap across -O0 vs -O2!) */
  uint16_t bad = make_u16(spi_read_byte(), spi_read_byte());

  /* CORRECT: Explicit semicolon sequencing guarantees hi reads before lo */
  uint8_t hi = spi_read_byte();
  uint8_t lo = spi_read_byte();
  uint16_t good = ((uint16_t)hi << 8U) | lo;
  ```
- **Short-Circuit Guarantees, Comma Operator & Clear-On-Read Traps (`C-SEQ-002`):**
  - The operators `&&`, `||`, `?:`, and the **comma operator** `(expr1, expr2)` guarantee strict left-to-right `sequenced-before` evaluation (so `if ((ptr != NULL) && (ptr->ready != 0U))` is 100% safe).
  - **Clear-On-Read (`rc_r`) Short-Circuit Trap:** In `if (rx_enabled && (USART2->RDR == 'A'))`, when `rx_enabled == 0`, `USART2->RDR` is **never read** due to short-circuiting. Because reading `USART2->RDR` clears the hardware `RXNE` flag (RM0440 §37.8.10), skipping the read leaves `RXNE = 1`, causing `USART2_IRQHandler` to **re-enter infinitely (ISR storm)**! Always read clear-on-read registers unconditionally into a local variable first: `uint8_t ch = (uint8_t)USART2->RDR; if (rx_enabled && (ch == 'A')) { ... }`.

---

## 4. C11 Memory Model (`<stdatomic.h>`), 64-Bit Tearing & Memory Barriers (`C-ATOM-001..004`)

*Prerequisites: `C-QUAL-002`, `C-SEQ-001`.*

### 4.1 C11 `<stdatomic.h>` Memory Orders & Exact Cortex-M4 Mapping (`C-ATOM-002`)

ISO C11 §7.17 introduces a formal hardware-portable concurrency memory model via `<stdatomic.h>` (`_Atomic T`, `atomic_load_explicit`, `atomic_store_explicit`, `atomic_fetch_add_explicit`, `atomic_compare_exchange_weak_explicit`). Choosing the right **memory order** directly controls whether GCC emits hardware `DMB` barriers on ARM Cortex-M4:

| C11 Memory Order | Ordering Guarantee | Cortex-M4 Thumb-2 Codegen (`arm-none-eabi-gcc 14.3.1 -O2`) | Embedded Bare-Metal Use Case |
|---|---|---|---|
| `memory_order_relaxed` | Atomicity of the single variable **only**; zero cross-variable ordering guarantee. | Plain `LDR`/`STR` (or `LDREX`/`STREX` loop for RMW); **0 `DMB` instructions**. | Statistics counters, diagnostic telemetry increments (`g_irq_count`), producer reading its *own* `head` index. |
| `memory_order_acquire` | Used on **loads**: no subsequent read/write in this thread/ISR can be reordered *before* this load. Synchronizes-with a matching `release` store. | `LDR` followed by **`DMB ISH`** (prevents payload read from hoisting before flag/index load). | Consumer reading SPSC `head` before reading `buf[]`, or main loop polling a `ready` flag before consuming payload. |
| `memory_order_release` | Used on **stores**: no prior read/write in this thread/ISR can be reordered *after* this store. Publishes all preceding writes. | **`DMB ISH`** followed by `STR` (prevents payload write from sinking after flag/index store). | Producer writing `buf[]` then publishing SPSC `head`, or ISR filling telemetry struct then setting `ready = 1`. |
| `memory_order_acq_rel` | Combined acquire + release for atomic RMW (`fetch_add`, `compare_exchange`). | `DMB ISH` + `LDREX`/`STREX` loop + `DMB ISH`. | Lock-free reference counting, atomic CAS resource handoff. |
| `memory_order_seq_cst` | Strict total global order across all `seq_cst` operations (**default** for `atomic_load`/`atomic_store` without `_explicit`). | Full `DMB ISH` barriers around loads, stores, and RMWs. | Safe default when unsure; slightly higher barrier overhead than explicit `acquire`/`release`. |

**Compile-Verified C11 `<stdatomic.h>` Lock-Free SPSC Ring Buffer & Saturating CAS Loop (`C-ATOM-002`):**
```c
#include <stdatomic.h>
#include <stdbool.h>
#include <stdint.h>

#define ATOM_RING_CAP 64U
_Static_assert((ATOM_RING_CAP & (ATOM_RING_CAP - 1U)) == 0U, "Power of 2 required");

typedef struct {
    uint8_t          buf[ATOM_RING_CAP];
    _Atomic uint32_t head;
    _Atomic uint32_t tail;
} c11_spsc_t;

bool c11_spsc_push(c11_spsc_t *rb, uint8_t byte)
{
    uint32_t h = atomic_load_explicit(&rb->head, memory_order_relaxed);
    uint32_t t = atomic_load_explicit(&rb->tail, memory_order_acquire); /* LDR + DMB ISH */
    if ((uint32_t)(h - t) >= ATOM_RING_CAP) {
        return false;
    }
    rb->buf[h & (ATOM_RING_CAP - 1U)] = byte;
    atomic_store_explicit(&rb->head, h + 1U, memory_order_release);     /* DMB ISH + STR */
    return true;
}

bool c11_spsc_pop(c11_spsc_t *rb, uint8_t *out)
{
    uint32_t t = atomic_load_explicit(&rb->tail, memory_order_relaxed);
    uint32_t h = atomic_load_explicit(&rb->head, memory_order_acquire); /* LDR + DMB ISH */
    if (h == t) {
        return false;
    }
    *out = rb->buf[t & (ATOM_RING_CAP - 1U)];
    atomic_store_explicit(&rb->tail, t + 1U, memory_order_release);     /* DMB ISH + STR */
    return true;
}

/* Lock-free CAS increment saturating at max_val: compiles to LDREX/CMP/STREX + DMB ISH */
uint32_t atomic_inc_sat(_Atomic uint32_t *counter, uint32_t max_val)
{
    uint32_t cur = atomic_load_explicit(counter, memory_order_relaxed);
    while (cur < max_val) {
        if (atomic_compare_exchange_weak_explicit(
                counter, &cur, cur + 1U,
                memory_order_acq_rel, memory_order_relaxed)) {
            return cur + 1U;
        }
    }
    return cur;
}
```
> **Critical Cortex-M4 Hardware Caveat:** `LDREX`/`STREX` uses the CPU's **local exclusive monitor**, which automatically clears on exception entry/return (`ERET`), making it 100% atomic between CPU Thread mode and ISRs. However, it does **NOT** monitor AHB bus traffic from **DMA1/DMA2** — never use `LDREX`/`STREX` to synchronize a memory word simultaneously written by DMA!

### 4.2 Natural Atomic Width vs 64-Bit Word Tearing (`C-ATOM-003`)

On 32-bit ARM Cortex-M4, a single naturally aligned `uint8_t`, `uint16_t`, or `uint32_t` load (`LDR`) or store (`STR`) is single-instruction atomic with respect to interrupts. However:
- **RMW (`cnt++`) is NOT atomic:** Compiles to `LDR` → `ADD` → `STR`. An ISR preempting between `LDR` and `STR` loses an increment.
- **64-bit `volatile uint64_t` reads/writes TEAR:** Even with `LDRD`, Cortex-M4 permits an interrupt to abort/restart or split two 32-bit bus transfers! If a 64-bit microsecond counter increments from `0x00000000_FFFFFFFFULL` to `0x00000001_00000000ULL` mid-read, the reader can observe `0x00000001_FFFFFFFFULL` (+4.29 billion µs jump!). Use the lock-free double-read loop (`hi1 → lo → hi2`, retry while `hi1 != hi2`) shown in [Stage 03 — EMB-ATOM-003](../03-embedded-c/README.md#82-ldrexstrex-atomics-64-bit-double-read--multi-field-seqlock-emb-atom-002-emb-atom-003).

### 4.3 Compiler Barriers vs ARMv7-M Hardware Barriers (`C-ATOM-004`)

| Barrier | Emitted Instruction | Purpose |
|---|---|---|
| **Compiler Barrier:** `__asm__ volatile ("" ::: "memory")` | **0 CPU instructions** | Prevents GCC from reordering memory loads/stores across this point (essential inside `PRIMASK`/`BASEPRI` critical sections and double-read loops). |
| **`__DMB()` (Data Memory Barrier)** | `dmb sy` | Guarantees all prior explicit memory accesses become visible to other bus masters (DMA1/DMA2) before subsequent accesses begin. |
| **`__DSB()` (Data Synchronization Barrier)** | `dsb sy` | Stalls the CPU until all prior memory writes and write-buffer transfers physically complete (mandatory after clearing ISR peripheral flags before exit per ES0430, after `RCC` clock enable, and before `__WFI()`). |
| **`__ISB()` (Instruction Synchronization Barrier)** | `isb sy` | Flushes the 3-stage pipeline so subsequent instructions fetch using updated core configuration (mandatory after `SCB->CPACR`, `SCB->VTOR`, `MPU->CTRL`, `MSR CONTROL`). |

---

## 5. Fixed-Point Q15/Q31 vs IEEE-754 Single-Precision FPU Math (`C-MATH-001..003`, `EMB-NUM-001`)

*Prerequisites: `C-INT-002..004`.*

### 5.1 Q15 & Q31 Fixed-Point Arithmetic & Hardware `__SSAT` (`C-MATH-001`)

STM32G474RE's hardware **CORDIC** (`q1.31`/`q1.15`) and **FMAC** (`q1.15`) accelerators and digital power control loops use signed **Qm.n** fixed-point numbers representing fractions in $[-1.0,\ +1.0 - 2^{-n}]$:
- **Q15 (`int16_t`):** Scale factor $2^{15} = 32768$. Value $x_{\text{real}} = \frac{x_{\text{q15}}}{32768}$. Range $[-1.0,\ +0.999969]$.
- **Q31 (`int32_t`):** Scale factor $2^{31} = 2147483648$. Range $[-1.0,\ +0.9999999995]$.
- **Saturation Prevents Wraparound Instability:** In a closed-loop motor/power PID controller, wrapping $+0.999 + 0.1$ to $-0.901$ violently reverses actuator drive! ARMv7E-M provides single-cycle hardware signed saturation **`SSAT`** (`__SSAT(val, bit_width)` in `cmsis_gcc.h`), clamping out-of-range results cleanly to $[-32768,\ +32767]$.

**Compile-Verified Q15/Q31 Saturating Arithmetic & Overflow-Safe ADC Scaling (`C-MATH-001`, `EMB-NUM-001`):**
```c
#include <stdint.h>
#include "stm32g474xx.h"

/* Single-cycle Cortex-M4 SSAT saturating Q15 addition */
static inline int16_t q15_add_sat(int16_t a, int16_t b)
{
    return (int16_t)__SSAT((int32_t)a + (int32_t)b, 16U);
}

/* Rounded + saturated Q15 x Q15 -> Q15 multiply (handles -1.0 * -1.0 -> +0.999969!) */
static inline int16_t q15_mul_sat(int16_t a, int16_t b)
{
    int32_t prod = ((int32_t)a * (int32_t)b) + (1L << 14U);
    return (int16_t)__SSAT(prod >> 15U, 16U);
}

/* Rounded + saturated Q31 x Q31 -> Q31 multiply via 64-bit intermediate */
static inline int32_t q31_mul_sat(int32_t a, int32_t b)
{
    int64_t prod    = ((int64_t)a * (int64_t)b) + (1LL << 30U);
    int64_t shifted = prod >> 31U;
    if (shifted > (int64_t)INT32_MAX) { return INT32_MAX; }
    if (shifted < (int64_t)INT32_MIN) { return INT32_MIN; }
    return (int32_t)shifted;
}

/* Zero-float rounded 12-bit ADC raw-to-millivolts conversion (e.g., vref_mv = 3300UL) */
static inline uint32_t adc12_to_millivolts(uint16_t raw_12b, uint32_t vref_mv)
{
    return (((uint32_t)raw_12b * vref_mv) + 2047UL) / 4095UL;
}
```

### 5.2 IEEE-754 Binary32, The `double` Literal Trap & FPU Lazy Stacking (`C-MATH-002`, `C-MATH-003`)

The STM32G474RE Cortex-M4F includes an **FPv4-SP (`-mfpu=fpv4-sp-d16`)** hardware FPU — **SINGLE-PRECISION (`float`, 32-bit) ONLY!**
- **IEEE-754 Binary32 (`float`):** 1 sign bit, 8 exponent bits (bias 127), 23 mantissa bits (~7 decimal digits). Never compare floats with `==`; use `fabsf(a - b) < eps` and guard against `NaN` (`isnan(x)`).
- **The Lethal `double` Literal Trap:**
  In C, an unsuffixed floating-point literal (`3.3`, `0.5`, `3.14159`) has type **`double` (64-bit)**:
  ```c
  /* BUG: Promotes adc_v to 64-bit double, calls SLOW software __aeabi_dmul (50-100+ cycles),
     and bloats Flash by ~2-4 KB! */
  float volts_slow = adc_v * 3.3 / 4095.0;

  /* FAST: Hardware single-precision VMUL.F32 + VDIV.F32 (1-14 cycles on FPv4-SP!) */
  float volts_fast = adc_v * (3.3f / 4095.0f);
  ```
  **Mandatory Protection:** Always add **`-Wdouble-promotion -Werror`** to your `Makefile` and use single-precision math functions (`sinf`, `sqrtf`, `fabsf`, NOT `sin`, `sqrt`, `fabs`).
- **Hard-Float vs SoftFP vs Soft ABI (`C-MATH-003`):**
  - `-mfloat-abi=soft`: No FPU instructions; all floats emulated in `libgcc`.
  - `-mfloat-abi=softfp`: Emits hardware FPU instructions, passes arguments in integer `R0–R3` (extra `VMOV` penalty).
  - `-mfloat-abi=hard`: Emits hardware FPU instructions AND passes float arguments directly in `S0–S15` (fastest! Requires enabling `SCB->CPACR` CP10/CP11 + `__DSB(); __ISB();` in `SystemInit()`, else immediate `NOCP` UsageFault).
  - **FPU Lazy Stacking (`FPCCR.ASPEN` / `LSPEN`, ARM AN298):** When Thread mode uses the FPU, entering an ISR reserves **+72 bytes** (`S0–S15` + `FPSCR` + alignment word, expanding the exception frame from 32 B to **104 B**) on the stack, deferring the actual register push until the ISR itself executes an FPU instruction! Never ignore this +72 B worst-case stack expansion when sizing ISR/task stacks.

---

## 6. Newlib / Newlib-Nano Thread-Safety & Reentrancy (`C-LIB-001`)

*Prerequisites: `C-FUNC-001`, `C-MEM-002`, `TOOL-GCC-003`, `RTOS-MEM-001`.*

Bare-metal and RTOS STM32 toolchains (`arm-none-eabi-gcc`) link against **Newlib** (or its size-optimized variant **Newlib-Nano** via `--specs=nano.specs`). Out of the box, Newlib is **NOT thread-safe and NOT ISR-safe** — misunderstanding its internal `struct _reent` architecture is one of the top causes of mysterious field crashes in multi-task FreeRTOS firmware.

### 6.1 Inside `struct _reent` & Why Standard `libc` Functions Are Non-Reentrant

Rather than scattering independent globals across every `.c` file, Newlib aggregates per-execution-context C library state into a single structure **`struct _reent`** (`<reent.h>`), pointed to by the global pointer **`_impure_ptr`** (`_REENT` macro). Inside `struct _reent` live:
- `_errno` — the backing integer for `errno`!
- `_stdin`, `_stdout`, `_stderr` (`__sFILE`) — buffered stdio streams used by `printf`/`puts`.
- `_strtok_last` — the internal pointer remembered between calls to `strtok(NULL, delim)`.
- `_rand_next` — PRNG seed state for `rand()`/`srand()`.
- `_asctime_buf`, `_localtime_buf`, `_gamma_signgam`, `_mprec` (multi-precision big-integer heap buffers used by `printf("%f")` / `_dtoa_r`!), and `_atexit`.

**Newlib vs Newlib-Nano (`--specs=nano.specs`, `_REENT_SMALL`):**
- Full **Newlib** allocates a monolithic `struct _reent` of **~1,064 bytes** per context.
- **Newlib-Nano** compiles with `_REENT_SMALL`, shrinking the base `struct _reent` to **~96 bytes** and lazily allocating auxiliary sub-structs via `_malloc_r()` on first use (such as the first time a task formats a string or uses `strtok`!).

**Four Concrete Preemption / Concurrency Failures When `C-LIB-001` Is Ignored:**

| Standard C Call | Hidden Global / `_reent` State | Failure Mode Across RTOS Tasks or ISRs |
|---|---|---|
| **`malloc` / `free` / `calloc` / `realloc`** | Global heap free-list (`_sbrk` arena) | Task A is preempted mid-`malloc()` while splitting a free chunk; Task B calls `malloc()`/`free()` and corrupts the linked free-list → silent memory overlap or **HardFault** hours later. Default `__malloc_lock()` in Newlib is an **empty stub (`{}`)!** |
| **`printf` / `vprintf` / `snprintf("%f")`** | `_reent->_stdout` buffer, `_write()` syscall, AND **`_malloc_r()`** (Newlib's `_vfprintf_r` / `_dtoa_r` dynamically allocates temporary buffers for floating-point `%f` and stdio buffering!) | Two tasks calling `printf` interleave bytes or corrupt heap via unguarded `_malloc_r`. Calling `printf` inside an **ISR** either corrupts active heap pointers or **deadlocks permanently** if the interrupted task held `__malloc_lock`! |
| **`strtok(buf, ",")`** | `_reent->_strtok_last` | Task A parses `"10,20,30"`, gets preempted after `"10"`; Task B parses `"A,B"`, overwriting `_strtok_last`; Task A resumes and silently parses Task B's string! |
| **`errno`** | `_reent->_errno` | Task A checks `errno` after `strtol()`/`sqrtf()`, gets preempted by Task B which triggers `ERANGE`; Task A reads Task B's error code! |

### 6.2 Implementing Recursive `__malloc_lock` / `__malloc_unlock`

Newlib wraps every internal heap operation (`_malloc_r`, `_free_r`, `_realloc_r`, `_calloc_r`) with calls to two weak/overrideable hooks:
- `void __malloc_lock(struct _reent *r);`
- `void __malloc_unlock(struct _reent *r);`

**Two Mandatory Engineering Rules for `__malloc_lock`:**
1. **Must Support Recursive Nesting!** Internal Newlib routines (e.g., `_realloc_r` calling `_malloc_r` + `_free_r`) can invoke `__malloc_lock()` *while the same task already holds the lock*. A non-recursive binary mutex would immediately **self-deadlock**! Track nesting depth (or use `vTaskSuspendAll()` / `xTaskResumeAll()` in FreeRTOS, which is natively recursive!).
2. **Must Trap ISR Violations!** Never call `malloc`/`free` (or `printf`) from an ISR. Checking `__get_IPSR() != 0UL` inside `__malloc_lock()` immediately catches any illegal Handler-mode heap call via `__BKPT(0)` / fault handler instead of silently corrupting SRAM.

**Compile-Verified Bare-Metal Recursive `__malloc_lock` / `__malloc_unlock` (`C-LIB-001`):**
```c
#include <reent.h>
#include <stdint.h>
#include "stm32g474xx.h"

/* Bare-metal recursive PRIMASK-based Newlib heap lock (nesting-safe + ISR trap) */
static uint32_t g_malloc_lock_depth    = 0UL;
static uint32_t g_malloc_saved_primask = 0UL;

void __malloc_lock(struct _reent *r)
{
    (void)r;
    /* Calling malloc/free from Handler mode (ISR) is strictly forbidden! */
    if (__get_IPSR() != 0UL) {
        __BKPT(0);
        for (;;) { }
    }
    uint32_t primask = __get_PRIMASK();
    __disable_irq();
    __asm__ volatile ("" ::: "memory");
    if (g_malloc_lock_depth == 0UL) {
        g_malloc_saved_primask = primask;
    }
    g_malloc_lock_depth++;
}

void __malloc_unlock(struct _reent *r)
{
    (void)r;
    if (g_malloc_lock_depth > 0UL) {
        g_malloc_lock_depth--;
        if (g_malloc_lock_depth == 0UL) {
            uint32_t primask = g_malloc_saved_primask;
            __asm__ volatile ("" ::: "memory");
            __set_PRIMASK(primask);
        }
    }
}
```

### 6.3 FreeRTOS Integration: `configUSE_NEWLIB_REENTRANT` & Modern TLS Support

In a multi-task **FreeRTOS** application on STM32G474RE:
1. **Per-Task `struct _reent` Swapping:**
   - **FreeRTOS $\le$ 10.5:** Set `#define configUSE_NEWLIB_REENTRANT 1` in `FreeRTOSConfig.h`. FreeRTOS embeds a `struct _reent xNewLib_reent` directly inside every task's TCB, initializes it via `_REENT_INIT_PTR()`, switches `_impure_ptr = &(pxCurrentTCB->xNewLib_reent)` on every `PendSV` context switch, and cleans it up with `_reclaim_reent()` when a task deletes.
   - **FreeRTOS 10.6+ / 11.x (Modern Replacement):** `configUSE_NEWLIB_REENTRANT` is deprecated in favor of `#define configUSE_C_RUNTIME_TLS_SUPPORT 1` paired with `#include "newlib_tls.h"` (or `picolibc_tls.h`), achieving the exact same per-task `_reent` isolation cleanly across C libraries.
   - **RAM Cost Trade-Off:** Enabling per-task `_reent` adds **~96 B per task** under Newlib-Nano (`--specs=nano.specs`) vs **~1 KB+ per task** under full Newlib!
2. **FreeRTOS `__malloc_lock` Pattern:**
   Instead of disabling all interrupts with `PRIMASK` (which would increase ISR latency during heap splits), FreeRTOS applications implement `__malloc_lock` using **`vTaskSuspendAll()`** (natively recursive, keeps all hardware ISRs running at zero added latency!) and `__malloc_unlock` using **`(void)xTaskResumeAll()`**, while still trapping `__get_IPSR() != 0UL`!
   *(Also note: if using FreeRTOS `heap_4.c`, `pvPortMalloc()` uses its own internal `vTaskSuspendAll()` lock and is separate from Newlib's `malloc()` unless you wrap/redirect Newlib `malloc` to `pvPortMalloc` via `-Wl,--wrap=malloc` — or better yet, ban Newlib `malloc` completely!)*

### 6.4 Safe Bare-Metal & ISR Alternatives (Zero Heap, Zero `_reent` Hazard)

For deterministic, safety-critical bare-metal and ISR code, **avoid Newlib's stateful functions altogether**:
1. **Formatting:** Never call `printf` inside an ISR. In Thread mode, use **`snprintf()` into a caller-owned stack buffer** (avoiding `%f` float formatting in Newlib-Nano unless integer-scaled `int32_t mv` is printed as `%ld.%03ld V`, which performs **zero hidden `_malloc_r` calls!**), or use a dedicated reentrant zero-heap formatter (`mpaland/printf` / `eyalroz/printf`) and transmit via a lock-free SPSC ring buffer (`FW-PAT-004`).
2. **Tokenization:** Replace `strtok()` with POSIX **`strtok_r(buf, delim, &saveptr)`** (explicit caller-owned `saveptr` on the stack; requires `#define _POSIX_C_SOURCE 200809L` under strict `-std=c11`, or `-std=gnu11`).
3. **Allocation:** Replace `malloc`/`free` with static caller-owned buffers or $O(1)$ fixed-block memory pools ([Stage 03 — FW-PAT-005](../03-embedded-c/README.md#85-hsm-o1-memory-pool-observer--command-patterns-fw-pat-001-005-007-fw-fsm-002)).

**Compile-Verified Reentrant Stack-Buffer Tokenization (`strtok_r`, `C-LIB-001`):**
```c
#define _POSIX_C_SOURCE 200809L
#include <stddef.h>
#include <string.h>

/* 100% reentrant: state lives in caller's stack frame (saveptr), never in _reent */
size_t split_csv_reentrant(char *line_buf, char *tokens[], size_t max_tokens)
{
    char *saveptr = NULL;
    size_t count  = 0U;
    char *tok = strtok_r(line_buf, ",", &saveptr);
    while ((tok != NULL) && (count < max_tokens)) {
        tokens[count++] = tok;
        tok = strtok_r(NULL, ",", &saveptr);
    }
    return count;
}
```

---

## 7. Assessment Questions

1. **(`C-ALIGN-002`)** Even when `SCB->CCR.UNALIGN_TRP == 0` (reset default on Cortex-M4), why can passing `&packed_frame.u32_field` to a helper function `void parse(uint32_t *p)` still trigger an immediate `UsageFault` (`CFSR.UNALIGNED = 1`)? Which exact ARMv7-M instruction classes always fault on unaligned addresses?
2. **(`C-UB-001`, `C-UB-002`)** Explain why `*(uint32_t *)&float_val` violates ISO C11 §6.5p7 strict aliasing, why `unsigned char *` is the legal exception, and why `memcpy(&u32, &f, sizeof(u32))` incurs zero library call overhead at `-O2`.
3. **(`C-UB-003`, `C-MEM-003`)** Classify each snippet as UB, Unspecified, or Implementation-Defined: (a) `combine(spi_rx(), spi_rx())`, (b) `int8_t x = -8; x >> 1;`, (c) `int v = *p; if (p == NULL) return -1;`. Explain why GCC `-O2` deletes the `if (p == NULL)` branch in (c).
4. **(`C-SEQ-002`)** Explain how short-circuiting in `if (rx_en && (USART2->RDR == 0x55U))` can trigger an infinite `USART2_IRQHandler` storm when `rx_en == 0`.
5. **(`C-ATOM-002`)** In `c11_spsc_push()` / `c11_spsc_pop()`, explain why the producer loads its *own* `head` with `memory_order_relaxed`, loads `tail` with `memory_order_acquire`, and stores `head + 1` with `memory_order_release`. What exact Cortex-M4 instruction implements `acquire`/`release` vs `atomic_compare_exchange_weak_explicit`? Why does `LDREX`/`STREX` NOT synchronize against DMA1/DMA2?
6. **(`C-MATH-001..003`)** Why does `float v = adc * 3.3 / 4095.0;` pull software `__aeabi_d*` routines into STM32G474RE Flash, and how does `q15_mul_sat()` prevent `-1.0 * -1.0` overflow wraparound using `__SSAT`?
7. **(`C-LIB-001`)** List 4 state fields stored in Newlib's `struct _reent`. Why must `__malloc_lock()` be **recursive** and trap `__get_IPSR() != 0`, what does FreeRTOS's `configUSE_NEWLIB_REENTRANT` (or `configUSE_C_RUNTIME_TLS_SUPPORT`) swap on context switch, and why can calling `printf("%f", x)` inside an ISR corrupt the heap or deadlock?

---

## 8. Verification Checklist (Disassembly & Fault Forensics)

- [ ] Compile `float_to_u32_bits` (`memcpy`) at `-O2 -mfpu=fpv4-sp-d16 -mfloat-abi=hard` and confirm `arm-none-eabi-objdump -d` contains **zero** `bl memcpy` branch.
- [ ] Compile `y = x * 3.3` vs `y = x * 3.3f` with `-Wdouble-promotion -Werror` and verify zero `__aeabi_d*` symbols appear in `firmware.map`.
- [ ] Enable `SCB->CCR |= SCB_CCR_UNALIGN_TRP_Msk`, dereference an unaligned pointer from a packed struct in GDB, confirm `CFSR.UNALIGNED == 1`, and fix with `read_be32_safe()` / `read_le32_safe()`.
- [ ] Verify `c11_spsc_push` emits `dmb ish` and `atomic_inc_sat` emits `ldrex`/`strex` + `dmb ish` in `arm-none-eabi-objdump -d`.
- [ ] Verify `__malloc_lock` / `__malloc_unlock` compiles cleanly against `<reent.h>` and traps Handler mode (`__get_IPSR() != 0UL`).

---

> **Sequential Navigation (Step 2B of 17 (Companion Deep-Dive)):**  
> **Prev:** [Stage 02 — C Language Foundations (`02-c-fundamentals/README.md`)](./README.md) | **Home:** [`START_HERE.md`](../START_HERE.md) | **Progress:** [`LEARNER_PROGRESS_DASHBOARD.md`](../LEARNERS/LEARNER_PROGRESS_DASHBOARD.md) | **Next:** [Stage 03 — Embedded C & MMIO (`03-embedded-c/`)](../03-embedded-c/README.md)
