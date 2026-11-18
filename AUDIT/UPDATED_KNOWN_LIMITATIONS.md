# Updated Known Limitations & Honest Remaining Scope (Phase 9 Red-Team Signed-Off — `v3.1.0`)

> **Engineering Honesty Policy:** This document supersedes the Part 4 `KNOWN_LIMITATIONS.md` snapshot (`v2.1.0`, when `0%` of firmware had run on silicon, FreeRTOS was unvendored, capstones had `0` `.c` files, and `12` concepts were `MISSING`) and incorporates **Phase 9 Agent 11 `[P8-HIGH-04]` recalibration** (`AUDIT/concepts_sot.json` `v2.1.0`). Every remaining limitation below is transparently preserved so learners and reviewers know the exact boundary between what IS physically verified on NUCLEO-G474RE silicon (`122` concepts `ONBOARD_HARDWARE_TESTED`) and what remains pending external hardware (`39`), bench instruments (`16`), restricted Flash/option-byte writes, or Ubuntu CI execution.

---

## 1. Deferred External Hardware & Breadboard Peripherals (`39` Concepts `EXTERNAL_HARDWARE_PENDING`)

While **Agent 4 physically verified `9/9` onboard silicon tests, `6/6` FreeRTOS `V11.1.0` self-tests, `10/12` drivers (+ `i2c_g4` `NACKF`), and `3/3` capstones in zero-wire onboard modes** on real **NUCLEO-G474RE (`MB1367 Rev C`, `COM8`, `VDD = 3.28 V`)**, all experiments requiring external components, manual button presses, analog stimuli, or jumper wires were intentionally deferred and documented with full wiring and electrical safety rules in [`HARDWARE_VALIDATION/PENDING_EXTERNAL_CONNECTIONS.md`](../HARDWARE_VALIDATION/PENDING_EXTERNAL_CONNECTIONS.md):

1. **6 / 19 STM32 Target Labs Pending External Hardware:**
   - **Lab 07** (`TIM3` 25 kHz RGB PWM): external common-cathode RGB LED + 3× `330 Ω` resistors (`PA6`/`PA7`/`PB0`).
   - **Lab 09** (`ADC1` Potentiometer): external `10 kΩ` potentiometer on `PA0`.
   - **Lab 11** (`SPI1` Master + `spi_g4.c`): `PB4 <-> PB5` loopback jumper or external `3.3 V` SPI sensor.
   - **Lab 12** (`I2C1` SSD1306 OLED + `i2c_g4.c` ACK/9-clock recovery): external `3.3 V` `0x3C` OLED + `4.7 kΩ` pull-ups on `PA15`/`PB7`.
   - **Lab 14** (`Stop 0` Low-Power & `JP6` `IDD`): manual `B1` (`PC13`) wake + microamp DMM/PPK2 across `JP6` (never `JP5`!).
   - **Lab 16** (`DAC1` + `OPAMP1` + `COMP1` -> `TIM1_BKIN`): external `PA1` DC stimulus + `>= 100 MHz` oscilloscope on `PA8`/`PA4`.
2. **Unexercised Interactive / Analog-Stimulus Features (Recalibrated in Phase 9 `[P8-HIGH-04]`):**
   - **`PERIPH-GPIO-004` (Switch Debouncing):** Software 4-state debounce FSM is host-tested in `test_c_foundations.c`, but `B1` (`PC13`) was only idle-read during unattended automation (`B1 Presses: 0` in Project 01, `btn=0` in FreeRTOS — `0` physical presses).
   - **`G4-COMP-001` (`COMP1–7` Analog Comparator Break):** `COMP1` was armed in Project 03 (`brk:0`), but no physical overcurrent voltage stimulus on `PA1` or `TIM1_BKIN` hardware trip capture was executed.
3. **Optional Capstone External Hardware (`19-projects/`):**
   - Project 01 external potentiometer (`PA0`) & PWM load/scope (`PA1`).
   - Project 02 & 03 external `I2C1` SSD1306 OLED (`PA15`/`PB7`, `0x3C`).
   - Project 03 external isolated buck/boost power stage (`POWER_SIM_MODE=0`, now wired to real `ADC1_IN1` `PA0` voltage-sense feedback with fail-safe `MOE=0` guard `[P8-HIGH-02]`) + oscilloscope verification of `500 ns` dead-time on `PA8`/`PB13`.
4. **Drivers Pending External Loopback / Slave (`2 / 12` Drivers):**
   - `spi_g4.c` (`EXTERNAL_HARDWARE_PENDING`) and `i2c_g4.c` external slave ACK / `i2c_recover_bus_9clks()` (`NACKF` absent-slave path already verified live on silicon).
