# Verified Educational Video Research Database

This directory contains **70 verified educational video lectures** (50 baseline + 20 newly live-verified additions covering bare-metal linker/startup, HardFault/MPU, DMA, STM32G4 HRTIM/CORDIC/FMAC/FDCAN, Bootloaders, Low-Power/Watchdogs, Unity/Ceedling, MISRA C, Cortex-M PendSV, FSM/HSM, and CoreSight SWO/ITM/DWT) supporting the Embedded C and STM32 Bare-Metal curriculum. Every video URL, HTTP 200 status, `playabilityStatus: OK`, duration, channel name, publish date, and topic mapping has been programmatically live-verified.

---

## Category: C Fundamentals

### Video 1: you will never ask about pointers again after watching this video
- **Instructor / Channel:** Low Level
- **Direct Link:** [https://www.youtube.com/watch?v=2ybLD6_2gKM](https://www.youtube.com/watch?v=2ybLD6_2gKM)
- **Platform:** YouTube
- **Duration:** 8m 3s | **Published Date:** 2022-06-19
- **Curriculum Alignment:** Stage - *Pointers & Memory*
- **Prerequisites:** Basic C syntax
- **Educational Value:** Unravels the mystery of pointers with visual memory representation and address dereferencing.
- **Concepts Not Covered (Gap Analysis):** Does not cover function pointers or volatile qualifiers.

## Category: ARM Architecture

### Video 2: you can learn assembly in 10 minutes (try it RIGHT NOW)
- **Instructor / Channel:** Low Level
- **Direct Link:** [https://www.youtube.com/watch?v=6S5KRJv-7RU](https://www.youtube.com/watch?v=6S5KRJv-7RU)
- **Platform:** YouTube
- **Duration:** 9m 48s | **Published Date:** 2023-05-08
- **Curriculum Alignment:** Stage - *Assembly Language*
- **Prerequisites:** Binary numbers
- **Educational Value:** Fast, high-impact overview of registers, load/store instructions, and assembly execution.
- **Concepts Not Covered (Gap Analysis):** Does not cover ARM Thumb-2 specific dual-instruction width or IT blocks.

## Category: C Fundamentals

### Video 3: Understanding and Using Function Pointers in C
- **Instructor / Channel:** Jacob Sorber
- **Direct Link:** [https://www.youtube.com/watch?v=axngwDJ79GY](https://www.youtube.com/watch?v=axngwDJ79GY)
- **Platform:** YouTube
- **Duration:** 4m 8s | **Published Date:** 2017-03-29
- **Curriculum Alignment:** Stage - *Function Pointers*
- **Prerequisites:** Pointers, Functions
- **Educational Value:** Explains syntax, function pointer tables, callback architectures, and dynamic dispatching in C.
- **Concepts Not Covered (Gap Analysis):** Does not discuss hardware interrupt vector tables.

## Category: Embedded C

### Video 4: Are Pointers Bad for Embedded Systems?
- **Instructor / Channel:** Jacob Sorber
- **Direct Link:** [https://www.youtube.com/watch?v=Bd-xNEOXohs](https://www.youtube.com/watch?v=Bd-xNEOXohs)
- **Platform:** YouTube
- **Duration:** 5m 7s | **Published Date:** 2022-06-14
- **Curriculum Alignment:** Stage - *Pointers in Embedded*
- **Prerequisites:** Pointers
- **Educational Value:** Addresses real-world trade-offs, pointer arithmetic, memory bounds, and defensive embedded pointer usage.
- **Concepts Not Covered (Gap Analysis):** Does not cover peripheral base address typecasting (`(GPIO_TypeDef*)`).

### Video 5: How to use the volatile keyword in C?
- **Instructor / Channel:** Jacob Sorber
- **Direct Link:** [https://www.youtube.com/watch?v=6tIWFEzzx9I](https://www.youtube.com/watch?v=6tIWFEzzx9I)
- **Platform:** YouTube
- **Duration:** 6m 48s | **Published Date:** 2019-12-14
- **Curriculum Alignment:** Stage - *Volatile Qualifier*
- **Prerequisites:** Variables, Compiler flags
- **Educational Value:** Demonstrates compiler optimization bugs when reading hardware registers and shows why volatile is mandatory.
- **Concepts Not Covered (Gap Analysis):** Does not cover memory barrier instructions (DMB, DSB, ISB).

## Category: C Fundamentals

### Video 6: Struct packing: No, you're not going crazy.
- **Instructor / Channel:** Jacob Sorber
- **Direct Link:** [https://www.youtube.com/watch?v=grk4QUDveFw](https://www.youtube.com/watch?v=grk4QUDveFw)
- **Platform:** YouTube
- **Duration:** 2m 46s | **Published Date:** 2018-06-12
- **Curriculum Alignment:** Stage - *Struct Alignment & Padding*
- **Prerequisites:** Structures, Data types
- **Educational Value:** Visually proves 32-bit boundary alignment, packing pragmas, and padding overhead in C structs.
- **Concepts Not Covered (Gap Analysis):** Does not cover bitfield endianness issues across different compilers.

### Video 7: Structs in C | What you Need to Know
- **Instructor / Channel:** Caleb Curry
- **Direct Link:** [https://www.youtube.com/watch?v=IAvfAC4H_0s](https://www.youtube.com/watch?v=IAvfAC4H_0s)
- **Platform:** YouTube
- **Duration:** 24m 39s | **Published Date:** 2023-04-06
- **Curriculum Alignment:** Stage - *Structs & Memory Layout*
- **Prerequisites:** Basic C types
- **Educational Value:** Comprehensive overview of defining, initializing, and accessing structs and pointers to structs.
- **Concepts Not Covered (Gap Analysis):** Focuses on desktop C rather than hardware register struct overlays.

## Category: Embedded C

### Video 8: STM32 Short #11 - EXCITING C feature - Bit Fields (or  bit packing)
- **Instructor / Channel:** STM32World
- **Direct Link:** [https://www.youtube.com/watch?v=KpaQ3TvN19g](https://www.youtube.com/watch?v=KpaQ3TvN19g)
- **Platform:** YouTube
- **Duration:** 17m 48s | **Published Date:** 2026-01-14
- **Curriculum Alignment:** Stage - *Bit-Fields & Bit Packing*
- **Prerequisites:** Bitwise operators, Structs
- **Educational Value:** Shows how bitfields map directly to MCU peripheral register fields.
- **Concepts Not Covered (Gap Analysis):** Does not cover non-atomic read-modify-write hazards with bitfields.

## Category: Architecture

### Video 9: Bistable 555 - 8-bit computer clock - part 3
- **Instructor / Channel:** Ben Eater
- **Direct Link:** [https://www.youtube.com/watch?v=WCwJNnx36Rk](https://www.youtube.com/watch?v=WCwJNnx36Rk)
- **Platform:** YouTube
- **Duration:** 10m 31s | **Published Date:** 2016-03-17
- **Curriculum Alignment:** Stage - *Clock & Bus Architecture*
- **Prerequisites:** Digital logic basics
- **Educational Value:** Builds intuition for astable clock circuits, clock gating, and synchronization.
- **Concepts Not Covered (Gap Analysis):** Uses 555 discrete logic rather than integrated PLL clock trees.

### Video 10: Testing our computer's registers - 8-bit register - Part 5
- **Instructor / Channel:** Ben Eater
- **Direct Link:** [https://www.youtube.com/watch?v=9WE3Obdjtv0](https://www.youtube.com/watch?v=9WE3Obdjtv0)
- **Platform:** YouTube
- **Duration:** 10m 53s | **Published Date:** 2016-05-02
- **Curriculum Alignment:** Stage - *CPU Registers & Data Bus*
- **Prerequisites:** Flip-flops, Latches
- **Educational Value:** Physical demonstration of tri-state buffers, register output enable, and bus routing.
- **Concepts Not Covered (Gap Analysis):** Focuses on 8-bit architecture rather than 32-bit ARM Cortex-M bus matrices.

## Category: Architecture & Embedded C

### Video 11: Embedded Systems Programming Lesson 2  Flow of Control
- **Instructor / Channel:** Niyazi Saral
- **Direct Link:** [https://www.youtube.com/watch?v=lYUq-TTpZFE](https://www.youtube.com/watch?v=lYUq-TTpZFE)
- **Platform:** YouTube
- **Duration:** 11m 0s | **Published Date:** 2014-01-21
- **Curriculum Alignment:** Stage - *Control Flow & Program Counter*
- **Prerequisites:** Basic C loops
- **Educational Value:** Steps into assembly in the debugger to show branch instructions, condition codes, and the PC register.
- **Concepts Not Covered (Gap Analysis):** Does not discuss hardware branch prediction or pipelining in advanced Cortex-M.

## Category: Embedded C

### Video 12: #3 Variables and Pointers
- **Instructor / Channel:** Quantum Leaps, LLC
- **Direct Link:** [https://www.youtube.com/watch?v=o9WpXYBqdPU](https://www.youtube.com/watch?v=o9WpXYBqdPU)
- **Platform:** YouTube
- **Duration:** 12m 22s | **Published Date:** 2013-01-20
- **Curriculum Alignment:** Stage - *Variables, Pointers & Memory Map*
- **Prerequisites:** Pointers
- **Educational Value:** Maps C local and static variables to RAM addresses and processor registers.
- **Concepts Not Covered (Gap Analysis):** Does not cover heap allocation or dynamic memory managers.

## Category: STM32 & GPIO

### Video 13: #4 How to control the world outside?
- **Instructor / Channel:** Quantum Leaps, LLC
- **Direct Link:** [https://www.youtube.com/watch?v=1Kjh0CAgnl4](https://www.youtube.com/watch?v=1Kjh0CAgnl4)
- **Platform:** YouTube
- **Duration:** 16m 45s | **Published Date:** 2013-01-28
- **Curriculum Alignment:** Stage - *Bare-Metal GPIO Control*
- **Prerequisites:** Memory-mapped I/O
- **Educational Value:** Connects schematic pinouts to datasheet memory maps and writes to GPIO registers in debugger.
- **Concepts Not Covered (Gap Analysis):** Uses TivaC Launchpad as hardware demo; register offsets differ from STM32.

## Category: Embedded C

### Video 14: #6 Bit-wise operators in C
- **Instructor / Channel:** Quantum Leaps, LLC
- **Direct Link:** [https://www.youtube.com/watch?v=7Iru_LM3qY0](https://www.youtube.com/watch?v=7Iru_LM3qY0)
- **Platform:** YouTube
- **Duration:** 17m 6s | **Published Date:** 2013-03-07
- **Curriculum Alignment:** Stage - *Bit-wise Operations in C*
- **Prerequisites:** Binary arithmetic
- **Educational Value:** Deep dive into AND, OR, XOR, NOT, shift operators and their assembly counterparts.
- **Concepts Not Covered (Gap Analysis):** Does not cover BSRR atomic bit set/reset registers available in STM32.

### Video 15: #7 Arrays and Pointer Arithmetic
- **Instructor / Channel:** Quantum Leaps, LLC
- **Direct Link:** [https://www.youtube.com/watch?v=pQs8vp7JOSk](https://www.youtube.com/watch?v=pQs8vp7JOSk)
- **Platform:** YouTube
- **Duration:** 15m 22s | **Published Date:** 2013-04-03
- **Curriculum Alignment:** Stage - *Arrays & Pointer Arithmetic*
- **Prerequisites:** Pointers, Arrays
- **Educational Value:** Explains machine-level pointer scaling (`sizeof`) and mapping consecutive register blocks.
- **Concepts Not Covered (Gap Analysis):** Does not discuss DMA multi-buffer pointer indexing.

## Category: ARM Architecture

### Video 16: #10 Stack Overflow and Other Pitfalls of Functions
- **Instructor / Channel:** Quantum Leaps, LLC
- **Direct Link:** [https://www.youtube.com/watch?v=jmzvued3w3Y](https://www.youtube.com/watch?v=jmzvued3w3Y)
- **Platform:** YouTube
- **Duration:** 27m 42s | **Published Date:** 2013-10-30
- **Curriculum Alignment:** Stage - *Call Stack & Stack Overflow*
- **Prerequisites:** Functions, Stack memory
- **Educational Value:** Dissects function activation records, stack frames, SP, LR, and detecting stack overflow bugs.
- **Concepts Not Covered (Gap Analysis):** Does not cover MPU (Memory Protection Unit) hardware stack protection.

## Category: Toolchain & Bare-Metal

### Video 17: STM32 Bare Metal Tutorial: Creating Your First Project & LED Blink  | STM32 Bare Metal Tutorial #1
- **Instructor / Channel:** STM32World
- **Direct Link:** [https://www.youtube.com/watch?v=m1C2LlFRwY4](https://www.youtube.com/watch?v=m1C2LlFRwY4)
- **Platform:** YouTube
- **Duration:** 53m 6s | **Published Date:** 2026-02-01
- **Curriculum Alignment:** Stage - *Bare-Metal Setup & LED Blink*
- **Prerequisites:** C basics, Terminal
- **Educational Value:** Zero-library bare-metal bootstrap from scratch with Makefile and direct register writes.
- **Concepts Not Covered (Gap Analysis):** Does not include CMSIS header definitions (uses manual address `#define`).

## Category: RCC & Clocks

### Video 18: STM32 Clock and High Speed External Crystal (HSE) | STM32 Bare Metal Tutorial #3
- **Instructor / Channel:** STM32World
- **Direct Link:** [https://www.youtube.com/watch?v=CmWvRszxFFg](https://www.youtube.com/watch?v=CmWvRszxFFg)
- **Platform:** YouTube
- **Duration:** 27m 31s | **Published Date:** 2026-02-16
- **Curriculum Alignment:** Stage - *HSE & PLL Configuration*
- **Prerequisites:** Clock fundamentals
- **Educational Value:** Shows register-level clock switching from internal RC oscillator to external crystal and PLL.
- **Concepts Not Covered (Gap Analysis):** Configured for STM32F4/F1; STM32G4 requires Boost mode voltage scaling above 150MHz.

## Category: Embedded C

### Video 19: STM32 Bare Metal Library | STM32 Bare Metal Tutorial #4
- **Instructor / Channel:** STM32World
- **Direct Link:** [https://www.youtube.com/watch?v=k-Vefsue5pM](https://www.youtube.com/watch?v=k-Vefsue5pM)
- **Platform:** YouTube
- **Duration:** 26m 56s | **Published Date:** 2026-02-22
- **Curriculum Alignment:** Stage - *Bare-Metal Library Architecture*
- **Prerequisites:** C headers, Structs
- **Educational Value:** Guides structuring register definitions into clean, reusable driver headers.
- **Concepts Not Covered (Gap Analysis):** Does not follow full CMSIS-Core coding standard.

## Category: Debugging

### Video 20: Bare metal peripheral from scratch and advanced register debugging - STM32 Bare Metal #6
- **Instructor / Channel:** STM32World
- **Direct Link:** [https://www.youtube.com/watch?v=4mSGkdPqDvY](https://www.youtube.com/watch?v=4mSGkdPqDvY)
- **Platform:** YouTube
- **Duration:** 32m 33s | **Published Date:** 2026-03-16
- **Curriculum Alignment:** Stage - *SVD Register Debugging in VS Code*
- **Prerequisites:** VS Code, GDB
- **Educational Value:** Demonstrates loading System View Description (SVD) XML files into VS Code Cortex-Debug.
- **Concepts Not Covered (Gap Analysis):** Does not cover ITM / SWO tracing.

## Category: Timers & GPIO

### Video 21: Bit-banging PWM - STM32 Bare Metal #9
- **Instructor / Channel:** STM32World
- **Direct Link:** [https://www.youtube.com/watch?v=VN7Ffy6yvZo](https://www.youtube.com/watch?v=VN7Ffy6yvZo)
- **Platform:** YouTube
- **Duration:** 22m 15s | **Published Date:** 2026-04-21
- **Curriculum Alignment:** Stage - *Bit-Banging PWM*
- **Prerequisites:** GPIO, Delay loops
- **Educational Value:** Demonstrates software timing limitations and why hardware timer PWM peripherals are essential.
- **Concepts Not Covered (Gap Analysis):** Does not use hardware timer capture/compare registers.

## Category: Timers

### Video 22: Timer with Interrupt - Bare Metal #8
- **Instructor / Channel:** STM32World
- **Direct Link:** [https://www.youtube.com/watch?v=mGVRcSBTeWI](https://www.youtube.com/watch?v=mGVRcSBTeWI)
- **Platform:** YouTube
- **Duration:** 37m 41s | **Published Date:** 2026-04-15
- **Curriculum Alignment:** Stage - *Timer with Periodic Interrupt*
- **Prerequisites:** Timers, NVIC
- **Educational Value:** Configures prescaler, auto-reload register, and interrupt generation without blocking the CPU.
- **Concepts Not Covered (Gap Analysis):** Does not cover timer synchronization or master/slave trigger modes.

## Category: Interrupts

### Video 23: How to use External Interrupts - STM32 Tutorial #6
- **Instructor / Channel:** STM32World
- **Direct Link:** [https://www.youtube.com/watch?v=eLrsCw1UDJU](https://www.youtube.com/watch?v=eLrsCw1UDJU)
- **Platform:** YouTube
- **Duration:** 15m 34s | **Published Date:** 2024-08-12
- **Curriculum Alignment:** Stage - *External Interrupts (EXTI)*
- **Prerequisites:** GPIO, NVIC
- **Educational Value:** Step-by-step routing of GPIO pin through SYSCFG to EXTI line and NVIC IRQ handling.
- **Concepts Not Covered (Gap Analysis):** Does not cover software-triggered interrupts or event generation mode.

## Category: Communication

### Video 24: STM32 UART Bare Metal | Hello, World! (Getting Started)
- **Instructor / Channel:** Ghost The Engineer
- **Direct Link:** [https://www.youtube.com/watch?v=amdG8GcUXSQ](https://www.youtube.com/watch?v=amdG8GcUXSQ)
- **Platform:** YouTube
- **Duration:** 12m 36s | **Published Date:** 2026-07-09
- **Curriculum Alignment:** Stage - *Bare-Metal UART Transmission*
- **Prerequisites:** Memory-mapped I/O
- **Educational Value:** Calculates baud rate divisor, configures GPIO alternate functions, and transmits bytes over serial.
- **Concepts Not Covered (Gap Analysis):** Does not cover interrupt-driven or DMA UART reception.

## Category: Analog Acquisition

### Video 25: STM32 Bare Metal GPIO Analog Input Tutorial | No HAL, No Libraries | STM32F401
- **Instructor / Channel:** STMetaTronics
- **Direct Link:** [https://www.youtube.com/watch?v=P5g-yXdhdxc](https://www.youtube.com/watch?v=P5g-yXdhdxc)
- **Platform:** YouTube
- **Duration:** 41m 21s | **Published Date:** 2026-04-19
- **Curriculum Alignment:** Stage - *Bare-Metal GPIO Analog Input*
- **Prerequisites:** GPIO modes
- **Educational Value:** Shows how to configure GPIO MODER bits for analog mode to disable digital input buffers.
- **Concepts Not Covered (Gap Analysis):** Does not configure the ADC conversion sequence or sample time.

## Category: RCC & Clocks

### Video 26: STM32 Bare-Metal: Configure Clocks Without HAL
- **Instructor / Channel:** Equip Embedded
- **Direct Link:** [https://www.youtube.com/watch?v=CxS2RdgcMC8](https://www.youtube.com/watch?v=CxS2RdgcMC8)
- **Platform:** YouTube
- **Duration:** 64m 30s | **Published Date:** 2026-07-31
- **Curriculum Alignment:** Stage - *RCC Clocks Without HAL*
- **Prerequisites:** Memory-mapped I/O
- **Educational Value:** Deep walkthrough of RCC registers, flash wait states, and PLL configuration on Cortex-M.
- **Concepts Not Covered (Gap Analysis):** Specific to STM32L4; G474 requires PWR_CR5 boost mode setting.

## Category: Analog Acquisition

### Video 27: STM32 ADC Full Course: From Basics to Advanced (DMA, Timer, Injected, Oversampling)
- **Instructor / Channel:** ControllersTech
- **Direct Link:** [https://www.youtube.com/watch?v=d0aO6HF2Eu0](https://www.youtube.com/watch?v=d0aO6HF2Eu0)
- **Platform:** YouTube
- **Duration:** 153m 1s | **Published Date:** 2026-04-14
- **Curriculum Alignment:** Stage - *STM32 ADC Full Course*
- **Prerequisites:** Analog signals, DMA
- **Educational Value:** Exhaustive treatment of single-shot, scan, continuous, timer-triggered, and DMA ADC modes.
- **Concepts Not Covered (Gap Analysis):** Uses HAL implementation; concepts must be translated to direct registers.

## Category: ARM Architecture

### Video 28: Lecture 15: Booting Process
- **Instructor / Channel:** Embedded Systems and Deep Learning
- **Direct Link:** [https://www.youtube.com/watch?v=3brOzLJmeek](https://www.youtube.com/watch?v=3brOzLJmeek)
- **Platform:** YouTube
- **Duration:** 9m 35s | **Published Date:** 2017-02-04
- **Curriculum Alignment:** Stage - *Cortex-M Booting Process*
- **Prerequisites:** Memory map, Assembly
- **Educational Value:** Authoritative explanation of the power-on reset sequence, SP loading from 0x00000000, and Reset_Handler.
- **Concepts Not Covered (Gap Analysis):** Does not cover dual-bank boot or option byte configuration.

### Video 29: Module 2 _18EC62_ARM Cortex M3 Instruction Sets and Programming
- **Instructor / Channel:** ATMEYA Electrocrats
- **Direct Link:** [https://www.youtube.com/watch?v=WXZ07AgO2OU](https://www.youtube.com/watch?v=WXZ07AgO2OU)
- **Platform:** YouTube
- **Duration:** 13m 46s | **Published Date:** 2023-05-29
- **Curriculum Alignment:** Stage - *Instruction Sets & Assembly*
- **Prerequisites:** CPU architecture
- **Educational Value:** Deep academic lecture on ARMv7-M 16/32-bit Thumb-2 instructions, pipeline, and condition flags.
- **Concepts Not Covered (Gap Analysis):** Does not provide hands-on GCC build workflow.

## Category: STM32 Fundamentals

### Video 30: STM32G4 & Real Time DSP: Part 1 Introduction to the STM32 Family and STM32G4
- **Instructor / Channel:** Easier In Practice
- **Direct Link:** [https://www.youtube.com/watch?v=SVT7n7hSzJU](https://www.youtube.com/watch?v=SVT7n7hSzJU)
- **Platform:** YouTube
- **Duration:** 11m 25s | **Published Date:** 2022-09-10
- **Curriculum Alignment:** Stage - *STM32G4 Family & Architecture*
- **Prerequisites:** Microcontroller basics
- **Educational Value:** Comprehensive introduction to the STM32G4 series: Cordic, FMAC, HRTIM, analog peripherals, and pinouts.
- **Concepts Not Covered (Gap Analysis):** High-level overview without code.

## Category: STM32G4 & GPIO

### Video 31: STM32G4 & Real Time DSP: Part 2 Blinky Tutorial, Digital Functions, and Debugging.
- **Instructor / Channel:** Easier In Practice
- **Direct Link:** [https://www.youtube.com/watch?v=pPnu4VC1dMg](https://www.youtube.com/watch?v=pPnu4VC1dMg)
- **Platform:** YouTube
- **Duration:** 13m 55s | **Published Date:** 2022-09-10
- **Curriculum Alignment:** Stage - *STM32G4 Blinky & Debugging*
- **Prerequisites:** STM32 basics
- **Educational Value:** First hands-on code on STM32G4 Nucleo board, clock initialization, and debugging tools.
- **Concepts Not Covered (Gap Analysis):** Uses IDE auto-generation for initial setup.

## Category: STM32G4 Timers

### Video 32: STM32G4 & Real Time DSP: Part 3 Timers, Execution Time, Non-Blocking Delays, Interrupts, and PWM
- **Instructor / Channel:** Easier In Practice
- **Direct Link:** [https://www.youtube.com/watch?v=jY9akJ5mHC0](https://www.youtube.com/watch?v=jY9akJ5mHC0)
- **Platform:** YouTube
- **Duration:** 23m 53s | **Published Date:** 2022-09-10
- **Curriculum Alignment:** Stage - *STM32G4 Timers & Hardware PWM*
- **Prerequisites:** GPIO, Clock tree
- **Educational Value:** Calculates timer frequencies, implements non-blocking delays, and generates hardware PWM on G4.
- **Concepts Not Covered (Gap Analysis):** Does not cover HRTIM (High Resolution Timer).

## Category: STM32G4 Analog

### Video 33: STM32G4 & Real Time DSP: Part 4 ADC, Polling, Direct Memory Access, Timer Triggered Conversions
- **Instructor / Channel:** Easier In Practice
- **Direct Link:** [https://www.youtube.com/watch?v=5-08Eaa_OlI](https://www.youtube.com/watch?v=5-08Eaa_OlI)
- **Platform:** YouTube
- **Duration:** 23m 38s | **Published Date:** 2022-09-10
- **Curriculum Alignment:** Stage - *STM32G4 ADC & DMA Integration*
- **Prerequisites:** ADC, DMA, Timers
- **Educational Value:** Directly targets STM32G4 fast ADCs, timer triggering, and DMA circular transfer.
- **Concepts Not Covered (Gap Analysis):** Uses HAL wrappers; bare-metal register mapping must be applied.

### Video 34: STM32G4 & Real Time DSP: Part 5 ADC to DAC with DSP, Multiplication, Addition, and Time Delays
- **Instructor / Channel:** Easier In Practice
- **Direct Link:** [https://www.youtube.com/watch?v=zA3PVZzaUiU](https://www.youtube.com/watch?v=zA3PVZzaUiU)
- **Platform:** YouTube
- **Duration:** 25m 43s | **Published Date:** 2022-09-10
- **Curriculum Alignment:** Stage - *STM32G4 ADC to DAC DSP Loopback*
- **Prerequisites:** ADC, DAC, DMA
- **Educational Value:** Builds a complete analog pipeline: ADC sampling -> DSP math -> DAC output in real time.
- **Concepts Not Covered (Gap Analysis):** Does not cover analog comparator integration.

## Category: STM32G4 DSP

### Video 35: STM32G4 & Real Time DSP: Part 6 Using the CMSIS DSP Library
- **Instructor / Channel:** Easier In Practice
- **Direct Link:** [https://www.youtube.com/watch?v=GH50tEgBpX8](https://www.youtube.com/watch?v=GH50tEgBpX8)
- **Platform:** YouTube
- **Duration:** 9m 43s | **Published Date:** 2022-09-29
- **Curriculum Alignment:** Stage - *CMSIS-DSP Math Library Integration*
- **Prerequisites:** C, Fixed-point math
- **Educational Value:** Integrates official ARM CMSIS-DSP library on STM32G4 for real-time digital filtering and convolution.
- **Concepts Not Covered (Gap Analysis):** Does not explore hardware Cordic/FMAC accelerators in G4.

## Category: Communication

### Video 36: STM32F4 Bare-Metal Lab: SPI & I2C Protocol Implementation (BME280)
- **Instructor / Channel:** Olorun_Republia
- **Direct Link:** [https://www.youtube.com/watch?v=rmm2gHUPN_g](https://www.youtube.com/watch?v=rmm2gHUPN_g)
- **Platform:** YouTube
- **Duration:** 10m 14s | **Published Date:** 2026-04-20
- **Curriculum Alignment:** Stage - *SPI & I2C Protocol Implementation*
- **Prerequisites:** Serial communication
- **Educational Value:** Register-level comparison and implementation of SPI and I2C protocols reading a BME280 sensor.
- **Concepts Not Covered (Gap Analysis):** Focuses on STM32F4; G4 I2C has advanced TIMINGR register calculation.

### Video 37: STM32 Bare-Metal Programming: Interfacing the ICM-20948 Over SPI
- **Instructor / Channel:** Equip Embedded
- **Direct Link:** [https://www.youtube.com/watch?v=2A-nG1kWA4s](https://www.youtube.com/watch?v=2A-nG1kWA4s)
- **Platform:** YouTube
- **Duration:** 31m 8s | **Published Date:** 2026-04-22
- **Curriculum Alignment:** Stage - *Bare-Metal SPI Interfacing*
- **Prerequisites:** GPIO, SPI basics
- **Educational Value:** Practical bare-metal SPI driver writing, clock polarity/phase selection, and sensor register reading.
- **Concepts Not Covered (Gap Analysis):** Does not use DMA for SPI data transfer.

### Video 38: STM32 I2C device Driver Part 2 || Bare Metal || SCL ||SDA ||
- **Instructor / Channel:** Gettobyte
- **Direct Link:** [https://www.youtube.com/watch?v=qNLE3yK2k7Q](https://www.youtube.com/watch?v=qNLE3yK2k7Q)
- **Platform:** YouTube
- **Duration:** 23m 17s | **Published Date:** 2021-07-08
- **Curriculum Alignment:** Stage - *Bare-Metal I2C Device Driver*
- **Prerequisites:** I2C protocol
- **Educational Value:** Low-level driver construction covering START, 7-bit address, ACK check, and STOP generation.
- **Concepts Not Covered (Gap Analysis):** Does not cover I2C SMBus or multimaster arbitration.

## Category: RTOS

### Video 39: Introduction to RTOS Part 1 - What is a Real-Time Operating System (RTOS)? | Digi-Key Electronics
- **Instructor / Channel:** DigiKey
- **Direct Link:** [https://www.youtube.com/watch?v=F321087yYy4](https://www.youtube.com/watch?v=F321087yYy4)
- **Platform:** YouTube
- **Duration:** 11m 34s | **Published Date:** 2021-01-04
- **Curriculum Alignment:** Stage - *RTOS Fundamentals & Superloop Comparison*
- **Prerequisites:** Embedded C, Interrupts
- **Educational Value:** Clear conceptual breakdown of why RTOS is needed, preemptive scheduling, and real-time deadlines.
- **Concepts Not Covered (Gap Analysis):** Theory only; coding starts in Part 2.

### Video 40: Introduction to RTOS Part 2 - Getting Started with FreeRTOS | Digi-Key Electronics
- **Instructor / Channel:** DigiKey
- **Direct Link:** [https://www.youtube.com/watch?v=JIr7Xm_riRs](https://www.youtube.com/watch?v=JIr7Xm_riRs)
- **Platform:** YouTube
- **Duration:** 11m 46s | **Published Date:** 2021-01-11
- **Curriculum Alignment:** Stage - *FreeRTOS Getting Started & Task Creation*
- **Prerequisites:** RTOS theory
- **Educational Value:** Teaches `xTaskCreate`, task parameters, priority assignments, and FreeRTOS project setup.
- **Concepts Not Covered (Gap Analysis):** Does not cover static task memory allocation (`xTaskCreateStatic`).

### Video 41: Introduction to RTOS Part 3 - Task Scheduling | Digi-Key Electronics
- **Instructor / Channel:** DigiKey
- **Direct Link:** [https://www.youtube.com/watch?v=95yUbClyf3E](https://www.youtube.com/watch?v=95yUbClyf3E)
- **Platform:** YouTube
- **Duration:** 10m 59s | **Published Date:** 2021-01-18
- **Curriculum Alignment:** Stage - *Preemptive Task Scheduling & Context Switching*
- **Prerequisites:** Task creation
- **Educational Value:** Visualizes tick interrupts, round-robin scheduling, task states (Ready, Running, Blocked, Suspended).
- **Concepts Not Covered (Gap Analysis):** Does not show the low-level PendSV assembly context switch handler.

### Video 42: Introduction to RTOS Part 4 - Memory Management | Digi-Key Electronics
- **Instructor / Channel:** DigiKey
- **Direct Link:** [https://www.youtube.com/watch?v=Qske3yZRW5I](https://www.youtube.com/watch?v=Qske3yZRW5I)
- **Platform:** YouTube
- **Duration:** 10m 40s | **Published Date:** 2021-01-25
- **Curriculum Alignment:** Stage - *RTOS Memory Management & Heap Allocation*
- **Prerequisites:** Pointers, Heap
- **Educational Value:** Examines FreeRTOS heap implementations (heap_1 to heap_5), stack overflow hook functions.
- **Concepts Not Covered (Gap Analysis):** Does not cover MPU-based memory isolation between tasks.

### Video 43: Introduction to RTOS Part 5 - Queue | Digi-Key Electronics
- **Instructor / Channel:** DigiKey
- **Direct Link:** [https://www.youtube.com/watch?v=pHJ3lxOoWeI](https://www.youtube.com/watch?v=pHJ3lxOoWeI)
- **Platform:** YouTube
- **Duration:** 12m 1s | **Published Date:** 2021-02-08
- **Curriculum Alignment:** Stage - *Queues & Inter-Task Communication*
- **Prerequisites:** FreeRTOS tasks
- **Educational Value:** Mastery of `xQueueCreate`, `xQueueSend`, `xQueueReceive` for safe thread communication.
- **Concepts Not Covered (Gap Analysis):** Does not cover zero-copy queue passing using pointers.

### Video 44: Introduction to RTOS Part 6 - Mutex | Digi-Key Electronics
- **Instructor / Channel:** DigiKey
- **Direct Link:** [https://www.youtube.com/watch?v=I55auRpbiTs](https://www.youtube.com/watch?v=I55auRpbiTs)
- **Platform:** YouTube
- **Duration:** 13m 44s | **Published Date:** 2021-02-15
- **Curriculum Alignment:** Stage - *Mutexes & Critical Sections*
- **Prerequisites:** Queues, Concurrency
- **Educational Value:** Explains mutual exclusion, shared resources, race conditions, and `xSemaphoreCreateMutex`.
- **Concepts Not Covered (Gap Analysis):** Does not cover recursive mutexes.

### Video 45: Introduction to RTOS Part 7 - Semaphore | Digi-Key Electronics
- **Instructor / Channel:** DigiKey
- **Direct Link:** [https://www.youtube.com/watch?v=5JcMtbA9QEE](https://www.youtube.com/watch?v=5JcMtbA9QEE)
- **Platform:** YouTube
- **Duration:** 12m 7s | **Published Date:** 2021-02-22
- **Curriculum Alignment:** Stage - *Counting & Binary Semaphores*
- **Prerequisites:** Mutexes
- **Educational Value:** Differentiates mutex ownership from semaphore signaling and task synchronization.
- **Concepts Not Covered (Gap Analysis):** Does not cover event flags / event groups.

### Video 46: Introduction to RTOS Part 8 - Software Timer | Digi-Key Electronics
- **Instructor / Channel:** DigiKey
- **Direct Link:** [https://www.youtube.com/watch?v=b1f1Iex0Tso](https://www.youtube.com/watch?v=b1f1Iex0Tso)
- **Platform:** YouTube
- **Duration:** 12m 59s | **Published Date:** 2021-03-08
- **Curriculum Alignment:** Stage - *Software Timers*
- **Prerequisites:** Semaphores, System tick
- **Educational Value:** Shows how to create one-shot and periodic software timers managed by FreeRTOS daemon task.
- **Concepts Not Covered (Gap Analysis):** Does not compare software timer jitter with hardware timer interrupts.

### Video 47: Introduction to RTOS Part 9 - Hardware Interrupts | Digi-Key Electronics
- **Instructor / Channel:** DigiKey
- **Direct Link:** [https://www.youtube.com/watch?v=qsflCf6ahXU](https://www.youtube.com/watch?v=qsflCf6ahXU)
- **Platform:** YouTube
- **Duration:** 15m 10s | **Published Date:** 2021-03-15
- **Curriculum Alignment:** Stage - *Hardware Interrupts & FromISR APIs*
- **Prerequisites:** NVIC, FreeRTOS queues
- **Educational Value:** Critical lesson on calling RTOS primitives from hardware ISRs using `xQueueSendFromISR` and deferred interrupt processing.
- **Concepts Not Covered (Gap Analysis):** Does not discuss ARM Cortex-M `configMAX_SYSCALL_INTERRUPT_PRIORITY` configuration.

### Video 48: Introduction to RTOS Part 10 - Deadlock and Starvation | Digi-Key Electronics
- **Instructor / Channel:** DigiKey
- **Direct Link:** [https://www.youtube.com/watch?v=hRsWi4HIENc](https://www.youtube.com/watch?v=hRsWi4HIENc)
- **Platform:** YouTube
- **Duration:** 12m 18s | **Published Date:** 2021-03-22
- **Curriculum Alignment:** Stage - *Deadlock & Starvation*
- **Prerequisites:** Mutexes, Priorities
- **Educational Value:** Demonstrates dining philosophers problem, circular wait conditions, and deadlock avoidance.
- **Concepts Not Covered (Gap Analysis):** Does not cover formal verification of concurrent systems.

### Video 49: Introduction to RTOS Part 11 - Priority Inversion | Digi-Key Electronics
- **Instructor / Channel:** DigiKey
- **Direct Link:** [https://www.youtube.com/watch?v=C2xKhxROmhA](https://www.youtube.com/watch?v=C2xKhxROmhA)
- **Platform:** YouTube
- **Duration:** 12m 35s | **Published Date:** 2021-03-29
- **Curriculum Alignment:** Stage - *Priority Inversion & Priority Inheritance*
- **Prerequisites:** Mutexes, Scheduling
- **Educational Value:** Deep explanation of the Mars Pathfinder priority inversion bug and how priority inheritance fixes it.
- **Concepts Not Covered (Gap Analysis):** Does not discuss priority ceiling protocol.

### Video 50: Introduction to RTOS Part 12 - Multicore Systems | Digi-Key Electronics
- **Instructor / Channel:** DigiKey
- **Direct Link:** [https://www.youtube.com/watch?v=LPSHUcH5aQc](https://www.youtube.com/watch?v=LPSHUcH5aQc)
- **Platform:** YouTube
- **Duration:** 16m 27s | **Published Date:** 2021-04-05
- **Curriculum Alignment:** Stage - *Multicore Systems & Symmetric Multiprocessing*
- **Prerequisites:** Advanced RTOS
- **Educational Value:** Explains dual-core scheduling, core affinity, and inter-processor communication.
- **Concepts Not Covered (Gap Analysis):** Cortex-M4 on G474RE is single core; provides general architectural knowledge.

---

<!-- Newly Live-Verified Gap-Remediation Videos (#51-#70) -->

## Category: Toolchain & Bare-Metal

### Video 51: Bare Metal Embedded C Lecture 4: Writing Linker Scripts and Section Placement
- **Instructor / Channel:** Fastbit Embedded Brain Academy
- **Direct Link:** [https://www.youtube.com/watch?v=B7oKdUvRhQQ](https://www.youtube.com/watch?v=B7oKdUvRhQQ)
- **Platform:** YouTube
- **Duration:** 37m 40s | **Published Date:** 2020-04-25
- **Curriculum Alignment:** Stage - *GNU Linker Scripts & Section Placement*
- **Prerequisites:** C compilation, Memory maps
- **Educational Value:** Walks through writing a GNU ld linker script from scratch for ARM Cortex-M4: MEMORY regions, SECTIONS, .text/.data/.bss, VMA vs LMA (AT>FLASH), and boundary symbols.
- **Concepts Not Covered (Gap Analysis):** Targets STM32F407 Flash/SRAM sizes; adapt to STM32G474RE 512 KB Flash, 96 KB SRAM1/2, and 32 KB CCM-SRAM (stm32g474re.ld).

### Video 52: Bare Metal Embedded C Lecture 3: How to Write an MCU Startup File from Scratch
- **Instructor / Channel:** Fastbit Embedded Brain Academy
- **Direct Link:** [https://www.youtube.com/watch?v=2Hm8eEHsgls](https://www.youtube.com/watch?v=2Hm8eEHsgls)
- **Platform:** YouTube
- **Duration:** 25m 23s | **Published Date:** 2020-04-24
- **Curriculum Alignment:** Stage - *Startup Code & Vector Table From Scratch*
- **Prerequisites:** C pointers, Linker scripts
- **Educational Value:** Builds a bare-metal C startup file for ARM Cortex-M4 from scratch: vector table array placement, weak default handler aliases, .data copy from Flash to SRAM, .bss zeroing, and main() invocation.
- **Concepts Not Covered (Gap Analysis):** Targets STM32F407 IRQ count; STM32G474RE has 102 external IRQs (stm32g474xx.h) and requires FPU CPACR enable before hard-float main().


## Category: ARM Architecture

### Video 53: How to Debug HardFaults on ARM Cortex-M MCUs
- **Instructor / Channel:** Memfault
- **Direct Link:** [https://www.youtube.com/watch?v=3_ar-3OjXnA](https://www.youtube.com/watch?v=3_ar-3OjXnA)
- **Platform:** YouTube
- **Duration:** 57m 57s | **Published Date:** 2021-06-16
- **Curriculum Alignment:** Stage - *Cortex-M HardFault & Fault Register Debugging*
- **Prerequisites:** Cortex-M exceptions, Stack frames, GDB
- **Educational Value:** Authoritative Memfault webinar detailing Cortex-M exception frame unwinding (MSP/PSP), decoding CFSR (MMFSR/BFSR/UFSR), HFSR, MMFAR/BFAR, and isolating crashes in GDB.
- **Concepts Not Covered (Gap Analysis):** Covers general ARM Cortex-M fault triage; pair with PM0214 Sec 4.4.9-4.4.14 and nvic-systick-register-map.md for STM32G474RE register offsets.

### Video 54: STM32 MPU tips - 1 MPU usage in STM32 with ARM Cortex M7
- **Instructor / Channel:** STMicroelectronics
- **Direct Link:** [https://www.youtube.com/watch?v=6IUfxSAFhlw](https://www.youtube.com/watch?v=6IUfxSAFhlw)
- **Platform:** YouTube
- **Duration:** 20m 54s | **Published Date:** 2020-05-22
- **Curriculum Alignment:** Stage - *ARMv7-M Memory Protection Unit (MPU)*
- **Prerequisites:** Cortex-M memory map, Exceptions
- **Educational Value:** Official STMicroelectronics training on ARMv7-M PMSAv7 MPU configuration (MPU_CTRL, RBAR, RASR, region size, AP, XN, TEX/C/B/S memory attributes).
- **Concepts Not Covered (Gap Analysis):** Focuses on Cortex-M7 speculative-access rules; STM32G474RE Cortex-M4 shares the 8-region PMSAv7 register model without M7 caches (pair with PM0214 Sec 4.2 and mpu_armv7.h).


## Category: DMA & Peripherals

### Video 55: #9. ADC DMA in STM32 using Registers  || Internal Temp sensor
- **Instructor / Channel:** ControllersTech
- **Direct Link:** [https://www.youtube.com/watch?v=Ysp8hpp8bxI](https://www.youtube.com/watch?v=Ysp8hpp8bxI)
- **Platform:** YouTube
- **Duration:** 29m 36s | **Published Date:** 2021-05-29
- **Curriculum Alignment:** Stage - *Bare-Metal STM32 DMA Using Registers*
- **Prerequisites:** Pointers, Peripheral registers, ADC
- **Educational Value:** Direct register-level STM32 DMA setup without HAL: configuring peripheral/memory addresses, transfer count, circular mode, and peripheral-to-memory data flow.
- **Concepts Not Covered (Gap Analysis):** Targets STM32F4 stream-based DMA; STM32G474RE uses channel-based DMA1/DMA2 routed via DMAMUX1_Channelx->CCR (pair with dma-dmamux-register-map.md).


## Category: Communication

### Video 56: FDCAN External Loop-back Mode
- **Instructor / Channel:** STMicroelectronics
- **Direct Link:** [https://www.youtube.com/watch?v=4xVnlDdouBg](https://www.youtube.com/watch?v=4xVnlDdouBg)
- **Platform:** YouTube
- **Duration:** 20m 20s | **Published Date:** 2022-04-25
- **Curriculum Alignment:** Stage - *STM32 FDCAN Protocol & Loop-Back Mode*
- **Prerequisites:** Serial protocols, Clock trees
- **Educational Value:** Official STMicroelectronics walkthrough of CAN-FD framing, bit-rate switching (BRS), FDCAN architecture, AN5348 guidance, and external loopback verification without a full bus.
- **Concepts Not Covered (Gap Analysis):** Demonstrated via STM32CubeIDE/HAL on STM32H7; STM32G474RE integrates 3 FDCAN instances with fixed 212-word Message RAM per instance (pair with fdcan-register-map.md).


## Category: STM32G4 Timers

### Video 57: STM32G4 OLT - 44 . WDG TIMERS High Resolution Timer HRTIM
- **Instructor / Channel:** STMicroelectronics
- **Direct Link:** [https://www.youtube.com/watch?v=JwJG251RscQ](https://www.youtube.com/watch?v=JwJG251RscQ)
- **Platform:** YouTube
- **Duration:** 38m 12s | **Published Date:** 2021-09-20
- **Curriculum Alignment:** Stage - *STM32G4 High-Resolution Timer (HRTIM)*
- **Prerequisites:** Timers, PWM, Dead-time
- **Educational Value:** Official STMicroelectronics STM32G474 Online Training detailing HRTIM1 184 ps DLL resolution, Master Timer + 6 Sub-Timers (TIMA-TIMF), crossbar set/reset events, dead-time, burst mode, and fault protection.
- **Concepts Not Covered (Gap Analysis):** Architecture and feature lecture without bare-metal C code; pair with hrtim-register-map.md, RM0440 Ch 27, and AN4539 for register-level programming.


## Category: STM32G4 DSP

### Video 58: CORDIC in Practice, Part 2: Cosine and Sine
- **Instructor / Channel:** STMicroelectronics
- **Direct Link:** [https://www.youtube.com/watch?v=SeDyNtKRDMU](https://www.youtube.com/watch?v=SeDyNtKRDMU)
- **Platform:** YouTube
- **Duration:** 30m 52s | **Published Date:** 2020-11-10
- **Curriculum Alignment:** Stage - *STM32G4 CORDIC Coprocessor (Cos/Sin)*
- **Prerequisites:** Fixed-point math, STM32G4 architecture
- **Educational Value:** Official STMicroelectronics hands-on STM32G4 CORDIC training explaining Q1.31/Q1.15 angle scaling, CSR configuration, WDATA/RDATA pipeline, precision cycles, and AN5325 benchmarks.
- **Concepts Not Covered (Gap Analysis):** Uses STM32CubeIDE/LL-HAL scaffolding; translate directly to bare-metal CORDIC->CSR/WDATA/RDATA registers via cordic-fmac-crc-rng-register-map.md.

### Video 59: STM32H7 OLT - 62. Peripheral Filter Math Accelerator FMAC
- **Instructor / Channel:** STMicroelectronics
- **Direct Link:** [https://www.youtube.com/watch?v=AQCTcl0eFRk](https://www.youtube.com/watch?v=AQCTcl0eFRk)
- **Platform:** YouTube
- **Duration:** 20m 42s | **Published Date:** 2021-03-16
- **Curriculum Alignment:** Stage - *STM32 Filter Math Accelerator (FMAC)*
- **Prerequisites:** Digital filters (FIR/IIR), Fixed-point math
- **Educational Value:** Official STMicroelectronics training on the FMAC hardware accelerator: 256x16-bit internal SRAM, X1/X2/Y circular buffers, preload, and autonomous FIR/IIR filter execution.
- **Concepts Not Covered (Gap Analysis):** Labeled STM32H7 OLT but covers the identical ST FMAC peripheral IP present in STM32G474RE; pair with cordic-fmac-crc-rng-register-map.md and AN5305.


## Category: Bootloaders & Flash

### Video 60: How to Create a Super Simple Bootloader, Part 4: Developing the App Code
- **Instructor / Channel:** STMicroelectronics
- **Direct Link:** [https://www.youtube.com/watch?v=MFIzI-UNaEs](https://www.youtube.com/watch?v=MFIzI-UNaEs)
- **Platform:** YouTube
- **Duration:** 6m 38s | **Published Date:** 2021-07-08
- **Curriculum Alignment:** Stage - *Custom Bootloader App & VTOR Relocation*
- **Prerequisites:** Linker scripts, Vector tables, Flash memory
- **Educational Value:** Official STMicroelectronics guide showing how to partition internal Flash, shift an application linker script origin, relocate SCB->VTOR, and jump cleanly from a custom bootloader.
- **Concepts Not Covered (Gap Analysis):** Targets NUCLEO-G071RB with STM32CubeIDE; STM32G474RE requires 64-bit double-word Flash programming and supports Dual-Bank RWW (pair with pwr-flash-register-map.md and RM0440 Ch 3).

### Video 61: STM32 Secure Bootloader: The Bootloader | Bootloader #2
- **Instructor / Channel:** STM32World
- **Direct Link:** [https://www.youtube.com/watch?v=nagbMzDy2FA](https://www.youtube.com/watch?v=nagbMzDy2FA)
- **Platform:** YouTube
- **Duration:** 31m 55s | **Published Date:** 2026-11-16
- **Curriculum Alignment:** Stage - *STM32 Secure Bootloader & Signature Verification*
- **Prerequisites:** Bootloaders, Linker scripts, Cryptography basics
- **Educational Value:** Practical STM32 bootloader implementation covering firmware image verification, digital signature checking prior to flashing, and safe application handoff.
- **Concepts Not Covered (Gap Analysis):** Targets STM32 USB DFU workflow; adapt to STM32G474RE Dual-Bank A/B rollback, hardware CRC-32, and Option Byte RDP/WRP via RM0440 Ch 3 and APP-OTA-001.


## Category: Power & Reliability

### Video 62: STM32 Stop Mode Tutorial | Save Power Efficiently
- **Instructor / Channel:** ControllersTech
- **Direct Link:** [https://www.youtube.com/watch?v=UtQhc4XV8k4](https://www.youtube.com/watch?v=UtQhc4XV8k4)
- **Platform:** YouTube
- **Duration:** 19m 24s | **Published Date:** 2020-03-25
- **Curriculum Alignment:** Stage - *STM32 Stop Mode & Wakeup Clock Recovery*
- **Prerequisites:** RCC clocks, EXTI interrupts, PWR basics
- **Educational Value:** Demonstrates entering STM32 Stop mode via WFI, waking from EXTI/RTC interrupts, and performing mandatory post-Stop PLL/SYSCLK re-initialization.
- **Concepts Not Covered (Gap Analysis):** Uses STM32CubeIDE/HAL on legacy STM32; STM32G474RE implements Stop 0 and Stop 1 (no Stop 2) plus Standby/Shutdown (pair with pwr-flash-register-map.md and lab-14).

### Video 63: STM32 Watchdog Tutorial – IWDG vs WWDG with Examples
- **Instructor / Channel:** ControllersTech
- **Direct Link:** [https://www.youtube.com/watch?v=AelNsnpfbcM](https://www.youtube.com/watch?v=AelNsnpfbcM)
- **Platform:** YouTube
- **Duration:** 16m 55s | **Published Date:** 2020-07-19
- **Curriculum Alignment:** Stage - *Hardware Watchdogs (IWDG vs WWDG)*
- **Prerequisites:** Timers, Clock sources (LSI/PCLK1)
- **Educational Value:** Contrasts Independent Watchdog (IWDG clocked by LSI) against Window Watchdog (WWDG clocked by PCLK1) with timeout calculations and refresh-window failure modes.
- **Concepts Not Covered (Gap Analysis):** Demonstrated via STM32CubeIDE/HAL; translate to direct IWDG->KR/PR/RLR and WWDG->CR/CFR register writes using watchdog-register-map.md and lab-13.


## Category: Firmware Engineering

### Video 64: Unit Testing With Ceedling
- **Instructor / Channel:** ORCASat Command and Data Handling
- **Direct Link:** [https://www.youtube.com/watch?v=QnkO4q95o6s](https://www.youtube.com/watch?v=QnkO4q95o6s)
- **Platform:** YouTube
- **Duration:** 39m 15s | **Published Date:** 2020-05-10
- **Curriculum Alignment:** Stage - *Embedded C Unit Testing with Unity & Ceedling*
- **Prerequisites:** C functions, Headers, Host GCC
- **Educational Value:** Flight-software (CubeSat C&DH) walkthrough demonstrating host-based Embedded C unit testing, test-driven workflow, and mocking hardware dependencies with Unity and Ceedling.
- **Concepts Not Covered (Gap Analysis):** Does not cover on-target hardware-in-the-loop (HIL) testing or MISRA C static analysis; pair with 17-advanced-firmware-engineering/README.md.

### Video 65: MISRA C 2023 What You Need to Know | Embedded Computing Design
- **Instructor / Channel:** Parasoft
- **Direct Link:** [https://www.youtube.com/watch?v=R5gUzPYYs1s](https://www.youtube.com/watch?v=R5gUzPYYs1s)
- **Platform:** YouTube
- **Duration:** 51m 11s | **Published Date:** 2023-10-26
- **Curriculum Alignment:** Stage - *MISRA C:2023 Coding Standard & Static Analysis*
- **Prerequisites:** C11/C99 language rules, Compiler warnings
- **Educational Value:** In-depth overview of MISRA C:2023 structure, Mandatory/Required/Advisory rules, Directives, essential type model, deviation workflows, and CI static analysis integration.
- **Concepts Not Covered (Gap Analysis):** Vendor-hosted technical webinar; pair with open-source cppcheck/clang-tidy workflows and 21-reference-library/industry-standards-misra-barr.md.


## Category: RTOS

### Video 66: #23 RTOS Part-2: Automating the context switch
- **Instructor / Channel:** Quantum Leaps, LLC
- **Direct Link:** [https://www.youtube.com/watch?v=PKml9ki3178](https://www.youtube.com/watch?v=PKml9ki3178)
- **Platform:** YouTube
- **Duration:** 37m 36s | **Published Date:** 2018-04-15
- **Curriculum Alignment:** Stage - *ARM Cortex-M PendSV Context Switch Internals*
- **Prerequisites:** ARM Cortex-M exceptions, Stack frames, Assembly
- **Educational Value:** Bare-metal ARM Cortex-M walkthrough writing a real PendSV_Handler context switcher in assembly: saving/restoring R4-R11, swapping thread stack pointers, and triggering ICSR.PENDSVSET.
- **Concepts Not Covered (Gap Analysis):** Implements a minimal teaching kernel (MiROS) rather than FreeRTOS; supplement FreeRTOS BASEPRI and configMAX_SYSCALL_INTERRUPT_PRIORITY via 16-rtos/README.md and freertos-context-switch.mmd.


## Category: Firmware Engineering

### Video 67: #35 State Machines Part-1: What is a state machine?
- **Instructor / Channel:** Quantum Leaps, LLC
- **Direct Link:** [https://www.youtube.com/watch?v=EBSxZKjgBXI](https://www.youtube.com/watch?v=EBSxZKjgBXI)
- **Platform:** YouTube
- **Duration:** 24m 15s | **Published Date:** 2020-11-22
- **Curriculum Alignment:** Stage - *Event-Driven Finite State Machines (FSM)*
- **Prerequisites:** Embedded C, Function pointers, Interrupts
- **Educational Value:** Teaches event-driven embedded firmware design and clean C Finite State Machines, showing how explicit state variables and events eliminate brittle nested-flag spaghetti code.
- **Concepts Not Covered (Gap Analysis):** Introduces flat FSMs before nesting; follow immediately with Video #68 for Hierarchical State Machines (HSM) and Active Objects.

### Video 68: Active Objects, Hierarchical State Machines and Modeling in Practice
- **Instructor / Channel:** Quantum Leaps, LLC
- **Direct Link:** [https://www.youtube.com/watch?v=h_u92uLssDo](https://www.youtube.com/watch?v=h_u92uLssDo)
- **Platform:** YouTube
- **Duration:** 40m 53s | **Published Date:** 2021-10-22
- **Curriculum Alignment:** Stage - *Hierarchical State Machines (HSM) & Active Objects*
- **Prerequisites:** FSMs, RTOS concurrency, Embedded C
- **Educational Value:** Conference keynote by Miro Samek demonstrating Hierarchical State Machines (HSM), Run-to-Completion (RTC) semantics, and the Active Object (Actor) concurrency architecture for deterministic embedded systems.
- **Concepts Not Covered (Gap Analysis):** Demonstrates QP modeling concepts alongside C architecture; implement bare-metal C state tables and ring-buffer event queues via 17-advanced-firmware-engineering/README.md.


## Category: Debugging

### Video 69: Debugging, Logging, and Monitoring via SWO Trace (STM32 ITM & DWT) - Phil's Lab #173
- **Instructor / Channel:** Phil’s Lab
- **Direct Link:** [https://www.youtube.com/watch?v=rdmJgQW5JrQ](https://www.youtube.com/watch?v=rdmJgQW5JrQ)
- **Platform:** YouTube
- **Duration:** 23m 9s | **Published Date:** 2026-05-04
- **Curriculum Alignment:** Stage - *CoreSight SWO, ITM Printf & DWT Live Trace*
- **Prerequisites:** SWD debugging, Cortex-M peripherals
- **Educational Value:** Practical demonstration of CoreSight Serial Wire Output (SWO), ITM stimulus port 0 printf logging without UART overhead, and DWT hardware watchpoint/live variable tracing on STM32.
- **Concepts Not Covered (Gap Analysis):** Demonstrated in STM32CubeIDE on STM32H7; NUCLEO-G474RE routes SWO on PB3 via ST-LINK/V3E (pair with nvic-systick-register-map.md for bare-metal DWT->CYCCNT and ITM_SendChar).

### Video 70: STM32G4 OLT - 9 . System Debug
- **Instructor / Channel:** STMicroelectronics
- **Direct Link:** [https://www.youtube.com/watch?v=t5EH2DNKbAU](https://www.youtube.com/watch?v=t5EH2DNKbAU)
- **Platform:** YouTube
- **Duration:** 15m 8s | **Published Date:** 2019-10-08
- **Curriculum Alignment:** Stage - *STM32G4 CoreSight System Debug Architecture*
- **Prerequisites:** ARM Cortex-M4 architecture, SWD
- **Educational Value:** Official STMicroelectronics STM32G4 Online Training detailing STM32G4 CoreSight debug blocks: SWJ-DP, FPB breakpoints, DWT watchpoints/cycle counter, ITM/TPIU/SWO, ETM, and DBGMCU peripheral freeze.
- **Concepts Not Covered (Gap Analysis):** Architectural hardware reference lecture without live GDB commands; pair with Video #69, ST AN4989, and 05-toolchain-and-debugging/README.md.

