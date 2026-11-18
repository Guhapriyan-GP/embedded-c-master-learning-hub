# Curriculum Roadmap & Milestone Schedule

This roadmap defines the canonical, strictly acyclic progression through the **Embedded C Master Learning Hub** across **4 phases, 17 learning stages (`Stage 00` + `Stages 01–15` with `06A/06B`), 20 practical laboratories (`Labs 01–20`), 5 mastery examinations, and 3 capstone blueprints**.

> **Dependency Order Note:** Per [`01-master-concept-map/overview.md`](01-master-concept-map/overview.md), **Basic GPIO on reset 16 MHz HSI16 (`08-gpio/`, Labs 01–02)** is completed *before* **170 MHz RCC PLL Boost (`07-rcc-and-clocks/`, Lab 03)**, and **Interrupts/SysTick/NVIC/EXTI (`10-interrupts/`, Labs 04–05)** are completed *before* **Hardware Timers & PWM (`09-timers/`, Labs 06–07)**.

---

### Phase 1: Foundations & Bare-Metal Core (`Stages 00–04`, Level 1–2)
- **Milestone 1.0 (`01b-digital-electronics-and-hw-integration/`):** Ohm's law, RC $\tau$, CMOS thresholds, DS12288 `FT`/`TT_a` & $\pm 20\text{ mA}$ limits, UM2505 `MB1367` schematics (`JP6` IDD vs `JP5`, Active-HIGH `B1` `PC13`, `SB6`), oscilloscope/logic-analyzer SOPs.
- **Milestone 1.1 (`02-c-fundamentals/`):** ISO C11 translation pipeline, `<stdint.h>`, two's complement, integer promotions, bitwise masking, pointers, structs/unions, storage classes, UB taxonomy, and passing `tests/test_c_foundations.c` (35/35 host assertions) + Stage 01 Exam.
- **Milestone 1.2 (`03-embedded-c/`):** MMIO struct overlays, `volatile` limits, `rc_w1`/`rc_w0`/`w` register semantics, `MODIFY_REG`, `DMB`/`DSB`/`ISB` barriers, `PRIMASK`/`BASEPRI` critical sections, `LDREX`/`STREX`, CMSIS-Core/Device + Stage 02 Exam.
- **Milestone 1.3 (`04-arm-cortex-m/`):** ARMv7E-M Cortex-M4F r0p1 pipeline, `R0–R15` (`PC+4`, `bit[0]=0`), `xPSR` (`N,Z,C,V,Q,GE`), `MSP`/`PSP`, Thumb-2, AAPCS hard-float ABI, 8-word vs 26-word FPU frames, `EXC_RETURN`, lazy stacking, `CFSR`/`HFSR`/`MMFAR`/`BFAR`, 8-region MPU, DSP/SIMD.
- **Milestone 1.4 (`05-toolchain-and-debugging/`):** `arm-none-eabi-gcc` + Make/CMake, linker scripts (`FLASH`, `RAM`, `CCMRAM (xrw)` + `_siccmram`, `.noinit`), 118-entry STM32G474RE vector table (472 B, 512 B aligned), OpenOCD/STLINK-V3E SWD, GDB CLI, SVD, `DWT->CYCCNT`, `DBGMCU` + Stage 03 Exam.

---

### Phase 2: Core STM32G474RE Platform Bring-Up (`Stages 05–07`, Level 1–2)
- **Milestone 2.1 (`06-stm32-fundamentals/`):** Multi-layer AHB BusMatrix (including AHB2 `DAC1–4` & `0x20018000` CCM-SRAM DMA alias), LQFP64 52-pin boundaries (`GPIOE`/`SPI4`/`FMC`/`AES` exclusions), RM0440/DS12288/UM2505/ES0430 navigation.
- **Milestone 2.2 (`08-gpio/` — Stage 06A):** Reset `11b` Analog `MODER`, atomic `BSRR`/`BRR`, `OSPEEDR`, `AFR`, `PWR_CR3.UCPD_DBDIS` — **Labs 01–02** (3-way Register/LL/HAL blinky + non-blocking 4-state SysTick FSM debouncer on Active-HIGH `PC13`).
- **Milestone 2.3 (`07-rcc-and-clocks/` — Stage 06B):** 24 MHz HSE X3, PLL bounds ($[2.66, 16]\text{ MHz}$ / $[96, 344]\text{ MHz}$), `PWR_SR2.VOSF`, `PWR_CR5 @ 0x80` Range 1 Boost, Flash 4WS + ART, mandatory RM0440 §6.1.5 `HPRE=/2` ($\ge 1\ \mu\text{s}$) ramp, `PA8` MCO — **Lab 03**.
- **Milestone 2.4 (`10-interrupts/` — Stage 07):** 24-bit SysTick, 4-bit NVIC `PRIGROUP`, `SYSCFG->EXTICR`, Active-HIGH `EXTI->RTSR1`, direct `EXTI->PR1 = mask` + `__DSB()` — **Labs 04–05** (SysTick scheduler + 3-way EXTI13).

