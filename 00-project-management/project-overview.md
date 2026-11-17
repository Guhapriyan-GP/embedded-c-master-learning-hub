# Project Overview: Embedded C Master Learning Hub

## 1. Executive Summary
The **Embedded C Master Learning Hub** is an authoritative, deeply researched engineering curriculum and reference repository. It bridges the gap between high-level computer science theory and hardware-level microcontroller execution, focusing on bare-metal firmware development without reliance on bloated third-party abstractions.

The primary target platform is the **STMicroelectronics NUCLEO-G474RE** evaluation board featuring the **STM32G474RET6** microcontroller, an ARM Cortex-M4 core operating up to 170 MHz with an integrated Floating Point Unit (FPU) and Digital Signal Processing (DSP) instruction extensions.

## 2. Core Educational Philosophy
1. **Strict Dependency Order:** Concepts are taught only after their underlying electrical, mathematical, and architectural prerequisites are mastered (e.g., bitwise logic precedes register manipulation; memory maps precede linker scripts; clock trees precede baud rate dividers).
2. **Direct Register Transparency:** Learning begins with direct register access using CMSIS core headers to understand hardware mechanisms before examining higher-level abstraction libraries (such as HAL or LL).
3. **Engineering Rigor:** Every peripheral is studied alongside official manufacturer documentation (Reference Manual RM0440, Datasheet DS12288, and Cortex-M4 Programming Manual PM0214).
4. **Hands-On Verification:** Every theoretical principle is accompanied by concrete, compilable C code and debugging procedures utilizing GNU GDB, OpenOCD, and hardware SVD inspection in Visual Studio Code.

## 3. Comparative Architectural Context
While the primary development focus is on the STM32G4 family, the curriculum regularly contrasts architectural variations across the broader STM32 ecosystem:
- **STM32F103 (Cortex-M3):** Legacy bus layout, simple RCC structure, classical APB clock distribution.
- **STM32F446RE (Cortex-M4):** High-speed multi-layer AHB matrix, ART accelerator, traditional timer configurations.
- **STM32G0 series (Cortex-M0+):** Ultra-low-power, single-cycle I/O port, simplified NVIC structure.
- **STM32G4 series (Cortex-M4):** Advanced analog peripherals, Dual-Bank Flash, Core Voltage Boost mode, DMAMUX router.
- **STM32H7 series (Cortex-M7):** Multi-domain architecture (D1, D2, D3), L1 cache management, AXI bus matrix, complex memory coherency requirements.
