# Stage 05: GNU ARM Toolchain, Linker Scripts, Startup Code & Hardware Debugging

> **Curriculum Track:** Level 1–3 Bare-Metal Build, Link & Debug Engineering (`TOOL-*`, all 32 concepts)  
> **Verified Toolchain:** `arm-none-eabi-gcc 14.3.1` (GNU Tools for STM32), GNU Binutils, OpenOCD, GDB, VS Code Cortex-Debug  
> **Target Probe & MCU:** Onboard **STLINK-V3E** on **NUCLEO-G474RE (MB1367, UM2505)** targeting **STM32G474RET6**  
> **Code Verification Policy:** Linker/startup/Makefile snippets below are **Statically Reviewed against `stm32g474xx.h` & GNU LD/GCC 14.3.1 (Not Tested on Physical Hardware)**.

---

## 1. Learning Objectives

Upon completing Stage 05, you will master the entire bare-metal compile → link → flash → debug → verify pipeline:

| Domain | Concept IDs | Level | Mastery Objective |
|---|---|---|---|
| Compilation & Binutils | `TOOL-PIPE-001`, `TOOL-GCC-001..003`, `TOOL-FMT-001` | L1–L2 | Trace `-E`/`-S`/`-c`/link (`-save-temps`), wield `objcopy`/`objdump`/`nm`/`readelf`/`size`/`addr2line`, configure hard-float FPv4-SP ABI, link `newlib-nano` (`--specs=nano.specs`, `_sbrk`, `_write`), and decode ELF/HEX/SREC/BIN formats. |
| Optimizations & Warnings | `TOOL-OPT-001..002`, `TOOL-WARN-001` | L1–L2 | Compare `-O0`/`-Og`/`-Os`/`-O2`/`-O3`/`-flto`, apply `-ffunction-sections -fdata-sections -Wl,--gc-sections` with `KEEP()`, and enforce strict warnings (`-Werror -Wconversion -Wdouble-promotion -Wvla -fstack-usage`). |
| Linker & Startup | `TOOL-LD-001..003`, `TOOL-BOOT-001..002`, `TOOL-LINK-001..002` | L1–L2 | Author STM32G474RE linker scripts (`FLASH`/`RAM`/`CCMRAM (xrw)`/`VMA` vs `LMA`/`ALIGN(512)` `.isr_vector`/`.noinit`/`ASSERT()`), build the complete 118-entry vector table & `Reset_Handler` (`.data`/`.ccmram`/`.bss`/`__libc_init_array`), manage `.a` archives/weak symbols, and explain why dynamic linking (`TOOL-LINK-002`) is `NOT_APPLICABLE` to bare-metal XIP firmware. |
| Analysis & Memory Profiling | `TOOL-ANALYZE-001`, `TOOL-DASM-001`, `TOOL-MEM-001..002` | L1–L2 | Audit `.map` files (`--cref`, `--print-memory-usage`), mixed disassembly (`objdump -d -S`), DWARF `-g3`, static stack depth (`-fstack-usage`), and runtime `0xA5A5A5A5` stack watermarking. |
| SWD, GDB, SVD & Trace | `TOOL-GDB-001..002`, `TOOL-PROBE-001..002`, `TOOL-FLASH-001`, `TOOL-DBG-001..003` | L1–L3 | Operate STLINK-V3E + OpenOCD + CLI GDB + `.gdbinit`, configure VS Code Cortex-Debug + `STM32G474.svd`, recover locked SWD via connect-under-reset, use 6 FPB breakpoints, 4 DWT watchpoints + `CYCCNT`, `DBGMCU` freeze, SWO/ITM (`PB3`), and SEGGER RTT. |
| Build Systems & QA | `TOOL-BUILD-001..002`, `TOOL-QA-001..003` | L2–L3 | Architect portable GNU Make (`-MMD -MP`) and CMake + Ninja (`CMakePresets.json`, `compile_commands.json`), automate static analysis (`cppcheck`, `clang-tidy`, `-fanalyzer`), and run host unit tests (Unity/CMock) & HIL/emulation (`pytest`/Renode). |

---

## 2. Prerequisites

- [Stage 02 — C Language Foundations](../02-c-fundamentals/README.md) (`C-COMP-*`, `C-STOR-*`, `C-OPT-*`).
- [Stage 04 — ARM Cortex-M4F Microarchitecture](../04-arm-cortex-m/README.md) (`ARM-MEM-001`, `ARM-EXC-003`, `ARM-FPU-001`, `ARM-DBG-001`).

