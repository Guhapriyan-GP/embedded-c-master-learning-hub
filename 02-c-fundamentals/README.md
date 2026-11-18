# Stage 02: C Language Foundations for Bare-Metal Embedded Systems

> **Curriculum Track:** Level 1–3 Foundation (ISO C11 / `arm-none-eabi-gcc -std=gnu11` on Cortex-M4F)  
> **Companion Deep-Dive:** [Advanced C: Alignment, Strict Aliasing, Undefined Behavior, Atomics & Fixed/Floating-Point Math](./advanced-c-ub-memory-model.md)  
> **Mandatory Verification Harness:** [tests/test_c_foundations.c](../tests/test_c_foundations.c)  
> **Code Verification Policy:** All C examples below are **Statically Reviewed (Not Tested on Physical Hardware)** unless explicitly verified via `arm-none-eabi-gcc 14.3.1`.

---

## 1. Learning Objectives

Upon completing Stage 02 (including its companion guide `advanced-c-ub-memory-model.md`), you will master all 18 ISO C embedded domains (62 atomic concepts):

| Domain | Concept IDs | Level | Core Mastery Outcomes |
|---|---|---|---|
| Compilation & Linkage | `C-COMP-001..004` | L1–L2 | Trace ISO C's 8 translation phases and GCC's 4 stages (`cpp` → `cc1` → `as` → `ld`), separate translation units cleanly, apply `-fno-common` & ODR, and use freestanding C11 headers (`-ffreestanding`). |
| Integers & Promotions | `C-INT-001..004` | L1–L2 | Use `<stdint.h>` (`uintN_t`, `uintptr_t`, `size_t`, `ptrdiff_t`), two's complement & `INT_MIN` asymmetry, signed overflow UB vs unsigned modular wraparound, and integer promotions (`~uint8_t` trap!). |
| Bitwise & Shifts | `C-BIT-001..003` | L1 | Write glitch-free shadow-register bit masking, avoid all shift UBs (`1 << 31`, `x << 32`, negative shifts), and implement portable bitfield packing/unpacking & sub-word sign extension. |
| Pointers & Arrays | `C-PTR-001..004`, `C-ARR-001..004` | L1–L3 | Apply scaled pointer math, read all `const`/`volatile` pointer qualifier combinations, avoid Cortex-M `0x00000000` NULL dereference silently succeeding, understand `restrict`, array-to-pointer decay & exceptions, `.rodata` strings, 2D DMA buffers, C99 FAMs, and banned VLAs (`-Wvla`). |
| Composite Types | `C-TYPE-001..004` | L1–L2 | Design structs with C99 designated initializers, tagged unions, avoid C bit-field MMIO bus-width hazards, handle `enum` ABI sizing (`-fshort-enums`), and encapsulate drivers with `typedef` opaque pointers. |
| Storage, Qualifiers & Macros | `C-STOR-001..004`, `C-QUAL-001..004`, `C-PREP-001..004` | L1–L2 | Master `static` (internal linkage vs block persistence), `extern` (including linker symbols `&_sdata`), `const` `.rodata` SRAM conservation, `static inline` vs C99 `inline`, `_Static_assert`, `do { ... } while (0)`, `#`, `##`, `__VA_ARGS__`, and ISO-compliant include guards. |
| Functions, Memory & Optimization | `C-FPTR-001..003`, `C-MEM-001..003`, `C-FUNC-001..003`, `C-OPT-001..003` | L1–L2 | Build callback APIs (`void *ctx`) and `.rodata` FSM jump tables, map C objects to ELF sections, eliminate `malloc`/dangling-pointer/recursion hazards (`-fstack-usage`), and understand `-O0..-Os/-Og/-flto` & `--gc-sections`. |
| Alignment, UB, Atomics & Math | `C-ALIGN-001..003`, `C-UB-001..003`, `C-SEQ-001..002`, `C-ATOM-001..004`, `C-MATH-001..003` | L2–L3 | Covered in depth in [advanced-c-ub-memory-model.md](./advanced-c-ub-memory-model.md): struct alignment/padding/`packed` faults, Little-Endian & `__REV`, strict aliasing & `memcpy` punning, C11 `sequenced-before`, `<stdatomic.h>` & barriers, Q15/Q31 fixed-point, and IEEE-754 `double` literal traps. |

---

## 2. Prerequisites

- Basic programming logic (variables, loops, conditionals, functions) and binary/hexadecimal representation.
- Recommended companion: [Stage 01b — Digital Electronics & Hardware Integration](../01b-digital-electronics-and-hw-integration/README.md) (`DIG-ELEC-001..003`).

---

## 3. Detailed Technical Theory

### 3.1 ISO C 8 Translation Phases, Translation Units & Freestanding C (`C-COMP-001..004`)

