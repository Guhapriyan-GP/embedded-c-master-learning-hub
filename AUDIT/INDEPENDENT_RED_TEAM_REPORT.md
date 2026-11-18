# Independent Technical Red-Team Audit Report (Agent 10)

**Auditor Role:** Agent 10 — Independent Technical Red-Team Auditor (Adversarial Verification)  
**Target Repository:** `D:\General_ant\embedded-c-master-learning-hub`  
**Target Silicon / Board:** STM32G474RET6 (LQFP64, ARM Cortex-M4F r0p1) / NUCLEO-G474RE (MB1367 Rev C)  
**Primary Ground-Truth Authorities Used:**
- Vendored ST CMSIS `23-code-library/Drivers/CMSIS/Device/ST/STM32G4xx/Include/stm32g474xx.h`
- Vendored ARM CMSIS `23-code-library/Drivers/CMSIS/Core/Include/core_cm4.h`
- ST RM0440 (STM32G4 Reference Manual), DS12288 (STM32G474 Datasheet), UM2505 (NUCLEO-G474RE User Manual), ES0430 (STM32G474 Errata), PM0214 (Cortex-M4 Programming Manual), `stm32g4xx_hal_dma.h`, MISRA C:2012/2023
- Empirical cross-compilation via `arm-none-eabi-gcc 14.3.1` (`-mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard -O2 -std=c11 -Wall -Wextra -Werror`) and automated Python AST/DAG/SoT verification scripts.

---

## 1. Executive Verdict

**RED-TEAM VERDICT: CONDITIONAL HOLD — DO NOT SIGN OFF PART 4 AS-IS.**

While Agents 1–9 made substantial, verifiable progress over the pre-Part-4 baseline (all 102 external IRQ slots in `startup_stm32g474re.c` now match `stm32g474xx.h` 1:1, the linker map is memory-safe, all 18 bare-metal lab code blocks in Labs 01–14 and 16–19 compile cleanly under strict `-Wall -Wextra -Werror -Wconversion -Wdouble-promotion -Wvla`, and physical-hardware testing honesty is cleanly maintained), **independent adversarial testing uncovered 2 CRITICAL, 6 HIGH, 6 MEDIUM, and 3 LOW defects** that escaped Agents 1–9 and `scripts/validate_repo.py`:

| Severity | Count | Summary of Uncaught Defects |
|---|---:|---|
| **CRITICAL** | **2** | (1) Live runtime trap in production template (`main.c` + `fault_handler.c` + `startup_stm32g474re.c`): `FaultHandler_EnableDetailedFaults(true)` enables MemManage/BusFault/UsageFault while those 3 vectors remain aliased to `Default_Handler` (`bkpt #0`), bypassing `g_crash_log` and corrupting post-mortem PC; plus unguarded `stacked_frame[0..7]` dereference locks up the core on MSP overflow. (2) `opamp-comp-vrefbuf-register-map.md` L63–68 **fails to compile** (3 nonexistent `OPAMP_CSR_*` macros) and L32 documents overlapping bitfields `HYST[18:16]` / `BLANKSEL[21:18]`. |
| **HIGH** | **6** | (1) `09-timers/README.md` L217 **fails to compile** (`TIM_AF1_BKCMP1E` vs real `TIM1_AF1_BKCMP1E`). (2) `lab-19` L268 uses nonexistent `SCB->ACTLR` (real CMSIS: `SCnSCB->ACTLR`) and lacks MSP bounds check. (3) `lab-12` L38/L43/L63 `TIMINGR = 0x30612144` violates RM0440 `t_SCLDEL >= t_r + t_SU;DAT` for `t_r > 64.7 ns` AND permanent `2.2 kΩ` pull-up on `PB8-BOOT0` forces `2.705 V` at reset (booting into System ROM Bootloader unless `nSWBOOT0 = 0`). (4) False claim in `AUDIT/PREREQUISITE_DEPENDENCY_AUDIT.md` L78 (`"every Level 1 concept depends exclusively on Level 1 concepts"`) — programmatic DAG audit found **23 inverted-level edges** (`18` L1→L2 + `5` L2→L3) plus bogus synthetic prerequisites (`PROF-GIT-001` → `C-INT-004`). (5) Severe **Status Inflation** in `AUDIT/concepts_sot.json` (`81.6%` / `271 COMPLETE` inflated by ~75–95 concepts: 14 uncompiled FreeRTOS concepts, 17 concepts falsely mapped to 72-line `gpio_g4.c`, 53 C/CS concepts mapped to 211-line `test_c_foundations.c`, and unimplemented tools like `Doxyfile`/`clang-tidy`). (6) **12 genuinely important embedded C / STM32G474RE concepts** completely absent (`0` hits) from `AUDIT/concepts_sot.json`. |
| **MEDIUM** | **6** | (1) `clock_g4.h` L28–29 promises `PLL_P`/`PLL_Q` outputs while `clock_g4.c` L88–95 never enables `PLLPEN`/`PLLQEN`; `Clock_DelayUs()` hangs if `Clock_DWT_Init()` wasn't called. (2) `ring_buffer.c` mixes C11 `atomic_thread_fence()` with non-atomic `volatile uint32_t` (C11 data race) and has non-atomic `rb_count()` observer underflow risk. (3) `05-toolchain-and-debugging/README.md` L196 falsely claims its standalone `Reset_Handler` snippet is `Compile-Verified` (fails with `-Wimplicit-function-declaration`). (4) Wrong RM0440 chapter numbers (`lab-16` L60 cites Ch. 27 for TIM1 instead of **Ch. 28**; `rtc-tamp-usb-register-map.md` L9 cites Ch. 34 for RTC instead of **Ch. 37**; `lab-16` L308 uses `BLANKSEL` instead of `BLANKING`). (5) RNG error-handling bugs in `cordic-fmac-crc-rng-register-map.md` L108 and `17-advanced-firmware-engineering/README.md` L170–176 (`rc_w0` RMW anti-pattern, dead `SECS` check after `DRDY` spin, missing `RNGEN` toggle recovery, missing RCC readbacks). (6) Cross-audit drift: `PREREQUISITE_DEPENDENCY_AUDIT.md` L47–56 stage numbering is off-by-one vs `16-rtos/README.md` (`# Stage 15`) and `17-advanced-firmware-engineering/README.md` (`# Stage 16`); `KNOWN_LIMITATIONS.md` L57 contains stale Mermaid failure claim. |
| **LOW** | **3** | (1) `21-reference-library/industry-standards-misra-barr.md` L21 misclassifies MISRA C:2012/2023 Rule 11.4 as `Required` (official category is **`Advisory`**). (2) `spi-i2c-register-map.md` L119 `I2C1_ReadReg()` Phase 2 read loop omits `I2C_ISR_NACKF` check. (3) `AUDIT/concepts_sot.json` `concept` strings retain raw Markdown noise (`****`, `**9. Git Workflows:**`). |

---

## 2. Independent Primary-Source Verification Matrix (28 Checks)

Every check below was executed independently against `stm32g474xx.h`, `core_cm4.h`, official ST/ARM manuals, `arm-none-eabi-gcc 14.3.1`, and direct Python verification scripts.

