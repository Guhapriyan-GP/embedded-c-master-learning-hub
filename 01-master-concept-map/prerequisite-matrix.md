# Concept-by-Concept Prerequisite Matrix

This matrix maps core embedded concepts (tagged by **Mastery Tier** and **Source-of-Truth Concept ID**) to their mandatory prerequisites. Skipping a prerequisite causes predictable, hard-to-diagnose hardware and concurrency failures.

| Target Concept (Concept ID) | Tier | Mandatory Prerequisites | Why the Prerequisite is Essential |
|---|---|---|---|
| **Memory-Mapped Registers** (`EMB-MMIO-001`) | L1 | `C-PTR-001`, `C-STRUCT-001`, Hexadecimal | Peripheral registers are accessed via volatile struct pointers anchored at absolute 32-bit addresses. |
| **Atomic Bit Manipulation** (`EMB-ATOM-001`) | L1 | `C-BIT-001`, `EMB-VOL-001`, RMW Hazards | Multi-cycle `LDR`/`ORR`/`STR` on `ODR` races with ISRs; `BSRR`/`BRR`, `rc_w1`, and `rc_w0` require direct writes. |
| **Core Architecture & Faults** (`ARM-CORE-*`, `ARM-FLT-001`) | L1–L2 | `EMB-MMIO-001`, `ARM-ABI-002`, `ARM-EXC-001` | Decoding `HardFault` requires understanding 8-word vs. 26-word FPU stack frames, `MSP`/`PSP`, and `CFSR`/`HFSR`. |
| **Linker Scripts & Startup** (`TOOL-LD-001`, `ARM-EXC-003`) | L1–L2 | `ARM-MEM-001`, `.text`/`.data`/`.bss` | C runtime demands `.data` and `.ccmram` LMA→VMA copy, `.bss` zeroing, and a 512-byte aligned 118-entry vector table. |
| **Basic GPIO on HSI16** (`PERIPH-GPIO-001`) | L1 | `DIG-ELEC-001`, `EMB-ATOM-001`, `RCC->AHB2ENR` | GPIO defaults to `11` Analog on STM32G4; writes are ignored without AHB2 clock enable, and `B1` (`PC13`) is Active-HIGH. |
| **170 / 160 MHz Clock Tree (RCC)** (`STM32-RCC-002`, `STM32-PWR-001`) | L2 | `PERIPH-GPIO-001`, `STM32-FLASH-001` | Switching >150 MHz (160 or 170 MHz) requires `PWR_CR5` Boost (`VOSF==0`), Flash 4WS, `HPRE=/2` ($\ge 1\ \mu\text{s}$), and GPIO `PA8` MCO verification. |
| **NVIC, SysTick & EXTI** (`ARM-NVIC-001`, `PERIPH-EXTI-001`) | L2 | `ARM-EXC-001`, `PERIPH-GPIO-001`, `STM32-RCC-002` | Asynchronous IRQs require `SYSCFG` clock enable, 4-bit NVIC priority grouping, direct `PR1` clear, and ARM AN321 peripheral register readback + `__DSB()`. |
| **Timers, PWM & Dead-Time** (`PERIPH-TIM-001..003`) | L2 | `STM32-RCC-002`, `PERIPH-EXTI-001`, `PERIPH-GPIO-001` | Timer ISRs (`TIM2_IRQn`) require NVIC mastery first; Adv timers require `BDTR.MOE=1` and `DTG` non-overlap math. |
| **UART / SPI / I2C Drivers** (`PERIPH-UART/SPI/I2C-001`) | L2 | `STM32-RCC-002`, `PERIPH-GPIO-001`, `DIG-ELEC-001` | `USART_BRR` and `I2C_TIMINGR` depend on exact kernel clocks; SPI 8-bit requires `FRXTH=1`; I2C requires external pull-ups. |
| **ADC & DAC Mixed-Signal** (`PERIPH-ADC-001`, `PERIPH-DAC-001`) | L2 | `STM32-RCC-002`, `PERIPH-GPIO-001`, `DIG-ELEC-001` | ADC clock MUST stay $\le 60\ \text{MHz}$ (`CKMODE=11` `HCLK/4`); DAC on AHB2 requires `HFSEL=10` at 170 MHz. |
| **DMA & DMAMUX1** (`PERIPH-DMA-001`) | L2 | `ARM-MEM-001`, `PERIPH-EXTI-001`, `PERIPH-ADC-001` | Requires `DMAMUX1EN`, RM0440 Table 91 IDs, individual flag clears (ES0430), and `0x20018000` alias for CCM-SRAM. |
| **HRTIM1, FDCAN, CORDIC, FMAC** (`PERIPH-*-001`) | L3 | `PERIPH-TIM-003`, `PERIPH-DMA-001`, `C-TYPE-001` | Flagship G474RE engines rely on DLL calibration, fixed Message RAM offsets, and Q1.15/Q1.31 fixed-point scaling. |
| **Dual-Bank Flash & Bootloaders** (`BOOT-*`, `SEC-*`) | L2–L3 | `TOOL-LD-001`, `STM32-FLASH-002`, `ARM-EXC-003` | Safe IAP/bootloading requires 64-bit aligned ECC writes, DCache flush, full peripheral teardown, and 512-byte `VTOR`. |
| **Watchdogs & Low-Power Modes** (`REL-*`, `STM32-PWR-002`) | L2 | `STM32-RCC-002`, `PERIPH-EXTI-001`, `ARM-PWR-001` | Stop 0/1 wakeup reverts SYSCLK to HSI16 and requires EXTI ISR + clearing `SLEEPDEEP`; current measured on `JP6`. |
| **FreeRTOS `ARM_CM4F`** (`RTOS-*`) | L2–L3 | `ARM-CORE-004`, `ARM-NVIC-001`, `ARM-FPU-001` | Context switching depends on `PSP`/`MSP`, `PendSV`, `BASEPRI` (`configMAX_SYSCALL_INTERRUPT_PRIORITY`), and FPU lazy stacking. |
| **MPU, MISRA C & Unit Testing** (`ARM-MPU-001`, `PROF-*`, `TEST-*`) | L2–L3 | `C-FPTR-001`, `ARM-FLT-001`, `TOOL-GCC-001` | Test seams use vtables/link substitution; MPU traps stack overflows into `MemManage`; CI enforces `-Werror` & `cppcheck`. |
