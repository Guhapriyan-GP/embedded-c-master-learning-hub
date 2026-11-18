# Stage 02 Companion Deep-Dive: Alignment, Undefined Behavior, C11 Memory Model & Fixed/Floating-Point Math

> **Parent Module:** [Stage 02 — C Language Foundations](./README.md)  
> **Concept IDs Covered:** `C-ALIGN-001..003`, `C-UB-001..003`, `C-SEQ-001..002`, `C-ATOM-001..004`, `C-MATH-001..003`, `C-PTR-004`, `C-OPT-001..003` (Levels 2–3)  
> **Target ABI:** ARM Cortex-M4F (`-mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard -std=gnu11`)  
> **Code Verification Policy:** All C code snippets below are **Statically Reviewed (Not Tested on Physical Hardware)** unless explicitly tagged **Compile-Verified (`arm-none-eabi-gcc 14.3.1`)**.

---

## 1. Alignment, Padding, `__attribute__((packed))` & Cortex-M4 Unaligned Faults (`C-ALIGN-001..003`)

### 1.1 Natural Alignment & `_Alignof` / `_Alignas` (`C-ALIGN-001`)

Under ARM AAPCS (§5.1, §7.1.8), every scalar type has a natural alignment equal to its size:
- `uint8_t`: 1-byte aligned
- `uint16_t`: 2-byte aligned (`addr % 2 == 0`)
- `uint32_t`, `float`, pointers: 4-byte aligned (`addr % 4 == 0`)
- `uint64_t`, `double`: **8-byte aligned (`addr % 8 == 0`)**

Use C11 `_Alignof(T)` to query alignment, `offsetof(S, m)` (`<stddef.h>`) to verify member byte offsets, and **`_Alignas(N)`** to enforce hardware alignment constraints (e.g., STM32G474RE relocated vector tables require **512-byte alignment** for `SCB->VTOR`, and DMA/MPU buffers require power-of-2 alignment):

```c
#include <stdalign.h>
#include <stdint.h>

/* Relocated SRAM vector table for STM32G474RE (118 vectors = 472 bytes -> 512 B aligned!) */
_Alignas(512) static uintptr_t sram_vector_table[118];
```

### 1.2 The `__attribute__((packed))` Hazard & Cortex-M4 `UNALIGN_TRP` (`C-ALIGN-002`)

Many online tutorials dangerously recommend overlaying `__attribute__((packed))` structs onto raw communication byte buffers. Understand the **exact ARM Cortex-M4 hardware reality** (ARMv7-M ARM §A3.2.1):

| Instruction Class | `SCB->CCR.UNALIGN_TRP == 0` (Reset Default) | `SCB->CCR.UNALIGN_TRP == 1` |
|---|---|---|
| Single `LDR`, `STR`, `LDRH`, `STRH`, `LDRSH` | Hardware splits into multiple bus cycles (slower, **non-atomic!**) | **Immediate `UsageFault` (`CFSR.UNALIGNED = 1`)** |
| Multi-word `LDRD`, `STRD`, `LDM`, `STM`, `PUSH`, `POP` | **ALWAYS `UsageFault` (`CFSR.UNALIGNED = 1`)!** | **ALWAYS `UsageFault`!** |
| Exclusive `LDREX`, `STREX` & FPU `VLDR`/`VSTR` | **ALWAYS `UsageFault`!** | **ALWAYS `UsageFault`!** |

**Why Taking the Address of a Packed Member Crashes:**
If `struct __attribute__((packed)) Frame { uint8_t hdr; uint32_t val; };` is passed by member pointer (`process(&f.val);`), GCC loses the `packed` qualifier on the resulting `uint32_t *` (flagged by `-Waddress-of-packed-member`). Inside `process()`, GCC assumes 4-byte alignment and may emit `LDRD`/`LDM` — triggering an immediate **UsageFault / HardFault**!

**The Production-Safe Solution — Explicit Byte-Wise Serialization or `memcpy`:**
```c
#include <stdint.h>
#include <string.h>

/* 100% alignment-safe, endianness-explicit Big-Endian wire deserialization */
static inline uint32_t read_be32(const uint8_t *p)
{
    return ((uint32_t)p[0] << 24U) |
           ((uint32_t)p[1] << 16U) |
           ((uint32_t)p[2] <<  8U) |
           ((uint32_t)p[3]);
}
```

### 1.3 Endianness & Single-Cycle Cortex-M4 Byte Swap (`C-ALIGN-003`)

STM32G474RE (ARM Cortex-M4) is **Little-Endian**: `uint32_t x = 0x12345678UL` stores bytes `[0x78, 0x56, 0x34, 0x12]` from lowest to highest address. Network and industrial wire protocols (TCP/IP, Modbus RTU, many CAN/SPI sensor payloads) are **Big-Endian** (MSB first).
- Use `__builtin_bswap32(x)` / CMSIS `__REV(x)` (compiles to a single-cycle `REV r0, r0` instruction) or `__REV16(x)` for 16-bit halfword swaps.

