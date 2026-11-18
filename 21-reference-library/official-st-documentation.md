# Official STMicroelectronics Documentation Reference

This library catalogues the authoritative STMicroelectronics primary documentation for the **STM32G474RET6 (LQFP64)** microcontroller, the **NUCLEO-G474RE (MB1367)** board, silicon errata, and core Application Notes.

**Concept IDs:** `RES-REF-001`, `STM32-ARCH-001`, `STM32-ERRATA-001`

---

### 1. Primary STM32G474RE Documentation

#### A. STM32G4 Series Reference Manual (`RM0440`)
- **Document Number:** **RM0440** (Rev 8 / Rev 9 — note chapter shifts between Rev 8 and Rev 9 are called out explicitly below)
- **Title:** *STM32G4 Series advanced Arm®-based 32-bit MCUs*
- **Verified Key Chapters for Bare-Metal Firmware:**
  - **Ch 2:** Memory and bus architecture (Multi-layer AHB BusMatrix, 128 KB SRAM including CCM-SRAM `0x10000000` / `0x20018000` alias, §2.3 Bit-banding)
  - **Ch 3:** Embedded Flash memory (Wait states Table 9, ART caches, Dual-Bank `DBANK` 2 KB vs. Single-Bank 4 KB, 64-bit writes, 8-bit ECC `FLASH_ECCR`, RDP/WRP/PCROP/Securable Memory `SEC1R`)
  - **Ch 6:** Power control (**PWR** — Range 1 Boost `PWR_CR5 @ 0x80`, `PWR_SR2.VOSF`, §6.1.5 `HPRE=/2` transition, Sleep/LPRun/LPSleep/Stop 0/Stop 1/Standby/Shutdown, `UCPD_DBDIS`)
  - **Ch 7:** Reset and clock control (**RCC** — HSI16, HSE, HSI48, LSI, LSE, Main PLL, `CCIPR`/`CCIPR2`, CSS, MCO, `CSR` reset flags)
  - **Ch 8:** Clock recovery system (**CRS** — crystal-less HSI48 USB trimming)
  - **Ch 9:** General-purpose I/Os (**GPIO** — `11` Analog reset default, `BSRR`, `BRR`, `LCKR`, `AFR`)
  - **Ch 10:** System configuration controller (**SYSCFG** — `EXTICR`, `SCSR`, `CFGR2` safety lockup, `SWPR`)
  - **Ch 12:** Direct memory access controller (**DMA1/DMA2** — 16 channels)
  - **Ch 13:** DMA request multiplexer (**DMAMUX1** — Table 91 Request IDs, Sync, Request Generators)
  - **Ch 15:** Extended interrupt and event controller (**EXTI** — Lines 0–43)
  - **Ch 16 / 17 / 18:** **CRC** (Ch 16), **CORDIC** (Ch 17), **FMAC** (Ch 18)
  - **Ch 21 / 22 / 23 / 24 / 25:** **ADC1–5** (Ch 21), **DAC1–4** (Ch 22), **VREFBUF** (Ch 23), **COMP1–7** (Ch 24), **OPAMP1–6** (Ch 25)
  - **Ch 26:** **QUADSPI** (`0xA0001000`, XIP `0x90000000`)
  - **Ch 27:** High-resolution timer (**HRTIM1** — 184 ps DLL, Master + TIMA–TIMF)
  - **Ch 28–32:** Timers — **Ch 28** Adv `TIM1/8/20`, **Ch 29** GP `TIM2–5`, **Ch 30** GP `TIM15–17`, **Ch 31** Basic `TIM6/7`, **Ch 32** `LPTIM1`
  - **Ch 33 / 34 / 35 / 36 / 38:** **RNG** (Ch 33), **RTC** (Ch 34), **IWDG** (Ch 35), **WWDG** (Ch 36), **TAMP** (Ch 38 — 32 Backup Registers)
  - **Ch 37–42 (Rev 8 vs. Rev 9):** **USART/UART** (Rev 8 Ch 37 / Rev 9 Ch 40), **LPUART** (Rev 8 Ch 38 / Rev 9 Ch 41), **I2C** (Rev 8 Ch 39 / Rev 9 Ch 41–42), **SPI/I2S** (Rev 8 Ch 40 / Rev 9 Ch 42), **SAI1** (Ch 41)
  - **Ch 43 / 44 / 45 / 47:** **USB FS** (Ch 43), **FDCAN1–3** (Ch 44), **UCPD1** (Ch 45), **DBGMCU** (Ch 47)

