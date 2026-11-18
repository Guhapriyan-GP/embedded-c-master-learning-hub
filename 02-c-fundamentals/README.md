# Stage 02: C Language Foundations for Bare-Metal Embedded Systems

> **Sequential Navigation (Step 2 of 17 — Canonical Acyclic Order):**  
> **Home:** [`START_HERE.md`](../START_HERE.md) | **Roadmap:** [`learning-sequence.md`](../01-master-concept-map/learning-sequence.md) | **Prev:** [Stage 01b — Digital Electronics & HW Integration (`01b-*`)](../01b-digital-electronics-and-hw-integration/README.md) | **Next:** [Stage 03 — Embedded C & MMIO (`03-embedded-c/`)](../03-embedded-c/README.md)  
> **Prerequisites:** [Stage 01b (`DIG-ELEC-*`)](../01b-digital-electronics-and-hw-integration/README.md)

> **Curriculum Track:** Level 1–3 Foundation (ISO C11 / `arm-none-eabi-gcc 14.3.1 -std=c11` on Cortex-M4F)  
> **Companion Deep-Dive:** [Advanced C: Alignment, Strict Aliasing, Undefined Behavior, C11 Atomics, Fixed/Floating-Point Math & Newlib Reentrancy](./advanced-c-ub-memory-model.md)  
> **Mandatory Verification Harness:** [tests/test_c_foundations.c](../tests/test_c_foundations.c)  
> **Code Verification Policy:** All C11 examples in Section 8 are **Compile-Verified (`arm-none-eabi-gcc 14.3.1`, `-std=c11 -mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard -DSTM32G474xx -O2 -Wall -Wextra -Werror`, Not Tested on Physical Hardware)**.

---

## 1. Learning Objectives

Upon completing Stage 02 (including its companion guide [advanced-c-ub-memory-model.md](./advanced-c-ub-memory-model.md)), you will master all ISO C embedded domains and computer science foundations:

| Domain | Concept IDs | Level | Prerequisites | Core Mastery Outcomes |
|---|---|---|---|---|
| CS Foundations & Determinism | `CS-NUM-001`, `CS-BOOL-001`, `CS-DATA-001`, `CS-FSM-001`, `CS-CONC-001`, `CS-ALGO-001` | L1–L2 | None → `CS-DATA-001` | Apply positional binary/hex math, De Morgan simplification, static data structures, finite automata, concurrency invariants, and deterministic $O(1)$ vs $O(N)$ WCET analysis via Cortex-M4 `__CLZ()` and `DWT->CYCCNT`. |
| Compilation, TUs & Linkage | `C-COMP-001..004` | L1–L2 | `CS-NUM-001` | Trace ISO C's 8 translation phases and GCC's 4 stages (`cpp` → `cc1` → `as` → `ld`), separate translation units cleanly, apply `-fno-common` & ODR, and use freestanding C11 headers (`-ffreestanding`). |
| Integers, Promotions & Overflow | `C-INT-001..004` | L1–L2 | `C-COMP-004` | Use `<stdint.h>` (`uintN_t`, `uintptr_t`, `size_t`, `ptrdiff_t`), two's complement & `INT_MIN` asymmetry, signed overflow UB vs unsigned modular wraparound, and integer promotion / usual arithmetic conversion rules (`~uint8_t`, `uint16_t * uint16_t`). |
| Bitwise & Shift Safety | `C-BIT-001..003` | L1 | `C-INT-001..004` | Write glitch-free single-store shadow-register bit masking, avoid all shift UBs (`1 << 31`, `x << 32`, negative shifts), and implement portable bitfield extraction & sub-word sign extension. |
| Pointers, Arrays & Strings | `C-PTR-001..004`, `C-ARR-001..004` | L1–L3 | `C-INT-001` | Apply scaled pointer math, read all `const`/`volatile` qualifier combinations, avoid Cortex-M `0x00000000` NULL-read aliasing, use `restrict`, handle array decay & exceptions, `.rodata` strings, row-major 2D DMA arrays, C99 FAMs, and banned VLAs (`-Wvla`). |
| Composite Types | `C-TYPE-001..004` | L1–L2 | `C-PTR-001`, `C-BIT-001` | Design structs with C99 designated initializers, tagged (discriminated) unions, avoid C bit-field MMIO bus-width hazards, handle `enum` ABI sizing (`-fshort-enums`), and encapsulate drivers with `typedef` opaque pointers. |
| Storage, Qualifiers & Macros | `C-STOR-001..004`, `C-QUAL-001..004`, `C-PREP-001..004` | L1–L2 | `C-COMP-002..003` | Master `static` (internal linkage vs block persistence), `extern` (including linker symbols `&_sdata`), `.bss` zero-init vs indeterminate locals, `const` `.rodata`, `static inline` vs C99 `inline`, `_Static_assert`, `do { ... } while (0)`, two-level `#`/`##`, X-Macros, `__VA_ARGS__`, and ISO include guards. |
| Functions, Memory & Optimization | `C-FPTR-001..003`, `C-MEM-001..003`, `C-FUNC-001..003`, `C-OPT-001..003` | L1–L2 | `C-PTR-001`, `C-STOR-001..002` | Build `void *ctx` callback APIs and bounds-checked `.rodata` FSM jump tables, map C objects to ELF sections, eliminate `malloc`/dangling-pointer/recursion hazards (`-fstack-usage`), understand AAPCS `R0–R3` call overhead, and apply `-O0..-Os/-Og/-flto` & `--gc-sections`. |
| Alignment, UB, Atomics, Math & Newlib | `C-ALIGN-001..003`, `C-UB-001..003`, `C-SEQ-001..002`, `C-ATOM-001..004`, `C-MATH-001..003`, `C-LIB-001` | L2–L3 | Stage 02 Core | Covered in full depth in [advanced-c-ub-memory-model.md](./advanced-c-ub-memory-model.md): alignment/padding/`packed` faults, Little-Endian & `__REV`, strict aliasing & `memcpy` punning, C11 `sequenced-before`, `<stdatomic.h>` memory orders, Q15/Q31 fixed-point, IEEE-754 `double` traps, and Newlib `_reent`/`__malloc_lock`. |

---

## 2. Prerequisites

- **Foundational Logic & Number Systems:** Binary/hexadecimal representation (`CS-NUM-001`), Boolean algebra & De Morgan's laws (`CS-BOOL-001`).
- **Recommended Hardware Companion:** [Stage 01b — Digital Electronics & Hardware Integration](../01b-digital-electronics-and-hw-integration/README.md) (`DIG-ELEC-001..003`).
- **Toolchain Setup:** `arm-none-eabi-gcc 14.3.1` on `PATH` for cross-compiling STM32G474RE (Cortex-M4F) snippets and inspecting `.i`, `.s`, `.su`, and `objdump -d` outputs.

---

## 3. Detailed Technical Theory

### 3.1 ISO C 8 Translation Phases, Translation Units, Linkage & ODR (`C-COMP-001..004`)

*Prerequisites: `CS-NUM-001`.*