---

## 2. Strict Aliasing, Safe Type Punning & Behavioral Taxonomy (`C-UB-001..003`, `C-PTR-004`)

### 2.1 Strict Aliasing Rule (ISO C11 §6.5p7) (`C-UB-001`)

At `-O2`, `-O3`, and `-Os`, GCC enables **`-fstrict-aliasing`** by default. The compiler assumes two pointers of **incompatible types** (e.g., `uint32_t *` and `float *`, or `uint32_t *` and `uint16_t *`) **never point to the same memory address**!
- **Exception:** `unsigned char *` / `char *` / `uint8_t *` (when `typedef unsigned char uint8_t`) is explicitly permitted to inspect the raw bytes of any object.
- **Classic Miscompilation Bug:**
  ```c
  /* UNDEFINED BEHAVIOR: violates strict aliasing! */
  uint32_t float_bits_broken(float f)
  {
      return *(uint32_t *)&f; /* GCC -O2 may optimize away the store to f or return garbage! */
  }
  ```

### 2.2 Safe Type Punning with `memcpy` (`C-UB-002`)

The **100% ISO C11 (and C++20) conformant, alignment-safe, strict-aliasing-safe** way to reinterpret raw bit representations is `memcpy`:
```c
#include <stdint.h>
#include <string.h>

uint32_t float_to_u32_bits(float f)
{
    uint32_t u = 0U;
    _Static_assert(sizeof(u) == sizeof(f), "IEEE-754 binary32 required");
    memcpy(&u, &f, sizeof(u));
    return u;
}
```
At `arm-none-eabi-gcc -O2 -mfpu=fpv4-sp-d16 -mfloat-abi=hard`, GCC recognizes the fixed-size `memcpy` and compiles the entire function into a **single zero-overhead hardware instruction**: `vmov r0, s0; bx lr` — **zero call to `libc` `memcpy`!**
*(Note: C99/C11 Footnote 95 allows type punning via a `union`, which works in ISO C, but is UB in C++ and flagged by MISRA C Rule 19.2; `memcpy` is portable across both C and C++.)*

### 2.3 The `restrict` Qualifier (`C-PTR-004`)

When two pointers have the *same* type (`float *out`, `const float *in`), GCC must assume they *might* overlap and therefore reloads `in[i]` after every write to `out[i]`. Adding C99 **`restrict`** promises the compiler that the regions do not overlap, unlocking loop vectorization and register caching in DSP/FIR kernels:
```c
void vec_scale_f32(const float * restrict src, float * restrict dst, float gain, size_t n)
{
    for (size_t i = 0U; i < n; ++i) {
        dst[i] = src[i] * gain;
    }
}
```
*(Passing overlapping buffers to `restrict` parameters is UB!)*

### 2.4 Behavioral Taxonomy: UB vs Unspecified vs Implementation-Defined (`C-UB-003`)

| Category | ISO C11 Definition | Compiler Obligation | Embedded Examples |
|---|---|---|---|
| **Undefined Behavior (UB)** | Erroneous construct; standard imposes **no requirements** (§3.4.3, Annex J.2) | Compiler assumes UB **never happens** and optimizes aggressively (may delete NULL checks or overflow guards!) | Signed overflow, `1 << 31`, `x << 32`, strict-aliasing violation, out-of-bounds access, NULL dereference, uninitialized read, modifying string literal. |
| **Unspecified Behavior** | Standard allows 2+ valid choices without requiring documentation (§3.4.4, Annex J.1) | Choice may change between `-O0` and `-O2` or between statements | **Function argument evaluation order:** `combine(SPI_Read(), SPI_Read())` — which read runs first is unspecified! |
| **Implementation-Defined** | Standard allows 2+ choices, but toolchain **must document** its choice (§3.4.1, Annex J.3) | Stable for a specific compiler/ABI target (`arm-none-eabi-gcc`) | Signedness of plain `char` (`unsigned` on ARM), `sizeof(int)`, right-shift of negative `int`, bit-field layout, `enum` width. |

---

## 3. Evaluation Order, C11 `sequenced-before` & Short-Circuiting (`C-SEQ-001..002`)

- **Unsequenced Side-Effect UB (`C-SEQ-001`):** Modifying a scalar object twice (or modifying and reading it independently) without a `sequenced-before` relation (C11 §6.5p2) is **UB**: `i = i++ + 1;`, `buf[i] = i++;`. Compile with `-Wsequence-point`.
- **Function Argument Evaluation Hazard:** Never read hardware FIFOs twice inside a single function call:
  ```c
  /* BUG: Unspecified which SPI1->DR read happens first (MSB vs LSB swapped randomly!) */
  uint16_t bad = make_u16(spi_read_byte(), spi_read_byte());

  /* CORRECT: Explicit semicolon sequencing */
  uint8_t hi = spi_read_byte();
  uint8_t lo = spi_read_byte();
  uint16_t good = ((uint16_t)hi << 8U) | lo;
  ```