---

## 3. Detailed Technical Theory

### 3.1 GCC Pipeline, ARM Binutils & Hard-Float ABI (`TOOL-PIPE-001`, `TOOL-GCC-001..003`, `TOOL-FMT-001`)

1. **4-Stage Build Flow (`-save-temps`):** `main.c` $\xrightarrow{\text{cpp (-E)}}$ `main.i` $\xrightarrow{\text{cc1 (-S)}}$ `main.s` $\xrightarrow{\text{as (-c)}}$ `main.o` $\xrightarrow{\text{ld}}$ `firmware.elf`.
2. **Essential ARM GNU Binutils:**
   - `arm-none-eabi-objcopy`: Converts `.elf` → Intel HEX (`-O ihex`), Motorola SREC (`-O srec`), or raw flashable `.bin` (`-O binary`). *(Warning: Raw `.bin` carries no address metadata! Any large address gap between two `PT_LOAD` LMAs creates multi-megabyte zero-padded `.bin` files!)*
   - `arm-none-eabi-objdump -h -S -d firmware.elf`: Mixed C + Thumb-2 disassembly (requires `-g3` DWARF debug info — `.debug_*` sections live only in `.elf` and add **0 bytes** to Flash!).
   - `arm-none-eabi-nm --size-sort -S -r firmware.elf`: Lists largest Flash/RAM symbols and binding codes (`T`/`t` text, `D`/`d` data, `B`/`b` bss, `R` rodata, `W` weak, `U` undefined).
   - `arm-none-eabi-readelf -h -l -S -A firmware.elf`: Inspects ELF headers, `PT_LOAD` VMA/LMA segments, section sizes, and ARM EABI attributes (`Tag_ABI_VFP_args`).
   - `arm-none-eabi-size -A firmware.elf`: Exact per-section Flash/RAM breakdown (paired with linker flag `-Wl,--print-memory-usage`).
   - `arm-none-eabi-addr2line -e firmware.elf -f -i 0x080003A4`: Maps a raw HardFault `PC` directly back to exact C source file and line!
3. **Mandatory STM32G474RE Target & Hard-Float ABI Flags (`TOOL-GCC-002`):**
   ```text
   -mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard
   ```
   - `-mthumb`: Mandatory on Cortex-M (no ARM 32-bit state exists).
   - `-mfpu=fpv4-sp-d16`: Single-precision hardware FPU (32×32-bit `S0–S31` aliased as 16×64-bit `D0–D15`).
   - `-mfloat-abi=hard`: Passes `float` arguments directly in `S0–S15` (fastest; incompatible with `soft`/`softfp` precompiled `.a` libraries!).
4. **Newlib vs Newlib-Nano & Syscalls (`TOOL-GCC-003`):**
   - Full `newlib` consumes ~30–60 KB Flash for `printf`. Passing **`--specs=nano.specs --specs=nosys.specs`** links **`newlib-nano`** (~4–8 KB Flash). Enable floating-point `printf` only when needed via `-u _printf_float`.
   - Retarget `printf()` to USART2 by implementing `int _write(int file, char *ptr, int len)`. If dynamic allocation is ever linked, `_sbrk()` must check heap growth against current `MSP` to prevent heap-stack collision. Never ship semihosting (`--specs=rdimon.specs`, executes `BKPT 0xAB`) in standalone firmware — it hard-halts immediately when detached from GDB!

---

### 3.2 Optimizations, Dead-Code Stripping & Hardened Warnings (`TOOL-OPT-001..002`, `TOOL-WARN-001`)

| Flag | Purpose & Bare-Metal Impact |
|---|---|
| `-Og -g3 -gdwarf-4` | **Best debug profile:** Fast, compact, preserves accurate GDB line stepping & macro info. |
| `-Os` / `-O2` | **Release profiles:** `-Os` minimizes Flash; `-O2` maximizes DSP/control loop speed. |
| `-flto` | Cross-TU Link-Time Optimization (mark weak ISRs/vectors `__attribute__((used))`!). |
| `-ffunction-sections -fdata-sections -Wl,--gc-sections` | Places every function/variable in its own section so `ld` deletes dead code (`--print-gc-sections` lists stripped items; **requires `KEEP(*(.isr_vector))`**!). |
| `-fno-tree-loop-distribute-patterns` | Prevents `-O2`/`-O3` from replacing `Reset_Handler`'s `.bss` zero-loop with a recursive `memset()` call under `-nostdlib`. |
| `-Wall -Wextra -Werror -Wpedantic -Wshadow -Wconversion -Wdouble-promotion -Wundef -Wvla -fstack-usage` | Production warning gate catching accidental `double` literals (`-Wdouble-promotion`), stack VLAs, truncation, shadowing, and generating per-function `.su` stack reports. |

