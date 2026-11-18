# Changelog

All notable technical updates, defect remediations, audit releases, and curriculum expansions are documented in this file.

---

### [v3.1.0] - 2026-11-18 — Phase 9 Independent Red-Team Closure (`12/12` Agent 11 Findings Resolved) & Final Remediation Sign-Off

#### Source-of-Truth Recalibration (`AUDIT/concepts_sot.json` `v2.1.0`) & Final Sign-Off (`[P8-HIGH-04]`)
- **Strict `ONBOARD_HARDWARE_TESTED` De-Inflation (`140 -> 122`):** Downgraded `18` unexercised or bench-unmeasured concepts (`ARM-PWR-001`, `PERIPH-TIM-001`, `STM32-RCC-003`, `TOOL-DBG-003`, `ARM-SEC-001`, `PERIPH-UART-004`, `ARM-MPU-001`, `ARM-MEM-002`, `STM32-BOOT-001`, `MCU-IO-001` -> `CROSS_COMPILED`; `PERIPH-GPIO-004`, `G4-COMP-001` -> `EXTERNAL_HARDWARE_PENDING`; `PERIPH-BDTR-001`, `PERIPH-PWM-001`, `PERIPH-DAC-001`, `HWSW-ANA-001`, `PERIPH-GPIO-002`, `PERIPH-TIM-003` -> `NEEDS_BENCH_INSTRUMENT`), yielding an airtight **`122` `ONBOARD_HARDWARE_TESTED`**, `53` `HOST_TESTED`, `105` `CROSS_COMPILED`, `39` `EXTERNAL_HARDWARE_PENDING`, `16` `NEEDS_BENCH_INSTRUMENT`, `15` `NOT_APPLICABLE`.
- **CI-Only Tooling De-Inflation (`267 -> 265 COMPLETE`, `72 -> 74 PARTIAL`):** Held `PROF-SA-002` (`clang-tidy`/`clang-format`) and `PROF-DOC-001` (`doxygen`) honestly at **`PARTIAL`** until verified green on Ubuntu GitHub Actions CI, and documented `ARM-MPU-001` transparently (`lab-19` compile-verified + Test 8A region arming & `DIV0` `.noinit` persistence verified, while intentional `MemManage` trigger remains pending).
- **Mandatory Final Sign-Off Report:** Created [`AUDIT/FINAL_REMEDIATION_REPORT.md`](AUDIT/FINAL_REMEDIATION_REPORT.md) with complete `12/12` Phase 8 Agent 11 disposition table and deterministically regenerated `COMPLETE_CONCEPT_INVENTORY.md`, `CURRICULUM_COVERAGE_MATRIX.md`, `UPDATED_CONCEPT_COVERAGE_MATRIX.md`, `PARTIAL_CONCEPT_COMPLETION_REPORT.md`, and `UPDATED_KNOWN_LIMITATIONS.md`.

#### Firmware, Driver, Safety, CI & Hygiene Fixes (`[P8-HIGH-01..03]`, `[P8-MED-01..05]`, `[P8-LOW-01..03]`)
- **CI Pipeline Hardening (`[P8-HIGH-01]`):** Updated `.github/workflows/firmware-ci.yml` for LCOV 2.0 (`--rc branch_coverage=1 --ignore-errors inconsistent,unused`), tightened `cppcheck` warning/portability gate, and reconciled `.clang-format` check.
- **Capstone 03 External Power-Stage Safety (`[P8-HIGH-02]`, `[P8-MED-05]`):** Replaced open-loop software plant in `POWER_SIM_MODE=0` with real closed-loop `ADC1_IN1` (`PA0`) voltage-sense feedback + immediate fail-safe `MOE=0`/`CCR1=0` trip on ADC fault, and eliminated implementation-defined signed right-shifts in `power_ctrl.h`.
- **RM0440 / UM10204 Driver & FreeRTOS Fixes (`[P8-HIGH-03]`, `[P8-MED-01..04]`):** Fixed `i2c_g4.h` `I2C_TIMINGR` (`0x70811021`, `0x70710F1F`, `0x30420F13`) + `PB8-BOOT0` `2.705 V` warning, fixed `GPIO_LockPin()` false-positive on 2nd pin + added `GPIO_LockPins()`, fixed edge cases in `tim_g4.c`/`usart_g4.c`/`i2c_g4.c`/`adc_g4.c`/`spi_g4.c`, fixed `TIM2->SR` `rc_w0` write & `RCC_CSR_RMVF` clear in `23-code-library/freertos/`, synchronized `lab-19` status, and expanded host unit tests to **`594/594`** passing assertions!
- **Hygiene (`[P8-LOW-01..03]`):** Added `*.gcda`/`*.gcno`/`*.gcov`/`*.info` to `.gitignore`, guarded post-`main()` return in `startup_stm32g474re.c`, and sanitized local workspace path in `LEARNERS/BEGINNER_SETUP_WINDOWS11.md`.