- **Short-Circuit Guarantees & Clear-On-Read Traps (`C-SEQ-002`):**
  The operators `&&`, `||`, `?:`, and the comma operator `,` guarantee strict left-to-right `sequenced-before` evaluation. This makes `if ((ptr != NULL) && (ptr->ready != 0U))` 100% safe. **However**, beware short-circuiting a `volatile` clear-on-read register: in `if (flag && (USART2->RDR == 'A'))`, when `flag == 0`, `USART2->RDR` is **never read**, leaving `RXNE` asserted and potentially re-entering an RX ISR infinitely!

---

## 4. C11 Atomics (`<stdatomic.h>`), 64-Bit Tearing & Memory Barriers (`C-ATOM-001..004`)

### 4.1 Natural Atomic Width vs 64-Bit Word Tearing (`C-ATOM-003`)

On 32-bit ARM Cortex-M4, a single naturally aligned `uint8_t`, `uint16_t`, or `uint32_t` load (`LDR`) or store (`STR`) is **single-instruction atomic** with respect to interrupts. However:
- **RMW (`cnt++`) is NOT atomic:** Compiles to `LDR` → `ADD` → `STR`. An ISR preempting between `LDR` and `STR` loses an increment.
- **64-bit `volatile uint64_t` reads/writes TEAR:** Even with `LDRD`, Cortex-M4 allows an interrupt to abort/restart or split two 32-bit transfers! If a 64-bit microsecond counter increments from `0x00000000_FFFFFFFFULL` to `0x00000001_00000000ULL` mid-read, the reader can observe `0x00000001_FFFFFFFFULL` (jumping **+4.29 billion µs** into the future!).
- **Lock-Free Double-Read Remedy:**
  ```c
  extern volatile uint32_t g_time_hi;
  extern volatile uint32_t g_time_lo;

  uint64_t read_u64_atomic(void)
  {
      uint32_t hi1, lo, hi2;
      do {
          hi1 = g_time_hi;
          __asm__ volatile ("" ::: "memory");
          lo  = g_time_lo;
          __asm__ volatile ("" ::: "memory");
          hi2 = g_time_hi;
      } while (hi1 != hi2);
      return ((uint64_t)hi1 << 32U) | lo;
  }
  ```

### 4.2 Compiler Barriers vs ARMv7-M Hardware Barriers (`C-ATOM-004`)

| Barrier | Emitted Instruction | Purpose |
|---|---|---|
| **Compiler Barrier:** `__asm__ volatile ("" ::: "memory")` | **0 CPU instructions** | Prevents GCC from reordering memory loads/stores across this line (essential inside critical sections and before/after volatile publish flags). |
| **`__DMB()` (Data Memory Barrier)** | `dmb sy` | Guarantees all prior explicit memory accesses become visible to other bus masters (DMA1/DMA2) before subsequent accesses begin. |
| **`__DSB()` (Data Synchronization Barrier)** | `dsb sy` | Stalls the CPU until all prior memory writes and write-buffer transfers physically complete (mandatory after clearing ISR peripheral flags before exit per ES0430, or before `__WFI()`). |
| **`__ISB()` (Instruction Synchronization Barrier)** | `isb sy` | Flushes the 3-stage pipeline so subsequent instructions fetch using updated core configuration (mandatory after `SCB->CPACR`, `SCB->VTOR`, `MPU->CTRL`, `MSR CONTROL`). |

### 4.3 C11 `<stdatomic.h>` Mapping to Cortex-M4 `LDREX`/`STREX` (`C-ATOM-002`)

Under `arm-none-eabi-gcc -mcpu=cortex-m4 -std=gnu11`, C11 `<stdatomic.h>` compiles lock-free operations directly into Cortex-M4 hardware exclusive monitor loops (`LDREX`/`STREX`) plus `DMB` barriers — **with zero interrupt masking**:
```c
#include <stdatomic.h>
#include <stdint.h>

static _Atomic uint32_t g_packet_count = 0U;

void count_packet_lockfree(void)
{
    /* Compiles to: ldrex / adds / strex / cmp / bne loop on Cortex-M4! */
    (void)atomic_fetch_add_explicit(&g_packet_count, 1U, memory_order_relaxed);
}
```
*(Caveat: Cortex-M4 `LDREX`/`STREX` synchronizes CPU thread vs ISR only — it does **NOT** synchronize against DMA bus masters!)*

---

## 5. Fixed-Point Q15/Q31 vs IEEE-754 Single-Precision FPU Math (`C-MATH-001..003`)