| # | Target File & Lines | Technical Claim / Artifact Verified | Primary Ground Truth | Result |
|---:|---|---|---|---|
| **01** | `23-code-library/templates/Src/startup_stm32g474re.c` L201–323 | All 102 external IRQ slots (`IRQn 0..101`, slot 85 reserved) + 16 core vectors = 118 entries (472 B) | `stm32g474xx.h` `IRQn_Type` (programmatic 102/102 check) | **PASS** |
| **02** | `23-code-library/templates/stm32g474re.ld` L24–29, L185–188 | `FLASH 0x08000000 512K`, `RAM 0x20000000 96K` (SRAM1 80K + SRAM2 16K), `_estack = 0x20018000`, `CCMRAM 0x10000000 32K (xrw)` | `stm32g474xx.h` (`SRAM1_SIZE_MAX=0x14000`, `SRAM2_BASE=0x20014000`, `SRAM2_SIZE=0x4000`, `CCMSRAM_BASE=0x10000000`, `CCMSRAM_SIZE=0x8000`) | **PASS** |
| **03** | `23-code-library/Drivers/fault_handler.c` L14–61 + `startup_stm32g474re.c` L75–77 + `main.c` L38 | Detailed fault enabling & HardFault crash capture into `.noinit` | PM0214 §4.4.9–4.4.14; ARMv7-M exception escalation rules | **FAIL (CRITICAL)** |
| **04** | `23-code-library/Drivers/clock_g4.c` L19–131, L177–188 & `clock_g4.h` L28–29 | 170 MHz Boost Mode (`PWR->CR5`, `PWR->SR2`, 4 WS, `HPRE=/2` ramp), `HSE24` X3 crystal, `PLL_P`/`PLL_Q`, `Clock_DelayUs` | RM0440 §6.1.5, Table 9, UM2505, `stm32g474xx.h` | **FAIL (MEDIUM)** |
| **05** | `23-code-library/Drivers/ring_buffer.c` L9–19, L48–111 & `ring_buffer.h` L31–37 | C11 acquire/release lock-free SPSC barriers & `rb_count()` safety | ISO C11 §5.1.2.4 / §7.17.4 (`atomic_thread_fence` vs `volatile`) | **FAIL (MEDIUM)** |
| **06** | `18-practical-laboratory/lab-01..14, 16..19` | Claim: `Compile-Verified (arm-none-eabi-gcc 14.3.1, -Wall -Wextra -Werror)` | Direct compilation of all 18 main C blocks with `-O2 -std=c11 -Wall -Wextra -Werror -Wconversion -Wdouble-promotion -Wvla` | **PASS (18/18)** |
| **07** | `18-practical-laboratory/lab-03/README.md` L48–72, L87–152 | PLL math (`16/4*85/2 = 170 MHz`), `PWR->CR5 @ 0x80`, `PWR->SR2 @ 0x14`, `FLASH_ACR_LATENCY_4WS`, `HPRE=/2` | RM0440 §6.1.5, Table 9, `stm32g474xx.h` | **PASS** |
| **08** | `18-practical-laboratory/lab-10/README.md` L45–63, L201–263 | `CKMODE=11b` (42.5 MHz ≤ 60 MHz), `SMP16=640.5` (15.07 µs ≥ 5 µs), `t_START >= 120 µs`, `DMAMUX1 Req 5 = ADC1`, ES0430 `CHTIF1`/`CTCIF1` | DS12288 §5.3.19/24, RM0440 Table 91, ES0430 §2.3.1, `stm32g474xx.h` | **PASS** |
| **09** | `18-practical-laboratory/lab-12/README.md` L34–45, L52–67, L191 | `TIMINGR = 0x30612144` @ 170 MHz (`PRESC=3, SCLDEL=6, SDADEL=1, SCLH=0x21, SCLL=0x44`) & `PB8-BOOT0` external `2.2 kΩ` pull-up | RM0440 §39.4.9 (`t_SCLDEL >= t_r + t_SU;DAT`), UM2505 `R36` 10 kΩ (`2.705 V` latches `BOOT0=1`!) | **FAIL (HIGH)** |
| **10** | `18-practical-laboratory/lab-16/README.md` L60, L65–67, L176–226, L308 | `DAC1 HFSEL=10b`, `OPAMP1 PGGAIN=2 (×8)`, `COMP1 INMSEL=5`, `TIM1_AF1_BKCMP1E`, RM0440 chapter & `BLANKSEL` | `stm32g474xx.h` (`COMP_CSR_BLANKING`), RM0440 Ch. 28 (TIM1, not Ch. 27) | **FAIL (MEDIUM)** |
| **11** | `18-practical-laboratory/lab-17/README.md` L10–11, L53–57, L146–227, L326 | `CORDIC 0x40020C00`, `FMAC 0x40021400`, `CORDIC->CSR` bits, `FMAC` `FUNC=1/2/8`, `DMAMUX1` `FMAC_READ=110` / `FMAC_WRITE=111` | `stm32g474xx.h`, `stm32g4xx_hal_dma.h`, RM0440 Ch. 17–18 | **PASS** |
| **12** | `18-practical-laboratory/lab-18/README.md` L9, L42–61, L146–264 | `FDCAN1 0x40006400`, `SRAMCAN_BASE 0x4000A400`, `RX_FIFO0 +0x0B0`, `TX_BUF +0x278`, 500k/2M BRS math, IEEE 802.3 CRC-32 | `stm32g474xx.h`, RM0440 Ch. 44 §44.3.3, Ch. 16 | **PASS** |
| **13** | `18-practical-laboratory/lab-19/README.md` L45–50, L102–178, L268 | MPU `RBAR`/`RASR` (`SIZE=4` → 32 B, `AP=0`, `XN=1`), stacked frame offsets, `SCB->ACTLR` (L268), MSP overflow check | `core_cm4.h` (`SCnSCB->ACTLR`, NOT `SCB->ACTLR`!), PM0214 §4.6 | **FAIL (HIGH)** |
| **14** | `20-technical-diagrams/registers/opamp-comp-vrefbuf-register-map.md` L32, L63–68 | `COMPx_CSR` bitfields & `OPAMP1_PGA_x8_InternalADC_Init()` CMSIS macros | `stm32g474xx.h` & direct `arm-none-eabi-gcc` compile | **FAIL (CRITICAL)** |
| **15** | `09-timers/README.md` L217 | `TIM1_Complementary_PWM_50kHz_Init()` (`TIM1->AF1 = TIM_AF1_BKCMP1E`) | `stm32g474xx.h` (`TIM1_AF1_BKCMP1E`) & direct `arm-none-eabi-gcc` compile | **FAIL (HIGH)** |
| **16** | `20-technical-diagrams/registers/rtc-tamp-usb-register-map.md` L5–9, L21–62, L73–102 | `RTC`/`TAMP`/`USB`/`CRS` base addresses, offsets, RM0440 chapters, clock-enable readbacks | `stm32g474xx.h` (`offsetof` all pass); RM0440 RTC is **Ch. 37** (not Ch. 34) | **FAIL (MEDIUM)** |
| **17** | `20-technical-diagrams/registers/cordic-fmac-crc-rng-register-map.md` L5–8, L19–116 & `17-advanced-firmware-engineering/README.md` L166–181 | `CORDIC`/`FMAC`/`CRC`/`RNG` bases/offsets/DMAMUX IDs (`110..113`) & RNG error recovery | `stm32g474xx.h` (offsets pass); RM0440 Ch. 33 RNG `rc_w0` & `SECS` recovery fails | **FAIL (MEDIUM)** |
| **18** | `20-technical-diagrams/registers/hrtim-register-map.md` L5–11, L25–73, L85–118 | `HRTIM1` bases (`0x40016800`/`880`/`B80`), offsets (`DLLCR 0x4C`, `OENR 0x14`, `DTxR 0x38`, `BDMADR 0x70`), IRQs `67..74`, Ch. 27, C snippet | `stm32g474xx.h` `offsetof` + `arm-none-eabi-gcc` compile | **PASS** |
| **19** | `20-technical-diagrams/registers/fdcan-register-map.md` L5–9, L19–54, L69–97 | `FDCAN1–3` & `FDCAN_CONFIG` bases, all 19 struct offsets, fixed `0x350` B RAM partition, 500k @ 24 MHz HSE snippet | `stm32g474xx.h` `offsetof` + `arm-none-eabi-gcc` compile | **PASS** |
| **20** | `20-technical-diagrams/registers/pwr-flash-register-map.md` L4–6, L18–60, L85–108 | `PWR_SR1 0x10`, `PWR_SR2 0x14`, `PWR_CR5 0x80`, `UCPD_DBDIS` b14, `FLASH_SEC1R 0x70`, `SEC2R 0x74`, Table 9 WS, C snippet | `stm32g474xx.h` `offsetof` + RM0440 Table 9 + `arm-none-eabi-gcc` compile | **PASS** |
| **21** | `20-technical-diagrams/registers/rcc-register-map.md` L4–6, L16–97, L108–159 | `RCC_CR.HSION` b8 / `HSIRDY` b10, `CRRCR.HSI48CAL[15:7]`, `CCIPR 0x88`, `CCIPR2 0x9C`, HSE24 Boost snippet | `stm32g474xx.h` `offsetof` + macro check + `arm-none-eabi-gcc` compile | **PASS** |
| **22** | `20-technical-diagrams/registers/adc-dac-register-map.md` L4–6, L16–60, L72–122 | `ADC1 0x50000000`, `DAC1..4 0x50000800..1400` (AHB2), `ADC_CR` reset `0x20000000`, `CKMODE=11b`, `HFSEL` b14, C snippets | `stm32g474xx.h` + DS12288 + `arm-none-eabi-gcc` compile | **PASS** |
| **23** | `20-technical-diagrams/registers/dma-dmamux-register-map.md` L4–8, L16–58, L72–116 | `DMA1`/`DMA2`/`DMAMUX1` bases, IRQs `11..17/96`, `56..60/97..99`, `94`, Table 91 Request IDs (`5`, `24..27`, `110..115`), C snippet | `stm32g474xx.h` + `stm32g4xx_hal_dma.h` + `arm-none-eabi-gcc` compile | **PASS** |
| **24** | `20-technical-diagrams/registers/spi-i2c-register-map.md` L16–57, L69–127 | `SPI_CR2.FRXTH` b12, `I2C_RXDR 0x24` / `TXDR 0x28`, `I2C1_ReadReg()` Phase 2 NACK handling | `stm32g474xx.h` (offsets & compile pass); Phase 2 loop omits `I2C_ISR_NACKF` | **FAIL (LOW)** |
| **25** | `21-reference-library/industry-standards-misra-barr.md` L21, L59 | Claim: `MISRA C:2012/2023 Rule 11.4 (Required)` | Official MISRA C:2012 & C:2023 Rule 11.4 classification = **`Advisory`** | **FAIL (LOW)** |
| **26** | `05-toolchain-and-debugging/README.md` L196 & `Reset_Handler` block | Claim: `C startup snippet Compile-Verified (arm-none-eabi-gcc 14.3.1, -Wall -Wextra -Werror)` | Standalone compile fails (`-Wimplicit-function-declaration` for `__libc_init_array`, `main`) | **FAIL (MEDIUM)** |
| **27** | `AUDIT/PREREQUISITE_DEPENDENCY_AUDIT.md` L47–56, L78 & `AUDIT/KNOWN_LIMITATIONS.md` L57 | Claim: `"every Level 1 concept depends exclusively on Level 1 concepts"`; stage numbers; `.mmd` validation | Programmatic DAG check: **23 inverted edges** (18 L1→L2, 5 L2→L3); off-by-one stage numbers; `.mmd` passes 12/12 | **FAIL (HIGH)** |
| **28** | `AUDIT/concepts_sot.json` (`271 COMPLETE` / `81.6%`) | Status inflation & heading-inflation spot-check against actual `repository_location` files | ~75–95 concepts marked `COMPLETE` lack compiled code, tests, or file-level implementation | **FAIL (HIGH)** |