---

### 3.3 STM32G474RE Linker Script Architecture (`TOOL-LD-001..003`)

The GNU Linker Script (`stm32g474re.ld`) maps input sections into physical STM32G474RET6 silicon regions:
- **`MEMORY` Block:**
  - `FLASH (rx)   : ORIGIN = 0x08000000, LENGTH = 512K`
  - `RAM (xrw)    : ORIGIN = 0x20000000, LENGTH = 96K` *(80 KB SRAM1 + 16 KB SRAM2 contiguous)*
  - `CCMRAM (xrw) : ORIGIN = 0x10000000, LENGTH = 32K` *(**MUST include `x` execute flag!** STM32G4 CCM-SRAM executes code at 0 wait states on I-Code/D-Code buses! Note: `0x20018000` is the S-Bus DMA alias of the SAME 32 KB — do not allocate both!)*
- **VMA vs LMA (`AT>` & `LOADADDR()`):**
  - **VMA (Virtual/Execution Address):** Where the CPU reads/writes the section at runtime (e.g., `.data` in `RAM` `0x20000000`; `.ccmram` in `CCMRAM` `0x10000000`).
  - **LMA (Load Address):** Where the initializer bytes are burned inside non-volatile `FLASH` (`> RAM AT> FLASH` and `> CCMRAM AT> FLASH`). Export `_sidata = LOADADDR(.data);` and **`_siccmram = LOADADDR(.ccmram);`** for `Reset_Handler`!
- **Critical Linker Rules Often Broken in Starter Scripts:**
  1. **`.isr_vector` Alignment:** Use **`ALIGN(512)`** (not `ALIGN(4)`) so relocated vector tables satisfy Cortex-M4 `SCB->VTOR` alignment (`472 B -> 512 B`). Always wrap in **`KEEP(*(.isr_vector))`**.
  2. **Constructor Arrays:** Include `.preinit_array`, `.init_array`, and `.fini_array` so `__libc_init_array()` executes `__attribute__((constructor))` hooks cleanly.
  3. **`.noinit (NOLOAD)` Section:** Placed in `RAM` with `(NOLOAD)` so crash telemetry structs survive soft/watchdog resets without being zeroed by `.bss` init!
  4. **`._user_heap_stack (NOLOAD)` & `ASSERT()` Guards:** Verify at link time that `(_estack % 8) == 0` (AAPCS), `SIZEOF(.isr_vector) == 472`, and RAM has room for `_Min_Stack_Size`.
  5. **NEVER `/DISCARD/ { libc.a(*); libm.a(*); libgcc.a(*); }`:** Discarding `libgcc.a` breaks 64-bit integer division (`__aeabi_uldivmod`) and `newlib-nano` `printf()`!

---

### 3.4 Complete Boot Sequence, 118-Entry Vector Table & Static Archives (`TOOL-BOOT-001..002`, `TOOL-LINK-001..002`)

1. **Hardware Reset Fetch:** Core loads initial `MSP` (`_estack = 0x20018000`) from `0x00000000` (aliased to `0x08000000`), loads `Reset_Handler` (with `bit[0] = 1` Thumb bit) from `0x00000004`, and branches.
2. **Complete STM32G474RE Vector Table (`472 bytes = 0x1D8`):**
   Per `stm32g474xx.h` (`IRQn_Type`), STM32G474RE has **16 core exceptions + 102 external IRQs (`IRQ 0..101`, with IRQ 85 reserved `0` since AES does not exist on G474)** = **118 total 32-bit entries**! Use `typedef void (*isr_fn_t)(void);` (or a union `{ isr_fn_t fn; uintptr_t addr; }`) to avoid `-Wpedantic` function-to-object pointer cast warnings, and bind handlers with `__attribute__((weak, alias("Default_Handler")))`.