---

### [v3.0.0] - 2026-11-18 — Complete Remediation, 12 Drivers, Pinned FreeRTOS V11.1.0, 3 Capstones & Physical NUCLEO-G474RE Onboard Hardware Verification

#### Physical NUCLEO-G474RE Onboard Hardware Validation (`MB1367 Rev C`, `COM8`, `VDD = 3.28–3.29 V`)
- **Onboard Silicon Self-Test Suite (`HARDWARE_VALIDATION/onboard_selftest/` — `9/9 PASS`):** Verified `DBGMCU_IDCODE = 0x20036469` (`Rev X`, `512 KB` Flash), `PA5`/`PC13` GPIO, **BOTH `160 MHz` (`1,599,997` cyc/10 ms) & `170 MHz` (`1,699,989` cyc/10 ms) Range 1 Boost** (`PWR_CR5 = 0`, `4 WS`), `TIM2` & `EXTI13`, `DMA1` MEM2MEM & `ADC1` circular DMA, internal `VDDA = 3288 mV` & `Die Temp = 32.91 °C`, `CORDIC` (`9.72×` faster than FPU, `2e-6` max err), `FMAC` (`0 LSB` diff, `2.32×` speedup), `CRC-32 = 0xCBF43926` + `FDCAN1` 16 B BRS loopback (`ECR=0`), `MPU` + `.noinit` `DIV0` `UsageFault` reboot persistence (`CFSR = 0x02000000`), and `IWDG` (`6/6` kicks).
- **Authentic Real-Silicon Defects Caught & Fixed:** Physical bring-up caught missing `GPIO_EnableClock()` calls across Capstone Projects `01–03` (initially silencing `PA2` `USART2_TX` with `0` warnings and `CFSR = 0`) and intra-object GNU `ld --wrap` resolution on `HardFault_Handler_C`; both were fixed, rebuilt (`0` warnings), and verified live on `COM8` ([`AUDIT/HARDWARE_VALIDATION_SUMMARY.md`](AUDIT/HARDWARE_VALIDATION_SUMMARY.md)).
- **Safe Restoration & Deferred Schedule:** Board restored cleanly to 170 MHz template (`3.28 V`, zero option-byte changes); all external breadboard peripherals and `5` bench-instrument `NEEDS_VERIFICATION` measurements documented in [`HARDWARE_VALIDATION/PENDING_EXTERNAL_CONNECTIONS.md`](HARDWARE_VALIDATION/PENDING_EXTERNAL_CONNECTIONS.md).

#### Concept Inventory (`AUDIT/concepts_sot.json` `v2.0.0`) & All 12 `MISSING` Concepts Resolved
- **Zero `MISSING` Concepts Remaining:** All **12** previously `MISSING` concepts (`C-LIB-001`, `FW-DRV-003`, `GPIO-FMP-001`, `TIM-DMA-001`, `TOOL-LD-004`, `TOOL-DBG-004`, `HWSW-ANA-001`, `COMM-LP-001`, `COMM-CAN-002`, `COMM-USB-002`, `MEM-OB-001`, `SEC-BOOT-002`) now have deep multi-section theory, verified registers, compile-verified C11 code (`arm-none-eabi-gcc 14.3.1 -Werror`), and assessments (`7` promoted to `COMPLETE`, `5` honestly `PARTIAL` pending external hardware or restricted Flash/option-byte writes).
- **Zero-Inflation Individual Re-Evaluation (`350` Concepts, `27/27` Domains, `0` Cycles, `0` Inverted Edges):** Recalibrated honestly from `190 COMPLETE / 137 PARTIAL / 12 MISSING / 5 NEEDS_VERIF / 6 N/A` to **`267 COMPLETE (77.6% of 344 applicable) / 72 PARTIAL (20.9%) / 0 MISSING (0.0%) / 5 NEEDS_VERIFICATION (1.5%) / 6 NOT_APPLICABLE`**, with explicit per-concept `hardware_status` (`140 ONBOARD_HARDWARE_TESTED`, `53 HOST_TESTED`, `95 CROSS_COMPILED`, `37 EXTERNAL_HARDWARE_PENDING`, `10 NEEDS_BENCH_INSTRUMENT`, `15 NOT_APPLICABLE`).
- **180 `_Static_assert` Register-Map Audit & Dual Clock Profiles:** Re-audited all 16 STM32G474RE register maps against `stm32g474xx.h` / `core_cm4.h` and documented both `170 MHz` and `160 MHz` Boost profiles across stages, labs, drivers, and Capstones.