**Scorecard Summary:** **14 PASS / 14 FAIL** across 28 deep independent checks (`2 CRITICAL`, `6 HIGH`, `6 MEDIUM`, `3 LOW` unique findings consolidated below).

---

## 3. Detailed Adversarial Findings & Concrete Remediation Actions

### CRITICAL FINDINGS

#### [CRIT-01] Production Template `FaultHandler_EnableDetailedFaults(true)` Routes Faults to `Default_Handler`, Bypassing & Corrupting `g_crash_log` (+ Stack-Overflow Lockup)
- **Files & Lines:**
  - `23-code-library/templates/Src/main.c` L38 (`FaultHandler_EnableDetailedFaults(true);`)
  - `23-code-library/Drivers/fault_handler.c` L14–26, L37–54
  - `23-code-library/templates/Src/startup_stm32g474re.c` L74–77, L85–93
- **Verified Defect:**
  1. `main.c` L38 explicitly calls `FaultHandler_EnableDetailedFaults(true)`, setting `SCB->SHCSR |= MEMFAULTENA | BUSFAULTENA | USGFAULTENA` and `SCB->CCR |= DIV_0_TRP`.
  2. However, `startup_stm32g474re.c` L75–77 aliases `MemManage_Handler`, `BusFault_Handler`, and `UsageFault_Handler` to `Default_Handler` (`while (1) { __asm__ volatile("bkpt #0"); }`), and `fault_handler.c` **never defines strong overrides** for any of those three handlers (it only defines `HardFault_Handler_C`).
  3. Consequently, whenever a MemManage fault, BusFault, or UsageFault (such as integer `/0`) occurs in the starter firmware:
     - **With debugger attached (`C_DEBUGEN=1`):** Execution halts inside `Default_Handler`; `g_crash_log` is **never written**.
     - **Standalone (`C_DEBUGEN=0`):** Executing `bkpt #0` inside `Default_Handler` triggers Debug/HardFault escalation (`HFSR.DEBUGEVT`), invoking `HardFault_Handler_C` with the **stack frame of `Default_Handler`**, permanently overwriting the real faulting `PC`/`LR` with `Default_Handler`'s address!
  4. In addition, `fault_handler.c` L42–51 checks only `if (stacked_frame != (uint32_t *)0)` before reading `stacked_frame[0..7]`. On an MSP stack overflow below `0x20000000` (e.g., `0x1FFFFFDC`), `stacked_frame` is non-NULL yet points into unmapped reserved memory, causing an immediate **double fault → Cortex-M4 Lockup** before `g_crash_log.cfsr`/`hfsr` can even be saved.
- **Concrete Remediation:**
  1. In `startup_stm32g474re.c` (or `fault_handler.c`), route `MemManage_Handler`, `BusFault_Handler`, and `UsageFault_Handler` through naked MSP/PSP trampolines calling `HardFault_Handler_C` (or a unified `Fault_Handler_C(uint32_t *frame, uint32_t exc_num)`), AND move `CoreDebug->DHCSR` check inside `Default_Handler` so `bkpt #0` is never executed when no debugger is attached.
  2. Bounds-check `stacked_frame` against valid SRAM (`((uintptr_t)stacked_frame >= SRAM1_BASE) && ((uintptr_t)stacked_frame <= (SRAM2_BASE + SRAM2_SIZE - 32U))` or valid CCMRAM) before dereferencing `stacked_frame[0..7]`, and populate `SCB->CFSR`/`HFSR`/`MMFAR`/`BFAR` *before* dereferencing the stacked frame.

---

#### [CRIT-02] `opamp-comp-vrefbuf-register-map.md` Fails Compilation (3 Nonexistent CMSIS Macros) & Documents Overlapping `COMPx_CSR` Bitfields
- **File & Lines:** `20-technical-diagrams/registers/opamp-comp-vrefbuf-register-map.md` L11, L32, L63–68
- **Verified Defect:**
  1. Direct compilation of the C code block (L52–82) with `arm-none-eabi-gcc 14.3.1` **fails with 3 hard errors**:
     - L65: `OPAMP_CSR_PGAGAIN_Pos` undeclared (real `stm32g474xx.h` macro: **`OPAMP_CSR_PGGAIN_Pos`**)
     - L67: `OPAMP_CSR_OPAINTOEN` undeclared (real `stm32g474xx.h` macro: **`OPAMP_CSR_OPAMPINTEN`**)
     - L68: `OPAMP_CSR_OPAEN` undeclared (real `stm32g474xx.h` macro: **`OPAMP_CSR_OPAMPxEN`**)
  2. L32 documents `COMPx_CSR` bitfields as `HYST[18:16]` and `BLANKSEL[21:18]` — an impossible overlap on bit 18! Verified directly in `stm32g474xx.h`: `COMP_CSR_HYST` is bits `[18:16]` (`Pos=16, Msk=0x7<<16`), and the blanking field is **`COMP_CSR_BLANKING`** at bits **`[21:19]`** (`Pos=19, Msk=0x7<<19`).
  3. L11 erroneously asserts `"Every base address, struct offset, and IRQ verified directly against stm32g474xx.h"` despite these broken bitfield names and non-compiling macros.
