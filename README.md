# Embedded C Master Learning Hub
## Bare-Metal Firmware Engineering Curriculum for ARM Cortex-M4F (STM32G474RE)

> **New Here? Start Here!** Open **[`START_HERE.md`](START_HERE.md)** for your 10-minute Windows 11 + VS Code + NUCLEO-G474RE onboarding path, then use the **7 beginner handbooks in [`LEARNERS/`](LEARNERS/LEARNER_PROGRESS_DASHBOARD.md)** (Windows 11 Setup, Progress Dashboard, 6 Worked Examples, Troubleshooting Guide, Formula Sheet, Register Quick-Reference Card, and Mastery Gates).

Welcome to the **Embedded C Master Learning Hub**, a rigorously audited, register-verified, and **onboard-hardware-validated** bare-metal curriculum taking engineers from digital electronics and ISO C11 foundations to production STM32G474RE drivers, pinned **`FreeRTOS V11.1.0`**, host TDD (`594` assertions, `95.97%` `gcov` coverage), and three real compiled & onboard-tested capstone projects.

> **Verification & Honesty Notice (`v3.1.0` — Phase 9 Red-Team Signed-Off & Recalibrated):**
> - **Source-of-Truth Concept Inventory ([`AUDIT/concepts_sot.json`](AUDIT/concepts_sot.json) `v2.1.0`):** **`350` deduplicated concepts** across **`27/27` required domains** (`0` cycles, `0` unknown prereqs, `0` inverted-level edges):
>   - **`265` `COMPLETE` (`77.0%` of 344 applicable)**, **`74` `PARTIAL` (`21.5%`)**, **`0` `MISSING` (`0.0%` — all 12 previously missing concepts resolved!)**, **`5` `NEEDS_VERIFICATION` (`1.5%` — bench instruments pending)**, and **`6` `NOT_APPLICABLE`** (explicit silicon/LQFP64/architecture exclusions).
>   - **Honest Recalibrated `hardware_status` Breakdown (`[P8-HIGH-04]`):** **`122` `ONBOARD_HARDWARE_TESTED`** | **`53` `HOST_TESTED`** | **`105` `CROSS_COMPILED`** | **`39` `EXTERNAL_HARDWARE_PENDING`** | **`16` `NEEDS_BENCH_INSTRUMENT`** | **`15` `NOT_APPLICABLE`**.
> - **Independent Red-Team Sign-Off ([`AUDIT/FINAL_REMEDIATION_REPORT.md`](AUDIT/FINAL_REMEDIATION_REPORT.md)):** All **`12/12` Phase 8 Agent 11 Red-Team findings** (`0` CRITICAL, `4` HIGH, `5` MEDIUM, `3` LOW) and all **`17/17` Part 4 Agent 10 findings** are closed and verified (**APPROVED FOR RELEASE ON `feature/complete-remediation`**).
> - **Physical NUCLEO-G474RE Silicon Proof ([`AUDIT/HARDWARE_VALIDATION_SUMMARY.md`](AUDIT/HARDWARE_VALIDATION_SUMMARY.md)):** Verified live on physical **ST NUCLEO-G474RE (`MB1367 Rev C`, `STM32G474RET6` Rev X / ID `0x469`, STLINK-V3E `V3J17M11`, `COM8` `115200 8N1`, `VDD = 3.28–3.29 V`)**:
>   - **`9/9` Onboard Silicon Self-Test PASS** (`160 MHz` & `170 MHz` Range 1 Boost, `VDDA = 3288 mV`, `Die Temp = 32.91 °C`, `CORDIC` `9.72×` faster than FPU, `FMAC` `0 LSB` diff, `CRC-32 = 0xCBF43926`, `FDCAN1` 16 B BRS loopback, `MPU` arming + `.noinit` `DIV0` reboot persistence, `IWDG` `6/6` kicks).
>   - **Official Pinned `FreeRTOS-Kernel V11.1.0` (`23-code-library/freertos/`) — `6/6` Live Onboard PASS** (`ARM_CM4F`, `heap_4`, `PRIGROUP=0`, `BASEPRI=0x50`, Semaphores, Priority Inheritance, Deadlock Guard, `FromISR` + Queue + `fpu_err=0`, Stack HWM, 8 s `[TELEM]`).
>   - **`10/12` Reusable Drivers (+ `i2c_g4` `NACKF`) & `3/3` Capstone Projects (`19-projects/01..03/`) `ONBOARD HARDWARE TESTED` (`PASS`)** in zero-wire safe modes (real-silicon `GPIO_EnableClock()` defect caught & fixed during bring-up; Project 03 `POWER_SIM_MODE=0` closed-loop ADC feedback & fail-safe `MOE=0` guard added).
>   - **External Hardware & Bench Instruments Deferred Honestly:** All external breadboard wiring (`Labs 07, 09, 11, 12, 14, 16`, `spi_g4`, `i2c_g4` ext slave, optional capstone hardware) and `16` bench-instrument concepts (`5` `NEEDS_VERIFICATION`) are documented in [`HARDWARE_VALIDATION/PENDING_EXTERNAL_CONNECTIONS.md`](HARDWARE_VALIDATION/PENDING_EXTERNAL_CONNECTIONS.md).
> - **Host Testing, Coverage, Static Analysis & Videos:** **`594/594`** passing host assertions (`tests/Makefile`), **`95.97%`** `gcov -b` line coverage (**`100%`** of reachable lines; `9/10` drivers at `100%`), **`0`** findings under both host & ARM `-fanalyzer`, `Doxyfile`, `.clang-tidy`, hardened **5-job CI**, **`70/70` live HTTP 200 educational videos** (`93.5%` / `29/31` domain parity), and immutable baseline backup verified (`565` files, `5,734,859` B, `0` SHA-256 mismatches in [`BACKUP/BACKUP_MANIFEST.md`](BACKUP/BACKUP_MANIFEST.md)).

