# Requirements Traceability Matrix

This matrix maps every mandatory requirement from the curriculum specification to its corresponding module within the repository.

| Requirement ID | Requirement Category | Specific Concept Scope | Repository Module Location | Status |
|---|---|---|---|---|
| **REQ-C-01** | C Foundations | Compilation phases, Preprocessor, Header guards, Include mechanics | `02-c-fundamentals/` | Verified & Complete |
| **REQ-C-02** | C Foundations | Data types, Fixed-width integers (`stdint.h`), 2's complement | `02-c-fundamentals/` | Verified & Complete |
| **REQ-C-03** | C Foundations | Bitwise operators, Bit masking, Packing, Shifts, Bitfields | `02-c-fundamentals/` | Verified & Complete |
| **REQ-C-04** | C Foundations | Pointers, Pointer arithmetic, Structs, Alignment, Padding, Unions | `02-c-fundamentals/` | Verified & Complete |
| **REQ-C-05** | C Foundations | Storage classes (`static`, `extern`), Qualifiers (`const`, `volatile`) | `02-c-fundamentals/` | Verified & Complete |
| **REQ-EMB-01** | Embedded C | Memory-Mapped I/O, Peripheral register addressing, Base offsets | `03-embedded-c/` | Verified & Complete |
| **REQ-EMB-02** | Embedded C | Volatile memory semantics, Atomicity, Race conditions, Critical sections | `03-embedded-c/` | Verified & Complete |
| **REQ-EMB-03** | Embedded C | Hardware Abstraction Layers, CMSIS Core, HAL vs LL vs Direct Register | `03-embedded-c/` | Verified & Complete |
| **REQ-EMB-04** | Embedded C | Defensive programming, MISRA C:2012 principles, Compile-time assertions | `03-embedded-c/` | Verified & Complete |
| **REQ-ARM-01** | ARM Cortex-M4 | ARMv7-M Architecture, Core registers (R0-R15), PSR, Dual Stack Pointers | `04-arm-cortex-m/` | Verified & Complete |
| **REQ-ARM-02** | ARM Cortex-M4 | Operating modes (Thread/Handler), Privileged/Unprivileged execution | `04-arm-cortex-m/` | Verified & Complete |
| **REQ-ARM-03** | ARM Cortex-M4 | Bus matrix, AHB/APB buses, Memory map, System Control Space (SCS) | `04-arm-cortex-m/` | Verified & Complete |
| **REQ-ARM-04** | ARM Cortex-M4 | Exceptions, NVIC vectoring, Priority grouping, Subpriorities, SysTick | `04-arm-cortex-m/` | Verified & Complete |
| **REQ-ARM-05** | ARM Cortex-M4 | Fault handling, HardFault analysis, BusFault, UsageFault, Stack unwinding | `04-arm-cortex-m/` | Verified & Complete |
| **REQ-TOOL-01** | Toolchain & Debug | ARM GNU Toolchain, Preprocessor, Compiler, Assembler, Linker (`ld`) | `05-toolchain-and-debugging/` | Verified & Complete |
| **REQ-TOOL-02** | Toolchain & Debug | Linker scripts (`.ld`), Memory regions, Sections (`.text`, `.data`, `.bss`) | `05-toolchain-and-debugging/` | Verified & Complete |
| **REQ-TOOL-03** | Toolchain & Debug | Startup code (`startup.c`), Vector table layout, `Reset_Handler` sequence | `05-toolchain-and-debugging/` | Verified & Complete |
| **REQ-TOOL-04** | Toolchain & Debug | OpenOCD, ST-LINK SWD interface, GDB debugging workflow, SVD registers | `05-toolchain-and-debugging/` | Verified & Complete |
| **REQ-STM-01** | STM32 Architecture | STM32 family comparative analysis (F1, F4, G0, G4, H7) | `06-stm32-fundamentals/` | Verified & Complete |
| **REQ-STM-02** | STM32 Architecture | STM32G474RE microarchitecture, Datasheet and Reference Manual navigation | `06-stm32-fundamentals/` | Verified & Complete |
| **REQ-RCC-01** | Clock Engine | Clock sources (HSI16, HSE, LSI, LSE, HSI48), PLL multipliers and dividers | `07-rcc-and-clocks/` | Verified & Complete |
| **REQ-RCC-02** | Clock Engine | Voltage scaling, Range 1 Boost Mode, Flash wait states at 170 MHz | `07-rcc-and-clocks/` | Verified & Complete |
| **REQ-GPIO-01** | GPIO Peripherals | Push-pull, Open-drain, Pull-up, Pull-down, High-speed output dynamics | `08-gpio/` | Verified & Complete |
| **REQ-GPIO-02** | GPIO Peripherals | Register mapping (MODER, OTYPER, OSPEEDR, PUPDR, IDR, ODR, BSRR, AFR) | `08-gpio/` | Verified & Complete |
| **REQ-TIM-01** | Timers & Timing | Counter architecture, Prescaler, Auto-reload, Timebase calculation | `09-timers/` | Verified & Complete |
| **REQ-TIM-02** | Timers & Timing | Output compare, PWM generation, Dead-time insertion, Break inputs | `09-timers/` | Verified & Complete |
| **REQ-INT-01** | Interrupt Architecture| EXTI routing via SYSCFG, Rising/falling triggers, Pending flag clearance | `10-interrupts/` | Verified & Complete |
| **REQ-INT-02** | Interrupt Architecture| NVIC prioritization, Preemption vs Subpriority, Atomic critical sections | `10-interrupts/` | Verified & Complete |
| **REQ-COMM-01**| Serial Protocols | USART/UART asynchronous communication, Baud rate calculations, Retargeting | `11-communication/` | Verified & Complete |
| **REQ-COMM-02**| Serial Protocols | SPI protocol, Master/Slave modes, Clock polarity (CPOL) and phase (CPHA) | `11-communication/` | Verified & Complete |
| **REQ-COMM-03**| Serial Protocols | I2C bus timing, Start/Stop generation, 7-bit addressing, TIMINGR setup | `11-communication/` | Verified & Complete |
| **REQ-ANA-01** | Analog Subsystems | 12-bit SAR ADC, Sampling time, Calibration, Scan mode, Regular/Injected | `12-adc-dac-analog/` | Verified & Complete |
| **REQ-ANA-02** | Analog Subsystems | DAC generation, Analog comparators (COMP), Integrated OPAMPs | `12-adc-dac-analog/` | Verified & Complete |
| **REQ-DMA-01** | DMA Controller | DMA architecture, Channel priority, Circular mode, Double buffering | `13-dma/` | Verified & Complete |
| **REQ-DMA-02** | DMA Controller | DMAMUX request routing on STM32G4, Peripheral-to-memory data pipelines | `13-dma/` | Verified & Complete |
| **REQ-MEM-01** | Memory & Storage | Embedded Flash dual-bank memory, In-Application Programming, EEPROM emulation| `14-memory-and-storage/` | Verified & Complete |
| **REQ-PWR-01** | Power & Reliability | Low-power states (Sleep, Stop, Standby), Watchdogs (IWDG, WWDG), BOR/PVD | `15-power-management/` | Verified & Complete |
| **REQ-RTOS-01**| RTOS & Concurrency | Superloop vs Preemptive RTOS, Task creation, Scheduling, FreeRTOS kernel | `16-rtos/` | Verified & Complete |
| **REQ-RTOS-02**| RTOS & Concurrency | Queues, Semaphores, Mutexes, Priority Inversion, Priority Inheritance | `16-rtos/` | Verified & Complete |
| **REQ-ADV-01** | Firmware Engineering| Layered architectures, Finite State Machines (FSM), Ring buffers | `17-advanced-firmware-engineering/` | Verified & Complete |
| **REQ-VID-01** | Video Verification | At least 30 genuine educational video lectures verified and mapped | `22-video-library/` (50 videos) | Verified & Complete |
| **REQ-LAB-01** | Practical Roadmap | Sequential hands-on laboratory experiments for NUCLEO-G474RE | `18-practical-laboratory/` | Verified & Complete |