- **Concrete Remediation:**
  1. Fix L65/L67/L68 to `OPAMP_CSR_PGGAIN_Pos`, `OPAMP_CSR_OPAMPINTEN`, and `OPAMP_CSR_OPAMPxEN`, and update L21/L62 documentation text to note the CMSIS vs RM0440 naming difference.
  2. Fix L32 to `BLANKING[21:19]` (3 bits, `COMP_CSR_BLANKING`).
  3. Add an automated C-snippet compile gate for all `20-technical-diagrams/registers/*.md` files inside `scripts/validate_repo.py`.

---

### HIGH FINDINGS

#### [HIGH-01] `09-timers/README.md` L217 Fails Compilation (`TIM_AF1_BKCMP1E` Undefined)
- **File & Lines:** `09-timers/README.md` L217
- **Verified Defect:** `TIM1_Complementary_PWM_50kHz_Init()` writes `TIM1->AF1 = TIM_AF1_BKCMP1E;`. Direct compilation with `arm-none-eabi-gcc 14.3.1` fails: `'TIM_AF1_BKCMP1E' undeclared; did you mean 'TIM1_AF1_BKCMP1E'?`. Unlike `TIM_BDTR_*`, STM32G4 CMSIS prefixes `AF1` comparator break bits per timer instance (`TIM1_AF1_BKCMP1E`, `TIM8_AF1_BKCMP1E`, `TIM20_AF1_BKCMP1E`).
- **Concrete Remediation:** Replace `TIM_AF1_BKCMP1E` with `TIM1_AF1_BKCMP1E` at `09-timers/README.md` L217 and compile-verify all stage README snippets in CI.

---

#### [HIGH-02] `lab-19` L268 Uses Invalid CMSIS Struct Member `SCB->ACTLR` (Does Not Exist in `core_cm4.h`)
- **File & Lines:** `18-practical-laboratory/lab-19-hardfault-mpu-and-crash-diagnostics/README.md` L105–112, L268
- **Verified Defect:**
  1. Extension Exercise 3 (L268) instructs learners to execute `SCB->ACTLR |= SCnSCB_ACTLR_DISDEFWBUF_Msk;`. In ARM CMSIS `core_cm4.h`, `SCB_Type` (`0xE000ED00`) has **no `ACTLR` member** — `ACTLR` resides at `0xE000E008` in **`SCnSCB_Type` (`SCnSCB->ACTLR`)**! Any learner attempting Exercise 3 hits an immediate compile error.
  2. Just like `fault_handler.c`, `Fault_DecodeAndPersist()` (L105–112) dereferences `stack_frame[0..7]` without validating that `stack_frame` lies inside SRAM — so Extension Exercise 1 ("induce an infinite recursive function to catch a real stack overflow") works only if the MPU guard catches it *before* SP crosses `0x20000000`, whereas an unguarded MSP underflow causes double-fault Lockup.
- **Concrete Remediation:** Change `SCB->ACTLR` to `SCnSCB->ACTLR` at L268, and add an SRAM range guard around `stack_frame[0..7]` in `Fault_DecodeAndPersist()`.

---

#### [HIGH-03] `lab-12` (`I2C1` 400 kHz): `TIMINGR = 0x30612144` Violates RM0440 `t_SCLDEL` Constraint for `t_r > 65 ns` AND `PB8-BOOT0` Wiring Forces System ROM Bootloader on Reset
- **File & Lines:** `18-practical-laboratory/lab-12-i2c-master-ssd1306-oled/README.md` L36–44, L60–67, L191
- **Verified Defect:**
  1. **`t_SCLDEL` Timing Violation:** At `170 MHz` with `PRESC = 3` (`t_PRESC = 23.529 ns`) and `SCLDEL = 6`, `t_SCLDEL = (6 + 1) × 23.529 ns = 164.7 ns`. Per **RM0440 §39.4.9**, the I2C data setup delay must satisfy $t_{\text{SCLDEL}} \ge t_{r(\max)} + t_{\text{SU;DAT}}$ ($t_{\text{SU;DAT}} = 100\text{ ns}$ for Fast Mode). Thus `0x30612144` only meets spec when bus rise time $t_r \le 64.7\text{ ns}$, directly contradicting Lab 12 L38's own design assumption of `< 300 ns` rise time (which requires $t_{\text{SCLDEL}} \ge 400\text{ ns}$, achievable e.g. with `PRESC = 7` (`t_PRESC = 47.06 ns`), `SCLDEL = 8` (`423.5 ns`), `SDADEL = 0/1`, `SCLH = 16` (`800 ns`), `SCLL = 33` (`1600 ns`) → `0x70811021` or switching `RCC->CCIPR.I2C1SEL = 10b` (`HSI16`) with ST's validated 16 MHz Fast-Mode `TIMINGR`).
  2. **Boot-Trapping Hardware Defect on `PB8-BOOT0`:** Lab 12 L38 & L43 instruct students to attach a permanent external `2.2 kΩ` pull-up to `3.3 V` on `PB8` (`I2C1_SCL`), which fights NUCLEO-G474RE's onboard `10 kΩ` pull-down `R36` to produce $V_{\text{PB8}} = 3.3 \times \frac{10}{12.2} = \mathbf{2.705\text{ V}} > V_{\text{IH}}$. Because factory `FLASH->OPTR.nSWBOOT0 = 1` samples physical `PB8-BOOT0` at reset, **every student who wires Lab 12's table verbatim will boot into the ST System ROM Bootloader instead of User Flash on power-up or NRST!** Lab 12 mentions `nSWBOOT0 = 0` only as an unimplemented parenthetical note while still defaulting the code and wiring table to `PB8`/`PB9`.
- **Concrete Remediation:**
  1. Recompute `TIMINGR` with `PRESC >= 7` (e.g., `0x70811021` @ 170 MHz giving `t_SCLDEL = 423.5 ns >= 300 + 100 ns`, `t_SCLH = 800 ns`, `t_SCLL = 1600 ns`, `f_SCL ~ 380 kHz`) — or select `HSI16` via `RCC->CCIPR.I2C1SEL`.
  2. Make **`PA15` (`I2C1_SCL`, AF4) + `PB7` (`I2C1_SDA`, AF4)** the *default* wiring and code implementation for Lab 12, moving `PB8`/`PB9` to an advisory note that requires explicitly clearing `FLASH->OPTR.nSWBOOT0 = 0` via STM32CubeProgrammer first.

---

#### [HIGH-04] False DAG Tier Claim in `AUDIT/PREREQUISITE_DEPENDENCY_AUDIT.md` (23 Inverted Difficulty Dependencies + Nonsensical Synthetic Edges)
- **Files & Lines:**
  - `AUDIT/PREREQUISITE_DEPENDENCY_AUDIT.md` L78
  - `AUDIT/concepts_sot.json` (23 prerequisite edges)
