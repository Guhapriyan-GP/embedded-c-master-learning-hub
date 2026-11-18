# START HERE — Beginner's Guide to Mastering Bare-Metal Embedded C on NUCLEO-G474RE

Welcome to the **Embedded C Master Learning Hub**! If you are opening this repository for the first time on **Windows 11** with **VS Code** and a **ST NUCLEO-G474RE** board, **this is your single starting point**.

> **Engineering Honesty & Verification Status (`v4.0.0` — Canonical Single Source of Truth: [`AUDIT/release_metrics.json`](AUDIT/release_metrics.json)):**
> - **Onboard Target Silicon Proof ([`AUDIT/HARDWARE_EVIDENCE_AUDIT.md`](AUDIT/HARDWARE_EVIDENCE_AUDIT.md)):** Tested live on physical **ST NUCLEO-G474RE (`MB1367 Rev C`, `STM32G474RET6` Rev X / ID `0x469`, STLINK-V3E `V3J17M11`, `VDD = 3.28–3.29 V`)**:
>   - **`9/9` Onboard Silicon Self-Test PASS** (`160 MHz` & `170 MHz` Range 1 Boost, `VDDA = 3288 mV`, `Die Temp = 32.91 °C`, `CORDIC` `9.72×` faster than FPU, `FMAC` `0 LSB` diff, `CRC-32 = 0xCBF43926`, `FDCAN1` 16 B BRS loopback, `MPU` arming + `.noinit` `DIV0` reboot persistence, `IWDG` `6/6` kicks).
>   - **Official Pinned `FreeRTOS-Kernel V11.1.0` (`23-code-library/freertos/`) — `6/6` Live Onboard PASS** (`ARM_CM4F`, `heap_4`, `PRIGROUP=0`, `BASEPRI=0x50`, Semaphores, Priority Inheritance, Deadlock Guard, `FromISR` + Queue + `fpu_err=0`, Stack HWM, 8 s `[TELEM]`).
>   - **`10/12` Reusable Drivers & `3/3` Capstone Projects (`19-projects/01..03/`) `ONBOARD HARDWARE TESTED` (`PASS`)** in zero-wire safe modes.
> - **Starter Template, 12 Reusable Drivers & Labs 01–20:** `18/18` bare-metal labs cross-compiled (`arm-none-eabi-gcc 14.3.1`, `-Wall -Wextra -Werror`, zero warnings), Lab 15 FreeRTOS cross-compiled, and Lab 20 host-tested.
> - **Host C Unit Tests (`tests/`):** **`873/873` passing assertions** across 5 test suites (`gcc 15.2.0`), `96.04%` reachable-line coverage, `0` `-fanalyzer` warnings.
> - **Deferred Hardware:** Breadboard sensor/instrument setups (`Labs 07, 09, 11, 12, 14, 16`) and external power-stage connections (`Capstone 03` `POWER_SIM_MODE=0`) are honestly documented in [`HARDWARE_VALIDATION/PENDING_EXTERNAL_CONNECTIONS.md`](HARDWARE_VALIDATION/PENDING_EXTERNAL_CONNECTIONS.md). Always inspect jumpers (`JP6` vs `JP5`) and voltage limits ($V_{\text{DD}} \le 3.3\text{ V}$) before connecting external hardware.

---

## 1. How to Learn Effectively (The 5-Step Mastery Loop)

Do **not** read 17 chapters passively from top to bottom before touching a compiler. Embedded engineering is learned by closing the loop between C syntax, silicon registers, and live debugger inspection. For **every stage**, follow this exact loop:

```text
[1. Theory] -> [2. Register Map] -> [3. Immediate Lab] -> [4. GDB Verification] -> [5. Mastery Gate]
```

