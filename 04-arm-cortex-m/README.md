# Stage 03: ARM Cortex-M4 Microarchitecture & Assembly

## 1. Prerequisites
Stage 02 (Memory-mapped I/O, C compilation pipeline).

## 2. Core Technical Theory & Engineering Principles
The STM32G474RE is powered by an ARM Cortex-M4 processor core implementing the ARMv7-M architecture.

### 1. Core Processor Registers
The core contains sixteen 32-bit registers:
- **R0 - R12:** General-purpose registers (R0-R3 pass function arguments; R4-R11 are callee-saved).
- **R13 (SP):** Stack Pointer. Cortex-M implements two banked stack pointers:
  - **Main Stack Pointer (MSP):** Used by default on reset, in Handler Mode (ISRs), and by the OS kernel.
  - **Process Stack Pointer (PSP):** Typically assigned to individual application threads under an RTOS.
- **R14 (LR):** Link Register. Stores the return address for function calls, or a special `EXC_RETURN` code during exception entry.
- **R15 (PC):** Program Counter. Holds the address of the currently executing instruction (always bit 0 = 1 for Thumb state).
- **xPSR:** Program Status Register (APSR flags: N, Z, C, V; IPSR: current active interrupt number; EPSR: Thumb execution bit).

### 2. Operating Modes & Privilege Levels
- **Thread Mode:** Normal application execution mode (entered on reset). Can run Privileged or Unprivileged.
- **Handler Mode:** Always Privileged. Entered automatically when executing an exception or interrupt service routine.
- **Privilege Transition:** An unprivileged task cannot grant itself privilege; it must invoke the `SVC` (Supervisor Call) exception to request operating system elevation.

### 3. Hardware Exception Stacking
When an interrupt fires, the Cortex-M hardware automatically pushes 8 registers onto the active stack in 12 clock cycles before entering the ISR:
$$	ext{Stack Frame: } [	ext{R0, R1, R2, R3, R12, LR, Return PC, xPSR}]$$
This hardware stacking enables fast, zero-overhead C interrupt handlers without assembly wrappers.

## 3. Practical Coding Exercises & Challenges
1. Inspect the stack frame during a breakpoint in an interrupt handler using GDB.
2. Write inline assembly to read the current value of the PSP stack pointer.
3. Analyze a HardFault using the Cortex-M Configurable Fault Status Register (CFSR).

## 4. Expected Learning Outcomes & Mastery Criteria
Deep understanding of core CPU registers, operating modes, exception stacking, and fault analysis.