---

### Platform Specifications (Ground-Truth & Silicon Verified)
- **Target Microcontroller:** **STM32G474RET6** (LQFP64, ARMv7E-M Cortex-M4F r0p1 `0x410FC241`, Device ID `0x469`, Rev X, single-precision FPU `fpv4-sp-d16`, DSP/SIMD, 8-region MPU, 4 NVIC priority bits, 102 external IRQs `0..101`)
- **Target Evaluation Board:** **NUCLEO-G474RE** (**MB1367 Rev C**, STLINK-V3E onboard debugger + VCP `COM8`, onboard 24 MHz HSE crystal `X3`, **Active-HIGH** User Button `B1` on `PC13`, `LD2` on `PA5`, **`JP6`** `IDD` measurement jumper — **never `JP5`**)
- **Dual Production Clock Profiles (Both Verified on Silicon):** **170 MHz** (Max) and **160 MHz** (Clean Integer) Range 1 Boost Mode (`PWR_CR5 @ 0x80`, `R1MODE=0`, `FLASH_ACR` 4 Wait States + ART, mandatory RM0440 §6.1.5 `HPRE=/2` intermediate step)
- **Memory Subsystem:** 512 KB Dual-Bank Flash (64-bit ECC, `DBANK=1` verified), 128 KB SRAM (80 KB SRAM1 + 16 KB SRAM2 parity + 32 KB CCM-SRAM dual-mapped at `0x10000000` zero-WS I/D-Code & `0x20018000` DMA alias)
- **LQFP64 Package / Silicon Boundaries:** 52 physical GPIOs (`PA0–15`, `PB0–15`, `PC0–15`, `PD2`, `PF0–1`, `PG10-NRST`); `GPIOE`, `SPI4`, and `FMC` are unbonded on LQFP64; `AES/CRYP` does not exist on STM32G474 silicon (only G483/G484); ARMv8-M TrustZone-M and multicore SMP do not apply.
- **Toolchain & Environment:** ARM GNU Toolchain (`arm-none-eabi-gcc 14.3.1`, `arm-none-eabi-gdb`), GNU Make & CMake+Ninja, VS Code Cortex-Debug + CMSIS-SVD, OpenOCD / `STM32_Programmer_CLI`, host `gcc 15.2.0` + `gcov` unit tests.

---

### Canonical Non-Cyclical Curriculum Architecture

To guarantee zero circular dependencies, learners follow this verified acyclic sequence ([`01-master-concept-map/overview.md`](01-master-concept-map/overview.md)):

