# Technical Correctness Verification & Remediation Report (Stream 1 — Agents 2, 3, 4, 5)

> **Date:** 2026-11-18  
> **Repository:** `<repo-root>/` (Base Commit: `main @ 543d21e`)  
> **Audit Stream & Agents:** Stream 1 — Agents 2 (Embedded C Concurrency), 3 (ARM Cortex-M4F Microarchitecture), 4 (STM32G474RET6 Silicon & LQFP64 Package), and 5 (Interrupt Latency, Real-Time & Errata)  
> **Exclusive Audited & Remediated Scope:** `10-interrupts/README.md`, `04-arm-cortex-m/README.md`, `08-gpio/README.md`, `09-timers/README.md`, `13-dma/README.md`, all 16 register maps in `20-technical-diagrams/registers/*.md`, and this report (`AUDIT/TECHNICAL_CORRECTNESS_REPORT.md`)  
> **Primary Ground-Truth Sources:** Vendored CMSIS headers (`stm32g474xx.h`, `core_cm4.h`, `cmsis_gcc.h`, `mpu_armv7.h`), ST **RM0440**, ST **DS12288**, ST **UM2505** (MB1367), ST **PM0214**, ST **ES0430**, ARM **DDI 0403E.e**, ARM **DDI 0439**, ARM **AN298**, and ARM **AN321 (DAI0321A)**  
> **Verification Honesty Policy:** Zero physical NUCLEO-G474RE board bench measurements were fabricated or executed in this verification pass. All bare-metal C code artifacts in the remediated scope are honestly labeled **Compile-Verified (`arm-none-eabi-gcc 14.3.1`, `-O2 -Wall -Wextra -Werror`, Not Tested on Physical Hardware)**.

---

## 1. Executive Summary

Stream 1 completed a targeted Section 4 (STM32G474RET6 LQFP64 technical correctness, dual 160/170 MHz clock profile preservation, and CMSIS header grounding) and Section 5 (interrupt latency qualification and nuanced ARM Erratum 838869 vs. ARM AN321 §4.9 decision rules) audit across all 21 technical documents in its exclusive partition:

| Audit Dimension | Scope Audited | Findings Opened | Fixed & Verified | Open | Status |
|---|---|---:|---:|---:|---|
| **Interrupt Latency Qualification (Section 5)** | `10-interrupts`, `04-arm-cortex-m`, `09-timers`, `nvic-systick` map | 3 | 3 | 0 | **PASS** |
| **Nuanced Erratum 838869 & Readback/`__DSB()` Rule (Section 5)** | `10-interrupts`, `04-arm-cortex-m`, `09-timers`, `13-dma`, 4 register maps | 3 | 3 | 0 | **PASS** |
| **STM32G474RET6 LQFP64 & Dual 160/170 MHz Profiles (Section 4)** | All 5 Stage READMEs + all 16 register maps | 2 | 2 | 0 | **PASS** |
| **Code Compilation, MathJax & Control-Byte Hygiene** | All 22 files (incl. this report) & 18 C code blocks | 0 | 0 | 0 | **PASS (`18/18` GCC, `0` MathJax, `0` Ctrl Bytes)** |
| **Total** | **22 Files Verified** | **8** | **8** | **0** | **100% CLOSED** |

---

## 2. STM32G474RET6 (LQFP64) Silicon, Clock Profile & Package Verification (Section 4)

### 2.1 Vendored CMSIS Header Ground-Truth Verification

Verified directly against `23-code-library/Drivers/CMSIS/Device/ST/STM32G4xx/Include/stm32g474xx.h` and `23-code-library/Drivers/CMSIS/Core/Include/core_cm4.h`:

| Silicon / Core Parameter | Verified Header Macro & Exact Value | Source File & Line | Curriculum Alignment |
|---|---|---|---|
| **Cortex-M4 Revision** | `#define __CM4_REV 0x0001U` (r0p1) | `stm32g474xx.h:47` | Consistent (`SCB->CPUID = 0x410FC241`, ES0430 §2.1.3 applies) |
| **MPU & FPU Presence** | `__MPU_PRESENT = 1U`, `__FPU_PRESENT = 1U` | `stm32g474xx.h:48, 51` | Consistent (8-region ARMv7-M MPU, FPv4-SP single-precision FPU) |
| **NVIC Priority Bits & IRQs** | `__NVIC_PRIO_BITS = 4U`; 102 External IRQs (`0..101`) | `stm32g474xx.h:49, 89–173` | Consistent (`EXTI15_10_IRQn = 40`, `TIM2_IRQn = 28`, `DMA1_Channel1_IRQn = 11`, `DMAMUX_OVR_IRQn = 94`, `DMA1_Channel8_IRQn = 96`) |
| **Boost Mode & Flash Latency** | `PWR_CR5_R1MODE_Pos = 8U`, `FLASH_ACR_LATENCY_4WS = 0x4U` | `stm32g474xx.h:5080, 11615` | Consistent across both 160 MHz and 170 MHz Range 1 Boost profiles |
| **Multi-Cycle & Write-Buffer Control** | `SCnSCB_ACTLR_DISMCYCINT_Pos = 0U`, `SCnSCB_ACTLR_DISDEFWBUF_Pos = 1U` | `core_cm4.h:745–749` | Consistent in `04-arm-cortex-m` and `10-interrupts` §3.3.1 |
| **FPU Lazy Stacking Control** | `FPU_FPCCR_ASPEN_Pos = 31U`, `FPU_FPCCR_LSPEN_Pos = 30U` | `core_cm4.h:1321–1325` | Consistent in `04-arm-cortex-m` §3.6 and `10-interrupts` §3.3.1 |

### 2.2 Dual 160 MHz & 170 MHz Clock Profiles Preserved Cleanly

Both production clock profiles (**Profile A: 170 MHz Max Boost** and **Profile B: 160 MHz Clean Integer Boost**, plus **Profile C: 150 MHz Normal** where tabulated) remain cleanly documented and unaltered across all audited stages and register maps:

| File | 170 MHz Profile A | 160 MHz Profile B | Status |
|---|---|---|---|
| `20-technical-diagrams/registers/rcc-register-map.md` | `R1MODE=0`, `4 WS`, `HPRE=/2` (`85 MHz` $\ge 1\ \mu\text{s}$), `SystemClock_Config_170MHz_HSE()` | `R1MODE=0`, `4 WS`, `HPRE=/2` (`80 MHz` $\ge 1\ \mu\text{s}$), `SystemClock_Config_160MHz_HSE()` | **VERIFIED UNTOUCHED** |
| `20-technical-diagrams/registers/pwr-flash-register-map.md` | RM0440 Table 9 `4 WS` mandatory (`> 136 MHz` Boost) | RM0440 Table 9 `4 WS` mandatory (`> 136 MHz` Boost) | **VERIFIED UNTOUCHED** |
| `20-technical-diagrams/registers/adc-dac-register-map.md` | `ADC CKMODE=11` (`42.5 MHz`), `DAC->MCR.HFSEL = 10b` (`> 160 MHz`) | `ADC CKMODE=11` (`40.000 MHz`), `DAC->MCR.HFSEL = 01b` ($\le 160\text{ MHz}$) | **VERIFIED UNTOUCHED** |
| `20-technical-diagrams/registers/hrtim-register-map.md` | `5.44 GHz` / `183.8 ps` | `5.12 GHz` / `195.3 ps` | **VERIFIED UNTOUCHED** |
| `20-technical-diagrams/registers/usart-register-map.md` | `BRR = 1476` (`0x05C4`, `-0.021%`) | `BRR = 1389` (`0x056D`, `-0.008%`) | **VERIFIED UNTOUCHED** |
| `20-technical-diagrams/registers/watchdog-register-map.md` | `WWDG T_max = 197.4 ms` | `WWDG T_max = 209.7 ms` | **VERIFIED UNTOUCHED** |
| `20-technical-diagrams/registers/nvic-systick-register-map.md` | `TPI->ACPR = 84` (2 MHz SWO) | `TPI->ACPR = 79` (2 MHz SWO) | **VERIFIED UNTOUCHED** |
| `10-interrupts/README.md`, `04-arm-cortex-m/README.md`, `08-gpio/README.md`, `09-timers/README.md`, `13-dma/README.md` | SysTick `LOAD=169999`, `TIM2 PSC=16999`, `5.882 ns/cycle`, `680 MB/s` peak AHB | SysTick `LOAD=159999`, `TIM2 PSC=15999`, `6.250 ns/cycle`, `640 MB/s` peak AHB | **VERIFIED & PARITY ALIGNED** |

