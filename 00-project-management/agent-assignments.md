# Multi-Agent Research Structure & Workstream Allocation

To ensure technical depth, accuracy, and thoroughness, the research and architecture of this curriculum were partitioned into eleven specialized research workstreams, each overseen by a dedicated agent profile:

| Agent Identifier | Specialization Stream | Primary Responsibilities |
|---|---|---|
| **Agent 0** | **Master Research Coordinator** | Overall curriculum architecture, dependency synthesis, requirements traceability, and cross-stream conflict resolution. |
| **Agent 1** | **C Programming Foundations** | Memory representation, data types, bitwise operators, pointers, memory models, alignment, storage classes, and standard undefined behaviors. |
| **Agent 2** | **Embedded C Systems** | Memory-mapped I/O, volatile access semantics, atomic bit manipulation, read-modify-write safety, critical sections, CMSIS vs HAL vs LL comparison, and MISRA C guidelines. |
| **Agent 3** | **ARM Cortex-M Architecture** | ARMv7-M microarchitecture, processor registers, Thumb-2 ISA, bus matrix, operating modes, exception stacking, NVIC architecture, FPU, and DSP extensions. |
| **Agent 4** | **STM32 Hardware & RCC Engine** | System clock architectures, HSI/HSE/PLL configuration, flash wait states, voltage scaling, Core Boost Mode (170 MHz), bus prescalers, and power domains. |
| **Agent 5** | **Timers & Real-Time Events** | Counter architectures, prescaler and auto-reload calculations, input capture, output compare, PWM generation, dead-time insertion, and hardware interrupt latency. |
| **Agent 6** | **Serial Protocols & Acquisition** | UART/USART baud rate mathematics, SPI timing modes (CPOL/CPHA), I2C timing configurations, CAN/CAN FD protocols, SAR ADC sampling engines, and DAC interfacing. |
| **Agent 7** | **Toolchain & Debug Infrastructure** | GNU Toolchain (`arm-none-eabi-gcc`), linker script anatomy, memory section mapping (`.text`, `.data`, `.bss`), startup assembly, vector tables, OpenOCD, GDB, and SVD debugging. |
| **Agent 8** | **RTOS & Concurrency Architecture** | Preemptive multitasking, FreeRTOS kernel architecture, context switching, queues, semaphores, mutexes, priority inversion, and low-power idle states. |
| **Agent 9** | **Educational Video Research** | Discovery, canonical URL verification, timestamping, prerequisite mapping, and educational evaluation of at least 30 genuine video lectures. |
| **Agent 10** | **Independent Technical Auditor** | Verification of register offsets against official ST reference manuals (RM0440), mathematical validation of timing formulas, and link integrity verification. |

### Technical Conflict Resolution Record
1. **Clock Distribution to Timers:** Verified that on STM32G4, when the APB prescaler is 1, the timer clock equals the APB clock; if the APB prescaler is greater than 1, the timer clock is multiplied by 2.
2. **Flash Wait States at 170 MHz:** Verified against RM0440 Section 3.3.3 that operating the STM32G4 at 170 MHz in Range 1 Boost mode requires 4 Flash wait states (5 CPU cycles access time).
3. **GPIO Bit Manipulation:** Explicitly distinguished between non-atomic Read-Modify-Write operations on the Output Data Register (`ODR`) and atomic single-cycle writes to the Bit Set/Reset Register (`BSRR`).
