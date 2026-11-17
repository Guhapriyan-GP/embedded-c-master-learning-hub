# Stage 04: GNU Toolchain, Linker Scripts & Startup Code

## 1. Prerequisites
Stage 03 (ARM Cortex-M4 microarchitecture).

## 2. Core Technical Theory & Engineering Principles
A bare-metal C program requires an explicit linker script and startup file to establish memory layout and boot the processor into `main()`.

### 1. Linker Script Architecture (`stm32g474re.ld`)
The linker script directs the GNU Linker (`arm-none-eabi-ld`) on how to combine object files and allocate memory:
- **`MEMORY` Command:** Defines physical memory boundaries:
  - `FLASH (rx)  : ORIGIN = 0x08000000, LENGTH = 512K`
  - `RAM (xrw)   : ORIGIN = 0x20000000, LENGTH = 96K`
  - `CCMSRAM (rw): ORIGIN = 0x10000000, LENGTH = 32K`
- **`SECTIONS` Command:** Maps code and data sections:
  - **`.text`:** Executable machine code, vector table, and `const` literals (placed in Flash).
  - **`.data`:** Initialized global and static variables. Loaded into Flash (LMA), but allocated execution space in SRAM (VMA).
  - **`.bss`:** Uninitialized global and static variables (allocated in SRAM, zeroed at startup).
  - **Stack & Heap:** Stack grows downwards from top of SRAM; heap grows upwards.

### 2. Startup Code Execution Flow (`startup.c`)
1. On power-on, the CPU loads the initial Stack Pointer (MSP) from address `0x00000000` (Flash origin `0x08000000`).
2. CPU loads the address of `Reset_Handler` from address `0x00000004` and begins execution.
3. `Reset_Handler` copies initialized variables from Flash LMA to SRAM VMA (`_sidata` -> `_sdata` to `_edata`).
4. `Reset_Handler` zeroes out the `.bss` section in SRAM (`_sbss` to `_ebss`).
5. Calls `SystemInit()` to initialize the FPU coprocessor.
6. Calls `main()`.

## 3. Practical Coding Exercises & Challenges
1. Write a custom linker script placing an array into CCM-SRAM.
2. Disassemble a compiled `.elf` file using `arm-none-eabi-objdump -d`.
3. Inspect symbol locations using `arm-none-eabi-nm`.

## 4. Expected Learning Outcomes & Mastery Criteria
Ability to write bare-metal linker scripts and startup code from scratch without IDE wizards.
