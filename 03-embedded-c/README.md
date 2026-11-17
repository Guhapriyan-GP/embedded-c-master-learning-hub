# Stage 02: Embedded C & Memory-Mapped Architecture

## 1. Prerequisites
Stage 01 (Pointers, Bitwise operations, Structs).

## 2. Core Technical Theory & Engineering Principles
Embedded C differs from desktop C because it interacts directly with physical hardware registers mapped to specific addresses in the microcontroller's memory map.

### 1. Memory-Mapped I/O (MMIO)
In ARM Cortex-M processors, peripheral registers are mapped into the same 32-bit address space as Flash and SRAM. Controlling hardware is achieved by dereferencing pointers to these addresses:
```c
/* Direct address dereferencing */
#define GPIOA_MODER (*((volatile uint32_t *)0x48000000UL))
GPIOA_MODER |= (1U << 10); /* Set bit 10 */
```

### 2. The `volatile` Qualifier
The C compiler's optimizer assumes memory locations only change if modified by the current execution thread. Peripheral registers change asynchronously due to hardware events (e.g., received serial bytes or timer counts).
Without `volatile`, the compiler may cache register values in CPU core registers (R0-R12) or optimize away wait loops:
```c
/* BUG: Without volatile, optimizer converts this to an infinite loop! */
uint32_t *status_reg = (uint32_t *)0x4000441CUL;
while (!(*status_reg & (1U << 5)));

/* CORRECT: Force fresh bus read on every iteration */
volatile uint32_t *status_reg = (volatile uint32_t *)0x4000441CUL;
while (!(*status_reg & (1U << 5)));
```

### 3. Read-Modify-Write Hazards & Atomicity
Modifying a register field requires three CPU steps:
1. **Read (LDR):** Load current register value from memory into a CPU register.
2. **Modify (ORR/BIC):** Modify target bits in CPU register.
3. **Write (STR):** Store modified value back to peripheral memory.

If an interrupt triggers between Step 1 and Step 3 and modifies the same register, its modifications will be overwritten and lost when the main thread resumes.
**Remedy:** Use atomic hardware registers (such as STM32's `GPIOx->BSRR`) or enter a critical section.

## 3. Practical Coding Exercises & Challenges
1. Create a C structure representing a peripheral with 4 consecutive 32-bit registers.
2. Write a macro to test, set, and clear multi-bit fields atomically.
3. Demonstrate compiler loop elimination in assembly with and without volatile.

## 4. Expected Learning Outcomes & Mastery Criteria
Ability to write safe, efficient, interrupt-resilient hardware register drivers.
