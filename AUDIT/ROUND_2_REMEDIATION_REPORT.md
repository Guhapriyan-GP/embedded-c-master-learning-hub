# Audit Round 2 — Complete Remediation Report

> [!NOTE]
> **Historical Audit Snapshot Notice:** This document records an earlier audit milestone (Round 2 / commit `848d4ed`, 2026-11-18). Its findings and metric counts reflect that historical state and are preserved verbatim for traceability. For the authoritative, canonical release metrics and active status of the current repository (`v4.0.0`), see [`AUDIT/release_metrics.json`](release_metrics.json) and [`AUDIT/RELEASE_INTEGRITY_REPORT.md`](RELEASE_INTEGRITY_REPORT.md).

> **Date:** 2026-11-18  
> **Branch:** `feature/complete-remediation`  
> **Target Silicon / Board:** **STM32G474RET6** (LQFP64, ARM Cortex-M4F r0p1 `__CM4_REV = 0x0001U`, `__NVIC_PRIO_BITS = 4U`) on **NUCLEO-G474RE (MB1367 Rev C)**  
> **Validator Gate:** [`scripts/validate_repo.py`](../scripts/validate_repo.py) **`v4.2.0`** (`19/19` synthetic regressions caught, `0 ERROR` across all 3 execution modes)  
> **Clock Profile Preservation Guarantee:** Both **`160 MHz`** and **`170 MHz`** Range 1 Boost Mode clock profiles remain **100% intact and untouched**.

---

## 1. Executive Summary

Audit Round 2 executed four coordinated, partition-isolated engineering workstreams followed by a full repository-wide consolidation sweep:

1. **Team A (ARM Cortex-M4, STM32G474RE, Interrupt/NVIC & Embedded C Concurrency):** Audited and remediated **12 core curriculum, register-map, driver, and learner files** to achieve primary-source accuracy on **ARM Cortex-M4 r0p1 Erratum 838869 (`ST ES0430 §2.1.3`)** vs **ARM AN321 §4.9**, `BASEPRI` unmasked high-priority interrupt latency honesty (`__set_BASEPRI_MAX()` + compiler barrier), and `GPIOx->BSRR` cross-pin atomic set/reset vs non-atomic same-pin read-`ODR`-write-`BSRR` toggle semantics.
2. **Team B (Practical Laboratory & LL/HAL Specialist):** Audited and remediated all **20 / 20 practical laboratories** (`18-practical-laboratory/lab-01..20/README.md`), closing the missing Section 1 `Prerequisites` gap (`0/20 -> 20/20`), achieving strict 3-way functional parity across **CMSIS Direct Register / STM32CubeG4 LL / STM32CubeG4 HAL** in `lab-05-exti-pushbutton-interrupt/README.md`, eliminating all overstated `"atomic toggle"` and `"zero-latency"` lab claims, and cross-compiling all `18/18` bare-metal labs cleanly (`0` warnings / `0` errors) plus executing host `lab-20` (`exit 0`).
3. **Team C (Python Validation & CI Specialist):** Upgraded `scripts/validate_repo.py` from `v4.1.0` to **`v4.2.0`**, closing all 7 Audit Round-2 Section 4 requirements (`4.A–4.G`), removing build-stage short-circuiting with an exact 9-state taxonomy, separating Mermaid syntax validation from honest `mmdc` visual rendering (`UNVERIFIED`), adding `3xx` redirect tracking + offline resilience + strict 12-column `video-database.csv` schema enforcement, enforcing `20/20` lab `Prerequisites` and `16/16` RM0440/PM0214 citations, and proving `19/19` synthetic regression resistance.
4. **Round 2 Consolidation Sweep (Worker R2-Consolidate):** Eliminated **100%** of remaining out-of-partition residual occurrences across `14-memory-and-storage/`, `19-projects/03-.../`, `21-reference-library/`, `23-code-library/` (`README.md`, `freertos/Inc/FreeRTOSConfig.h`, `templates/Src/main.c`), `24-assessments/`, `25-glossary/`, `tests/unit/test_gpio_mock.c`, and `lab-05/README.md:71`, authored the 3 mandatory Round 2 reports, updated `AUDIT/MASTER_REMEDIATION_TRACKER.md`, and re-verified all firmware builds, static analyzers, host tests (`594/594`), and validator modes (`0 ERROR`).