### 2.3 STM32G474RET6 LQFP64 Package Exclusions Verification

| Excluded Peripheral | Silicon vs. LQFP64 Hardware Truth | Primary Evidence | Audited Documentation Consistency |
|---|---|---|---|
| **`AES` / `CRYP`** | **Absent on STM32G474 silicon** (exists only on STM32G483/G484) | `0` matches for `AES`/`AES_BASE` in `stm32g474xx.h`; DS12288 Table 2 | Explicitly documented in `cordic-fmac-crc-rng-register-map.md` L3 — **CONSISTENT** |
| **`FMC`** | Declared in G474 die header (`FMC_Bank1_R_BASE`), **unbonded on LQFP64** | `stm32g474xx.h:1160–1479`; DS12288 Table 12 | Explicitly noted as unbonded on LQFP64 in `04-arm-cortex-m/README.md` L96 & `rcc-register-map.md` L31 — **CONSISTENT** |
| **`GPIOE`** | Declared in G474 die header, **0 bonded pins on LQFP64** (52 GPIOs total across PA/PB/PC/PD2/PF0–1/PG10) | DS12288 Table 2 & Table 12 | Explicitly excluded in `08-gpio/README.md` L13/L48/L388–389, `gpio-register-map.md` L3, & `exti-syscfg-register-map.md` L26 — **CONSISTENT** |
| **`SPI4`** | Declared in G474 die header (`SPI4_BASE = APB2 + 0x3C00`), **unbonded on LQFP64** (all pins on Port E) | `stm32g474xx.h:1224, 1388`; DS12288 Table 12 | Explicitly excluded in `spi-i2c-register-map.md` L4 — **CONSISTENT** |

---

## 3. Interrupt Latency Qualification Audit (Section 5)

All occurrences of Cortex-M4 **12-cycle stacking** and **6-cycle tail-chaining** in `10-interrupts/README.md` (§3.3, §6.2, §13), `04-arm-cortex-m/README.md` (§1, §3.6, §4, §7), `09-timers/README.md` (§3.2), and `20-technical-diagrams/registers/nvic-systick-register-map.md` are now explicitly qualified as **ideal ARMv7E-M architectural minimum figures** (assuming zero-wait-state vector/instruction fetches and zero bus contention) — **NOT guaranteed physical NUCLEO-G474RE board latency**.

`10-interrupts/README.md` now includes a dedicated subsection (**§3.3.1 — Real-World STM32G474RE Physical Interrupt Latency Contributors (160 / 170 MHz)**) quantifying all **6** real-world contributors:
1. **Flash Wait States (`4 WS` @ 160/170 MHz Boost)** on ART 1 KB I-Cache misses (`+4` cycles per uncached Flash fetch, typically `+4` to `+8+` cycles for cold vector + ISR entry).
2. **SRAM vs. Flash Fetch & CCM-SRAM (`0x10000000`) Zero-Wait-State Benefit** (dedicated I-Code/D-Code `0 WS` fetch vs. shared S-Bus SRAM or `4 WS` Flash).
3. **AHB BusMatrix Contention** (round-robin arbitration stalls when concurrent `DMA1`/`DMA2` bursts access the same SRAM bank as `MSP` or S-Bus vectors).
4. **Multi-Cycle Non-Interruptible Instructions & Bus-Stalled Loads** (`SDIV`/`UDIV`, `LDM`/`STM` when `SCnSCB->ACTLR.DISMCYCINT = 1`, or in-flight slow peripheral bus reads).
5. **FPU Lazy Stacking (`FPU->FPCCR.ASPEN | LSPEN`)** (`0` extra cycles for non-FPU ISRs; `+18` words = `72` bytes pushed on demand if the ISR touches an FPU instruction while `CONTROL.FPCA = 1`).
6. **Higher-Priority IRQ Preemption & Active `PRIMASK`/`BASEPRI` Critical Sections** ($t_{\text{crit}} + t_{\text{higher ISRs}}$).

