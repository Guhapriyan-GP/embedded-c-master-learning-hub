# Stage 09: Interrupts, Exceptions & NVIC Vectoring

## 1. Prerequisites
Stage 08 (Hardware timers) & Stage 03 (ARM Cortex-M4 architecture).

## 2. Core Technical Theory & Engineering Principles
Interrupts allow microcontrollers to respond instantaneously to external and internal hardware events without wasting execution cycles in polling loops.

### 1. The Nested Vectored Interrupt Controller (NVIC)
The Cortex-M4 NVIC provides deterministic interrupt latency (12 clock cycles):
- **Vector Table:** Array of 32-bit function pointers located at the base of memory (`0x08000000`).
- **Priority Grouping:** Configures priority registers into Preemption Priority (determines which ISR can interrupt another) and Subpriority (resolves simultaneous triggers).
- **Tail-Chaining:** When an interrupt finishes and another is pending, the processor skips unstacking and stacking, executing the next ISR in just 6 cycles.

### 2. External Interrupts (EXTI)
External pin events route through the EXTI controller:
1. Select pin source port in `SYSCFG->EXTICRx`.
2. Configure edge detection in `EXTI->RTSR1` (rising) or `EXTI->FTSR1` (falling).
3. Unmask interrupt line in `EXTI->IMR1`.
4. Enable IRQ in NVIC using `NVIC_EnableIRQ()`.
5. **Critical Requirement:** The ISR must clear the pending bit in `EXTI->PR1` by writing a `1` to prevent immediate re-triggering.

## 3. Practical Coding Exercises & Challenges
1. Demonstrate interrupt preemption between a low-priority timer and a high-priority button ISR.
2. Measure interrupt latency using an output pin toggled at the start of an ISR.
3. Implement a reentrant critical section protecting shared memory variables.

## 4. Expected Learning Outcomes & Mastery Criteria
Mastery of low-latency interrupt design, NVIC priority grouping, and thread-safe data sharing.