---

## 2. Primary-Source Technical Corrections Summary

### 2.1 ARM Cortex-M4 r0p1 Erratum 838869 (`ST ES0430 §2.1.3`) vs ARM AN321 §4.9
- **Exact Erratum Title:** *"Store immediate overlapping exception return operation might vector to incorrect interrupt"* (Category B / Rare on Cortex-M4 r0p1, `__CM4_REV = 0x0001U`).
- **Exact Failure Mode:** Under specific bus wait-state overlap when a buffered store-immediate (`STR`/`STRH`/`STRB` with immediate offset or pre/post-index) stalls past `BX LR` while a late higher-priority interrupt arrives, the core can **vector to an incorrect interrupt handler** (NOT inherently a duplicate EXTI interrupt).
- **Exact Workaround & Separation from AN321 §4.9:**
  1. **`(void)REG;` dummy peripheral readback (ARM AN321 §4.9):** Flushes the downstream **AHB-to-APB posted-write bridge** so peripheral flag clears (`EXTI->PR1`, `TIMx->SR`) physically commit at the slave before ISR exit, preventing spurious **same-ISR tail-chained re-entry**.
  2. **`__DSB();` before exception return (ARM Erratum 838869 / ST ES0430 §2.1.3):** Drains the **Cortex-M4 internal write buffer** before `BX LR`, protecting *any* trailing buffered store-immediate (`EXTI->PR1`, `GPIOA->BSRR`, or SRAM counters) in leaf ISRs compiled under `-O2` (and now also placed at the tail of `EXTI_DispatchRange()` in `23-code-library/Drivers/exti_g4.c`).

### 2.2 `BASEPRI` Unmasked High-Priority ISRs (No `"Zero-Latency"` Overclaim)
- Replaced every defective `"zero-latency"` ISR claim with **"unmasked high-priority ISRs (unaffected by `BASEPRI` critical-section masking)"** (or `"zero additional BASEPRI masking delay"`), explicitly documenting the 4 remaining physical latency sources: (1) 12+ cycle hardware exception stacking, (2) Flash wait-state / AHB bus contention, (3) multi-cycle load/store/divide instructions, and (4) preemption by higher-priority IRQs, NMI, or HardFault.
- Upgraded `10-interrupts/README.md` §8.1 critical-section nesting helper from `__set_BASEPRI()` to **`__set_BASEPRI_MAX()`** paired with compiler barriers (`__asm__ volatile ("" ::: "memory")`).

### 2.3 `GPIOx->BSRR` Cross-Pin Atomicity vs Non-Atomic Same-Pin Toggle
- Clarified everywhere that writing `GPIOx->BSRR` (`BS[15:0]` / `BR[31:16]`) is **atomic for single-write pin set/reset and cross-pin ISR-safe** (never corrupts sibling pins on the same GPIO port).
- Clarified equally everywhere that toggling a pin by **reading `GPIOx->ODR` and conditionally writing `GPIOx->BSRR`** (including `GPIO_TogglePin()`, `LL_GPIO_TogglePin()`, and `HAL_GPIO_TogglePin()`) is **NOT atomic as a complete toggle** if two concurrent execution contexts (e.g., `main()` + ISR or two RTOS tasks) toggle the **same pin**. Under single-writer ownership per pin it is race-free; under multi-writer same-pin contention, callers must wrap the toggle in a `PRIMASK`/`BASEPRI` critical section.

---

## 3. Repository-Wide Before vs. After Grep Audit