Before a single Cortex-M4 instruction is emitted, ISO/IEC 9899:2011 (§5.1.1.2) defines **8 strict translation phases**:
1. **Physical character mapping** (trigraphs / source charset).
2. **Line splicing:** Every backslash followed immediately by a newline (`\` + `\n`) is deleted, joining multi-line macros before tokenization.
3. **Tokenization & comment stripping:** Every comment (`/* ... */` or `// ...`) is replaced by a **single space** (not deleted!).
4. **Preprocessing (`cpp`):** Executes `#include`, `#define`, `#if`/`#ifdef`, `#pragma`, and expands macros. Produces a single **Translation Unit (TU)** (`.i` file via `arm-none-eabi-gcc -E`).
5. **Escape sequence conversion** (`\n`, `\x1B`) inside character/string literals.
6. **Adjacent string literal concatenation** (`"STM32" "G474"` → `"STM32G474"`).
7. **Compilation & Assembly (`cc1` → `.s`, `as` → `.o`):** Syntax/semantic analysis, optimization, Thumb-2 code generation, and assembly into a relocatable ELF object file (`.o`) containing symbol and relocation tables.
8. **Linking (`ld` → `.elf`):** Resolves external references across all `.o` files and static `.a` archives, assigns physical Flash/SRAM addresses per the linker script, and outputs the executable `.elf`.

**Declarations vs Definitions & `-fno-common` (`C-COMP-002`, `C-COMP-003`):**
- A **declaration** introduces a name and type without allocating storage (`extern uint32_t g_ticks;`, `void uart_init(void);`). It belongs in `.h` headers.
- A **definition** allocates storage or supplies a function body (`uint32_t g_ticks = 0U;`). It belongs in **exactly one** `.c` file (One Definition Rule).
- Starting with **GCC 10+**, **`-fno-common`** is the default: placing an uninitialized global `uint32_t g_flag;` (a tentative definition) inside a header included by two `.c` files triggers an immediate `multiple definition of 'g_flag'` linker error instead of silently merging them!

**Freestanding vs Hosted C (`C-COMP-004`):**
Bare-metal microcontrollers have no OS kernel, filesystem, or console (`-ffreestanding`). ISO C11 (§4p6) guarantees only **freestanding headers** are always available without an OS runtime: `<stdint.h>`, `<stddef.h>`, `<stdbool.h>`, `<limits.h>`, `<float.h>`, `<stdarg.h>`, `<stdalign.h>`, `<stdnoreturn.h>`, `<iso646.h>`. Hosted libraries (`<stdio.h>` `printf`, `<stdlib.h>` `malloc`) require explicit low-level syscalls (`_write`, `_sbrk`).

---

### 3.2 Fixed-Width Integers, Two's Complement, Promotions & Overflow (`C-INT-001..004`)

Bare C types (`int`, `short`, `long`) have implementation-defined widths across architectures. Embedded firmware must use `<stdint.h>` and `<stddef.h>`:
- **Exact-width:** `int8_t`, `uint8_t`, `int16_t`, `uint16_t`, `int32_t`, `uint32_t`, `int64_t`, `uint64_t`.
- **Pointer/Object-sized:** `uintptr_t` / `intptr_t` (for safe MMIO address integer arithmetic), `size_t` (unsigned byte counts / `sizeof` result), `ptrdiff_t` (signed pointer difference).
- **Portable `printf` formatters (`<inttypes.h>`):** Use `PRIu32`, `PRId32`, `PRIx32` (since `uint32_t` is `unsigned long` on `arm-none-eabi-gcc` 32-bit EABI, while it is `unsigned int` on x86_64 Linux!).
- **ARM AAPCS `char` Signedness Trap:** Per ARM AAPCS (§7.1.1), plain `char` is **`unsigned`** by default on ARM (whereas it is `signed` on x86_64!). Always write `int8_t` or `uint8_t` for numeric bytes and reserve `char` strictly for ASCII text.

**Two's Complement & `INT_MIN` Asymmetry (`C-INT-002`):**
An $N$-bit signed integer spans $[-2^{N-1},\ +2^{N-1}-1]$. Negation computes bitwise inversion plus 1:
$$\text{Two's Complement}(x) = (\sim x) + 1$$
Example in 8 bits: $+5 = \texttt{00000101}_2 \rightarrow \sim 5 = \texttt{11111010}_2 \rightarrow +1 = \texttt{11111011}_2 = \texttt{0xFB} = -5$.
- **Sign Extension:** Widening `int8_t(-5)` (`0xFB`) to `int32_t` replicates bit 7 (`SXTB` instruction on Cortex-M4) yielding `0xFFFFFFFB`. Widening `uint8_t(251)` (`0xFB`) zero-extends (`UXTB`) yielding `0x000000FB`.
- **`INT_MIN` Asymmetry:** Because `-(-128)` = `+128` cannot fit in `int8_t` (`max = +127`), negating `INT32_MIN` (`-2147483648`) or calling `abs(INT32_MIN)` causes **Signed Integer Overflow Undefined Behavior (UB)**!

**Signed Overflow UB vs Unsigned Modular Wraparound (`C-INT-003`):**
- **Unsigned arithmetic** is strictly defined modulo $2^N$ (ISO C11 §6.2.5p9). `0xFFFFFFFFUL + 1UL == 0UL` is 100% legal and guarantees wraparound-safe timer elapsed math: `(uint32_t)(now - start) >= period` works seamlessly even across the 49.7-day 32-bit millisecond rollover!
- **Signed integer overflow** is **Undefined Behavior (UB)** (ISO C11 §6.5p5). At `-O2`, GCC legally assumes signed overflow *never happens*, so `if (x + 1 < x)` for `int32_t x` is optimized away to a constant `false`!

**Integer Promotions & Usual Arithmetic Conversions (`C-INT-004`):**
Every integer type narrower than `int` (`uint8_t`, `int8_t`, `uint16_t`, `int16_t`) is **automatically promoted to signed `int`** (32-bit on Cortex-M4) before arithmetic or bitwise operations! This causes three infamous embedded bugs:
1. **The `~uint8_t` Trap:**
   ```c
   uint8_t a = 0x55U;
   if (~a == 0xAAU) { /* ALWAYS FALSE! */ }
   ```
   `a` promotes to 32-bit `int` `0x00000055`, so `~a` evaluates to `0xFFFFFFAA` (negative!), which never equals `0x000000AA`. **Fix:** `if ((uint8_t)~a == 0xAAU)`.
2. **The `uint16_t * uint16_t` Signed Overflow UB Trap:**
   On 32-bit ARM, `uint16_t x = 60000U, y = 60000U; uint32_t p = x * y;` promotes BOTH `x` and `y` to **signed 32-bit `int`**, computing `3,600,000,000 > INT32_MAX (2,147,483,647)` → **Signed Overflow UB**! **Fix:** `uint32_t p = (uint32_t)x * (uint32_t)y;`.
3. **Signed/Unsigned Comparison Trap:**
   In `int32_t a = -1; uint32_t b = 1U; if (a < b)`, usual arithmetic conversions convert `a` to `uint32_t` (`0xFFFFFFFFU`), making `-1 < 1U` evaluate **FALSE**! Always compile with `-Wsign-compare -Wconversion`.

---

### 3.3 Bitwise Manipulation & Shift Undefined Behavior (`C-BIT-001..003`)

- **Glitch-Free Multi-Bit Register Update (`C-BIT-001`):**
  Writing `GPIOA->MODER &= ~(3UL << 10U); GPIOA->MODER |= (2UL << 10U);` emits **two** bus stores, momentarily putting `PA5` into Input mode (`00b`) between stores! Always modify in a local variable and perform a **single store**:
  ```c
  uint32_t tmp = GPIOA->MODER;
  tmp = (tmp & ~(3UL << 10U)) | (2UL << 10U);
  GPIOA->MODER = tmp;
  ```
- **Four Bit-Shift UB / Implementation-Defined Traps (`C-BIT-002`):**
  1. **Shifting into the sign bit of a signed `int`:** `(1 << 31)` is **UB** in C99/C11! Always use unsigned literals: **`(1UL << 31U)`**.
  2. **Shift count $\ge$ width or $< 0$:** `x << 32` or `x >> -1` is **UB**! (ARM `LSL` returns `0` for count `32`, whereas x86 masks count `32 & 31 == 0` returning `x`!)
  3. **Left-shifting a negative value:** `(-1) << 2` is **UB** in C11.
  4. **Right-shifting a negative signed integer:** `(-8) >> 1` is **implementation-defined** (GCC on ARM emits arithmetic shift `ASR`, preserving sign, but portable code avoids relying on it without documenting it).
- **Portable Sub-Word Sign Extension (`C-BIT-003`):**
  To sign-extend an unsigned 12-bit two's-complement sensor reading `raw` (`0x000..0xFFF`) into `int32_t` with **zero UB**:
  ```c
  int32_t sign_extend_12bit(uint32_t raw)
  {
      uint32_t masked = raw & 0x0FFFUL;
      return (int32_t)(masked ^ 0x0800UL) - 0x0800L;
  }
  ```

---

### 3.4 Pointers, Qualifiers, Arrays, Strings & Memory Hazards (`C-PTR-001..004`, `C-ARR-001..004`)

1. **Scaled Pointer Arithmetic & One-Past-The-End Rule (`C-PTR-001`):**
   Incrementing `T *p` adds `sizeof(T)` bytes. Subtracting two pointers into the *same* array (`p2 - p1`) returns `ptrdiff_t` in **elements**, not bytes. Computing `&arr[N]` (one past the last element) is legal for loop bounds, but dereferencing `&arr[N]` or computing `&arr[-1]` is **UB**.
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
   - Pointer arithmetic on `void *` is illegal in ISO C (GCC extension only; reject with `-Wpedantic -Wpointer-arith`).
   - **Cortex-M4 Critical Warning:** Unlike Linux/Windows where dereferencing `NULL` (`0x00000000`) immediately causes a `SIGSEGV`, on STM32G474RE address `0x00000000` is **aliased to Flash (`0x08000000`)** when `BOOT0 = 0`! Reading `*(uint32_t *)0` **silently succeeds and returns `_estack` (`0x20018000`)** instead of faulting! Always check `if (ptr == NULL)` defensively and consider an MPU Region 0 null-guard (`ARM-MPU-001`).
4. **Array-to-Pointer Decay & Its Exceptions (`C-ARR-001`):**
   In expressions and function parameters (`void tx(uint8_t buf[64])`), an array **decays** to a pointer to its first element (`uint8_t *`), so `sizeof(buf)` inside `tx()` is **4 bytes**, NOT 64! The **only exceptions** where an array does NOT decay are: `sizeof(arr)`, `&arr` (yields `T (*)[N]`), `_Alignof(arr)`, and string literal initialization (`char s[] = "OK";`).
5. **Strings & `.rodata` Placement (`C-ARR-002`):**
   - `const char *msg = "READY";` places `"READY\0"` in Flash `.rodata` (`0x0800xxxx`); writing `msg[0] = 'X'` is UB and triggers a Flash BusFault/HardFault.
   - `char buf[] = "READY";` copies 6 bytes onto the stack every time the function runs.
   - Ban unbounded `strcpy`/`sprintf`/`gets`; use `snprintf(buf, sizeof(buf), ...)` and check its return value for truncation.
6. **Multidimensional Arrays for DMA (`C-ARR-003`):**
   C arrays are strictly **row-major**. Interleaved 2-channel ADC DMA buffers (`volatile uint16_t adc_buf[32][2]`) pass cleanly to functions as `const volatile uint16_t (*buf)[2]` without losing column stride information.
7. **Flexible Array Members (FAM) vs Banned VLAs (`C-ARR-004`):**
   - **C99 FAM** (`struct packet { uint16_t len; uint8_t payload[]; };`) cleanly overlays variable-length headers over fixed static buffer pools (`sizeof(struct packet)` excludes `payload[]`).
   - **Variable-Length Arrays (VLAs)** (`uint8_t tmp[user_len];`) allocate unbounded stack frames at runtime, causing silent stack overflow on constrained MCU SRAM. **Banned by MISRA C:2012/2023 Rule 18.8** — always pass **`-Wvla -Werror`**!

---

### 3.5 Structs, Unions, Bit-Fields, Enums & Typedef (`C-TYPE-001..004`)

1. **Structs & C99 Designated Initializers (`C-TYPE-001`):**
   Initialize driver configuration structs explicitly by field name (unspecified fields are guaranteed zero-initialized):
   ```c
   const uart_cfg_t cfg = { .baud = 115200UL, .stop_bits = 1U, .parity = UART_PARITY_NONE };
   ```
2. **Tagged (Discriminated) Unions (`C-TYPE-002`):**
   A `union` overlays all members at offset 0 (`sizeof` = largest member + alignment padding). In RTOS message queues, wrap unions inside a **tagged struct** (`struct { msg_type_t type; union { ... } data; }`) and always switch on `type` before reading the active payload member.
3. **Why C Bit-Fields Are Dangerous for MMIO Registers (`C-TYPE-003`):**
   Mapping STM32 registers with `struct { uint32_t EN : 1; uint32_t MODE : 2; }` has **four fatal ISO C flaws**:
   - Bit ordering (LSB-first vs MSB-first) is **implementation-defined**.
   - Signedness of plain `int : N` is **implementation-defined** (`int f : 1` can only hold `0` and `-1`!).
   - Straddling storage units is **implementation-defined**.
   - **Bus-Width Hardware Hazard:** GCC may compile a write to a `volatile` bit-field into an 8-bit `STRB` or 16-bit `STRH` bus transaction on an STM32 peripheral register that mandates strict **32-bit `STR`** word access, or emit non-atomic RMW sequences on `rc_w1`/`rc_w0` status registers! **Rule:** Always use CMSIS `_Pos`/`_Msk` shift-and-mask macros for MMIO.
4. **Enumerations (`enum`), `-fshort-enums` & Opaque `typedef` (`C-TYPE-004`):**
   The underlying integer size of an `enum` is implementation-defined (4 bytes by default on ARM EABI; 1 or 2 bytes if `-fshort-enums` is passed — **never mix object files compiled with different `-fshort-enums` settings!**). Use opaque pointer typedefs (`typedef struct uart_ctx uart_ctx_t;` in `.h`, full `struct uart_ctx { ... };` hidden inside `.c`) to enforce module encapsulation.

---

### 3.6 Storage Classes, Qualifiers & Preprocessor Hygiene (`C-STOR-001..004`, `C-QUAL-001..004`, `C-PREP-001..004`)

| Declaration | Scope | Linkage | Storage Duration | STM32G474RE Memory Section |
|---|---|---|---|---|
| `uint32_t g_cnt = 5U;` | File | External | Static (whole run) | `.data` (LMA Flash, VMA SRAM) |
| `uint32_t g_buf[64];` | File | External | Static (zeroed at boot) | `.bss` (VMA SRAM, 0 B Flash) |
| `static uint32_t s_cnt;` | File | **Internal** (TU-private) | Static (zeroed at boot) | `.bss` (symbol `b` in `nm`) |
| `const uint16_t LUT[256] = {...};` | File | External / Internal (`static`) | Static | **`.rodata` (Flash ONLY — 0 B SRAM!)** |
| `static uint32_t calls = 0U;` (inside fn) | Block | None | **Static** (persists across calls!) | `.bss` / `.data` |
| `uint32_t tmp;` (inside fn) | Block | None | Automatic (**uninitialized garbage!**) | Call Stack (MSP/PSP) |

- **`static` Dual Semantics (`C-STOR-002`):** At **file scope**, `static` restricts linkage to the current `.c` file (preventing namespace collisions). Inside a **function block**, `static` gives a local variable persistent static lifetime across calls (warning: **makes the function non-reentrant!**).
- **`extern` & Linker Symbols Trap (`C-STOR-003`):** Linker-script symbols (`extern uint32_t _sdata;`) have **NO storage value** — only an **address (`&_sdata`)**! Reading `_sdata` directly dereferences the first word of `.data` (a fatal startup bug); always take `&_sdata`!
- **`const` Flash Conservation (`C-QUAL-001`):** On STM32G474RE (512 KB Flash vs 96 KB SRAM1+SRAM2), forgetting `const` on a 4 KB sine/font table wastes **4 KB of SRAM AND 4 KB of Flash** (`.data` LMA + VMA). Adding `static const` puts it strictly in `.rodata` (Flash only, 0 bytes SRAM).
- **`static inline` vs C99 `inline` (`C-QUAL-003`):** Bare C99 `inline int f(void)` in a header emits **no out-of-line definition**, causing `undefined reference` linker errors at `-O0` unless paired with `extern inline` in one `.c` file. Always use **`static inline`** (CMSIS `__STATIC_INLINE`) in headers.
- **`_Static_assert` (`C-QUAL-004`):** C11 compile-time checks zero-cost structural invariants:
  ```c
  _Static_assert(sizeof(uint32_t) == 4U, "ARM Cortex-M4 requires 32-bit uint32_t");
  _Static_assert((RING_SIZE & (RING_SIZE - 1U)) == 0U, "Ring buffer must be power of 2");
  ```
- **Preprocessor Hygiene (`C-PREP-001..004`):**
  1. **Parenthesize every parameter AND the whole expression:** `#define MUL(a, b) ((a) * (b))`. Better yet, prefer **`static inline` functions** to avoid double-evaluation bugs (`MAX(ADC1->DR, 100U)` reads a hardware FIFO register **twice**!).
  2. **Multi-statement macro guard:** Always wrap in `do { ... } while (0)` (no trailing semicolon) so `if (cond) MY_MACRO(); else ...` compiles safely.
  3. **Two-level stringification & token pasting:** `#define STR_IMPL(x) #x` / `#define STR(x) STR_IMPL(x)` expands `__LINE__` before stringifying; `##` concatenates tokens (`GPIO##port`). Variadic macros (`#define LOG(fmt, ...)`) compile to `((void)0)` in release builds.
  4. **Include guards:** Use `#ifndef UART_DRV_H` / `#define UART_DRV_H` ... `#endif` — **never** start user include guards with `_` + uppercase or `__` (reserved for the compiler/standard library by ISO C11 §7.1.3!).

---

### 3.7 Function Pointers, Callbacks, Jump Tables, Stack & Optimizations (`C-FPTR-001..003`, `C-MEM-001..003`, `C-FUNC-001..003`, `C-OPT-001..003`)

1. **Function Pointers & Cortex-M Thumb Bit-0 Rule (`C-FPTR-001`):**
   Always use `typedef` for readability: `typedef void (*exti_cb_t)(void *user_ctx);`. On ARM Cortex-M4, every valid function address has **LSB bit 0 = `1`** (indicating Thumb state for `BLX`/`BX`). Casting function pointers to object pointers `(uint32_t *)` violates ISO C11 §6.3.2.3p8 (emits `-Wpedantic` warnings).
2. **Callbacks (`void *ctx`) & `.rodata` FSM Jump Tables (`C-FPTR-002`, `C-FPTR-003`):**
   Pass an opaque `void *ctx` context pointer alongside every driver callback to avoid mutable globals. Store FSM handler tables as `static const state_fn_t fsm_table[STATE_COUNT]` in Flash `.rodata` and **always bounds-check** `if (state < STATE_COUNT)` before indirect dispatch!
3. **Why `malloc`/`free` & Recursion Are Banned in Embedded Systems (`C-MEM-002`, `C-MEM-003`, `C-FUNC-002`):**
   - `malloc`/`free` suffers from external heap fragmentation, non-deterministic latency, non-reentrant newlib lock hazards, and silent heap-stack collisions (banned by MISRA Rule 21.3). Use static buffers or $O(1)$ fixed-block memory pools.
   - Returning `&local_var` creates a **dangling pointer** whose memory is silently clobbered by the next function call or **Cortex-M hardware ISR stacking** (`-Wreturn-local-addr`).
   - Recursion causes unbounded stack growth (banned by MISRA Rule 17.2). Convert to iterative loops and audit every function's frame with **`arm-none-eabi-gcc -fstack-usage`** (generates `.su` files).
4. **GCC Optimization Levels (`C-OPT-001..003`):**
   - `-O0`: Zero optimization; huge Flash, slow, hides UB/missing-`volatile` bugs.
   - `-Og`: Debug-friendly optimization (recommended development default).
   - `-Os`: Optimize for minimal Flash code size.
   - `-O2` / `-O3`: Aggressive speed optimizations (inlining, loop unrolling, `-fstrict-aliasing`). Beware GCC transforming `.bss` zero-loops into `memset()` calls at `-O2`/`-O3` under `-nostdlib` (prevent with `-fno-tree-loop-distribute-patterns` or custom `memset`).
   - `-ffunction-sections -fdata-sections -Wl,--gc-sections`: Places each symbol in its own section so the linker strips unreferenced code/data (requires `KEEP(*(.isr_vector))` in `.ld` so the vector table is never stripped!).
   - `-flto`: Link-Time Optimization across `.c` files (annotate weak ISRs/vectors with `__attribute__((used))` so LTO preserves them).

> **Continue to the Companion Deep-Dive:** Read [advanced-c-ub-memory-model.md](./advanced-c-ub-memory-model.md) for full engineering coverage of **Alignment, Padding, `__attribute__((packed))` UsageFaults, Little-Endian & `__REV`, Strict Aliasing, `memcpy` Type Punning, UB Taxonomy, C11 `sequenced-before`, `<stdatomic.h>`, Compiler Barriers, Q15/Q31 Fixed-Point, and IEEE-754 `double` Literal Traps**.

---

## 4. Important Terminology

| Term | Engineering Definition |
|---|---|
| **Translation Unit (TU)** | A single `.c` source file after all `#include` expansions and macro substitutions (`cpp` output), compiled independently into one `.o` object file. |
| **Integer Promotion** | Automatic implicit widening of integer types narrower than `int` (`uint8_t`, `int16_t`) to signed `int` before arithmetic or bitwise operations. |
| **Undefined Behavior (UB)** | Program construct for which the ISO C standard imposes zero requirements; optimizing compilers assume UB never occurs and may delete safety checks or emit nonsensical code. |
| **Array Decay** | Automatic conversion of an array expression (`T[N]`) into a pointer to its first element (`T *`), losing array length information (`sizeof` yields pointer size). |
| **Internal Linkage** | Symbol visibility restricted to a single translation unit via file-scope `static`, preventing cross-file symbol collisions. |
| **Reentrancy** | Property of a function allowing it to be safely interrupted mid-execution (by an ISR or RTOS task) and concurrently re-entered without data corruption. |
| **Flexible Array Member (FAM)** | A C99 unsized trailing array `uint8_t data[];` at the end of a `struct` for clean variable-length packet framing over static buffers. |

---

## 5. Register-Level Explanations

Although Stage 02 focuses on ISO C semantics, two Cortex-M4/STM32G474RE registers directly interact with C language behavior:
- **`SCB->CCR` (`0xE000ED14`, `core_cm4.h`):**
  - Bit 3 — **`UNALIGN_TRP`**: When set to `1`, *any* unaligned halfword/word access (such as dereferencing a misaligned pointer into a `__attribute__((packed))` struct) immediately triggers a `UsageFault` (`CFSR.UNALIGNED = 1`). Even when `UNALIGN_TRP = 0`, unaligned `LDRD`/`STRD`/`LDM`/`STM` **always** fault!
  - Bit 4 — **`DIV_0_TRP`**: When set to `1`, integer division by zero triggers a `UsageFault` (`CFSR.DIVBYZERO = 1`) instead of silently returning `0`.
- **`GPIOA->MODER` (`0x48000000`, `stm32g474xx.h`):** Demonstrates why C bit-fields are unsafe for MMIO and why single-store shadow RMW (`uint32_t tmp = GPIOA->MODER; ... GPIOA->MODER = tmp;`) prevents transient mode glitches.

---

## 6. Relevant Formulas & Calculations

### 6.1 Signed & Unsigned $N$-Bit Integer Ranges
$$\text{Unsigned } N\text{-bit}: \left[0,\ 2^N - 1\right], \qquad \text{Signed Two's Complement } N\text{-bit}: \left[-2^{N-1},\ 2^{N-1} - 1\right]$$

### 6.2 Two's Complement Negation & Wraparound-Safe Elapsed Time
$$-x \equiv (\sim x) + 1 \pmod{2^N}, \qquad \Delta t = \left(\text{uint32\_t}\right)(t_{\text{now}} - t_{\text{start}}) \pmod{2^{32}}$$

**Worked Example:** `t_start = 0xFFFFFFF0UL`, `t_now = 0x00000010UL`:
$$\Delta t = (0\text{x}00000010 - 0\text{xFFFFFFF0}) \bmod 2^{32} = 0\text{x}00000020 = 32\text{ ticks (exact!)}$$

### 6.3 Struct Alignment & Padding Offset Formula
For member $i$ with alignment requirement $A_i = \text{\_Alignof}(T_i)$:
$$\text{offset}_i = \left\lceil \frac{\text{current\_offset}}{A_i} \right\rceil \cdot A_i, \qquad \text{sizeof}(S) = \left\lceil \frac{\text{end\_offset}}{\max_i A_i} \right\rceil \cdot \max_i A_i$$

---

## 7. Official Documentation References

- **ISO/IEC 9899:2011 (C11 Standard):** §4p6 (Freestanding), §5.1.1.2 (8 Translation Phases), §6.2.2 (Linkage), §6.2.5/§6.3.1 (Integer Promotions & Conversions), §6.5.7 (Bitwise Shifts), §6.7.3 (Type Qualifiers), §6.7.10 (`_Static_assert`), §6.10 (Preprocessing), Annex J (UB/Unspecified/Implementation-Defined).
- **ARM IHI 0042 (AAPCS):** *Procedure Call Standard for the Arm Architecture* (§5 Alignment, §7.1.1 unsigned `char`, §7.1.3 `enum` sizes, §7.1.7 bit-fields).
- **GNU GCC Manual:** §3.8 (Warning Options: `-Wconversion`, `-Wdouble-promotion`, `-Wvla`, `-Wshadow`), §3.11 (Optimization Options), §3.18 (`-fno-common`, `-fstack-usage`).
- **MISRA C:2012 / C:2023** & **SEI CERT C Coding Standard**.

---

## 8. Verified Code Examples

> **Verification Status:** **Compile-Verified (`arm-none-eabi-gcc 14.3.1`, `-O2 -Wall -Wextra -Werror -Wconversion -Wdouble-promotion -Wvla`)** (Not Tested on Physical Hardware; companion unit tests in [tests/test_c_foundations.c](../tests/test_c_foundations.c)).

### 8.1 UB-Free Bitfield Packing, 12-Bit Sign Extension, Array-Safe Length & Wraparound Math

```c
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* Compile-time safe ARRAY_LEN: fails to compile if passed a decayed pointer (GCC/Clang) */
#define ARRAY_LEN(arr) \
    (sizeof(arr) / sizeof((arr)[0]) + \
     0U * sizeof(char[1 - 2 * __builtin_types_compatible_p(__typeof__(arr), __typeof__(&(arr)[0]))]))

/* Multi-statement macro wrapped safely in do { ... } while (0) */
#define REG_MODIFY_32(reg, clear_mask, set_mask)                \
    do {                                                        \
        uint32_t _reg_tmp = (reg);                              \
        _reg_tmp = (_reg_tmp & ~(uint32_t)(clear_mask)) |       \
                   ((uint32_t)(set_mask) & (uint32_t)(clear_mask)); \
        (reg) = _reg_tmp;                                       \
    } while (0)

/* Extract width-bit field starting at lsb (shift-UB safe) */
static inline uint32_t field_get(uint32_t reg, uint32_t lsb, uint32_t width)
{
    if ((lsb >= 32U) || (width == 0U) || (width > 32U)) {
        return 0UL;
    }
    uint32_t mask = (width == 32U) ? 0xFFFFFFFFUL : ((1UL << width) - 1UL);
    return (reg >> lsb) & mask;
}

/* Portable UB-free 12-bit two's-complement sign extension */
static inline int32_t sign_extend_12(uint32_t raw_12b)
{
    uint32_t val = raw_12b & 0x0FFFUL;
    return (int32_t)(val ^ 0x0800UL) - 0x0800L;
}

/* Wraparound-safe 32-bit unsigned elapsed check */
static inline bool timeout_expired(uint32_t now, uint32_t start, uint32_t period)
{
    return ((uint32_t)(now - start) >= period);
}

/* Struct layout optimization: ordered largest-to-smallest alignment */
typedef struct {
    uint32_t timestamp_ms; /* offset 0 */
    uint16_t adc_raw;      /* offset 4 */
    uint8_t  channel;      /* offset 6 */
    uint8_t  status;       /* offset 7 */
} sensor_sample_t;

_Static_assert(sizeof(sensor_sample_t) == 8U, "Expected 8-byte tightly ordered struct");
_Static_assert(offsetof(sensor_sample_t, adc_raw) == 4U, "Unexpected adc_raw offset");
```

---

## 9. Practical STM32G474RE Experiments

1. **Run the Mandatory Stage 02 C Verification Harness (`tests/test_c_foundations.c`):**
   - Compile and run [../tests/test_c_foundations.c](../tests/test_c_foundations.c) natively on your host PC with hardened flags:
     ```bash
     gcc -std=c11 -Wall -Wextra -Werror -Wconversion -Wsign-compare -Wshadow -Wvla -Wpedantic \
         -fsanitize=address,undefined tests/test_c_foundations.c -o build/test_c_foundations && ./build/test_c_foundations
     ```
2. **Inspect GCC Pipeline Artifacts (`-save-temps`):**
   - Compile a driver snippet with `arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb -O2 -save-temps -c` and inspect `.i` (macro expansion), `.s` (Thumb-2 `SXTB`/`UXTB`/`UBFX`/`REV`), and `.su` (`-fstack-usage`).
3. **`.rodata` vs `.data` SRAM Audit:**
   - Declare a 4 KB sine table with and without `static const`; run `arm-none-eabi-size` to verify 4,096 bytes moving out of `.data` (SRAM+Flash) into `.rodata` (Flash-only).
   - **Debugging Checkpoint:** In GDB on NUCLEO-G474RE, inspect `x/4bx &val` for `uint32_t val = 0x12345678UL` (confirming Little-Endian `78 56 34 12`) and verify `x/1wx 0x00000000` returns `_estack` (proving the Cortex-M NULL-read silent alias trap!).

---

## 10. Common Mistakes & Anti-Patterns

1. **Writing `1 << 31` instead of `1UL << 31U`:** Signed 32-bit shift into the sign bit is Undefined Behavior in ISO C11.
2. **Comparing `~u8_var == 0xAAU`:** Integer promotion widens `u8_var` to signed `int` before `~`, setting bits `[31:8]` to `1`. Always cast `(uint8_t)~u8_var`.
3. **Using `sizeof(buf)` inside a function receiving `uint8_t buf[64]`:** Array decay makes `sizeof(buf) == 4` on 32-bit ARM! Always pass `(uint8_t *buf, size_t len)`.
4. **Mapping STM32 registers with C bit-fields:** Compiler may emit 8-bit `STRB` or non-atomic RMW sequences, breaking 32-bit-only and `rc_w1`/`rc_w0` registers.
5. **Dereferencing linker symbols directly (`uint32_t x = _sdata;`):** Linker symbols have addresses (`&_sdata`), not values!
6. **Using `static` local variables inside functions called by ISRs:** Silently breaks reentrancy and corrupts state under preemption.
7. **Using VLAs (`uint8_t buf[n];`) or recursion on MCU stacks:** Causes silent SRAM overflow into `.bss`/`.data`. Enforce `-Wvla -Werror` and `-fstack-usage`.
8. **Forgetting `const` on lookup tables and string pointers:** Wastes scarce SRAM and duplicates data across `.data` LMA + VMA.

---

## 11. Hands-On Exercises

1. **Bit Reversal & Field Extraction:** Write a shift-UB-free function to reverse all 32 bits of a `uint32_t` in portable C, then compare its Cortex-M4 disassembly at `-O2` against CMSIS `__RBIT(x)`.
2. **Struct Padding Optimization:** Given `struct Bad { uint8_t a; uint32_t b; uint8_t c; uint16_t d; uint64_t e; };`, compute its exact `sizeof` and member offsets on ARM AAPCS (8-byte `uint64_t` alignment!), then reorder its fields to minimize `sizeof` without `__attribute__((packed))`, proving offsets with `_Static_assert(offsetof(...))`.
3. **Multi-TU Linkage Lab:** Split a button counter into 3 files (`counter.h`, `counter.c`, `main.c`), intentionally trigger a `-fno-common` duplicate-symbol error, then fix it using `extern` in `.h`, one definition in `.c`, and `static` private helpers verified via `arm-none-eabi-nm`.
4. **Table-Driven FSM:** Implement a 4-state non-blocking button debouncer using a `static const` function-pointer table in `.rodata` with bounds-checked state dispatch.

---

## 12. Assessment Questions

1. Explain what `uint8_t a = 0xFFU; if (~a == 0x00U)` evaluates to on ARM Cortex-M4 and trace the exact ISO C integer promotion steps.
2. Contrast the meaning of `static` at file scope vs inside a function body, and explain why a block-scope `static` buffer breaks ISR reentrancy.
3. Why does `void process(uint8_t frame[128])` print `4` when evaluating `sizeof(frame)` on STM32G474RE? List the 4 contexts where an array does **not** decay.
4. Explain why `int32_t x = INT32_MAX; if (x + 1 < x)` can be legally deleted by GCC at `-O2`, whereas `(uint32_t)(now - start) >= 1000U` is guaranteed safe across rollover.
5. Why is `const volatile uint32_t *` not a contradiction in bare-metal Embedded C? Give a real STM32G474RE register example.
6. What happens on STM32G474RE (`BOOT0 = 0`) if firmware accidentally executes `uint32_t v = *(uint32_t *)0;`? Why does it not crash like a desktop segfault?

---

## 13. Measurable Completion Criteria

- [ ] **Automated C Test Suite Passed:** [tests/test_c_foundations.c](../tests/test_c_foundations.c) compiles with zero warnings under `-std=c11 -Wall -Wextra -Werror -Wconversion -Wshadow -Wvla -Wpedantic -fsanitize=address,undefined` and passes 100% of assertions.
- [ ] **Zero Shift/Promotion/UB Bugs:** Accurately identify and repair integer promotion, signed overflow, shift UB, array decay, and macro double-evaluation defects.
- [ ] **Memory & Section Mapping Verified:** Predict exact ELF sections (`.text`, `.rodata`, `.data`, `.bss`, stack) and struct `sizeof`/`offsetof` layouts, verified against `arm-none-eabi-nm` and `arm-none-eabi-size`.
- [ ] **Companion Deep-Dive Completed:** Complete all concepts and exercises in [advanced-c-ub-memory-model.md](./advanced-c-ub-memory-model.md) (`C-ALIGN-*`, `C-UB-*`, `C-SEQ-*`, `C-ATOM-*`, `C-MATH-*`).
