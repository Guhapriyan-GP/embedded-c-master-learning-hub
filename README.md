# Embedded C Master Learning Hub
## Bare-Metal Firmware Engineering Curriculum for ARM Cortex-M4F (STM32G474RE)

Welcome to the **Embedded C Master Learning Hub**, an audited, register-verified curriculum designed to take engineers from digital electronics and ISO C11 foundations to advanced bare-metal STM32G474RE firmware architecture, real-time systems, and host-verified testing.

> **Verification & Honesty Notice (`v2.1.0` — Red-Team Audited & Recalibrated):**
> - **Source-of-Truth Concept Inventory:** **350 deduplicated concepts** across **27/27 required domains** ([`AUDIT/concepts_sot.json`](AUDIT/concepts_sot.json), `0` cycles, `0` inverted DAG edges): **190 `COMPLETE`** (**55.2%** of 344 applicable — strict rubric requiring deep theory + ground-truth ref + register map/diagram + compile/host-verified code), **137 `PARTIAL`** (theory/register-maps complete; standalone lab, vendored FreeRTOS kernel, or CI tool pending), **12 `MISSING`** (Agent 10 Red-Team Phase 5 additions), **5 `NEEDS_VERIFICATION`** (physical bench measurement pending), and **6 `NOT_APPLICABLE`** (verified silicon/LQFP64/architecture exclusions).
> - **Firmware & Test Proof:** Starter template (`BUILD_TYPE=debug` & `release`), 4 reusable drivers (post-Red-Team remediated), and Labs `01–14` & `16–19` are **Compile-Verified (`arm-none-eabi-gcc 14.3.1`, `-Wall -Wextra -Werror`)** with ELF `.isr_vector` verified at **472 B (118 vectors, 512 B aligned)**; host C unit tests (`tests/Makefile`) are **Compile- & Execution-Verified (`gcc 15.2.0`, 113/113 assertions passed)**; `Lab 15` (FreeRTOS) is **Statically Reviewed**.
> - **Physical Hardware Status:** **Not Tested on Physical NUCLEO-G474RE Hardware yet.** See [`AUDIT/FINAL_COMPLETENESS_REPORT.md`](AUDIT/FINAL_COMPLETENESS_REPORT.md), [`AUDIT/KNOWN_LIMITATIONS.md`](AUDIT/KNOWN_LIMITATIONS.md), and [`AUDIT/CURRICULUM_COVERAGE_MATRIX.md`](AUDIT/CURRICULUM_COVERAGE_MATRIX.md).

---

### Platform Specifications (Ground-Truth Verified)
- **Target Microcontroller:** **STM32G474RET6** (LQFP64, ARMv7E-M Cortex-M4F r0p1 with single-precision FPU `fpv4-sp-d16`, DSP/SIMD, 8-region MPU, 4 NVIC priority bits, 102 external IRQs `0..101`)
- **Target Evaluation Board:** **NUCLEO-G474RE** (**MB1367 Rev C**, STLINK-V3E onboard debugger, onboard 24 MHz HSE crystal X3, **Active-HIGH** User Button B1 on `PC13`, `LD2` on `PA5`, **`JP6`** `IDD` measurement jumper — **never `JP5`**)
- **Core Frequency:** Up to **170 MHz** (Range 1 Boost Mode via `PWR_CR5 @ 0x80`, `FLASH_ACR` 4 Wait States, RM0440 §6.1.5 `HPRE=/2` intermediate step)
- **Memory Subsystem:** 512 KB Dual-Bank Flash (64-bit ECC), 128 KB SRAM (80 KB SRAM1 + 16 KB SRAM2 parity + 32 KB CCM-SRAM dual-mapped at `0x10000000` zero-WS I/D-Code & `0x20018000` DMA alias)
- **LQFP64 Package / Silicon Boundaries:** 52 physical GPIOs (`PA0–15`, `PB0–15`, `PC0–15`, `PD2`, `PF0–1`, `PG10-NRST`); `GPIOE`, `SPI4`, and `FMC` are unbonded on LQFP64; `AES/CRYP` does not exist on STM32G474 silicon (only G483/G484); ARMv8-M TrustZone-M and multicore SMP do not apply.
- **Toolchain & Environment:** ARM GNU Toolchain (`arm-none-eabi-gcc`, `arm-none-eabi-gdb`), GNU Make & CMake+Ninja, VS Code Cortex-Debug + CMSIS-SVD, OpenOCD / STM32CubeProgrammer CLI, host `gcc` unit tests.

---

### Canonical Non-Cyclical Curriculum Architecture

To guarantee zero circular dependencies, learners follow this verified acyclic sequence ([`01-master-concept-map/overview.md`](01-master-concept-map/overview.md)):