---

## 4. Nuanced Erratum 838869 & Readback/`__DSB()` Decision Rule Audit (Section 5)

Replaced blanket "always require both `(void)REG;` and `__DSB();`" wording across `10-interrupts/README.md` (§3.6, §8.2, §10), `04-arm-cortex-m/README.md` (§3.8, §10), `09-timers/README.md` (§3.2, §8.1, §10), `13-dma/README.md` (§3.5, §7, §8.1), and 4 register maps (`nvic-systick`, `exti-syscfg`, `timers`, `dma-dmamux`) with the authoritative **3-Row Engineering Decision Table** (canonical home in `10-interrupts/README.md` §3.6):

| Case | Scenario | Recommended Synchronization Pattern | Architectural Basis |
|---|---|---|---|
| **Case 1** | APB/Peripheral flag clear early in a **non-leaf ISR with substantial work before exit** | Direct write (`EXTI->PR1 = mask;`, `TIMx->SR = ~flag;`, `DMAx->IFCR = mask;`) + dummy readback (`(void)REG;`) | **ARM AN321 §4.9** commits the clear across the AHB-to-APB bridge (or AHB peripheral slave); with substantial instructions/calls before return (or `POP {..., PC}`), tail `__DSB()` is **defensive** rather than strictly required by Erratum 838869. |
| **Case 2** | **Short/leaf ISR** with a trailing buffered store immediately before `BX LR` at 160/170 MHz | Early `(void)REG;` readback **AND** trailing `__DSB();` before `}` | **ARM AN321 §4.9** (peripheral commit) **+** **ARM Cortex-M4 r0p1 Erratum 838869 / ST ES0430 §2.1.3** (drains write buffer for trailing stores right before `-O2`-generated `BX LR`). |
| **Case 3** | **Core Interrupt / System Control** (`NVIC_DisableIRQ`, `SCB->VTOR`, `MPU->CTRL`) | **`__DSB(); __ISB();`** (**no peripheral readback**) | **ARM AN321 §4.6 / §4.11** (Strongly-Ordered PPB core registers; no APB bridge involved). |

---

## 5. Complete Findings & Remediation Ledger (10-Field Mandatory Format)

