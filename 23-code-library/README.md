# Production-Grade Bare-Metal Code Library & Starter Template (STM32G474RE)

This directory contains the standalone bare-metal starter template, linker script, startup code, CMSIS device headers, and reusable bare-metal drivers for the **STM32G474RET6** on the **NUCLEO-G474RE (MB1367 Rev C)** development board.

> **Verification Status:**
> - **Compile-Verified (`arm-none-eabi-gcc 14.3.1`):** All template and driver sources compile cleanly with zero warnings in both `BUILD_TYPE=debug` (`-Og -g3`) and `BUILD_TYPE=release` (`-O2 -g`) under `-std=c11 -Wall -Wextra -Werror -Wpedantic -Wshadow -Wconversion -Wdouble-promotion -Wundef`.
> - **ELF Structure Verified (`arm-none-eabi-readelf -S`, `arm-none-eabi-size -A`):** `.isr_vector` verified at exactly 472 bytes (118 entries = 16 core + 102 STM32G474RE IRQs), 512-byte aligned; `.ccmram` (executable `xrw`) and `.noinit` sections verified.
> - **Hardware Status:** Statically reviewed against CMSIS `stm32g474xx.h`, RM0440, DS12288, PM0214, and UM2505 — **Not Tested on Physical NUCLEO-G474RE Hardware**.

---

## Directory Structure

```text
23-code-library/
├── Drivers/
│   ├── CMSIS/                         # Single source of truth for ARM CMSIS-Core & ST STM32G4xx headers
│   ├── clock_g4.h / clock_g4.c        # 170 MHz Range 1 Boost Mode (RM0440 §6.1.5) + DWT->CYCCNT delays
│   ├── gpio_g4.h  / gpio_g4.c         # Atomic BSRR & glitch-free STM32G4 GPIO configuration driver
│   ├── ring_buffer.h / ring_buffer.c  # Lock-free SPSC power-of-2 ring buffer (host unit-testable)
│   └── fault_handler.h / fault_handler.c  # Cortex-M4 HardFault CFSR/HFSR/MMFAR/BFAR .noinit crash logger
└── templates/                         # Standalone buildable starter project
    ├── Makefile                       # Portable GNU Make (debug/release, -MMD -MP, -fstack-usage)
    ├── CMakeLists.txt                 # Modern CMake build exporting compile_commands.json
    ├── cmake/arm-none-eabi-gcc.cmake  # ARM Cortex-M4F CMake cross-compilation toolchain file
    ├── stm32g474re.ld                 # Linker script (FLASH 512K, RAM 96K, CCMRAM 32K xrw, .noinit, ASSERTs)
    ├── Inc/                           # Application headers
    └── Src/
        ├── startup_stm32g474re.c      # Complete 118-entry vector table, .ccmram/.data/.bss init, HardFault trampoline
        ├── system_stm32g4xx.c         # CMSIS SystemInit (FPU CPACR + DSB/ISB) & SystemCoreClockUpdate
        └── main.c                     # 170 MHz Boost blinky (PA5 LD2 via BSRR + Active-HIGH PC13 B1 per UM2505)
```

---

## Build Instructions (GNU Make)

```bash
cd 23-code-library/templates

# Debug build (-Og -g3, default)
make

# Release build (-O2 -g)
make BUILD_TYPE=release

# Generate mixed C + Thumb-2 disassembly listing (build/firmware.lss)
make disasm

# Detailed per-section ELF size report
make size

# Clean build artifacts
make clean
```

> **Custom Toolchain Path:** If `arm-none-eabi-gcc` is not on your `PATH`, pass `TOOLCHAIN_BIN`:
> ```bash
> make TOOLCHAIN_BIN=/opt/arm-gnu-toolchain/bin
> ```

---

## Build Instructions (CMake + Ninja)

```bash
cd 23-code-library/templates

cmake -B build/cmake -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE=cmake/arm-none-eabi-gcc.cmake \
  -DCMAKE_BUILD_TYPE=Debug

cmake --build build/cmake
```

---

## Flashing & Debugging (NUCLEO-G474RE with STLINK-V3E)

Per **UM2505**, the NUCLEO-G474RE integrates an onboard **STLINK-V3E** debug probe.

```bash
# Flash using upstream OpenOCD (>= 0.12)
make flash-openocd

# Or flash using STM32CubeProgrammer CLI
make flash
```

### CMSIS-SVD Setup for VS Code Cortex-Debug
Download the official `STM32G474.svd` from STMicroelectronics (STM32G4 SVD package or `cmsis-svd-data`) and save it to:
`23-code-library/Drivers/SVD/STM32G474.svd`
The repository's `.vscode/launch.json` points Cortex-Debug directly to this path for live peripheral register inspection (`XPERIPHERALS`).