- **Verified Defect:**
  `AUDIT/PREREQUISITE_DEPENDENCY_AUDIT.md` L78 explicitly claims:
  > *"Zero circular dependencies; **every Level 1 concept depends exclusively on Level 1 concepts**."*

  Programmatic verification of `AUDIT/concepts_sot.json` proves the graph is acyclic (`0` cycles, `0` unknown IDs), **but the Level 1 exclusivity claim is FALSE**. There are **23 inverted-level prerequisite edges** (`18` Level 1 → Level 2 edges and `5` Level 2 → Level 3 edges), including glaringly nonsensical automated-synthesis artifacts:

  | Concept ID (Child) | Child Level | Prerequisite ID | Prereq Level | Red-Team Assessment |
  |---|---|---|---|---|
  | `PROF-GIT-001` (Git Workflows) | Level 1 | `C-INT-004` (Integer Promotions) | Level 2 | **Nonsensical fallback prereq** — Git does not depend on C integer promotions! |
  | `PROF-DOC-001` (API Docs/Doxygen) | Level 1 | `C-INT-004` (Integer Promotions) | Level 2 | **Nonsensical fallback prereq** |
  | `RES-VID-001` / `RES-VID-002` (Video DB) | Level 1 | `C-INT-004` (Integer Promotions) | Level 2 | **Nonsensical fallback prereq** — Video metadata has zero dependency on C promotions! |
  | `RES-ASSESS-001` (Mastery Assessments) | Level 1 | `C-INT-004` (Integer Promotions) | Level 2 | **Nonsensical fallback prereq** |
  | `PERIPH-RNG-001` (Hardware RNG) | Level 2 | `ARM-MPU-001` (8-Region MPU) | Level 3 | **False hardware dependency** — RNG requires `HSI48`/`CLK48SEL`, NOT Cortex-M4 MPU! |
  | `PERIPH-GPIO-001` (Basic GPIO) | Level 1 | `STM32-RCC-001` (PLL 170 MHz) | Level 2 | **Contradicts P3 remediation** (`PREREQUISITE_DEPENDENCY_AUDIT.md` L15 puts GPIO *before* RCC PLL!) |
  | `PERIPH-TIM-001` / `PERIPH-UART-001` | Level 1 | `STM32-RCC-001` (PLL 170 MHz) | Level 2 | Inverted tier (either promote TIM/UART to L2 or split basic RCC clock-gating to L1) |
  | `ARM-SYST-001` (SysTick) | Level 1 | `ARM-NVIC-001` (NVIC Priority) | Level 2 | Inverted tier |
  | `C-UB-003` (Taxonomy of UB) | Level 2 | `C-UB-001` (Strict Aliasing) | Level 3 | Inverted — Taxonomy should be prerequisite *to* Strict Aliasing, not vice versa! |
  | `ARM-FPU-001` (Cortex-M4 FPU) | Level 2 | `ARM-SYNC-002` (Memory Barriers) | Level 3 | Inverted tier |
  | `HWSW-PWR-001` (JP6 Power Profiling) | Level 2 | `RT-PWR-001` (Low-Power Modes) | Level 3 | Inverted tier |
  | *(+10 more L1→L2 / L2→L3 edges)* | L1 / L2 | `C-INT-004`, `C-UB-003`, `C-COMP-003`, `ARCH-EXC-001`, `PERIPH-SYNC-001`, `EMB-ISR-002`, `EMB-ATOM-001`, `FW-EXEC-003`, `FW-FSM-001` | L2 / L3 | All violate strict monotonic tier ordering |

- **Concrete Remediation:**
  1. Strip bogus fallback `C-INT-004` prerequisites from `PROF-GIT-001`, `PROF-DOC-001`, `RES-VID-001/002`, `RES-ASSESS-001`, and remove `ARM-MPU-001` from `PERIPH-RNG-001` (replace with `STM32-RCC-001`).
  2. Reconcile all 23 inverted-level edges so `prereq_level <= child_level` holds strictly across all 338 concepts (and add an automated level-monotonicity check into `scripts/validate_repo.py`).
  3. Reverse `PERIPH-GPIO-001 → STM32-RCC-001` so the JSON DAG actually matches the pedagogical sequence claimed in `PREREQUISITE_DEPENDENCY_AUDIT.md` L15.

---

#### [HIGH-05] Severe Status & Heading Inflation in `AUDIT/concepts_sot.json` (`271 COMPLETE` / `81.6%` Overstated by ~75–95 Concepts)
- **Files & Lines:** `AUDIT/concepts_sot.json`, `AUDIT/CURRICULUM_COVERAGE_MATRIX.md` L12, L21–48, L53
- **Verified Defect:** Programmatic inspection of every concept marked `current_status: "COMPLETE"` against its `repository_location` files exposed **four systematic status-inflation patterns**:
  1. **14 Uncompiled FreeRTOS Concepts Marked `COMPLETE` (Domain 19):**
     - `RTOS-ARCH-001`, `RTOS-CTX-001`, `RTOS-HAZ-001`, `RTOS-HAZ-002`, `RTOS-IPC-001`, `RTOS-ISR-001`, `RTOS-ISR-002`, `RTOS-MEM-001`, `RTOS-SCHED-001`, `RTOS-SYNC-001`, `RTOS-SYNC-002`, `RTOS-SYNC-004`, `RTOS-TASK-001`, `RTOS-TASK-002` are all marked **`COMPLETE`** pointing to `16-rtos/README.md; 18-practical-laboratory/lab-15-freertos-multitask-producer-consumer/README.md`.
     - Yet `AUDIT/KNOWN_LIMITATIONS.md` L22–23 and `lab-15/README.md` L3 explicitly admit FreeRTOS kernel sources are **not vendored** and Lab 15 is **not compiled**! Marking 14 RTOS implementation concepts `COMPLETE` without a single compiled RTOS binary contradicts the repo's own strict completion rubric.
  2. **17 Embedded C Concepts Falsely Mapped to 72-Line `gpio_g4.c` (Domain 3):**
     - `EMB-ATOM-001` (`PRIMASK`/`BASEPRI` critical sections), `EMB-ATOM-002` (`LDREX`/`STREX`), `EMB-ATOM-003` (torn multi-word race conditions), `EMB-ATOM-004` (SPSC ring buffer), `EMB-ISR-001/002` (reentrancy / deferred ISR), `EMB-NUM-001` (fixed-point Q-formats), `EMB-REG-003` (`rc_w1`/`rc_w0`), `EMB-VOL-002/003` (memory barriers) are ALL marked `COMPLETE` with `repository_location = "03-embedded-c/README.md; 23-code-library/Drivers/gpio_g4.c"`.
     - Inspection of `23-code-library/Drivers/gpio_g4.c` (lines 1–72) proves it contains **ZERO** `PRIMASK`/`BASEPRI`, **ZERO** `LDREX`/`STREX`, **ZERO** ISRs, **ZERO** fixed-point math, **ZERO** `__DMB()`/`__DSB()`, and **ZERO** `rc_w1`/`rc_w0` registers!
  3. **53 C/CS Concepts Bulk-Mapped to 386-Line `02-c-fundamentals/README.md` + 211-Line `tests/test_c_foundations.c` (Domains 1–2):**
     - `tests/test_c_foundations.c` L5–6 explicitly states it tests only **5 concepts** (`C-DATA-001`, `C-BIT-001`, `C-PTR-001`, `C-MEM-001`, `EMB-MMIO-001` — 35 assertions).
     - Yet `concepts_sot.json` bulk-marks **53 concepts** `COMPLETE` citing `tests/test_c_foundations.c`, including `C-FPTR-001..003` (function pointers, callbacks, jump tables), `C-PTR-004` (`restrict`), `C-ARR-004` (FAM/VLA), `C-PREP-003` (`##`/`#`), `C-FUNC-003` (AAPCS register passing), `C-STOR-004` (`auto`/`register`), `CS-CONC-001`, `CS-FSM-001`, and `C-COMP-001..004` — **none of which have a single unit test in `test_c_foundations.c`**!
  4. **Unimplemented Tooling & Hardware Timing Marked `COMPLETE` (Domain 23 — 43 concepts mapped to 289-line `17-advanced-firmware-engineering/README.md`, avg 6.7 lines/concept):**
     - `PROF-DOC-001` (Automated API Docs / Doxygen) is marked **`COMPLETE`**, yet `AUDIT/KNOWN_LIMITATIONS.md` L56 explicitly states **`No Doxyfile ... Yet`**!
     - `PROF-SA-002` (`clang-tidy`) is marked **`COMPLETE`**, yet `KNOWN_LIMITATIONS.md` L56 explicitly states **`No .clang-tidy ... Yet`**!
     - `PROF-WCET-002` (Hardware Timing Verification) is marked **`COMPLETE`**, yet `0.0%` of firmware has ever been run on physical hardware!
