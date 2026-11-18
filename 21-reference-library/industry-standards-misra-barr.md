# Industry Standards: MISRA C:2012/2023, SEI CERT C & Barr Group Guidelines

Safety-critical and production embedded C firmware adheres to formal coding standards to eliminate undefined behavior (UB), integer wrap vulnerabilities, silent implicit conversions, and concurrency defects.

**Concept IDs:** `PROF-STD-001`, `PROF-STD-002`, `PROF-STD-003`, `PROF-STD-004`

---

### 1. MISRA C:2012 & MISRA C:2023

MISRA C (*Motor Industry Software Reliability Association*) is the primary standard for automotive (ISO 26262), medical (IEC 62304), industrial (IEC 61508), and aerospace firmware.

#### Structure & Classification
- **Directives vs. Rules:** *Directives* require process/design context (e.g., Dir 4.1 runtime failure minimization, Dir 4.6 typedefs for numeric types); *Rules* are statically verifiable in source code.
- **Categories:** **Mandatory** (deviations strictly forbidden), **Required** (formal documented deviation required), **Advisory** (recommended; disapplication documented).
- **Essential Type Model:** Classifies expressions into *Boolean*, *Character*, *Signed*, *Unsigned*, and *Floating* essential types to prevent signed/unsigned sign-extension and bitwise shift defects.

#### Core Bare-Metal MISRA Rules
- **Rule 8.4:** External object/function definitions must have a visible compatible declaration in a header.
- **Rule 10.1 / 10.3 / 10.4:** Operands shall match appropriate essential types (never shift or mask signed `int` values; always use `U` suffixes).
- **Rule 11.4 (Advisory):** Conversions between object pointers and integers should not be performed *(official MISRA C:2012/2023 category is **Advisory**; safety-critical projects frequently elevate it to Required in their compliance plan, governing CMSIS MMIO base-address casts like `(GPIO_TypeDef *)0x48000000UL` with a formal Deviation Permit)*.
- **Rule 12.2 (Required):** Shift count must be $0 \le s < \text{width}$ of the promoted left operand.
- **Rule 17.7 (Required):** Return values of non-void functions must be checked (or explicitly cast `(void)` with rationale).
- **Rule 21.3 (Required):** Dynamic heap functions (`malloc`, `calloc`, `realloc`, `free`) are prohibited in safety-critical firmware.

#### MISRA C:2023 Updates (C11 / C18 Support)
MISRA C:2023 consolidates Amendments 1–4 and adds full support for **ISO C11 / C18**:
- Adds rules governing **`_Generic`** selection expressions, **`_Static_assert`**, **`_Noreturn`**, and **`<stdatomic.h>`** concurrency/data-race prevention (critical for ISR + RTOS shared variables).
- Tightens rules on uninitialized automatic variables, flexible array members, and thread-unsafe standard library calls (`strtok`, `rand`, non-reentrant `errno`).

---

### 2. SEI CERT C Secure Coding Standard (Carnegie Mellon SEI)

While MISRA focuses on safety/predictability, **SEI CERT C** targets exploitable security and reliability defects (enforceable via `clang-tidy --checks='cert-*'`):
- **`INT30-C` (Ensure unsigned integer operations do not wrap):** Prevents infinite underflow loops such as `for (uint32_t d = 6799; d > 0; d -= 50)` wrapping `49 - 50` to `0xFFFFFFFF`!
- **`INT32-C` (Ensure signed integer operations do not overflow):** Signed overflow is Undefined Behavior in C (optimizers legally delete overflow checks).
- **`ARR30-C` / `ARR38-C` (Do not form or use out-of-bounds pointers or array subscripts):** Mandatory bounds validation on DMA/UART/CAN buffers.
- **`STR31-C` (Guarantee storage for strings has sufficient space for null terminator):** Prefer bounded length-explicit functions (`strnlen`, `snprintf`) over `strcpy`/`sprintf`.
- **`EXP33-C` (Do not read uninitialized memory):** Initialize all local structs/arrays and zero parity-protected SRAM before reads.
- **`MEM30-C` / `MEM35-C`:** Prevents use-after-free and wrong `sizeof` allocation bugs (eliminated entirely by static allocation).

---

### 3. Barr Group Embedded C Coding Standard (2018)
- **Fixed-Width Integers:** Always use `<stdint.h>` (`uint8_t`, `int16_t`, `uint32_t`) — never bare `int` or `long`.
- **Mandatory Braces `{}`:** Every `if`, `else`, `for`, `while`, `do` body must be enclosed in braces.
- **Unsigned Literal Suffixes:** Always append `U`/`UL` to bitmasks and shift operands (`1U << 5U`).
- **`const` & `volatile` Discipline:** Mark lookup tables `static const` (Flash `.rodata`); mark ISR-shared flags and MMIO pointers `volatile`.

---

### 4. Formal MISRA Deviation Workflow (MISRA Compliance:2020)

Real bare-metal firmware must formally document any disapplied Advisory rule (such as Rule 11.4 for memory-mapped register pointers in `stm32g474xx.h`) or Required rule deviation. Production teams govern exceptions via a **Deviation Permit**:

```text
Deviation ID:      DEV-STM32G4-001
Standard / Rule:   MISRA C:2023 Rule 11.4 (Advisory — elevated to Required in project safety plan: Integer-to-pointer cast)
Scope:             CMSIS Device Header (stm32g474xx.h) & drivers/mmio_access.h only
Technical Reason:  ARM Cortex-M4 peripherals reside at fixed hardware physical addresses
                   (e.g., GPIOA_BASE = 0x48000000UL) requiring integer-to-struct-pointer casts.
Risk Mitigation:   Addresses verified against RM0440 & stm32g474xx.h; isolated to HAL layer;
                   verified by static assertions on struct offsets and host fake-register tests.
Inline Suppression: /* cppcheck-suppress misra-c2012-11.4 ; DEV-STM32G4-001 */
Reviewer Sign-Off: Principal Firmware Engineer / Safety Assessor
```
