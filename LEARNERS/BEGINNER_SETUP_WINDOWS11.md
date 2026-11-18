# Windows 11 Beginner Setup Guide (VS Code + ARM GCC + NUCLEO-G474RE)

This step-by-step guide prepares a clean **Windows 11** PC for building, unit-testing, flashing, and live-debugging bare-metal **STM32G474RE** firmware in **VS Code**.

> **Concept IDs Covered:** `TOOL-GCC-001`, `TOOL-BUILD-001`, `TOOL-OCD-001`, `TOOL-FLASH-001`, `TOOL-GDB-001`, `EMB-CMSIS-003`, `TEST-UNIT-001`  
> **Repository Verification Baseline:** Compile-Verified with `arm-none-eabi-gcc 14.3.1` and host `gcc 15.2.0`; hardware debugging steps are documented against ST `UM2505` (STLINK-V3E) and upstream OpenOCD $\ge 0.12$.

---

## 1. Install the 6 Core Tools on Windows 11

Use simple installation paths **without spaces** whenever possible (e.g., `C:\Tools\...`), because GNU Make and GDB behave cleanest when paths contain no spaces.

| # | Tool | Recommended Windows 11 Installation Source | Typical `bin` Folder for `PATH` |
|---:|---|---|---|
| 1 | **ARM GNU Toolchain** (`arm-none-eabi-gcc`, `gdb`, `size`, `readelf`) | Official Arm Developer *Arm GNU Toolchain* Windows installer (`.exe`, choose *Add path to environment variable*) — **or** reuse the toolchain bundled inside STM32CubeIDE (`C:\ST\STM32CubeIDE_*\...\tools\bin`). | `C:\Program Files (x86)\Arm GNU Toolchain arm-none-eabi\14.3 rel1\bin` |
| 2 | **GNU Make** (`make`) | **MSYS2** (`pacman -S make`) or `winget install GnuWin32.Make` or xPack Windows Build Tools. *(Recommended: MSYS2 UCRT64 gives you both `make` and host `gcc` cleanly!)* | `C:\msys64\ucrt64\bin` and `C:\msys64\usr\bin` |
| 3 | **Host C Compiler (`gcc`)** | **MSYS2 UCRT64:** `pacman -S mingw-w64-ucrt-x86_64-gcc` (needed for `make -C tests test`). | `C:\msys64\ucrt64\bin` |
| 4 | **OpenOCD ($\ge 0.12.0$)** | Official xPack OpenOCD Windows zip extracted to `C:\Tools\xpack-openocd\`. | `C:\Tools\xpack-openocd\bin` |
| 5 | **STM32CubeProgrammer** | Official STMicroelectronics installer (`STM32CubeProgrammer`). **Also installs the STLINK-V3E USB and Virtual COM Port drivers!** | `C:\Program Files\STMicroelectronics\STM32Cube\STM32CubeProgrammer\bin` |
| 6 | **Python 3** ($\ge 3.10$) | `winget install Python.Python.3.12` (check *Add Python to PATH*). | Auto-added by installer |

---

## 2. Configure Windows 11 `PATH` Properly

1. Press **`Win + S`**, type **`Edit environment variables for your account`**, and press **Enter**.
2. Under **User variables**, select **`Path`** → click **Edit** → **New**, and add the `bin` directories from the table above (ARM GNU Toolchain, MSYS2/Make, OpenOCD, STM32CubeProgrammer).
3. Click **OK** on all dialogs, then **close and reopen** PowerShell and VS Code so the updated `PATH` takes effect.

> **Fallback Tip:** Even if `arm-none-eabi-gcc` is not on your `PATH`, [`23-code-library/templates/Makefile`](../23-code-library/templates/Makefile) automatically searches `C:\ST\STM32CubeIDE_*\...\tools\bin`, or you can pass `make TOOLCHAIN_BIN=C:/path/to/bin`.

---

## 3. Verify Every CLI Tool in PowerShell

Open a fresh **PowerShell** window and run all five version checks:

```powershell
arm-none-eabi-gcc --version
make --version
openocd --version
STM32_Programmer_CLI --version
gcc --version
python --version
```

**Expected Healthy Output Checklist:**
- `arm-none-eabi-gcc`: reports `13.x` or `14.x` (repository verified on `14.3.1`).
- `make`: reports `GNU Make 4.x` (or `3.81+`).
- `openocd`: reports `Open On-Chip Debugger 0.12.0` (or newer).
- `STM32_Programmer_CLI`: reports `STM32CubeProgrammer v2.x`.
- `gcc`: reports native Windows x86_64 GCC (e.g., MSYS2 `gcc 14.x`/`15.x`) for host unit tests.
- `python`: reports `Python 3.10+`.

---

## 4. Connect NUCLEO-G474RE & Find Your ST-LINK COM Port

1. **Before plugging in USB**, visually verify the board jumpers (`UM2505`):
   - **`JP5`** (`5V_SEL`): jumper installed on **`5V_STLK`** (pins 1–2) to power from USB.
   - **`JP6`** (`IDD`): jumper **installed** (bridges `VDD` to the MCU — without `JP6`, the STM32 has no power!).
2. Connect the **Micro-USB** data cable to **`CN1`** (top ST-LINK edge of the board).
3. Open **Device Manager** (`Win + X` → **Device Manager**):
   - Expand **Ports (COM & LPT)** → note **`STMicroelectronics STLink Virtual COM Port (COMx)`** (e.g., `COM4`). This is connected to **`PA2` (TX) / `PA3` (RX)** (`USART2` AF7 / `LPUART1` AF12).
   - Expand **Universal Serial Bus devices** → confirm **`STM32 STLink`** appears with no yellow warning triangle.
4. *(Optional CLI check)* Verify SWD detection without touching Flash:
   ```powershell
   STM32_Programmer_CLI -c port=SWD
   ```
   You should see `Device name : STM32G474xx`, `Flash size : 512 KBytes`, and `ST-LINK SN`.

---

## 5. Install VS Code Extensions & Load `STM32G474.svd`

### Step 5.1 — Required VS Code Extensions
Open VS Code (`code C:\workspace\embedded-c-master-learning-hub`) and install from the Extensions sidebar (`Ctrl+Shift+X`):
1. **C/C++** (`ms-vscode.cpptools`) — IntelliSense configured by [`.vscode/c_cpp_properties.json`](../.vscode/c_cpp_properties.json) (`STM32G474xx`, `-mcpu=cortex-m4`, `-mfpu=fpv4-sp-d16`, `-mfloat-abi=hard`, C11).
2. **Cortex-Debug** (`marus25.cortex-debug`) — Hardware breakpoint, disassembly, live watch, SWO ITM console, and peripheral register view configured by [`.vscode/launch.json`](../.vscode/launch.json).
3. **Serial Monitor** (`ms-vscode.vscode-serial-monitor`) — *(Optional, or use PuTTY / Tera Term)* for viewing `USART2` `115200 8N1` UART logs directly inside VS Code.

### Step 5.2 — Add the CMSIS-SVD File for Live Peripheral Register Viewing
Due to STMicroelectronics redistribution licensing, `STM32G474.svd` is not committed inside this repository. Without it, debugging still works, but the VS Code **`XPERIPHERALS`** pane will be empty.

1. Obtain `STM32G474.svd` from **either**:
   - Official STMicroelectronics **STM32G4 System View Description** download (`st.com` → STM32G474RE → *CAD Resources* → *STM32G4 SVD*), **or**
   - An existing STM32CubeIDE install (`C:\ST\STM32CubeIDE_*\...\STM32G474.svd`), **or**
   - The open `cmsis-svd-data` repository (`data/STMicro/STM32G474.svd`).
2. Create the folder `23-code-library\Drivers\SVD\` and copy the file to the exact path referenced in [`.vscode/launch.json`](../.vscode/launch.json):
   ```text
   23-code-library/Drivers/SVD/STM32G474.svd
   ```
3. During your next `F5` debug session, open the **Run and Debug** sidebar → **`XPERIPHERALS`** to expand `RCC`, `PWR`, `FLASH`, `GPIOA`, `TIM2`, `USART2`, `ADC1`, and inspect live bitfields!

### Step 5.3 — Configure Serial Terminal (`USART2` VCP)
For [`Lab 08`](../18-practical-laboratory/lab-08-usart-baremetal-driver-printf/README.md) and any `printf` output:
- **Port:** `COMx` from Device Manager (Step 4)
- **Baud Rate:** `115200`
- **Format:** `8 data bits, No parity, 1 stop bit (8N1)`, **Flow control:** `None`
- **Line ending:** `CR+LF` (or `LF`)

---

## 6. Run the Full Repository Self-Test (Final Setup Gate)

Run these three commands from the repository root to prove your Windows 11 environment is 100% ready:

```powershell
# 1. Validate repo structure, links, register invariants, builds & host tests
python scripts/validate_repo.py

# 2. Run host C unit tests directly (35 C foundation + 78 ring buffer assertions)
make -C tests test

# 3. Build bare-metal STM32G474RE starter template (debug & release)
make -C 23-code-library/templates clean all
make -C 23-code-library/templates BUILD_TYPE=release clean all
```

**You are setup-complete when:**
- `python scripts/validate_repo.py` finishes with **`0 FAIL`**.
- `make -C tests test` prints **`[ALL HOST UNIT TESTS PASSED]`**.
- `make -C 23-code-library/templates` outputs `build/firmware.elf` with **0 errors and 0 warnings**.

> **Hit a snag?** Jump straight to **[`LEARNERS/TROUBLESHOOTING_GUIDE.md`](TROUBLESHOOTING_GUIDE.md)** for exact Windows 11, PATH, compiler, linker, and ST-LINK fixes.
> **Ready to start learning?** Open **[`LEARNERS/LEARNER_PROGRESS_DASHBOARD.md`](LEARNER_PROGRESS_DASHBOARD.md)** and check off Setup Complete!