3. **`Reset_Handler` Initialization Sequence:**
   1. Optionally paint stack (`_ebss.._estack`) with `0xA5A5A5A5UL` (`TOOL-MEM-002`) and zero parity-protected RAM if `SRAM_PE` is enabled.
   2. Copy `.data` from `_sidata` (Flash) → `_sdata.._edata` (SRAM1).
   3. **Copy `.ccmram` from `_siccmram` (Flash) → `_sccmram.._eccmram` (`0x10000000`)!**
   4. Zero `.bss` (`_sbss.._ebss`).
   5. Call `SystemInit()` (enables FPU `SCB->CPACR` CP10/CP11 + **`__DSB(); __ISB();`**!).
   6. Call `__libc_init_array()` (constructors / newlib init).
   7. Branch to `main()`.
4. **Static Archives (`.a`) vs Dynamic Linking (`TOOL-LINK-001`, `TOOL-LINK-002`):**
   - Create reusable driver libraries with `arm-none-eabi-ar rcs libdrv.a gpio.o uart.o`. GNU `ld` scans archives **left-to-right**, so place `-ldrv` *after* `main.o` (or wrap circular deps in `-Wl,--start-group ... -Wl,--end-group`).
   - **`TOOL-LINK-002` (Dynamic Linking / `.so`/`.dll`/PLT/GOT): `NOT_APPLICABLE` to bare-metal STM32G474RE.** MMU-less Cortex-M4 executes statically linked Execute-In-Place (XIP) firmware directly from internal Flash with absolute VMA resolution.

---

### 3.5 STLINK-V3E, SWD, OpenOCD, GDB, VS Code SVD & Real-Time Trace (`TOOL-GDB-*`, `TOOL-PROBE-*`, `TOOL-FLASH-001`, `TOOL-DBG-*`)

1. **STLINK-V3E & SWD Interface (`TOOL-PROBE-001`):**
   NUCLEO-G474RE (UM2505) integrates an **STLINK-V3E** (USB High-Speed, faster SWD clock & integrated VCP on `PA2`/`PA3`, unlike legacy ST-LINK/V2-1). SWD uses only **`PA13` (`SWDIO`)**, **`PA14` (`SWCLK`)**, **`NRST`**, `GND`, plus **`PB3` (`SWO`)** for single-wire trace.
   - **Recovering a "Bricked" Chip (SWD disabled or stuck in Stop/Standby):** Hold hardware reset via **Connect Under Reset**:
     - OpenOCD: `openocd -f interface/stlink.cfg -f target/stm32g4x.cfg -c "reset_config srst_only connect_assert_srst"`
     - STM32CubeProgrammer CLI: `STM32_Programmer_CLI -c port=SWD mode=UR -e all`
2. **OpenOCD + CLI GDB + `.gdbinit` (`TOOL-GDB-001`, `TOOL-GDB-002`, `TOOL-FLASH-001`):**
   Start GDB server on TCP `:3333` (Telnet `:4444`) using upstream OpenOCD:
   ```bash
   openocd -f interface/stlink.cfg -f target/stm32g4x.cfg
   ```
   Connect `arm-none-eabi-gdb build/firmware.elf` with `.gdbinit`:
   ```gdb
   target extended-remote :3333
   monitor reset halt
   load
   monitor reset init
   thb main
   continue
   ```
   Essential GDB commands: `hb func` (hardware breakpoint), `watch *(uint32_t*)0x20000010` (DWT data write watchpoint), `x/16wx $sp` (stack hex dump), `x/1wx 0x48000000` (`GPIOA->MODER`), `info reg`, `bt full`, `disassemble /m main`, `si` (step 1 Thumb-2 instruction).
3. **VS Code Cortex-Debug & CMSIS-SVD (`TOOL-PROBE-002`):**
   Pair `marus25.cortex-debug` with `STM32G474.svd` in `.vscode/launch.json` (`"svdFile": "${workspaceFolder}/STM32G474.svd"`) to browse live decoded bitfields of `RCC`, `GPIOA`, `TIM2`, `ADC1`, and `SCB->CFSR` in the `CORTEX PERIPHERALS` panel without leaving the editor!
