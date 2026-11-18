# Curriculum Roadmap & Milestone Schedule (`v3.0.0`)

This roadmap defines the canonical, strictly acyclic progression through the **Embedded C Master Learning Hub** across **Onboarding (`START_HERE.md` + `LEARNERS/`), 4 engineering phases, 17 learning stages (`Stage 01b` + `Stages 02–16`), 20 practical laboratories (`Labs 01–20`), 5 mastery examinations, and 3 real compiled & onboard-hardware-tested capstone firmware projects**.

> **Dependency Order Note:** Per [`01-master-concept-map/overview.md`](01-master-concept-map/overview.md), **Basic GPIO on reset 16 MHz HSI16 (`08-gpio/`, Labs 01–02)** is completed *before* **Dual 170 MHz / 160 MHz RCC PLL Boost (`07-rcc-and-clocks/`, Lab 03)**, and **Interrupts/SysTick/NVIC/EXTI (`10-interrupts/`, Labs 04–05)** are completed *before* **Hardware Timers & PWM (`09-timers/`, Labs 06–07)**.

---

### Phase 0: Onboarding & Environment Setup (`START_HERE.md` + `LEARNERS/`)
- **Milestone 0.1:** Complete [`LEARNERS/BEGINNER_SETUP_WINDOWS11.md`](LEARNERS/BEGINNER_SETUP_WINDOWS11.md), verify `arm-none-eabi-gcc`, `gcc`, `make`, `STM32_Programmer_CLI`, and pass the 3-command setup self-test (`python scripts/validate_repo.py`, `make -C tests test` -> `574/574 PASS`, `make -C 23-code-library/templates`).
- **Milestone 0.2:** Open [`LEARNERS/LEARNER_PROGRESS_DASHBOARD.md`](LEARNERS/LEARNER_PROGRESS_DASHBOARD.md) and bookmark [`LEARNERS/REGISTER_QUICK_REFERENCE.md`](LEARNERS/REGISTER_QUICK_REFERENCE.md), [`LEARNERS/FORMULA_AND_CALCULATION_SHEET.md`](LEARNERS/FORMULA_AND_CALCULATION_SHEET.md), and [`LEARNERS/TROUBLESHOOTING_GUIDE.md`](LEARNERS/TROUBLESHOOTING_GUIDE.md).

---

### Phase 1: Foundations & Bare-Metal Core (`Stage 01b` + `Stages 02–05`, Level 1–2)
- **Milestone 1.0 (`01b-digital-electronics-and-hw-integration/`):** Ohm's law, RC $\tau$, CMOS thresholds, DS12288 `FT`/`TT_a` & $\pm 20\text{ mA}$ limits, UM2505 `MB1367` schematics (`JP6` IDD vs `JP5`, Active-HIGH `B1` `PC13`, `SB6`), analog rail integrity (`HWSW-ANA-001` `VREFINT_CAL`), and lab instrument SOPs.
- **Milestone 1.1 (`02-c-fundamentals/`):** ISO C11 translation pipeline, `<stdint.h>`, two's complement, integer promotions, bitwise masking, pointers, 2D DMA arrays, C99 FAMs, tagged unions, function-pointer jump tables, `<stdatomic.h>`, Newlib `_reent`/`__malloc_lock` (`C-LIB-001`), and passing `tests/test_c_foundations.c` (**116/116** host assertions) + Stage 01 Exam.
- **Milestone 1.2 (`03-embedded-c/`):** MMIO struct overlays, `volatile` limits, `rc_w1`/`rc_w0`/`w` semantics, `MODIFY_REG`, `DMB`/`DSB`/`ISB` barriers, nested `PRIMASK`/`BASEPRI`, `LDREX`/`STREX` CAS, seqlocks, zero-overhead static HAL dispatch (`FW-DRV-003`), async driver/proxy/HSM patterns, MISRA C:2023 + Stage 02 Exam.
- **Milestone 1.3 (`04-arm-cortex-m/`):** ARMv7E-M Cortex-M4F r0p1 pipeline, `R0–R15` (`PC+4`, `bit[0]=0`), `xPSR` (`N,Z,C,V,Q,GE`), `MSP`/`PSP`, Thumb-2, AAPCS hard-float ABI, 8-word vs 26-word FPU frames, `EXC_RETURN`, lazy stacking, `CFSR`/`HFSR`/`MMFAR`/`BFAR`, 8-region MPU, DSP/SIMD (`SMLAD`, `__SSAT`).
- **Milestone 1.4 (`05-toolchain-and-debugging/`):** `arm-none-eabi-gcc` + Make/CMake, linker scripts (`FLASH`, `RAM`, `CCMRAM (xrw)` + `_siccmram`, `.noinit`), GNU `ld --wrap` & `FLASH`↔`CCMRAM` veneers (`TOOL-LD-004`), 118-IRQ vector table (472 B, 512 B aligned), STLINK-V3E/OpenOCD/GDB/SVD, `DWT->CYCCNT`, `SWO`/`TPIU->ACPR` (`TOOL-DBG-004`) + Stage 03 Exam.

