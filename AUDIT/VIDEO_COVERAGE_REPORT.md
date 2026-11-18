# Educational Video Coverage & Live-Verification Audit Report

**Repository:** `D:\General_ant\embedded-c-master-learning-hub`  
**Branch:** `feature/complete-remediation`  
**Target Platform:** STM32G474RET6 (ARM Cortex-M4F @ 170 MHz) on NUCLEO-G474RE (`MB1367`)  
**Audit & Live-Verification Date:** `2026-11-18`  
**Concept IDs:** `RES-VID-001`, `RES-VID-002`  
**Verification Method:** Live programmatic Python HTTP verification against BOTH YouTube oEmbed (`https://www.youtube.com/oembed?format=json`) AND live YouTube watch-page HTML (`playabilityStatus.status`, `lengthSeconds`, `publishDate`, `title`, `author_name`) across all 70 entries in `22-video-library/video-database.csv`.

---

## 1. Executive Summary (Pre- vs. Post-Remediation)

| Metric | Pre-Remediation Baseline | Post-Remediation Verified | Change |
|---|---:|---:|---|
| **Total Curated Videos (`video-database.csv`)** | 50 | **70** | **+20 live-verified videos** (`#51`–`#70`) |
| **Live HTTP 200 Pass Rate (oEmbed + Watch Page)** | 50 / 50 (100.0%) | **70 / 70 (100.0%)** | **100.0% live verified, 0 broken links** |
| **Live `playabilityStatus: OK` Pass Rate** | 50 / 50 (100.0%) | **70 / 70 (100.0%)** | **100.0% playable, 0 private/deleted** |
| **Duplicate YouTube Video IDs** | 0 | **0** | Verified via set intersection |
| **Metadata Drift on Baseline `#1`–`#50`** | 0 | **0** | 50/50 exact title, channel, and `publishDate` match |
| **Mapped Videos in `topic-coverage-matrix.md`** | 50 / 50 (initially 45/50) | **70 / 70 (100.0%)** | 0 unmapped videos |
| **Target Gap Topics Covered by Videos (15 Targets)** | 0 / 15 (0.0%) | **14 / 15 (93.3%)** | **+14 gap topics closed** |
| **Overall Curriculum Topic Parity (31 Core Domains)** | 15 / 31 (48.4% strict) / ~70% broad | **29 / 31 (93.5%)** | **+45.1 percentage points** |
| **Remaining Zero-Video Domains (Authoritative Written Docs)** | 16 / 31 | **2 / 31 (6.5%)** | FPU/Barriers/`LDREX` & OPAMP/COMP/VREFBUF |

---

## 2. Live HTTP 200 Verification Proof — All 20 Newly Added Videos (`#51`–`#70`)

Every single new video below was discovered via live web search (`search_web`), resolved from redirect to canonical `youtube.com/watch?v=<id>`, and independently verified live via Python on `2026-11-18`.

