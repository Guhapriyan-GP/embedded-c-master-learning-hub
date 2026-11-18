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


---
---

# Part II: Phase 5 Complete Remediation Independent Red-Team Audit (Agent 11 — 2026-11-18)

> **Historical Preservation Notice:** Part I above (Lines 1–320) is preserved verbatim as the historical Part 4 Agent 10 audit baseline. Part II below is an independent, adversarial Phase 8 verification of all Phase 5 Complete Remediation deliverables (Agents 0–10 & Worker S).

**Auditor Role:** Agent 11 — Independent Technical Red-Team Auditor (Phase 8 Adversarial Gate)  
**Audit Date:** `2026-11-18`  
**Target Repository:** `D:\General_ant\embedded-c-master-learning-hub` (`feature/complete-remediation`)  
**Immutable Backup Verified:** `D:\General_ant\embedded-c-master-learning-hub-backup` (`565` files, `5,734,859` bytes)  
**Primary Ground-Truth Authorities Used:**
- Vendored ST CMSIS `23-code-library/Drivers/CMSIS/Device/ST/STM32G4xx/Include/stm32g474xx.h` & ARM CMSIS `core_cm4.h`
- ST RM0440 (STM32G4 Reference Manual), DS12288 (STM32G474 Datasheet), UM2505 (NUCLEO-G474RE User Manual), ES0430 (STM32G474 Errata), PM0214 (Cortex-M4 Programming Manual), NXP UM10204 (I2C-bus Spec), ISO C11, LCOV 2.0 documentation
- Empirical local execution: `arm-none-eabi-gcc 14.3.1` (`27` CMSIS `_Static_assert` checks, `24` newly added stage C11 blocks, `-fanalyzer` across all 12 drivers + starter template, full `make clean all` across `templates`, `freertos`, and Capstones `01`–`03`), `gcc 15.2.0` (`574/574` host assertions + `-fanalyzer`), `python scripts/validate_repo.py` (`16 PASS`), independent SHA-256 backup verification, full secret/PII scan, and programmatic `concepts_sot.json` v2.0.0 DAG + hardware-evidence audit.

---

## II.1 Executive Verdict

**PHASE 8 RED-TEAM VERDICT: CONDITIONAL HOLD — 0 CRITICAL, 4 HIGH, 5 MEDIUM, 3 LOW.**

### What Genuinely Improved & Passed Adversarial Verification
1. **All Part 4 CRITICAL & Compile-Error Defects Are Confirmed Fixed:**
   - `[CRIT-01]` (`fault_handler.c` + `startup_stm32g474re.c`): `MemManage_Handler`, `BusFault_Handler`, `UsageFault_Handler` now alias `Fault_Trampoline`; `stacked_frame` is bounds-checked against SRAM1+SRAM2 (`0x20000000..0x20017FE0`) and CCM-SRAM (`0x10000000..0x10007FE0`); `Default_Handler` checks `CoreDebug->DHCSR.C_DEBUGEN` before `bkpt #0`.
   - `[CRIT-02]` (`opamp-comp-vrefbuf-register-map.md`): Fixed `OPAMP_CSR_PGGAIN_Pos`/`OPAMP_CSR_OPAMPINTEN`/`OPAMP_CSR_OPAMPxEN` and `COMP_CSR_BLANKING[21:19]` — now compiles cleanly with `0` warnings.
   - `[HIGH-01..04]` (`09-timers/README.md` `TIM1_AF1_BKCMP1E`, `lab-19` `SCnSCB->ACTLR`, `lab-12` `0x70811021` + `PA15`/`PB7`, `concepts_sot.json` DAG `0` cycles / `0` unknown prereqs / `0` inverted-level edges): all independently verified FIXED.
2. **Real Physical NUCLEO-G474RE Hardware Validation Honesty (`PASS` on External-Hardware Boundaries):**
   - ZERO external-hardware experiments (`Lab 07`, `09`, `11`, `12`, `14` `JP6` ammeter, `16` external scope, external OLED/pot/power-stage) are falsely claimed as physically verified.
   - ZERO destructive or Flash option-byte (`OPTR`/RDP/WRP/PCROP/SEC/BOOT0) modifications were executed.
   - Real silicon bring-up authentically caught and fixed missing `GPIO_EnableClock()` calls across all 3 Capstones (`19-projects/01..03/Src/main.c`).
3. **12 Previously `MISSING` Concepts Resolved & Compile-Verified (`PASS`):**
   - All `12` sections (`C-LIB-001`, `FW-DRV-003`, `GPIO-FMP-001`, `TIM-DMA-001`, `COMM-LP-001`, `COMM-CAN-002`, `COMM-USB-002`, `MEM-OB-001`, `SEC-BOOT-002`, `TOOL-LD-004`, `TOOL-DBG-004`, `HWSW-ANA-001`) contain substantive theory and `24/24` tested C11 code blocks compile cleanly with `arm-none-eabi-gcc 14.3.1 -std=c11 -O2 -Wall -Wextra -Werror`.
4. **Local Backup Integrity (`100% VERIFIED — UNMODIFIED`), Secret Scan (`0 SECRETS / 0 PII`), and Git Artifact Hygiene (`0 UNTRACKED ARTIFACTS`).**

### Why Conditional Hold Is Issued (4 Mandatory HIGH Fixes Before Final Commit & Push)

| Severity | Count | Summary of Verified Phase 8 Defects |
|---|---:|---|
| **CRITICAL** | **0** | None. (Both Part 4 CRITICAL traps are confirmed resolved.) |
| **HIGH** | **4** | (1) **Guaranteed GitHub Actions CI failures** in `.github/workflows/firmware-ci.yml`: `clang-format --dry-run --Werror` violates `.clang-format` on **317** aligned `=`, **102** lines >100 cols, and **12** single-line `case` statements; plus `lcov --rc lcov_branch_coverage=1` breaks under LCOV 2.0 on `ubuntu-latest` (Ubuntu 24.04). (2) **Capstone Project 03 `POWER_SIM_MODE=0` safety/correctness trap**: `vControlLoopTask` still runs the fake software `buck_sim_step(&plant)` open-loop with zero ADC feedback even when compiled as `"EXTERNAL HARDWARE POWER STAGE"`. (3) **`i2c_g4.h` `TIMINGR` overclocking & `PB8-BOOT0` doc regression**: `170MHz_400kHz` (`0x30B21F3B` -> `437 kHz` max / `412 kHz` typ, `t_SCLDEL=282 ns < 400 ns`), `160MHz_400kHz` (`0x30B21D38` -> `435 kHz` max), and `16MHz_100kHz` (`0x00423F4F` -> `107 kHz` max, `t_SCLDEL=312.5 ns` leaving only `62.5 ns` rise-time margin!) violate NXP UM10204 & RM0440 §39.4.9, contradicting Lab 12's spec-compliant `0x70811021`/`0x70710F1F`; and L16–18 contradicts the `PB8-BOOT0` pull-up warning. (4) **`ONBOARD_HARDWARE_TESTED` & `COMPLETE` over-promotion in `AUDIT/concepts_sot.json` v2.0.0**: **~16–20 concepts** marked `ONBOARD_HARDWARE_TESTED` (out of `140`) were never executed or measured on silicon (`0` hits in all 6 flashed images for `WFI`/`WFE`, `TIM6`/`TIM7`, `HSE`/`CSS`/`LSE`/`MCO`, `ITM`/`TPIU` SWO, `nPRIV`/security, `USART` ORE/FE/NE/PE error injection; plus unexercised `MPU` violation, `B1` debouncing, `BDTR`/`PWM`/`DAC`/`COMP` measurements); and `PROF-SA-002`/`PROF-DOC-001` were promoted to `COMPLETE` despite never being executed locally and `.clang-format` failing. |
| **MEDIUM** | **5** | (1) `gpio_g4.c` L137 `GPIO_LockPin()` returns false-positive `true` when attempting to lock a 2nd pin on an already-locked port. (2) Five subtle driver edge-case / RM0440 compliance bugs (`tim_g4.c` L57 `0xFFFFFFFF` 32-bit ARR wrap to `0%`, `usart_g4.c`/`i2c_g4.c` ignoring `RCC->CCIPR`, `adc_g4.c` L70 shared `CCR` write & L102 uncleaned `DMAEN`, `i2c_g4.c` L81 missing `TXDR` flush on NACK, `spi_g4.c` L15 integer floor-division SCK overshoot). (3) `onboard_selftest/Src/main.c` Test 8A arms MPU Region 7 but never triggers an actual MPU `MemManage` fault, and overrides `UsageFault_Handler` (bypassing `fault_handler.c` in the final 9/9 run). (4) Cross-document drift (`lab-19` L308 still says `Queued for Physical Execution`; `freertos/Src/main.c` L229 uses `rc_w0` RMW `TIM2->SR &= ~TIM_SR_UIF`; `rtos_bsp.c` L132 leaves `RCC_CSR_RMVF` set). (5) `19-projects/03/Inc/power_ctrl.h` L123/L137 implementation-defined signed right-shift on negative `int32_t`. |
| **LOW** | **3** | (1) `.gitignore` omits `*.gcda`, `*.gcno`, `*.gcov`, `*.info`. (2) `startup_stm32g474re.c` L393 has unguarded `bkpt #0` if `main()` returns standalone. (3) Local workspace path `D:\General_ant\...` appears in learner-facing `LEARNERS/BEGINNER_SETUP_WINDOWS11.md` (and 6 internal audit/backup docs; zero PII/username). |

---

## II.2 Independent Primary-Source Verification Matrix (22 Deep Phase 8 Checks)

