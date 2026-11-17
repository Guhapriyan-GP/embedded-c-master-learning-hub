# Official STMicroelectronics Documentation Reference

This library catalogues the foundational reference documentation for the STM32G4 microcontroller family, the NUCLEO-G474RE development board, and comparative STM32 platforms.

---

### 1. Primary STM32G4 Documentation

#### A. STM32G4 Series Reference Manual (RM0440)
- **Document Number:** RM0440 (Rev 8 or latest)
- **Title:** *STM32G4 Series advanced Arm®-based 32-bit MCUs*
- **Scope:** 2,100+ pages of detailed peripheral hardware architecture, register maps, bit definitions, memory layouts, and reset values.
- **Key Chapters for Bare-Metal Firmware:**
  - Chapter 2: Memory and bus architecture (Bus matrix, CCM-SRAM, bit-banding)
  - Chapter 3: Embedded Flash memory (Wait states, latency, dual-bank organization)
  - Chapter 5: Power control (PWR) & Core Voltage Scaling (Range 1 Boost Mode)
  - Chapter 6: Reset and clock control (RCC) & PLL configuration
  - Chapter 9: General-purpose I/Os (GPIO) & Alternate function mapping
  - Chapter 11: Direct memory access controller (DMA) & DMAMUX router
  - Chapter 14: Extended interrupt and event controller (EXTI)
  - Chapter 21-29: General-purpose, basic, and advanced timers (TIM1, TIM2, TIM3, TIM4, TIM6, TIM7, TIM8)
  - Chapter 34: Universal synchronous/asynchronous receiver transmitter (USART/UART)
  - Chapter 35: Serial peripheral interface (SPI)
  - Chapter 36: Inter-integrated circuit interface (I2C)
  - Chapter 21: Analog-to-digital converters (ADC1 to ADC5) & Calibration

#### B. STM32G474xB/xC/xE Datasheet (DS12288)
- **Document Number:** DS12288 (Rev 6 or latest)
- **Title:** *Arm® Cortex®-M4 32-bit MCU+FPU, 512 KB Flash, 170 MHz/213 DMIPS, 128 KB SRAM*
- **Scope:** Electrical specifications, pinout descriptions, alternate function multiplexing table, operating temperature ranges, voltage ratings, and package pinouts.
- **Key Sections for Development:**
  - Table 13: *Pin definition table* (identifies default pin assignments and alternate function numbers AF0-AF15)
  - Table 14: *Alternate function mapping* (critical for configuring `GPIOx->AFR[0]` and `GPIOx->AFR[1]`)
  - Section 5: *Electrical characteristics* (I/O current limits, ADC input impedance, oscillator startup times)

#### C. STM32 Cortex-M4 Programming Manual (PM0214)
- **Document Number:** PM0214
- **Title:** *STM32 Cortex®-M4 MCUs and MPUs programming manual*
- **Scope:** Processor core programming model, instruction set, Nested Vectored Interrupt Controller (NVIC), System Control Block (SCB), SysTick timer, and Memory Protection Unit (MPU).

#### D. NUCLEO-64 Board User Manual (UM2505)
- **Document Number:** UM2505
- **Title:** *STM32G4 Nucleo-64 boards (MB1367)*
- **Scope:** Schematic diagrams, solder bridge configurations, power supply selection, onboard ST-LINK/V3E debugger interface, user LED (Green on `PA5`), and user pushbutton (Blue on `PC13`).

---

### 2. Comparative Family References
- **STM32F103 Reference Manual (RM0008):** Classic architecture, 72 MHz max, register differences in GPIO (CRL/CRH vs MODER).
- **STM32F446 Reference Manual (RM0390):** 180 MHz Cortex-M4, multi-layer AHB matrix, ART accelerator.
- **STM32G0 Series Reference Manual (RM0444):** Single-cycle I/O, simplified RCC, Cortex-M0+ core.
- **STM32H7 Series Reference Manual (RM0433):** 480 MHz dual-core Cortex-M7/M4, 64-bit AXI bus, L1 caches.