---

### Phase 2: Core STM32G474RE Platform Bring-Up (`Stages 06A–08`, Level 1–2)
- **Milestone 2.1 (`06-stm32-fundamentals/`):** Multi-layer AHB BusMatrix (`0x20018000` CCM-SRAM DMA alias, AHB2 `DAC1–4`), LQFP64 52-pin boundaries (`GPIOE`/`SPI4`/`FMC`/`AES` exclusions), RM0440/DS12288/UM2505/ES0430 navigation.
- **Milestone 2.2 (`08-gpio/`):** Reset `11b` Analog `MODER`, atomic `BSRR`/`BRR`, `LCKR` (`gpio_g4.c`), `SYSCFG->CFGR1` Fm+ 20 mA (`GPIO-FMP-001`), `PWR_CR3.UCPD_DBDIS` — **Labs 01–02** (`ONBOARD HARDWARE TESTED`).
- **Milestone 2.3 (`07-rcc-and-clocks/`):** 24 MHz HSE `X3`, PLL bounds ($[2.66, 16]\text{ MHz}$ / $[96, 344]\text{ MHz}$), `PWR_SR2.VOSF`, `PWR_CR5 @ 0x80` Range 1 Boost, Flash 4WS + ART, mandatory RM0440 §6.1.5 `HPRE=/2` ($\ge 1\ \mu\text{s}$) ramp, **dual `170 MHz` & `160 MHz` Boost profiles** (`clock_g4.c`) — **Lab 03** (`ONBOARD HARDWARE TESTED`).
- **Milestone 2.4 (`10-interrupts/`):** 24-bit SysTick, 4-bit NVIC `PRIGROUP`, `SYSCFG->EXTICR`, Active-HIGH `EXTI->RTSR1`, direct `EXTI->PR1 = mask` + `__DSB()` (`exti_g4.c`) — **Labs 04–05** (`ONBOARD HARDWARE TESTED`).

---