| # | YouTube ID | Live oEmbed / Watch HTTP | `playabilityStatus` | Verified Duration | Verified Publish Date | Verified Channel | Verified Title | Target Gap Topic & Concept IDs | Platform / Abstraction Honesty Tag |
|---:|---|:---:|:---:|---|---|---|---|---|---|
| **51** | `B7oKdUvRhQQ` | `200 / 200` | `OK` | 37m 40s | 2020-04-25 | Fastbit Embedded Brain Academy | Bare Metal Embedded C Lecture 4: Writing Linker Scripts and Section Placement | Gap #1 Linker Scripts (`TOOL-LD-001..004`) | **Bare-Metal** (Cortex-M4 GCC; adapt F407 sizes to G474RE) |
| **52** | `2Hm8eEHsgls` | `200 / 200` | `OK` | 25m 23s | 2020-04-24 | Fastbit Embedded Brain Academy | Bare Metal Embedded C Lecture 3: How to Write an MCU Startup File from Scratch | Gap #2 Startup & Vector Tables (`TOOL-BOOT-001..002`, `ARM-EXC-003`) | **Bare-Metal** (Cortex-M4 GCC; adapt 102 G474RE IRQs + FPU) |
| **53** | `3_ar-3OjXnA` | `200 / 200` | `OK` | 57m 57s | 2021-06-16 | Memfault | How to Debug HardFaults on ARM Cortex-M MCUs | Gap #3 HardFault/`CFSR`/`HFSR` (`ARM-FLT-001`, `TOOL-DBG-002`) | **Bare-Metal Architecture-Exact** (ARM Cortex-M3/M4) |
| **54** | `6IUfxSAFhlw` | `200 / 200` | `OK` | 20m 54s | 2020-05-22 | STMicroelectronics | STM32 MPU tips - 1 MPU usage in STM32 with ARM Cortex M7 | Gap #4 Cortex-M MPU (`ARM-MPU-001`, `ARM-CORE-005`) | **Official ST** (Cortex-M7 caveat; shared ARMv7-M PMSAv7 MPU) |
| **55** | `Ysp8hpp8bxI` | `200 / 200` | `OK` | 29m 36s | 2021-05-29 | ControllersTech | #9. ADC DMA in STM32 using Registers \|\| Internal Temp sensor | Gap #6 Bare-Metal DMA (`PERIPH-DMA-001..003`, `DMA-CONT-001`) | **Bare-Metal Registers** (STM32F4 stream caveat; pair with G4 DMAMUX1 map) |
| **56** | `4xVnlDdouBg` | `200 / 200` | `OK` | 20m 20s | 2022-04-25 | STMicroelectronics | FDCAN External Loop-back Mode | Gap #7 STM32 FDCAN (`G4-FDCAN-001`) | **Official ST** (CubeIDE/HAL & H7 caveat; cites `AN5348`; pair with G4 fixed Message RAM map) |
| **57** | `JwJG251RscQ` | `200 / 200` | `OK` | 38m 12s | 2021-09-20 | STMicroelectronics | STM32G4 OLT - 44 . WDG TIMERS High Resolution Timer HRTIM | Gap #8 STM32G4 HRTIM1 (`G4-HRTIM-001`) | **Official STM32G4 Exact** (Architecture lecture; pair with `hrtim-register-map.md` & `AN4539`) |
| **58** | `SeDyNtKRDMU` | `200 / 200` | `OK` | 30m 52s | 2020-11-10 | STMicroelectronics | CORDIC in Practice, Part 2: Cosine and Sine | Gap #9 STM32G4 CORDIC (`G4-CORDIC-001`) | **Official STM32G4 Exact** (`AN5325`; CubeIDE scaffolding; pair with bare-metal register map) |
| **59** | `AQCTcl0eFRk` | `200 / 200` | `OK` | 20m 42s | 2021-03-16 | STMicroelectronics | STM32H7 OLT - 62. Peripheral Filter Math Accelerator FMAC | Gap #9 STM32G4 FMAC (`G4-FMAC-001`) | **Official ST** (H7 OLT slide label; identical ST FMAC IP in STM32G474RE; pair with `AN5305`) |
| **60** | `MFIzI-UNaEs` | `200 / 200` | `OK` | 6m 38s | 2021-07-08 | STMicroelectronics | How to Create a Super Simple Bootloader, Part 4: Developing the App Code | Gap #10 Bootloader & `VTOR` (`BOOT-JUMP-001`, `STM32-FLASH-002`) | **Official ST** (NUCLEO-G071RB / CubeIDE caveat; teaches linker offset & `SCB->VTOR`) |
| **61** | `nagbMzDy2FA` | `200 / 200` | `OK` | 31m 55s | 2026-11-16 | STM32World | STM32 Secure Bootloader: The Bootloader \| Bootloader #2 | Gap #10 Flash & Secure Bootloader (`BOOT-FLASH-001`, `BOOT-VERIF-001`, `SEC-BOOT-001..002`) | **Practical STM32 C** (USB DFU + digital signature verification; adapt G4 64-bit Flash & Dual-Bank) |
| **62** | `UtQhc4XV8k4` | `200 / 200` | `OK` | 19m 24s | 2020-03-25 | ControllersTech | STM32 Stop Mode Tutorial \| Save Power Efficiently | Gap #11 Low-Power Stop Mode (`STM32-PWR-002`, `RT-PWR-001..002`) | **HAL Caveat** (CubeIDE/legacy STM32; G4 has Stop 0/1 only — pair with `lab-14`) |
| **63** | `AelNsnpfbcM` | `200 / 200` | `OK` | 16m 55s | 2020-07-19 | ControllersTech | STM32 Watchdog Tutorial – IWDG vs WWDG with Examples | Gap #11 Hardware Watchdogs (`RT-WDG-001..002`) | **HAL Caveat** (CubeIDE; translate to `IWDG->KR/PR/RLR` & `WWDG->CR/CFR` via `lab-13`) |
| **64** | `QnkO4q95o6s` | `200 / 200` | `OK` | 39m 15s | 2020-05-10 | ORCASat Command and Data Handling | Unit Testing With Ceedling | Gap #12 Unit Testing / Unity / Ceedling (`TEST-UNIT-001..002`) | **Embedded C Host TDD** (Real CubeSat flight-software Unity/Ceedling walkthrough) |
| **65** | `R5gUzPYYs1s` | `200 / 200` | `OK` | 51m 11s | 2023-10-26 | Parasoft | MISRA C 2023 What You Need to Know \| Embedded Computing Design | Gap #12 MISRA C (`PROF-STD-001..004`, `PROF-SA-001..003`) | **Technical Webinar** (Vendor caveat; pair with open-source `cppcheck` & `industry-standards-misra-barr.md`) |
| **66** | `PKml9ki3178` | `200 / 200` | `OK` | 37m 36s | 2018-04-15 | Quantum Leaps, LLC | #23 RTOS Part-2: Automating the context switch | Gap #13 Cortex-M `PendSV` Context Switch (`RTOS-CTX-001`, `RTOS-PORT-001`) | **Bare-Metal Cortex-M Exact** (Raw `PendSV_Handler` assembly; supplement FreeRTOS `BASEPRI` via `16-rtos/README.md`) |
| **67** | `EBSxZKjgBXI` | `200 / 200` | `OK` | 24m 15s | 2020-11-22 | Quantum Leaps, LLC | #35 State Machines Part-1: What is a state machine? | Gap #14 Event-Driven FSM (`FW-FSM-001`, `CS-FSM-001`) | **Bare-Metal Embedded C Gold Standard** (Miro Samek) |
| **68** | `h_u92uLssDo` | `200 / 200` | `OK` | 40m 53s | 2021-10-22 | Quantum Leaps, LLC | Active Objects, Hierarchical State Machines and Modeling in Practice | Gap #14 HSM & Active Objects (`FW-FSM-002`) | **Embedded C Architecture Gold Standard** (Miro Samek EOC keynote) |
| **69** | `rdmJgQW5JrQ` | `200 / 200` | `OK` | 23m 9s | 2026-05-04 | Phil’s Lab | Debugging, Logging, and Monitoring via SWO Trace (STM32 ITM & DWT) - Phil's Lab #173 | Gap #15 CoreSight SWO/ITM/DWT (`ARM-DBG-001`, `TOOL-DBG-003..004`) | **Practical STM32 Trace** (CubeIDE/H7 caveat; G474RE routes SWO on `PB3`) |
| **70** | `t5EH2DNKbAU` | `200 / 200` | `OK` | 15m 8s | 2019-10-08 | STMicroelectronics | STM32G4 OLT - 9 . System Debug | Gap #15 STM32G4 CoreSight Debug (`ARM-DBG-001`, `STM32-DBGMCU-001`, `PROF-WCET-001`) | **Official STM32G4 Exact** (SWJ-DP, FPB, DWT, ITM/TPIU/SWO, ETM, DBGMCU) |