#### 12-Driver Library, Official Pinned `FreeRTOS V11.1.0` & 3 Compiled Capstones
- **Expanded Driver Library (`23-code-library/Drivers/`):** Added `exti_g4`, `tim_g4`, `usart_g4`, `spi_g4`, `i2c_g4`, `adc_g4`, `dma_g4`, and `iwdg_g4` alongside enhanced `clock_g4` (`160`/`170 MHz`), `gpio_g4`, `ring_buffer`, and `fault_handler` (**12/12** zero warnings debug/release & no-GC link; **10/12** + `i2c_g4` `NACKF` verified live on silicon).
- **Official Pinned FreeRTOS (`23-code-library/freertos/`):** Vendored upstream `FreeRTOS-Kernel V11.1.0` (`SHA dbf70559b27d...`, `GCC/ARM_CM4F`, `heap_4.c`), verified `SVC`/`PendSV`/`SysTick` bindings via `nm`/`objdump`, and passed **`6/6` live onboard self-tests** + 8 s telemetry on NUCLEO-G474RE (`fpu_err=0`, `heap=21104/21104`).
- **3 Real Compiled & Onboard-Tested Capstones (`19-projects/01..03/`):** Implemented full multi-file firmware (`Inc/`, `Src/`, `Makefile`, `0` warnings) for Smart Instrument Panel (`01`), High-Speed DMA Data Logger (`02`), and FreeRTOS Digital Power Controller (`03`), all **3/3 verified live on `COM8`**.

#### Automated Host Testing (`574` Assertions), `95.97%` `gcov`, CI, 70 Live Videos & Beginner Onboarding
- **Testing & Quality (`tests/`, `Doxyfile`, `.clang-tidy`, `.github/workflows/firmware-ci.yml`):** Expanded host tests from `113` to **`574` passing assertions** across 4 suites (`10` real drivers), measured **`95.97%` `gcov -b` line coverage** (`100%` of reachable lines; `9/10` drivers at `100%`) and **`98.06%`** branch execution, verified **`0` findings** under both host & ARM `-fanalyzer`, and upgraded to a 5-job CI pipeline.
- **Educational Videos (`22-video-library/`):** Expanded from `50` to **`70/70` live HTTP 200 verified videos** (`playabilityStatus: OK`, `0` duplicates, **`93.5%`** / `29/31` curriculum parity).
- **Learner Experience & Backup (`START_HERE.md`, `LEARNERS/`, `BACKUP/`):** Created `START_HERE.md`, 7 beginner handbooks (`BEGINNER_SETUP_WINDOWS11`, `LEARNER_PROGRESS_DASHBOARD`, `WORKED_EXAMPLES`, `TROUBLESHOOTING_GUIDE`, `FORMULA_AND_CALCULATION_SHEET`, `REGISTER_QUICK_REFERENCE`, `REVISION_NOTES_AND_MASTERY_GATES`), and immutable baseline backup (`565` files, `0` SHA-256 mismatches).

---

### [v2.1.0] - 2026-11-18 — Independent Red-Team Remediation, Honest Status Recalibration & Final Sign-Off

#### Independent Red-Team Resolution (All 17 / 17 Agent 10 Findings Closed)
- **Source-of-Truth Recalibration & Expansion ([`AUDIT/concepts_sot.json`](AUDIT/concepts_sot.json)):**
  - Expanded from `338` to **350 deduplicated concepts** across **27/27 domains** by adding all **12 Agent 10 independently discovered Phase 5 concepts** (`C-LIB-001`, `GPIO-FMP-001`, `TIM-DMA-001`, `COMM-LP-001`, `COMM-CAN-002`, `SEC-BOOT-002`, `TOOL-LD-004`, `COMM-USB-002`, `TOOL-DBG-004`, `MEM-OB-001`, `FW-DRV-003`, `HWSW-ANA-001`) honestly marked `MISSING` (`[HIGH-06]`).
  - Eliminated status inflation (`[HIGH-05]`) by downgrading **81** over-claimed concepts (14 uncompiled FreeRTOS concepts, 21 untested C concepts, unconfigured tools `Doxyfile`/`.clang-tidy`/`cppcheck`, hardware debug/bench items, and mismatched `gpio_g4.c`/`test_c_foundations.c` pointers) to **`PARTIAL`**, establishing an honest, defensible strict completion rate of **`190 / 344` (`55.2%`) `COMPLETE`**, **`137` `PARTIAL`**, **`12` `MISSING`**, **`5` `NEEDS_VERIFICATION`**, and **`6` `NOT_APPLICABLE`**.
  - Eliminated all **23 inverted difficulty-level DAG edges** and bogus synthetic prerequisites (`[HIGH-04]`), verified programmatically at **`0` cycles, `0` unknown prereq IDs, and `0` inverted-level edges**.
  - Stripped `100%` of raw Markdown formatting artifacts (`[LOW-03]`) from `concepts_sot.json` and regenerated all dependent audit tables cleanly.