---

### Phase 3: Peripherals, Mixed-Signal & Hardware Accelerators (`Stages 08–11`, Level 2–3)
- **Milestone 3.1 (`09-timers/` — Stage 08):** Basic (`TIM6/7`), GP (`TIM2/5` 32-bit, `TIM3/4`, `TIM15–17`), Advanced (`TIM1/8/20` complementary PWM + `BDTR.DTG` + `BKIN`), `LPTIM1`, and flagship `HRTIM1` (184 ps DLL) — **Labs 06–07** (TIM2 ISR with `EGR=UG`/`rc_w0`, TIM3 25 kHz 3-channel RGB PWM).
- **Milestone 3.2 (`11-communication/` — Stage 09):** `USART1–3`/`UART4–5`/`LPUART1` (`RDR`/`TDR`, SPSC ring buffer), `SPI1–3` (`FRXTH=1`, 2-byte reads), `I2C1–4` (`TIMINGR`, `RXDR`/`TXDR`, `RELOAD`, 9-clock recovery), `FDCAN1–3` (shared Message RAM `0x4000A400`), `USB FS + CRS`, `UCPD1`, `SAI1` — **Labs 08, 11, 12, 18**.
- **Milestone 3.3 (`12-adc-dac-analog/` — Stage 10):** 5× 12-bit SAR ADCs ($\le 60\text{ MHz}$ via `CKMODE=11` `42.5 MHz`, `DEEPPWD`, calibration, $t_{S\_\text{temp}} \ge 5\ \mu\text{s}$), AHB2 `DAC1–4` (`HFSEL=10`), `OPAMP1–6` PGA, `COMP1–7` $\to$ `TIM1_BKIN`, `VREFBUF` — **Labs 09 & 16**.
- **Milestone 3.4 (`13-dma/` — Stage 11):** `DMA1/2` (16 ch), `DMAMUX1` (Table 91 IDs, ReqGen, Sync, ES0430 individual flag clears), Circular Ping-Pong `HTIF`/`TCIF`, `CORDIC` (`q1.31`), and `FMAC` (`q1.15`) — **Labs 10 & 17** + Stage 04 Exam.

---

### Phase 4: Memory, Reliability, RTOS & Production Verification (`Stages 12–15`, Level 2–3)
- **Milestone 4.1 (`14-memory-and-storage/` — Stage 12):** Dual-Bank (`DBANK=1`, 2 KB) vs Single-Bank (`DBANK=0`, 4 KB) 64-bit ECC Flash RWW, 9-step safe bootloader jump (`VTOR` 512 B align), `QUADSPI`, `CRC`, `RNG`, and STM32G474RE security (`RDP` L0/1/2 warning, `WRP`, `PCROP`, `SEC1R/2R`).
- **Milestone 4.2 (`15-power-management/` — Stage 13):** `IWDG` (`PR=100b`, `WINR`, `DBGMCU` freeze), `WWDG` (`EWI`), `RTC`/`TAMP` 32 backup regs, `BOR`/`PVD`, and Sleep / LP-Run / Stop 0 / Stop 1 / Standby+`RRS` / Shutdown (`JP6` IDD profiling) — **Labs 13–14**.
- **Milestone 4.3 (`16-rtos/` — Stage 14):** FreeRTOS `ARM_CM4F` (`PendSV`/`SVC`/`SysTick`, FPU lazy stacking, `NVIC_SetPriorityGrouping(0)`, `BASEPRI`/`configMAX_SYSCALL_INTERRUPT_PRIORITY`, static tasks, priority inheritance vs deadlock, RMS/RTA) — **Lab 15** (Statically Reviewed).
- **Milestone 4.4 (`17-advanced-firmware-engineering/` — Stage 15):** Layered architecture, FSM/HSM, MISRA C:2023 & SEI CERT C, `cppcheck`/`-fanalyzer`/`.clang-format`, 8-region MPU stack guard + naked-ASM HardFault `.noinit` logger, host TDD (`tests/unit/test_ring_buffer.c`, 78/78 assertions) & GitHub Actions CI — **Labs 19–20**, Stage 05 Exam, and Capstone Blueprints `01–03`.