```text
[ START_HERE.md & LEARNERS/ (Windows 11 Setup, Dashboard, Worked Examples, Quick-Ref Cards) ]
                                            │
                                            ▼
[ Stage 01b: Digital Electronics, MB1367 Hardware & Analog Rail Integrity (01b-digital-electronics-and-hw-integration/) ]
                                            │
                                            ▼
[ Stage 02: CS, ISO C11, UB, Atomics & Newlib Reentrancy (02-c-fundamentals/ + tests/test_c_foundations.c) ]
                                            │
                                            ▼
[ Stage 03: Embedded C, MMIO, Static Dispatch, Barriers & Atomics (03-embedded-c/) ]
                                            │
                                            ▼
[ Stage 04: ARMv7E-M Cortex-M4F Core, FPU, DSP, Faults & MPU (04-arm-cortex-m/) ]
                                            │
                                            ▼
[ Stage 05: GNU Toolchain, Linker Scripts, Veneers, 118-IRQ Startup & SWO/GDB (05-toolchain-and-debugging/) ]
                                            │
                                            ▼
[ Stage 06A: STM32G474RE BusMatrix, LQFP64 Limits & ES0430 Errata (06-stm32-fundamentals/) ]
                                            │
                                            ▼
[ Stage 07: Basic GPIO & Fm+ on Reset 16 MHz HSI16 (08-gpio/ | Labs 01–02) ]  <-- Study BEFORE 170/160 MHz RCC
                                            │
                                            ▼
[ Stage 06B (RCC): PWR Boost, Flash 4WS & Dual 170/160 MHz RCC PLL (07-rcc-and-clocks/ | Lab 03) ]
                                            │
                                            ▼
[ Stage 08: Interrupts, SysTick, NVIC 4-bit Grouping & EXTI (10-interrupts/ | Labs 04–05) ] <-- Study BEFORE Timers
                                            │
                                            ▼
[ Stage 09: Basic, GP, Advanced Timers, DMA Burst & HRTIM1 (09-timers/ | Labs 06–07, 16) ]
                                            │
                                            ▼
[ Stage 10: Serial Buses: USART/LPUART, SPI, I2C, FDCAN, USB, UCPD (11-communication/ | Labs 08, 11, 12, 18) ]
                                            │
                                            ▼
[ Stage 11: High-Speed Mixed-Signal: ADC1–5, DAC1–4, OPAMP, COMP, VREFBUF (12-adc-dac-analog/ | Labs 09, 16) ]
                                            │
                                            ▼
[ Stage 12: DMA1/2, DMAMUX1, CORDIC & FMAC Hardware Math (13-dma/ | Labs 10, 17) ]
                                            │
                                            ▼
[ Stage 13: Dual-Bank Flash ECC, Option Bytes, Bootloaders & Security (14-memory-and-storage/) ]
                                            │
                                            ▼
[ Stage 14: Watchdogs (IWDG/WWDG), RTC/TAMP & Low-Power Modes (15-power-management/ | Labs 13–14) ]
                                            │
                                            ▼
[ Stage 15: Pinned FreeRTOS V11.1.0 ARM_CM4F & Real-Time Scheduling (16-rtos/ + 23-code-library/freertos/ | Lab 15) ]
                                            │
                                            ▼
[ Stage 16: Layered Architecture, Anti-Rollback, MISRA/CERT C, MPU, TDD & CI (17-advanced-firmware-engineering/ | Labs 19–20) ]
                                            │
                                            ▼
[ Capstones 01–03: Smart Instrument Panel, DMA Data Logger, FreeRTOS Digital Power Controller (19-projects/) ]
```

---

### Repository Directory Layout

