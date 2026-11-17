# Sequential Learning Roadmap & Advancement Criteria

This document provides the exact order of study. A learner must not advance to a subsequent stage until the completion criteria and practical lab for the current stage are satisfied.

---

### Stage 01: Computer Science & C Language Foundations
- **Primary Goal:** Master binary data representations, two's complement, fixed-width integer semantics, bit manipulation, and pointer arithmetic.
- **Core Topics:** Preprocessor, compilation stages, bit masking (`&`, `|`, `^`, `~`, `<<`, `>>`), pointers, memory alignment, structs, padding, `const`, `volatile`, `static`, `extern`.
- **Laboratory:** `tests/test_c_foundations.c` (Standalone unit tests for bit packing and pointer offsets).
- **Completion Criteria:** Pass the Stage 01 Assessment with 100% accuracy on pointer arithmetic and bitmask derivation.

---

### Stage 02: Embedded C & Memory-Mapped Architecture
- **Primary Goal:** Translate C language structures into physical hardware register reads and writes.
- **Core Topics:** Memory-mapped I/O, peripheral base addresses, pointer typecasting to volatile structs, read-modify-write safety, atomicity, compiler optimization hazards.
- **Laboratory:** Direct register simulation and CMSIS struct mapping.
- **Completion Criteria:** Correctly explain why `volatile` is required for register pointers and demonstrate atomic bit-clearing using masks.

---

### Stage 03: ARM Cortex-M4 Microarchitecture & Assembly
- **Primary Goal:** Understand the processor core that executes the C code.
- **Core Topics:** ARMv7-M architecture, R0-R15 core registers, Program Status Register (xPSR), Main Stack Pointer (MSP) vs Process Stack Pointer (PSP), Thread vs Handler mode, Thumb-2 instruction set, load/store architecture, exception entry/exit sequence, NVIC vector table.
- **Laboratory:** Stepping through assembly instructions in GDB, inspecting register changes and stack frame generation.
- **Completion Criteria:** Manually trace an exception entry, identifying all 8 registers automatically pushed onto the stack.

---

### Stage 04: GNU Toolchain, Linker Scripts & Startup Code
- **Primary Goal:** Master the complete bare-metal build and boot process.
- **Core Topics:** GCC compilation pipeline, object files, ELF analysis (`readelf`, `objdump`, `nm`), linker script syntax (`MEMORY`, `SECTIONS`, `.text`, `.data`, `.bss`), VMA vs LMA, startup code (`Reset_Handler`), vector table definition, OpenOCD configuration, GDB scripts, SVD register visualization.
- **Laboratory:** Build and flash a bare-metal firmware project from scratch using only a Makefile and custom linker script.
- **Completion Criteria:** Successfully write a minimal `startup.c` that initializes `.data`, zeroes `.bss`, and calls `main()`.

---

### Stage 05: STM32 Platform Architecture & Documentation
- **Primary Goal:** Navigate official ST technical documentation independently.
- **Core Topics:** STM32 family landscape, STM32G4 microarchitecture, bus matrix (AHB, APB1, APB2), memory map address ranges (Flash, SRAM1, SRAM2, CCM-SRAM, Peripherals), Reference Manual (RM0440) structure, Datasheet electrical ratings, pin alternate function multiplexing.
- **Laboratory:** Locate and verify peripheral base addresses in RM0440 and correlate with `stm32g474xx.h`.
- **Completion Criteria:** Locate any peripheral register address and bitfield definition in RM0440 within two minutes.

---

### Stage 06: Power, Reset & Clock Control (RCC) Engine
- **Primary Goal:** Master microcontroller clock tree synthesis and system power modes.
- **Core Topics:** Internal oscillators (HSI16, LSI), External crystals (HSE, LSE), Phase-Locked Loop (PLL) configuration (M, N, R, Q, P dividers), SYSCLK selection, AHB/APB prescalers, Core Voltage Scaling Range 1 Boost Mode, Flash wait states (latency).
- **Laboratory:** `lab-03-rcc-pll-170mhz-clock-switch` (Configure STM32G4 to maximum rated 170 MHz).
- **Completion Criteria:** Correctly calculate and verify clock frequencies across SYSCLK, HCLK, PCLK1, and PCLK2 using register reads.

---

### Stage 07: General-Purpose I/O (GPIO) Hardware Engine
- **Primary Goal:** Master digital input and output hardware configurations.
- **Core Topics:** GPIO pin internal electrical circuit, Push-pull vs Open-drain, Pull-up/pull-down resistors, I/O speed ratings, MODER, ODR, IDR, BSRR atomic bit set/reset, Alternate Function registers (AFRL/AFRH).
- **Laboratory:** `lab-01-baremetal-gpio-led-blinky` and `lab-02-gpio-pushbutton-debouncing`.
- **Completion Criteria:** Implement atomic LED toggling and pushbutton state reading with reliable software debouncing.

---

### Stage 08: Interrupts, Exceptions & NVIC Vectoring
- **Primary Goal:** Master asynchronous hardware event handling.
- **Core Topics:** Polling vs Interrupts, NVIC architecture, Interrupt Set/Clear Enable (ISER/ICER), Priority Grouping (Preemption vs Subpriority), EXTI line routing via SYSCFG, ISR execution flow, interrupt latency, flag clearing, race conditions, critical sections.
- **Laboratory:** `lab-05-exti-pushbutton-interrupt`.
- **Completion Criteria:** Successfully trigger an ISR from an external button push, verify preemption priority, and handle flag clearance without re-entry bugs.