- **Realistic Strict Completion Assessment:** Downgrading uncompiled RTOS concepts (14), unimplemented tooling (`PROF-DOC-001`, `PROF-SA-002`, `PROF-WCET-002`, etc.), and theory-only concepts lacking dedicated tested/compiled code (~55–75 across Domains 1–5, 20, 23) shifts true strict `COMPLETE` from the claimed **`81.6%` (`271/332`)** down to approximately **`53%–58%` (`~175–195/332`)**, with `~130–150` genuinely `PARTIAL`.
- **Concrete Remediation:**
  1. Downgrade the 14 FreeRTOS concepts (`RTOS-*`), `PROF-DOC-001`, `PROF-SA-002`, `PROF-WCET-002`, and any concept lacking a matching compiled/tested implementation from `COMPLETE` to `PARTIAL` (or add the missing unit tests in `tests/test_c_foundations.c`, `Doxyfile`, `.clang-tidy`, and vendored FreeRTOS build!).
  2. Fix mismatched `repository_location` pointers (e.g., map `EMB-ATOM-004` to `ring_buffer.c`, `EMB-VOL-003` to `ring_buffer.c`/`fault_handler.c`, `EMB-REG-003` to `lab-06`/`lab-09`, NOT `gpio_g4.c`!).
  3. Recompute `AUDIT/CURRICULUM_COVERAGE_MATRIX.md` honestly from the corrected `concepts_sot.json`.

---

#### [HIGH-06] 12 Important Embedded C / STM32G474RE / Firmware Concepts Still Overlooked by Agents 1–9
Every concept below was programmatically verified to have **ZERO (`0`) hits** across all 338 concepts (`id`, `concept`, `explanation`, `practical_implementation`) in `AUDIT/concepts_sot.json`:

| # | Overlooked Concept | Why It Matters for STM32G474RE / Production Embedded C |
|---:|---|---|
| **1** | **Newlib / Newlib-Nano Thread-Safety (`_reent`, `__malloc_lock`, `configUSE_NEWLIB_REENTRANT`)** | Calling `printf`/`snprintf`/`strtok` from multiple FreeRTOS tasks without reentrancy hooks silently corrupts heap/stdio buffers (`_impure_ptr`)! |
| **2** | **`SYSCFG` I2C Fast-Mode Plus 20 mA Drive (`SYSCFG->CFGR1.I2C_PBx_FMP` / `I2C1_FMP`)** | Mandatory STM32G474RE register to run I2C at 1 MHz Fast-Mode Plus or drive heavy bus capacitance — distinct from `GPIOx->OSPEEDR`! |
| **3** | **Timer DMA Burst Mode (`TIMx->DCR` @ `0x3DC` & `TIMx->DMAR` @ `0x3E0`)** | Allows a single DMA channel to atomically update `ARR` + `CCR1..CCR4` per PWM cycle (essential for 3-phase motor FOC / power converters); shifted from `0x48` on F4 to `0x3DC` on G4! |
| **4** | **`LPUART1` / `USART` Autonomous Wakeup from Stop Mode (`CR1.UESM`, `CR3.WUS`, `RCC_CR.HSIKERON`)** | Key low-power STM32G4 feature enabling Wake-on-Address / Wake-on-Start-Bit directly from Stop 0/1 using LSE or HSI16 kernel clock without CPU polling. |
| **5** | **`FDCAN` Transmitter Delay Compensation (`TDCR`/`TDCV`), TX Event FIFO (`TXEFS`), & Bus-Off Recovery State Machine** | Real CAN-FD at ≥ 2 Mbps on a physical transceiver fails without `TDCR.TDCO` compensating transceiver loop delay; production CAN nodes also require deterministic `PSR.BO` Bus-Off recovery (`INIT` toggle + backoff). |
| **6** | **Bootloader Anti-Rollback Protection & Monotonic Version Counters** | Even with Ed25519 signature verification, an attacker can replay an older signed vulnerable binary unless the bootloader enforces a monotonic minimum security version in protected Flash/Option Bytes. |
| **7** | **GNU `ld` `--wrap=symbol` Link-Time Test Seams & ARM Branch Veneers (`__veneer`)** | `--wrap` enables zero-overhead mocking of legacy/vendor functions without modifying production headers; veneers matter when calling `.ccmram` (`0x10000000`) ↔ `FLASH` (`0x08000000`) (`> ±16 MB` Thumb-2 `BL` range requires `BLX` / `-mlong-calls` or linker veneers!). |
| **8** | **USB FS PMA 16-Bit Halfword Access Alignment Quirk & Endpoint Double-Buffering (`DTOG_TX`/`SW_BUF`)** | On STM32G4 USB FS (`0x40006000`), Packet Memory Area requires 16-bit halfword alignment semantics (different from STM32F1 32-bit stride!) and `USB_EPxR` toggle-on-write-1 (`t`) bits are a classic trap causing silent enumeration failures. |
| **9** | **`SWO` Wire Encoding (`NRZ` vs Manchester) & `TPIU->ACPR` Baud Prescaler Math** | Explaining `ITM->PORT[0]` without `TPIU->SPPR` (`2 = NRZ`) and `TPIU->ACPR = (170000000 / 2000000) - 1 = 84` leaves learners unable to bring up SWO at 170 MHz on STLINK-V3E. |
| **10** | **Flash Option-Byte `OBL_LAUNCH` Cold-Reset Mechanics & Bank-Swap (`BFB2`) Brick Traps** | Setting `FLASH_CR_OBL_LAUNCH` immediately forces a system reset; toggling `DBANK` or `BFB2` without a valid vector table at `0x08040000` causes immediate boot failure. |
| **11** | **Zero-Overhead Static HAL Dispatch (X-Macros, Static-Inline Board Policies) vs Function-Pointer Vtables** | Vtables inhibit inlining and cost indirect branch overhead in 170 MHz ISR fast paths; production bare-metal firmware pairs vtables (for runtime polymorphism) with compile-time static dispatch for hot loops. |
| **12** | **Analog Power-Rail Integrity (`VDDA`/`VREF+` Ferrite Bead, Decoupling Capacitor ESR & Resonance, Ground Plane Return Paths)** | Essential for achieving STM32G474RE's rated 12-bit ENOB on mixed-signal PCBs (`ADC1–5` + `OPAMP1–6` + 170 MHz digital switching noise). |

- **Concrete Remediation:** Add these 12 concepts to `AUDIT/concepts_sot.json` (bringing the inventory to 350 concepts) as `PARTIAL`/`MISSING` with explicit Phase 5 roadmap tracking — especially highlighting #7 (`.ccmram` `0x10000000` ↔ `FLASH` `0x08000000` `BL` veneer reachability!) and #1 (newlib thread-safety under FreeRTOS).

---

### MEDIUM FINDINGS