4. **Hardware Breakpoints, Watchpoints, SWO/ITM & SEGGER RTT (`TOOL-DBG-001..003`):**
   - **FPB (6 HW Breakpoints):** Flash is read-only, so GDB must use the Cortex-M4's **6 hardware FPB comparators** (RAM allows unlimited software `BKPT`).
   - **DWT (4 Data Watchpoints + `CYCCNT`):** Catch elusive pointer corruption immediately on the exact offending `STR` instruction using `watch`.
   - **`DBGMCU` Freeze:** Set `DBGMCU->APB1FZR1 |= DBGMCU_APB1FZR1_DBG_IWDG_STOP | DBGMCU_APB1FZR1_DBG_TIM2_STOP;` so the watchdog doesn't reset the MCU while halted at a breakpoint!
   - **Non-Blocking Live Trace:** Stream `ITM_SendChar()` over `PB3` (`SWO`) at 2 MHz without halting the CPU, OR use **SEGGER RTT** (lock-free RAM ring buffer read by OpenOCD/probe-rs over SWD — needs **zero extra pins**!).

---

### 3.6 Build Systems, Static Analysis, Stack Watermarking & Automated Testing (`TOOL-MEM-*`, `TOOL-BUILD-*`, `TOOL-QA-*`)

- **Portable GNU Make (`TOOL-BUILD-001`):** Always pass `-MMD -MP` and `-include $(DEPS)` so editing any `.h` header automatically recompiles every dependent `.c` file! Support `BUILD_TYPE=debug|release`, preserve directory paths in `build/`, and never hardcode machine-specific Windows IDE paths.
- **Modern CMake + Ninja (`TOOL-BUILD-002`):** Use a `cmake/arm-none-eabi.cmake` toolchain file + `CMakePresets.json` + `-DCMAKE_EXPORT_COMPILE_COMMANDS=ON` so `clangd` and `clang-tidy` understand exact cross-compiler defines and include paths.
- **Static & Runtime Stack Verification (`TOOL-MEM-001`, `TOOL-MEM-002`):** Combine compile-time `-fstack-usage` `.su` files + worst-case nested ISR frame math (32 B / 104 B FPU) with runtime **`0xA5A5A5A5` stack painting** in `Reset_Handler` to measure empirical peak stack high-water mark!
- **Static Analysis & Automated Testing (`TOOL-QA-001..003`):**
  - Run `cppcheck --enable=all --std=c11 --error-exitcode=1`, `clang-tidy`, `arm-none-eabi-gcc -fanalyzer`, and `clang-format --dry-run --Werror` on every commit.
  - Run host-native **Unity / Ceedling / CppUTest** unit tests (with fake `GPIO_TypeDef` RAM structs and `-fsanitize=address,undefined`) plus **Renode / QEMU** headless emulation and **`pytest` + `pyserial` + OpenOCD** HIL tests on NUCLEO-G474RE!

---

## 4. Important Terminology

| Term | Engineering Definition |
|---|---|
| **VMA vs LMA** | Virtual Memory Address (runtime execution address, e.g., SRAM `0x20000000`) vs Load Memory Address (non-volatile Flash storage address, e.g., `0x0800xxxx`). |
| **Relocation** | Placeholder record inside a `.o` file (`R_ARM_THM_CALL`) patched by GNU `ld` once final absolute symbol addresses are assigned. |
| **Weak Symbol** | Symbol marked `__attribute__((weak))` that the linker silently replaces if any translation unit defines a strong symbol with the same name (used for default ISR vectors). |
| **CMSIS-SVD** | System View Description XML file describing every peripheral base address, register offset, and bitfield for live IDE debugger inspection. |
| **Connect Under Reset** | SWD recovery sequence asserting physical `NRST` while the debugger attaches, halting the CPU before broken firmware can disable `PA13`/`PA14` or enter Shutdown mode. |
| **Stack Watermarking** | Filling unused stack RAM with a known sentinel (`0xA5A5A5A5`) at boot and scanning upward after stress testing to measure peak bytes actually touched. |

---

## 5. Register-Level Explanations

- **`DBGMCU->APB1FZR1` (`0xE0042008`, RM0440 §47.16):**
  - `DBG_TIM2_STOP` (bit 0) .. `DBG_TIM7_STOP` (bit 5): Halts timers when CPU stops at a GDB breakpoint.
  - `DBG_WWDG_STOP` (bit 11) & `DBG_IWDG_STOP` (bit 12): Prevents watchdogs from resetting the board mid-debug session!
- **`SCB->CPACR` (`0xE000ED88`):** Must be enabled (`0xF << 20`) + `__DSB(); __ISB();` inside `SystemInit()` before any `-mfloat-abi=hard` function runs.

---

## 6. Relevant Formulas & Calculations

