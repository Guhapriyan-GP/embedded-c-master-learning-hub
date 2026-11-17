# Embedded C Master Learning Hub
## Complete Bare-Metal Firmware Engineering Curriculum for ARM Cortex-M4 (STM32G474RE)

Welcome to the **Embedded C Master Learning Hub**, a comprehensive, rigorous, and production-grade curriculum designed to take engineers from computer programming foundations to advanced bare-metal embedded firmware architecture.

---

### Platform Specifications
- **Target Microcontroller:** STM32G474RET6 (ARM Cortex-M4 with FPU and DSP instructions)
- **Target Evaluation Board:** NUCLEO-G474RE
- **Core Frequency:** Up to 170 MHz (with Core Voltage Scaling Range 1 Boost Mode)
- **Memory Subsystem:** 512 KB Dual-Bank Flash, 128 KB SRAM (including 32 KB CCM-SRAM)
- **Toolchain:** ARM GNU Toolchain (`arm-none-eabi-gcc`, `arm-none-eabi-gdb`)
- **Development Environment:** Visual Studio Code with Cortex-Debug, SVD register inspection, and GNU Make
- **Firmware Methodology:** Bare-Metal Register Programming using standard CMSIS core headers, transitioning to modular layered drivers and real-time concurrency.

---

### Curriculum Architecture

The curriculum is structured into sequential stages that strictly follow technical dependencies:

```
[ Stage 01: Computer Science & C Language Foundations ]
                         │
                         ▼
[ Stage 02: Embedded C & Memory-Mapped Architecture ]
                         │
                         ▼
[ Stage 03: ARM Cortex-M4 Microarchitecture & Assembly ]
                         │
                         ▼
[ Stage 04: GNU Toolchain, Linker Scripts & Startup Code ]
                         │
                         ▼
[ Stage 05: STM32 Platform Architecture & Documentation Navigation ]
                         │
                         ▼
[ Stage 06: Power, Reset & Clock Control (RCC) Engine ]
                         │
                         ▼
[ Stage 07: General-Purpose I/O (GPIO) Hardware Engine ]
                         │
                         ▼
[ Stage 08: Interrupts, Exceptions & NVIC Vectoring ]
                         │
                         ▼
[ Stage 09: General-Purpose & Advanced Timers (TIMx) ]
                         │
                         ▼
[ Stage 10: Asynchronous & Synchronous Serial Communication (UART, SPI, I2C) ]
                         │
                         ▼
[ Stage 11: High-Speed Analog Signal Acquisition (ADC, DAC, OPAMP, COMP) ]
                         │
                         ▼
[ Stage 12: Direct Memory Access (DMA) & DMAMUX Architecture ]
                         │
                         ▼
[ Stage 13: Embedded Memory Subsystems, Flash & Bootloaders ]
                         │
                         ▼
[ Stage 14: System Reliability, Watchdogs (IWDG, WWDG) & Low-Power Modes ]
                         │
                         ▼
[ Stage 15: Real-Time Operating Systems (FreeRTOS) & Concurrency ]
                         │
                         ▼
[ Stage 16: Defensive Firmware Architecture & Verification ]
```

---

### Repository Directory Layout

- `00-project-management/`: Curriculum governance, requirements traceability, and quality audit records.
- `01-master-concept-map/`: Complete Mermaid dependency graphs, prerequisite matrices, and learning sequences.
- `02-c-fundamentals/` through `17-advanced-firmware-engineering/`: The sixteen technical learning stages.
- `18-practical-laboratory/`: 15 reproducible bare-metal laboratory experiments for the NUCLEO-G474RE board.
- `19-projects/`: Three end-to-end practical capstone projects (instrument panel, DMA logger, RTOS digital controller).
- `20-technical-diagrams/`: Mermaid diagrams, hardware architectures, and detailed register bitfield maps.
- `21-reference-library/`: Exhaustive technical bibliography (ST documentation, ARM manuals, textbooks, standards).
- `22-video-library/`: 50 individually verified educational video lectures mapped to topics.
- `23-code-library/`: Production-grade build templates (Makefile, linker script, startup code, register drivers).
- `24-assessments/`: Five comprehensive mastery examinations.
- `25-glossary/`: Embedded systems engineering terms and register abbreviations.
- `scripts/`: Python utilities for repository verification, link validation, and automated compilation.
