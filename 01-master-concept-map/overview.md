# Master Concept Map & Learning Architecture Overview

## 1. Structural Philosophy
The curriculum follows a strict, non-cyclical dependency hierarchy. In embedded systems, jumping ahead without mastering foundational mechanics causes fatal bugs that are exceedingly difficult to diagnose in hardware:
- Writing to a peripheral register before understanding the peripheral bus clock results in a silent CPU freeze or BusFault.
- Implementing an interrupt handler without understanding volatile semantics or atomic bit manipulation leads to intermittent race conditions.
- Configuring a 170 MHz system clock without adjusting Flash memory wait states causes core instruction fetch corruption.
- Implementing an RTOS queue without understanding pointer lifetimes leads to silent memory corruption and hard-to-trace crashes.

## 2. Global Stage Progression

```
[Phase 1: Foundations]
  Stage 01: Computer Science & C Language Foundations
  Stage 02: Embedded C & Memory-Mapped Architecture
  Stage 03: ARM Cortex-M4 Microarchitecture & Assembly
  Stage 04: GNU Toolchain, Linker Scripts & Startup Code

[Phase 2: Core STM32 Engine]
  Stage 05: STM32 Platform Architecture & Documentation Navigation
  Stage 06: Power, Reset & Clock Control (RCC) Engine
  Stage 07: General-Purpose I/O (GPIO) Hardware Engine
  Stage 08: Interrupts, Exceptions & NVIC Vectoring

[Phase 3: Peripherals & Data Transport]
  Stage 09: General-Purpose & Advanced Timers (TIMx)
  Stage 10: Asynchronous & Synchronous Serial Communication (UART, SPI, I2C)
  Stage 11: High-Speed Analog Signal Acquisition (ADC, DAC, OPAMP, COMP)
  Stage 12: Direct Memory Access (DMA) & DMAMUX Architecture

[Phase 4: Systems, Concurrency & Production Architecture]
  Stage 13: Embedded Memory Subsystems, Flash & Bootloaders
  Stage 14: System Reliability, Watchdogs (IWDG, WWDG) & Low-Power Modes
  Stage 15: Real-Time Operating Systems (FreeRTOS) & Concurrency
  Stage 16: Defensive Firmware Architecture, Testing & Verification
```

## 3. Parallel Learning Tracks
Once the core platform is mastered (through Stage 08), the curriculum provides independent parallel learning branches:
- **Branch A (Timing & Motion):** Stage 09 (Timers & PWM) $\rightarrow$ Motor control & encoder interfaces.
- **Branch B (Digital Communication):** Stage 10 (Serial buses) $\rightarrow$ Sensor networking & industrial protocols (CAN FD).
- **Branch C (Mixed-Signal Acquisition):** Stage 11 (Analog) & Stage 12 (DMA) $\rightarrow$ High-speed digital filtering and DSP.
All branches converge at Stage 15 (Real-Time Operating Systems) and Stage 16 (Production Firmware Architecture).