- **Final Audit Sign-Off & Cross-Audit Reconciliation (`[MED-06]`):**
  - Created mandatory 10th audit report [`AUDIT/FINAL_COMPLETENESS_REPORT.md`](AUDIT/FINAL_COMPLETENESS_REPORT.md) with deterministic 17-metric scorecard, 7-axis coverage summary, and 17/17 Red-Team findings disposition table.
  - Reconciled stage numbering (`Stage 01b` + `Stages 02–16`) across `AUDIT/PREREQUISITE_DEPENDENCY_AUDIT.md` and `README.md`, updated `AUDIT/TECHNICAL_VERIFICATION_REPORT.md` with all 17 Red-Team resolutions, and removed the stale `.mmd` fence limitation from `AUDIT/KNOWN_LIMITATIONS.md`.
- **Critical Firmware, Register-Map & Lab Fixes (`CRIT-01..02`, `HIGH-01..03`, `MED-01..05`, `LOW-01..02`):**
  - Resolved unified `MemManage`/`BusFault`/`UsageFault` trampolines + SRAM bounds guard + debugger-guarded `bkpt #0` in `fault_handler.c`/`startup_stm32g474re.c`, fixed `OPAMP_CSR_*` & `COMP_CSR_BLANKING[21:19]` in `opamp-comp-vrefbuf-register-map.md`, fixed `TIM1_AF1_BKCMP1E` in `09-timers/README.md`, fixed `SCnSCB->ACTLR` in `lab-19`, fixed `lab-12` `TIMINGR` (`t_SCLDEL >= 400 ns`) + default `PA15`/`PB7` wiring, fixed `clock_g4.c` `PLLPEN`/`PLLQEN` + DWT guard, migrated `ring_buffer.c` to C11 `_Atomic`, fixed `05-toolchain-and-debugging/README.md` prototypes, fixed RM0440 Ch. 28/37 citations, fixed `RNG` `rc_w0`/recovery & 5 RCC readbacks, fixed MISRA Rule 11.4 (`Advisory`), and added `NACKF` handling in `spi-i2c-register-map.md`.

---

### [v2.0.0] - 2026-11-18 — Part 4 Full Audit, Technical Remediation & Curriculum Expansion

#### Canonical Audit & Governance Suite (`AUDIT/`)
- **Source-of-Truth Concept Inventory ([`AUDIT/concepts_sot.json`](AUDIT/concepts_sot.json)):** Programmatically synthesized **338 deduplicated concepts** across **all 27 required embedded domains** (`271 COMPLETE`, `56 PARTIAL`, `5 NEEDS_VERIFICATION`, `0 MISSING`, `6 NOT_APPLICABLE`), verified 100% acyclic DAG and 100% existing repository target paths.
- **Full Audit Suite Generated:** Created [`COMPLETE_CONCEPT_INVENTORY.md`](AUDIT/COMPLETE_CONCEPT_INVENTORY.md), [`CURRICULUM_COVERAGE_MATRIX.md`](AUDIT/CURRICULUM_COVERAGE_MATRIX.md) (7-axis honest scorecard), [`MISSING_CONCEPTS_AND_GAPS.md`](AUDIT/MISSING_CONCEPTS_AND_GAPS.md), [`STM32G474RE_PERIPHERAL_INVENTORY.md`](AUDIT/STM32G474RE_PERIPHERAL_INVENTORY.md), [`PREREQUISITE_DEPENDENCY_AUDIT.md`](AUDIT/PREREQUISITE_DEPENDENCY_AUDIT.md), [`PRACTICAL_IMPLEMENTATION_COVERAGE.md`](AUDIT/PRACTICAL_IMPLEMENTATION_COVERAGE.md), [`TECHNICAL_VERIFICATION_REPORT.md`](AUDIT/TECHNICAL_VERIFICATION_REPORT.md), and [`KNOWN_LIMITATIONS.md`](AUDIT/KNOWN_LIMITATIONS.md).
- **Replaced False-Green Governance:** Rewrote `00-project-management/quality-audit.md` and `requirements-tracker.md` with exact measured coverage and explicit hardware-unverified disclosures.

