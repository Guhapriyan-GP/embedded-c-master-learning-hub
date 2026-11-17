# Concept-by-Concept Prerequisite Matrix

This matrix maps every core embedded concept to its absolute mandatory prerequisites. Attempting to implement or study a target concept without fulfilling all listed prerequisites will result in conceptual gaps and difficult debugging scenarios.

| Target Concept | Mandatory Prerequisites | Why the Prerequisite is Essential |
|---|---|---|
| **Memory-Mapped Registers** | Pointers, Typecasting, Hexadecimal notation | A register is accessed by dereferencing a volatile pointer to an absolute 32-bit physical address. |
| **Atomic Bit Manipulation** | Bitwise operators (`&`, `|`, `~`, `^`), RMW hazards | Modifying individual register bits requires masking; multi-cycle Read-Modify-Write instructions can be corrupted by interrupts. |
| **Volatile Qualifier** | C compiler optimization, Caches, Memory access | The compiler will eliminate reads or writes to memory locations that it assumes cannot change from outside the C scope. |
| **NVIC & Interrupts** | Vector tables, Operating modes, Exception stack frame | Interrupts run asynchronously in Handler mode and save the caller context automatically onto the stack. |
| **Linker Scripts** | Memory map, Object sections (`.text`, `.data`, `.bss`) | The linker maps logical code and data sections to physical Flash and SRAM memory boundaries. |
| **Startup Code** | Reset sequence, C runtime requirements, Vector table | C requires that `.data` is initialized from Flash and `.bss` is zeroed in SRAM before `main()` is executed. |
| **Clock Tree (RCC)** | Oscillators, PLL mathematics, Flash latency | The CPU and peripherals cannot operate above default frequencies without configuring PLL multipliers and Flash wait states. |
| **GPIO Configuration** | Peripheral bus clock, Push-pull vs Open-drain | Setting GPIO registers has no effect if the GPIO peripheral clock is disabled in RCC. |
| **Hardware Timers** | Bus clock frequency, Prescaler division, Overflow | Setting timer periods requires calculating clock divisions based on the peripheral bus clock frequency. |
| **UART Baud Rates** | Clock frequency, Oversampling, Fractional division | The UART baud rate register (`BRR`) divides the peripheral clock to match the serial transmission baud rate. |
| **SPI / I2C Buses** | Shift registers, Clock polarity/phase, Open-drain | Synchronous and open-drain multi-drop buses require strict adherence to bus timing and pull-up resistor specifications. |
| **ADC Signal Acquisition**| Analog input mode, SAR timing, Sampling impedance | The ADC input pin must have digital Schmitt triggers disabled, and input capacitance requires adequate sampling time. |
| **DMA Data Transport** | Bus masters, Memory alignments, Buffer wrapping | DMA directly transfers data across the bus matrix without CPU intervention; incorrect pointers cause memory corruption. |
| **RTOS Multitasking** | Context switching, PendSV exception, Stack allocation | An RTOS saves and restores CPU core registers on separate task stacks during timer-driven preemptive switches. |
| **Mutexes & Semaphores** | Critical sections, Race conditions, Scheduling | Mutexes prevent simultaneous access to shared resources and use priority inheritance to prevent priority inversion. |