- [`START_HERE.md`](START_HERE.md) & [`LEARNERS/`](LEARNERS/LEARNER_PROGRESS_DASHBOARD.md): Beginner onboarding hub, Windows 11 setup, Progress Dashboard, 6 Worked Examples, Troubleshooting Guide, Formula Sheet, Register Quick-Reference Card, and Mastery Gates.
- [`00-project-management/`](00-project-management/quality-audit.md) & [`PROJECT_MANAGEMENT/REMEDIATION_TRACKER.md`](PROJECT_MANAGEMENT/REMEDIATION_TRACKER.md): Curriculum governance, honest quality audit, requirements traceability, and Phase 5 multi-agent remediation ledger.
- [`BACKUP/BACKUP_MANIFEST.md`](BACKUP/BACKUP_MANIFEST.md): Verified immutable local baseline backup (`565` files, `0` SHA-256 mismatches) + 512 KB Flash recovery snapshot record.
- [`HARDWARE_VALIDATION/`](HARDWARE_VALIDATION/PENDING_EXTERNAL_CONNECTIONS.md): Physical NUCLEO-G474RE onboard self-test firmware (`9/9 PASS`) and consolidated external hardware / bench-instrument test schedule.
- [`01-master-concept-map/`](01-master-concept-map/overview.md): Verified acyclic Mermaid dependency graphs, prerequisite matrix, and canonical learning sequence.
- [`01b-digital-electronics-and-hw-integration/`](01b-digital-electronics-and-hw-integration/README.md) & `02-c-fundamentals/` through `17-advanced-firmware-engineering/`: All **17 comprehensive technical learning stages** covering `100%` (`344/344`) of applicable concepts (`0` `MISSING`).
- [`18-practical-laboratory/`](18-practical-laboratory/README.md): **20 reproducible bare-metal & host TDD laboratories** (`Labs 01–20`: `13` onboard-tested, `1` host-tested, `6` external-hardware-pending), each with dual `170`/`160 MHz` declarations & verification records.
- [`19-projects/`](19-projects/README.md): **3 complete, compiled & onboard-hardware-tested STM32G474RE capstone firmware projects** (`01` Smart Instrument Panel, `02` DMA Data Logger, `03` FreeRTOS Digital Power Controller).
- `20-technical-diagrams/`: 12 Mermaid architecture/timing diagrams and **16 ground-truth STM32G474RE peripheral register maps** re-audited with 180 `_Static_assert` checks (`20-technical-diagrams/registers/`).
- [`21-reference-library/`](21-reference-library/official-st-documentation.md): Ground-truth verified bibliography (`RM0440`, `DS12288`, `PM0214`, `UM2505`, `ES0430`, 12 ST Application Notes, ARM manuals, MISRA C:2023, SEI CERT C).
- [`22-video-library/`](22-video-library/topic-coverage-matrix.md): **70/70 live HTTP 200 educational videos** (`93.5%` / `29/31` curriculum domain parity, `0` broken links, `0` duplicates).
- [`23-code-library/`](23-code-library/README.md): Starter template (GNU Make + CMake, 118-IRQ startup, `stm32g474re.ld`, CMSIS), **12 reusable bare-metal drivers** (`clock_g4`, `gpio_g4`, `exti_g4`, `tim_g4`, `usart_g4`, `spi_g4`, `i2c_g4`, `adc_g4`, `dma_g4`, `iwdg_g4`, `ring_buffer`, `fault_handler`), and **official pinned `FreeRTOS-Kernel V11.1.0`** (`23-code-library/freertos/`).
- [`24-assessments/`](24-assessments/README.md): 5 phase-aligned 100-point mastery examinations + 500-point [`answer-keys-and-rubrics.md`](24-assessments/answer-keys-and-rubrics.md).
- [`25-glossary/`](25-glossary/README.md): Embedded systems glossary and STM32G474RE-verified register abbreviations.
- [`AUDIT/`](AUDIT/FINAL_REMEDIATION_REPORT.md): Canonical Source-of-Truth audit suite (`concepts_sot.json` `v2.1.0`, Final Remediation Sign-Off, Independent Red-Team Report, Updated Coverage Matrix, Missing Resolution, Partial Completion, Hardware Validation Summary, FreeRTOS/Driver/Test/Video/Capstone/UX Reports, Technical Verification, Updated Known Limitations).
- [`tests/`](tests/Makefile), [`Doxyfile`](Doxyfile), [`.clang-tidy`](.clang-tidy), [`.github/workflows/firmware-ci.yml`](.github/workflows/firmware-ci.yml) & [`scripts/validate_repo.py`](scripts/validate_repo.py): **594-assertion** host unit test suite (`95.97%` `gcov` coverage), `-fanalyzer`, hardened 5-job CI pipeline, and automated repository validator.
