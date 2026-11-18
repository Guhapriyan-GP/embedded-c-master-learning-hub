# Changelog

All notable technical updates, defect remediations, audit releases, and curriculum expansions are documented in this file.

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