| # | Target File & Lines | Technical Claim / Artifact Independently Verified | Primary Ground Truth / Tool | Result |
|---:|---|---|---|---|
| **01** | `23-code-library/Drivers/clock_g4.c` L27–253 | `Clock_Init_160MHz()` (`N=80`), `Clock_Init_170MHz()` (`N=85`), `HSE24` (`M=6`), `PWR->CR5`, `FLASH->ACR` 4 WS, `HPRE=/2 -> >=1 us -> /1`, `PLLPEN`/`PLLQEN`, auto-DWT guard, `SystemCoreClockUpdate()` (`HSE_VALUE=24 MHz`) | RM0440 §6.1.5, Table 9, `stm32g474xx.h`, `system_stm32g4xx.c` L81 | **PASS** |
| **02** | `23-code-library/Drivers/exti_g4.c` L9–155 | `SYSCFG->EXTICR[pin/4]` nibble shift, `SYSCFGEN` readback, `EXTI->PR1 = mask; __DSB();` (ES0430 §2.1.3), `EXTI_DispatchRange()` | RM0440 Ch. 10 & 15, ES0430 §2.1.3, `stm32g474xx.h` | **PASS** |
| **03** | `23-code-library/Drivers/tim_g4.c` L9–258 | `TIM2/3/6/7` APB1 `TIMCLK` math, `ARPE`/`OCxPE`, `EGR=UG`, `SR = ~UIF` (`rc_w0`), `tim_calc_pwm_ccr()` 32-bit `arr == 0xFFFFFFFFU` wraparound | RM0440 §7.2.8 & Ch. 29, `stm32g474xx.h` (`TIM_CCMR1_OC1M == 0x00010070`) | **FAIL (MEDIUM)** |
| **04** | `23-code-library/Drivers/usart_g4.c` L8–219 | `USART1/2` & `LPUART1` (`256*fclk/baud`, `[0x300..0xFFFFF]`) BRR math, `ICR` error clears, SPSC RX IRQ handler; ignores `RCC->CCIPR` kernel clock mux | RM0440 Ch. 38, `stm32g474xx.h` | **FAIL (MEDIUM)** |
| **05** | `23-code-library/Drivers/spi_g4.c` L8–159 | `SPI1/2/3`, `SSM|SSI`, `DS=7` (8-bit) + `FRXTH=1`, 8-bit `*(__IO uint8_t *)&spi->DR`, `FTLVL|BSY` idle wait; `spi_calc_br_div()` floor-division overshoot | RM0440 §42.5.9, `stm32g474xx.h` | **FAIL (MEDIUM)** |
| **06** | `23-code-library/Drivers/i2c_g4.h` L16–18, L67–72 & `i2c_g4.c` L40–332 | All 6 `I2C_TIMINGR` constants (`170/160/16 MHz` × `100/400 kHz`), `PB8-BOOT0` note, `RELOAD` >255 B, `NACKF`/`BERR`/`ARLO` + `TXDR` flush, 9-clock recovery | NXP UM10204, RM0440 §39.4.9 & §39.7.7, UM2505 `R36` | **FAIL (HIGH)** |
| **07** | `23-code-library/Drivers/adc_g4.c` L14–211 & `adc_g4.h` L36–47 | `CKMODE=11b` (`42.5/40 MHz <= 60 MHz`), `DEEPPWD->ADVREGEN->25us->ADCAL->ADRDY->ADEN`, `TS_CAL1`/`TS_CAL2`/`VREFINT_CAL` (`0x1FFF75A8`/`CA`/`AA`, 30/130 °C @ 3.0 V); shared `CCR` write & uncleaned `DMAEN` | DS12288 §3.14 & §5.3.19, RM0440 §21.4.6 & §21.7.17 | **FAIL (MEDIUM)** |
| **08** | `23-code-library/Drivers/dma_g4.c` L9–243 | Explicit `DMA1/2_Channel1..8` pointer tables (0x14 stride safe), `DMAMUX1EN` + readback, individual `CTCIF`/`CHTIF`/`CTEIF` (never `CGIF`), alignment check, `__DSB()` | ES0430 §2.3.1, RM0440 Ch. 11–12, `stm32g474xx.h` (`_Static_assert`) | **PASS** |
| **09** | `23-code-library/Drivers/iwdg_g4.c` L9–99 | `DBGMCU->APB1FZR1.DBG_IWDG_STOP`, `LSION`/`LSIRDY`, `0xCCCC`/`0x5555`/`PVU|RVU`/`0xAAAA`, `RCC_CSR_RMVF` clear | RM0440 Ch. 35, `stm32g474xx.h` | **PASS** |
| **10** | `23-code-library/Drivers/gpio_g4.c` L9–138 | `GPIO_EnableClock` A–G readback, `MODER` written last, `GPIO_LockPin()` 4-step `LCKR` sequence false-positive on 2nd pin | RM0440 §9.4.8, `stm32g474xx.h` | **FAIL (MEDIUM)** |
| **11** | `23-code-library/Drivers/fault_handler.c` + `ring_buffer.c` + `startup_stm32g474re.c` | Part 4 `[CRIT-01]` & `[MED-02]` fixes: `Fault_Trampoline` for all 4 fault vectors, SRAM/CCM SP bounds check, C11 `<stdatomic.h>` acquire/release, `Default_Handler` `C_DEBUGEN` check; unguarded `bkpt #0` at L393 | `core_cm4.h`, PM0214, ISO C11 §7.17, `arm-none-eabi-gcc -fanalyzer` | **PASS (1 LOW nit)** |
| **12** | `23-code-library/freertos/` (`Inc/FreeRTOSConfig.h`, `Src/rtos_bsp.c`, `Src/main.c`, `FreeRTOS-Kernel/VERSION.txt`) | Pinned `V11.1.0` (`dbf70559`), `PRIGROUP=0`, `MAX_SYSCALL=5` (`0x50`), `TIM2=6`/`EXTI13=7`, static stacks (`192..512` words), real debug/release build; `TIM2->SR &= ~UIF` RMW (`main.c` L229) & `RMVF` (`rtos_bsp.c` L132) | `core_cm4.h`, FreeRTOS `ARM_CM4F` port, RM0440, `make clean all` | **FAIL (MEDIUM nit)** |
| **13** | `19-projects/01-beginner-smart-instrument-panel/Src/main.c` | `GPIO_EnableClock(GPIOA/C)` fix, `PA0`/`PA1` conflict-free, Active-HIGH `PC13`, SPSC RX IRQ, `IWDG`, `170`/`160 MHz` compile | `stm32g474xx.h`, UM2505, `make clean all` | **PASS** |
| **14** | `19-projects/02-intermediate-dma-data-logger/Src/main.c` | `GPIO_EnableClock(GPIOA/B)` fix, `TIM3` 100 Hz TRGO (`MMS=010`), `ADC1` `EXTSEL=4`, `DMA1_Ch1` `DMAMUX=5`, ES0430 flag clears, HW `CRC-32`, read-only `OPTR.DBANK`, `PA15`/`PB7` OLED probe | RM0440 Table 163, ES0430 §2.3.1, `make clean all` | **PASS** |
| **15** | `19-projects/03-advanced-freertos-digital-power-controller/Src/main.c` & `Inc/power_ctrl.h` | `GPIO_EnableClock(A/B/C)` fix, `TIM1` 50 kHz center-aligned (`ARR=1700`, `DTG=85` = 500 ns), `DAC1` `HFSEL=10b` + `COMP1` -> `TIM1_AF1_BKCMP1E`, `AOE=0`, `POWER_SIM_MODE=0` control loop behavior | RM0440 Ch. 22, 24, 28, `stm32g474xx.h`, `make POWER_SIM_MODE=0` | **FAIL (HIGH)** |
| **16** | `HARDWARE_VALIDATION/onboard_selftest/Src/main.c` L1–981 | All 9 onboard tests (`MCU ID`, `GPIO`, `160`/`170 MHz`, `TIM2`/`EXTI13`, `DMA`/`ADC1`, `CORDIC`/`FMAC`, `FDCAN1`/`CRC-32`, `MPU`/`DIV0` `.noinit`, `IWDG`); Test 8A never triggers real MPU violation | `stm32g474xx.h`, `core_cm4.h`, RM0440 | **FAIL (MEDIUM)** |
| **17** | All 12 Newly Resolved Concept Sections (`C-LIB-001` .. `HWSW-ANA-001`) | `27` CMSIS `_Static_assert`s (`DCR=0x3DC`, `DMAR=0x3E0`, `TDCR=0x48`, `SYSCFG_CFGR1_*_FMP`, `DMAMUX TIM1_UP=46`, `FDCAN TXEF=+0x260`, `USB` 16-bit PMA) + direct compile of all `24` C11 code blocks | `stm32g474xx.h`, `core_cm4.h`, RM0440 Table 91, `arm-none-eabi-gcc 14.3.1 -Werror` | **PASS (24/24)** |
| **18** | `AUDIT/HARDWARE_VALIDATION_SUMMARY.md`, `PENDING_EXTERNAL_CONNECTIONS.md`, `PRACTICAL_IMPLEMENTATION_COVERAGE.md` | External hardware honesty (`Labs 07, 09, 11, 12, 14, 16`, OLED/pot/power-stage) & zero option-byte mutations; minor drift vs `lab-19/README.md` L308 | Cross-file audit against source & logs | **PASS (1 MED drift)** |
| **19** | `AUDIT/concepts_sot.json` v2.0.0 (`350` concepts, `267 COMPLETE`, `140 ONBOARD_HARDWARE_TESTED`) | Programmatic DAG check (`0` cycles, `0` unknown, `0` inverted-level edges) + grep audit of all `140` `ONBOARD_HARDWARE_TESTED` & `70` promotions against actual 6 flashed binaries | Independent Python AST/DAG/grep verification | **FAIL (HIGH)** |
| **20** | `D:\General_ant\embedded-c-master-learning-hub-backup` vs `BACKUP/BACKUP_MANIFEST.md` | File count (`565`), total size (`5,734,859` B), aggregate SHA-256 (`84aa5ef4...015cb5`), `git rev-parse HEAD` (`585dc4fa...`), clean `git status` | Independent Python walk & SHA-256 recomputation | **PASS (100% MATCH)** |
| **21** | Full Working Repo Secret & PII Scan (excl. `.git/`) | Scan for `ghp_`, `github_pat_`, `gho_`, `ghs_`, `AKIA`, `sk-`, PEM keys, passwords, `C:\Users\Guhapriyan`, `D:\General_ant` | Independent Python full-tree regex scan | **PASS (0 secrets/PII; 1 LOW path)** |
| **22** | `.github/workflows/firmware-ci.yml`, `.clang-format`, `.clang-tidy`, `Doxyfile`, `.gitignore`, `git status` | Local execution (`validate_repo.py` 16/16, `make -C tests test/analyze` 574/574, ARM `-fanalyzer` 0 warn, 5 firmware builds PASS) vs Ubuntu CI (`clang-format --dry-run --Werror`, `lcov 2.0`, `cppcheck`) | Direct CLI execution + `.clang-format` static violation audit + LCOV 2.0 docs | **FAIL (HIGH)** |

---

## II.3 Detailed Adversarial Findings & Concrete Remediation Actions