### 6.1 Flash Image Utilization & `.bin` Size Verification
$$\text{Flash Used} = \text{sizeof}(\text{.isr\_vector}) + \text{sizeof}(\text{.text}) + \text{sizeof}(\text{.rodata}) + \text{sizeof}(\text{.data}) + \text{sizeof}(\text{.ccmram})$$
$$\text{SRAM1+2 Used} = \text{sizeof}(\text{.data}) + \text{sizeof}(\text{.bss}) + \text{sizeof}(\text{.\_user\_heap\_stack})$$
*(Remember: `.bss` and `.noinit` occupy **0 bytes** in Flash / `.bin`!)*

### 6.2 Worst-Case Static Stack Bound
$$S_{\text{total}} = S_{\text{main\_call\_chain}} + \sum_{k=1}^{N_{\text{prio\_levels}}} \left( S_{\text{hw\_frame},k} + S_{\text{ISR\_max},k} \right) \le 0.75 \times \text{\_Min\_Stack\_Size}$$

---

## 7. Official Documentation References

- **GNU GCC Manual** (Ch. 3 Options, §3.8 Warnings, §3.11 Optimization, `-fstack-usage`, `-fanalyzer`).
- **GNU Binutils & GNU LD Manuals** (`ld` Scripts: `MEMORY`, `SECTIONS`, `AT>`, `LOADADDR`, `KEEP`, `NOLOAD`, `ASSERT`, `--gc-sections`, `--print-memory-usage`; `objdump`, `objcopy`, `nm`, `readelf`, `addr2line`, `ar`).
- **OpenOCD User's Guide** & **ST UM2237** (*STM32CubeProgrammer CLI*).
- **ST UM2505** (*NUCLEO-G474RE STLINK-V3E & SWD*), **RM0440** (Table 111 Vector Table & Ch. 47 `DBGMCU`), **ARM DDI 0439** (FPB/DWT/ITM).

---

## 8. Verified Code Examples

> **Verification Status:** C startup snippet **Compile-Verified (`arm-none-eabi-gcc 14.3.1`, `-O2 -Wall -Wextra -Werror -Wconversion -Wdouble-promotion -Wvla`)**; Linker script excerpt Statically Reviewed against GNU LD & `stm32g474xx.h` (Not Tested on Physical Hardware).

### 8.1 Corrected STM32G474RE Linker Script Excerpt (`ALIGN(512)`, `CCMRAM (xrw)`, `.noinit`, `ASSERT()`)

```ld
ENTRY(Reset_Handler)

_estack = ORIGIN(RAM) + LENGTH(RAM); /* 0x20018000 (8-byte aligned) */
_Min_Heap_Size  = 0x0200;
_Min_Stack_Size = 0x0800;

MEMORY
{
  FLASH  (rx)  : ORIGIN = 0x08000000, LENGTH = 512K
  RAM    (xrw) : ORIGIN = 0x20000000, LENGTH = 96K
  CCMRAM (xrw) : ORIGIN = 0x10000000, LENGTH = 32K
}

SECTIONS
{
  .isr_vector :
  {
    . = ALIGN(512);
    KEEP(*(.isr_vector))
    . = ALIGN(4);
  } >FLASH

  .text :
  {
    . = ALIGN(4);
    *(.text .text*)
    *(.rodata .rodata*)
    . = ALIGN(4);
  } >FLASH

  .preinit_array : { PROVIDE_HIDDEN(__preinit_array_start = .); KEEP(*(.preinit_array*)) PROVIDE_HIDDEN(__preinit_array_end = .); } >FLASH
  .init_array    : { PROVIDE_HIDDEN(__init_array_start = .); KEEP(*(SORT(.init_array.*))) KEEP(*(.init_array*)) PROVIDE_HIDDEN(__init_array_end = .); } >FLASH

  _sidata = LOADADDR(.data);
  .data : { . = ALIGN(4); _sdata = .; *(.data .data*) . = ALIGN(4); _edata = .; } >RAM AT> FLASH

  _siccmram = LOADADDR(.ccmram);
  .ccmram : { . = ALIGN(4); _sccmram = .; *(.ccmram .ccmram*) . = ALIGN(4); _eccmram = .; } >CCMRAM AT> FLASH

  .bss (NOLOAD) : { . = ALIGN(4); _sbss = .; *(.bss .bss*) *(COMMON) . = ALIGN(4); _ebss = .; } >RAM
  .noinit (NOLOAD) : { . = ALIGN(4); *(.noinit .noinit*) . = ALIGN(4); } >RAM

  ._user_heap_stack (NOLOAD) :
  {
    . = ALIGN(8);
    PROVIDE(end = .);
    PROVIDE(_end = .);
    . = . + _Min_Heap_Size;
    . = . + _Min_Stack_Size;
    . = ALIGN(8);
  } >RAM
}

ASSERT((_estack % 8) == 0, "ERROR: _estack must be 8-byte aligned per AAPCS!")
ASSERT(SIZEOF(.isr_vector) == 472, "ERROR: STM32G474RE vector table must be exactly 118 entries (472 bytes)!")
```

