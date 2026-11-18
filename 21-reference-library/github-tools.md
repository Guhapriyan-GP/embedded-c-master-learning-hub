# Open-Source Embedded Toolchain, Verification & Lab Utilities

This reference catalogues production-grade open-source tools supporting bare-metal STM32G474RE compilation, debugging, static analysis, host unit testing, emulation, and hardware protocol capture.

**Concept IDs:** `TOOL-GCC-001`, `TOOL-OCD-001`, `PROF-SA-001..003`, `TEST-UNIT-001`, `TEST-HIL-002`, `HWSW-LAB-002`

---

### 1. Cross-Compilation & Binary Inspection

#### A. ARM GNU Toolchain (`arm-none-eabi-gcc`)
- **Purpose:** ISO C11/C17 cross-compiler and binutils targeting ARMv7E-M (`-mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard`).
- **Key Utilities:**
  - `arm-none-eabi-gcc`: Compiler, assembler, linker driver (enable `-Wall -Wextra -Werror -Wshadow -Wconversion -Wdouble-promotion -fstack-usage -fanalyzer`).
  - `arm-none-eabi-size -A` / `-Wl,--print-memory-usage`: Flash (`.text`+`.data`) and SRAM (`.data`+`.bss`+`.ccmram`) budgeting.
  - `arm-none-eabi-nm --size-sort -S -r`: Ranks largest symbols by Flash/RAM consumption.
  - `arm-none-eabi-objdump -d -S`: Interleaved C-and-Thumb-2 disassembly verification.
  - `arm-none-eabi-objcopy`: Generates `.bin` and `.hex` images from `.elf`.
  - `arm-none-eabi-gdb`: Remote GDB client for SWD debugging, register inspection, and batch HIL scripting.

---

### 2. Flashing, SWD Debugging & Live Trace

#### A. OpenOCD (`https://github.com/openocd-org/openocd`)
- **NUCLEO-G474RE (ST-LINK/V3E) Command:**
  ```bash
  openocd -f interface/stlink.cfg -f target/stm32g4x.cfg
  ```
- Supports GDB server (`:3333`), Telnet (`:4444`), and ITM/SWO trace capture (`tpiu config`).

#### B. `probe-rs` (`https://github.com/probe-rs/probe-rs`)
- Modern, single-binary Rust-based debug probe toolkit supporting ST-LINK/V3E, J-Link, and CMSIS-DAP with built-in flashing (`probe-rs download`), GDB server, and zero-config **RTT (Real-Time Transfer)** live console streaming.

#### C. VS Code Extensions
- **Cortex-Debug (`marus25`):** GDB frontend with STM32G474 SVD register view, Cortex-M4 fault/xPSR decoding, and SWO console.
- **clangd / C/C++ Extension:** Code navigation powered by `compile_commands.json`.

---

### 3. Static Analysis & Formatting

| Tool | Purpose | Example Bare-Metal CI Invocation |
|---|---|---|
| **`cppcheck`** | Deep C static analysis + **MISRA C:2012/2023 addon** | `cppcheck --enable=all --std=c11 --platform=arm32-wchar_t4 --addon=misra --inline-suppr --error-exitcode=1 Src/` |
| **`clang-tidy`** | LLVM AST linter (`bugprone-*`, `cert-*`, `readability-*`, `misc-*`) | `clang-tidy -p build/compile_commands.json --checks='-*,bugprone-*,cert-*,misc-*' Src/*.c` |
| **`clang-format`** | Automated deterministic C formatting gate | `clang-format --dry-run --Werror Src/*.c Inc/*.h` |
| **GCC `-fanalyzer`** | Built-in interprocedural static analyzer (null deref, leaks, uninit) | `arm-none-eabi-gcc -fanalyzer -Werror ...` |

---

### 4. Host Unit Testing, Emulation & Hardware Protocol Capture

1. **Ceedling / Unity / CMock (`ThrowTheSwitch`):**
   - **Unity:** Lightweight C unit-test framework compiled natively on host x86/x64 (`gcc -fsanitize=address,undefined` for ASan/UBSan bug detection!).
   - **CMock:** Auto-generates mock driver implementations from `.h` files to test NACK, timeout, and ISR error paths without hardware.
   - **Ceedling:** Build/test orchestrator integrating Unity, CMock, and `gcov`/`lcov` branch coverage reports.
2. **Renode (`https://github.com/renode/renode`):**
   - Open-source deterministic hardware emulator for ARM Cortex-M and STM32 platforms (`.repl` platform descriptions + `.resc` scripts) enabling headless firmware boot, UART protocol tests, and Robot Framework / `pytest` assertions in GitHub Actions CI without physical boards.
3. **PulseView / `sigrok` (`https://sigrok.org`):**
   - Open-source logic analyzer GUI (`PulseView`) and CLI (`sigrok-cli`) supporting FX2LAFW/Saleae/DreamSourceLab probes with built-in protocol decoders for **UART, SPI, I2C, CAN/CAN-FD, I2S, PWM, and SWD** (sample at $\ge 4\times\text{--}10\times$ maximum bus frequency).