### 5.1 Q15 & Q31 Fixed-Point Arithmetic (`C-MATH-001`)

STM32G474RE's signature **CORDIC** (`q1.31`/`q1.15`) and **FMAC** (`q1.15`) accelerators and digital power control loops use signed **Qm.n** fixed-point numbers representing fractions in $[-1.0,\ +1.0 - 2^{-n}]$:
- **Q15 (`int16_t`):** Scale factor $2^{15} = 32768$. Value $x_{\text{real}} = \frac{x_{\text{q15}}}{32768}$. Range $[-1.0,\ +0.999969]$.
- **Q31 (`int32_t`):** Scale factor $2^{31} = 2147483648$.
- **Q15 Multiply with Rounding & Cortex-M4 Hardware Saturation (`__SSAT`):**
  Multiplying two Q15 numbers produces a Q30 value in `int32_t`. Add rounding bias `(1 << 14)`, shift right by 15, and saturate to 16-bit signed using ARMv7E-M `SSAT` (`__SSAT(val, 16)` in `cmsis_gcc.h`) to prevent `-1.0 * -1.0 = +1.0` overflow wraparound:
  ```c
  #include <stdint.h>
  #include "cmsis_gcc.h"

  static inline int16_t q15_mul_sat(int16_t a, int16_t b)
  {
      int32_t prod = ((int32_t)a * (int32_t)b) + (1L << 14U);
      return (int16_t)__SSAT(prod >> 15U, 16U);
  }
  ```

### 5.2 IEEE-754 Binary32 & The Cortex-M4F `double` Literal Trap (`C-MATH-002`, `C-MATH-003`)

The STM32G474RE Cortex-M4F includes an **FPv4-SP (`-mfpu=fpv4-sp-d16`)** hardware FPU — **SINGLE-PRECISION (`float`, 32-bit) ONLY!**
- **IEEE-754 Binary32 (`float`):** 1 sign bit, 8 exponent bits (bias 127), 23 mantissa bits (~7 decimal digits). Never compare floats with `==`; use `fabsf(a - b) < eps` and guard against `NaN` (`isnan(x)`).
- **The Lethal `double` Literal Trap:**
  In C, an unsuffixed floating-point literal (`3.3`, `0.5`, `3.14159`) has type **`double` (64-bit)**!
  ```c
  /* TRAP: Promotes adc_v to 64-bit double, calls SLOW software __aeabi_dmul (50-100+ cycles),
     and bloats Flash by ~2-4 KB! */
  float volts_slow = adc_v * 3.3 / 4095.0;

  /* FAST: Hardware single-precision VMUL.F32 + VDIV.F32 (1-14 cycles on FPv4-SP!) */
  float volts_fast = adc_v * (3.3f / 4095.0f);
  ```
  **Mandatory Protection:** Always add **`-Wdouble-promotion -Werror`** to your `Makefile` and use single-precision math functions (`sinf`, `sqrtf`, `fabsf`, NOT `sin`, `sqrt`, `fabs`).
- **Hard-Float vs SoftFP vs Soft ABI (`C-MATH-003`):**
  - `-mfloat-abi=soft`: No FPU instructions; all floats emulated in `libgcc`.
  - `-mfloat-abi=softfp`: Emits hardware FPU instructions, but passes arguments in integer `R0–R3` (requires extra `VMOV` transfers).
  - `-mfloat-abi=hard`: Emits hardware FPU instructions AND passes float arguments directly in `S0–S15` (fastest! Requires enabling `SCB->CPACR` CP10/CP11 + `__DSB(); __ISB();` in `SystemInit()`, else immediate `NOCP` UsageFault!).
  - Remember: using FPU instructions inside an ISR triggers hardware stacking of `S0–S15` + `FPSCR` (**+72 bytes**, expanding the exception frame from 32 B to **104 B**; see [Stage 04 — ARM Cortex-M4](../04-arm-cortex-m/README.md)).

---

## 6. Verification Checklist (Disassembly & Fault Forensics)

- [ ] Compile `float_to_u32_bits` (`memcpy`) at `-O2 -mfpu=fpv4-sp-d16 -mfloat-abi=hard` and confirm `arm-none-eabi-objdump -d` emits a single `vmov r0, s0` with zero `memcpy` branch.
- [ ] Compile `y = x * 3.3` vs `y = x * 3.3f` with `-Wdouble-promotion` and verify zero `__aeabi_d*` symbols appear in `firmware.map`.
- [ ] Enable `SCB->CCR |= SCB_CCR_UNALIGN_TRP_Msk`, dereference an unaligned pointer from a packed struct in GDB, confirm `CFSR.UNALIGNED == 1`, and fix with `read_be32()`.
- [ ] Verify `atomic_fetch_add_explicit` emits `ldrex`/`strex` in disassembly (`objdump -d`).
