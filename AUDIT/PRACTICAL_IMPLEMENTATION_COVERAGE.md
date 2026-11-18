# Practical Implementation & Verification Coverage Report

> **Strict Hardware Verification Honesty:**
> - **Compile-Verified (`arm-none-eabi-gcc 14.3.1`):** Starter template (`BUILD_TYPE=debug` and `BUILD_TYPE=release`), 4 reusable bare-metal drivers (`clock_g4`, `gpio_g4`, `ring_buffer`, `fault_handler`), and Labs `01–14` & `16–19` compiled cleanly with zero warnings under `-std=c11 -Wall -Wextra -Werror -Wpedantic -Wshadow -Wconversion -Wdouble-promotion -Wundef`.
> - **Host Compile- & Execution-Verified (`gcc 15.2.0`):** `tests/test_c_foundations.c` (**35/35 assertions passed**) and `tests/unit/test_ring_buffer.c` (**78/78 assertions passed**) — **113/113 host assertions passed, 0 failed**.
> - **Statically Reviewed (Not Compiled Against External Kernel):** `Lab 15` (FreeRTOS `ARM_CM4F`) and Capstone Projects `01–03` (FreeRTOS kernel sources are not vendored in-repo; Capstone projects are architectural blueprints).
> - **Physical Hardware Status:** **0 / 20 STM32 target labs have been executed on physical NUCLEO-G474RE (`MB1367`) silicon.** All register behaviors are verified against official CMSIS `stm32g474xx.h`, RM0440, DS12288, PM0214, UM2505, and ES0430.

---

## 1. Template Firmware & Reusable Driver Verification Matrix (`23-code-library/`)

