# Open-Source Development Tools & GitHub Utilities

This guide documents open-source utilities and tools supporting the bare-metal STM32 firmware engineering workflow.

---

### 1. Embedded Build & Toolchain Utilities

#### A. ARM GNU Toolchain (`arm-none-eabi-gcc`)
- **Official Upstream:** Arm Developer / Free Software Foundation
- **Purpose:** Cross-compilation toolchain for ARM Cortex-M processors.
- **Key Executables:**
  - `arm-none-eabi-gcc`: C/C++ compiler and preprocessor.
  - `arm-none-eabi-as`: GNU assembler.
  - `arm-none-eabi-ld`: GNU linker.
  - `arm-none-eabi-objcopy`: Extracts raw `.bin` or Intel `.hex` flash images from `.elf` binaries.
  - `arm-none-eabi-size`: Analyzes memory footprint (`text`, `data`, `bss`).
  - `arm-none-eabi-gdb`: GNU debugger for remote target debugging over GDB server.

#### B. OpenOCD (Open On-Chip Debugger)
- **Official Upstream:** `https://github.com/openocd-org/openocd`
- **Purpose:** On-chip JTAG/SWD in-system programming and GDB debug server.
- **Nucleo-G474RE Command Line:**
  ```bash
  openocd -f interface/stlink.cfg -f target/stm32g4x.cfg
  ```

#### C. ST-LINK Tools / STLinkServer
- **Official Upstream:** `https://github.com/stlink-org/stlink`
- **Purpose:** Open-source utilities for flashing and communicating with ST-LINK/V2 and ST-LINK/V3 interfaces.

---

### 2. VS Code Firmware Development Extensions
1. **Cortex-Debug (marus25):** Full GDB frontend with peripheral SVD register inspection, core register breakdown, and memory visualization.
2. **C/C++ Extension (Microsoft):** Language server, IntelliSense, and code navigation using `c_cpp_properties.json`.
3. **Memory View:** Real-time RAM and Flash memory hex viewer during active GDB debug sessions.

---

### 3. Firmware Verification & Quality Tools
1. **Ceedling / Unity / CMock:** Unit testing harness for Embedded C code running natively or in QEMU.
2. **Cppcheck:** Open-source static analysis tool for detecting undefined behavior, memory leaks, and bounds violations in C firmware.
3. **Clang-Format:** Enforces consistent source code formatting and indentation according to predefined standards.
