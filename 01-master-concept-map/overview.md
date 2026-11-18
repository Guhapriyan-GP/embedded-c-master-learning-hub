# Master Concept Map & Learning Architecture Overview

## 1. Structural Philosophy

This curriculum enforces a **strict, non-cyclical dependency hierarchy** tailored to the **STM32G474RET6 (ARM Cortex-M4F @ 170 MHz)** on the **NUCLEO-G474RE (MB1367)** board:
- **GPIO-on-HSI16 Before 170 MHz Clock Boost:** Learners bring up basic GPIO (`08-gpio/`, Labs 01–02) on the reset 16 MHz HSI16 clock first, so `PA5` (`LD2`) and `PA8` (`MCO`) are already mastered when verifying the 170 MHz PLL + Range 1 Boost + Flash 4WS switch (`07-rcc-and-clocks/`, Lab 03).
- **Interrupts & NVIC Strictly Before Timers:** Core exception stacking, SysTick, NVIC 4-bit priority grouping, and EXTI (`10-interrupts/`, Labs 04–05) precede hardware timer interrupts and PWM (`09-timers/`, Labs 06–07) so `TIM2_IRQHandler` has zero forward dependencies.
- **Honest Hardware Boundaries:** Every concept is explicitly classified by **Mastery Tier (Level 1 Foundational / Level 2 Production / Level 3 Specialist)** and architectural scope (**Generic C / ARMv7E-M Cortex-M4F / STM32G474RET6 LQFP64 Exact**), including silicon-vs-package exclusions (`GPIOE`, `SPI4`, `FMC` unbonded on LQFP64; `AES/CRYP` absent on G474 silicon).

---

## 2. Canonical Stage Progression & Folder Reconciliation

| Canonical Step | Tier | Physical Repo Folder | Source-of-Truth Concept IDs | Assigned Labs |
|---|---|---|---|---|
| **Stage 00** | L1 | *(Prerequisite Bridge)* | `DIG-ELEC-*`, `HWSW-PCB-*`, `HWSW-LAB-*` | MB1367 Schematic & Scope Bring-Up |
| **Stage 01** | L1 | `02-c-fundamentals/` | `C-TYPE-*`, `C-BIT-*`, `C-PTR-*`, `C-STRUCT-*`, `C-QUAL-*` | Exam 01 |
| **Stage 02** | L1 | `03-embedded-c/` | `EMB-MMIO-*`, `EMB-VOL-*`, `EMB-ATOM-*`, `EMB-CMSIS-*` | Exam 02 |
| **Stage 03** | L1–L2 | `04-arm-cortex-m/` | `ARM-CORE-*`, `ARM-ISA-*`, `ARM-MEM-*`, `ARM-ABI-*`, `ARM-EXC-*`, `ARM-FLT-*`, `ARM-FPU-*`, `ARM-MPU-*` | Exam 03 |
| **Stage 04** | L1–L2 | `05-toolchain-and-debugging/` | `TOOL-GCC-*`, `TOOL-LD-*`, `TOOL-OCD-*`, `ARM-DBG-*`, `PROF-STK-*`, `PROF-WCET-*` | Template Build & GDB/SWO |
| **Stage 05** | L1 | `06-stm32-fundamentals/` | `STM32-ARCH-*`, `STM32-SRAM-*`, `STM32-BOOT-*`, `STM32-ERRATA-*` | BusMatrix & `stm32g474xx.h` Audit |
| **Stage 06A** | L1 | `08-gpio/` *(study before 07!)* | `PERIPH-GPIO-001` | Lab 01 (Blinky), Lab 02 (Debounce) |
| **Stage 06B** | L2 | `07-rcc-and-clocks/` | `STM32-RCC-*`, `STM32-PWR-001`, `STM32-FLASH-001` | Lab 03 (170 MHz Boost + MCO) |
| **Stage 07** | L2 | `10-interrupts/` *(study before 09!)* | `ARM-NVIC-001`, `ARM-SYST-001`, `PERIPH-EXTI-001` | Lab 04 (SysTick), Lab 05 (EXTI B1) |
| **Stage 08** | L2–L3 | `09-timers/` | `PERIPH-TIM-001..003`, `PERIPH-LPTIM-001`, `PERIPH-HRTIM-001` | Lab 06 (TIM2 ISR), Lab 07 (TIM3 PWM) |
| **Stage 09** | L2–L3 | `11-communication/` | `PERIPH-UART/SPI/I2C/FDCAN/USB/UCPD/SAI/QSPI-001` | Labs 08 (USART), 11 (SPI), 12 (I2C) |
| **Stage 10** | L2–L3 | `12-adc-dac-analog/` | `PERIPH-ADC/DAC/OPAMP/COMP/VREFBUF-001` | Lab 09 (ADC 42.5 MHz) |
| **Stage 11** | L2–L3 | `13-dma/` | `PERIPH-DMA-001`, `PERIPH-CORDIC-001`, `PERIPH-FMAC-001` | Lab 10 (ADC+DMA) + Exam 04 |
| **Stage 12** | L2–L3 | `14-memory-and-storage/` | `STM32-FLASH-002`, `BOOT-*`, `SEC-*`, `PERIPH-CRC/RNG-001` | Dual-Bank RWW & Bootloader |
| **Stage 13** | L2 | `15-power-management/` | `STM32-PWR-002..003`, `PERIPH-WDG-*`, `PERIPH-RTC-001`, `REL-*` | Lab 13 (IWDG), Lab 14 (Stop 0/1) |
| **Stage 14** | L2–L3 | `16-rtos/` | `RTOS-*` | Lab 15 (FreeRTOS Producer-Consumer) |
| **Stage 15** | L2–L3 | `17-advanced-firmware-engineering/` | `PROF-*`, `TEST-*`, `BOOT-ARCH-001`, `REL-CRASH-001` | Exam 05 & Capstones (`19-projects/`) |

---

## 3. Parallel Specialization Tracks (Post-Stage 07)
Once Core Bring-Up and Interrupts/NVIC (Stages 00–07) are mastered, learners can branch into parallel tracks:
- **Track A (Power Electronics & Motion):** `TIM1/8/20` Complementary Dead-Time → `COMP1–7` Hardware Break → **`HRTIM1` (184 ps)** + **`FMAC`** 3p3z Digital Compensator + **`CORDIC`** FOC Trig.
- **Track B (Industrial & Automotive Comms):** `USART`/`LPUART` + `SPI` + `I2C` → **`FDCAN1–3`** (CAN-FD) + **USB FS + CRS** + **`QUADSPI` XIP**.
- **Track C (Safety, Security & Production Firmware):** Dual-Bank Bootloaders + `RDP`/`WRP`/`PCROP`/`SEC1R`/`TAMP` + FreeRTOS `ARM_CM4F` + `MPU` + MISRA C / CERT C / Unity / CI-CD.

See [`concept-map.mmd`](concept-map.mmd), [`dependency-graph.mmd`](dependency-graph.mmd), [`learning-sequence.md`](learning-sequence.md), and [`prerequisite-matrix.md`](prerequisite-matrix.md).