#### [MED-01] `clock_g4.c` / `clock_g4.h` API Contract Mismatch (`PLLPEN`/`PLLQEN` Disabled) & `Clock_DelayUs()` Uninitialized-DWT Hang
- **Files & Lines:** `23-code-library/Drivers/clock_g4.h` L28–29; `23-code-library/Drivers/clock_g4.c` L88–95, L177–188
- **Verified Defect:**
  1. `clock_g4.h` L28–29 states `Clock_Init_170MHz()` *"Also configures PLL_P = 340/6 = 56.67 MHz ... and PLL_Q = 340/8 = 42.5 MHz"*, yet `clock_g4.c` L88–95 sets the dividers (`PLLQ=3`, `PLLPDIV=6`) **without setting `RCC_PLLCFGR_PLLPEN` or `RCC_PLLCFGR_PLLQEN`**, leaving both clock outputs gated OFF! Any downstream ADC or FDCAN/QSPI driver selecting `PLLP`/`PLLQ` will stall silently.
  2. `Clock_DelayUs()` (L183–188) spins on `DWT->CYCCNT` without verifying `(DWT->CTRL & DWT_CTRL_CYCCNTENA_Msk) != 0U`, causing an **infinite hang** if called before `Clock_DWT_Init()`, and silently overflows `uint32_t wait_cycles = us * cycles_per_us;` if `us > 25,264,513` (~25.26 s @ 170 MHz).
- **Concrete Remediation:** Either enable `RCC_PLLCFGR_PLLPEN | RCC_PLLCFGR_PLLQEN` in `clock_g4.c` L92 (or explicitly document in `clock_g4.h` that callers must enable them), and auto-initialize DWT (or guard `CYCCNTENA`) + chunk large delays in `Clock_DelayUs()`.

---

#### [MED-02] `ring_buffer.c` Mixes C11 `atomic_thread_fence()` with Non-Atomic `volatile` Indices & Has Observer-Context `rb_count()` Race
- **Files & Lines:** `23-code-library/Drivers/ring_buffer.h` L35–36; `23-code-library/Drivers/ring_buffer.c` L9–19, L48–62, L77–110
- **Verified Defect:**
  1. `ring_buffer.h` declares `head` and `tail` as `volatile uint32_t`, while `ring_buffer.c` L10–12 uses C11 `atomic_thread_fence(memory_order_release/acquire)`. Under ISO C11 §5.1.2.4 / §7.17.4, a thread fence only establishes happens-before synchronization between **atomic** operations (`_Atomic` + `atomic_load_explicit`/`atomic_store_explicit`), NOT `volatile` accesses. On single-core Cortex-M4 GCC emits `dmb sy` so it happens to work in practice, but under C11 semantics (or multi-threaded host ThreadSanitizer tests) it is technically Undefined Behavior.
  2. `rb_count()` L52–54 reads `head` then `tail` non-atomically; if called from a third context (or preempted across wrap/reset), `rb_free()` L61 (`capacity - rb_count(rb)`) can underflow to a huge unsigned integer.
- **Concrete Remediation:** Document strictly in `ring_buffer.h` that `rb_count()`/`rb_free()` are safe only from the producer or consumer context (not an arbitrary 3rd observer), and migrate `head`/`tail` to C11 `_Atomic uint32_t` with `atomic_load_explicit(..., memory_order_acquire)` / `atomic_store_explicit(..., memory_order_release)` (falling back to `volatile` + `__DMB()` when `__STDC_NO_ATOMICS__` is defined).

---

#### [MED-03] `05-toolchain-and-debugging/README.md` L196 Falsely Labels Standalone `Reset_Handler` Snippet as `Compile-Verified`
- **File & Lines:** `05-toolchain-and-debugging/README.md` L196 & `Reset_Handler` C block
- **Verified Defect:** L196 claims `"C startup snippet Compile-Verified (arm-none-eabi-gcc 14.3.1, -O2 -Wall -Wextra -Werror...)"`, yet direct compilation of that block fails with `error: implicit declaration of function '__libc_init_array'` and `error: implicit declaration of function 'main'` because forward prototypes are omitted.
- **Concrete Remediation:** Add `extern void __libc_init_array(void);` and `extern int main(void);` to the snippet and verify it compiles standalone with `-Werror`.

---

#### [MED-04] Remaining RM0440 Chapter-Number & Bitfield-Name Mismatches (`lab-16`, `rtc-tamp-usb-register-map.md`)
- **Files & Lines:**
  - `18-practical-laboratory/lab-16-dac-opamp-comp-hardware-break/README.md` L60, L308
  - `20-technical-diagrams/registers/rtc-tamp-usb-register-map.md` L9
- **Verified Defect:**
  1. `lab-16` L60 cites **RM0440 Chapter 27** for *Advanced-control timers TIM1*. In RM0440, Chapter 27 is **HRTIM**; Advanced-control timers (`TIM1/TIM8/TIM20`) is **Chapter 28** (as correctly stated in `timers-register-map.md` L6!).
  2. `lab-16` L308 refers to `COMP1->CSR.BLANKSEL` instead of actual CMSIS `COMP_CSR_BLANKING` (`[21:19]`).
  3. `rtc-tamp-usb-register-map.md` L9 cites **RM0440 Ch 34** for *RTC*. In RM0440, **RTC is Chapter 37** (Ch. 35 = IWDG, Ch. 36 = WWDG, Ch. 37 = RTC, Ch. 38 = TAMP).
- **Concrete Remediation:** Correct `lab-16` L60 → `Chapter 28`, `lab-16` L308 → `COMP_CSR_BLANKING`, and `rtc-tamp-usb-register-map.md` L9 → `Ch 37 (RTC)`.

---

#### [MED-05] Hardware `RNG` Error-Handling Bugs & Missing RCC Clock-Enable Readbacks
- **Files & Lines:**
  - `17-advanced-firmware-engineering/README.md` L166–181
  - `20-technical-diagrams/registers/cordic-fmac-crc-rng-register-map.md` L77, L88, L103, L108
  - `20-technical-diagrams/registers/rtc-tamp-usb-register-map.md` L74, L89
