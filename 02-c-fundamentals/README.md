# Stage 01: Computer Science & C Language Foundations

## 1. Prerequisites
Basic high-level programming logic or algebra.

## 2. Core Technical Theory & Engineering Principles
The C language remains the lingua franca of embedded systems because it provides direct, unabstracted control over memory layouts, bitwise representations, and hardware access while maintaining structured syntax.

### 1. The C Compilation Pipeline
A C program transforms through four distinct phases:
1. **Preprocessing (`cpp`):** Handles `#include`, `#define`, `#ifdef` macros. Strips comments and produces `.i` intermediate files.
2. **Compilation (`cc1`):** Parses syntax, enforces type rules, performs optimizations, and generates architecture-specific assembly code (`.s`).
3. **Assembly (`as`):** Translates assembly mnemonics into machine code opcodes, generating relocatable object files (`.o`).
4. **Linking (`ld`):** Resolves symbolic references, assigns physical addresses based on the linker script, and outputs an executable (`.elf`).

### 2. Fixed-Width Integer Types & Memory Representation
Bare C types (`int`, `short`, `long`) have implementation-defined widths. In embedded firmware, ambiguous bit widths cause catastrophic overflows and porting bugs. Always use `<stdint.h>`:
- `uint8_t` / `int8_t`: Exactly 8 bits (0 to 255 / -128 to +127)
- `uint16_t` / `int16_t`: Exactly 16 bits (0 to 65,535 / -32,768 to +32,767)
- `uint32_t` / `int32_t`: Exactly 32 bits (0 to 4,294,967,295 / -2,147,483,648 to +2,147,483,647)

**Two's Complement Arithmetic:**
Negative numbers are represented by inverting all bits (one's complement) and adding 1:
$$	ext{Two's Complement}(x) = (\sim x) + 1$$
Example for -5 in 8 bits: $5 = 	exttt{00000101b} ightarrow \sim 5 = 	exttt{11111010b} ightarrow +1 = 	exttt{11111011b} = 	exttt{0xFB}$.

### 3. Bitwise Operators & Register Masking
Microcontroller configuration is conducted entirely through bit manipulation:
- **Bitwise AND (`&`):** Used to isolate or clear specific bits: `REG &= ~(1U << bit);`
- **Bitwise OR (`|`):** Used to set specific bits without disturbing others: `REG |= (1U << bit);`
- **Bitwise XOR (`^`):** Used to toggle bit states: `REG ^= (1U << bit);`
- **Bitwise NOT (`~`):** Inverts all bits, creating bitmask complements.
- **Left Shift (`<<`) & Right Shift (`>>`):** Moves bits into target bitfield positions.

### 4. Pointer Mechanics & Struct Alignment
A pointer stores a 32-bit memory address. On ARM Cortex-M, memory access is 32-bit aligned.
- **Pointer Scaling:** Incrementing a `uint32_t *ptr` by 1 advances the physical memory address by 4 bytes.
- **Structure Padding:** Compilers insert padding bytes so that 32-bit members align on 4-byte boundaries. Pack structures carefully or use `__attribute__((packed))` when mapping external binary payloads.

## 3. Practical Coding Exercises & Challenges
1. Write a function to reverse the bit order of a 32-bit integer.
2. Calculate the struct size and padding offsets for mixed data types.
3. Write an unpacking macro to extract an 8-bit field from bits [19:12] of a register.

## 4. Expected Learning Outcomes & Mastery Criteria
Mastery of bitwise logic, binary arithmetic, pointer dereferencing, and struct memory layout.