1. **Theory (`01b-*` through `17-*`):** Read the stage `README.md` to build the mental model and note the Stage Concept IDs. Keep [`LEARNERS/FORMULA_AND_CALCULATION_SHEET.md`](LEARNERS/FORMULA_AND_CALCULATION_SHEET.md) open beside you.
2. **Register Map (`20-technical-diagrams/registers/` & `LEARNERS/REGISTER_QUICK_REFERENCE.md`):** Before writing a single line of C, inspect the exact peripheral base address, offsets, reset values, and access types (`RW`, `WO`, `rc_w1`, `rc_w0`).
3. **Immediate Lab (`18-practical-laboratory/` & `LEARNERS/WORKED_EXAMPLES.md`):** Build and flash the matching lab (`make`) — or run host C tests (`make -C tests test`) for C-language stages. Zero warnings under `-Werror` is mandatory.
4. **GDB Verification (VS Code Cortex-Debug or `arm-none-eabi-gdb`):** Set a breakpoint, inspect live peripheral registers (`clock_regs`, `fault_regs`, SVD `XPERIPHERALS`), single-step instructions, and verify the hardware state matches your math.
5. **Mastery Gate (`LEARNERS/REVISION_NOTES_AND_MASTERY_GATES.md` & `24-assessments/`):** Check off the stage in [`LEARNERS/LEARNER_PROGRESS_DASHBOARD.md`](LEARNERS/LEARNER_PROGRESS_DASHBOARD.md) **only** when you can explain the mechanism, write the code cleanly, and prove it in GDB/tests.

---

## 2. Hardware Requirements (NUCLEO-G474RE MB1367 Rev C)

### What You Need on Day 1 (Zero External Wires!)
You only need **two items** to complete **all of Level 1** and **14+ safe onboard labs**:
- **ST NUCLEO-G474RE** board (**MB1367 Rev C**, STM32G474RET6 LQFP64 MCU)
- **One USB-A/USB-C to Micro-USB data cable** (confirm it carries data lines, not charge-only!)

### Essential Onboard Hardware Map (`UM2505` Verified)

| Feature | Exact Pin / Header | Critical Hardware Fact & Beginner Trap |
|---|---|---|
| **Onboard Debugger & Serial Bridge** | **STLINK-V3E** (`CN1` Micro-USB) | Provides SWD flashing/debugging **and** a USB Virtual COM Port (VCP) over one cable. |
| **User LED (`LD2`, Green)** | **`PA5`** (via `SB6`) | Active-HIGH (`GPIOA->BSRR = GPIO_BSRR_BS5` turns LED ON). Also shared with Arduino `D13` / `SPI1_SCK` (watch LED loading on fast SPI). |
| **User Pushbutton (`B1`, Blue)** | **`PC13`** | **ACTIVE-HIGH** with an external pull-down resistor (`UM2505 §6.6`). Pressing `B1` drives `PC13` to `3.3 V` (`IDR` bit 13 = `1`, rising edge `RTSR1`). Do **not** enable internal pull-up! |
| **Virtual COM Port (UART)** | **`PA2` (TX) / `PA3` (RX)** | Wired to STLINK-V3E via `SB17`/`SB23`. Supports **`USART2` (AF7)** or **`LPUART1` (AF12)**. Zero external USB-UART dongle needed! |
| **High-Speed Crystal (`X3`)** | **`PF0` / `PF1`** | Populated onboard **24 MHz HSE crystal** (plus internal 16 MHz `HSI16` at reset). |
| **Current Measurement Jumper** | **`JP6` (`IDD`)** | Put an ammeter on **`JP6`** to measure MCU current (`Lab 14`). **NEVER** confuse with **`JP5`** (`5V_SEL` power source selector)! |
| **`PB8-BOOT0` Trap** | **`PB8`** (`R36`) | Has an external **10 kΩ pull-down** (`R36`) on MB1367. Internal ~40 kΩ pull-up cannot pull `PB8` HIGH for `I2C1_SCL` ($0.66\text{ V}$ divider) — use external 2.2–4.7 kΩ pull-ups or remap `I2C1_SCL` to `PA15`. |
| **`PA9` / `PA10` UCPD Trap** | **`PA9`, `PA10` (`PB6`, `PB4`)** | STM32G4 enables **5.1 kΩ USB-C dead-battery pull-downs** by default out of reset. Always set `PWR->CR3 |= PWR_CR3_UCPD_DBDIS;` before using these pins! |
| **Unbonded LQFP64 Features** | N/A on STM32G474RET6 | **No `GPIOE`**, **no `SPI4`**, **no `FMC`**, and **no `AES/CRYP`** peripheral on STM32G474RET6 (52 bonded GPIOs: `PA0–15`, `PB0–15`, `PC0–15`, `PD2`, `PF0–1`, `PG10-NRST`). |