---

### Stage 09: General-Purpose & Advanced Timers (TIMx)
- **Primary Goal:** Master precise hardware timebases and PWM generation.
- **Core Topics:** Counter modes (Up, Down, Center-aligned), Prescaler (PSC), Auto-Reload Register (ARR), Counter Register (CNT), Update Event (UEV), Output Compare, Pulse Width Modulation (PWM), Input Capture, Dead-time insertion, Break inputs.
- **Laboratory:** `lab-06-general-purpose-timer-periodic-interrupt` and `lab-07-timer-pwm-rgb-led-fading`.
- **Completion Criteria:** Generate a clean 25 kHz PWM signal with dynamic duty cycle modulation verified on an oscilloscope or logic analyzer.

---

### Stage 10: Serial Communication Protocols (UART, SPI, I2C)
- **Primary Goal:** Establish reliable point-to-point and bus communication.
- **Core Topics:** Asynchronous framing, Baud rate divisor calculations (BRR), Interrupt-driven UART, SPI bus modes (CPOL/CPHA), Full-duplex shift registers, I2C bus dynamics, Start/Stop conditions, 7-bit addressing, TIMINGR setup, bus arbitration.
- **Laboratory:** `lab-08-usart-baremetal-driver-printf`, `lab-11-spi-master-sensor-interfacing`, and `lab-12-i2c-master-ssd1306-oled`.
- **Completion Criteria:** Retarget standard C `printf` through a bare-metal UART driver and communicate with an external I2C OLED display or SPI sensor.

---

### Stage 11: High-Speed Analog Signal Acquisition (ADC, DAC)
- **Primary Goal:** Digitize analog voltages and generate continuous analog signals.
- **Core Topics:** Successive Approximation Register (SAR) ADC architecture, Resolution, Quantization error, Sampling time ($t_{\text{SMP}}$), ADC internal calibration sequence, Regular vs Injected sequences, Scan mode, Continuous mode, Analog Watchdog, DAC voltage output.
- **Laboratory:** `lab-09-adc-single-channel-potentiometer`.
- **Completion Criteria:** Perform self-calibration, sample an analog input, and calculate the exact input voltage using fixed-point arithmetic.

---

### Stage 12: Direct Memory Access (DMA) & DMAMUX Architecture
- **Primary Goal:** Offload high-bandwidth data transfers from the CPU.
- **Core Topics:** DMA controller architecture, Bus masters, Memory-to-Memory, Peripheral-to-Memory, Memory-to-Peripheral, Circular mode, Double buffering, Half-Transfer and Transfer-Complete interrupts, DMAMUX request mapping on STM32G4.
- **Laboratory:** `lab-10-adc-dma-multichannel-circular-buffer`.
- **Completion Criteria:** Stream continuous multichannel ADC conversions into an SRAM buffer via DMA with zero CPU intervention during sampling.

---

### Stage 13: Embedded Memory Subsystems, Flash & Bootloaders
- **Primary Goal:** Master non-volatile storage and runtime firmware update mechanisms.
- **Core Topics:** Flash memory organization, Dual-bank architecture, Page erase and programming cycles, Flash endurance and retention, EEPROM emulation strategies, In-Application Programming (IAP), Custom bootloader design, Vector table relocation (`SCB->VTOR`).
- **Laboratory:** Flash page erase and write routine with data verification.
- **Completion Criteria:** Write a simple secondary bootloader that validates an application image checksum and branches to application space.

---

### Stage 14: System Reliability, Watchdogs & Low-Power Modes
- **Primary Goal:** Build fail-safe, power-optimized industrial systems.
- **Core Topics:** Independent Watchdog (IWDG) with dedicated LSI clock, Window Watchdog (WWDG) for execution window enforcement, Power Voltage Detector (PVD), Brown-Out Reset (BOR), Sleep, Stop 0/1/2, Standby, and Shutdown modes, Wakeup sources.
- **Laboratory:** `lab-13-iwdg-hardware-watchdog` and `lab-14-low-power-stop-mode-wakeup`.
- **Completion Criteria:** Configure an IWDG timeout and enter Stop mode, waking up cleanly via an external interrupt.

---

### Stage 15: Real-Time Operating Systems (FreeRTOS) & Concurrency
- **Primary Goal:** Architect deterministic, concurrent embedded software systems.
- **Core Topics:** Superloop limitations, Preemptive priority scheduling, Task Control Block (TCB), Context switching via SysTick and PendSV, Task states, Queues, Binary and Counting Semaphores, Mutexes, Priority Inversion, Priority Inheritance, `FromISR` APIs.
- **Laboratory:** `lab-15-freertos-multitask-producer-consumer`.
- **Completion Criteria:** Implement a multi-tasking application where high-priority sensor sampling tasks communicate with communication tasks via queues without starvation or priority inversion.

---

### Stage 16: Defensive Firmware Architecture & Verification
- **Primary Goal:** Produce robust, testable, production-ready embedded firmware.
- **Core Topics:** Layered firmware architecture (HAL/LL/Driver/Application), Finite State Machines (FSM), Circular ring buffers, Compile-time and runtime assertions, HardFault handlers with stack trace logging, MISRA C:2012 compliance, Unit testing embedded code.
- **Laboratory:** Capstone project implementation with static analysis validation.
- **Completion Criteria:** Complete a capstone project with zero compiler warnings under `-Wall -Wextra -Werror` and pass static analysis checks.