Scanned across **all `.md`, `.h`, and `.c` files** in the repository (excluding `.git/`, vendored `FreeRTOS-Kernel/`, and historical `AUDIT/` quote records):

| Pattern / Defect Class | Before Round 2 (Raw Non-AUDIT Matches) | After Round 2 (Raw Non-AUDIT Matches) | Remaining **Defective** Claims (Non-AUDIT) | Explanation of Benign Remaining Raw Matches |
|---|---:|---:|---:|---|
| `zero[- ]latency` | `13` (`5` Team A + `1` Lab 05 + `7` Consolidation) | **`1`** | **`0`** | Sole raw match is the explicit anti-pattern warning in [`10-interrupts/README.md`](../10-interrupts/README.md) L60: *`Avoid calling priorities 0..(N-1) "zero-latency ISRs."`* |
| `atomic.{0,80}toggle` / `toggle.{0,80}atomic` / `led_toggle_atomic` | `40` (`11` Team A + `23` Labs + `6` Consolidation) | **`19`** | **`0`** | All 19 remaining raw matches are explicit technical nuance clarifications (e.g., *"NOT atomic as a complete toggle"*, *"Atomic BSRR set/reset & sibling-safe BSRR toggle"*, *"cross-pin atomic on Port A"*). **`0` `led_toggle_atomic` symbols remain.** |
| `mandatory.{0,120}DSB` / `DSB.{0,120}mandatory` | `6` (`1` Team A + `5` outside) | **`0`** | **`0`** | **Zero** overstated `"mandatory"` + `DSB` claims remain anywhere outside historical `AUDIT/` records. |
| `838869` (Erratum conflation check) | `49` *(contained conflations & omissions)* | **`53`** *(expanded authoritative citations)* | **`0`** | All `53/53` non-AUDIT citations accurately separate ARM Erratum 838869 (`ST ES0430 §2.1.3`, wrong-vectoring) from ARM AN321 §4.9 (APB dummy readback). |

---

## 4. `lab-05-exti-pushbutton-interrupt` 3-Way Functional Parity Matrix

All three standalone programs in [`18-practical-laboratory/lab-05-exti-pushbutton-interrupt/README.md`](../18-practical-laboratory/lab-05-exti-pushbutton-interrupt/README.md) implement identical hardware configuration and runtime behavior on **NUCLEO-G474RE (`PA5` `LD2`, `PC13` Active-HIGH `B1`)**:

| Parity Dimension | Approach A — CMSIS Direct Register (§6.1) | Approach B — STM32CubeG4 LL (§6.2) | Approach C — STM32CubeG4 HAL (§6.3) |
|---|---|---|---|
| **Standardized Scope Tag** | `[COMPLETE STANDALONE IMPLEMENTATION — Bare-Metal CMSIS, Cross-Compiled]` | `[COMPLETE STANDALONE IMPLEMENTATION — STM32CubeG4 LL Environment Required, Statically Reviewed]` | `[COMPLETE STANDALONE IMPLEMENTATION — STM32CubeG4 HAL Environment Required, Statically Reviewed]` |
| **Peripheral Clocks (`GPIOA`, `GPIOC`, `SYSCFG`)** | `RCC->AHB2ENR` + `RCC->APB2ENR` + dual dummy readback (`AHB2ENR`, `APB2ENR`) + `__DSB()` | `LL_AHB2_GRP1_EnableClock` (`GPIOA`/`GPIOC`) + `LL_APB2_GRP1_EnableClock(SYSCFG)` | `__HAL_RCC_GPIOA_CLK_ENABLE()` / `__HAL_RCC_GPIOC_CLK_ENABLE()` + explicit note that `HAL_GPIO_Init()` enables `SYSCFG` internally |
| **`PA5` (`LD2`) Output Init + Explicit Initial LOW** | `GPIOA->BSRR = GPIO_BSRR_BR5;` + Push-Pull, Low Speed, No Pull, Output (`MODER=01b`) | `LL_GPIO_ResetOutputPin(GPIOA, LL_GPIO_PIN_5)` + Push-Pull, Low Speed, No Pull, Output | `HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_RESET)` + `GPIO_MODE_OUTPUT_PP`, `NOPULL`, `LOW` |
| **`PC13` (`B1`) Digital Input Init (No Pull)** | `GPIOC->MODER=00b`, `GPIOC->PUPDR=00b` | `LL_GPIO_MODE_INPUT`, `LL_GPIO_PULL_NO` | `GPIO_MODE_IT_RISING`, `GPIO_NOPULL` |
| **`SYSCFG` EXTI13 -> Port C Routing** | `SYSCFG->EXTICR[3]` cleared (`Msk`) & set `SYSCFG_EXTICR4_EXTI13_PC` (`0x20`) | `LL_SYSCFG_SetEXTISource(LL_SYSCFG_EXTI_PORTC, LL_SYSCFG_EXTI_LINE13)` | Performed internally by `HAL_GPIO_Init(GPIOC, &gpio)` |
| **Rising-Edge Only (Falling Disabled)** | `EXTI->RTSR1 \|= RT13; EXTI->FTSR1 &= ~FT13;` | `LL_EXTI_EnableRisingTrig_0_31` + `LL_EXTI_DisableFallingTrig_0_31` | `GPIO_MODE_IT_RISING` inside `HAL_GPIO_Init()` |
| **Stale `PR1` Clear & Unmask** | `EXTI->PR1 = PIF13; (void)EXTI->PR1; EXTI->IMR1 \|= IM13;` | `LL_EXTI_ClearFlag_0_31` + `(void)EXTI->PR1;` + `LL_EXTI_EnableIT_0_31` | `__HAL_GPIO_EXTI_CLEAR_IT(GPIO_PIN_13); (void)EXTI->PR1;` |
| **NVIC (`PRIGROUP`, Priority 3, Clear, Enable)** | `NVIC_SetPriorityGrouping(0U)`, prio `3U`, `ClearPendingIRQ`, `EnableIRQ` | `NVIC_SetPriorityGrouping(0U)`, prio `3U`, `ClearPendingIRQ`, `EnableIRQ` | `HAL_NVIC_SetPriorityGrouping(NVIC_PRIORITYGROUP_4)`, prio `(3U, 0U)`, `ClearPendingIRQ`, `EnableIRQ` |
| **Complete ISR + AN321 §4.9 Readback + ES0430 `__DSB()`** | `EXTI15_10_IRQHandler`: `PR1 = PIF13; (void)EXTI->PR1; __DSB();` | `EXTI15_10_IRQHandler`: `LL_EXTI_ClearFlag_0_31` + `(void)EXTI->PR1; __DSB();` | `EXTI15_10_IRQHandler`: `HAL_GPIO_EXTI_IRQHandler` + `(void)EXTI->PR1; __DSB();` + `HAL_GPIO_EXTI_Callback` |
| **Equivalent `LD2` Toggle (Single-Writer)** | `ODR` read -> `BSRR` (`BS5`/`BR5`) | `LL_GPIO_TogglePin(GPIOA, LL_GPIO_PIN_5)` (`ODR` -> `BSRR`) | `HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_5)` (`ODR` -> `BSRR`) |
| **Main Loop** | `while (1) { __WFI(); }` | `while (1) { __WFI(); }` | `while (1) { __WFI(); }` + `SysTick_Handler -> HAL_IncTick()` |
| **Verification Status** | **Compile-Verified (`arm-none-eabi-gcc 14.3.1`, `1,304 B .o`, `0` warn)** | **Statically Reviewed (CubeG4 LL headers not installed on host)** | **Statically Reviewed (CubeG4 HAL headers not installed on host)** |

---

## 5. Honest Local STM32CubeG4 HAL/LL & Mermaid Tooling Disclosure

