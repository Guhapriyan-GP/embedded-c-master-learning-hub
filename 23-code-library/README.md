# Production-Grade Bare-Metal Code Library & Starter Template (STM32G474RE)

This directory contains the standalone bare-metal starter template, linker script, startup code, CMSIS device headers, and a **12-driver reusable bare-metal CMSIS driver library** for the **STM32G474RET6** on the **NUCLEO-G474RE (MB1367 Rev C)** development board.

> **Honest Verification Status:**
> - **Compile-Verified (`arm-none-eabi-gcc 14.3.1`):** All template sources and all **12 reusable drivers** compile cleanly with **0 warnings, 0 errors** in both `BUILD_TYPE=debug` (`-Og -g3`) and `BUILD_TYPE=release` (`-O2 -g`) under `-std=c11 -Wall -Wextra -Werror -Wpedantic -Wshadow -Wconversion -Wdouble-promotion -Wundef`, and link cleanly both with `--gc-sections` and without `--gc-sections` (full 12-driver archive link: `13,992 B` Flash, `2,272 B` RAM).
> - **Pure Calculation Helpers Host-Verified (`gcc 15.2.0`, `-DDRIVER_HOST_TEST`):** All pure register/math helper functions across `exti_g4`, `tim_g4`, `usart_g4`, `spi_g4`, `i2c_g4`, `adc_g4`, `dma_g4`, `iwdg_g4`, and `ring_buffer` compile and pass host unit assertions.
> - **Hardware Status:** Statically reviewed against CMSIS `stm32g474xx.h`, RM0440, DS12288, ES0430, PM0214, and UM2505 — **Not Tested on Physical NUCLEO-G474RE Hardware** (see [AUDIT/DRIVER_LIBRARY_VERIFICATION.md](../AUDIT/DRIVER_LIBRARY_VERIFICATION.md) for full verification matrix and pending hardware test hooks).

---

## Directory Structure

```text
23-code-library/
├── Drivers/                                   # 12 Reusable Bare-Metal CMSIS Drivers (STM32G474RE)
│   ├── CMSIS/                                 # Single source of truth for ARM CMSIS-Core & ST STM32G4xx headers
│   ├── clock_g4.h      / clock_g4.c           # 170 MHz & 160 MHz Range 1 Boost Mode (RM0440 §6.1.5), profile query, DWT delays
│   ├── gpio_g4.h       / gpio_g4.c            # Atomic BSRR set/reset/toggle, glitch-free Input/Output/AF/Analog & LCKR helpers
│   ├── ring_buffer.h   / ring_buffer.c        # Lock-free C11 <stdatomic.h> SPSC power-of-2 ring buffer (host-testable)
│   ├── fault_handler.h / fault_handler.c      # Cortex-M4 HardFault CFSR/HFSR/MMFAR/BFAR .noinit logger + SRAM/CCM bounds check
│   ├── exti_g4.h       / exti_g4.c            # SYSCFG->EXTICR routing, edge config, callback dispatch, rc_w1 PR1 + __DSB()
│   ├── tim_g4.h        / tim_g4.c             # TIM2/3/6/7 periodic + TIM2/3 glitch-free PWM (ARPE/OCxPE/UG), rc_w0 SR clear
│   ├── usart_g4.h      / usart_g4.c           # USART1/2 & LPUART1 dynamic BRR, bounded TX, ring-buffer RX ISR, ORE/FE/NE/PE clear
│   ├── spi_g4.h        / spi_g4.c             # SPI1/2/3 master (Modes 0-3), SPI_CR2_FRXTH, 8-bit DR FIFO access, bounded timeouts
│   ├── i2c_g4.h        / i2c_g4.c             # STM32G4 v2 I2C1/2/3 (170/160 MHz TIMINGR), ReSTART read, RELOAD >255B, 9-clk recovery
│   ├── adc_g4.h        / adc_g4.c             # ADC1/2 safe CKMODE=HCLK/4 (<=60 MHz), VREFINT + calibrated Temp Sensor (TS_CAL1/2)
│   ├── dma_g4.h        / dma_g4.c             # DMA1/2 + DMAMUX1 routing, MEM2MEM sync, circular P2M, ES0430 §2.3.1 individual IFCR clear
│   └── iwdg_g4.h       / iwdg_g4.c            # IWDG ms init (with DBGMCU_APB1FZR1 debug freeze option), bounded SR sync, kick/reset flags
└── templates/                                 # Standalone buildable starter project (links all 12 drivers)
    ├── Makefile                               # Portable GNU Make (debug/release, -MMD -MP, -fstack-usage)
    ├── CMakeLists.txt                         # Modern CMake build exporting compile_commands.json
    ├── cmake/arm-none-eabi-gcc.cmake          # ARM Cortex-M4F CMake cross-compilation toolchain file
    ├── stm32g474re.ld                         # Linker script (FLASH 512K, RAM 96K, CCMRAM 32K xrw, .noinit, ASSERTs)
    ├── Inc/                                   # Application headers
    └── Src/
        ├── startup_stm32g474re.c              # Complete 118-entry vector table, .ccmram/.data/.bss init, HardFault trampoline
        ├── system_stm32g4xx.c                 # CMSIS SystemInit (FPU CPACR + DSB/ISB) & SystemCoreClockUpdate
        └── main.c                             # 170 MHz Boost blinky (PA5 LD2 via BSRR + Active-HIGH PC13 B1 per UM2505)
```