5. **Deferred Complex External-Bus / Motor Stacks (`PARTIAL`):**
   - `PERIPH-USB-001`, `COMM-USB-002`, `CLK-CRS-001` (bare-metal USB FS HID/CDC enumeration + external `PA11`/`PA12` connector),
   - `G4-UCPD-001` (USB-C PD BMC/4b5b stack),
   - `COMM-CAN-002` (5 Mbps TDC & Bus-Off with external CAN-FD transceiver),
   - `PERIPH-SAI-001` (external I2S audio codec),
   - `PERIPH-QSPI-001` (external QSPI NOR Flash XIP `0x90000000`),
   - `PERIPH-ENC-001`, `PERIPH-IC-001..002`, `PERIPH-OC-001`, `PERIPH-SPI-002..003`, `PERIPH-SIG-001`, `APP-FOC-001` (3-phase motor inverter),
   - `RTOS-PWR-001` & `PERIPH-LPTIM-001` (`LPTIM1` Stop 1 Tickless Idle) & `RTOS-IPC-002` (FreeRTOS Stream/Message Buffers self-test).

---

## 2. Bench-Instrument Measurements (`16` Concepts `NEEDS_BENCH_INSTRUMENT`, incl. `5` `NEEDS_VERIFICATION`)

### 2.1 Five Core `NEEDS_VERIFICATION` Measurements (`5` Concepts)
These **5 concepts** remain honestly **`NEEDS_VERIFICATION`** (zero false promotion) until measured with bench test equipment per [`HARDWARE_VALIDATION/PENDING_EXTERNAL_CONNECTIONS.md`](../HARDWARE_VALIDATION/PENDING_EXTERNAL_CONNECTIONS.md) §3:

1. **`DIG-ELEC-006`** — `>= 200 MHz` oscilloscope measurement of `OSPEEDR` slew-rate ringing & `33 Ω` series source damping on `PA8` (`MCO`).
2. **`G4-HRTIM-001`** — `>= 500 MHz` oscilloscope verification of `HRTIM1` `184 ps` DLL calibration & fine `CMP1xR` edge stepping.
3. **`ANA-VREF-001`** — `>= 5½-digit` DMM accuracy verification of internal `VREFBUF` (`2.048`/`2.500`/`2.900 V`) **after** safely opening the UM2505 `VDD-VREF+` solder bridge.
4. **`RT-PWR-002`** — Precision microamp DMM / PPK2 leakage delta across **`JP6`** comparing floating digital inputs (`MODER=00b`) vs Analog-parked pins (`MODER=11b`).
5. **`HWSW-PWR-001`** — Full NUCLEO-G474RE **`JP6` (`IDD`)** current profile across Run (`170`/`160 MHz`), Sleep, Stop 0, and Stop 1 vs DS12288 Tables 27–33.

### 2.2 Six Phase 9 Recalibrated Waveform & Rail Measurements (`COMPLETE`, `NEEDS_BENCH_INSTRUMENT`)
Following Agent 11 `[P8-HIGH-04]`, these **6 concepts** were downgraded from `ONBOARD_HARDWARE_TESTED` to **`NEEDS_BENCH_INSTRUMENT`** because registers were configured in firmware (or factory `VREFINT` read), while physical pin waveforms or rail ripple were not captured on an oscilloscope/DMM:
- **`PERIPH-PWM-001`**, **`PERIPH-BDTR-001`**, **`PERIPH-TIM-003`** (PWM duty, `500 ns` dead-time, and complementary outputs on `PA1`/`PA8`/`PB13`),
- **`PERIPH-DAC-001`** (`DAC1` `PA4` analog output voltage/waveform),
- **`PERIPH-GPIO-002`** (`OSPEEDR` edge slew-rate / EMI ringing),
- **`HWSW-ANA-001`** (analog `VDDA`/`VREF+` rail ripple, MLCC ESR/resonance, and ground return paths beyond factory `VREFINT` `3288 mV` check).

*(Plus `5` bench-SOP `PARTIAL` concepts: `HWSW-LAB-001..003`, `PROF-PWR-002`, `REL-BOR-001` -> `5 + 6 + 5 = 16` total `NEEDS_BENCH_INSTRUMENT`.)*

---

## 3. Unexercised On-Chip Features & Hardware-Safety-Restricted Operations

1. **On-Chip Features Not Executed in the 6 Flashed Binaries (Recalibrated to `CROSS_COMPILED` in `[P8-HIGH-04]`):**
   - `ARM-PWR-001` (`WFI`/`WFE`/`SLEEPDEEP`), `PERIPH-TIM-001` (`TIM6`/`TIM7`), `STM32-RCC-003` (`HSE`/`CSS`/`LSE`/`MCO`), `TOOL-DBG-003` (`SWO`/`ITM` wire trace — only `DWT->CYCCNT` ran), `ARM-SEC-001` (`nPRIV`/security transitions), `PERIPH-UART-004` (`ORE`/`FE`/`NE`/`PE` hardware error injection), `ARM-MEM-002` (bit-banding absent on STM32G474RE), `STM32-BOOT-001` (boot-mode switching), `MCU-IO-001` (pad ESD/Schmitt thresholds), and **`ARM-MPU-001`** (`onboard_selftest` Test 8A armed MPU Region 7 and verified benign read + `DIV0` `UsageFault` `.noinit` persistence, but did not trigger an intentional `MemManage` `DACCVIOL|MMARVALID` violation on silicon).