### 8.2 `.ccmram` Copy & Stack Watermarking in `Reset_Handler` (`TOOL-BOOT-002`, `TOOL-MEM-002`)

```c
#include <stdint.h>

extern uint32_t _sidata, _sdata, _edata;
extern uint32_t _siccmram, _sccmram, _eccmram;
extern uint32_t _sbss, _ebss, _estack;

extern void SystemInit(void);
extern void __libc_init_array(void);
extern int  main(void);

void Reset_Handler(void)
{
    /* 1. Copy .data from Flash LMA -> SRAM VMA */
    uint32_t *src = &_sidata;
    for (uint32_t *dst = &_sdata; dst < &_edata; ) { *dst++ = *src++; }

    /* 2. Copy .ccmram from Flash LMA -> CCM-SRAM VMA (0x10000000) */
    src = &_siccmram;
    for (uint32_t *dst = &_sccmram; dst < &_eccmram; ) { *dst++ = *src++; }

    /* 3. Zero .bss */
    for (uint32_t *dst = &_sbss; dst < &_ebss; ) { *dst++ = 0UL; }

    /* 4. Paint remaining stack up to current SP with 0xA5A5A5A5 for watermark profiling */
    uint32_t current_sp = 0UL;
    __asm__ volatile ("mov %0, sp" : "=r"(current_sp));
    for (uint32_t *p = &_ebss; (uintptr_t)p < (uintptr_t)current_sp; ++p) {
        *p = 0xA5A5A5A5UL;
    }

    SystemInit();
    __libc_init_array();
    (void)main();
    while (1) { }
}
```

---

## 9. Practical STM32G474RE Experiments

1. **Full Inspection of `23-code-library/templates/`:**
   - Build with `make`, then run:
     ```bash
     arm-none-eabi-readelf -S -l build/firmware.elf
     arm-none-eabi-size -A build/firmware.elf
     arm-none-eabi-nm --size-sort -S -r build/firmware.elf | head -n 15
     arm-none-eabi-objdump -h -S -d build/firmware.elf > build/firmware.lss
     ```
   - Verify `.isr_vector` is exactly `0x1D8` (472 bytes), `.data` LMA sits in `0x0800xxxx` with VMA `0x20000000`, and `.debug_*` sections have `Addr = 0x00000000`.
2. **CLI GDB & DWT Watchpoint Hunt (`TOOL-GDB-002`, `TOOL-DBG-001`):**
   - Launch `openocd -f interface/stlink.cfg -f target/stm32g4x.cfg`, attach `arm-none-eabi-gdb`, set `watch *(uint32_t*)&g_canary`, intentionally overwrite `g_canary` via a bad pointer, and observe GDB halting on the exact offending `STR` instruction!
3. **SWD Lockout Recovery Drill (`TOOL-PROBE-001`):**
   - Reassign `PA13` to GPIO output, confirm normal SWD attach fails, and recover cleanly using `STM32_Programmer_CLI -c port=SWD mode=UR -e all` (Connect Under Reset).
   - **Debugging Checkpoint:** Inspect `x/4wx 0x08000000` in GDB to verify word 0 = `0x20018000` (`_estack`), word 1 = `Reset_Handler | 1` (odd Thumb address), and confirm `x/1wx 0xE0042008` (`DBGMCU->APB1FZR1`) freezes `IWDG` when halted.

---

## 10. Common Mistakes & Anti-Patterns