### HIGH FINDINGS (Mandatory Before Final Commit & Push)

#### [P8-HIGH-01] Guaranteed GitHub Actions CI Failure (`clang-format --dry-run --Werror` & `lcov 2.0` on `ubuntu-latest`)
- **Files & Lines:**
  - `.github/workflows/firmware-ci.yml` L51–52, L153–171, L190–199
  - `.clang-format` L8, L14–15
  - `23-code-library/Drivers/*.c`, `23-code-library/Drivers/*.h`, `23-code-library/templates/Src/main.c`, `23-code-library/templates/Src/startup_stm32g474re.c`
- **Verified Evidence:**
  1. `clang-format`, `clang-tidy`, `cppcheck`, and `doxygen` are not installed on the local workstation (`AUDIT/TEST_COVERAGE_REPORT.md` L116–119), so the `static-analysis-and-format` and `doxygen-docs` CI jobs were never executed locally.
  2. `.clang-format` enforces `ColumnLimit: 100`, `AlignConsecutiveAssignments: false`, `AlignConsecutiveDeclarations: false`, and `BasedOnStyle: LLVM` (where `AllowShortCaseLabelsOnASingleLine` defaults to `false`). Independent static inspection of just `23-code-library/Drivers/*.{c,h}` and `23-code-library/templates/Src/{main,startup_stm32g474re}.c` measured:
     - **317** multi-space aligned `=` assignments (e.g., `exti_g4.c` L13–14, L61–63; `clock_g4.c` L239–241; `adc_g4.c` L33–37; `dma_g4.c` L25–32),
     - **102** code lines exceeding `100` columns (e.g., `startup_stm32g474re.c` L127–228),
     - **12** single-line `case X: stmt` statements (e.g., `exti_g4.c` L50–55, `tim_g4.c` L175–179, `dma_g4.c` L77–80).
     Therefore, `.github/workflows/firmware-ci.yml` L192 (`clang-format --dry-run --Werror`) is **100% guaranteed to fail** on the first GitHub Actions run!
  3. `.github/workflows/firmware-ci.yml` L51–52 passes `--rc lcov_branch_coverage=1` to `lcov`. On `ubuntu-latest` (Ubuntu 24.04), `apt-get install lcov` installs **LCOV 2.0+**, where `lcov_branch_coverage` is deprecated/errored (replaced by `--rc branch_coverage=1`) and GCC 13/14 branch data triggers fatal consistency errors unless `--ignore-errors inconsistent,inconsistent` (or `--ignore-errors unused,inconsistent`) is supplied.
  4. `.github/workflows/firmware-ci.yml` L153 runs `cppcheck --enable=warning,style,performance,portability --error-exitcode=1` on `tim_g4.c` — whose lines 18–44 and 63 were already proven mathematically unreachable in `AUDIT/TEST_COVERAGE_REPORT.md` L61–69 (prime trigger for `cppcheck` `knownConditionTrueFalse` style errors with `--error-exitcode=1`).
- **Concrete Remediation:**
  1. Either run `clang-format -i` across every file listed in `firmware-ci.yml` L193–199 (or align `.clang-format` rules AND remove `--Werror` / set `continue-on-error: true` on untested local tools until verified on an actual Ubuntu runner!).
  2. Update `firmware-ci.yml` L51–52 from `--rc lcov_branch_coverage=1` to `--rc branch_coverage=1 --ignore-errors inconsistent` (or guard for LCOV 1 vs 2).
  3. Update `firmware-ci.yml` L153 `cppcheck` to restrict `--error-exitcode=1` to `--enable=warning,portability` (or remove the unreachable dead clamps in `tim_g4.c` L18–44 / L63) and verify the first GitHub Actions run passes green.

---

#### [P8-HIGH-02] Capstone Project 03 `POWER_SIM_MODE=0` ("EXTERNAL HARDWARE POWER STAGE") Runs Open-Loop Against Software Plant
- **Files & Lines:**
  - `19-projects/03-advanced-freertos-digital-power-controller/Src/main.c` L193–200, L212–216, L355–414, L553–557
  - `AUDIT/CAPSTONE_IMPLEMENTATION_REPORT.md` L16
  - `HARDWARE_VALIDATION/PENDING_EXTERNAL_CONNECTIONS.md` L35
- **Verified Defect:**
  1. `19-projects/03/Src/main.c` L556 prints `"Mode : EXTERNAL HARDWARE POWER STAGE"` when built with `POWER_SIM_MODE=0`, and `PENDING_EXTERNAL_CONNECTIONS.md` L35 instructs learners to rebuild with `make POWER_SIM_MODE=0` to bring up a physical `<= 24 V` buck/boost power stage.
  2. However, `POWER_SIM_MODE` is ONLY checked in `pwr_dac1_comp1_init()` (L193, L212) and the boot banner (L553). Inside `vControlLoopTask()` (L355–414), there is **zero `#if (POWER_SIM_MODE != 0U)` check and zero ADC initialization/sampling** — L383–391 unconditionally compute `err_mv = v_ref - plant.v_out_mv` and advance `buck_sim_step(&plant, duty_pm, !faulted)`!
  3. Consequently, connecting a physical power stage under `POWER_SIM_MODE=0` drives `TIM1->CCR1` completely **open-loop against an internal software simulation** while ignoring real output voltage feedback — an unacceptable control/safety hazard for power electronics firmware.
- **Concrete Remediation:**
  1. In `19-projects/03/Src/main.c`, implement a real `#else` ADC voltage-sense feedback path for `POWER_SIM_MODE == 0` (using `adc_g4.h` `adc_init_single_ended(ADC1)` + `adc_read_channel()` scaled by external resistor-divider ratio, with over-voltage trip guard) **OR** fail compilation at `#if (POWER_SIM_MODE == 0U)` with `#error "External ADC feedback channel & voltage-divider scaling must be wired and configured before energizing a physical power stage"` and document that explicitly in `CAPSTONE_IMPLEMENTATION_REPORT.md` L16 and `PENDING_EXTERNAL_CONNECTIONS.md` L35.

---

#### [P8-HIGH-03] `i2c_g4.h` `TIMINGR` Constants Overclock I2C Bus & Violate `t_SCLDEL` + Contradict `PB8-BOOT0` Safety Fix
- **Files & Lines:**
  - `23-code-library/Drivers/i2c_g4.h` L16–18, L62–72
  - `AUDIT/DRIVER_LIBRARY_VERIFICATION.md` L45–48
  - Compare against correctly derived `18-practical-laboratory/lab-12-i2c-master-ssd1306-oled/README.md` L56–81
- **Verified Defect (Exact RM0440 §39.4.9 + NXP UM10204 Math):**
  Using RM0440 §39.4.9 ($t_{\text{PRESC}} = (\text{PRESC}+1)t_{\text{I2CCLK}}$, $t_{\text{SCL}} = t_{\text{SCLL}} + t_{\text{SCLH}} + t_{\text{SYNC1}} + t_{\text{SYNC2}}$, $t_{\text{SYNC1+2(min)}} \ge 4 t_{\text{I2CCLK}} + 2 t_{\text{AF(min)}}$ with $t_{\text{AF(min)}} = 50\text{ ns}$):

  | Macro in `i2c_g4.h` | Value | $t_{\text{SCLL}}$ / $t_{\text{SCLH}}$ | $t_{\text{SCLDEL}}$ | $f_{\text{SCL(max)}}$ (fast edges) | Spec Violation |
  |---|---|---|---|---|---|
  | `I2C_TIMINGR_170MHZ_400KHZ` | `0x30B21F3B` | `1412` / `753 ns` | `282.4 ns` | **`437.0 kHz`** (typ `411.8 kHz`) | **Exceeds `400 kHz` FM max**; `t_SCLDEL < 400 ns` (`t_r(max)+t_SU;DAT`) |
  | `I2C_TIMINGR_160MHZ_400KHZ` | `0x30B21D38` | `1425` / `750 ns` | `300.0 ns` | **`434.8 kHz`** (typ `409.8 kHz`) | **Exceeds `400 kHz` FM max**; `t_SCLDEL < 400 ns` |
  | `I2C_TIMINGR_16MHZ_100KHZ` | `0x00423F4F` | `5000` / `4000 ns` | **`312.5 ns`** | **`107.0 kHz`** (typ `103.4 kHz`) | **Exceeds `100 kHz` SM max**; `t_SCLDEL` leaves only **`62.5 ns`** max $t_r$ (`< 250 ns` typ / `1000 ns` UM10204 max!) |
  | `I2C_TIMINGR_170MHZ_100KHZ` | `0x70C45D6C` | `5129` / `4424 ns` | `611.8 ns` | `103.3 kHz` (typ `100.0 kHz`) | Marginally > `100 kHz` on low-C bus (`t_r` max `362 ns` vs `1000 ns` UM10204) |

  Furthermore, `i2c_g4.h` L16–18 notes `PB8` is `PB8-BOOT0` with `R36 = 10 kΩ` and immediately states `"Always use external 2.2..4.7 kOhm pull-ups on SCL/SDA (or route I2C1 to PA15/PB7)"` — omitting the critical warning from `Lab 12` L40 that placing a permanent `2.2 kΩ` pull-up on `PB8` drives `2.705 V` at reset and locks the board into ST System ROM Bootloader unless `nSWBOOT0 = 0`!
- **Concrete Remediation:**
  1. Replace `I2C_TIMINGR_170MHZ_400KHZ` with Lab 12's verified **`0x70811021UL`** (`f_max = 396.3 kHz <= 400 kHz`, `t_SCLDEL = 423.5 ns >= 400 ns`) and `I2C_TIMINGR_160MHZ_400KHZ` with **`0x70710F1FUL`** (`f_max = 396.0 kHz <= 400 kHz`, `t_SCLDEL = 400.0 ns`).
  2. Replace `I2C_TIMINGR_16MHZ_100KHZ` with e.g. `PRESC=3` (`t_PRESC=250 ns`), `SCLDEL=4` (`t_SCLDEL=1250 ns >= 1250 ns` full UM10204 worst-case!), `SDADEL=2` (`500 ns`), `SCLH=0x13` (`5000 ns`), `SCLL=0x17` (`6000 ns`, `f_max = 88.1 kHz <= 100 kHz`) = **`0x30421317UL`** (or `SCLH=0x10`, `SCLL=0x15` -> `f_max = 98.5 kHz`, **`0x30421015UL`**); and bump `170MHz_100kHz` / `160MHz_100kHz` `PRESC`/`SCLL`/`SCLH` slightly so `f_max <= 100.0 kHz` even on low-capacitance buses.
  3. Update `tests/unit/test_drivers_math.c` to assert BOTH $f_{\text{SCL(max)}} \le f_{\text{target}}$ AND $t_{\text{SCLDEL}} \ge t_{r(\max)} + t_{\text{SU;DAT}}$, and fix `i2c_g4.h` L16–18 to warn explicitly against permanent pull-ups on `PB8-BOOT0` unless `nSWBOOT0 = 0`.