### Optional External Kit (Needed Only Later for Labs 07, 09, 11, 12)
Breadboard + jumper wires, 10 kΩ potentiometer (`Lab 09`), common-cathode RGB LED + 3× 220 Ω resistors (`Lab 07`), SPI sensor module (`Lab 11`), SSD1306 I2C OLED + 2× 4.7 kΩ pull-ups (`Lab 12`), and an inexpensive USB logic analyzer (8-ch 24 MHz + PulseView/sigrok).

---

## 3. Software Setup on Windows 11 (Quick Checklist)

Complete step-by-step installation, Windows `PATH` configuration, Device Manager COM-port discovery, VS Code extensions, and SVD setup are in **[`LEARNERS/BEGINNER_SETUP_WINDOWS11.md`](LEARNERS/BEGINNER_SETUP_WINDOWS11.md)**.

Required free tools on Windows 11:
1. **Visual Studio Code** + extensions: `C/C++` (`ms-vscode.cpptools`) and `Cortex-Debug` (`marus25.cortex-debug`)
2. **ARM GNU Toolchain** (`arm-none-eabi-gcc`, `arm-none-eabi-gdb`, `arm-none-eabi-size`, `arm-none-eabi-readelf`)
3. **GNU Make** (via MSYS2, Chocolatey, Scoop, or xPack Windows Build Tools)
4. **OpenOCD** ($\ge 0.12.0$, for SWD debugging & flashing via STLINK-V3E)
5. **STM32CubeProgrammer CLI** (`STM32_Programmer_CLI`, includes ST-LINK USB/VCP drivers)
6. **MSYS2 / MinGW-w64 `gcc`** (for running host C unit tests in `tests/`)
7. **Python 3** (for running `python scripts/validate_repo.py`)
8. **Serial Terminal:** VS Code *Serial Monitor* extension, PuTTY, or Tera Term (for `USART2` 115200 8N1 VCP output)

---

## 4. Your First Build, Flash & GDB Debug Session (10-Minute Walkthrough)