| Artifact | Path | Verification Evidence | Measured Build / Test Result |
|---|---|---|---|
| **Starter Firmware Template (Debug)** | [`23-code-library/templates/Makefile`](../23-code-library/templates/Makefile) | **Compile-Verified** (`arm-none-eabi-gcc 14.3.1`, `-Og -g3`, `BUILD_TYPE=debug`) | `FLASH = 2,576 B` (0.49% of 512 KB), `RAM = 1,704 B` (1.73% of 96 KB), `CCMRAM = 12 B` (0.04% of 32 KB); 0 compiler warnings |
| **Starter Firmware Template (Release)** | [`23-code-library/templates/Makefile`](../23-code-library/templates/Makefile) | **Compile-Verified** (`arm-none-eabi-gcc 14.3.1`, `-O2 -g`, `BUILD_TYPE=release`) | `FLASH = 2,440 B` (0.47%), `RAM = 1,704 B` (1.73%), `CCMRAM = 12 B` (0.04%); `.isr_vector` = **472 B (118 vectors) @ `0x08000000`, 512 B aligned**; `.ccmram` = `12 B AX @ 0x10000000`; `.noinit` = `100 B NOBITS @ 0x20000040` |
| **CMake + Ninja Cross-Build** | [`23-code-library/templates/CMakeLists.txt`](../23-code-library/templates/CMakeLists.txt) | **Statically Reviewed** (`cmake/arm-none-eabi-gcc.cmake` exports `compile_commands.json`) | Ready for CMake ≥ 3.23 + Ninja cross-compilation |
| **170 MHz Clock & DWT Driver** | [`23-code-library/Drivers/clock_g4.c`](../23-code-library/Drivers/clock_g4.c) | **Compile-Verified** (`arm-none-eabi-gcc 14.3.1`) | Implements full RM0440 §6.1.5 `VOSF -> R1MODE=0 -> 4WS -> HPRE=/2 -> >=1 us -> HPRE=/1` + `DWT->CYCCNT` |
| **Atomic GPIO Driver** | [`23-code-library/Drivers/gpio_g4.c`](../23-code-library/Drivers/gpio_g4.c) | **Compile-Verified** (`arm-none-eabi-gcc 14.3.1`) | Glitch-free `AFR/OTYPER/OSPEEDR/PUPDR` before `MODER`, atomic `BSRR`/`BRR`, dummy RCC readback |
| **Lock-Free SPSC Ring Buffer** | [`23-code-library/Drivers/ring_buffer.c`](../23-code-library/Drivers/ring_buffer.c) | **Compile- & Host Execution-Verified** (`arm-none-eabi-gcc 14.3.1` + `gcc 15.2.0`) | Power-of-two unsigned wrap-safe indices + compiler/DMB barriers; **78/78 host assertions passed** |
| **HardFault Crash Logger** | [`23-code-library/Drivers/fault_handler.c`](../23-code-library/Drivers/fault_handler.c) | **Compile-Verified** (`arm-none-eabi-gcc 14.3.1`) | Naked-ASM MSP/PSP trampoline (`EXC_RETURN[2]`), `CFSR`/`HFSR`/`MMFAR`/`BFAR` capture to `.noinit` + CRC |
| **Stage 01 C Foundations Tests** | [`tests/test_c_foundations.c`](../tests/test_c_foundations.c) | **Host Compile- & Execution-Verified** (`gcc 15.2.0`, `-Werror -Wpedantic`) | **35/35 assertions passed** (two's complement, integer promotions, bitfields, endianness, unsigned wrap, struct padding) |
| **Ring Buffer Host Unit Tests** | [`tests/unit/test_ring_buffer.c`](../tests/unit/test_ring_buffer.c) | **Host Compile- & Execution-Verified** (`gcc 15.2.0`, `-Werror -Wpedantic`) | **78/78 assertions passed** (init, empty/full boundaries, wrap-around, bulk burst, overrun policy, reset) |

---

## 2. Complete Practical Laboratory Coverage Matrix (`18-practical-laboratory/Labs 01–20`)

| Lab | Title & Link | Primary Concept IDs Covered | API Comparison Included? | Verification Status | Hardware Status |
|---|---|---|---|---|---|
| **Lab 01** | [Bare-Metal GPIO LED Blinky](../18-practical-laboratory/lab-01-baremetal-gpio-led-blinky/README.md) | `PERIPH-GPIO-001`, `PERIPH-GPIO-003`, `MCU-PERIPH-001` | **Yes — 3-Way (Register / ST LL / ST HAL)** | Compile-Verified (`arm-none-eabi-gcc 14.3.1`) | Not Tested on Physical Hardware |
| **Lab 02** | [Non-Blocking 4-State SysTick FSM Debouncer](../18-practical-laboratory/lab-02-gpio-pushbutton-debouncing/README.md) | `PERIPH-GPIO-004`, `CS-FSM-001`, `FW-PAT-003` | Bare-Metal CMSIS | Compile-Verified (`arm-none-eabi-gcc 14.3.1`) | Not Tested on Physical Hardware |
| **Lab 03** | [RCC PLL 170 MHz Boost Mode Switch & MCO](../18-practical-laboratory/lab-03-rcc-pll-170mhz-clock-switch/README.md) | `STM32-RCC-001..002`, `CLK-PLL-001`, `CLK-MCO-001`, `STM32-PWR-001` | Bare-Metal CMSIS | Compile-Verified (`arm-none-eabi-gcc 14.3.1`) | Not Tested on Physical Hardware |
| **Lab 04** | [SysTick Accurate 1 ms Delay & Cooperative Scheduler](../18-practical-laboratory/lab-04-systick-accurate-delay-generator/README.md) | `ARM-SYST-001`, `C-INT-003`, `FW-EXEC-003` | Bare-Metal CMSIS | Compile-Verified (`arm-none-eabi-gcc 14.3.1`) | Not Tested on Physical Hardware |
| **Lab 05** | [EXTI13 Rising-Edge Interrupt & NVIC Grouping](../18-practical-laboratory/lab-05-exti-pushbutton-interrupt/README.md) | `PERIPH-EXTI-001..002`, `ARM-NVIC-001`, `RT-IRQ-001..002` | **Yes — 3-Way (Register / ST LL / ST HAL)** | Compile-Verified (`arm-none-eabi-gcc 14.3.1`) | Not Tested on Physical Hardware |
| **Lab 06** | [TIM2 10 Hz Periodic Interrupt (`rc_w0` & `EGR=UG`)](../18-practical-laboratory/lab-06-general-purpose-timer-periodic-interrupt/README.md) | `PERIPH-TIM-002`, `EMB-REG-003`, `RT-IRQ-002` | Bare-Metal CMSIS | Compile-Verified (`arm-none-eabi-gcc 14.3.1`) | Not Tested on Physical Hardware |
| **Lab 07** | [TIM3 25 kHz 3-Channel RGB Hardware PWM](../18-practical-laboratory/lab-07-timer-pwm-rgb-led-fading/README.md) | `PERIPH-PWM-001`, `PERIPH-TIM-002`, `PROF-STD-002` | Bare-Metal CMSIS | Compile-Verified (`arm-none-eabi-gcc 14.3.1`) | Not Tested on Physical Hardware |
| **Lab 08** | [USART2 SPSC Ring Buffer & Retargeted `printf`](../18-practical-laboratory/lab-08-usart-baremetal-driver-printf/README.md) | `PERIPH-UART-001..002`, `EMB-ATOM-004`, `TOOL-GCC-003` | **Yes — 3-Way (Register / ST LL / ST HAL)** | Compile-Verified (`arm-none-eabi-gcc 14.3.1`) | Not Tested on Physical Hardware |
| **Lab 09** | [ADC1 42.5 MHz Calibration & Single-Channel Read](../18-practical-laboratory/lab-09-adc-single-channel-potentiometer/README.md) | `PERIPH-ADC-001`, `PERIPH-ADC-002`, `EMB-REG-003` | Bare-Metal CMSIS | Compile-Verified (`arm-none-eabi-gcc 14.3.1`) | Not Tested on Physical Hardware |
| **Lab 10** | [Multichannel ADC1 + Circular Ping-Pong DMA1](../18-practical-laboratory/lab-10-adc-dma-multichannel-circular-buffer/README.md) | `PERIPH-DMA-001..003`, `DMA-CONT-001`, `PERIPH-ADC-002` | Bare-Metal CMSIS | Compile-Verified (`arm-none-eabi-gcc 14.3.1`) | Not Tested on Physical Hardware |
| **Lab 11** | [SPI1 10.625 MHz Full-Duplex Master (`FRXTH=1`)](../18-practical-laboratory/lab-11-spi-master-sensor-interfacing/README.md) | `PERIPH-SPI-001`, `HWSW-PCB-002` | Bare-Metal CMSIS | Compile-Verified (`arm-none-eabi-gcc 14.3.1`) | Not Tested on Physical Hardware |
| **Lab 12** | [I2C1 400 kHz Fast Mode, `RELOAD` & Bus Recovery](../18-practical-laboratory/lab-12-i2c-master-ssd1306-oled/README.md) | `PERIPH-I2C-001..003`, `DIG-ELEC-004` | Bare-Metal CMSIS | Compile-Verified (`arm-none-eabi-gcc 14.3.1`) | Not Tested on Physical Hardware |
| **Lab 13** | [Independent Watchdog (`IWDG`) & `DBGMCU` Freeze](../18-practical-laboratory/lab-13-iwdg-hardware-watchdog/README.md) | `RT-WDG-001`, `STM32-DBGMCU-001`, `STM32-RCC-003` | Bare-Metal CMSIS | Compile-Verified (`arm-none-eabi-gcc 14.3.1`) | Not Tested on Physical Hardware |
| **Lab 14** | [Stop 0 Low-Power Mode, EXTI13 Wakeup & `JP6` IDD](../18-practical-laboratory/lab-14-low-power-stop-mode-wakeup/README.md) | `STM32-PWR-002`, `RT-PWR-001`, `HWSW-PWR-001` | Bare-Metal CMSIS | Compile-Verified (`arm-none-eabi-gcc 14.3.1`) | **Not Tested on Physical Hardware (`NEEDS_VERIFICATION` for `JP6` µA)** |
| **Lab 15** | [FreeRTOS Producer-Consumer, Static Tasks & `FromISR`](../18-practical-laboratory/lab-15-freertos-multitask-producer-consumer/README.md) | `RTOS-CTX-001`, `RTOS-IPC-001`, `RTOS-ISR-001..002`, `RTOS-MEM-001` | FreeRTOS `ARM_CM4F` | **Statically Reviewed** (FreeRTOS kernel not vendored in-repo) | Not Tested on Physical Hardware |
| **Lab 16** | [DAC1 + OPAMP1 PGA + COMP1 → `TIM1_BKIN` Break](../18-practical-laboratory/lab-16-dac-opamp-comp-hardware-break/README.md) | `PERIPH-DAC-001`, `G4-OPAMP-001`, `G4-COMP-001`, `PERIPH-BDTR-001..002` | Bare-Metal CMSIS | Compile-Verified (`arm-none-eabi-gcc 14.3.1`) | Not Tested on Physical Hardware |
| **Lab 17** | [CORDIC & FMAC Hardware Math vs FPU `DWT->CYCCNT`](../18-practical-laboratory/lab-17-cordic-fmac-hardware-math/README.md) | `G4-CORDIC-001`, `G4-FMAC-001`, `RT-TIME-001`, `ARM-FPU-001` | Bare-Metal CMSIS | Compile-Verified (`arm-none-eabi-gcc 14.3.1`) | Not Tested on Physical Hardware |
| **Lab 18** | [FDCAN1 Loopback (500k/2M BRS) & Hardware CRC-32](../18-practical-laboratory/lab-18-fdcan-loopback-and-crc/README.md) | `G4-FDCAN-001`, `PERIPH-CAN-001`, `PERIPH-CRC-001` | Bare-Metal CMSIS | Compile-Verified (`arm-none-eabi-gcc 14.3.1`) | Not Tested on Physical Hardware |
| **Lab 19** | [8-Region MPU Stack Guard & Naked-ASM HardFault](../18-practical-laboratory/lab-19-hardfault-mpu-and-crash-diagnostics/README.md) | `ARM-MPU-001`, `ARM-FLT-001`, `REL-CRASH-001`, `MEM-NOINIT-001` | Bare-Metal CMSIS | Compile-Verified (`arm-none-eabi-gcc 14.3.1`) | Not Tested on Physical Hardware |
| **Lab 20** | [Host TDD, Fake CMSIS Structs & `-fstack-usage`](../18-practical-laboratory/lab-20-host-unit-testing-and-driver-mocking/README.md) | `TEST-UNIT-001..002`, `TEST-MOCK-001`, `PROF-STK-001`, `PROF-DA-002` | Host C11 + Fake CMSIS | **Compile- & Execution-Verified (`gcc 15.2.0`, 113/113 assertions)** | Host Execution Verified (N/A hardware) |

---

## 3. Capstone Projects Coverage (`19-projects/`)

| Project | Path | Scope & Remediated Defects | Implementation Status |
|---|---|---|---|
| **Project 01** | [`01-beginner-smart-instrument-panel/README.md`](../19-projects/01-beginner-smart-instrument-panel/README.md) | Resolved `PA0` pin conflict (`PA0=ADC1_IN1`, `PA5=TIM2_CH1`), fixed Active-HIGH `PC13` rising-edge EXTI, moved debouncing out of ISR to tick FSM | **PARTIAL** — Architectural blueprint & verified pinout complete; standalone multi-file `.c` build deferred |
| **Project 02** | [`02-intermediate-dma-data-logger/README.md`](../19-projects/02-intermediate-dma-data-logger/README.md) | Added `TS_CAL1`/`TS_CAL2` + `VREFINT_CAL` formulas, $t_{S\_\text{temp}} \ge 5\ \mu\text{s}$, 64-bit Dual-Bank RWW Flash logging + `DCRST` flush | **PARTIAL** — Architectural blueprint & register math complete; standalone multi-file `.c` build deferred |
| **Project 03** | [`03-advanced-freertos-digital-power-controller/README.md`](../19-projects/03-advanced-freertos-digital-power-controller/README.md) | Replaced unsafe software-task overcurrent polling with asynchronous hardware `COMP1 -> TIM1/HRTIM1 BKIN`; fixed Priority 0 Idle-task starvation (shifted tasks to Priorities 1–4) | **PARTIAL** — Safety-compliant architecture complete; standalone FreeRTOS power-stage build deferred |

---

## 4. Explicit Deferred Practical Labs (Honest Backlog)

To maintain strict transparency, the following topics have full Part 4 theory, register maps, and authoritative citations, but do **not** yet have a dedicated standalone compiled lab binary:
1. `G4-HRTIM-001` — Standalone `HRTIM1` 200 kHz DLL-calibrated PWM lab + oscilloscope 184 ps jitter verification.
2. `PERIPH-USB-001` & `CLK-CRS-001` — Full bare-metal USB FS Device HID/CDC enumeration stack with crystal-less HSI48 + CRS.
3. `G4-UCPD-001` — USB Type-C Power Delivery BMC/4b5b protocol stack lab.
4. `PERIPH-SAI-001` & `PERIPH-QSPI-001` — External I2S audio codec (SAI1) and external QSPI NOR Flash XIP (`0x90000000`) labs.
5. `PERIPH-LPTIM-001` & `RTOS-PWR-001` — Full vendored FreeRTOS `ARM_CM4F` build + `LPTIM1` Stop 1 Tickless Idle lab.
6. `BOOT-VERIF-001` & `BOOT-UPD-001` & `SEC-*` — Dual-image flashed Bank 1/Bank 2 A/B OTA bootloader with `BFB2` swap & Ed25519 signature check.
7. `TEST-HIL-001` & `TEST-HIL-002` — Automated `pytest`+`pyserial`+OpenOCD HIL harness on physical hardware and headless Renode/QEMU CI runner.