---

## Reusable Driver Library Summary (12 Drivers)

| # | Driver | Concept IDs | Key STM32G474RE Safeguards | Host-Testable Pure Helpers (`-DDRIVER_HOST_TEST`) |
|---|---|---|---|---|
| 1 | `clock_g4` | `STM32-RCC-001`, `STM32-PWR-001`, `STM32-FLASH-001`, `TOOL-DBG-003` | `Clock_Init_160MHz()` & `Clock_Init_170MHz()` (HSI16 & 24 MHz HSE X3), Boost `R1MODE=0`, 4 WS, two-step `HPRE=/2 -> >=1 us -> HPRE=/1`, `Clock_GetSysClkHz()`, `Clock_GetProfile()` | — (MMIO/DWT) |
| 2 | `gpio_g4` | `PERIPH-GPIO-001..003`, `PERIPH-GPIO-005`, `EMB-MMIO-001` | Clears `0b11` Analog reset default; writes `MODER` last; branchless atomic `BSRR` toggle; `GPIO_ConfigInput/Output/AF/Analog` & `GPIO_LockPin()` | — (MMIO) |
| 3 | `ring_buffer` | `EMB-CONC-001`, `C-MEM-001`, `FW-ARCH-001` | Lock-free C11 `<stdatomic.h>` SPSC power-of-2 queue (`acquire`/`release`, zero malloc) | Entire module (`rb_*`) |
| 4 | `fault_handler` | `ARM-EXCP-001`, `TOOL-DBG-002`, `REL-FAULT-001` | Persistent `.noinit` `g_crash_log`, SRAM1+SRAM2 & CCM-SRAM stacked-SP bounds check | — (SCB/ASM) |
| 5 | `exti_g4` | `PERIPH-EXTI-001..003`, `PERIPH-SYSCFG-001`, `ARM-NVIC-001` | Enables `SYSCFGEN`, callback dispatch, direct `EXTI->PR1 = mask; __DSB();` (ES0430 §2.1.3) | `exti_calc_exticr_index_shift()` |
| 6 | `tim_g4` | `PERIPH-TIM-001..002`, `PERIPH-PWM-001` | Dynamic `SystemCoreClock` + APB x2 clock, `ARPE`/`OCxPE` + `EGR=UG`, `rc_w0` `SR = ~UIF` | `tim_calc_psc_arr()`, `tim_calc_pwm_ccr()` |
| 7 | `usart_g4` | `PERIPH-UART-001..002`, `PERIPH-UART-004` | Dynamic `BRR` for USART1/2 & 256x LPUART1, bounded TX, `ring_buffer_t` RX ISR, `ORE/FE/NE/PE` clear | `usart_calc_brr()`, `lpuart_calc_brr()` |
| 8 | `spi_g4` | `PERIPH-SPI-001` | Modes 0–3, `DS=0b0111` + **`SPI_CR2_FRXTH`**, strict 8-bit `*(__IO uint8_t *)&spi->DR`, `FTLVL==0 && !BSY` wait | `spi_calc_br_div()` |
| 9 | `i2c_g4` | `PERIPH-I2C-001..003` | Verified `TIMINGR` for 170/160/16 MHz (100 & 400 kHz), ReSTART read, `RELOAD` >255 B, `NACKF/BERR/ARLO` checks, 9-clock bus recovery | `i2c_select_timingr()` |
| 10 | `adc_g4` | `PERIPH-ADC-001` | `CKMODE=11` (`HCLK/4` <= 60 MHz), `DEEPPWD=0 -> ADVREGEN -> >=20 us -> ADCAL -> rc_w1 ADRDY -> ADEN`, `VREFINT` & Temp Sensor (`VSENSESEL` + `>=120 us` + 640.5 cycles) | `adc_raw_to_mv()`, `adc_calc_vdda_mv()`, `adc_temp_mdegc()` |
| 11 | `dma_g4` | `PERIPH-DMA-001..002`, `STM32-ERRATA-001` | `DMA1/2` + `DMAMUX1` routing, CMSIS channel tables, `MEM2MEM` sync, circular P2M, individual `CTCIF`/`CHTIF`/`CTEIF` clear (avoids ES0430 §2.3.1 `CGIFx` bug) | `dma_dmamux_channel_index()` |
| 12 | `iwdg_g4` | `PERIPH-WDG-001`, `STM32-RESET-001`, `STM32-DBGMCU-001` | Optional `DBGMCU->APB1FZR1.DBG_IWDG_STOP` debug freeze, bounded `LSIRDY` & `IWDG->SR` sync, `IWDGRSTF` cause inspection | `iwdg_calc_prescaler_reload()` |

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