#### B. STM32G474xB/xC/xE Datasheet (`DS12288`)
- **Document Number:** **DS12288** (Rev 6 or latest)
- **Title:** *Arm® Cortex®-M4 32-bit MCU+FPU, 512 KB Flash, 170 MHz/213 DMIPS, 128 KB SRAM*
- **Verified Key Tables & Electrical Limits:**
  - **Table 2:** *Device features and peripheral counts by package* (confirms LQFP64 `STM32G474RET6` has **52 GPIOs**, **3 SPIs [no SPI4]**, **4 I2Cs**, **3 FDCANs**, **0 FMC**, and **NO hardware AES/CRYP** — AES exists only on STM32G483/G484!)
  - **Table 12:** *STM32G474xB/xC/xE pin definitions* (LQFP64 physical pin numbers & 5 V-tolerant `FT` flags)
  - **Table 13:** ***Alternate function mapping*** (`AF0–AF15` for `GPIOx->AFR[0..1]`)
  - **Table 14:** ***Voltage characteristics*** ($V_{\text{DD}}$, $V_{\text{DDA}}$, absolute max ratings)
  - **Table 15:** ***Current characteristics*** (I/O pin current limit $\pm 20\ \text{mA}$, total $I_{\text{VDD}}/I_{\text{VSS}} \le 160\ \text{mA}$)
  - **§5.3.5 / §5.3.9–5.3.10 / §5.3.19:** Supply currents ($I_{DD}$), PLL limits ($f_{\text{PLL\_IN}} \in [2.66, 16]\ \text{MHz}$, $f_{\text{VCO}} \in [96, 344]\ \text{MHz}$), Sleep wakeup (`t_WUSLEEP` = 11–12 cycles), ADC max clock (**60 MHz** single / **52 MHz** multi-ADC, temp sensor $t_S \ge 5\ \mu\text{s}$).

#### C. STM32G474xx Silicon Errata Sheet (`ES0430`) — MANDATORY
- **Document Number:** **ES0430**
- **Title:** *STM32G471xx/473xx/474xx/483xx/484xx device errata*
- **Critical Workarounds Every Bare-Metal Engineer Must Apply:**
  - **§2.1.2 / §2.1.3 (Cortex-M4 Core / ARM 838869):** Execute `__DSB()` before returning from an ISR after clearing a peripheral interrupt flag; avoid 1–2 instruction ISRs interrupting `VDIV`/`VSQRT`.
  - **§2.2.1 (Debug):** Always use **SWD** (`PA13`/`PA14`), not full JTAG without `NJTRST`.
  - **§2.2.2–2.2.3 (Flash):** Flush DCache (`FLASH_ACR.DCRST`) after Flash writes during RWW.
  - **§2.3.1 (DMA — CRITICAL):** **NEVER clear global `DMA_IFCR.CGIFx`**; always clear individual `CTCIFx`/`CHTIFx`/`CTEIFx` bits to prevent transfer-error lockup.

#### D. Programming & Board User Manuals (`PM0214`, `UM2505`, `UM2553`)
- **`PM0214`:** *STM32 Cortex®-M4 MCUs and MPUs programming manual* (Core registers, Thumb-2 ISA, NVIC, SCB faults, SysTick, MPU, FPU).
- **`UM2505`:** *STM32G4 Nucleo-64 boards (MB1367)* — Schematics, **`JP6` ($I_{DD}$ current measurement)** vs. **`JP5` (5V source selector)**, **Active-HIGH** User Button `B1` (`PC13` external pull-down), Green LED `LD2` (`PA5` via `SB6`), **24 MHz HSE crystal `X3`**, `PA2`/`PA3` ST-LINK/V3E VCP (`SB17`/`SB23`), `PB8-BOOT0` 10 kΩ pull-down (`R36`).
- **`UM2553`:** *STM32CubeIDE quick start guide* (IDE reference only; not a board manual).