1. **Truncated or misaligned STM32G474RE vector table:** Missing IRQs `53–101` or misplacing `TIM6_DAC` (54), `TIM7_DAC` (55), `LPUART1` (91), or `DMA1_Channel7` (17) places `.text` code inside the vector table and causes instant HardFaults!
2. **Declaring `CCMRAM (rw)` without `x` or forgetting the `.ccmram` copy loop in `Reset_Handler`:** Executing code placed in `__attribute__((section(".ccmram")))` crashes on uninitialized RAM garbage!
3. **Putting `/DISCARD/ { libc.a(*); libm.a(*); libgcc.a(*); }` in the linker script:** Breaks 64-bit integer math and `newlib-nano` `printf`!
4. **Omitting `-MMD -MP` from Makefiles:** Editing a `.h` struct layout leaves stale `.o` files compiled with the old offsets — causing catastrophic runtime corruption!
5. **Omitting `KEEP(*(.isr_vector))` when `--gc-sections` is active:** The linker sees nothing calling `vector_table` and strips the entire vector table to 0 bytes!
6. **Hardcoding local Windows STM32CubeIDE OpenOCD paths in `Makefile`:** Breaks builds on every teammate's laptop and CI runner.
7. **Forgetting `DBGMCU->APB1FZR1` freeze bits when debugging watchdogs/timers:** `IWDG` keeps counting while the CPU is halted in GDB and resets the chip mid-step!

---

## 11. Hands-On Exercises

1. **Binary Size & Optimization Shootout:** Compile a firmware build containing a 64-tap FIR filter at `-O0`, `-Og`, `-Os`, `-O2`, and `-O2 -flto`. Tabulate `.text`, `.data`, `.bss` (`arm-none-eabi-size`) and FIR execution cycles (`DWT->CYCCNT`).
2. **Decode Intel HEX by Hand:** Run `arm-none-eabi-objcopy -O ihex` and manually decode the first 3 lines (Extended Linear Address record `04` = `0x0800`, Data record `00` showing little-endian `_estack`, and checksum byte).
3. **Static + Dynamic Stack Proof:** Enable `-fstack-usage -Wstack-usage=256`, compute theoretical worst-case stack depth from `.su` files, then verify runtime watermark via `0xA5A5A5A5` scanning in GDB.
4. **CMake + Ninja Cross-Build:** Write a `cmake/arm-none-eabi.cmake` toolchain file + `CMakeLists.txt` exporting `compile_commands.json`, and run `cppcheck` + `arm-none-eabi-gcc -fanalyzer` with zero warnings.

---

## 12. Assessment Questions

1. Explain the exact difference between VMA and LMA for `.data`, `.bss`, `.rodata`, and `.ccmram` on STM32G474RE. Which sections consume Flash space in `.bin`?
2. Why must `.isr_vector` use `ALIGN(512)` rather than `ALIGN(4)` on STM32G474RE, and what happens if `KEEP(*(.isr_vector))` is removed when `-Wl,--gc-sections` is enabled?
3. Compare `-mfloat-abi=soft`, `softfp`, and `hard` for `float add(float a, float b)` in generated Cortex-M4F assembly.
4. Why does `arm-none-eabi-gdb` stop accepting new breakpoints in Flash after breakpoint #6 on Cortex-M4, and what hardware block limits data watchpoints to 4?
5. Why does `arm-none-eabi-gcc -O2 -nostdlib` sometimes fail to link with `undefined reference to 'memset'` on `Reset_Handler`'s `.bss` zero-loop, and which flag prevents it?
6. How do you recover an STM32G474RE whose firmware accidentally configured `PA13`/`PA14` as GPIO outputs at boot?

---

## 13. Measurable Completion Criteria

- [ ] **Zero-Defect Linker & Startup Built:** Verify `readelf -S` shows `.isr_vector` = `0x1D8` (472 B, 512 B aligned), `CCMRAM` = `RWE`, `.ccmram` initialized cleanly at `0x10000000`, and zero `-Wpedantic -Werror` warnings.
- [ ] **Binutils Fluency Verified:** Map a raw `PC` to C line via `addr2line`, rank top Flash/RAM symbols via `nm --size-sort`, and verify `.map` `--print-memory-usage` output.
- [ ] **CLI GDB & VS Code SVD Verified:** Connect via OpenOCD + STLINK-V3E, execute a `.gdbinit` flash+debug script, catch a bug with a DWT watchpoint, and inspect live registers via `STM32G474.svd`.
- [ ] **Automated Build & QA Gate:** Build cleanly with portable Make (`-MMD -MP`) or CMake+Ninja, passing `cppcheck`, `-fanalyzer`, and `-fstack-usage` with zero errors.