- **Verified Defect:**
  1. In `17-advanced-firmware-engineering/README.md` L170–176, `HW_RNG_Read32()` spins on `while ((RNG->SR & RNG_SR_DRDY) == 0U)` *before* checking `RNG_SR_SECS | RNG_SR_CECS`. Per RM0440 Ch. 33, when a Seed Error (`SECS=1`) occurs, **`DRDY` stays `0`**, so the error-recovery branch at L173 is **unreachable** during a seed error (function always times out without clearing `SEIS`)!
  2. Both `17-advanced-firmware-engineering/README.md` L174 and `cordic-fmac-crc-rng-register-map.md` L108 clear `CEIS`/`SEIS` using RMW `RNG->SR &= ~(RNG_SR_CEIS | RNG_SR_SEIS);` — violating the repo's own `rc_w0` rule (BUG-16!) — and omit the RM0440 Ch. 33 seed-error recovery sequence (clear `SEIS`, toggle `RNGEN = 0` → `1`, wait `SECS == 0`, discard initial words).
  3. Five register-map snippets (`cordic-fmac-crc-rng-register-map.md` L77/88/103 and `rtc-tamp-usb-register-map.md` L74/89) enable `RCC->AHB1ENR`/`AHB2ENR`/`APB1ENR1` and immediately touch the peripheral register on the next line without the mandatory readback barrier `(void)RCC->...ENR;` (violating the repo's own `EMB-MMIO-003` rule!).
- **Concrete Remediation:** Move `SECS`/`CECS` check *inside* the polling loop, clear `SEIS`/`CEIS` with direct `rc_w0` write (`RNG->SR = ~(RNG_SR_CEIS | RNG_SR_SEIS);`), toggle `RNGEN` on seed error per RM0440 Ch. 33, and add `(void)RCC->...ENR;` readbacks in all 5 register-map functions.

---

#### [MED-06] Cross-Audit Numbering Drift & Stale Limitation Statements
- **Files & Lines:**
  - `AUDIT/PREREQUISITE_DEPENDENCY_AUDIT.md` L47–56 vs `16-rtos/README.md` L1, L28–30 vs `17-advanced-firmware-engineering/README.md` L1
  - `AUDIT/KNOWN_LIMITATIONS.md` L57
- **Verified Defect:**
  1. `AUDIT/PREREQUISITE_DEPENDENCY_AUDIT.md` L47–56 numbers `10-interrupts` as `Stage 07`, `16-rtos` as `Stage 14`, and `17-advanced-firmware-engineering` as `Stage 15`, whereas `16-rtos/README.md` L1 is `# Stage 15` (citing `10-interrupts` as `Stage 08` on L28!) and `17-advanced-firmware-engineering/README.md` L1 is `# Stage 16` — an off-by-one discrepancy caused by collapsing `06A`/`06B` into one stage number in the audit diagram.
  2. `AUDIT/KNOWN_LIMITATIONS.md` L57 states that standalone `.mmd` files wrap diagrams in ```` ```mermaid ```` fences causing `validate_repo.py` to flag them — which is **stale**: running `python scripts/validate_repo.py` reports `[PASS] All 12 Mermaid (.mmd) diagrams valid and free of HTML tags`.
- **Concrete Remediation:** Reconcile stage numbers identically across `PREREQUISITE_DEPENDENCY_AUDIT.md` and Stages `07–17`, and remove the stale `.mmd` fence item from `KNOWN_LIMITATIONS.md` L57.

---

### LOW FINDINGS

#### [LOW-01] MISRA C:2012/2023 Rule 11.4 Misclassified as `Required`
- **File & Lines:** `21-reference-library/industry-standards-misra-barr.md` L21, L59
- **Verified Defect:** L21 states `Rule 11.4 (Required)` and L59 writes `Advisory/Required`. Official MISRA C:2012 and MISRA C:2023 classify **Rule 11.4 strictly as `Advisory`** (whereas Rules 11.1, 11.2, 11.3 are `Required`).
- **Concrete Remediation:** Change L21 and L59 to `Rule 11.4 (Advisory)` (noting teams frequently elevate it to Required in safety plans, requiring a formal Deviation Permit).

#### [LOW-02] `spi-i2c-register-map.md` L119 Missing `NACKF` Check in Repeated-START Read Phase
- **File & Lines:** `20-technical-diagrams/registers/spi-i2c-register-map.md` L117–121
- **Verified Defect:** Phase 2 of `I2C1_ReadReg()` polls `while (!(I2C1->ISR & I2C_ISR_RXNE))` without checking `I2C_ISR_NACKF`, burning the full timeout if the slave NACKs the read address.
- **Concrete Remediation:** Poll `I2C_ISR_RXNE | I2C_ISR_NACKF` and exit immediately on `NACKF` (clearing `NACKCF`).

#### [LOW-03] Raw Markdown Formatting Artifacts Inside `AUDIT/concepts_sot.json`
- **File & Lines:** `AUDIT/concepts_sot.json` (multiple entries across Domains 2, 3, 19, 21, 23)
- **Verified Defect:** Machine-parsed concept names retain raw Markdown bold/heading prefixes such as `"**** Bit-Shift Undefined..."`, `"**9. Git Workflows:** ..."`, and `"**1. Coding Standards:** ..."`.
- **Concrete Remediation:** Strip leading/trailing Markdown `**` and numbered section prefixes from `concept` strings in `concepts_sot.json` and regenerate dependent Markdown tables.

---

## 4. Honesty & Provenance Audit Verdict

1. **Physical Hardware Testing Honesty (`PASS`):** Full-repository regex scanning across 109 Markdown/Mermaid files and all C/H sources found **ZERO false claims** that firmware was flashed to or measured on physical NUCLEO-G474RE silicon. Every lab (`Lab 01–19`), every stage README (`01b`–`17`), every register map, and `TECHNICAL_VERIFICATION_REPORT.md` / `KNOWN_LIMITATIONS.md` explicitly declare `"Not Tested on Physical Hardware"` / `"0.0% Executed on Physical Board"`.
2. **Compile-Verification & Completeness Honesty (`FAIL — MUST REMEDIATE`):**
   - `opamp-comp-vrefbuf-register-map.md` L63–68 and `09-timers/README.md` L217 **fail to compile** against `stm32g474xx.h`.
   - `05-toolchain-and-debugging/README.md` L196 falsely tags an uncompilable standalone snippet as `Compile-Verified`.
   - `lab-19` L268 teaches a non-compiling CMSIS register expression (`SCB->ACTLR`).
   - `AUDIT/PREREQUISITE_DEPENDENCY_AUDIT.md` L78 makes a demonstrably false Level 1 DAG exclusivity claim.
   - `AUDIT/CURRICULUM_COVERAGE_MATRIX.md` L53 overstates strict completion at `81.6%` (`271/332`) due to status inflation in `concepts_sot.json` (true strict completion ≈ `53%–58%`).

---

## 5. Mandatory Pre-Sign-Off Remediation Checklist

Before Part 4 / `FINAL_COMPLETENESS_REPORT.md` can be honestly signed off, the following items **MUST** be fixed and re-verified:

1. **[CRITICAL]** Fix `fault_handler.c` / `startup_stm32g474re.c` (`MemManage`/`BusFault`/`UsageFault` trampolines + SRAM bounds check on `stacked_frame` + debugger-guarded `bkpt #0` in `Default_Handler`) — **[CRIT-01]**.
2. **[CRITICAL]** Fix `opamp-comp-vrefbuf-register-map.md` L32 (`BLANKING[21:19]`) and L65–68 (`OPAMP_CSR_PGGAIN_Pos`, `OPAMP_CSR_OPAMPINTEN`, `OPAMP_CSR_OPAMPxEN`) — **[CRIT-02]**.
3. **[HIGH]** Fix `09-timers/README.md` L217 (`TIM1_AF1_BKCMP1E`) and `lab-19` L268 (`SCnSCB->ACTLR` + SRAM bounds check) — **[HIGH-01, HIGH-02]**.
4. **[HIGH]** Fix `lab-12` `TIMINGR` (`t_SCLDEL >= 400 ns`) and switch default wiring from `PB8-BOOT0` to `PA15`/`PB7` — **[HIGH-03]**.
5. **[HIGH]** Clean all 23 inverted-level prerequisite edges and bogus `C-INT-004`/`ARM-MPU-001` edges in `AUDIT/concepts_sot.json` and enforce level-monotonicity in `scripts/validate_repo.py` — **[HIGH-04]**.
6. **[HIGH]** Deflate inflated `COMPLETE` statuses in `AUDIT/concepts_sot.json` (14 uncompiled RTOS concepts, unimplemented `PROF-DOC-001`/`PROF-SA-002`/`PROF-WCET-002`, mismatched `gpio_g4.c`/`test_c_foundations.c` mappings) and recompute `AUDIT/CURRICULUM_COVERAGE_MATRIX.md` — **[HIGH-05]**.
7. **[HIGH]** Record the 12 verified overlooked concepts (especially newlib `_reent`, `.ccmram` ↔ `FLASH` veneers, `SYSCFG` FMP, `TIM` `DCR`/`DMAR`, `LPUART` Stop wakeup, `FDCAN` `TDCR`/Bus-Off, anti-rollback) in the SoT / Phase 5 roadmap — **[HIGH-06]**.
8. **[MEDIUM]** Fix `clock_g4.c`/`clock_g4.h` (`PLLPEN`/`PLLQEN` + DWT guard), `05-toolchain-and-debugging/README.md` prototypes, RM0440 Ch. 28/37 citations, `RNG` error-recovery & RCC readbacks, and off-by-one stage numbers / stale `KNOWN_LIMITATIONS.md` L57 — **[MED-01..06]**.
9. **[LOW]** Fix MISRA Rule 11.4 (`Advisory`), `I2C1_ReadReg()` Phase 2 `NACKF`, and Markdown noise in `concepts_sot.json` — **[LOW-01..03]**.
10. **[CI GATE]** Extend `scripts/validate_repo.py` to (a) compile-test all C blocks in `20-technical-diagrams/registers/*.md` and `18-practical-laboratory/lab-01..14,16..19`, and (b) assert `0` DAG cycles AND `0` inverted difficulty-level prerequisite edges in `AUDIT/concepts_sot.json`.