Open **PowerShell** (or VS Code Terminal: `` Ctrl+` ``) in the repository root `embedded-c-master-learning-hub/`.

### Step A — Verify Host C Unit Tests & Repository Health
```powershell
python scripts/validate_repo.py --skip-builds
make -C tests test
```
Expected result: `test_c_foundations` (**35/35 assertions**) and `test_ring_buffer` (**78/78 assertions**) pass with exit code `0`.

### Step B — Build the Bare-Metal Starter Template
```powershell
cd 23-code-library/templates
make BUILD_TYPE=debug
```
Expected output:
- Compiles `Src/main.c`, `Src/startup_stm32g474re.c`, `Src/system_stm32g4xx.c`, and the 4 reusable drivers (`clock_g4.c`, `gpio_g4.c`, `ring_buffer.c`, `fault_handler.c`) with **0 warnings** (`-Werror`).
- Produces `build/firmware.elf`, `build/firmware.bin`, `build/firmware.hex`, and `build/firmware.map`.
- Prints exact `FLASH` (~2.5 KB / 512 KB), `RAM` (~1.7 KB / 96 KB), and `CCMRAM` (12 B / 32 KB) usage.

### Step C — Plug In NUCLEO-G474RE and Flash
Plug the Micro-USB cable into `CN1`. Verify `LD4` (ST-LINK COM) lights up and `LD5` (5V PWR) is green. Flash using **either** method:

```powershell
# Method 1: STM32CubeProgrammer CLI (writes build/firmware.bin to 0x08000000, verifies & resets)
make flash

# Method 2: OpenOCD (programs build/firmware.elf, verifies & resets)
make flash-openocd
```

### Step D — Debug in VS Code (Recommended) or CLI GDB
- **VS Code One-Click Debug:**
  1. Press **`Ctrl+Shift+B`** to run the preconfigured build task (`Build Firmware (Debug)` in [`.vscode/tasks.json`](.vscode/tasks.json)).
  2. Press **`F5`** to launch `Debug STM32G474RE (OpenOCD + STLINK-V3E)` (defined in [`.vscode/launch.json`](.vscode/launch.json)).
  3. The CPU halts at `main()`. Hover variables, watch **`XPERIPHERALS`** (once `STM32G474.svd` is placed per [`LEARNERS/BEGINNER_SETUP_WINDOWS11.md`](LEARNERS/BEGINNER_SETUP_WINDOWS11.md)), and press **`F10`** (Step Over) / **`F5`** (Continue).
- **CLI GDB (2 terminals):**
  - Terminal 1: `openocd -f interface/stlink.cfg -f target/stm32g4x.cfg`
  - Terminal 2 (from repo root):
    ```powershell
    arm-none-eabi-gdb -x .gdbinit 23-code-library/templates/build/firmware.elf
    ```
  - Inside GDB, run the built-in helper macros from [`.gdbinit`](.gdbinit):
    ```gdb
    (gdb) flash_run      # Reset, load ELF, halt at main()
    (gdb) clock_regs     # Inspect RCC->CR/CFGR/PLLCFGR, PWR->CR1/SR2/CR5, FLASH->ACR
    (gdb) fault_regs     # Inspect SCB->CFSR/HFSR/MMFAR/BFAR + MSP/PSP/LR/PC
    ```

---

## 5. Exact Ordered Learning Roadmap (Acyclic Sequence)

> **Critical Study Order Rule (Why Folders Are Not Strictly Numerical):**
> 1. Study **Basic GPIO on reset 16 MHz HSI16** (`08-gpio/`, Labs 01–02) **BEFORE** **170 MHz RCC PLL Boost** (`07-rcc-and-clocks/`, Lab 03), because Lab 03 uses GPIO `PA5` and `PA8` (`MCO`) to verify the 170 MHz clock!
> 2. Study **Interrupts, SysTick, NVIC & EXTI** (`10-interrupts/`, Labs 04–05) **BEFORE** **Timers & PWM** (`09-timers/`, Labs 06–07), because Lab 06 immediately requires `TIM2_IRQHandler` and `NVIC_EnableIRQ()`!

Track your progress stage-by-stage in **[`LEARNERS/LEARNER_PROGRESS_DASHBOARD.md`](LEARNERS/LEARNER_PROGRESS_DASHBOARD.md)**.

| Order | Stage & Topic | Folder | Immediate Labs / Tests | Mastery Level |
|---:|---|---|---|---|
| 1 | **Stage 01b:** Digital Electronics, MB1367 Hardware & Lab Instruments | [`01b-digital-electronics-and-hw-integration/`](01b-digital-electronics-and-hw-integration/README.md) | Hardware schematic & resistor/RC math | Level 1 |
| 2 | **Stage 02:** ISO C11 Foundations, Bitwise Math, Pointers & UB | [`02-c-fundamentals/`](02-c-fundamentals/README.md) | `make -C tests test` (`35/35`) + [Stage 01 Exam](24-assessments/stage-01-c-mastery-exam.md) | Level 1 |
| 3 | **Stage 03:** Embedded C, MMIO, `volatile`, Barriers & Atomics | [`03-embedded-c/`](03-embedded-c/README.md) | [Worked Example 1](LEARNERS/WORKED_EXAMPLES.md) + [Stage 02 Exam](24-assessments/stage-02-embedded-c-register-exam.md) | Level 1 |
| 4 | **Stage 04:** ARMv7E-M Cortex-M4F Core, FPU, Faults & MPU | [`04-arm-cortex-m/`](04-arm-cortex-m/README.md) | [Worked Example 6](LEARNERS/WORKED_EXAMPLES.md) | Level 1–2 |
| 5 | **Stage 05:** GNU Toolchain, Linker Scripts, 118-IRQ Startup & GDB | [`05-toolchain-and-debugging/`](05-toolchain-and-debugging/README.md) | Build `23-code-library/templates/` + [Stage 03 Exam](24-assessments/stage-03-arm-cortex-m-architecture-exam.md) | Level 1–2 |
| 6 | **Stage 06:** STM32G474RE BusMatrix, LQFP64 Limits & Errata | [`06-stm32-fundamentals/`](06-stm32-fundamentals/README.md) | Inspect `stm32g474xx.h` & [`REGISTER_QUICK_REFERENCE.md`](LEARNERS/REGISTER_QUICK_REFERENCE.md) | Level 1 |
| 7 | **Stage 07 (06A):** Basic GPIO on 16 MHz HSI16 *(before RCC!)* | [`08-gpio/`](08-gpio/README.md) | [Lab 01](18-practical-laboratory/lab-01-baremetal-gpio-led-blinky/README.md), [Lab 02](18-practical-laboratory/lab-02-gpio-pushbutton-debouncing/README.md) | Level 1 |
| 8 | **Stage 08 (06B):** PWR Boost, Flash 4WS & 170 / 160 MHz RCC PLL | [`07-rcc-and-clocks/`](07-rcc-and-clocks/README.md) | [Lab 03](18-practical-laboratory/lab-03-rcc-pll-170mhz-clock-switch/README.md) | Level 2 |
| 9 | **Stage 09 (07):** SysTick, NVIC 4-Bit Grouping & EXTI *(before Timers!)* | [`10-interrupts/`](10-interrupts/README.md) | [Lab 04](18-practical-laboratory/lab-04-systick-accurate-delay-generator/README.md), [Lab 05](18-practical-laboratory/lab-05-exti-pushbutton-interrupt/README.md) | Level 2 |
| 10 | **Stage 10 (08):** General-Purpose, Advanced Timers (`TIMx`) & `HRTIM1` | [`09-timers/`](09-timers/README.md) | [Lab 06](18-practical-laboratory/lab-06-general-purpose-timer-periodic-interrupt/README.md), [Lab 07](18-practical-laboratory/lab-07-timer-pwm-rgb-led-fading/README.md) | Level 2–3 |
| 11 | **Stage 11 (09):** Serial Buses (`USART`, `SPI`, `I2C`, `FDCAN`, `USB`) | [`11-communication/`](11-communication/README.md) | [Lab 08](18-practical-laboratory/lab-08-usart-baremetal-driver-printf/README.md), [Lab 11](18-practical-laboratory/lab-11-spi-master-sensor-interfacing/README.md), [Lab 12](18-practical-laboratory/lab-12-i2c-master-ssd1306-oled/README.md), [Lab 18](18-practical-laboratory/lab-18-fdcan-loopback-and-crc/README.md) | Level 2–3 |
| 12 | **Stage 12 (10):** Mixed-Signal (`ADC1–5`, `DAC1–4`, `OPAMP`, `COMP`) | [`12-adc-dac-analog/`](12-adc-dac-analog/README.md) | [Lab 09](18-practical-laboratory/lab-09-adc-single-channel-potentiometer/README.md), [Lab 16](18-practical-laboratory/lab-16-dac-opamp-comp-hardware-break/README.md) | Level 2–3 |
| 13 | **Stage 13 (11):** `DMA1/2`, `DMAMUX1`, `CORDIC` & `FMAC` | [`13-dma/`](13-dma/README.md) | [Lab 10](18-practical-laboratory/lab-10-adc-dma-multichannel-circular-buffer/README.md), [Lab 17](18-practical-laboratory/lab-17-cordic-fmac-hardware-math/README.md) + [Stage 04 Exam](24-assessments/stage-04-stm32-peripherals-exam.md) | Level 2–3 |
| 14 | **Stage 14 (12):** Dual-Bank Flash ECC, Bootloaders & Security | [`14-memory-and-storage/`](14-memory-and-storage/README.md) | Linker/Flash RWW exercises | Level 2–3 |
| 15 | **Stage 15 (13):** Watchdogs (`IWDG`/`WWDG`), `RTC` & Low-Power Modes | [`15-power-management/`](15-power-management/README.md) | [Lab 13](18-practical-laboratory/lab-13-iwdg-hardware-watchdog/README.md), [Lab 14](18-practical-laboratory/lab-14-low-power-stop-mode-wakeup/README.md) | Level 2 |
| 16 | **Stage 16 (14):** FreeRTOS `ARM_CM4F` & Real-Time Scheduling | [`16-rtos/`](16-rtos/README.md) | [Lab 15](18-practical-laboratory/lab-15-freertos-multitask-producer-consumer/README.md) *(Statically Reviewed)* | Level 2–3 |
| 17 | **Stage 17 (15):** Layered Architecture, MISRA/CERT C, MPU, TDD & CI | [`17-advanced-firmware-engineering/`](17-advanced-firmware-engineering/README.md) | [Lab 19](18-practical-laboratory/lab-19-hardfault-mpu-and-crash-diagnostics/README.md), [Lab 20](18-practical-laboratory/lab-20-host-unit-testing-and-driver-mocking/README.md) + [Stage 05 Exam](24-assessments/stage-05-rtos-and-advanced-firmware-exam.md) + [Capstones](19-projects/README.md) | Level 2–3 |

---

## 6. Learner Companion Toolkit (`LEARNERS/`)

Keep these 8 practical handbooks bookmarked while studying:
- **[`LEARNERS/BEGINNER_SETUP_WINDOWS11.md`](LEARNERS/BEGINNER_SETUP_WINDOWS11.md)** — Exact Windows 11 installation, `PATH`, ST-LINK COM port, VS Code extensions, and SVD setup.
- **[`LEARNERS/LEARNER_PROGRESS_DASHBOARD.md`](LEARNERS/LEARNER_PROGRESS_DASHBOARD.md)** — Interactive checkbox tracker across Level 1, Level 2, and Level 3.
- **[`LEARNERS/WORKED_EXAMPLES.md`](LEARNERS/WORKED_EXAMPLES.md)** — 6 full step-by-step worked examples (Demo → Explanation → Registers → GDB → Lab → Exercise) for the trickiest STM32G474RE topics.
- **[`LEARNERS/TROUBLESHOOTING_GUIDE.md`](LEARNERS/TROUBLESHOOTING_GUIDE.md)** — Symptom → Root Cause → Exact Fix tables for compiler, linker, clock, register, SWD/GDB, and bus failures.
- **[`LEARNERS/FORMULA_AND_CALCULATION_SHEET.md`](LEARNERS/FORMULA_AND_CALCULATION_SHEET.md)** — Verified math cheat sheet for electronics, 170/160 MHz PLLs, timers, dead-time, UART/I2C, ADC/DAC, watchdogs, bit-banding, and RTOS RMS.
- **[`LEARNERS/REGISTER_QUICK_REFERENCE.md`](LEARNERS/REGISTER_QUICK_REFERENCE.md)** — Compact 1-page-per-subsystem STM32G474RE register desk reference verified against `stm32g474xx.h`.
- **[`LEARNERS/REVISION_NOTES_AND_MASTERY_GATES.md`](LEARNERS/REVISION_NOTES_AND_MASTERY_GATES.md)** — Measurable mastery gates & revision checklists proving real engineering skill.
- **[`LEARNERS/MKDOCS_PORTAL_PLAN.md`](LEARNERS/MKDOCS_PORTAL_PLAN.md)** — Zero-duplication GitHub Pages **MkDocs Material** portal architecture (`docs_dir: .`, MathJax v3, Mermaid, strict CI link gating).