2. **Prohibited Non-Destructive Flash, Option-Byte & Security Writes:**
   - Per `PROJECT_MANAGEMENT/REMEDIATION_TRACKER.md` §3, **zero option-byte writes (`FLASH->OPTR`), RDP/WRP/PCROP/Securable Memory mutations, or live Bank 1/2 Flash erases** were executed on the user's NUCLEO-G474RE:
   - **`MEM-OB-001`, `SEC-BOOT-001..002`, `SEC-MEM-001`, `SEC-RDP-001`, `SEC-CRYPTO-001`, `SEC-TAMP-001`, `STM32-FLASH-002`, `BOOT-FLASH/JUMP/VERIF/UPD-001`, `APP-OTA-001`, `MEM-ECC-001`** remain honestly **`PARTIAL`** (deep theory, verified registers, compile-verified C11 code, and read-only `FLASH_OPTR.DBANK=1` check completed; live destructive/irreversible operations require explicit user approval).

---

## 4. Local Windows 11 Workstation vs. GitHub Actions CI Tool Split

As documented transparently in [`AUDIT/TEST_COVERAGE_REPORT.md`](TEST_COVERAGE_REPORT.md) §6 and recalibrated under `[P8-HIGH-04]`:
- **Executed Locally on Windows 11 (`PASS`):** `gcc 15.2.0` (**`594/594`** host assertions across 4 suites / 10 drivers), `gcov 15.2.0 -b` (`95.97%` aggregate lines, `100%` reachable lines), host `gcc -fanalyzer` (`0` warnings), `arm-none-eabi-gcc 14.3.1` + `-fanalyzer` (`0` warnings across all 12 drivers, template, FreeRTOS `V11.1.0`, `onboard_selftest`, and all 3 capstones), and `python scripts/validate_repo.py`.
- **Configured in Ubuntu GitHub Actions CI ONLY (Not Installed Locally on Windows PATH — Held Honestly at `PARTIAL`):** `cppcheck` (`PROF-SA-001`), `clang-tidy` & `clang-format` (`PROF-SA-002`), `doxygen` (`PROF-DOC-001`), and `lcov 2.0` HTML reporting (CI workflow hardened in Phase 9 `[P8-HIGH-01]`; will be confirmed green on first GitHub Actions push).
- **Not Yet Automated in CI:** Linux ASan/UBSan (`PROF-DA-002`), CMake+Ninja CI execution (`TOOL-BUILD-002`), automated `pytest`+OpenOCD HIL runner (`TEST-HIL-001`), and headless Renode emulation script (`TEST-HIL-002`).
- **Interactive Debug & User-Downloaded SVD (`TOOL-DBG-001`, `TOOL-GDB-001..002`, `TOOL-PROBE-002`, `EMB-CMSIS-003`):** `.vscode/launch.json`, `.gdbinit`, and Windows 11 setup guides are ready; `STM32G474.svd` is user-downloaded due to ST redistribution licensing.

---

## 5. Educational Video Library Boundary (`2 / 31` Written-Only Domains)

Out of **31 core curriculum domains**, **29 / 31 (`93.5%`)** have live HTTP 200 verified videos across the **70-video library** (`AUDIT/VIDEO_COVERAGE_REPORT.md`). **2 domains intentionally remain authoritative written-documentation-only** (zero low-authority video padding):
1. **ARM Cortex-M4F FPU (`CPACR`/Lazy Stacking), Memory Barriers (`DMB`/`DSB`/`ISB`) & `LDREX`/`STREX`** — covered via ARM AN298, ARM AN321, ST PM0214 §4.6, and Stages 03–04.
2. **STM32G474RE Analog Front-End (`OPAMP1–6` PGA, `COMP1–7`, `VREFBUF`)** — covered via RM0440 Ch. 23–25, ST AN5306, ST AN5310, and `opamp-comp-vrefbuf-register-map.md`.

---

## 6. Minor Non-Fatal Toolchain & Backlog Notes

- **GNU `ld` Linker Info Notes:** `--specs=nosys.specs` emits expected informational notes for unreferenced syscalls (`_close`/`_lseek`/`_read`/`_write`) and `LOAD segment with RWX permissions` due to executable `CCMRAM (xrw)` — **0 compiler warnings** under `-Werror`.
- **Remaining `PARTIAL` Concepts (`74` Total Repo-Wide in `v2.1.0`):** `3` prose-only C concepts (`C-SEQ-001`, `C-SEQ-002`, `C-TYPE-003`), CI-only tooling (`PROF-SA-001`, `PROF-SA-002`, `PROF-DOC-001`), `PERIPH-RTC-001`, `RT-WDG-002`, `PERIPH-RNG-001`, `PERIPH-EXTI-003`, `PERIPH-ADC-004`, `PERIPH-SENS-001`, deferred external-bus/Flash/security/HIL concepts, and process practices (`PROF-GIT/DOC/REV/STD/STK`) are honestly retained as **`PARTIAL`** until dedicated executable tests or CI runs complete.
