# Stage 02 Mastery Exam: Embedded C & Register Manipulation

### Questions (100 Points Total)
1. **Volatile Semantics (20 Points):** Explain the exact machine code difference between accessing a non-volatile pointer versus a volatile pointer in a loop. When does the compiler generate a `LDR` instruction?
2. **Read-Modify-Write Hazards (20 Points):** Why is `GPIOA->ODR |= (1 << 5)` non-atomic? Trace the assembly instructions generated and demonstrate how an interrupt occurring between read and write causes data corruption.
3. **Atomic Bit Manipulation (20 Points):** Explain the internal hardware operation of the `BSRR` register. Why is writing `1` to `BS5` atomic and why does writing `0` have no effect?
4. **Memory-Mapped Macro Derivation (20 Points):** Starting from `PERIPH_BASE = 0x40000000UL`, show how CMSIS defines `GPIOA` as a pointer to `GPIO_TypeDef`. Write out the complete C `#define` chain.
5. **Critical Sections (20 Points):** Write a standard CMSIS code snippet to implement a reentrant critical section using `__get_PRIMASK()` and `__set_PRIMASK()`.
