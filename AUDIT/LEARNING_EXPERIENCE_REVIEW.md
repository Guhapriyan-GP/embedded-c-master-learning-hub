# Learning Experience & Beginner Usability Review

**Reviewer Role:** Agent 10 — Learning Experience and Usability  
**Target Learner Profile:** First-time bare-metal Embedded C learner on **Windows 11 + VS Code + ST NUCLEO-G474RE (MB1367 Rev C)**  
**Scope Audited:** Onboarding entry point (`START_HERE.md`) and 7 learner handbooks under `LEARNERS/`

---

## 1. Usability Improvements Delivered

Before this pass, the repository contained comprehensive Part 4 technical stages (`01b-*` through `17-*`), 20 labs, 16 register maps, and 11 audit reports, but lacked a dedicated beginner-friendly onboarding funnel on Windows 11. This pass resolves that friction through **8 purpose-built learner artifacts**:

| Artifact | Beginner Friction Solved | Navigation & Pedagogical Impact |
|---|---|---|
| [`START_HERE.md`](../START_HERE.md) | Newcomers didn't know where to begin or why `08-gpio/` comes before `07-rcc-and-clocks/` and `10-interrupts/` before `09-timers/`. | Single front-door guide explaining the 5-step mastery loop, zero-wire onboard MB1367 hardware map, 10-minute first build/flash/GDB walkthrough, and exact acyclic study sequence. |
| [`LEARNERS/BEGINNER_SETUP_WINDOWS11.md`](../LEARNERS/BEGINNER_SETUP_WINDOWS11.md) | Tool installation, Windows 11 `PATH`, MSYS2 host `gcc`, ST-LINK VCP COM discovery, and missing `STM32G474.svd` caused Day-1 stalls. | Exact Windows 11 install table, 6 CLI verification commands, Device Manager COM check, VS Code extension + SVD instructions, and 3-command setup self-test gate. |
| [`LEARNERS/LEARNER_PROGRESS_DASHBOARD.md`](../LEARNERS/LEARNER_PROGRESS_DASHBOARD.md) | Learners lacked a unified checkbox tracker separating onboard-safe labs from external-hardware labs. | Structured Level 1 / Level 2 / Level 3 dashboard tracking Concept IDs, Theory, Code/Lab, Hardware Verification (`Host Only` / `Onboard Only` / `External Pending`), Assessment, and Next Topic. |
| [`LEARNERS/WORKED_EXAMPLES.md`](../LEARNERS/WORKED_EXAMPLES.md) | Steep jump from C theory to raw STM32G474RE register code and live GDB inspection. | 6 end-to-end walkthroughs using uniform 6-step pedagogy (*Demo → Explanation → Register Code → GDB Commands → Full Lab → Independent Exercise*), covering `BSRR`, Analog `MODER=11b`/Active-HIGH `PC13`, 170/160 MHz Boost PLL, `EXTI13` + `x/8wx $sp`, `TIM2` `EGR=UG`/`rc_w0`, and HardFault `CFSR`/`BFAR` decoding. |
| [`LEARNERS/TROUBLESHOOTING_GUIDE.md`](../LEARNERS/TROUBLESHOOTING_GUIDE.md) | Beginners repeatedly hit `-Werror` conversion errors, linker `.isr_vector`/`.ccmram` failures, STM32G4 `MODER=11b`/`CR5@0x80`/`FRXTH`/`PB8-BOOT0` traps, and ST-LINK COM/SWD locks. | Fast Symptom → Root Cause → Exact Fix tables organized across 6 diagnostic categories. |
| [`LEARNERS/FORMULA_AND_CALCULATION_SHEET.md`](../LEARNERS/FORMULA_AND_CALCULATION_SHEET.md) | Key equations were scattered across 17 stage READMEs. | Single clean-LaTeX desk reference covering electronics/RC, DS12288 limits, UM10204 I2C $R_p$, 170/160 MHz PLLs, timers, 4-range `BDTR.DTG`, USART/LPUART/I2C, ADC `TS_CAL1/2`/`VREFINT_CAL`, DAC, IWDG/WWDG, Cortex-M4 bit-banding, and RMS RTOS bounds. |
| [`LEARNERS/REGISTER_QUICK_REFERENCE.md`](../LEARNERS/REGISTER_QUICK_REFERENCE.md) | Full 16 register maps are ideal for deep study but too long for side-by-side coding. | Compact 1-page-per-subsystem desk card verified directly against `stm32g474xx.h` & `core_cm4.h` across RCC, PWR, FLASH, GPIO, SYSCFG, EXTI, TIM, USART, SPI, I2C, ADC, DAC, DMA/DMAMUX, IWDG, and SCB/NVIC/DWT. |
| [`LEARNERS/REVISION_NOTES_AND_MASTERY_GATES.md`](../LEARNERS/REVISION_NOTES_AND_MASTERY_GATES.md) | Passive reading created an illusion of competence without debugger/compiler proof. | Measurable Level 1, 2, and 3 Mastery Gates requiring explanation, `-Werror` implementation, host tests, GDB inspection, and $\ge 85\%$ exam scores. |

---

## 2. Navigation Flow Assessment

- **Entry-to-First-Build Flow:** `START_HERE.md` → `LEARNERS/BEGINNER_SETUP_WINDOWS11.md` → `python scripts/validate_repo.py` + `make -C tests test` + `make -C 23-code-library/templates` → `LEARNERS/LEARNER_PROGRESS_DASHBOARD.md`. Every step has explicit expected output and a direct fallback link to `LEARNERS/TROUBLESHOOTING_GUIDE.md`.
- **Daily Study Flow:** Dashboard → Stage `README.md` → `LEARNERS/REGISTER_QUICK_REFERENCE.md` + `LEARNERS/FORMULA_AND_CALCULATION_SHEET.md` → `LEARNERS/WORKED_EXAMPLES.md` → `18-practical-laboratory/lab-XX` → `LEARNERS/REVISION_NOTES_AND_MASTERY_GATES.md`.
- **Technical & Verification Integrity:** Every register address, offset, reset value, access rule (`rc_w1`/`rc_w0`/`w`), IRQ number, and board pinout in `START_HERE.md` and `LEARNERS/*.md` was verified against `stm32g474xx.h`, `core_cm4.h`, and `20-technical-diagrams/registers/`. Zero physical hardware execution is falsely claimed.

---

## 3. Remaining Pedagogical Polish Items (Future Pass)

1. **Root `README.md` Prominent Link:** Add a single banner link at the very top of `README.md` pointing directly to `START_HERE.md` and `LEARNERS/` *(intentionally deferred from Agent 10 scope to avoid merge conflicts with parallel workers)*.
2. **Physical Hardware Bench Screenshots & Captures:** Once a physical NUCLEO-G474RE (MB1367 Rev C) bench run is performed, capture real VS Code `XPERIPHERALS` screenshots, `PA8` MCO oscilloscope waveforms (170 MHz / 160 MHz), and `JP6` microamp measurements to attach alongside `Worked Examples 3 & 4`.
3. **Automated Link Check & Control-Byte Gate:** Verified clean (`0` broken internal relative links, `0` raw ASCII control bytes) via `python scripts/validate_repo.py`.
4. **Printable PDF Export:** Optionally generate a single two-sided PDF desk card from `LEARNERS/FORMULA_AND_CALCULATION_SHEET.md` and `LEARNERS/REGISTER_QUICK_REFERENCE.md` for lab bench printing.