Before a single Cortex-M4 instruction is emitted, ISO/IEC 9899:2011 (§5.1.1.2) defines **8 strict translation phases**:
1. **Physical character mapping** (source character set normalization).
2. **Line splicing:** Every backslash immediately followed by a newline (`\` + `\n`) is deleted, joining multi-line macros before tokenization.
3. **Tokenization & comment stripping:** Every comment (`/* ... */` or `// ...`) is replaced by a **single space** (not deleted — `int/*x*/a` becomes `int a`, not `inta`).
4. **Preprocessing (`cpp`):** Executes `#include`, `#define`, `#if`/`#ifdef`, `#pragma`, and expands macros. Produces a single **Translation Unit (TU)** (`.i` via `arm-none-eabi-gcc -E`).
5. **Escape sequence conversion** (`\n`, `\x1B`) inside character and string literals.
6. **Adjacent string literal concatenation** (`"STM32" "G474"` → `"STM32G474"`).
7. **Compilation & Assembly (`cc1` → `.s`, `as` → `.o`):** Syntax/semantic analysis, optimization, Thumb-2 code generation, and assembly into a relocatable ELF object file (`.o`) with symbol and relocation tables.
8. **Linking (`ld` → `.elf`):** Resolves external references (`U`) against defined symbols (`T`/`D`/`B`/`R`) across all `.o` files and static `.a` archives, assigns Flash/SRAM VMA/LMA addresses per the linker script, and outputs the executable `.elf`.

**Linkage, Declarations vs Definitions, ODR & `-fno-common` (`C-COMP-002`, `C-COMP-003`):**
- **Three Linkage Kinds (ISO C11 §6.2.2):**
  1. **External linkage:** Visible across all TUs (default for file-scope functions and non-`static` file-scope variables; uppercase `T`/`D`/`B` in `arm-none-eabi-nm`).
  2. **Internal linkage:** Private to a single TU via file-scope `static` (lowercase `t`/`d`/`b` in `arm-none-eabi-nm`). Two `.c` files can each define `static uint32_t s_state;` with zero collision.
  3. **No linkage:** Block-scope variables, function parameters, struct members, and `typedef` names.
- **Declaration vs Definition & One Definition Rule (ODR):**
  - A **declaration** introduces a name and type without allocating storage (`extern uint32_t g_ticks;`, `void uart_init(void);`) — place in `.h` headers.
  - A **definition** allocates storage or supplies a function body (`uint32_t g_ticks = 0U;`) — place in **exactly one** `.c` file.
- **`-fno-common` (GCC 10+ Default):** Legacy C allowed uninitialized file-scope tentative definitions (`uint32_t g_flag;`) placed in a header included by two `.c` files to silently merge into a single `COMMON` symbol. GCC 10+ defaults to **`-fno-common`**, placing each uninitialized global directly into `.bss` and immediately failing at link time with `multiple definition of 'g_flag'`. This enforces strict ODR and prevents accidental type-mismatched global merging.

**Freestanding vs Hosted C (`C-COMP-004`):**
Bare-metal microcontrollers have no OS kernel, filesystem, or console (`-ffreestanding`). ISO C11 (§4p6) guarantees only **freestanding headers** are always available without an OS runtime: `<stdint.h>`, `<stddef.h>`, `<stdbool.h>`, `<limits.h>`, `<float.h>`, `<stdarg.h>`, `<stdalign.h>`, `<stdnoreturn.h>`, `<iso646.h>`. Hosted libraries (`<stdio.h>` `printf`, `<stdlib.h>` `malloc`) require explicit low-level Newlib syscalls (`_write`, `_sbrk`) and thread-safety locks (see [C-LIB-001](./advanced-c-ub-memory-model.md#6-newlib--newlib-nano-thread-safety--reentrancy-c-lib-001)).

---

### 3.2 Fixed-Width Integers, Two's Complement, Usual Arithmetic Conversions & Overflow (`C-INT-001..004`)

*Prerequisites: `C-COMP-004`, `CS-NUM-001`.*

Bare C types (`int`, `short`, `long`) have implementation-defined widths across architectures. Embedded firmware must use `<stdint.h>` and `<stddef.h>`:
- **Exact-width:** `int8_t`, `uint8_t`, `int16_t`, `uint16_t`, `int32_t`, `uint32_t`, `int64_t`, `uint64_t`.
- **Pointer/Object-sized:** `uintptr_t` / `intptr_t` (for safe integer-pointer round-trips), `size_t` (unsigned byte count / `sizeof` result), `ptrdiff_t` (signed pointer difference).
- **Portable `printf` formatters (`<inttypes.h>`):** Use `PRIu32`, `PRId32`, `PRIx32` (`uint32_t` is `unsigned long` on `arm-none-eabi-gcc` 32-bit EABI, whereas it is `unsigned int` on x86_64 Linux).
- **ARM AAPCS `char` Signedness Trap:** Per ARM AAPCS (§7.1.1), plain `char` is **`unsigned`** by default on ARM (whereas it is `signed` on x86_64). Always write `int8_t` or `uint8_t` for numeric bytes and reserve `char` strictly for ASCII text.

**Two's Complement & `INT_MIN` Asymmetry (`C-INT-002`):**
An $N$-bit signed integer spans $[-2^{N-1},\ +2^{N-1}-1]$. Negation computes bitwise inversion plus 1:
$$\text{Two's Complement}(x) = (\sim x) + 1$$
Example in 8 bits: $+5 = \texttt{00000101}_2 \rightarrow \sim 5 = \texttt{11111010}_2 \rightarrow +1 = \texttt{11111011}_2 = \texttt{0xFB} = -5$.
- **Sign Extension:** Widening `int8_t(-5)` (`0xFB`) to `int32_t` replicates bit 7 (`SXTB` on Cortex-M4) yielding `0xFFFFFFFB`. Widening `uint8_t(251)` (`0xFB`) zero-extends (`UXTB`) yielding `0x000000FB`.
- **`INT_MIN` Asymmetry:** Because `-(-128)` = `+128` overflows `int8_t` (`max = +127`), negating `INT32_MIN` (`-2147483648`) or calling `abs(INT32_MIN)` causes **Signed Integer Overflow Undefined Behavior (UB)**.

**Signed Overflow UB vs Unsigned Modular Wraparound (`C-INT-003`):**
- **Unsigned arithmetic** is strictly defined modulo $2^N$ (ISO C11 §6.2.5p9). `0xFFFFFFFFUL + 1UL == 0UL` is guaranteed and enables wraparound-safe timer elapsed checks: `(uint32_t)(now - start) >= period` works seamlessly across the 49.7-day 32-bit 1 ms rollover.
- **Signed integer overflow** is **Undefined Behavior (UB)** (ISO C11 §6.5p5). At `-O2`, GCC assumes signed overflow *never happens*, so `if (x + 1 < x)` for `int32_t x` is optimized away to constant `false`.

**Integer Promotions & Usual Arithmetic Conversions (`C-INT-004`):**
Per ISO C11 §6.3.1.1 (Integer Promotions), every integer type with conversion rank less than `int` (`bool`, `char`, `uint8_t`, `int8_t`, `uint16_t`, `int16_t`) is **implicitly promoted to signed `int`** (32-bit on Cortex-M4, because `int` can represent all values of `uint16_t`: `0..65535`) before arithmetic or bitwise operators execute. Then ISO C11 §6.3.1.8 (Usual Arithmetic Conversions) balances binary operands: if one operand is `unsigned` with rank $\ge$ the `signed` operand's rank, the signed operand converts to `unsigned`!

This creates **four notorious embedded C traps**:
1. **The `~uint8_t` Trap:**
   `uint8_t a = 0x55U; if (~a == 0xAAU)` is **ALWAYS FALSE!** `a` promotes to 32-bit signed `int` `0x00000055`, so `~a` evaluates to `0xFFFFFFAA` (`-86`), which never equals `0x000000AA`. **Fix:** `if ((uint8_t)(~a) == 0xAAU)`.
2. **The `uint16_t * uint16_t` Signed Overflow UB Trap:**
   On 32-bit ARM, `uint16_t x = 60000U, y = 60000U; uint32_t p = x * y;` promotes **both** `x` and `y` to **signed 32-bit `int`**, computing `3,600,000,000 > INT32_MAX (2,147,483,647)` → **Signed Overflow UB**! **Fix:** `uint32_t p = (uint32_t)x * (uint32_t)y;`.
3. **The Signed/Unsigned Comparison Trap:**
   In `int32_t a = -1; uint32_t b = 1U; if (a < b)`, usual arithmetic conversions convert `a` to `uint32_t` (`0xFFFFFFFFU`), making `-1 < 1U` evaluate **FALSE**! Always compile with `-Wsign-compare -Wconversion`.
4. **Unsuffixed Literal Width Trap:**
   `1 << 31` has signed 32-bit `int` operand `1` (UB!). `1000 * 1000 * 1000 * 10` overflows 32-bit `int` before assignment to `uint64_t`. Always suffix unsigned/wide constants explicitly (`1UL`, `1000ULL`).

---

### 3.3 Bitwise Manipulation & Shift Undefined Behavior (`C-BIT-001..003`)

*Prerequisites: `C-INT-001..004`.*

- **Glitch-Free Multi-Bit Register Update (`C-BIT-001`):**
  Writing `GPIOA->MODER &= ~(3UL << 10U); GPIOA->MODER |= (2UL << 10U);` emits **two** bus stores, momentarily putting `PA5` into Input mode (`00b`) between stores. Always compute in a local variable and execute a **single store**:
  ```c
  uint32_t tmp = GPIOA->MODER;
  tmp = (tmp & ~(3UL << 10U)) | (2UL << 10U);
  GPIOA->MODER = tmp;
  ```
- **Four Bit-Shift UB / Implementation-Defined Traps (`C-BIT-002`):**
  1. **Shifting into the sign bit of a signed `int`:** `(1 << 31)` is **UB** in C99/C11. Always use unsigned literals: **`(1UL << 31U)`**.
  2. **Shift count $\ge$ width or $< 0$:** `x << 32` or `x >> -1` is **UB** (ARM `LSL` returns `0` for count `32`, whereas x86 masks `32 & 31 == 0` returning `x`).
  3. **Left-shifting a negative value:** `(-1) << 2` is **UB** in C11.
  4. **Right-shifting a negative signed integer:** `(-8) >> 1` is **implementation-defined** (GCC on ARM emits arithmetic shift `ASR`, preserving sign; portable code documents this assumption or uses unsigned masks).
- **Portable Sub-Word Sign Extension (`C-BIT-003`):**
  To sign-extend an unsigned 12-bit two's-complement sensor reading `raw` (`0x000..0xFFF`) into `int32_t` with **zero UB**:
  ```c
  static inline int32_t sign_extend_12bit(uint32_t raw)
  {
      uint32_t masked = raw & 0x0FFFUL;
      return (int32_t)(masked ^ 0x0800UL) - 0x0800L;
  }
  ```

---

### 3.4 Pointers, Qualifiers, Arrays, Strings, 2D DMA & FAM vs VLA (`C-PTR-001..004`, `C-ARR-001..004`)

*Prerequisites: `C-INT-001`, `C-STOR-001`.*

1. **Scaled Pointer Arithmetic & One-Past-The-End Rule (`C-PTR-001`):**
   Incrementing `T *p` adds `sizeof(T)` bytes. Subtracting two pointers into the *same* array (`p2 - p1`) yields `ptrdiff_t` in **elements**, not bytes. Forming `&arr[N]` (one past the last element) is valid for loop termination, but dereferencing `&arr[N]`, forming `&arr[-1]`, or comparing pointers to unrelated objects with `<` is **UB**.
2. **Reading Pointer Qualifier Combinations Right-to-Left (`C-PTR-002`):**

   | Declaration | Pointer Mutable? | Pointee Mutable? | Embedded Use Case |
   |---|---|---|---|
   | `uint32_t *p` | Yes | Yes | Normal RAM buffer pointer |
   | `const uint32_t *p` | Yes | **No** | Pointer to Flash `.rodata` LUT |
   | `uint32_t * const p` | **No** | Yes | Fixed-address RAM buffer handle |
   | `const uint32_t * const p` | **No** | **No** | Vector table entry / fixed ROM table |
   | `volatile uint32_t *p` | Yes | Yes (fresh bus I/O) | Read/write MMIO register (`__IO`) |
   | `const volatile uint32_t *p` | Yes | **No** (CPU RO, HW RW!) | Read-only hardware status register (`__I`, e.g., `GPIOA->IDR`) |

3. **`void *`, `uintptr_t` & The Cortex-M `0x00000000` NULL Dereference Trap (`C-PTR-003`):**
   - Pointer arithmetic on `void *` is illegal in ISO C11 (GCC extension only; reject with `-Wpedantic -Wpointer-arith`).
   - **Cortex-M4 Critical Warning:** Unlike Linux/Windows where dereferencing `NULL` (`0x00000000`) immediately causes `SIGSEGV`, on STM32G474RE address `0x00000000` is **aliased to Flash (`0x08000000`)** when `BOOT0 = 0` (RM0440 §2.6). Reading `*(volatile uint32_t *)0` **silently succeeds and returns `_estack` (`0x20018000`)** instead of faulting! Always validate `if (ptr == NULL)` and consider an MPU Region 0 null-guard (`ARM-MPU-001`).
4. **Array-to-Pointer Decay & Its 4 Exceptions (`C-ARR-001`):**
   In expressions and function parameters (`void tx(uint8_t buf[64])`), an array **decays** to a pointer to its first element (`uint8_t *`), so `sizeof(buf)` inside `tx()` is **4 bytes**, NOT 64! The **only 4 exceptions** where an array does NOT decay are: `sizeof(arr)`, `&arr` (yields `T (*)[N]`), `_Alignof(arr)`, and string literal initialization (`char s[] = "OK";`).
5. **Strings & `.rodata` Placement (`C-ARR-002`):**
   - `const char *msg = "READY";` places `"READY\0"` in Flash `.rodata` (`0x0800xxxx`); writing `msg[0] = 'X'` is UB and triggers a Flash BusFault/HardFault.
   - `char buf[] = "READY";` copies 6 bytes onto the stack every time the function runs.
   - Ban unbounded `strcpy`/`sprintf`/`gets`; use `snprintf(buf, sizeof(buf), ...)` and check its return value for truncation (`ret < 0 || (size_t)ret >= sizeof(buf)`).
6. **Multidimensional Row-Major Arrays & Pointer-to-Array DMA Parameters (`C-ARR-003`):**
   C arrays are strictly **row-major**: in `volatile uint16_t dma_buf[32][2]`, element `[1][0]` sits immediately after `[0][1]` (`&dma_buf[1][0] == (volatile uint16_t *)&dma_buf[0][0] + 2`), matching interleaved 2-channel STM32 ADC scan-mode DMA writes (`CH0, CH1, CH0, CH1...`). Passing `dma_buf` to a function decays the outer dimension only, yielding a **pointer-to-array** parameter `const volatile uint16_t (*buf)[2]` (preserving the 2-channel row stride cleanly without manual 1D index flattening; see Section 8.2).
7. **Flexible Array Members (C99 FAM) vs Banned VLAs (`C-ARR-004`):**
   - **C99 FAM (`struct proto_frame { uint16_t len; uint16_t crc16; uint8_t payload[]; };`):** `sizeof(proto_frame_t)` excludes `payload[]` (equals `4` bytes). In bare-metal firmware without `malloc`, overlay a FAM cleanly onto a statically allocated `_Alignas(proto_frame_t) uint8_t storage[sizeof(proto_frame_t) + MAX_PAYLOAD]` buffer (verified with `_Static_assert` in Section 8.2).
   - **Variable-Length Arrays (VLAs)** (`uint8_t tmp[user_len];`) compute stack frame sizes dynamically at runtime, defeating static `-fstack-usage` bounds analysis and causing silent SRAM stack overflow. **Banned by MISRA C:2012/2023 Rule 18.8** — always pass **`-Wvla -Werror`**.

---

### 3.5 Structs, Discriminated Unions, C Bit-Fields, Enums & Opaque `typedef` (`C-TYPE-001..004`)

*Prerequisites: `C-PTR-001`, `C-BIT-001`, `C-ALIGN-001`.*

1. **Structs & C99 Designated Initializers (`C-TYPE-001`):**
   Initialize driver configuration structs explicitly by field name (unspecified fields are guaranteed zero-initialized):
   ```c
   const uart_cfg_t cfg = { .baud = 115200UL, .stop_bits = 1U, .parity = UART_PARITY_NONE };
   ```
2. **Discriminated (Tagged) Unions (`C-TYPE-002`):**
   A `union` overlays all members at byte offset `0`, with `sizeof(union)` equal to the largest member rounded up to the strictest member alignment (`max_align`). In RTOS queues and command frames, **always** wrap a `union` inside a **tagged struct** (`struct { msg_tag_t tag; union { ... } payload; }`) and validate `msg->tag` in a `switch`/`if` before reading the active payload member (Section 8.3). This saves significant SRAM compared to placing all payload structs side by side while preventing inactive-member misinterpretation.
3. **Why C Bit-Fields Are Dangerous for MMIO Registers (`C-TYPE-003`):**
   Mapping STM32 registers with `typedef struct { uint32_t EN : 1; uint32_t MODE : 2; } reg_bf_t;` has **four ISO C11 implementation-defined hazards** (Annex J.3.9) plus a severe hardware bus hazard:
   1. **Bit allocation order** (LSB-first vs MSB-first within a storage unit) is implementation-defined.
   2. **Signedness of plain `int f : N`** is implementation-defined (`int f : 1` may hold only `0` and `-1`).
   3. **Storage-unit straddling** across 32-bit boundaries is implementation-defined.
   4. **Alignment of bit-field containers** depends on the ABI (ARM AAPCS §7.1.7).
   5. **Hardware Bus-Width & RMW Hazard:** Under `arm-none-eabi-gcc -O2`, writing a single `volatile` bit-field member may emit an 8-bit `STRB`, 16-bit `STRH`, or `LDR`+`BFI`+`STR` sequence. On STM32G4 peripherals requiring 32-bit word accesses or `rc_w1`/`rc_w0` direct assignment, this silently corrupts adjacent flags or causes bus errors. **Rule:** Always use CMSIS `_Pos`/`_Msk` shift-and-mask macros (`MODIFY_REG`) for MMIO.
4. **Enumerations (`enum`), `-fshort-enums` & Opaque `typedef` (`C-TYPE-004`):**
   The underlying integer width of an `enum` is implementation-defined (4 bytes by default on ARM EABI; 1 or 2 bytes if `-fshort-enums` is passed — **never link object files compiled with mixed `-fshort-enums` flags**, as struct offsets will silently diverge!). Use opaque pointer typedefs (`typedef struct uart_ctx uart_ctx_t;` in `.h`, full `struct uart_ctx { ... };` hidden inside `.c`) to enforce strict translation-unit encapsulation.

---

### 3.6 Storage Classes, Qualifiers & Preprocessor Hygiene (`C-STOR-001..004`, `C-QUAL-001..004`, `C-PREP-001..004`)

*Prerequisites: `C-COMP-002..003`.*

| Declaration | Scope | Linkage | Storage Duration | STM32G474RE Memory Section |
|---|---|---|---|---|
| `uint32_t g_cnt = 5U;` | File | External | Static (whole run) | `.data` (LMA Flash, VMA SRAM) |
| `uint32_t g_buf[64];` | File | External | Static (zeroed at boot) | `.bss` (VMA SRAM, 0 B Flash) |
| `static uint32_t s_cnt;` | File | **Internal** (TU-private) | Static (zeroed at boot) | `.bss` (symbol `b` in `nm`) |
| `const uint16_t LUT[256] = {...};` | File | External / Internal (`static`) | Static | **`.rodata` (Flash ONLY — 0 B SRAM!)** |
| `static uint32_t calls = 0U;` (inside fn) | Block | None | **Static** (persists across calls!) | `.bss` / `.data` |
| `uint32_t tmp;` (inside fn) | Block | None | Automatic (**indeterminate garbage!**) | Call Stack (`MSP`/`PSP`) |

- **`static` Dual Semantics (`C-STOR-002`):** At **file scope**, `static` restricts linkage to the current `.c` file (preventing namespace collisions). Inside a **function block**, `static` gives a local variable persistent static lifetime across calls — **warning: makes the function non-reentrant across ISRs/RTOS tasks!**
- **`extern` & Linker Symbols Trap (`C-STOR-003`):** Linker-script symbols (`extern uint32_t _sdata;`) have **NO storage value** — only an **address (`&_sdata`)**. Reading `_sdata` directly dereferences the first word of `.data`; always take `&_sdata`.
- **Legacy `auto` / `register` & `.bss` Zero-Init vs Uninitialized Stack Locals (`C-STOR-004`):**
  - `auto` is the redundant default for block-scope automatic variables.
  - `register` hints CPU-register allocation and **forbids taking the object's address (`&x` is a compile-time constraint violation, ISO C11 §6.5.3.2p1)**; modern GCC ignores it for optimization.
  - Static-duration objects without explicit initializers reside in `.bss` and are guaranteed zeroed by `Reset_Handler` before `main()`. Automatic stack locals are **uninitialized** — reading them before writing is **UB** (enforce `-Wuninitialized -Werror`).
- **`const` Flash Conservation (`C-QUAL-001`):** On STM32G474RE (512 KB Flash vs 96 KB SRAM1+SRAM2 + 32 KB CCM-SRAM), forgetting `const` on a 4 KB sine/font table wastes **4 KB of SRAM AND 4 KB of Flash** (`.data` LMA + VMA). Adding `static const` places it strictly in `.rodata` (Flash only, 0 B SRAM).
- **`static inline` vs C99 `inline` (`C-QUAL-003`):** Bare C99 `inline int f(void)` in a header emits **no out-of-line definition**, causing `undefined reference` linker errors at `-O0` unless paired with `extern inline` in one `.c` file. Always use **`static inline`** (CMSIS `__STATIC_INLINE`) in headers.
- **`_Static_assert` (`C-QUAL-004`):** C11 zero-cost compile-time checks (`_Static_assert(sizeof(proto_frame_t) == 4U, "...");`).
- **Preprocessor Hygiene (`C-PREP-001..004`):**
  1. **Parenthesize every parameter AND the whole expression** (`#define MUL(a, b) ((a) * (b))`), or prefer **`static inline` functions** to eliminate double-evaluation bugs (`MAX(ADC1->DR, 100U)` reads a hardware FIFO register twice!).
  2. **Multi-statement macro guard:** Always wrap in `do { ... } while (0)` (no trailing semicolon) so `if (cond) MY_MACRO(); else ...` compiles cleanly.
  3. **Two-Level Stringification, Token Pasting, Variadics & X-Macros (`C-PREP-003`):** Because `#` and `##` inhibit macro expansion of their direct arguments, use a two-level indirection (`#define STR_IMPL(x) #x` / `#define STR(x) STR_IMPL(x)`) so `STR(__LINE__)` expands to `"42"` instead of `"__LINE__"`. Use `##` (`#define GPIO_PORT(p) GPIO##p`) for peripheral generation, variadic macros (`#define DEBUG_LOG(fmt, ...) ((void)0)`) for zero-cost release builds, and **X-Macros** (`FAULT_LIST(X)`) to keep enum values and Flash string tables 100% synchronized from a single source of truth (Section 8.3).
  4. **Include guards:** Use `#ifndef UART_DRV_H` / `#define UART_DRV_H` ... `#endif` — **never** start user include guards with `_` + uppercase or `__` (reserved for the implementation by ISO C11 §7.1.3).

---

### 3.7 Function Pointers, Object Lifetime, Reentrancy, AAPCS, Determinism & GCC Optimization (`C-FPTR-001..003`, `C-MEM-001..003`, `C-FUNC-001..003`, `C-OPT-001..003`, `CS-ALGO-001`)

*Prerequisites: `C-PTR-001`, `C-STOR-001..002`, `CS-DATA-001`.*

1. **Function Pointers & Cortex-M Thumb Bit-0 Rule (`C-FPTR-001`):**
   Always alias with `typedef`: `typedef void (*exti_cb_t)(void *user_ctx);`. On ARM Cortex-M4, every valid function address has **LSB bit 0 = `1`** (Thumb state for `BLX`/`BX`; jumping to an even address triggers UsageFault `INVSTATE`). Casting function pointers to object pointers `(uint32_t *)` violates ISO C11 §6.3.2.3p8.
2. **Callbacks (`void *ctx`) & Bounds-Checked `.rodata` FSM Jump Tables (`C-FPTR-002`, `C-FPTR-003`):**
   - Pass an opaque `void *user_ctx` pointer alongside every driver callback so reusable drivers require zero mutable globals; update the `{ fn, ctx }` pair atomically inside a `PRIMASK` critical section and **always check `if (cb != NULL)`** before calling from an ISR (Section 8.4).
   - Store FSM handler tables in Flash `.rodata` (`static const btn_state_fn_t g_btn_fsm_table[BTN_STATE_COUNT]`) and **always bounds-check `if (fsm->state < BTN_STATE_COUNT)`** before indirect dispatch (Section 8.4).
3. **Why `malloc`/`free` Is Banned (`C-MEM-002`):**
   Dynamic allocation suffers from **external heap fragmentation** (e.g., on a 512 B heap, allocating eight 64 B blocks and freeing every odd block leaves 256 B free, yet a single 128 B `malloc(128)` returns `NULL`!), unbounded $O(N)$ search latency, silent heap-stack collisions, and Newlib lock hazards (banned by MISRA C:2023 Rule 21.3). Use caller-owned static buffers or $O(1)$ fixed-block memory pools (see [Stage 03 — FW-PAT-005](../03-embedded-c/README.md#85-hsm-o1-memory-pool-observer--command-patterns-fw-pat-001-005-007-fw-fsm-002)).
4. **Object Lifetime, Pointer Provenance & Dangling Pointers (`C-MEM-003`):**
   An automatic object's lifetime ends immediately when its enclosing block exits (ISO C11 §6.2.4p2). Returning `&local_var` creates a **dangling pointer**: the value may momentarily appear intact in single-threaded tests, only to be **silently clobbered by the very next function call or Cortex-M4 hardware ISR exception stacking** (which pushes `R0–R3, R12, LR, PC, xPSR` — 32 bytes — onto the dead stack slot!). Furthermore, under ISO C11 pointer provenance rules, a pointer only retains validity for the exact object allocation from which it was derived — reusing a freed or out-of-scope pointer is UB even if the numeric address happens to match a new object. Enable `-Wreturn-local-addr -Werror` and always pass caller-owned output buffers `(char *out, size_t cap)`.
5. **Reentrancy, Iterative Recursion Elimination & ARM AAPCS Call Overhead (`C-FUNC-001..003`):**
   - **Reentrancy (`C-FUNC-001`):** A function is reentrant iff it uses *only* automatic stack locals, `const` `.rodata`, or caller-owned context structs, modifies no unprotected `static`/global state, and calls only reentrant callees. Refactoring `static char fmt_buf[64]` into `format_u32_reentrant(char *out_buf, size_t out_cap, uint32_t value)` (Section 8.5) eliminates ISR/task preemption corruption.
   - **Recursion Ban & `-fstack-usage` (`C-FUNC-002`):** Recursion causes unbounded stack growth (banned by MISRA Rule 17.2). Convert recursive algorithms (e.g., binary search, CRC, tree traversal) into iterative loops with $O(1)$ stack frames and audit every function with **`arm-none-eabi-gcc -fstack-usage`**. Compile-verified `.su` measurements on Section 8.5 confirm `bsearch_u16_iterative` uses exactly **`4 static` bytes** of stack with zero `dynamic` or unbounded entries!
   - **AAPCS Register Passing & Call Overhead (`C-FUNC-003`):** Under ARM AAPCS (IHI 0042), the first **4 integer arguments** pass in `R0–R3` (and up to 16 `float` arguments in `S0–S15` under `-mfloat-abi=hard`), return values pass in `R0` (`R0:R1` for 64-bit), and `SP` must remain **8-byte aligned** at public call boundaries. Arguments 5+ and large structs passed by value spill to the stack: verified via `-fstack-usage`, `sum_by_const_ptr(const big16_t *p)` uses **`0 static` bytes**, whereas `sum_6_args(a,b,c,d,e,f)` incurs **`8 static` bytes** of stack frame overhead! Use `const T *` for structs $> 8$ bytes, and weigh `static inline` (eliminates `BL`/`BX` overhead for tiny 1–5 line helpers) against Flash code-size bloat on large functions.
6. **Deterministic Time & Space Complexity vs WCET (`CS-ALGO-001`):**
   In hard real-time embedded systems, **Worst-Case Execution Time (WCET)** — not average-case Big-O — determines whether deadlines hold. A hash table with amortized $O(1)$ average lookup can suffer $O(N)$ worst-case collisions, violating a 10 µs control-loop budget. By contrast, a 32-priority ready bitmap decoded via Cortex-M4's single-cycle **`CLZ`** instruction (`31U - __CLZ(bitmap)`, Section 8.1) executes in **strictly constant $O(1)$ time** regardless of priority count, whereas a linear scan is $O(N)$ (up to 32 loop iterations). Measure exact cycle bounds on STM32G474RE using `DWT->CYCCNT` (`CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk`, `DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk`).
7. **GCC Optimization Levels & Linker Stripping (`C-OPT-001..003`):**
   - `-O0`: Zero optimization; huge Flash, slow, hides UB/missing-`volatile` bugs.
   - `-O1`: Basic dead-code elimination and register allocation without aggressive aliasing/inlining.
   - `-Og`: Debug-friendly optimization (recommended interactive GDB default).
   - `-Os`: Optimize for minimal Flash code size (disables code-expanding loop unrolling).
   - `-O2` / `-O3`: Aggressive speed optimizations (inlining, `-fstrict-aliasing`, loop vectorization/unrolling). **Bare-Metal Startup Warning:** At `-O2`/`-O3`, GCC's `-ftree-loop-distribute-patterns` can replace a hand-written `.bss` zero-loop inside `Reset_Handler` with a call to `memset()`, causing infinite recursion or missing-symbol errors under `-nostdlib`! Mitigate with `-fno-tree-loop-distribute-patterns` or `__attribute__((optimize("no-tree-loop-distribute-patterns")))`.
   - `-ffunction-sections -fdata-sections -Wl,--gc-sections`: Places each symbol in its own ELF section so `ld` strips unreferenced code/data (requires `KEEP(*(.isr_vector))` in `.ld` so the vector table is preserved!).
   - `-flto`: Whole-program Link-Time Optimization across `.c` TUs (annotate weak ISRs and `.isr_vector` with `__attribute__((used))` so LTO never eliminates them).

> **Continue to the Companion Deep-Dive:** Read [advanced-c-ub-memory-model.md](./advanced-c-ub-memory-model.md) for complete coverage of **Alignment, Padding, `__attribute__((packed))` UsageFaults, Strict Aliasing, `memcpy` Punning, UB Taxonomy, C11 `sequenced-before`, `<stdatomic.h>` Memory Orders, Q15/Q31 Fixed-Point, IEEE-754 `double` Literal Traps, and Newlib `_reent`/`__malloc_lock` (`C-LIB-001`)**.

---

## 4. Important Terminology

| Term | Engineering Definition |
|---|---|
| **Translation Unit (TU)** | A single `.c` source file after all `#include` expansions and macro substitutions (`cpp` output), compiled independently into one `.o` object file. |
| **Integer Promotion** | Implicit widening of integer types narrower than `int` (`uint8_t`, `int16_t`) to signed `int` before arithmetic or bitwise operations (ISO C11 §6.3.1.1). |
| **Undefined Behavior (UB)** | Program construct for which ISO C imposes zero requirements; optimizing compilers assume UB never occurs and may delete safety checks or emit broken code. |
| **Array Decay** | Automatic conversion of an array expression (`T[N]`) into a pointer to its first element (`T *`), losing array length information (`sizeof` yields pointer size). |
| **Internal Linkage** | Symbol visibility restricted to a single translation unit via file-scope `static`, preventing cross-file symbol collisions. |
| **Reentrancy** | Property of a function allowing it to be safely interrupted mid-execution (by an ISR or RTOS task) and concurrently re-entered without state corruption. |
| **Flexible Array Member (FAM)** | A C99 unsized trailing array `uint8_t payload[];` at the end of a `struct` for clean variable-length packet framing over statically aligned buffers. |
| **WCET (Worst-Case Execution Time)** | The strict upper bound on CPU cycles required by a routine across all possible valid inputs, governing real-time deadline schedulability. |

---

## 5. Register-Level Explanations

Verified directly against `core_cm4.h` and `stm32g474xx.h`:
- **`SCB->CCR` (`0xE000ED14`, `core_cm4.h`):**
  - Bit 3 — **`UNALIGN_TRP`** (`SCB_CCR_UNALIGN_TRP_Msk`): When `1`, *any* unaligned halfword/word access immediately triggers a `UsageFault` (`CFSR.UNALIGNED = 1`). Even when `UNALIGN_TRP == 0`, unaligned `LDRD`/`STRD`/`LDM`/`STM`/`LDREX`/`STREX`/`VLDR`/`VSTR` **always** fault!
  - Bit 4 — **`DIV_0_TRP`** (`SCB_CCR_DIV_0_TRP_Msk`): When `1`, integer division by zero triggers a `UsageFault` (`CFSR.DIVBYZERO = 1`) instead of silently returning `0`.
- **`CoreDebug->DEMCR` (`0xE000EDFC`) & `DWT->CTRL` / `DWT->CYCCNT` (`0xE0001000` / `0xE0001004`, `core_cm4.h`):**
  - Enabling `CoreDebug_DEMCR_TRCENA_Msk` (bit 24) and `DWT_CTRL_CYCCNTENA_Msk` (bit 0) activates the free-running 32-bit hardware cycle counter for exact WCET benchmarking (`CS-ALGO-001`).
- **`GPIOA->MODER` (`0x48000000`, `stm32g474xx.h`):** Demonstrates why C bit-fields are unsafe for MMIO and why single-store shadow RMW (`uint32_t tmp = GPIOA->MODER; ... GPIOA->MODER = tmp;`) prevents transient mode glitches.

---

## 6. Relevant Formulas & Calculations

### 6.1 Signed & Unsigned $N$-Bit Integer Ranges
$$\text{Unsigned } N\text{-bit}: \left[0,\ 2^N - 1\right], \qquad \text{Signed Two's Complement } N\text{-bit}: \left[-2^{N-1},\ 2^{N-1} - 1\right]$$

### 6.2 Two's Complement Negation & Wraparound-Safe Elapsed Time
$$-x \equiv (\sim x) + 1 \pmod{2^N}, \qquad \Delta t = \left(t_{\text{now}} - t_{\text{start}}\right) \bmod 2^{32}$$

**Worked Example:** `t_start = 0xFFFFFFF0UL`, `t_now = 0x00000010UL`:
$$\Delta t = (0\text{x}00000010 - 0\text{xFFFFFFF0}) \bmod 2^{32} = 0\text{x}00000020 = 32\text{ ticks (exact!)}$$

### 6.3 Struct Alignment & Padding Offset Formula
For member $i$ with alignment requirement $A_i = \text{alignof}(T_i)$ (`_Alignof(T_i)` in C11):
$$\text{offset}_i = \left\lceil \frac{\text{offset}_{\text{curr}}}{A_i} \right\rceil \cdot A_i, \qquad \text{sizeof}(S) = \left\lceil \frac{\text{offset}_{\text{end}}}{\max_i A_i} \right\rceil \cdot \max_i A_i$$

---

## 7. Official Documentation References

- **ISO/IEC 9899:2011 (C11 Standard):** §4p6 (Freestanding), §5.1.1.2 (8 Translation Phases), §6.2.2 (Linkage), §6.2.4 (Storage Durations & Lifetime), §6.2.5/§6.3.1 (Integer Promotions & Usual Arithmetic Conversions), §6.5.7 (Bitwise Shifts), §6.7.2.1 (Structs, Unions, Bit-Fields, FAMs), §6.7.3 (Type Qualifiers), §6.7.10 (`_Static_assert`), §6.10 (Preprocessing), Annex J.
- **ARM IHI 0042 (AAPCS):** *Procedure Call Standard for the Arm Architecture* (§5 Alignment & `R0–R3` register passing, §7.1.1 unsigned `char`, §7.1.3 `enum` sizes, §7.1.7 bit-fields).
- **GNU GCC Manual:** §3.8 (Warning Options: `-Wconversion`, `-Wdouble-promotion`, `-Wvla`, `-Wshadow`, `-Wreturn-local-addr`), §3.11 (Optimization Options), §3.18 (`-fno-common`, `-fstack-usage`).
- **MISRA C:2012 / C:2023** & **SEI CERT C Coding Standard**.

---

## 8. Verified Code Examples

> **Verification Status:** All snippets below are **Compile-Verified (`arm-none-eabi-gcc 14.3.1`, `-std=c11 -mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard -DSTM32G474xx -O2 -Wall -Wextra -Werror -Wdouble-promotion -Wvla`)** against vendored `stm32g474xx.h` & `core_cm4.h` (**Not Tested on Physical Hardware**; host unit tests in [tests/test_c_foundations.c](../tests/test_c_foundations.c)).

### 8.1 $O(1)$ Priority Bitmap (`__CLZ`), DWT Cycle Timer, Safe Promotions & Bitfield Helpers (`CS-ALGO-001`, `C-INT-004`, `C-BIT-001..003`)

```c
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "stm32g474xx.h"

/* Compile-time safe ARRAY_LEN: fails to compile if passed a decayed pointer (GCC/Clang) */
#define ARRAY_LEN(arr) \
    (sizeof(arr) / sizeof((arr)[0]) + \
     0U * sizeof(char[1 - 2 * __builtin_types_compatible_p(__typeof__(arr), __typeof__(&(arr)[0]))]))

/* Multi-statement macro wrapped safely in do { ... } while (0) */
#define REG_MODIFY_32(reg, clear_mask, set_mask)                    \
    do {                                                            \
        uint32_t _reg_tmp = (reg);                                  \
        _reg_tmp = (_reg_tmp & ~(uint32_t)(clear_mask)) |           \
                   ((uint32_t)(set_mask) & (uint32_t)(clear_mask)); \
        (reg) = _reg_tmp;                                           \
    } while (0)

/* CS-ALGO-001: Cortex-M4 DWT cycle counter for WCET benchmarking */
static inline void dwt_cycle_counter_init(void)
{
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CYCCNT = 0UL;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
}

static inline uint32_t dwt_cycles_now(void)
{
    return DWT->CYCCNT;
}

/* CS-ALGO-001: O(1) deterministic highest-priority ready bit via single-cycle CLZ */
static inline int32_t ready_highest_o1(uint32_t ready_bitmap)
{
    if (ready_bitmap == 0UL) {
        return -1;
    }
    return (int32_t)(31U - (uint32_t)__CLZ(ready_bitmap));
}

/* CS-ALGO-001: O(N) linear scan baseline (up to 32 loop iterations) */
static inline int32_t ready_highest_on(uint32_t ready_bitmap)
{
    for (int32_t bit = 31; bit >= 0; --bit) {
        if ((ready_bitmap & (1UL << (uint32_t)bit)) != 0UL) {
            return bit;
        }
    }
    return -1;
}

/* C-INT-004: Promotion-safe bitwise inversion & uint16_t multiply without signed-int UB */
static inline bool u8_bitwise_not_matches(uint8_t val, uint8_t expected)
{
    return ((uint8_t)(~val) == expected);
}

static inline uint32_t u16_mul_no_ub(uint16_t a, uint16_t b)
{
    return (uint32_t)a * (uint32_t)b;
}

/* C-BIT-003: Portable UB-free 12-bit two's-complement sign extension */
static inline int32_t sign_extend_12(uint32_t raw_12b)
{
    uint32_t val = raw_12b & 0x0FFFUL;
    return (int32_t)(val ^ 0x0800UL) - 0x0800L;
}

/* C-INT-003: Wraparound-safe 32-bit unsigned elapsed check */
static inline bool timeout_expired(uint32_t now, uint32_t start, uint32_t period)
{
    return ((uint32_t)(now - start) >= period);
}
```

### 8.2 Row-Major 2D DMA Buffer & Static-Aligned C99 FAM Framing (`C-ARR-003`, `C-ARR-004`)

```c
#include <stdalign.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

/* C-ARR-003: Interleaved 2-channel ADC DMA buffer (row-major: [frame][channel]) */
#define ADC_DMA_FRAMES 32U
#define ADC_DMA_CHANS   2U

static volatile uint16_t g_adc_dma_buf[ADC_DMA_FRAMES][ADC_DMA_CHANS];

void process_dual_adc_frames(const volatile uint16_t (*buf)[ADC_DMA_CHANS],
                             size_t frame_count,
                             uint32_t *out_avg_ch0,
                             uint32_t *out_avg_ch1)
{
    uint32_t sum0 = 0UL;
    uint32_t sum1 = 0UL;
    if ((buf == NULL) || (frame_count == 0U)) {
        return;
    }
    for (size_t i = 0U; i < frame_count; ++i) {
        sum0 += buf[i][0];
        sum1 += buf[i][1];
    }
    if (out_avg_ch0 != NULL) { *out_avg_ch0 = sum0 / (uint32_t)frame_count; }
    if (out_avg_ch1 != NULL) { *out_avg_ch1 = sum1 / (uint32_t)frame_count; }
}

/* C-ARR-004: C99 Flexible Array Member over static aligned storage (0 malloc, 0 VLA) */
typedef struct {
    uint16_t len;
    uint16_t crc16;
    uint8_t  payload[]; /* Zero contribution to sizeof(proto_frame_t) */
} proto_frame_t;

_Static_assert(sizeof(proto_frame_t) == 4U, "FAM must not add to header size");
_Static_assert(offsetof(proto_frame_t, payload) == 4U, "Payload must follow crc16");

#define PROTO_MAX_PAYLOAD 64U
static _Alignas(proto_frame_t) uint8_t g_frame_storage[sizeof(proto_frame_t) + PROTO_MAX_PAYLOAD];

proto_frame_t *proto_frame_init_static(const uint8_t *src, uint16_t len, uint16_t crc16)
{
    if ((src == NULL) || (len > PROTO_MAX_PAYLOAD)) {
        return NULL;
    }
    proto_frame_t *hdr = (proto_frame_t *)g_frame_storage;
    hdr->len   = len;
    hdr->crc16 = crc16;
    memcpy(hdr->payload, src, len);
    return hdr;
}
```

### 8.3 Discriminated (Tagged) Union & X-Macro / Two-Level Preprocessor Hygiene (`C-TYPE-002`, `C-PREP-003`)

```c
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include "stm32g474xx.h"

/* C-TYPE-002: Tagged Union for RTOS/superloop telemetry messages */
typedef enum {
    MSG_TAG_TEMP = 0,
    MSG_TAG_CAN  = 1,
    MSG_TAG_FAULT = 2
} msg_tag_t;

typedef struct {
    msg_tag_t tag;
    union {
        struct { int32_t milli_deg_c; uint32_t timestamp_ms; } temp;
        struct { uint32_t std_id; uint8_t dlc; uint8_t data[8]; } can;
        struct { uint16_t code; uint16_t line; } fault;
    } payload;
} telemetry_msg_t;

_Static_assert(sizeof(telemetry_msg_t) >= (sizeof(msg_tag_t) + 12U),
               "Tagged union must hold largest payload");

int32_t telemetry_extract_temp(const telemetry_msg_t *msg, int32_t *out_mc)
{
    if ((msg == NULL) || (out_mc == NULL)) { return -1; }
    if (msg->tag != MSG_TAG_TEMP)          { return -2; } /* Guard against inactive member */
    *out_mc = msg->payload.temp.milli_deg_c;
    return 0;
}

/* C-PREP-003: Two-level stringification, token pasting, variadic macro & X-Macro table */
#define STR_IMPL(x) #x
#define STR(x)      STR_IMPL(x)
#define GPIO_PORT(letter) GPIO##letter

#ifndef FW_DEBUG_LOG_ENABLE
#define FW_DEBUG_LOG_ENABLE 0
#endif

#if FW_DEBUG_LOG_ENABLE
#define DEBUG_LOG(fmt, ...) ((void)printf("[L" STR(__LINE__) "] " fmt "\n", ##__VA_ARGS__))
#else
#define DEBUG_LOG(fmt, ...) ((void)0)
#endif

#define FAULT_LIST(X)             \
    X(FAULT_NONE,       0U, "OK") \
    X(FAULT_OVERVOLT,   1U, "OV") \
    X(FAULT_OVERCURR,   2U, "OC") \
    X(FAULT_OVERTEMP,   3U, "OT")

typedef enum {
#define X_ENUM(name, code, str) name = (code),
    FAULT_LIST(X_ENUM)
#undef X_ENUM
    FAULT_COUNT
} fault_code_t;

static const char *const g_fault_names[FAULT_COUNT] = {
#define X_NAME(name, code, str) [name] = (str),
    FAULT_LIST(X_NAME)
#undef X_NAME
};

const char *fault_code_str(fault_code_t c)
{
    GPIO_TypeDef *pa = GPIO_PORT(A);
    (void)pa;
    DEBUG_LOG("fault=%u", (unsigned)c);
    return (c < FAULT_COUNT) ? g_fault_names[c] : "UNKNOWN";
}
```

### 8.4 Callback Registration (`void *ctx`) & Flash `.rodata` Table-Driven FSM (`C-FPTR-002`, `C-FPTR-003`)

```c
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "stm32g474xx.h"

/* C-FPTR-002: Context-preserving callback registration with PRIMASK atomic publish */
typedef void (*exti_cb_t)(void *user_ctx);

typedef struct {
    exti_cb_t fn;
    void     *ctx;
} exti_slot_t;

static exti_slot_t g_exti_slots[16];

bool exti_register_callback(uint8_t line, exti_cb_t cb, void *user_ctx)
{
    if (line >= 16U) { return false; }
    uint32_t primask = __get_PRIMASK();
    __disable_irq();
    __asm__ volatile ("" ::: "memory");
    g_exti_slots[line].fn  = cb;
    g_exti_slots[line].ctx = user_ctx;
    __asm__ volatile ("" ::: "memory");
    __set_PRIMASK(primask);
    return true;
}

void exti_dispatch_line(uint8_t line)
{
    if (line < 16U) {
        exti_cb_t cb = g_exti_slots[line].fn;
        void *ctx    = g_exti_slots[line].ctx;
        if (cb != NULL) { cb(ctx); }
    }
}

/* C-FPTR-003: Bounds-checked Flash .rodata Table-Driven Debounce FSM */
typedef enum {
    BTN_RELEASED = 0,
    BTN_DEBOUNCE_PRESS,
    BTN_PRESSED,
    BTN_DEBOUNCE_RELEASE,
    BTN_STATE_COUNT
} btn_state_t;

typedef struct {
    btn_state_t state;
    uint8_t     ticks;
    uint32_t    press_count;
} btn_fsm_t;

typedef void (*btn_state_fn_t)(btn_fsm_t *fsm, bool raw_pressed);

static void st_released(btn_fsm_t *f, bool raw)    { if (raw)  { f->ticks = 1U; f->state = BTN_DEBOUNCE_PRESS; } }
static void st_deb_press(btn_fsm_t *f, bool raw)   { if (!raw) { f->state = BTN_RELEASED; } else if (++f->ticks >= 10U) { f->press_count++; f->state = BTN_PRESSED; } }
static void st_pressed(btn_fsm_t *f, bool raw)     { if (!raw) { f->ticks = 1U; f->state = BTN_DEBOUNCE_RELEASE; } }
static void st_deb_release(btn_fsm_t *f, bool raw) { if (raw)  { f->state = BTN_PRESSED; } else if (++f->ticks >= 10U) { f->state = BTN_RELEASED; } }

static const btn_state_fn_t g_btn_fsm_table[BTN_STATE_COUNT] = {
    [BTN_RELEASED]         = st_released,
    [BTN_DEBOUNCE_PRESS]   = st_deb_press,
    [BTN_PRESSED]          = st_pressed,
    [BTN_DEBOUNCE_RELEASE] = st_deb_release,
};

void btn_fsm_step(btn_fsm_t *fsm, bool raw_pressed)
{
    if ((fsm != NULL) && (fsm->state < BTN_STATE_COUNT)) {
        g_btn_fsm_table[fsm->state](fsm, raw_pressed);
    }
}
```

### 8.5 Reentrant Formatter, Iterative $O(1)$-Stack Search & AAPCS Call-Overhead Comparison (`C-FUNC-001..003`)

```c
#include <inttypes.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

/* C-FUNC-001: Strictly reentrant formatter (caller-owned output buffer, zero static locals) */
int format_u32_reentrant(char *out_buf, size_t out_cap, uint32_t value)
{
    if ((out_buf == NULL) || (out_cap == 0U)) { return -1; }
    return snprintf(out_buf, out_cap, "%" PRIu32, value);
}

/* C-FUNC-002: Iterative binary search (verified 4 B static stack via -fstack-usage!) */
int32_t bsearch_u16_iterative(const uint16_t *sorted, size_t n, uint16_t key)
{
    size_t lo = 0U, hi = n;
    while (lo < hi) {
        size_t mid = lo + ((hi - lo) >> 1U);
        if (sorted[mid] == key) { return (int32_t)mid; }
        if (sorted[mid] < key)  { lo = mid + 1U; }
        else                    { hi = mid; }
    }
    return -1;
}

/* C-FUNC-003: AAPCS comparison: 1 pointer in R0 (0 B stack) vs 6 args (8 B stack spill) */
typedef struct { uint32_t a, b, c, d; } big16_t;

uint32_t sum_by_const_ptr(const big16_t *p)
{
    return p->a + p->b + p->c + p->d;
}

uint32_t sum_6_args(uint32_t a, uint32_t b, uint32_t c, uint32_t d, uint32_t e, uint32_t f)
{
    return a + b + c + d + e + f;
}
```

---

## 9. Practical STM32G474RE Experiments

1. **Run the Mandatory Stage 02 C Verification Harness (`tests/test_c_foundations.c`):**
   - Compile and run [../tests/test_c_foundations.c](../tests/test_c_foundations.c) natively on your host PC with hardened flags:
     ```bash
     gcc -std=c11 -Wall -Wextra -Werror -Wconversion -Wsign-compare -Wshadow -Wvla -Wpedantic \
         -fsanitize=address,undefined tests/test_c_foundations.c -o build/test_c_foundations && ./build/test_c_foundations
     ```
2. **Inspect GCC Pipeline Artifacts (`-save-temps` & `-fstack-usage`):**
   - Compile a driver snippet with `arm-none-eabi-gcc -std=c11 -mcpu=cortex-m4 -mthumb -O2 -save-temps -fstack-usage -c` and inspect `.i` (two-level macro & X-Macro expansion), `.s` (Thumb-2 `SXTB`/`UXTB`/`CLZ`/`REV`), and `.su` (confirming `0..16 static` bytes and zero `dynamic` stack frames).
3. **`.rodata` vs `.data` SRAM Audit:**
   - Declare a 4 KB sine table with and without `static const`; run `arm-none-eabi-size` to verify 4,096 bytes move out of `.data` (SRAM+Flash) into `.rodata` (Flash-only).
   - **Debugging Checkpoint:** In GDB on NUCLEO-G474RE, inspect `x/4bx &val` for `uint32_t val = 0x12345678UL` (Little-Endian `78 56 34 12`) and `x/1wx 0x00000000` (returns `_estack`, proving the Cortex-M NULL-read silent alias trap).

---

## 10. Common Mistakes & Anti-Patterns

1. **Writing `1 << 31` instead of `1UL << 31U`:** Signed 32-bit shift into the sign bit is Undefined Behavior in ISO C11.
2. **Comparing `~u8_var == 0xAAU` or multiplying `uint16_t * uint16_t` uncast:** Integer promotion widens narrow unsigned types to signed `int` first! Cast `(uint8_t)(~u8_var)` and `(uint32_t)a * (uint32_t)b`.
3. **Using `sizeof(buf)` inside a function receiving `uint8_t buf[64]`:** Array decay makes `sizeof(buf) == 4` on 32-bit ARM. Always pass `(uint8_t *buf, size_t len)`.
4. **Mapping STM32 registers with C bit-fields:** Compiler may emit 8-bit `STRB` or non-atomic RMW sequences, breaking 32-bit-only and `rc_w1`/`rc_w0` registers.
5. **Dereferencing linker symbols directly (`uint32_t x = _sdata;`):** Linker symbols have addresses (`&_sdata`), not values.
6. **Using `static` local variables inside functions called by ISRs:** Silently breaks reentrancy and corrupts state under preemption.
7. **Using VLAs (`uint8_t buf[n];`) or recursion on MCU stacks:** Causes unbounded SRAM overflow into `.bss`/`.data`. Enforce `-Wvla -Werror` and `-fstack-usage`.
8. **Returning `&local_var` from a function:** Dangling stack pointer silently overwritten by the next call or Cortex-M4 hardware ISR stacking.
9. **Forgetting `const` on lookup tables and string pointers:** Wastes scarce SRAM and duplicates data across `.data` LMA + VMA.

---

## 11. Hands-On Exercises

1. **$O(1)$ vs $O(N)$ WCET Benchmark (`CS-ALGO-001`):** Benchmark `ready_highest_o1(0x00000001UL)` (`__CLZ`) against `ready_highest_on(0x00000001UL)` using `DWT->CYCCNT` at `-O2` and compare worst-case cycle counts.
2. **Struct Padding Optimization (`C-ALIGN-001`):** Given `struct Bad { uint8_t a; uint32_t b; uint8_t c; uint16_t d; uint64_t e; };`, compute its exact `sizeof` and member offsets on ARM AAPCS (8-byte `uint64_t` alignment), then reorder its fields to minimize `sizeof` without `__attribute__((packed))`, proving offsets with `_Static_assert(offsetof(...))`.
3. **Multi-TU Linkage Lab (`C-COMP-002..003`):** Split a button counter into 3 files (`counter.h`, `counter.c`, `main.c`), intentionally trigger a `-fno-common` duplicate-symbol error, then fix it using `extern` in `.h`, one definition in `.c`, and `static` private helpers verified via `arm-none-eabi-nm`.
4. **Tagged Union + FAM Packet Parser (`C-TYPE-002`, `C-ARR-004`):** Combine `proto_frame_t` and `telemetry_msg_t` into a zero-`malloc` static parser verified under `-Wvla -Werror -Wconversion`.

---

## 12. Assessment Questions

1. **(`C-INT-004`)** Explain what `uint8_t a = 0xFFU; if (~a == 0x00U)` evaluates to on ARM Cortex-M4, and why `uint16_t x = 60000U, y = 60000U; uint32_t p = x * y;` invokes signed overflow UB. Trace the exact ISO C11 §6.3.1.1 integer promotion steps for both.
2. **(`C-STOR-002`, `C-FUNC-001`)** Contrast `static` at file scope vs inside a function body, and explain how a block-scope `static char buf[64]` breaks reentrancy when shared between `main()` and an ISR.
3. **(`C-ARR-001`, `C-ARR-003`)** Why does `void process(uint8_t frame[128])` evaluate `sizeof(frame) == 4` on STM32G474RE? List the 4 contexts where an array does **not** decay, and write the exact parameter signature to accept `volatile uint16_t adc_buf[32][2]` without flattening it to 1D.
4. **(`C-INT-003`, `C-MEM-003`)** Explain why GCC `-O2` may legally delete `if (x + 1 < x)` for `int32_t x`, whereas `(uint32_t)(now - start) >= 1000U` is guaranteed safe across rollover; then explain why returning `&local_var` is clobbered even if no other function is called (Cortex-M4 ISR stacking).
5. **(`C-PTR-002`, `C-PTR-003`)** Why is `const volatile uint32_t *` not a contradiction in bare-metal Embedded C? What happens on STM32G474RE (`BOOT0 = 0`) if firmware reads `*(volatile uint32_t *)0`?
6. **(`C-TYPE-003`, `C-COMP-003`)** State the 4 implementation-defined C bit-field properties plus the ARM bus-width hazard that make bit-fields unsafe for STM32 MMIO registers; then explain what GCC's `-fno-common` default catches across multiple translation units.
7. **(`C-FUNC-002..003`, `CS-ALGO-001`)** Why does ARM AAPCS incur extra stack usage when a function takes 6 integer arguments instead of 4 or receives a 16-byte struct by value? Contrast average-case Big-O with hard real-time WCET using `ready_highest_o1()` (`__CLZ`) vs linear scan.
8. **(`C-OPT-001..003`)** Why can `-O2` break a bare-metal `-nostdlib` `Reset_Handler` `.bss` zeroing loop via `-ftree-loop-distribute-patterns`, and why must `.isr_vector` be annotated with `KEEP()` / `__attribute__((used))` under `--gc-sections` and `-flto`?

---

## 13. Measurable Completion Criteria

- [ ] **Automated C Test Suite Passed:** [tests/test_c_foundations.c](../tests/test_c_foundations.c) compiles with zero warnings under `-std=c11 -Wall -Wextra -Werror -Wconversion -Wshadow -Wvla -Wpedantic -fsanitize=address,undefined` and passes 100% of assertions.
- [ ] **Zero Shift/Promotion/UB Bugs:** Accurately identify and repair integer promotion, signed overflow, shift UB, array decay, dangling-pointer, and macro double-evaluation defects.
- [ ] **Memory, Stack & Section Mapping Verified:** Predict exact ELF sections (`.text`, `.rodata`, `.data`, `.bss`, stack), struct `sizeof`/`offsetof` layouts, and bounded `.su` stack frames via `arm-none-eabi-nm`, `arm-none-eabi-size`, and `-fstack-usage`.
- [ ] **Companion Deep-Dive Completed:** Complete all concepts and assessments in [advanced-c-ub-memory-model.md](./advanced-c-ub-memory-model.md) (`C-ALIGN-*`, `C-UB-*`, `C-SEQ-*`, `C-ATOM-*`, `C-MATH-*`, `C-LIB-001`).

---

> **Sequential Navigation (Step 2 of 17):**  
> **Prev:** [Stage 01b — Digital Electronics & HW Integration (`01b-*`)](../01b-digital-electronics-and-hw-integration/README.md) | **Home:** [`START_HERE.md`](../START_HERE.md) | **Progress:** [`LEARNER_PROGRESS_DASHBOARD.md`](../LEARNERS/LEARNER_PROGRESS_DASHBOARD.md) | **Next:** [Stage 03 — Embedded C & MMIO (`03-embedded-c/`)](../03-embedded-c/README.md)