To maintain strict verification honesty with zero inflation:
1. **STM32CubeG4 HAL/LL Headers Unavailable on Host:** Exhaustive host search across `C:\ST`, `%USERPROFILE%\STM32Cube`, `D:\ST`, `D:\STM32Cube`, and `<workspace-root>/` confirmed **zero** installations of `stm32g4xx_hal.h`, `stm32g4xx_ll_exti.h`, or `stm32g4xx_ll_gpio.h`. Consequently, all STM32CubeG4 LL and HAL code blocks in `Lab 01`, `Lab 05`, and `Lab 08` are **NOT** claimed as locally compiled; every block carries an explicit label: **`STM32CubeG4 HAL/LL package not installed on host — Statically Reviewed against official STMicroelectronics/stm32g4xx-hal-driver API`**. All CMSIS direct-register code blocks (`18/18` bare-metal labs, `12/12` drivers, starter template, FreeRTOS V11.1.0, and `3/3` Capstones) are locally **Compile-Verified** against the vendored CMSIS `stm32g474xx.h` + `core_cm4.h` using `arm-none-eabi-gcc 14.3.1`.
2. **Mermaid CLI (`mmdc`) Unavailable on Host:** `shutil.which("mmdc")` returns `None`. `scripts/validate_repo.py` v4.2.0 therefore separates `mermaid_syntax` (`12/12 INFO/PASS`) from `mermaid_render` (`UNVERIFIED — 'mmdc' (@mermaid-js/mermaid-cli) tooling unavailable on host`).
3. **No Round 2 Physical Hardware Re-Flashing:** Physical NUCLEO-G474RE onboard test logs from Phase 5 / Agent 4 remain valid baseline silicon evidence, while all Round 2 code and comment updates are honestly labeled **Compile-Verified (`arm-none-eabi-gcc 14.3.1`, Not Tested on Physical Hardware in Round 2)** or **Host-Tested (`gcc 15.2.0`, `594/594` assertions)**.

---

## 6. Complete Inventory of Files Modified / Created in Audit Round 2

- **Team A (12 files):** `10-interrupts/README.md`, `02-c-fundamentals/advanced-c-ub-memory-model.md`, `03-embedded-c/README.md`, `06-stm32-fundamentals/README.md`, `08-gpio/README.md`, `09-timers/README.md`, `16-rtos/README.md`, `20-technical-diagrams/registers/exti-syscfg-register-map.md`, `23-code-library/Drivers/gpio_g4.h`, `23-code-library/Drivers/exti_g4.c`, `23-code-library/Drivers/tim_g4.h`, `LEARNERS/WORKED_EXAMPLES.md`.
- **Team B (20 files):** `18-practical-laboratory/lab-01..20/README.md` (deep 3-way parity in `lab-05`, atomicity/barrier alignment in `lab-01/02/05/06/08/10/13/14/15/16`, Section 1 `Prerequisites` in all 20 labs).
- **Team C (2 files):** `scripts/validate_repo.py` (`v4.1.0` -> `v4.2.0`), `AUDIT/VALIDATOR_QUALITY_REPORT.md`.
- **Consolidation Sweep (14 files):** `14-memory-and-storage/README.md`, `19-projects/03-advanced-freertos-digital-power-controller/Inc/FreeRTOSConfig.h`, `21-reference-library/official-st-documentation.md`, `23-code-library/README.md`, `23-code-library/freertos/Inc/FreeRTOSConfig.h`, `23-code-library/templates/Src/main.c`, `24-assessments/answer-keys-and-rubrics.md`, `24-assessments/stage-05-rtos-and-advanced-firmware-exam.md`, `25-glossary/embedded-systems-glossary.md`, `tests/unit/test_gpio_mock.c`, `AUDIT/MASTER_REMEDIATION_TRACKER.md`, plus 3 new Round 2 reports (`AUDIT/ROUND_2_FINDINGS.md`, `AUDIT/ROUND_2_REMEDIATION_REPORT.md`, `AUDIT/ROUND_2_VALIDATION_RESULTS.md`).
