# Industry Standards: MISRA C & Barr Group Coding Guidelines

Safety-critical and high-reliability embedded systems adhere to strict coding guidelines to eliminate undefined behaviors, silent type conversions, and memory corruption bugs.

---

### 1. MISRA C:2012 (Guidelines for the Use of the C Language in Critical Systems)
MISRA C is the gold standard for automotive, aerospace, and medical embedded software:
- **Rule 8.4 (Declaration before Definition):** A compatible declaration must be visible when an object or function with external linkage is defined.
- **Rule 10.1 (Operand Types):** Operands shall not be of an inappropriate essential type (avoids implicit bitwise operations on signed values).
- **Rule 11.4 (Pointers and Integers):** A conversion should not be performed between a pointer to object and an integral type (exceptions carved out for hardware register base addresses).
- **Rule 12.2 (Shift Operators):** The right hand operand of a shift operator shall lie between zero and one less than the width in bits of the promoted left hand operand.
- **Rule 17.7 (Ignored Return Values):** The value returned by a function having non-void return type shall be used.
- **Rule 21.3 (Dynamic Allocation Prohibition):** The memory allocation and deallocation functions of `<stdlib.h>` (`malloc`, `free`) shall not be used in safety-critical firmware.

---

### 2. Barr Group Embedded C Coding Standard
Developed by Michael Barr, this standard emphasizes readability, maintainability, and bug avoidance:
- **Explicit Fixed-Width Types:** Always use `<stdint.h>` types (`uint8_t`, `int16_t`, `uint32_t`) rather than bare C types (`int`, `short`, `long`) whose bit widths vary across compilers.
- **Volatile Qualifiers:** Any variable modified within an ISR that is read in the main thread must be qualified as `volatile`.
- **Braces and Control Flow:** Every `if`, `else`, `while`, `for`, and `do` statement must enclose its body within curly braces `{}` even if containing a single line, preventing classic "dangling else" vulnerabilities.
- **Literal Value Constants:** Unsigned integer literals must be explicitly suffixed with `U` (e.g., `(1U << 5)` rather than `(1 << 5)`).
