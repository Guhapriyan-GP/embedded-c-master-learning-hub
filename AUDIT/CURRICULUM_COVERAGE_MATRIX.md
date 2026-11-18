# Curriculum Coverage Matrix (Baseline vs Post-Part-4 & Red-Team Recalibrated)

> **Integrity Standard:** All counts below are computed programmatically from [`AUDIT/concepts_sot.json`](concepts_sot.json) following the independent adversarial audit ([`AUDIT/INDEPENDENT_RED_TEAM_REPORT.md`](INDEPENDENT_RED_TEAM_REPORT.md)). Following `[HIGH-05]` status de-inflation, `COMPLETE` is awarded **strictly** to concepts backed by deep multi-section theory + ground-truth references + verified register maps/diagrams + dedicated compile-verified or host-executed code/tests (`23-code-library/`, `tests/`, or compile-verified `Labs 01–14, 16–20`). Uncompiled FreeRTOS (`Lab 15`), unconfigured tools (`Doxyfile`, `.clang-tidy`, `cppcheck` CI), prose-only concepts, and physical bench measurements are classified honestly as `PARTIAL`, `MISSING`, or `NEEDS_VERIFICATION`.

## 1. Seven-Axis Honest Completeness Scorecard

| # | Verification Dimension | Current Measured Coverage | Exact Quantitative Evidence | Honest Caveats / Remaining Gaps |
|---:|---|---|---|---|
| **1** | **Concept Identification** | **100.0% (27/27 Domains)** | `350` deduplicated Concept IDs (`344` applicable + `6` explicit `NOT_APPLICABLE`) incorporating Agents 01–09 (`338`) + Agent 10 Red-Team additions (`+12`) | Covers ISO C11, ARMv7E-M Cortex-M4F, STM32G474RET6 LQFP64 silicon/package ground truth, and 12 newly tracked advanced topics (`MISSING`) |
| **2** | **Theory & Register Documentation** | **96.5% Applicable Theory Covered (`332/344`)** | 17 full stage modules (`01b` + `02`–`17`, 18–43 KB each) + 16 STM32G474RE register maps + 12 Mermaid diagrams + 0 control-char corruptions | 12 newly discovered Agent 10 topics (`C-LIB-001`, `GPIO-FMP-001`, `TIM-DMA-001`, `COMM-LP-001`, `COMM-CAN-002`, `SEC-BOOT-002`, `TOOL-LD-004`, `COMM-USB-002`, `TOOL-DBG-004`, `MEM-OB-001`, `FW-DRV-003`, `HWSW-ANA-001`) queued for Phase 5 |
| **3** | **Official References & Citations** | **100.0% Ground-Truth Verified** | RM0440 chapters reconciled (incl. Red-Team Ch. 28 TIM1 & Ch. 37 RTC fixes), DS12288 Table 13/14 fixed, UM2505 MB1367 verified, ES0430 + 12 ST ANs added, ARM DDI 0403E.e / PM0214 / MISRA C:2023 (Rule 11.4 Advisory) verified | Exact Cortex-M4 TRM revision suffix (`DDI 0439B`/`D` for r0p1) explicitly noted |
| **4** | **Practical Exercises & Strict Code Verification** | **`55.2%` Strict `COMPLETE` (`190/344`)** / `39.8%` `PARTIAL` (`137`) / `3.5%` `MISSING` (`12`) | 20 labs (`Labs 01–14, 16–19` compile-verified `arm-none-eabi-gcc 14.3.1`; `Lab 15` statically reviewed; `Lab 20` host execution-verified), 4 reusable drivers (post-Red-Team remediated), 113/113 host assertions passing | Downgraded 81 over-claimed concepts (14 uncompiled FreeRTOS, 21 untested C concepts, unconfigured `clang-tidy`/`cppcheck`/CMake/hardware debug sessions, prose-only practices) from `COMPLETE` -> `PARTIAL` to eliminate status inflation |
| **5** | **Physical Hardware Bench Verification** | **0.0% Executed on Physical Board** (`5` items flagged `NEEDS_VERIFICATION`) | **Not Tested on Physical NUCLEO-G474RE Hardware.** All firmware verification is compile-verified (`arm-none-eabi-gcc 14.3.1`), ELF-inspected (`readelf`), and host-tested (`gcc 15.2.0`) | Requires physical NUCLEO-G474RE (`MB1367`), oscilloscope, logic analyzer, and ammeter/PPK2 (`JP6`) to validate real µA currents, `VREFBUF` noise floor, `HRTIM1` 184 ps jitter, and `OSPEEDR` ringing |
| **6** | **Educational Video Resources** | **100.0% URL Validity (50/50 Live); ~70% Topic Parity** | 50/50 YouTube URLs verified live HTTP 200 (oEmbed + metadata); 50/50 mapped in `topic-coverage-matrix.md` | 24% ESP32 RTOS (with Video #50 `NOT_APPLICABLE`), 12% HAL-based STM32G4, and **15 core bare-metal/G4 topics still have 0 bare-metal videos** (compensated via written docs/register maps) |
| **7** | **Automated Validation & CI Gates** | **Active (`validate_repo.py` + GitHub Actions CI)** | `scripts/validate_repo.py` validates structure, 0 control chars, internal links, 12 `.mmd` diagrams, 350-concept SoT schema + 0 cycles + 0 inverted DAG edges, 9 register invariants, 17 stages, 20 labs, 50 videos, debug+release ARM builds, `.isr_vector` (472 B, 512 B align), and 113 host tests | HIL `pytest`, headless Renode/QEMU CI emulation, `Doxyfile`, `.clang-tidy`, `cppcheck`, and `lcov` CI jobs remain Phase 5 enhancements |

## 2. Initial (Pre-Part-4) vs Post-Red-Team Recalibrated Coverage by All 27 Domains

| # | Domain | Total | Init COMPLETE | Init PARTIAL | Init MISSING | Curr COMPLETE | Curr PARTIAL | Curr MISSING | Curr NEEDS_VERIF | NOT_APPL | Strict Complete % (of Applicable) |
|---:|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| 1 | **Computer Science Foundations** | 6 | 0 | 2 | 4 | **5** | 1 | 0 | 0 | 0 | 83.3% |
| 2 | **C Programming Language** | 59 | 0 | 21 | 38 | **37** | 21 | 1 | 0 | 0 | 62.7% |
| 3 | **Embedded C** | 18 | 0 | 8 | 10 | **14** | 4 | 0 | 0 | 0 | 77.8% |
| 4 | **Computer Architecture** | 5 | 0 | 3 | 2 | **5** | 0 | 0 | 0 | 0 | 100.0% |
| 5 | **ARM Cortex-M Architecture** | 26 | 0 | 15 | 10 | **22** | 3 | 0 | 0 | 1 | 88.0% |
| 6 | **Digital Electronics Foundations** | 6 | 0 | 1 | 5 | **5** | 0 | 0 | 1 | 0 | 83.3% |
| 7 | **Microcontroller Architecture** | 5 | 0 | 2 | 3 | **5** | 0 | 0 | 0 | 0 | 100.0% |
| 8 | **STM32 Architecture** | 4 | 0 | 1 | 3 | **4** | 0 | 0 | 0 | 0 | 100.0% |
| 9 | **STM32G4-Specific Features** | 11 | 0 | 1 | 7 | **6** | 1 | 0 | 1 | 3 | 75.0% |
| 10 | **Clock Systems** | 6 | 0 | 4 | 2 | **5** | 1 | 0 | 0 | 0 | 83.3% |
| 11 | **Memory Systems** | 12 | 0 | 5 | 7 | **3** | 8 | 1 | 0 | 0 | 25.0% |
| 12 | **GPIO and Digital Interfaces** | 7 | 0 | 5 | 2 | **5** | 1 | 1 | 0 | 0 | 71.4% |
| 13 | **Timers and Counters** | 14 | 0 | 2 | 12 | **7** | 6 | 1 | 0 | 0 | 50.0% |
| 14 | **Interrupts and Exceptions** | 5 | 0 | 3 | 2 | **4** | 1 | 0 | 0 | 0 | 80.0% |
| 15 | **Communication Protocols** | 18 | 0 | 4 | 14 | **8** | 7 | 3 | 0 | 0 | 44.4% |
| 16 | **Analog Peripherals** | 9 | 0 | 3 | 6 | **4** | 4 | 0 | 1 | 0 | 44.4% |
| 17 | **DMA and Data Acquisition** | 4 | 0 | 3 | 1 | **4** | 0 | 0 | 0 | 0 | 100.0% |
| 18 | **Real-Time Systems** | 8 | 0 | 5 | 3 | **3** | 5 | 0 | 0 | 0 | 37.5% |
| 19 | **RTOS** | 21 | 0 | 10 | 10 | **0** | 20 | 0 | 0 | 1 | 0.0% |
| 20 | **Toolchains and Debugging** | 29 | 0 | 15 | 13 | **16** | 10 | 2 | 0 | 1 | 57.1% |
| 21 | **Firmware Architecture** | 15 | 0 | 6 | 9 | **10** | 4 | 1 | 0 | 0 | 66.7% |
| 22 | **Testing and Verification** | 6 | 0 | 0 | 6 | **3** | 3 | 0 | 0 | 0 | 50.0% |
| 23 | **Professional Engineering Practices** | 30 | 1 | 14 | 15 | **10** | 20 | 0 | 0 | 0 | 33.3% |
| 24 | **Power Management** | 6 | 0 | 4 | 2 | **3** | 2 | 0 | 1 | 0 | 50.0% |
| 25 | **Firmware Security** | 8 | 0 | 0 | 8 | **0** | 7 | 1 | 0 | 0 | 0.0% |
| 26 | **Hardware-Software Integration** | 7 | 0 | 0 | 7 | **2** | 3 | 1 | 1 | 0 | 28.6% |
| 27 | **Advanced Embedded Applications** | 5 | 0 | 3 | 2 | **0** | 5 | 0 | 0 | 0 | 0.0% |
| — | **TOTAL (27/27 Domains)** | **350** | **1** | **140** | **203** | **190** | **137** | **12** | **5** | **6** | **55.2%** |

## 3. Aggregate Progression & Red-Team De-Inflation Summary

- **Pre-Part-4 Baseline Strict Completion (of 344 applicable concepts):** `1/344` (**0.3%**) — confirming the original governance claims of 100% PASS were false-green.
- **Post-Part-4 Pre-Red-Team Claim (deprecated):** `271/332` (`81.6%`) — flagged by Agent 10 (`[HIGH-05]`) as inflated by ~75–95 concepts (14 uncompiled FreeRTOS concepts, unconfigured tools, mismatched `gpio_g4.c`/`test_c_foundations.c` mappings, and prose-only concepts).
- **Post-Red-Team Strict Recalibrated Coverage (of 344 applicable concepts):** **`190/344` (`55.2%`) `COMPLETE`**, **`137/344` (`39.8%`) `PARTIAL`**, **`5/344` (`1.5%`) `NEEDS_VERIFICATION`**, and **`12/344` (`3.5%`) `MISSING`** (the 12 newly identified Agent 10 concepts tracked transparently for Phase 5).
- **Explicit Architectural Exclusions (`NOT_APPLICABLE`):** `6` concepts (`ARM-SEC-002`, `TOOL-LINK-002`, `G4-AES-001`, `G4-FMC-001`, `G4-SPI4-001`, `RTOS-SMP-001`) cleanly separated so learners never confuse unavailable silicon/architecture features with curriculum omissions.