---

## 3. Baseline Verification Proof — Preserved Videos (`#1`–`#50`)

All 50 pre-existing rows in `22-video-library/video-database.csv` (`#1`–`#50`) were preserved untouched and re-verified live in the exact same `2026-11-18` Python pass:
- **HTTP 200 oEmbed:** `50 / 50` (100%)
- **HTTP 200 Watch Page:** `50 / 50` (100%)
- **`playabilityStatus: OK`:** `50 / 50` (100%)
- **Exact CSV Title / Channel / `publishDate` Match:** `50 / 50` (0 metadata drifts)
- **Combined Database Total:** **`70 / 70` (100.0% live verified)**

---

## 4. Quality-Gate Rejections (Anti-Padding Evidence)

During candidate verification, multiple discovered URLs were **explicitly rejected** to preserve strict engineering integrity:
1. **`6v978pL45y4`** (returned in a search summary for STM32 Watchdogs) — **REJECTED: Live HTTP 404 Not Found** on YouTube oEmbed. Replaced with live-verified ControllersTech redirect `AelNsnpfbcM` (`HTTP 200`, 39,052 views).
2. **`UPpeZOiHb_o`** (*ARM LDREX and STREX, SVC and WFI...*, channel `Embedded Intelligence Studio`, 2026-11-14) — **REJECTED: Only 1 view / unvetted low-authority channel.** Rather than pad Target Gap #5 with weak content, Topic #5 is transparently maintained as Authoritative Written-Resource-Only (Section 5).
3. **`tO11pxeIUTs`** (*How to use Unity framework... (Arabic)*) — **REJECTED: Non-English lecture** for an English curriculum; selected English CubeSat flight-software Ceedling lecture `QnkO4q95o6s` (16,905 views) instead.
4. **`DfiwWxTRIZE`** (Quantum Leaps #14, IAR/TivaC) and **`tqRs5-Z8RRw`** (FastBit 6m PendSV animation) — **DE-DUPLICATED** because FastBit Lecture 3 (`2Hm8eEHsgls`, GCC Cortex-M4 startup) and Quantum Leaps #23 (`PKml9ki3178`, 37m PendSV assembly) already provide deeper, non-redundant bare-metal coverage.

---

## 5. Honest Remaining Gaps & Authoritative Written-Resource Mapping

Two core curriculum domains intentionally retain **0 videos** because official ARM/ST primary technical manuals are strictly more accurate and rigorous than available YouTube lectures:

1. **ARM Cortex-M4F FPU (`CPACR`, Hard-Float ABI, Lazy Stacking `FPCCR`), Memory Barriers (`DMB`/`DSB`/`ISB`), & Exclusive Atomics (`LDREX`/`STREX`)**
   - **Concept IDs:** `ARM-FPU-001`, `C-MATH-003`, `ARM-EXC-001`, `ARM-SYNC-001`, `ARM-SYNC-002`, `C-ATOM-002`, `C-ATOM-004`, `EMB-ATOM-002`
   - **Authoritative Written References:**
     - **ARM Application Note AN298** — *Cortex-M4(F) Lazy Stacking and Context Switching*
     - **ARM Application Note AN321** — *ARM Cortex-M Programming Guide to Memory Barrier Instructions*
     - **ST PM0214 §4.6** — *Floating Point Unit (FPU)* & **ARM DDI 0403E.e** (ARMv7-M Architecture Reference Manual)
     - Repo docs: `04-arm-cortex-m/README.md`, `20-technical-diagrams/registers/nvic-systick-register-map.md`

2. **STM32G474RE Analog Front-End (`OPAMP1–6` PGA, `COMP1–7` Hardware Break, `VREFBUF`)**
   - **Concept IDs:** `G4-OPAMP-001`, `G4-COMP-001`, `ANA-VREF-001`
   - **Authoritative Written References:**
     - **ST RM0440 Chapters 23–25** (VREFBUF, COMP, OPAMP)
     - **ST AN5306** — *Operational amplifier (OPAMP) usage in STM32G4 Series*
     - **ST AN5310** — *Guideline for using analog features of STM32G4 Series versus STM32F3 Series*
     - Repo docs: `20-technical-diagrams/registers/opamp-comp-vrefbuf-register-map.md`, `12-adc-dac-analog/README.md`

### Bare-Metal STM32G474RE Translation Notes (Where Curated Videos Use HAL or Sibling MCUs)
- **STM32G4 `DMAMUX1` Registers (`#55` uses F4 DMA streams):** Pair with `20-technical-diagrams/registers/dma-dmamux-register-map.md`, `RM0440 Ch 12–13`, `ES0430 §2.3.1`, and `lab-10`.
- **FreeRTOS `ARM_CM4F` `BASEPRI` & `configMAX_SYSCALL_INTERRUPT_PRIORITY` (`#39`–`#50` run on ESP32; `#66` builds MiROS):** Pair with FreeRTOS official ARM Cortex-M RTOS documentation, `16-rtos/README.md`, `freertos-context-switch.mmd`, and `lab-15`.
- **STM32G474RE `PWR_CR5` Range 1 Boost Mode & Stop 0/1 (`#18`/`#26`/`#62` target F4/L4/legacy STM32):** Pair with `rcc-register-map.md`, `pwr-flash-register-map.md`, `lab-03`, and `lab-14`.
- **STM32G474RE Dual-Bank 64-bit Flash ECC & Fixed 212-Word FDCAN Message RAM (`#56`/`#60` target H7/G0):** Pair with `pwr-flash-register-map.md`, `fdcan-register-map.md`, `RM0440 Ch 3 & Ch 44`, and `ST AN5348`.