---

#### [P8-HIGH-04] `ONBOARD_HARDWARE_TESTED` (`140`) & `COMPLETE` (`267`) Over-Promotion in `AUDIT/concepts_sot.json` v2.0.0
- **Files & Lines:**
  - `AUDIT/concepts_sot.json` (`v2.0.0`)
  - `AUDIT/UPDATED_CONCEPT_COVERAGE_MATRIX.md` L25, L36
  - `AUDIT/PARTIAL_CONCEPT_COMPLETION_REPORT.md` L42, L65, L75, L77
- **Verified Evidence (Direct Grep Across All 6 Flashed Firmware Binaries):**
  While `concepts_sot.json` v2.0.0 fixed all DAG defects (`0` cycles, `0` unknown prereqs, `0` inverted-level edges) and kept external labs honestly `EXTERNAL_HARDWARE_PENDING`, **at least 16–20 concepts marked `ONBOARD_HARDWARE_TESTED` were never physically exercised or measured on silicon**:
  1. **Zero code occurrences (`0` hits) across all 6 flashed images** (`onboard_selftest`, `freertos`, `Project 01..03`, `templates`):
     - `ARM-PWR-001` (`WFI`, `WFE`, `SEV`, `SLEEPONEXIT`, `SLEEPDEEP`) — `0` hits; `Lab 14` is `EXTERNAL_HARDWARE_PENDING`!
     - `PERIPH-TIM-001` (`Basic Timers TIM6, TIM7`) — `0` hits!
     - `STM32-RCC-003` (`CSS HSE/LSE, MCO, CCIPR, Reset Flags`) — `0` hits for `HSEON`/`CSSON`/`LSEON`/`MCOSEL` (`CLK-MCO-001` is `EXTERNAL_HARDWARE_PENDING`)!
     - `TOOL-DBG-003` (`SWO/ITM`) — `0` hits for `ITM->`/`TPI->` (only `DWT->CYCCNT` was tested)!
     - `ARM-SEC-001` (`Cortex-M4 & STM32G474RE Security Architecture`) — `0` hits for `nPRIV`/RDP/PCROP/SEC!
     - `PERIPH-UART-004` (`USART Error Flags ORE/FE/NE/PE`) — `0` error-flag fault injections executed on silicon (only normal TX/RX ran)!
  2. **Unexercised / Unmeasured on Hardware:**
     - `ARM-MPU-001`: `onboard_selftest` Test 8A arms Region 7 (`g_mpu_guard_zone`) but performs only a benign read outside it (`g_mpu_valid_var`) and triggers `DIV0` `UsageFault`, never an actual MPU `MemManage` violation!
     - `ARM-MEM-002` (Bit-banding is physically absent on STM32G474RE — should be `CROSS_COMPILED` or `NOT_APPLICABLE`, not `ONBOARD_HARDWARE_TESTED`).
     - `STM32-BOOT-001` (`PB8-BOOT0`, `nSWBOOT0`/`nBOOT0` — boot modes and option bytes were strictly never altered or tested).
     - `PERIPH-GPIO-004` (Switch debouncing — `B1 Presses: 0` in Project 01 and `btn=0` in FreeRTOS; `B1` was never physically pressed during unattended automation).
     - `PERIPH-BDTR-001`, `PERIPH-PWM-001`, `G4-COMP-001`, `PERIPH-DAC-001` — initialized in registers (`Project 01`/`03`), yet NO PWM/dead-time waveform (`PA1`/`PA8`/`PB13`), DAC analog voltage (`PA4`), or COMP1 overcurrent trip (`brk:0` in Project 03!) was ever triggered or measured (inconsistent with `PERIPH-BDTR-002`, `PERIPH-PWM-002`, `Lab 07`, `Lab 16` being `EXTERNAL_HARDWARE_PENDING`).
     - `HWSW-ANA-001` (Analog PCB ferrite bead / MLCC ESR-ESL / return-path design — reading `VREFINT = 3288 mV` on ST's factory Nucleo PCB verifies `ADC1`/`VREFINT`, not custom analog PCB design).
  3. **Premature `COMPLETE` Promotions on Untested CI Tools:**
     - `PROF-SA-002` (`.clang-tidy` & `.clang-format`) and `PROF-DOC-001` (`Doxyfile`) were promoted `PARTIAL -> COMPLETE` even though neither `clang-format`, `clang-tidy`, nor `doxygen` was executed locally, and `.clang-format` fails with >430 formatting diffs (`[P8-HIGH-01]`).
- **Concrete Remediation:**
  1. Reclassify the ~16–20 unexercised concepts listed above from `ONBOARD_HARDWARE_TESTED` to their honest `hardware_status` (`CROSS_COMPILED`, `EXTERNAL_HARDWARE_PENDING`, or `NEEDS_BENCH_INSTRUMENT`), bringing `ONBOARD_HARDWARE_TESTED` to an airtight **`~120–124`**.
  2. Hold `PROF-SA-002` and `PROF-DOC-001` at `PARTIAL` until GitHub Actions CI passes green (or locally verify them), and regenerate `AUDIT/UPDATED_CONCEPT_COVERAGE_MATRIX.md`, `AUDIT/PARTIAL_CONCEPT_COMPLETION_REPORT.md`, and `AUDIT/FINAL_COMPLETENESS_REPORT.md` deterministically from `concepts_sot.json`.

---

### MEDIUM FINDINGS

#### [P8-MED-01] `GPIO_LockPin()` Returns False-Positive `true` on Second Pin Lock
- **File & Lines:** `23-code-library/Drivers/gpio_g4.c` L124–138
- **Verified Defect:** Per RM0440 §9.4.8, once `LCKK` (`bit 16`) is latched `1` on a port, all subsequent writes to `LCKR` are ignored until reset while `LCKK` continues to read `1`. Checking only `(port->LCKR & GPIO_LCKR_LCKK) != 0U` causes a second call `GPIO_LockPin(GPIOA, 6)` (after `GPIO_LockPin(GPIOA, 5)`) to return **`true`** even though `LCK6` remains `0` (unlocked)!
- **Concrete Remediation:** Change L137 to `return ((port->LCKR & (GPIO_LCKR_LCKK | mask)) == (GPIO_LCKR_LCKK | mask));` and add `GPIO_LockPins(GPIO_TypeDef *port, uint16_t pin_mask)` so multiple pins on a port can be locked atomically in one sequence.

#### [P8-MED-02] Subtle Edge-Case & RM0440 Compliance Gaps Across 5 New Drivers
- **Files & Lines:**
  - `23-code-library/Drivers/tim_g4.c` L57: `return (arr == 0xFFFFU) ? 0xFFFFU : (arr + 1U);` wraps `uint32_t` to **`0U` (0% duty)** when `arr == 0xFFFFFFFFU` on 32-bit `TIM2` (fix: `if ((arr == 0xFFFFU) || (arr == 0xFFFFFFFFU)) return arr;`).
  - `23-code-library/Drivers/usart_g4.c` L35–45 & `i2c_g4.c` L40–43: Query only `PCLK1`/`PCLK2` without checking `RCC->CCIPR` (`USART1SEL`/`USART2SEL`/`LPUART1SEL`/`I2C1..3SEL`), silently producing wrong `BRR`/`TIMINGR` if the caller selected `HSI16`/`SYSCLK`/`LSE`.
  - `23-code-library/Drivers/adc_g4.c` L70–71 & L102: Modifies shared `ADC12_COMMON->CCR.CKMODE` during `adc_init_single_ended(ADC2, ...)` without checking `ADC1->CR.ADEN == 0` (RM0440 §21.7.17 requires both ADCs disabled to write `CKMODE`), and L102 does not clear `ADC_CFGR_DMAEN | ADC_CFGR_DMACFG` on re-init.
  - `23-code-library/Drivers/i2c_g4.c` L81–90: On `NACKF`, clears `NACKCF`/`STOPCF` without setting `i2c->ISR = I2C_ISR_TXE` (RM0440 §39.7.7) to flush any unshifted byte remaining in `TXDR`.
  - `23-code-library/Drivers/spi_g4.c` L15: Floor division `(pclk_hz / div) <= max_sck_hz` can exceed `max_sck_hz` slightly when `pclk_hz % div != 0` (fix: `(uint64_t)pclk_hz <= (uint64_t)max_sck_hz * div`).

#### [P8-MED-03] `onboard_selftest` Test 8A Never Triggers an Actual MPU Violation & Overrides `UsageFault_Handler`
- **File & Lines:** `HARDWARE_VALIDATION/onboard_selftest/Src/main.c` L161–231, L810–857
- **Verified Defect:** Test 8A configures MPU Region 7 (`g_mpu_guard_zone[32]`, `AP=0`, `XN=1`) and verifies a benign read of `g_mpu_valid_var` outside the guard zone, then triggers `sdiv 42/0` (`UsageFault`). It never accesses `g_mpu_guard_zone[0]` to prove an actual `MemManage` (`DACCVIOL | MMARVALID`) MPU trap on silicon. Also, overriding `UsageFault_Handler` with `Selftest_UsageFault_C()` bypassed `startup_stm32g474re.c`'s `Fault_Trampoline` and `fault_handler.c`'s `HardFault_Handler_C()` during the final 9/9 automated run (though `HardFault_Handler_C()` was verified separately via SWD SRAM readback during initial bring-up).
- **Concrete Remediation:** Document this boundary clearly in `HARDWARE_VALIDATION_SUMMARY.md` (or add a Phase-1B `MemManage` write to `g_mpu_guard_zone[0]` verifying `CFSR & 0x82 == 0x82` and `MMFAR == &g_mpu_guard_zone[0]`).

#### [P8-MED-04] Cross-Document Lab Status Drift & `rc_w0` RMW Anti-Pattern in FreeRTOS Reference Firmware
- **Files & Lines:**
  - `18-practical-laboratory/lab-19-hardfault-mpu-and-crash-diagnostics/README.md` L308 (vs `AUDIT/PRACTICAL_IMPLEMENTATION_COVERAGE.md` L49)
  - `23-code-library/freertos/Src/main.c` L229 (`TIM2->SR &= ~TIM_SR_UIF;`)
  - `23-code-library/freertos/Src/rtos_bsp.c` L132 (`RCC->CSR |= RCC_CSR_RMVF;`)
- **Verified Defect:** Individual lab READMEs (e.g., `lab-19` L308) still read `CROSS-COMPILED (Queued for Physical Execution)` while `PRACTICAL_IMPLEMENTATION_COVERAGE.md` marks them `ONBOARD HARDWARE TESTED (via onboard_selftest)`; `freertos/Src/main.c` L229 uses RMW `&= ~` on an `rc_w0` timer status register (contradicting `EMB-REG-003` — fix to `TIM2->SR = ~TIM_SR_UIF;`); and `rtos_bsp.c` L132 omits clearing `RMVF` back to `0` after readback.

#### [P8-MED-05] Implementation-Defined Signed Right-Shift in Project 03 Q15 PI Controller
- **File & Lines:** `19-projects/03-advanced-freertos-digital-power-controller/Inc/power_ctrl.h` L123, L137
- **Verified Defect:** `(pi->kp_q15 * err_scaled) >> Q15_SHIFT` and `cand_integ >> Q15_SHIFT` right-shift potentially negative `int32_t` values (ISO C11 §6.5.7/5 implementation-defined; MISRA C:2012 Rule 10.1). Replace with `/ Q15_ONE` (which GCC optimizes cleanly or documents explicit arithmetic-shift assumption).

---

### LOW FINDINGS

- **`[P8-LOW-01]` `.gitignore` Missing Coverage Artifact Patterns (`.gitignore` L1–18):** Add `*.gcda`, `*.gcno`, `*.gcov`, `*.info` so manual `gcov` runs outside `build/` never pollute `git status`.
- **`[P8-LOW-02]` Unguarded `bkpt #0` After `main()` Return (`23-code-library/templates/Src/startup_stm32g474re.c` L393):** Guard with `COREDEBUG_DHCSR_ADDR & COREDEBUG_DHCSR_DEBUGEN` (matching `Default_Handler` L62) or call `Default_Handler()`.
- **`[P8-LOW-03]` Local Workspace Path in Learner Setup Guide (`LEARNERS/BEGINNER_SETUP_WINDOWS11.md` + 6 internal audit/backup docs):** Zero PII/usernames (`0` hits for `Guhapriyan`), zero secrets; replace `D:\General_ant\...` in `LEARNERS/BEGINNER_SETUP_WINDOWS11.md` with a generic path placeholder (e.g., `C:\workspace\embedded-c-master-learning-hub`).

---

## II.4 Backup Integrity Proof, Secret Scan Proof & Build/CI Hygiene Proof

### 1. Local Baseline Backup Integrity (`D:\General_ant\embedded-c-master-learning-hub-backup`) — **VERIFIED IMMUTABLE (PASS)**
Independent recursive walk and SHA-256 recomputation against `BACKUP/BACKUP_MANIFEST.md`:
- **Total Files:** `565` (Expected: `565` — **EXACT MATCH**)
- **Total Bytes:** `5,734,859` bytes (Expected: `5,734,859` — **EXACT MATCH**)
- **Aggregate Manifest SHA-256** (`<sha256>  <win_relpath>\r\n`, sorted):  
  `84aa5ef482853df5f0af1e4c401bd6a3104fca9fe9e182def4ca6530ab015cb5` (**EXACT MATCH**)
- **Backup `git rev-parse HEAD`:** `585dc4fa081cf7f27e5e437254e814f13f4b46cd` (**EXACT MATCH**)
- **Backup `git status --short`:** Empty (`0` modified / `0` untracked — **NEVER MODIFIED OR OVERWRITTEN**)

### 2. Full-Repository Secret & Sensitive Information Scan — **CLEAN (PASS)**
Scanned all non-binary files in `D:\General_ant\embedded-c-master-learning-hub` (excluding `.git/`):
- GitHub PATs (`ghp_*`, `github_pat_*`, `gho_*`, `ghs_*`): **`0` matches**
- Cloud/API keys (`AKIA*`, `sk-*`) & Private Keys (`-----BEGIN ... PRIVATE KEY-----`): **`0` matches**
- Hardcoded credential assignments (`password=`, `api_key=`, `secret_key=`, `access_token=`): **`0` matches**
- Private OS username paths (`C:\Users\Guhapriyan...` / `Guhapriyan` substring): **`0` matches**
- Generic non-PII local drive path (`General_ant`): `7` Markdown files (`6` internal audit/backup/tracker docs + `LEARNERS/BEGINNER_SETUP_WINDOWS11.md` — flagged as `[P8-LOW-03]`).

### 3. Build & Git Artifact Hygiene — **CLEAN WORKING TREE (PASS) / CI GATES NEED FIXES (`[P8-HIGH-01]`)**
- `git status --short -uall`: **`0` untracked build artifacts** (`*.elf`, `*.o`, `*.obj`, `*.exe`, `*.gcda`, `*.gcno`, `*.gcov`, `*.bin`, `*.hex`, `*.map`, `*.d`, `*.su`, `*.info`). All 7 `build/` directories on disk are cleanly excluded by `.gitignore`.
- Local builds empirically verified `PASS` (`0` warnings / `0` errors): `scripts/validate_repo.py --skip-builds` (`16/16 PASS`), `make -C tests test` (`574/574 PASS`), `make -C tests analyze` (`0` warnings), `arm-none-eabi-gcc -fanalyzer` (`0` warnings), `23-code-library/templates` (`debug` & `release`), `23-code-library/freertos` (`debug`), and `19-projects/01..03` (`debug`, including `POWER_SIM_MODE=0` & `1`).

---

## II.5 Mandatory Pre-Commit Sign-Off Checklist (Phase 8 Gate)

Resolve the **4 HIGH** items below before final commit and push to GitHub:
1. **`[P8-HIGH-01]`** Fix `.github/workflows/firmware-ci.yml` (`lcov --rc branch_coverage=1 --ignore-errors inconsistent`, `cppcheck` style-vs-error gate, and reconcile `.clang-format` / formatting across targeted `.c`/`.h` files so CI passes green on first push).
2. **`[P8-HIGH-02]`** Guard or implement real ADC feedback for `POWER_SIM_MODE == 0` in `19-projects/03-advanced-freertos-digital-power-controller/Src/main.c` L383–391 (never run open-loop software plant while reporting `EXTERNAL HARDWARE POWER STAGE`).
3. **`[P8-HIGH-03]`** Fix `23-code-library/Drivers/i2c_g4.h` `I2C_TIMINGR_*` constants (`0x70811021`, `0x70710F1F`, `0x30421015`, etc.), add max-frequency & `t_SCLDEL` assertions to `tests/unit/test_drivers_math.c`, and correct the `PB8-BOOT0` pull-up warning in `i2c_g4.h` L16–18.
4. **`[P8-HIGH-04]`** Recalibrate the ~16–20 over-promoted `ONBOARD_HARDWARE_TESTED` concepts and `PROF-SA-002`/`PROF-DOC-001` `COMPLETE` flags in `AUDIT/concepts_sot.json` v2.0.0 and regenerate dependent summary tables.


---
---

# Part III: Final Post-Remediation Independent Red-Team Audit (Agent 12 — 2026-11-18)

> **Historical Preservation Notice:** Parts I (Lines 1–321) and II (Lines 326–561) above are preserved verbatim. Part III below is an independent, adversarial post-remediation challenge of all Wave 1–2 deliverables from Specialists 1–4 and Worker C2 on branch `feature/complete-remediation`.

**Auditor Role:** Agent 12 — Independent Technical Red-Team Auditor (Final Post-Remediation Challenge)  
**Audit Date:** `2026-11-18`  
**Target Repository / Branch:** `D:\General_ant\embedded-c-master-learning-hub` (`feature/complete-remediation`)  
**Primary Ground-Truth Authorities Used:**
- Vendored ST CMSIS `23-code-library/Drivers/CMSIS/Device/ST/STM32G4xx/Include/stm32g474xx.h` & ARM CMSIS `core_cm4.h`
- Official ST `STMicroelectronics/stm32g4xx-hal-driver` API headers (`stm32g4xx_ll_bus.h`, `stm32g4xx_ll_gpio.h`, `stm32g4xx_ll_system.h`, `stm32g4xx_ll_exti.h`, `stm32g4xx_ll_usart.h`, `stm32g4xx_hal_gpio.h`, `stm32g4xx_hal_cortex.h`, `stm32g4xx_hal_uart.h`)
- **ARM AN321 (`DAI0321A`) §4.9**, **ARM Cortex-M4 Erratum 838869 / ST ES0430 §2.1.3**, **ST RM0440 §39.4.9 / §21.4.8 / Ch. 37**, **ST UM2505 §6.3–§6.4**, **ST PM0214 §2.3.7**
- Empirical execution: live `python scripts/validate_repo.py` (full build + ELF + `594`-assertion host test run), 7 isolated `tempfile.TemporaryDirectory()` adversarial fault-injection mutations against `scripts/validate_repo.py` v4.0.0, independent 130-file / 1,786-span strict GitHub MathJax scanner, and programmatic `AUDIT/concepts_sot.json` + `AUDIT/TECHNICAL_DEFECTS.md` integrity checks.

---

## III.1 Executive Verdict

**FINAL POST-REMEDIATION RED-TEAM VERDICT: CONDITIONAL HOLD — `0 CRITICAL`, `3 HIGH`, `4 MEDIUM`, `3 LOW`.**

### What Independently Passed Adversarial Challenge
1. **Firmware Builds, ELF Layout & Host Unit Tests (`PASS`):** Independent live execution of `python scripts/validate_repo.py` confirmed starter template (`debug` & `release`), ELF `.isr_vector` (`0x08000000`, `472 B`, `512 B` align), pinned `FreeRTOS-Kernel V11.1.0`, all `3/3` Capstones (`0` warnings / `0` errors), and **`594 / 594`** host C11 assertions across `4` suites (`0` failed, auto-cleaned afterward).
2. **Core Finding A Architecture & Implementation (`PASS` in Primary Targets):** `10-interrupts/README.md` §3.3/§3.6/§7/§8.2, `04-arm-cortex-m/README.md` §3.8, `09-timers/README.md` §3.2/§8.1, `exti-syscfg-register-map.md` §3–§4, `exti_g4.h` (`EXTI_ClearPending`), `tim_g4.h` (`TIM_ClearUpdateFlag`), `tim_g4.c` L155–156, `dma_g4.c` L124–126, `adc_g4.c` L99–103, and `freertos/Src/main.c` L229–261 accurately separate ARM AN321 §4.9 (`(void)REG;` AHB-to-APB bridge readback) from ARM Erratum 838869 / ST ES0430 §2.1.3 (`__DSB();` before exception return) and correctly clarify 6-cycle tail-chaining.
3. **STM32CubeG4 LL & HAL API Spellings Across Labs `01`, `05`, and `08` (`100% EXACT`):** Line-by-line audit against official `STMicroelectronics/stm32g4xx-hal-driver` verified every LL/HAL macro and function signature — including `LL_GPIO_SPEED_FREQ_LOW`/`HIGH`, `LL_SYSCFG_SetEXTISource(LL_SYSCFG_EXTI_PORTC, LL_SYSCFG_EXTI_LINE13)`, `LL_EXTI_IsActiveFlag_0_31`/`ClearFlag_0_31`/`EnableIT_0_31`, `LL_GPIO_TogglePin()` (confirmed atomic via `BSRR` in STM32CubeG4), 5-arg `LL_USART_SetBaudRate(..., LL_USART_PRESCALER_DIV1, LL_USART_OVERSAMPLING_16, baud)`, `LL_USART_IsActiveFlag_RXNE_RXFNE`/`TXE_TXFNF`, `HAL_GPIO_Init()` internal `__HAL_RCC_SYSCFG_CLK_ENABLE()`, and `HAL_NVIC_ClearPendingIRQ()`.
4. **Physical Hardware Testing Honesty (`PASS`):** Zero false physical-hardware claims across `LAB_VERIFICATION_MATRIX.md`, `HARDWARE_VERIFICATION_MATRIX.md`, `HARDWARE_VALIDATION_SUMMARY.md`, and all 20 labs; the `PENDING — USER CONNECTION REQUIRED` boundary (`6` external labs, `5` bench concepts, `0` Option-Byte mutations) is strictly maintained.
5. **Learner-Facing GitHub MathJax Remediation (`PASS` on Target User Docs):** `01-master-concept-map/learning-sequence.md` L69 (`f_{\text{PLL-IN}}`), `LEARNERS/FORMULA_AND_CALCULATION_SHEET.md`, all `17/17` curriculum stages, all `20/20` labs, and all `16/16` register maps contain **zero** `\text{...\_...}` / `\mathrm{...\_...}` / `\texttt{...\_...}` MathJax bugs.
6. **SoT DAG Mathematical Acyclicity (`PASS`):** Independent Kahn topological sort of `AUDIT/concepts_sot.json` v2.1.0 confirmed `350` unique IDs, `27/27` domains, `0` cycles, `0` self-loops, `0` unknown prerequisites, and `0` inverted difficulty-level edges.

### Severity Tally of Newly Uncovered Post-Remediation Defects

| Severity | Count | Finding IDs & Summary |
|---|---:|---|
| **CRITICAL** | **0** | None. |
| **HIGH** | **3** | `[R12-HIGH-01]` Stale spec-violating `I2C TIMINGR = 0x30612144` still published as `"Verified Lab 12"` in `LEARNERS/FORMULA_AND_CALCULATION_SHEET.md` L95; `[R12-HIGH-02]` Residual hardware-hazard `IDD drop on JP5` in `AUDIT/concepts_sot.json` L3295 (`ARM-PWR-001`) + **5 remaining GitHub MathJax/truncated-math defects** in `AUDIT/` (`MISSING_CONCEPTS_AND_GAPS.md` L315/316/400, `COMPLETE_CONCEPT_INVENTORY.md` L283, `TECHNICAL_VERIFICATION_REPORT.md` L25) + unmatched backticks/truncations; `[R12-HIGH-03]` **5 empirically proven blind spots** in `scripts/validate_repo.py` v4.0.0 (`M2` table bitfields, `M4` `#anchor` links, `M5` `\texttt` MathJax, `M6` unclosed `$` / bare `\_` in math, `M7` register-map/lab C-snippet macros). |
| **MEDIUM** | **4** | `[R12-MED-01]` `lab-08` Approach C (HAL) non-`volatile` `g_hal_rx_byte` & premature `HAL_UART_Receive_IT()` re-arm race condition (+ L525 overstatement); `[R12-MED-02]` Residual Finding A conflation in `learning-sequence.md` L76, `lab-05` L324/L70, `exti_g4.h` L7–8, and `tim_g4.h` L11–12; `[R12-MED-03]` `AUDIT/TECHNICAL_DEFECTS.md` L52 (`DEF-C-002`) unescaped `|` splits table row into **11 columns** instead of 9; `[R12-MED-04]` Stale pre-merge claims in `AUDIT/EDUCATIONAL_QUALITY_REPORT.md` L79–80/L130–133 + off-by-one stage numbering in `learning-sequence.md` (`Stage 07..15` vs `Stage 08..16`). |
| **LOW** | **3** | `[R12-LOW-01]` `exti-syscfg-register-map.md` L9 (`Compile-Verified`) vs L61 (`Statically Reviewed`) tag mismatch; `[R12-LOW-02]` `21-reference-library/official-st-documentation.md` L30 still cites `RTC (Ch 34)` instead of RM0440 **Ch 37**; `[R12-LOW-03]` Minor precision refinement on ARM AN321 §4.9 asynchronous clock-domain IRQ synchronizer latency in `10-interrupts/README.md` L85. |

---

## III.2 Independent Verification & Adversarial Mutation Matrix

| # | Challenge Area | Target Artifact(s) & Exact Lines | Verification Method / Primary Source | Result |
|---:|---|---|---|---|
| **01** | **Finding A** | `10-interrupts/README.md` L61, L79–94, L162–163, L269–272; `04-arm-cortex-m/README.md` L149, L231–232; `09-timers/README.md` L59–64, L210–214 | Exact text check vs ARM AN321 §4.9 & ST ES0430 §2.1.3 (ARM 838869) | **PASS** *(1 LOW nuance)* |
| **02** | **Finding A (Residual)** | `01-master-concept-map/learning-sequence.md` L76; `lab-05/README.md` L70, L324; `exti_g4.h` L7–8; `tim_g4.h` L11–12; `exti-syscfg-register-map.md` L9 vs L61 | Cross-file consistency check against Finding A consensus | **FAIL (MEDIUM + LOW)** |
| **03** | **Finding B (LL/HAL API)** | `lab-01/README.md` L141–214; `lab-05/README.md` L155–286; `lab-08/README.md` L264–444 | Line-by-line audit vs official `STMicroelectronics/stm32g4xx-hal-driver` | **PASS (API spelling) / FAIL (MED race in `lab-08` HAL)** |
| **04** | **Finding B (Matrices)** | `AUDIT/LAB_VERIFICATION_MATRIX.md`, `AUDIT/HARDWARE_VERIFICATION_MATRIX.md`, `adc_g4.c` L98–104, `freertos/Src/main.c` L226–263 | Cross-check vs `HARDWARE_VALIDATION_SUMMARY.md` & RM0440 §21.4.8 | **PASS** |
| **05** | **Finding C (Live Run)** | `scripts/validate_repo.py` v4.0.0 full run (`make` ARM builds + ELF + `tests/`) | Direct local CLI execution (`exit 0`, `26 PASS / 1 WARN / 1 UNVERIFIED / 0 ERR`, `594/594` host tests) | **PASS** |
| **06** | **Finding C (Mutations)** | `scripts/validate_repo.py` v4.0.0 (`M1`–`M7` in isolated `tempfile.TemporaryDirectory()`) | Positive controls `M1` (bullet bit pos) & `M3` (broken link) caught (`2/2`); adversarial mutations `M2`, `M4`, `M5`, `M6`, `M7` **escaped with `0` errors (`5/5` blind spots)** | **FAIL (HIGH)** |
| **07** | **GitHub MathJax** | All `130` `.md` files (`1,786` math spans), incl. `learning-sequence.md` L69 & `FORMULA_AND_CALCULATION_SHEET.md` | Independent strict scanner (`\text`/`\mathrm`/`\texttt`/`\textbf`/`\mbox`, bare `\_`, `{}` balance, `$` closure, table pipes) | **FAIL (HIGH — 5 bugs left in 3 `AUDIT/` files)** |
| **08** | **Formula Desk Sheet** | `LEARNERS/FORMULA_AND_CALCULATION_SHEET.md` L95 | Cross-check vs RM0440 §39.4.9, `Lab 12`, & `i2c_g4.h` (`0x70811021` vs broken `0x30612144`) | **FAIL (HIGH)** |
| **09** | **SoT & Audit Docs** | `AUDIT/concepts_sot.json` L3295 (`ARM-PWR-001`), `PERIPH-ADC-002`, `PERIPH-BDTR-001`; `AUDIT/TECHNICAL_DEFECTS.md` L52; `AUDIT/EDUCATIONAL_QUALITY_REPORT.md` L79–80/130–133; `LEARNERS/MKDOCS_PORTAL_PLAN.md` | Programmatic JSON/DAG/backtick/JP5/9-column audit & cross-report review | **FAIL (HIGH + 2 MED)** |

---

## III.3 Detailed Adversarial Findings & Concrete Remediation Actions

### HIGH FINDINGS

#### `[R12-HIGH-01]` Stale Spec-Violating `I2C TIMINGR = 0x30612144` Still Published as `"Verified Lab 12"` in `LEARNERS/FORMULA_AND_CALCULATION_SHEET.md`
- **File & Line:** `LEARNERS/FORMULA_AND_CALCULATION_SHEET.md` L95
- **Verified Defect:** L95 states:
  > *"- **Verified `Lab 12` 400 kHz Fast Mode @ $f_{\text{I2CCLK}} = 170\text{ MHz}$:** `TIMINGR = 0x30612144` (`PRESC=3`, `SCLDEL=6`, `SDADEL=1`, `SCLH=0x21`, `SCLL=0x44`)."*
  Part 4 `[HIGH-03]` and Phase 8 `[P8-HIGH-03]` proved `0x30612144` violates **RM0440 §39.4.9** (`t_SCLDEL = 164.7 ns < 400 ns`, breaking for any bus rise time $t_r > 64.7\text{ ns}$), and both `Lab 12` and `23-code-library/Drivers/i2c_g4.h` were upgraded to **`0x70811021`**. Teaching learners the superseded, broken constant in the primary formula cheat sheet directly contradicts `Lab 12` and `i2c_g4.h`.
- **Concrete Remediation:** Replace `0x30612144` at `LEARNERS/FORMULA_AND_CALCULATION_SHEET.md` L95 with **`0x70811021`** (`PRESC=7`, `SCLDEL=8` → `t_SCLDEL = 423.5 ns >= 400 ns`, `SDADEL=1`, `SCLH=0x10`, `SCLL=0x21`, `f_max = 396.3 kHz <= 400 kHz`) and add the 160 MHz equivalent **`0x70710F1F`**.

#### `[R12-HIGH-02]` Residual `JP5` IDD Hardware Hazard in `AUDIT/concepts_sot.json` + 5 Remaining GitHub MathJax / Truncated-Math Defects in `AUDIT/`
- **Files & Lines:**
  - `AUDIT/concepts_sot.json` L3295 (`ARM-PWR-001.completion_criteria`)
  - `AUDIT/MISSING_CONCEPTS_AND_GAPS.md` L315, L316, L400
  - `AUDIT/COMPLETE_CONCEPT_INVENTORY.md` L283 (`PERIPH-ADC-002`, mirrored from `concepts_sot.json`)
  - `AUDIT/TECHNICAL_VERIFICATION_REPORT.md` L25
  - `AUDIT/concepts_sot.json` (`PERIPH-ADC-002` & `PERIPH-BDTR-001`)
- **Verified Defects:**
  1. **Hardware Safety Hazard (`ARM-PWR-001`, `AUDIT/concepts_sot.json` L3295):** Still instructs learners: *"measuring IDD drop on JP5"*. On NUCLEO-G474RE (`UM2505 §6.3–§6.4`), `JP5` is the `5V_SEL` power selector and **`JP6`** is the `IDD` current jumper! Worker C2 fixed `STM32-PWR-002` but overlooked `ARM-PWR-001`.
  2. **5 Real GitHub MathJax Rendering Defects Still Present (Despite `"136 -> 0"` Claim):**
     - `AUDIT/MISSING_CONCEPTS_AND_GAPS.md` L315 (`$I_{IO(\text{PIN})... |`), L316 (`$R_{p,\... |` pairing across table columns with `40 k$\Omega$`), and L400 (`$< 0.5\text{... |` pairing with `$t_{CONV}=...$`) — 80-char table cell truncation sliced inside `$...$` and `{...}`, leaving unclosed `$`, unclosed `{`, and unescaped `|` inside inline math that breaks GitHub table rendering!
     - `AUDIT/COMPLETE_CONCEPT_INVENTORY.md` L283 (2× `$t_{S\_temp} \ge 5\ \mu\text{s}$`, sourced from `concepts_sot.json` `PERIPH-ADC-002`) and `AUDIT/TECHNICAL_VERIFICATION_REPORT.md` L25 (`$t_{S\_\text{temp}} \ge 5\ \mu\text{s}$`) — GFM strips `\_` → `_` before MathJax parses the math span, corrupting `t_{S\_temp}` into $t_{S_t emp}$.
  3. **Residual SoT String Truncations:** `PERIPH-ADC-002.explanation` is truncated mid-math (`+ 5\text{–}15^\circ\text{C.`), `PERIPH-BDTR-001.concept` has an unmatched backtick (`` `BDTR.DTG ``), and `81` legacy `explanation`/`gap_summary` strings still contain mid-word/unmatched-backtick truncations.
- **Concrete Remediation:**
  1. Update `ARM-PWR-001.completion_criteria` in `AUDIT/concepts_sot.json` L3295 from `JP5` → **`JP6` (`IDD`, UM2505 §6.4 — never `JP5`)**.
  2. Replace `t_{S\_temp}` / `t_{S\_\text{temp}}` with `t_{S,\text{temp}}` in `concepts_sot.json` (`PERIPH-ADC-002`), `AUDIT/COMPLETE_CONCEPT_INVENTORY.md` L283, and `AUDIT/TECHNICAL_VERIFICATION_REPORT.md` L25; repair truncated math cells in `AUDIT/MISSING_CONCEPTS_AND_GAPS.md` L315, L316, and L400 and close backticks in `PERIPH-BDTR-001` & truncated `explanation` fields.

#### `[R12-HIGH-03]` 5 Empirically Proven Blind Spots in `scripts/validate_repo.py` v4.0.0
- **File & Lines:** `scripts/validate_repo.py` L122–145, L811–843, L1228–1285, L1388
- **Empirical Adversarial Mutation Proof (Isolated `tempfile.TemporaryDirectory()`):**
  - **Caught (`2/2` Positive Controls):** `M1` (`**Bit 8 (`HSION`)**` → `Bit 9` in `rcc-register-map.md`: **CAUGHT**, `BITPOS MISMATCH`) and `M3` (`[broken](does-not-exist.md)`: **CAUGHT**, `Broken internal inline links`).
  - **Escaped with `0 ERROR` (`5/5` Proven Blind Spots):**
    1. **`M2` (Table-Cell Bitfield Range Blind Spot, L811–843):** Re-injecting Part 4 `[CRIT-02]` (`BLANKING[21:19]` → `BLANKING[21:18]` in `opamp-comp-vrefbuf-register-map.md` table) exited **`0` errors** — Pass C only checks `21` `**Bit N (`FIELD`)**` bullet items and ignores all table-cell bit ranges (`FIELD[hi:lo]` / `(bN)`).
    2. **`M5` (`\texttt`/`\textbf`/`\textit`/`\mbox` MathJax Blind Spot, L1228):** Injecting `$f_{\texttt{TIM\_CLK}} = 170\text{ MHz}$` exited **`0` errors** — L1228 regex only matches `\text{}` and `\mathrm{}` (even though `TECHNICAL_DEFECTS.md` L60 explicitly cites `\texttt{...}`!).
    3. **`M6` (Unclosed `$` / Unbalanced `{` / Bare `\_` in Math Blind Spot, L1228–1285):** Injecting broken unclosed math `$f_{SYSCLK} = 170 MHz` exited **`0` errors** — which is exactly how `[R12-HIGH-02]` escaped v4.0.0!
    4. **`M4` (Markdown `#anchor` Fragment Blind Spot, L1388):** Injecting `[broken anchor](test_doc.md#totally-fake-heading)` exited **`0` errors** because L1388 strips `#fragment` without verifying target headings.
    5. **`M7` (Register-Map / Lab C-Snippet Compilation Blind Spot):** Re-injecting Part 4 `[CRIT-02]` (`OPAMP_CSR_OPAMPxEN` → `OPAMP_CSR_OPAEN` in `opamp-comp-vrefbuf-register-map.md`) exited **`0` errors** because v4.0.0 still does not compile C blocks extracted from `20-technical-diagrams/registers/*.md` or `18-practical-laboratory/lab-01..19/README.md`. *(Additionally, `REQUIRED_AUDIT_FILES` L122–145 omits the 6 newly created Section 9 reports.)*
- **Concrete Remediation:** Upgrade `validate_repo.py` to v4.1.0: (1) extend `scan_mathjax_in_file()` to check `\texttt`/`\textbf`/`\textit`/`\mbox`, bare `\_` in math, unbalanced `{}`/`$`, and unescaped `|` in table math; (2) extend Pass C to cross-check table-cell `FIELD[hi:lo]` and `(bN)` against CMSIS `_Pos`/`_Msk`; (3) verify local `#anchor` headings; (4) add an optional/default C-snippet compile gate for register maps & Approach A lab code; and (5) add the 6 new Section 9 reports to `REQUIRED_AUDIT_FILES`.

---

### MEDIUM FINDINGS

#### `[R12-MED-01]` `lab-08` Approach C (HAL) RX Buffer Data Race & Non-`volatile` Shared Variable
- **File & Lines:** `18-practical-laboratory/lab-08-usart-baremetal-driver-printf/README.md` L381, L392–397, L437–441, L525
- **Verified Defect:**
  1. `static uint8_t g_hal_rx_byte = 0U;` (L381) omits `volatile` even though `HAL_UART_IRQHandler()` writes it in ISR context and `main()` reads it in thread context.
  2. `HAL_UART_RxCpltCallback()` (L394–395) sets `g_hal_rx_ready = 1U;` and **immediately re-arms `HAL_UART_Receive_IT(&g_huart2, &g_hal_rx_byte, 1U)`** targeting the **exact same single-byte buffer** *before* `main()` copies `g_hal_rx_byte`, creating an ISR/main overwrite race on back-to-back bytes!
  3. L525 (`§12.1`) overstates that all 3 approaches include `(void)USART2->ISR; __DSB();` — Approach C's `USART2_IRQHandler()` (L388–390) calls only `HAL_UART_IRQHandler(&g_huart2);`.
- **Concrete Remediation:** Make `g_hal_rx_byte` `volatile` and either copy the received byte into a separate volatile/SPSC ring buffer inside `HAL_UART_RxCpltCallback()` before calling `HAL_UART_Receive_IT()`, or re-arm `HAL_UART_Receive_IT()` from `main()` after reading `ch`; qualify L525 to note `(void)USART2->ISR; __DSB();` is in Approaches A & B (and add `(void)USART2->ISR; __DSB();` after `HAL_UART_IRQHandler` in Approach C for parity).

#### `[R12-MED-02]` Residual Finding A Conflation Across 4 Files (`learning-sequence.md`, `lab-05`, `exti_g4.h`, `tim_g4.h`)
- **Files & Lines:**
  - `01-master-concept-map/learning-sequence.md` L76
  - `18-practical-laboratory/lab-05-exti-pushbutton-interrupt/README.md` L70, L324
  - `23-code-library/Drivers/exti_g4.h` L7–8
  - `23-code-library/Drivers/tim_g4.h` L11–12
- **Verified Defect:** Despite Finding A's clean separation in `10-interrupts/README.md` §3.6, these 4 locations retain pre-remediation conflation:
  - `learning-sequence.md` L76 still writes *"preventing phantom double-entry per `ES0430 §2.1.3` / ARM 838869"* (conflating APB bridge double-entry [AN321 §4.9] with ARM 838869 [incorrect interrupt vectoring]!).
  - `lab-05` L324 (`§9` Troubleshooting) still prescribes *"Posted APB2 write did not retire before ISR exit -> Add `__DSB();`"* (omitting `(void)EXTI->PR1;`), and L70 conflates ARM 838869 with tail-chaining a duplicate ISR entry.
  - `exti_g4.h` L7–8 top file banner still says *"followed by Cortex-M4 `__DSB()` (ARM Errata 838869 / ES0430 §2.1.3) to prevent double-triggering"* (while L102–113 was properly fixed), and `tim_g4.h` L11–12 omits `(void)tim->SR;` in its top banner (while L135–145 was fixed).
- **Concrete Remediation:** Align all 4 comments/table rows with `10-interrupts/README.md` §3.6 (`(void)REG;` = ARM AN321 §4.9 APB posted-write commit; `__DSB();` = CPU write-buffer drain + ARM Erratum 838869 / ST ES0430 §2.1.3).

#### `[R12-MED-03]` Unescaped Pipe (`|`) Splits `DEF-C-002` Row in `AUDIT/TECHNICAL_DEFECTS.md` into 11 Columns
- **File & Line:** `AUDIT/TECHNICAL_DEFECTS.md` L52
- **Verified Defect:** All `24` other `DEF-*` rows have exact `9/9` populated fields, but L52 (`DEF-C-002`) contains unescaped `|` characters inside inline code (`` `|` `` and `` `\|` ``), splitting the Markdown table row into **11 columns** and misaligning the rendered GitHub table.
- **Concrete Remediation:** Escape pipe characters inside `DEF-C-002` as `&#124;` (or spell out `"pipe character"`) so all `25/25` rows parse cleanly as exactly 9 columns.

#### `[R12-MED-04]` Stale Pre-Merge Claims in `AUDIT/EDUCATIONAL_QUALITY_REPORT.md` & Off-by-One Stage Numbering in `learning-sequence.md`
- **Files & Lines:**
  - `AUDIT/EDUCATIONAL_QUALITY_REPORT.md` L79–80, L130–133
  - `01-master-concept-map/learning-sequence.md` L74, L81, L88, L95, L102, L109, L115 vs `10-interrupts/README.md` L1 (`Stage 08`), `09-timers/README.md` L1 (`Stage 09`), etc.
- **Verified Defect:**
  1. `AUDIT/EDUCATIONAL_QUALITY_REPORT.md` L79–80 & L130–133 still state that stages `04/08/09/10` lack nav bars, recommend adding a MathJax guard to `validate_repo.py`, and treat physical NUCLEO-G474RE testing as unexecuted — all three are now stale post-Wave-2.
  2. `learning-sequence.md` numbers `10-interrupts/` as `Stage 07` and `09-timers/` as `Stage 08` (through `Stage 15`), whereas the actual stage README titles (`10-interrupts/README.md` L1, `09-timers/README.md` L1, ..., `17-advanced-firmware-engineering/README.md` L1) number them `Stage 08` through `Stage 16`.
- **Concrete Remediation:** Refresh `AUDIT/EDUCATIONAL_QUALITY_REPORT.md` §3 & §6 to reflect post-Wave-2 status (`17/17` nav bars deployed, validator v4.0.0 active, `13/20` labs onboard-tested) and align stage numbering consistently between `learning-sequence.md` and stage README headers.

---

### LOW FINDINGS

- **`[R12-LOW-01]` `20-technical-diagrams/registers/exti-syscfg-register-map.md` L9 vs L61 Tag Mismatch:** L9 declares `"C code snippets Compile-Verified"`, while L61 directly above the snippet says `"Statically Reviewed"`. Update L61 to `"Compile-Verified (Not Tested on Physical Hardware)"`.
- **`[R12-LOW-02]` `21-reference-library/official-st-documentation.md` L30 Stale RM0440 RTC Chapter:** Lists `RTC (Ch 34)` instead of RM0440 **Ch 37** (as fixed in `rtc-tamp-usb-register-map.md` under Part 4 `[MED-04]`).
- **`[R12-LOW-03]` ARM AN321 §4.9 Asynchronous Clock-Domain Synchronizer Nuance (`10-interrupts/README.md` L85):** Clarify in 1 sentence that while `(void)EXTI->PR1;` stalls until the AHB-to-APB2 bridge commits the clear at the EXTI slave, peripherals in slower/asynchronous clock domains (or APB prescaler `> /1`) may still exhibit 1–2 cycles of IRQ-wire synchronizer latency back to NVIC — reinforcing why clearing *early* in the ISR + checking the status flag on entry + readback + `__DSB()` is the complete pattern.

---

## III.4 Section 10 Final Acceptance Criteria Scorecard (10 Criteria)

| # | Acceptance Criterion | Independent Verdict | Evidence / Blocking Item(s) |
|---:|---|---|---|
| **1** | **Finding A:** Accurate ARM AN321 §4.9 vs ES0430 §2.1.3 (ARM 838869) & tail-chaining everywhere | **PARTIAL** | Core stages/maps/code PASS; residual conflation in `learning-sequence.md:76`, `lab-05:70/324`, `exti_g4.h:7–8`, `tim_g4.h:11–12` (`[R12-MED-02]`) |
| **2** | **Finding B:** Complete 3-way Bare-Metal/LL/HAL Labs `01/05/08` (exact CubeG4 APIs) & 20-lab 6-state matrices | **PARTIAL** | All CubeG4 LL/HAL spellings 100% exact & matrices clean; `lab-08` Approach C HAL RX buffer race (`[R12-MED-01]`) |
| **3** | **Finding C:** Regression-resistant `scripts/validate_repo.py` v4.0.0 & `0 ERROR` live run | **PARTIAL** | Live run exits `0` (`594/594` tests, 5 ARM builds PASS); 5 adversarial mutations escaped (`[R12-HIGH-03]`) |
| **4** | **GitHub MathJax:** Zero MathJax rendering defects repo-wide (`136 -> 0`) | **FAIL** | Learner docs 100% clean, but 5 defects remain in 3 `AUDIT/` files (`[R12-HIGH-02]`) |
| **5** | **Register & Hardware Math Ground Truth:** Zero stale silicon/board hazards across all docs | **FAIL** | 16 register maps PASS (0 CMSIS mismatches), but broken `0x30612144` in `FORMULA_AND_CALCULATION_SHEET.md:95` (`[R12-HIGH-01]`) & `JP5` IDD in `ARM-PWR-001` (`[R12-HIGH-02]`) |
| **6** | **Verification Honesty & Hardware Safety Boundary:** Zero false hardware claims; `PENDING — USER CONNECTION REQUIRED` enforced | **PASS** | 100% honest across all 20 labs, matrices, and reports; zero Option-Byte mutations |
| **7** | **Source-of-Truth `concepts_sot.json` v2.1.0 Integrity:** Acyclic DAG + clean metadata | **PARTIAL** | DAG spotless (`350` IDs, `0` cycles, `0` inverted edges); `ARM-PWR-001` (`JP5`), `PERIPH-ADC-002` (`t_{S\_temp}`), `PERIPH-BDTR-001` unmatched backtick (`[R12-HIGH-02]`) |
| **8** | **Consolidated Audit Suite:** `TECHNICAL_DEFECTS.md` 9 fields & cross-report synchronization | **PARTIAL** | `DEF-C-002` unescaped `|` breaks 9-column table (`[R12-MED-03]`); stale post-merge notes in `EDUCATIONAL_QUALITY_REPORT.md` (`[R12-MED-04]`) |
| **9** | **Builds, ELF & Host Tests:** Clean ARM GCC cross-builds (`0` warn) + `594/594` host tests | **PASS** | Independently re-executed live (`debug`/`release` template, `.isr_vector` 472 B @ 512 B, FreeRTOS V11.1.0, 3 Capstones, `594/594` PASS) |
| **10** | **Educational UX, Nav Bars, External URLs & MkDocs Plan:** Seamless navigation & zero-duplication portal plan | **PARTIAL** | `17/17` nav bars, `70/70` live URLs, strong `MKDOCS_PORTAL_PLAN.md`; off-by-one stage numbering in `learning-sequence.md` (`[R12-MED-04]`) |

**Criteria Satisfied:** **2 PASS / 6 PARTIAL / 2 FAIL** → **CONDITIONAL HOLD**.

---

## III.5 Mandatory Final Release Checklist (Before Final Commit & Merge)

1. **`[R12-HIGH-01]`** Update `LEARNERS/FORMULA_AND_CALCULATION_SHEET.md` L95 from broken `0x30612144` → **`0x70811021`** (170 MHz) / **`0x70710F1F`** (160 MHz).
2. **`[R12-HIGH-02]`** Fix `ARM-PWR-001.completion_criteria` (`JP5` → `JP6`), fix `t_{S\_temp}` / `t_{S\_\text{temp}}` → `t_{S,\text{temp}}` in `concepts_sot.json` (`PERIPH-ADC-002`), `AUDIT/COMPLETE_CONCEPT_INVENTORY.md` L283, and `AUDIT/TECHNICAL_VERIFICATION_REPORT.md` L25, and repair truncated math cells in `AUDIT/MISSING_CONCEPTS_AND_GAPS.md` L315/L316/L400 + `PERIPH-BDTR-001`.
3. **`[R12-HIGH-03]`** Harden `scripts/validate_repo.py` (`v4.0.0` → `v4.1.0`) to close the 5 proven blind spots (`\texttt`/`\textbf`/`\mbox` + bare `\_` + unclosed `$`/braces in MathJax, table-cell bit positions, `#anchor` verification, register-map/lab C-snippet compile check, and adding the 6 new Section 9 reports to `REQUIRED_AUDIT_FILES`).
4. **`[R12-MED-01..04]`** Fix `lab-08` HAL RX buffer race (`volatile` + ring buffer / post-read re-arm), clean residual Finding A conflation (`learning-sequence.md:76`, `lab-05:70/324`, `exti_g4.h:7–8`, `tim_g4.h:11–12`), escape `|` in `AUDIT/TECHNICAL_DEFECTS.md` L52, refresh `AUDIT/EDUCATIONAL_QUALITY_REPORT.md` L79–80/130–133, and align `learning-sequence.md` stage numbers.
5. **`[R12-LOW-01..03]`** Fix `exti-syscfg-register-map.md` L61 tag, `official-st-documentation.md` L30 (`RTC Ch 37`), and add 1-sentence AN321 §4.9 clock-domain synchronizer nuance to `10-interrupts/README.md` L85.