| Unique ID | Severity | Affected File | Exact Problem | Supporting Evidence | Proposed Correction | Responsible Agent | Test Procedure | Result | Final Status |
|---|---|---|---|---|---|---|---|---|---|
| **TC-001** | **HIGH** | [`10-interrupts/README.md`](../10-interrupts/README.md) (§3.3, §6.2, §13) | Presented 12-cycle stacking (`70.6 ns`) and 6-cycle tail-chaining (`35.3 ns`) without explicitly qualifying them as ideal ARMv7E-M architectural minimums (not guaranteed physical board latency), and omitted a quantitative breakdown of all 6 real-world STM32G474RE @ 160/170 MHz physical latency contributors. | `10-interrupts/README.md` L69–72, L179–182, L367; RM0440 Table 9 (`4 WS` @ 160/170 MHz); `core_cm4.h:748` (`ACTLR.DISMCYCINT`); `core_cm4.h:1321–1325` (`FPU_FPCCR`); ARM DDI 0439 / PM0214 §2.3.7. | Explicitly qualify 12-cycle stacking & 6-cycle tail-chaining as ideal architectural minimums; add **§3.3.1** table quantifying all 6 physical contributors; decompose $N_{\text{wait}}$ in §6.2 for both 170 MHz and 160 MHz; update §13 criterion to require empirical bench comparison (Flash vs. CCM-SRAM). | Agent 3 & Agent 5 | Static technical review against ARM DDI 0439, RM0440 Table 9, `core_cm4.h`; automated MathJax, control-byte, and `arm-none-eabi-gcc 14.3.1` compilation check. | `0` MathJax errors, `0` control bytes; §8.1–§8.2 compile `2/2 PASS` (`0` warnings); all 6 contributors verified. | **CLOSED & VERIFIED** |
| **TC-002** | **HIGH** | [`04-arm-cortex-m/README.md`](../04-arm-cortex-m/README.md) (§1, §3.6, §4, §7) | Described 12-cycle basic stacking and 6-cycle tail-chaining without explicitly qualifying them as ideal architectural minimums (not guaranteed physical NUCLEO-G474RE latency) and without linking the 6 STM32G474RE physical latency contributors. | `04-arm-cortex-m/README.md` L20, L113–115, L125, L177, L237; ARM DDI 0439; RM0440 Table 9. | Explicitly qualify 12-cycle stacking & 6-cycle tail-chaining as ideal ARMv7E-M architectural minimums across §1, §3.6, §4, and §7; add concise summary of all 6 physical STM32G474RE latency contributors cross-referencing `10-interrupts/README.md` §3.3.1. | Agent 3 & Agent 5 | Static review + automated MathJax/control-byte scan + `arm-none-eabi-gcc 14.3.1` compile verification of §8.1–§8.3. | `0` MathJax errors, `0` control bytes; §8.1–§8.3 compile `3/3 PASS` (`0` warnings). | **CLOSED & VERIFIED** |
| **TC-003** | **MEDIUM** | [`09-timers/README.md`](../09-timers/README.md) (§2, §3.2) | Lacked explicit timer ISR latency qualification contrasting the ideal 12-cycle Cortex-M4 architectural minimum against physical STM32G474RE latency contributors, leaving the rationale for hardware `COMPx -> TIMx_BKIN` and DMA Burst / `TRGO` under-grounded in §3.2. | `09-timers/README.md` L33–34, L59–64; RM0440 Table 9 & Ch. 28. | Add concise Timer ISR Latency Qualification bullet in §3.2 and prerequisite link in §2 cross-referencing `10-interrupts/README.md` §3.3.1. | Agent 4 & Agent 5 | Static review + MathJax/control-byte scan + `arm-none-eabi-gcc 14.3.1` compile verification of §8.1–§8.4. | `0` MathJax errors, `0` control bytes; §8.1–§8.4 compile `4/4 PASS` (`0` warnings). | **CLOSED & VERIFIED** |
| **TC-004** | **HIGH** | [`10-interrupts/README.md`](../10-interrupts/README.md) (§1, §3.6, §8.2, §10) | Presented an unconditional universal 3-step rule implying every EXTI/peripheral flag clear strictly requires both `(void)REG;` AND `__DSB()`, without differentiating non-leaf ISRs with substantial work before exit (Case 1), short/leaf ISRs ending with buffered stores before `BX LR` (Case 2), and core PPB interrupt control (Case 3). | `10-interrupts/README.md` L15, L109–113, L305–308, L332; ARM AN321 §4.6/§4.9/§4.11; ST ES0430 §2.1.3 (ARM Erratum 838869). | Add the crisp **3-Row Engineering Decision Table** (Cases 1, 2, 3) in §3.6; update §1, §8.2 (`EXTI15_10_IRQHandler` labeled Case 2), and §10 Mistake #3 accordingly. | Agent 2 & Agent 5 | Technical review against ARM AN321 & ES0430 §2.1.3 + `arm-none-eabi-gcc 14.3.1` compile check + MathJax/control-byte scan. | Table cleanly differentiates all 3 cases; `2/2` C blocks compile cleanly; `0` MathJax/control-byte issues. | **CLOSED & VERIFIED** |
| **TC-005** | **HIGH** | [`04-arm-cortex-m/README.md`](../04-arm-cortex-m/README.md) (§3.8, §10), [`09-timers/README.md`](../09-timers/README.md) (§3.2, §8.1, §10), [`20-technical-diagrams/registers/nvic-systick-register-map.md`](../20-technical-diagrams/registers/nvic-systick-register-map.md), [`exti-syscfg-register-map.md`](../20-technical-diagrams/registers/exti-syscfg-register-map.md), [`timers-register-map.md`](../20-technical-diagrams/registers/timers-register-map.md) | Used blanket `"Always ... (void)REG; and __DSB();"` wording without distinguishing Case 1 (non-leaf APB clear, defensive `__DSB()`) vs. Case 2 (short/leaf ISR before `BX LR`) vs. Case 3 (core `NVIC_DisableIRQ`/`VTOR`/`MPU->CTRL` `__DSB(); __ISB();`). `timers-register-map.md:56` also phrased hardware Break as `"zero CPU latency"`. | `04-arm-cortex-m` L149/L388; `09-timers` L59–64/L359; `nvic-systick` L36; `exti-syscfg` L46/L105; `timers` L56/L66. | Align all 5 files to the nuanced 3-Case Decision Rule cross-referencing `10-interrupts/README.md` §3.6; refine `timers-register-map.md:56` to `"zero CPU involvement"`; add latency qualification note in `nvic-systick-register-map.md`. | Agent 2, Agent 3, Agent 5 | Static review + `arm-none-eabi-gcc 14.3.1` compile verification of all affected C blocks + MathJax/control-byte scan. | All 5 files aligned; all C snippets compile with `0` warnings; `0` MathJax/control-byte errors. | **CLOSED & VERIFIED** |
| **TC-006** | **MEDIUM** | [`13-dma/README.md`](../13-dma/README.md) (§3.5, §6.1–§6.2, §7, §8.1), [`20-technical-diagrams/registers/dma-dmamux-register-map.md`](../20-technical-diagrams/registers/dma-dmamux-register-map.md) | `13-dma/README.md` §3.5 & §7 lacked nuanced DMA ISR readback/`__DSB()` guidance (cross-referencing `10-interrupts` §3.6) and ES0430 §2.1.3 / AN321 §4.9 citations; §8.1 `DMA1_Channel1_IRQHandler` lacked `(void)DMA1->ISR;` before `__DSB();` and cast pointers directly to `uint32_t` without `uintptr_t`; §6 lacked 160 MHz Profile B parity figures. | `13-dma/README.md` L63–69, L115–129, L166–167, L201; `dma-dmamux-register-map.md` L115–117; ARM AN321 §4.9; ES0430 §2.1.3. | Add nuanced AHB1 DMA ISR 3-case guidance in §3.5; add 160 MHz Profile B parity figures in §6.1–§6.2; add ES0430 §2.1.3 & AN321 §4.9 to §7; add `(void)DMA1->ISR;` (Case 2 comment) and `(uint32_t)(uintptr_t)` casts in §8.1; align `dma-dmamux-register-map.md` L115 comment. | Agent 2 & Agent 4 | Compile-verify `13-dma/README.md` §8.1 and `dma-dmamux-register-map.md` with `arm-none-eabi-gcc 14.3.1 -O2 -Wall -Wextra -Werror`; run MathJax/control-byte scan. | Both C blocks compile cleanly (`0` warnings, `0` errors); `0` MathJax/control-byte errors. | **CLOSED & VERIFIED** |
| **TC-007** | **LOW** | [`08-gpio/README.md`](../08-gpio/README.md) (L184), [`13-dma/README.md`](../13-dma/README.md) (L8, L139), [`nvic-systick-register-map.md`](../20-technical-diagrams/registers/nvic-systick-register-map.md) (L88), [`timers-register-map.md`](../20-technical-diagrams/registers/timers-register-map.md) (L72), [`dma-dmamux-register-map.md`](../20-technical-diagrams/registers/dma-dmamux-register-map.md) (L64) | Internal verification-label mismatch: file top banners stated `"Compile-Verified (arm-none-eabi-gcc 14.3.1)"` while individual code-section sub-banners still said `"Statically Reviewed"`. | `08-gpio/README.md` L5 vs L184; `nvic-systick` L8 vs L88; `timers` L8 vs L72; `dma-dmamux` L8 vs L64; `13-dma` L8/L139. | After actually compile-verifying all 18 C blocks with `arm-none-eabi-gcc 14.3.1 -O2 -Wall -Wextra -Werror`, align section banners to honestly state `"Compile-Verified (arm-none-eabi-gcc 14.3.1, -O2 -Wall -Wextra -Werror; Not Tested on Physical Hardware)"`. | Agent 2 & Agent 4 | Run `verify_stream1.py` extracting and cross-compiling every C block against vendored CMSIS headers. | `18/18` blocks PASS (`0` errors, `0` warnings); headers and section labels 100% consistent and honest. | **CLOSED & VERIFIED** |
| **TC-008** | **INFO** | All 5 Stage READMEs + all 16 register maps in `20-technical-diagrams/registers/*.md` | Section 4 verification audit required confirming dual 160 MHz & 170 MHz clock profiles remain untouched and LQFP64 package exclusions (`AES`, `FMC`, `GPIOE`, `SPI4`) remain consistent across every register map and stage README. | Direct inspection of `stm32g474xx.h` (`0` `AES` definitions, `SPI4`/`FMC` die-only), DS12288 Table 2 & Table 12, RM0440 Table 9, and all 21 technical Markdown files. | Document verified matrix in §2.2–§2.3 above; no destructive edits needed because all dual clock profiles and LQFP64 exclusions are already accurately maintained. | Agent 4 | Automated grep + manual review of all 16 register maps and 5 stage READMEs against `stm32g474xx.h`. | `100%` consistent (`160/170 MHz` intact; `AES`/`FMC`/`GPIOE`/`SPI4` exclusions verified). | **VERIFIED CLEAN** |