#### Critical Hardware, Silicon & Template Bug Fixes (18 Verified Defect Classes Resolved)
- **Complete 118-Entry Vector Table (`startup_stm32g474re.c`):** Fixed truncated 53-IRQ table, misaligned `LPUART1`/`TIM6_DAC`/`TIM7_DAC`/`DMA1_Ch7`/`ADC3` slots, typo `RTC_TAMP_LSECSSD`, and 59 `-Wpedantic` casts. Verified `.isr_vector` = **472 bytes, 512-byte aligned** via `arm-none-eabi-readelf`.
- **Linker Script & Build System (`stm32g474re.ld`, `Makefile`, `CMakeLists.txt`):** Fixed `CCMRAM (xrw)` + `_siccmram` copy loop, added `.noinit (NOLOAD)` and linker `ASSERT()`s, removed broken `/DISCARD/` on `libc/libm/libgcc`, removed hardcoded local Windows OpenOCD paths, deduplicated CMSIS headers, and added `BUILD_TYPE=debug|release` + `-MMD -MP` + `-fstack-usage`.
- **STM32G474RE Hardware Accuracy:** Fixed `PWR_CR5 @ 0x80` and `PWR_SR2.VOSF`, `RCC_CR.HSION` bit 8 / `HSIRDY` bit 10 / `SW` encodings, RM0440 §6.1.5 170 MHz `HPRE=/2` ramp, ADC `CKMODE=11` (`42.5 MHz <= 60 MHz` vs previous 170 MHz overclock), `ADC_CR` reset `0x20000000`, temp sensor $t_S \ge 5\ \mu\text{s}$, DMAMUX swapped USART RX/TX IDs (`24=U1_RX`, `25=U1_TX`), `GPIOx_MODER` Analog reset `11b`, Active-HIGH `PC13` B1 polarity (`UM2505`), **`JP6` (`IDD`)** vs `JP5` safety fix in Lab 14, IWDG `PR=100b` & `DBGMCU` freeze, `Lab 07` unsigned underflow infinite loop, `Lab 11` 2-byte SPI read & `FRXTH=1`, `Lab 12` `TIMINGR` + `NACKF` timeout + `PB8-BOOT0` trap, `Lab 15` 512-word stack + `NVIC_SetPriorityGrouping(0)`, and `Project 03` hardware `COMP1 -> TIM1_BKIN` safety break.
- **Documentation & Hygiene:** Eliminated all 53 raw ASCII control characters corrupting LaTeX formulas, removed stale `scratch/` directory, corrected RM0440 chapter numbers / DS12288 Table 13–14 / RM0399 / glossary `RDR`/`TDR`/`RXDR`/`TXDR`, and added `ES0430` + 12 ST Application Notes.

#### Major Curriculum, Lab, Driver & Testing Additions
- **17 Full Curriculum Stages:** Added new Stage 00 (`01b-digital-electronics-and-hw-integration/`, 43 KB) and expanded all 16 stages (`02`–`17`) from 27–48 line stubs into 18–35 KB engineering modules + `02-c-fundamentals/advanced-c-ub-memory-model.md`.
- **20 Laboratories & 4 Reusable Drivers:** Expanded from 15 to **20 laboratories** (adding `Labs 16–20`: DAC/OPAMP/COMP hardware break, CORDIC/FMAC math, FDCAN1+CRC-32, MPU/HardFault diagnostics, Host TDD), added 3-way Register/LL/HAL comparisons (`Labs 01, 05, 08`), and added compile-verified `clock_g4`, `gpio_g4`, `ring_buffer`, and `fault_handler` drivers.
- **16 Peripheral Register Maps & 500-Point Assessment Keys:** Added dedicated register maps for `HRTIM1`, `FDCAN1–3`, `CORDIC/FMAC/CRC/RNG`, `OPAMP/COMP/VREFBUF`, and `RTC/TAMP/USB`, plus full worked solutions in `24-assessments/answer-keys-and-rubrics.md`.
- **Automated Verification & Host Tests:** Added `tests/test_c_foundations.c` (35/35 assertions), `tests/unit/test_ring_buffer.c` (78/78 assertions), `.clang-format`, `.gdbinit`, `.vscode/` configs, `.github/workflows/firmware-ci.yml`, and upgraded `scripts/validate_repo.py`.

---

### [v1.0.0] - Initial Baseline Release (Parts 1–3)
- Initial 16-stage syllabus outline, 15 draft laboratories, 3 capstone outlines, 50 live-verified YouTube video entries, initial register tables, and starter build template (superseded and extensively remediated in `v2.0.0`).