### Phase 3: Peripherals, Mixed-Signal & Hardware Accelerators (`Stages 09–12`, Level 2–3)
- **Milestone 3.1 (`09-timers/`):** Basic (`TIM6/7`), GP (`TIM2/5` 32-bit, `TIM3/4`, `TIM15–17`), Advanced (`TIM1/8/20` complementary PWM + `BDTR.DTG` + `BKIN`), Timer DMA Burst (`TIMx->DCR`/`DMAR`, `TIM-DMA-001`), `LPTIM1`, and flagship `HRTIM1` (184 ps DLL) (`tim_g4.c`) — **Labs 06–07** (Lab 06 onboard-tested; Lab 07 external RGB pending).
- **Milestone 3.2 (`11-communication/`):** `USART1–3`/`LPUART1` (`usart_g4.c`, SPSC `ring_buffer.c`, Stop wakeup `COMM-LP-001`), `SPI1–3` (`spi_g4.c`, `FRXTH=1`), `I2C1–4` (`i2c_g4.c`, UM10204 `TIMINGR`, `RELOAD`, `PA15`/`PB7`), `FDCAN1–3` (shared Message RAM `0x4000A400`, `TDCR`/`TXEFS`/`PSR.BO` `COMM-CAN-002`), `USB FS` 16-bit PMA (`COMM-USB-002`) + CRS, `UCPD1`, `SAI1` — **Labs 08, 11, 12, 18** (Labs 08 & 18 onboard-tested; Labs 11–12 external-pending).
- **Milestone 3.3 (`12-adc-dac-analog/`):** 5× 12-bit SAR ADCs (`adc_g4.c`, `CKMODE=11b` `42.5`/`40.0 MHz`, `DEEPPWD`, `TS_CAL1`/`TS_CAL2`, `VREFINT_CAL`), AHB2 `DAC1–4` (`HFSEL=10`), `OPAMP1–6` PGA, `COMP1–7` → `TIM1_BKIN`, `VREFBUF` — **Labs 09 & 16** (external stimulus/scope pending; internal `VREFINT`/Temp verified onboard).
- **Milestone 3.4 (`13-dma/`):** `DMA1/2` (16 ch), `DMAMUX1` (`dma_g4.c`, Table 91 IDs, ES0430 individual flag clears), Circular Ping-Pong `HTIF`/`TCIF`, `CORDIC` (`9.72×` faster than FPU on silicon), and `FMAC` (`0 LSB` diff on silicon) — **Labs 10 & 17** (`ONBOARD HARDWARE TESTED`) + Stage 04 Exam.

---

### Phase 4: Memory, Reliability, FreeRTOS, TDD & Production Capstones (`Stages 13–16`, Level 2–3)
- **Milestone 4.1 (`14-memory-and-storage/`):** Dual-Bank (`DBANK=1`, 2 KB) vs Single-Bank (`DBANK=0`, 4 KB) 64-bit ECC Flash RWW, Option Bytes `OBL_LAUNCH`/`BFB2` (`MEM-OB-001`), 9-step bootloader jump, `QUADSPI`, `CRC-32`, `RNG`, and STM32G474RE security (`RDP`, `WRP`, `PCROP`, `SEC1R/2R`).
- **Milestone 4.2 (`15-power-management/`):** `IWDG` (`iwdg_g4.c`, `DBG_IWDG_STOP`), `WWDG`, `RTC`/`TAMP`, `BOR`/`PVD`, Sleep / LP-Run / Stop 0 / Stop 1 / Standby+`RRS` / Shutdown (`JP6` IDD) — **Labs 13–14** (Lab 13 onboard-tested; Lab 14 `JP6`/manual `B1` pending).
- **Milestone 4.3 (`16-rtos/` + `23-code-library/freertos/`):** Official pinned **`FreeRTOS-Kernel V11.1.0`** (`ARM_CM4F`, `heap_4`, `PRIGROUP=0`, `BASEPRI=0x50`, Semaphores, Priority Inheritance, Deadlock Guard, `FromISR` + Queue + `fpu_err=0`, Stack HWM, Event Group IWDG) — **Lab 15 (`ONBOARD HARDWARE TESTED` — `6/6 ALL_PASSED`)**.
- **Milestone 4.4 (`17-advanced-firmware-engineering/`):** Layered architecture, FSM/HSM, anti-rollback (`SEC-BOOT-002`), MISRA C:2023 & SEI CERT C, `gcc -fanalyzer`, `.clang-tidy`, `Doxyfile`, 8-region MPU + `.noinit` fault persistence (**Lab 19** `ONBOARD HARDWARE TESTED`), host TDD (`574/574` assertions, `95.97%` `gcov` coverage — **Lab 20** `HOST TESTED`), 5-job CI, Stage 05 Exam, and **Capstones `01–03`** (`19-projects/`, all 3 compiled & **`ONBOARD HARDWARE TESTED`**).