---

## 6. Automated Verification Evidence

Executed via `verify_stream1.py` on Windows using `arm-none-eabi-gcc (GNU Tools for STM32 14.3.rel1.20251027-0700) 14.3.1 20250623`:

```text
Compiler flags:
  arm-none-eabi-gcc -std=c11 -mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard \
                    -DSTM32G474xx -O2 -Wall -Wextra -Werror \
                    -I23-code-library/Drivers/CMSIS/Core/Include \
                    -I23-code-library/Drivers/CMSIS/Device/ST/STM32G4xx/Include -c

Results:
  - 10-interrupts/README.md (§8.1, §8.2)                          : 2/2 PASS (0 warnings, 0 errors)
  - 04-arm-cortex-m/README.md (§8.1, §8.2, §8.3)                  : 3/3 PASS (0 warnings, 0 errors)
  - 08-gpio/README.md (§8.1, §8.2, §8.3, §8.4)                    : 4/4 PASS (0 warnings, 0 errors)
  - 09-timers/README.md (§8.1, §8.2, §8.3, §8.4)                  : 4/4 PASS (0 warnings, 0 errors)
  - 13-dma/README.md (§8.1)                                       : 1/1 PASS (0 warnings, 0 errors)
  - 20-technical-diagrams/registers/nvic-systick-register-map.md  : 1/1 PASS (0 warnings, 0 errors)
  - 20-technical-diagrams/registers/exti-syscfg-register-map.md   : 1/1 PASS (0 warnings, 0 errors)
  - 20-technical-diagrams/registers/timers-register-map.md        : 1/1 PASS (0 warnings, 0 errors)
  - 20-technical-diagrams/registers/dma-dmamux-register-map.md    : 1/1 PASS (0 warnings, 0 errors)
  ----------------------------------------------------------------------------------------------
  Total C Blocks Compile-Verified                                 : 18/18 PASS (0 FAIL)
  Raw ASCII Control-Byte Hygiene (all 22 files incl. this report) : 0 violations
  GitHub MathJax Syntax Check (all 22 files incl. this report)    : 0 violations
  Physical NUCLEO-G474RE Hardware Bench Execution                 : NOT PERFORMED (honestly labeled)
```