```text
[ Stage 01b: Digital Electronics, MB1367 Hardware & Lab Instruments (01b-digital-electronics-and-hw-integration/) ]
                                            │
                                            ▼
[ Stage 02: CS & ISO C11 Language Foundations (02-c-fundamentals/ + tests/test_c_foundations.c) ]
                                            │
                                            ▼
[ Stage 03: Embedded C, MMIO, Volatile, Barriers & Atomics (03-embedded-c/) ]
                                            │
                                            ▼
[ Stage 04: ARMv7E-M Cortex-M4F Core, FPU, Faults & MPU (04-arm-cortex-m/) ]
                                            │
                                            ▼
[ Stage 05: GNU Toolchain, Linker Scripts, 118-IRQ Startup & GDB/SWD (05-toolchain-and-debugging/) ]
                                            │
                                            ▼
[ Stage 06A: STM32G474RE BusMatrix, LQFP64 Limits & ES0430 Errata (06-stm32-fundamentals/) ]
                                            │
                                            ▼
[ Stage 07: Basic GPIO on Reset 16 MHz HSI16 (08-gpio/ | Labs 01–02) ]  <-- Study BEFORE 170 MHz RCC
                                            │
                                            ▼
[ Stage 06B (RCC): PWR Boost, Flash 4WS & 170 MHz RCC PLL + MCO (07-rcc-and-clocks/ | Lab 03) ]
                                            │
                                            ▼
[ Stage 08: Interrupts, SysTick, NVIC 4-bit Grouping & EXTI (10-interrupts/ | Labs 04–05) ] <-- Study BEFORE Timers
                                            │
                                            ▼
[ Stage 09: Basic, GP, Advanced Timers (TIMx) & HRTIM1 (09-timers/ | Labs 06–07, 16) ]
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
[ Stage 13: Dual-Bank Flash ECC, Bootloaders & Silicon Security (14-memory-and-storage/) ]
                                            │
                                            ▼
[ Stage 14: Watchdogs (IWDG/WWDG), RTC/TAMP & Low-Power Modes (15-power-management/ | Labs 13–14) ]
                                            │
                                            ▼
[ Stage 15: FreeRTOS ARM_CM4F & Real-Time Scheduling (16-rtos/ | Lab 15) ]
                                            │
                                            ▼
[ Stage 16: Layered Firmware Architecture, MISRA/CERT C, MPU, TDD & CI (17-advanced-firmware-engineering/ | Labs 19–20) ]
```

---

### Repository Directory Layout

- [`00-project-management/`](00-project-management/quality-audit.md): Curriculum governance, honest quality audit, and requirements traceability matrix.
- [`01-master-concept-map/`](01-master-concept-map/overview.md): Verified acyclic Mermaid dependency graphs, prerequisite matrix, and canonical learning sequence.
- [`01b-digital-electronics-and-hw-integration/`](01b-digital-electronics-and-hw-integration/README.md) & `02-c-fundamentals/` through `17-advanced-firmware-engineering/`: All **17 comprehensive technical learning stages** (18–43 KB each).
- [`18-practical-laboratory/`](18-practical-laboratory/README.md): **20 reproducible bare-metal & host TDD laboratories** (`Labs 01–20`) including 3-way Register/LL/HAL comparisons (`Labs 01, 05, 08`).
- [`19-projects/`](19-projects/README.md): Three hardware-audited capstone architectural blueprints (Smart Instrument Panel, DMA Data Logger, FreeRTOS Digital Power Controller).
- `20-technical-diagrams/`: 12 Mermaid architecture/timing diagrams and **16 ground-truth STM32G474RE peripheral register maps** (`20-technical-diagrams/registers/`).
- [`21-reference-library/`](21-reference-library/official-st-documentation.md): Ground-truth verified bibliography (`RM0440`, `DS12288`, `PM0214`, `UM2505`, `ES0430`, 12 ST Application Notes, ARM manuals, MISRA C:2023, SEI CERT C).
- [`22-video-library/`](22-video-library/topic-coverage-matrix.md): **50 live-verified educational videos** mapped 100% with transparent platform caveats and written compensation for 15 zero-bare-metal-video topics.
- [`23-code-library/`](23-code-library/README.md): Compile-verified bare-metal starter template (GNU Make + CMake, fixed 118-IRQ startup, `stm32g474re.ld`, CMSIS) and 4 reusable drivers (`clock_g4`, `gpio_g4`, `ring_buffer`, `fault_handler`).
- [`24-assessments/`](24-assessments/README.md): 5 phase-aligned 100-point mastery examinations + 500-point [`answer-keys-and-rubrics.md`](24-assessments/answer-keys-and-rubrics.md).
- [`25-glossary/`](25-glossary/README.md): Expanded embedded systems glossary and STM32G474RE-verified register abbreviations.
- [`AUDIT/`](AUDIT/FINAL_COMPLETENESS_REPORT.md): Canonical Source-of-Truth audit suite (`concepts_sot.json` 350 concepts, final sign-off report, independent red-team report, inventory, coverage matrix, gaps, peripheral inventory, DAG audit, practical coverage, technical verification, known limitations).
- [`tests/`](tests/Makefile) & [`scripts/validate_repo.py`](scripts/validate_repo.py): Host C unit test suite (113 passing assertions) and automated repository validation script.
