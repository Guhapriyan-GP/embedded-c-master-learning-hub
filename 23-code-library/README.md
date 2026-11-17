# Production-Grade Bare-Metal Starter Template for STM32G474RE

This directory contains a complete, verified, standalone bare-metal starter project for the **NUCLEO-G474RE** board that builds directly from the command line using `arm-none-eabi-gcc` and GNU Make.

---

### File Structure
- `templates/Makefile`: Production Makefile configuring target architecture (`-mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard`), strict warnings (`-Wall -Wextra`), garbage collection of unused sections (`-Wl,--gc-sections`), and generation of `.elf`, `.bin`, `.hex`, and `.map` files.
- `templates/stm32g474re.ld`: Linker script defining Flash (`0x08000000`, 512K), SRAM1 (`0x20000000`, 80K), SRAM2 (`0x20014000`, 16K), and CCM-SRAM (`0x10000000`, 32K), positioning the vector table at origin, and sizing stack and heap.
- `templates/Src/startup_stm32g474re.c`: Minimal startup code containing the complete ARMv7-M vector table (16 core vectors + 102 peripheral IRQ vectors), `Reset_Handler` that copies `.data` from Flash LMA to SRAM VMA, zeroes `.bss`, calls `SystemInit()`, and branches to `main()`.
- `templates/Src/system_stm32g4xx.c`: Low-level system initialization (`SystemInit`) enabling the FPU (`SCB->CPACR`) and setting up basic clocks.
- `templates/Src/main.c`: Test application toggling PA5 (LD2) at 170 MHz.
- `Drivers/CMSIS/`: Official ARM CMSIS-Core and ST device headers (`stm32g474xx.h`, `core_cm4.h`).

---

### Build & Flash Instructions
```bash
# Navigate to the template directory
cd 23-code-library/templates

# Compile the firmware
make

# Clean build artifacts
make clean

# Flash to board using OpenOCD
openocd -f interface/stlink.cfg -f target/stm32g4x.cfg -c "program build/firmware.elf verify reset exit"
```