---

### 2. Core ST Application Notes for STM32G474RE Bare-Metal Mastery

| App Note | Official ST Title | Relevance to STM32G474RE Curriculum |
|---|---|---|
| **`AN2606`** | *STM32 microcontroller system memory boot mode* | Factory ROM bootloader (`0x1FFF0000`: USART1–3, I2C2–4, SPI1–3, FDCAN1, USB DFU), `nSWBOOT0`/`nBOOT0`/`PB8-BOOT0` |
| **`AN2834`** | *How to get the best ADC accuracy in STM32 microcontrollers* | RC source impedance settling ($R_{\text{AIN}} \cdot C_{\text{ADC}}$), calibration, `VREFINT`, PCB layout & oversampling |
| **`AN4013`** | *STM32 cross-series timer overview* | Cross-family timer comparison, encoder mode, complementary PWM, break inputs, master/slave sync |
| **`AN4539`** | *HRTIM cookbook* | Essential STM32G474RE **HRTIM1** 184 ps DLL calibration, buck/boost/LLC topologies, blanking & faults |
| **`AN4899`** | *STM32 microcontroller GPIO hardware settings and low-power consumption* | Why STM32G4 defaults to `11` Analog mode, floating-pin leakage, slew rate/EMI, `OSPEEDR` tuning |
| **`AN4989`** | *STM32 microcontroller debug toolbox* | SWD, FPB, DWT watchpoints/`CYCCNT`, ITM/SWO live trace (`PB3`), HardFault debugging |
| **`AN5093`** | *Getting started with STM32G4 Series hardware development* | Decoupling capacitors, crystal `X3` load capacitance, `VREF+`/`VDDA` filtering, `BOOT0`/`NRST` design |
| **`AN5305`** | *Digital filter implementation with the FMAC using STM32CubeG4* | **FMAC** `q1.15` FIR/IIR and 3p3z digital power compensator implementation |
| **`AN5306`** | *Operational amplifier (OPAMP) usage in STM32G4 Series* | **OPAMP1–6** PGA gains (×2..×64), `TCMR` synchronous muxing, internal ADC routing (`OPAINTOEN`) |
| **`AN5310`** | *Guideline for using analog features of STM32G4 Series versus STM32F3 Series* | Migration and precision tuning of STM32G4 ADC, DAC sawtooth/high-speed `HFSEL`, COMP, OPAMP |
| **`AN5325`** | *How to use CORDIC to perform mathematical functions using STM32CubeG4* | **CORDIC** `q1.31`/`q1.15` trigonometric/hyperbolic scaling, precision selection, DMA streaming |
| **`AN5348`** | *FDCAN peripheral on STM32 devices* | **FDCAN1–3** nominal/data bit timing (`NBTP`/`DBTP`), Message RAM partitioning, hardware filters |
| **`AN5394`** | *Getting started with projects based on the STM32L5 Series in STM32CubeIDE* | Comparative ARMv8-M TrustZone-M reference (illustrates `ARM-SEC-002` boundary vs. ARMv7E-M STM32G4) |

---

### 3. Comparative STM32 Family Reference Manuals
- **`RM0008` (STM32F103):** Legacy 72 MHz Cortex-M3 (`CRL`/`CRH` GPIO, fixed DMA channels, single `USART_DR`).
- **`RM0390` (STM32F446):** 180 MHz Cortex-M4 (`HSION` bit 0, `USART_DR`, no DMAMUX, no `PWR_CR5` Boost).
- **`RM0444` (STM32G0):** Cortex-M0+ single-cycle I/O, simplified clock tree, no `BASEPRI`/FPU/`LDREX`.
- **`RM0433` (STM32H742/743/753/750):** **Single-core** 480 MHz Cortex-M7 with 64-bit AXI bus and L1 caches.
- **`RM0399` (STM32H745/755/747/757):** **Dual-core** Cortex-M7 + Cortex-M4 asymmetric multiprocessor architecture.
