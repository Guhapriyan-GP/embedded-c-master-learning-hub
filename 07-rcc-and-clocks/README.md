# Stage 06: Power, Reset & Clock Control (RCC) Engine

> **Directory:** `07-rcc-and-clocks/` | **Target Platform:** STM32G474RET6 (LQFP64) on NUCLEO-G474RE (MB1367) | **Core:** ARM Cortex-M4F @ up to 170 MHz  
> **Code Verification Notice:** All bare-metal C snippets in this module are **Statically Reviewed against `stm32g474xx.h`, RM0440, DS12288, and UM2505 (Not Tested on Physical Hardware)**.

---

## 1. Learning Objectives & Concept Scope

By completing this module, you will master the complete STM32G474RE clock synthesis, voltage-scaling, Flash-latency, and reset-diagnostics pipeline:

| Concept ID | Concept Name | Architectural Scope | Mastery Tier |
|---|---|---|---|
| `STM32-RCC-001` | Oscillator Architecture: HSI16, HSE (24 MHz X3 Crystal), LSI (32 kHz), LSE (32.768 kHz), HSI48 | `[STM32G474RE Exact]` | Level 1 (Essential) |
| `STM32-RCC-002` | Main PLL ($M, N, P, Q, R$) Synthesis, Dual Clock Profiles (**170 MHz Max Boost** vs **160 MHz Clean Integer Boost** vs **150 MHz Normal**), `PWR->CR5.R1MODE`, Flash 4 WS, & Safe `HPRE=/2` Switch | `[STM32G474RE Exact]` | Level 2 (Production) |
| `STM32-RCC-003` | Peripheral Kernel Clock Muxing (`CCIPR`/`CCIPR2`), Clock Security System (CSS), MCO (`PA8`), & Reset Cause Diagnostics (`RCC->CSR`) | `[STM32G474RE Exact]` | Level 2 (Production) |
| `STM32-PWR-001` | Core Voltage Regulator Scaling (Range 1 Boost, Range 1 Normal, Range 2) & `PWR->SR2.VOSF` Polling | `[STM32G474RE Exact]` | Level 2 (Production) |
| `STM32-FLASH-001` | Flash Wait States (`FLASH->ACR.LATENCY`) & ART Accelerator (`PRFTEN`, `ICEN`, `DCEN`) Clock Coupling | `[STM32G474RE Exact]` | Level 2 (Production) |

---

## 2. Prerequisites & Curriculum Positioning

### Prerequisite Concept IDs
- **Stage 02 (`03-embedded-c/`):** `EMB-MMIO-001` (Volatile memory-mapped register access and read-modify-write masking).
- **Stage 04 (`05-toolchain-and-debugging/`):** `TOOL-GDB-001` (GDB/OpenOCD register inspection) and `TOOL-START-001` (`SystemInit()` leaves `SYSCLK = 16 MHz` HSI16).
- **Stage 05 (`06-stm32-fundamentals/`):** `STM32-ARCH-001` (AHB1/AHB2/APB1/APB2 BusMatrix and RM0440/DS12288 navigation).
- **For MCO Verification & Lab 03 Only:** `PERIPH-GPIO-001` from **Stage 07 (`08-gpio/`)** (basic GPIO Alternate Function setup on `PA8`).

### Pedagogical Sequencing Note (Two-Phase Execution)
To eliminate circular dependencies between RCC and GPIO:
1. **Phase A (Level 1 — Before `08-gpio/` Labs 01–02):** Read Sections 3.1 and 3.5 to understand the reset default **16 MHz HSI16** clock and **RCC peripheral clock gating** (`RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN` + synchronization dummy read). Complete **Labs 01–02** in [`08-gpio/`](../08-gpio/README.md) on default 16 MHz HSI16.
2. **Phase B (Level 2 — After Basic GPIO):** Return here to master the full **24 MHz HSE $\to$ 170 MHz PLL Range 1 Boost** bring-up sequence, Flash 4 WS, `CCIPR`/`CCIPR2`, CSS, and MCO output on `PA8`, then complete **Lab 03 (`lab-03-rcc-pll-170mhz-clock-switch`)**.

---

## 3. Core Technical Theory & Engineering Principles

### 3.1 Five Independent Clock Sources `[STM32G474RE Exact]`
Every STM32G474RE boots with **HSI16** selected as `SYSCLK` (`16 MHz`), all peripheral clocks gated OFF (except Flash and core infrastructure) to minimize power consumption.

| Oscillator | Nominal Frequency | Accuracy / Type | Primary Role | NUCLEO-G474RE (MB1367, UM2505) Hardware Truth |
|---|---|---|---|---|
| **HSI16** | `16 MHz` | $\pm 1\%$ factory-trimmed RC | Reset default `SYSCLK`, Stop-mode wakeup clock, UCPD1/Flash programming clock | Internal silicon RC; **must remain ON during Flash erase/program** per RM0440. |
| **HSE** | `4–48 MHz` | External quartz crystal or clock bypass (`HSEBYP`) | Precision PLL reference for UART, FDCAN, Timers, USB | **Default on MB1367 Rev C: onboard 24 MHz crystal `X3`** connected to `PF0-OSC_IN` / `PF1-OSC_OUT` (NOT ST-LINK MCO unless solder bridges are reconfigured per UM2505 §6.7!). |
| **LSI** | `~32 kHz` | Low-power internal RC ($\pm 5\%$ drift: `30.4–33.6 kHz`) | Independent Watchdog (`IWDG`), RTC/LPTIM1 low-cost clock | Internal silicon RC; auto-started when IWDG is activated. |
| **LSE** | `32.768 kHz` | External watch crystal (`PC14-OSC32_IN` / `PC15-OSC32_OUT`) | Precision RTC calendar, LPTIM1 tickless idle, LPUART1 9600 baud Stop wakeup | **Onboard 32.768 kHz crystal `X2`** populated by default on NUCLEO-G474RE. |
| **HSI48** | `48 MHz` | Internal RC auto-trimmed by CRS | Crystal-less USB FS Device & Hardware `RNG` (`CLK48SEL`) | Enabled via `RCC->CRRCR.HSI48ON` (bit 0); trimmed to $\pm 0.25\%$ against USB SOF by `CRS`. |

### 3.2 Main PLL Architecture & DS12288 Electrical Constraints `[STM32G474RE Exact]`
The Main PLL takes **HSI16** (`PLLSRC = 10b`) or **HSE** (`PLLSRC = 11b`), divides by input prescaler $M \in [1, 16]$, multiplies by $N \in [8, 127]$ inside a Voltage-Controlled Oscillator (VCO), and produces three independent output clocks:
- **`PLL_R` (`PLLREN=1`, $R \in \{2, 4, 6, 8\}$):** Drives `SYSCLK` up to **170 MHz** (Range 1 Boost).
- **`PLL_Q` (`PLLQEN=1`, $Q \in \{2, 4, 6, 8\}$):** Drives `FDCAN1–3`, `USB`/`RNG` (48 MHz), `QUADSPI`, and `SAI1`/`I2S` ($\le 170\text{ MHz}$).
- **`PLL_P` (`PLLPEN=1`, $P \in [2, 31]$ via `PLLPDIV`, or `7`/`17` via `PLLP` bit):** Drives asynchronous `ADC1–5` kernel clocks ($\le 60\text{ MHz}$ single / $\le 52\text{ MHz}$ multi-ADC).

**Strict DS12288 Operating Limits (NEVER violate!):**
- **PLL VCO Input Frequency ($f_{\text{PLL\_IN}} = f_{\text{IN}} / M$):** **$2.66\text{ MHz} \le f_{\text{PLL\_IN}} \le 16.0\text{ MHz}$** (note: `2.66 MHz`, NOT 4 MHz).
- **PLL VCO Output Frequency ($f_{\text{VCO}}$):**
  - Range 1 (Normal or Boost): **$96\text{ MHz} \le f_{\text{VCO}} \le 344\text{ MHz}$** (note: `96 MHz` minimum, NOT 64 MHz!).
  - Range 2: **$96\text{ MHz} \le f_{\text{VCO}} \le 128\text{ MHz}$**.
- **Output Frequency Limits:**
  - Range 1 Boost (`VOS=01`, `R1MODE=0`): $f_{\text{SYSCLK}} \le \mathbf{170\text{ MHz}}$.
  - Range 1 Normal (`VOS=01`, `R1MODE=1`): $f_{\text{SYSCLK}} \le \mathbf{150\text{ MHz}}$.
  - Range 2 (`VOS=10`): $f_{\text{SYSCLK}} \le \mathbf{26\text{ MHz}}$.

### 3.3 Core Voltage Scaling, Flash Latency & Mandatory 170 MHz Switch Sequence `[STM32G474RE Exact]`
Operating above 150 MHz requires raising the internal $V_{\text{CORE}}$ regulator from **Range 1 Normal (~1.2 V)** to **Range 1 Boost (~1.28 V)** by clearing `PWR->CR5.R1MODE = 0` (`PWR_CR5` is at offset **`0x80`**, bit 8) while `PWR->CR1.VOS = 01b`, and polling **`PWR->SR2.VOSF == 0`** (`PWR_SR2` is at offset **`0x14`**, bit 10).

Simultaneously, Embedded Flash read latency (`FLASH->ACR.LATENCY`) MUST be increased **before** raising `SYSCLK` (and decreased only **after** lowering `SYSCLK`):

| Voltage Scaling Mode | `0 WS` | `1 WS` | `2 WS` | `3 WS` | `4 WS` |
|---|---|---|---|---|---|
| **Range 1 Boost (`R1MODE=0`, $\le 170\text{ MHz}$)** | $\le 34\text{ MHz}$ | $\le 68\text{ MHz}$ | $\le 102\text{ MHz}$ | $\le 136\text{ MHz}$ | **$\le 170\text{ MHz}$** |
| **Range 1 Normal (`R1MODE=1`, $\le 150\text{ MHz}$)** | $\le 30\text{ MHz}$ | $\le 60\text{ MHz}$ | $\le 90\text{ MHz}$ | $\le 120\text{ MHz}$ | $\le 150\text{ MHz}$ |
| **Range 2 (`VOS=10`, $\le 26\text{ MHz}$)** | $\le 12\text{ MHz}$ | $\le 24\text{ MHz}$ | $\le 26\text{ MHz}$ | — | — |

#### Mandatory RM0440 §6.1.5 Intermediate `HPRE = /2` Stabilization Step (Applies to BOTH 170 MHz AND 160 MHz!)
Jumping directly from 16 MHz (`HPRE = /1`) to any frequency **$> 150\text{ MHz}$** (both **170 MHz** AND **160 MHz**!) causes a sudden $di/dt$ current transient on $V_{\text{CORE}}$ that can cause voltage droop and core lockup. Per **RM0440 §6.1.5 (*Dynamic voltage scaling management*)**, transitioning to $>150\text{ MHz}$ MUST follow this exact sequence:
1. Enable `PWREN`, ensure `VOS = 01b` (Range 1), poll `while (PWR->SR2 & PWR_SR2_VOSF) {}`.
2. Clear `PWR->CR5 &= ~PWR_CR5_R1MODE;` (select Range 1 Boost — required for **both 170 MHz and 160 MHz**!).
3. Configure `FLASH->ACR` for **4 WS** (`FLASH_ACR_LATENCY_4WS` — RM0440 Table 9 mandates 4 WS for `> 136 MHz` up to `170 MHz` in Boost Mode!) + `PRFTEN | ICEN | DCEN` and read back `FLASH->ACR` until `LATENCY == 4`.
4. Start HSE (24 MHz X3) or use HSI16; configure `PLLCFGR`, start PLL, wait `PLLRDY`.
5. **Set AHB Prescaler `HPRE = /2`** (`RCC_CFGR_HPRE_DIV2`, giving **85 MHz** for Profile A or **80 MHz** for Profile B intermediate HCLK).
6. **Switch `SYSCLK` to PLL** (`RCC->CFGR |= RCC_CFGR_SW_PLL`) and poll `SWS == RCC_CFGR_SWS_PLL`.
7. **Wait at least $1\ \mu\text{s}$** ($\ge 85$ cycles at 85 MHz or $\ge 80$ cycles at 80 MHz; safely 100–120 iterations).
8. **Restore AHB Prescaler `HPRE = /1`** (`HCLK = PCLK1 = PCLK2 = 170 MHz` or `160 MHz`).
9. Call `SystemCoreClockUpdate()` (or set `SystemCoreClock = 170000000UL` / `160000000UL`).

#### 3.3.1 Dual Production Clock Profiles: 170 MHz Max vs 160 MHz Clean Integer vs 150 MHz Normal `[STM32G474RE Exact]`

While **170 MHz** delivers maximum STM32G474RE throughput and **183.8 ps** `HRTIM1` resolution, many production control/DSP systems deliberately standardize on **160 MHz** for clean integer power-of-2 bus/ADC/timer/SPI divisions — or **150 MHz** to remain in lower-power **Range 1 Normal Mode** (`R1MODE = 1`). Never confuse the hardware requirements of **160 MHz** with **150 MHz**:

| Parameter / Peripheral | **Profile A: 170 MHz Max (Boost)** | **Profile B: 160 MHz Clean Integer (Boost)** | **Profile C: 150 MHz Normal (No Boost)** |
|---|---|---|---|
| **Voltage Scaling Mode** | **Range 1 Boost (`PWR->CR5.R1MODE = 0`)** | **Range 1 Boost (`PWR->CR5.R1MODE = 0`)** *(STILL $> 150\text{ MHz}$!)* | **Range 1 Normal (`PWR->CR5.R1MODE = 1`)** *(Lower $I_{\text{DD}}$!)* |
| **Flash Wait States (RM0440 Table 9)** | **`4 WS`** (`> 136 MHz` Boost) | **`4 WS`** (`> 136 MHz` Boost — NOT 3 WS!) | **`4 WS`** (`> 120 MHz` Normal) |
| **RM0440 §6.1.5 `HPRE = /2` Step** | **Mandatory** (`85 MHz` for $\ge 1\ \mu\text{s}$) | **Mandatory** (`80 MHz` for $\ge 1\ \mu\text{s}$) | **Not required** ($\le 150\text{ MHz}$ Normal) |
| **PLL from `HSI16` (16 MHz)** | `M=4` (`PLLM=3`), `N=85`, `R=2` (`VCO=340 MHz`) | `M=4` (`PLLM=3`), `N=80`, `R=2` (`VCO=320 MHz`) | `M=4` (`PLLM=3`), `N=75`, `R=2` (`VCO=300 MHz`) |
| **PLL from `HSE` (24 MHz X3)** | `M=6` (`PLLM=5`), `N=85`, `R=2` (`VCO=340 MHz`) | `M=3` (`PLLM=2`), `N=40`, `R=2` *or* `M=6, N=80, R=2` (`VCO=320 MHz`) | `M=6` (`PLLM=5`), `N=75`, `R=2` *or* `M=2, N=25, R=2` (`VCO=300 MHz`) |
| **`HCLK` / `PCLK1` / `PCLK2`** | `170.0 / 170.0 / 170.0 MHz` | `160.0 / 160.0 / 160.0 MHz` | `150.0 / 150.0 / 150.0 MHz` |
| **1 µs Timer Tick (`PSC`)** | `PSC = 169` (`170 MHz / 170 = 1 MHz`) | **`PSC = 159`** (`160 / 80 / 40 / 20 / 10 / 1 MHz` exact!) | `PSC = 149` (`150 MHz / 150 = 1 MHz`) |
| **Sync ADC (`CKMODE = 11b`, `HCLK/4`)** | `42.5 MHz` ($\le 60\text{ MHz}$ single / $\le 52\text{ MHz}$ multi) | **`40.000 MHz`** (exact `1.000 MSPS` @ 40 cyc; `4.000 MSPS` @ 10 cyc!) | `37.5 MHz` (or async `PLL_P = 300/6 = 50.0 MHz`) |
| **`DACx->MCR.HFSEL` Setting** | **`10b`** (`> 160 MHz` required!) | **`01b`** (`> 80 MHz` and $\le 160\text{ MHz}$) | **`01b`** (`> 80 MHz` and $\le 160\text{ MHz}$) |
| **`USART` 115,200 baud (`BRR`)** | `BRR = 1476` (`0x05C4`, `-0.021%` err) | **`BRR = 1389`** (`0x056D`, **`-0.008%` err**) | `BRR = 1302` (`0x0516`, `+0.006%` err) |
| **`SPI` `/4` & `/8` SCK Rates** | `42.5 MHz` / `21.25 MHz` | **`40.0 MHz` / `20.0 MHz`** (clean integer MHz!) | `37.5 MHz` / `18.75 MHz` |
| **`MCO` (`PA8`, `SYSCLK / 16`)** | `10.625 MHz` | **`10.000 MHz`** (clean scope reference!) | `9.375 MHz` |
| **`HRTIM1` ×32 DLL Resolution** | **`5.44 GHz` (`183.8 ps`)** | `5.12 GHz` (`195.3 ps`) | `4.80 GHz` (`208.3 ps`) |
| **NRZ SWO 2 MHz (`TPI->ACPR`)** | `84` (`0x54`, 0.000% err) | **`79` (`0x4F`, 0.000% err)** (`39` @ 4 MHz) | `74` (`0x4A`, 0.000% err) |
| **48 MHz USB / RNG Clock** | `HSI48 + CRS` (`340 / 48` non-integer) | `HSI48 + CRS` (`320 / 48` non-integer) | `HSI48 + CRS` (`300 / 48` non-integer) |

### 3.4 Bus Prescalers, Timer $\times 2$ Rule & Independent Kernel Clocks (`CCIPR`/`CCIPR2`)
- **AHB (`HPRE`):** Divides `SYSCLK` by `/1, /2, /4, ..., /512` to produce `HCLK` (CPU, DMA, SRAM, Flash, AHB1/2/3 peripherals).
- **APB1 (`PPRE1`) & APB2 (`PPRE2`):** Divide `HCLK` by `/1, /2, /4, /8, /16` to produce `PCLK1` and `PCLK2` (both rated up to **170 MHz** on STM32G474RE!).
- **APB Timer Clock Multiplier Rule:** If `PPREx == /1`, `TIMxCLK = PCLKx`; if `PPREx >= /2`, **`TIMxCLK = 2 * PCLKx`**.
- **Peripheral Kernel Clock Multiplexers (`RCC->CCIPR` @ `0x88`, `RCC->CCIPR2` @ `0x9C`):** Decouple peripheral kernel clocks from APB bus clocks so `USART1–3`, `UART4–5`, `LPUART1`, `I2C1–4`, `LPTIM1`, `SAI1`, `I2S23`, `FDCAN`, `CLK48` (USB/RNG), `ADC12`, `ADC345`, and `QUADSPI` can run on HSI16, LSE, HSE, PLLQ, PLLP, or SYSCLK independently of `HCLK` changes.

### 3.5 Clock Gating, Clock Security System (CSS), MCO & Reset Cause Flags
- **Peripheral Clock Gating (`AHB1ENR`, `AHB2ENR`, `AHB3ENR`, `APB1ENR1`, `APB1ENR2`, `APB2ENR`):** Every peripheral defaults to clock-OFF at reset. After enabling any peripheral clock bit, execute a **read-back dummy read** (e.g., `(void)RCC->AHB2ENR;`) to satisfy the 2-clock-cycle AHB/APB synchronization delay before writing peripheral registers!
- **Clock Security System (`RCC->CR.CSSON`, bit 19):** Hardware monitor on HSE. If the 24 MHz crystal stops or shorts, hardware automatically switches `SYSCLK` back to **HSI16**, disables HSE/PLL, sets `RCC->CIFR.CSSF`, and fires a **Non-Maskable Interrupt (`NMI_Handler`, `NonMaskableInt_IRQn = -14`)** where firmware can force safe-state outputs and clear `RCC->CICR.CSSC`. (LSE CSS uses `RCC->BDCR.LSECSSON` and `RTC_TAMP_LSECSS_IRQn = 2`.)
- **Microcontroller Clock Output (`MCO`):** Routes internal clocks (`SYSCLK`, `HSI16`, `HSE`, `PLL_R`, `LSI`, `LSE`, `HSI48`) divided by `MCOPRE` (`/1, /2, /4, /8, /16`) onto **`PA8` (AF0)** (or `PG10`) for direct oscilloscope verification without altering code timing.
- **Reset Cause Register (`RCC->CSR` @ `0x94`):** Latches sticky flags across resets: `LPWRRSTF` (b31), `WWDGRSTF` (b30), `IWDGRSTF` (b29), `SFTRSTF` (b28), `BORRSTF` (b27), `PINRSTF` (b26), `OBLRSTF` (b25). Read on boot for telemetry, then clear all flags by pulsing `RCC->CSR |= RCC_CSR_RMVF` (b23) followed by `RCC->CSR &= ~RCC_CSR_RMVF`.

---

## 4. Core Terminology & Acronyms

| Term / Acronym | Precise Engineering Definition |
|---|---|
| **HSI16 / HSE / LSI / LSE / HSI48** | High-Speed Internal (16 MHz), High-Speed External (24 MHz X3), Low-Speed Internal (~32 kHz), Low-Speed External (32.768 kHz X2), High-Speed Internal 48 MHz. |
| **PLL / VCO** | Phase-Locked Loop / Voltage-Controlled Oscillator multiplying reference clock $f_{\text{IN}}/M$ by $N$ within `[96, 344] MHz` (Range 1). |
| **Range 1 Boost (`R1MODE=0`)** | Elevated $V_{\text{CORE}}$ regulator mode (~1.28 V) mandatory for STM32G474RE operation in `(150, 170] MHz`. |
| **Wait States (WS)** | Extra CPU stall cycles inserted when Flash read access time exceeds one `HCLK` period (`4 WS` at 170 MHz). |
| **ART Accelerator** | Adaptive Real-Time Flash accelerator combining 64-bit prefetch (`PRFTEN`), 1 KB instruction cache (`ICEN`), and 256 B data cache (`DCEN`). |
| **CSS / NMI** | Clock Security System — hardware HSE failure detector triggering an unblockable Non-Maskable Interrupt (`IRQn = -14`) and HSI16 fallback. |
| **MCO** | Microcontroller Clock Output (`PA8`, AF0) — physical pin outputting divided internal clock signals for oscilloscope verification. |
| **Backup Domain (`BDCR`)** | VBAT-powered RCC domain controlling LSE and RTC clocks; write-protected until `PWR->CR1.DBP = 1`. |

---

## 5. Hardware Register Architecture & Bitfield Reference

All addresses, offsets, and bit positions below are verified directly against `stm32g474xx.h`.

### 5.1 `RCC_TypeDef` (`RCC_BASE = 0x40021000`, AHB1)
| Register | Offset | Reset Value | Critical Bitfields & Verified STM32G4 Positions |
|---|---|---|---|
| **`RCC->CR`** | `0x00` | `0x00000100` (transitions to `0x00000500` once `HSIRDY=1`) | **`HSION` = Bit 8** (NOT Bit 0!), **`HSIRDY` = Bit 10** (NOT Bit 1!), `HSEON` = Bit 16, `HSERDY` = Bit 17, `HSEBYP` = Bit 18, `CSSON` = Bit 19, `PLLON` = Bit 24, `PLLRDY` = Bit 25 |
| **`RCC->CFGR`** | `0x08` | `0x00000005` (`SW=01`, `SWS=01`) | **`SW[1:0]`**: `00=Reserved`, **`01=HSI16`**, **`10=HSE`**, **`11=PLL`**; `SWS[3:2]` (mirrors `SW`); `HPRE[7:4]`; `PPRE1[10:8]`; `PPRE2[13:11]`; `MCOSEL[27:24]`; `MCOPRE[30:28]` |
| **`RCC->PLLCFGR`** | `0x0C` | `0x00001000` | `PLLSRC[1:0]` (`10=HSI16`, `11=HSE`); `PLLM[7:4]` (encodes $M-1$, `0..15`); `PLLN[14:8]` (`8..127`); `PLLPEN` (b16); `PLLP` (b17); `PLLQEN` (b20); `PLLQ[22:21]` (`00=/2..11=/8`); `PLLREN` (b24); `PLLR[26:25]` (`00=/2..11=/8`); `PLLPDIV[31:27]` (`2..31`) |
| **`RCC->CIER/CIFR/CICR`** | `0x18/1C/20` | `0x00000000` | Clock interrupts & CSS flags (`CSSF` b8 in `CIFR`, `CSSC` b8 in `CICR`) |
| **`RCC->AHB1ENR..APB2ENR`** | `0x48..0x60` | `0x00000100` (`AHB1ENR.FLASHEN`) | Peripheral clock enables (`AHB2ENR` GPIO/ADC/DAC/RNG, `APB1ENR1` PWR/TIM2–7/USART2–5/I2C1–3/FDCAN/USB/LPTIM1, `APB2ENR` SYSCFG/TIM1/8/15–17/20/SPI1/USART1/SAI1/HRTIM1) |
| **`RCC->CCIPR`** | `0x88` | `0x00000000` | Independent kernel clocks: `USART1..3SEL`, `UART4..5SEL`, `LPUART1SEL`, `I2C1..3SEL`, `LPTIM1SEL`, `SAI1SEL`, `I2S23SEL`, `FDCANSEL[25:24]`, `CLK48SEL[27:26]`, `ADC12SEL[29:28]`, `ADC345SEL[31:30]` |
| **`RCC->BDCR`** | `0x90` | `0x00000000` | `LSEON` (b0), `LSERDY` (b1), `LSEBYP` (b2), `LSEDRV[4:3]`, `LSECSSON` (b5), `RTCSEL[9:8]`, `RTCEN` (b15), `BDRST` (b16) |
| **`RCC->CSR`** | `0x94` | Reset-dependent | `LSION` (b0), `LSIRDY` (b1), `RMVF` (b23), `OBLRSTF` (b25), `PINRSTF` (b26), `BORRSTF` (b27), `SFTRSTF` (b28), `IWDGRSTF` (b29), `WWDGRSTF` (b30), `LPWRRSTF` (b31) |
| **`RCC->CRRCR`** | `0x98` | `0x00000000` | `HSI48ON` (b0), `HSI48RDY` (b1), `HSI48CAL[15:7]` |
| **`RCC->CCIPR2`** | `0x9C` | `0x00000000` | `I2C4SEL[1:0]`, `QSPISEL[21:20]` |

### 5.2 Coupled `PWR` (`0x40007000`) & `FLASH` (`0x40022000`) Registers
| Register | Offset | Critical Bitfields |
|---|---|---|
| **`PWR->CR1`** | `0x00` | `VOS[10:9]`: `01=Range 1` (reset default), `10=Range 2`; `DBP` (b8) unlocks `RCC->BDCR`/RTC/TAMP |
| **`PWR->SR2`** | **`0x14`** | **`VOSF` = Bit 10** (`1` = regulator transitioning, `0` = voltage settled; NOTE: lives in `PWR->SR2`, NOT `PWR->SR1@0x10`!) |
| **`PWR->CR5`** | **`0x80`** | **`R1MODE` = Bit 8** (`0` = Range 1 Boost $\le 170\text{ MHz}$, `1` = Range 1 Normal $\le 150\text{ MHz}$; reset = `1`) |
| **`FLASH->ACR`** | `0x00` | `LATENCY[3:0]` (`0..4 WS`), `PRFTEN` (b8), `ICEN` (b9), `DCEN` (b10) |

---

## 6. Engineering Mathematics & Worked Formulas

### 6.1 Main PLL Frequency Synthesis Equations
$$f_{\text{PLL\_IN}} = \frac{f_{\text{IN}}}{M}, \quad f_{\text{PLL\_IN}} \in [2.66,\ 16.0]\text{ MHz}$$
$$f_{\text{VCO}} = f_{\text{PLL\_IN}} \times N = f_{\text{IN}} \times \frac{N}{M}, \quad f_{\text{VCO}} \in [96.0,\ 344.0]\text{ MHz}\text{ (Range 1)}$$
$$f_{\text{PLL\_R}}\ (\text{SYSCLK}) = \frac{f_{\text{VCO}}}{R}, \quad f_{\text{PLL\_Q}} = \frac{f_{\text{VCO}}}{Q}, \quad f_{\text{PLL\_P}} = \frac{f_{\text{VCO}}}{P}$$

### 6.2 Worked Example A: Exact 170 MHz from NUCLEO-G474RE 24 MHz HSE Crystal (`X3`)
- Given $f_{\text{IN}} = 24\text{ MHz}$ (`HSE`), choose $M = 6$ (`PLLM = 5`):
  $$f_{\text{PLL\_IN}} = \frac{24\text{ MHz}}{6} = 4.0\text{ MHz} \in [2.66,\ 16.0]\text{ MHz} \quad \checkmark$$
- Choose $N = 85$ (`PLLN = 85`):
  $$f_{\text{VCO}} = 4.0\text{ MHz} \times 85 = 340.0\text{ MHz} \in [96.0,\ 344.0]\text{ MHz} \quad \checkmark$$
- Choose $R = 2$ (`PLLR = 00b`):
  $$f_{\text{SYSCLK}} = \frac{340.0\text{ MHz}}{2} = \mathbf{170.0\text{ MHz}} \quad \checkmark$$
- Optional `PLL_P` for ADC (`PLLPDIV = 6` $\to P = 6$): $f_{\text{PLL\_P}} = 340 / 6 = 56.67\text{ MHz} \le 60\text{ MHz}$ (single ADC) or `PLLPDIV = 7` $\to 48.57\text{ MHz} \le 52\text{ MHz}$ (multi-ADC).
- Note on USB/RNG 48 MHz: Since $340\text{ MHz}$ is not evenly divisible by $48\text{ MHz}$, use **`HSI48` + `CRS`** (`CLK48SEL = 00b`) whenever running SYSCLK at 170 MHz!

### 6.3 Worked Example B: Exact 170 MHz from Internal 16 MHz `HSI16`
- Choose $M = 4$ (`PLLM = 3`) $\to f_{\text{PLL\_IN}} = 16 / 4 = 4.0\text{ MHz} \in [2.66,\ 16.0]\text{ MHz}$ $\checkmark$.
- Choose $N = 85$ $\to f_{\text{VCO}} = 340.0\text{ MHz}$ $\checkmark$; $R = 2$ $\to f_{\text{SYSCLK}} = 170.0\text{ MHz}$ $\checkmark$.

### 6.4 Worked Example C: Clean Integer 160 MHz Profile (HSE 24 MHz & HSI16 — Still Range 1 Boost + 4 WS!)
- **From HSE 24 MHz (`X3`):** Choose $M = 3$ (`PLLM = 2`) $\to f_{\text{PLL\_IN}} = 24 / 3 = 8.0\text{ MHz} \in [2.66, 16.0]\text{ MHz}$ $\checkmark$, $N = 40$ $\to f_{\text{VCO}} = 320.0\text{ MHz} \in [96, 344]\text{ MHz}$ $\checkmark$, $R = 2$ $\to f_{\text{SYSCLK}} = \mathbf{160.0\text{ MHz}}$ $\checkmark$ *(equivalently $M=6, N=80, R=2$ with $f_{\text{PLL\_IN}} = 4.0\text{ MHz}$)*.
- **From HSI16 (16 MHz):** Choose $M = 4$ (`PLLM = 3`) $\to 4.0\text{ MHz}$, $N = 80$ $\to f_{\text{VCO}} = 320.0\text{ MHz}$, $R = 2$ $\to f_{\text{SYSCLK}} = \mathbf{160.0\text{ MHz}}$ $\checkmark$.
- **Coupled Peripheral Math @ 160 MHz:**
  - `ADC CKMODE = 11b` (`HCLK/4`): $160 / 4 = \mathbf{40.000\text{ MHz}}$ (exact integer MHz $\le 52\text{ MHz}$ multi-ADC spec!).
  - `USART` 115,200 baud: $\text{BRR} = \text{round}(160{,}000{,}000 / 115{,}200) = \mathbf{1389}$ (`0x056D`, actual = $115{,}190.8\text{ bps}$, error = **$-0.008\%$**!).
  - `MCO` (`PA8`, `SYSCLK / 16`): $f_{\text{MCO}} = 160 / 16 = \mathbf{10.000\text{ MHz}}$ exact!

### 6.5 Worked Example D: MCO Verification Frequency on `PA8` at 170 MHz
With `SYSCLK = 170 MHz`, `MCOSEL = 0001b` (`SYSCLK`), and `MCOPRE = 100b` (`/16`):
$$f_{\text{MCO}} = \frac{170.0\text{ MHz}}{16} = \mathbf{10.625\text{ MHz}}$$
Using `/16` keeps the signal comfortably within standard 50–100 MHz oscilloscope probe bandwidth while preserving full frequency accuracy.

---

## 7. Official Documentation & Standards References

| Document | Exact Sections | Scope |
|---|---|---|
| **ST RM0440** (*STM32G4 Series Reference Manual*) | Ch. 3 (*FLASH ACR / Wait States*), Ch. 5/6 (*PWR*, §6.1.5 Boost transition), Ch. 6/7 (*RCC* — note Rev 8 vs Rev 9 chapter shift) | `[STM32G474RE Exact]` |
| **ST DS12288** (*STM32G474xB/xC/xE Datasheet*) | §5.3.9 (*External/Internal Clock Characteristics*), §5.3.10 (*PLL Characteristics*: $2.66–16\text{ MHz}$ input, $96–344\text{ MHz}$ VCO) | `[STM32G474RE Exact]` |
| **ST UM2505** (*STM32G4 Nucleo-64 MB1367 User Manual*) | §6.7 (*OSC Clock*: 24 MHz X3 crystal & 32.768 kHz X2 crystal default solder-bridge states) | `[STM32G474RE Exact]` |
| **ST ES0430** (*STM32G474xx Device Errata*) | §2.2.4 (*LSI stability with RTC + LSE CSS*) | `[STM32G474RE Exact]` |
| **ST AN5093** (*Getting Started with STM32G4 Hardware Dev*) | Crystal load-capacitance sizing & HSE layout guidelines | `[STM32G474RE Exact]` |

---

## 8. Production-Grade Bare-Metal C Code Examples

> **Verification Status:** Statically Reviewed against `stm32g474xx.h` & RM0440 (Not Tested on Physical Hardware). Includes bounded timeouts on every hardware ready-flag poll.

### 8.1 Safe 24 MHz HSE $\to$ 170 MHz Range 1 Boost Clock Configuration (RM0440 §6.1.5 Compliant)
```c
#include "stm32g474xx.h"
#include <stdint.h>

typedef enum {
    CLK_OK = 0,
    CLK_ERR_VOS_TIMEOUT,
    CLK_ERR_FLASH_LATENCY,
    CLK_ERR_HSE_TIMEOUT,
    CLK_ERR_PLL_TIMEOUT,
    CLK_ERR_SWS_TIMEOUT
} clk_status_t;

static clk_status_t wait_flag_eq(volatile const uint32_t *reg,
                                 uint32_t mask,
                                 uint32_t expected,
                                 uint32_t timeout,
                                 clk_status_t err_code)
{
    while (((*reg) & mask) != expected) {
        if (timeout-- == 0U) {
            return err_code;
        }
    }
    return CLK_OK;
}

clk_status_t SystemClock_Config_170MHz_HSE(void)
{
    clk_status_t st;

    /* 1. Enable PWR clock + AHB/APB sync readback */
    RCC->APB1ENR1 |= RCC_APB1ENR1_PWREN;
    (void)RCC->APB1ENR1;

    /* 2. Ensure Voltage Scaling Range 1 (VOS = 01b) and wait for VOSF == 0 */
    PWR->CR1 = (PWR->CR1 & ~PWR_CR1_VOS_Msk) | PWR_CR1_VOS_0;
    st = wait_flag_eq(&PWR->SR2, PWR_SR2_VOSF, 0U, 100000U, CLK_ERR_VOS_TIMEOUT);
    if (st != CLK_OK) { return st; }

    /* 3. Enter Range 1 Boost Mode (PWR->CR5 @ 0x80: R1MODE = 0) */
    PWR->CR5 &= ~PWR_CR5_R1MODE;

    /* 4. Configure Flash 4 Wait States + ART (Prefetch, ICache, DCache) BEFORE raising clock */
    FLASH->ACR = (FLASH->ACR & ~FLASH_ACR_LATENCY_Msk)
               | FLASH_ACR_LATENCY_4WS
               | FLASH_ACR_PRFTEN
               | FLASH_ACR_ICEN
               | FLASH_ACR_DCEN;
    st = wait_flag_eq(&FLASH->ACR, FLASH_ACR_LATENCY_Msk, FLASH_ACR_LATENCY_4WS,
                      10000U, CLK_ERR_FLASH_LATENCY);
    if (st != CLK_OK) { return st; }

    /* 5. Enable 24 MHz onboard HSE crystal X3 (HSEBYP = 0 for crystal!) */
    RCC->CR &= ~RCC_CR_HSEBYP;
    RCC->CR |= RCC_CR_HSEON;
    st = wait_flag_eq(&RCC->CR, RCC_CR_HSERDY, RCC_CR_HSERDY, 200000U, CLK_ERR_HSE_TIMEOUT);
    if (st != CLK_OK) { return st; }

    /* Optional: Enable Clock Security System (HSE failure -> NMI + HSI16 fallback) */
    RCC->CR |= RCC_CR_CSSON;

    /* 6. Disable PLL before reconfiguring PLLCFGR */
    RCC->CR &= ~RCC_CR_PLLON;
    st = wait_flag_eq(&RCC->CR, RCC_CR_PLLRDY, 0U, 100000U, CLK_ERR_PLL_TIMEOUT);
    if (st != CLK_OK) { return st; }

    /* 7. Main PLL: HSE (24 MHz) / M(6) = 4 MHz -> * N(85) = 340 MHz VCO -> / R(2) = 170 MHz */
    RCC->PLLCFGR = RCC_PLLCFGR_PLLSRC_HSE
                 | (5U << RCC_PLLCFGR_PLLM_Pos)   /* M = 5 + 1 = 6 */
                 | (85U << RCC_PLLCFGR_PLLN_Pos)  /* N = 85 */
                 | (0U << RCC_PLLCFGR_PLLR_Pos)   /* R = 2 */
                 | RCC_PLLCFGR_PLLREN;

    /* 8. Enable PLL and wait for lock */
    RCC->CR |= RCC_CR_PLLON;
    st = wait_flag_eq(&RCC->CR, RCC_CR_PLLRDY, RCC_CR_PLLRDY, 200000U, CLK_ERR_PLL_TIMEOUT);
    if (st != CLK_OK) { return st; }

    /* 9. MANDATORY RM0440 §6.1.5 STEP: Set intermediate HPRE = /2 (85 MHz HCLK) */
    RCC->CFGR = (RCC->CFGR & ~(RCC_CFGR_HPRE_Msk | RCC_CFGR_PPRE1_Msk | RCC_CFGR_PPRE2_Msk))
              | RCC_CFGR_HPRE_DIV2
              | RCC_CFGR_PPRE1_DIV1
              | RCC_CFGR_PPRE2_DIV1;

    /* 10. Switch SYSCLK source to PLL (SW = 11b) and verify SWS == 11b */
    RCC->CFGR = (RCC->CFGR & ~RCC_CFGR_SW_Msk) | RCC_CFGR_SW_PLL;
    st = wait_flag_eq(&RCC->CFGR, RCC_CFGR_SWS_Msk, RCC_CFGR_SWS_PLL,
                      100000U, CLK_ERR_SWS_TIMEOUT);
    if (st != CLK_OK) { return st; }

    /* 11. Stabilize >= 1 us at 85 MHz (>= 85 cycles; 120 loop iterations >> 1 us) */
    for (volatile uint32_t i = 0U; i < 120U; ++i) {
        __NOP();
    }

    /* 12. Restore HPRE = /1 -> HCLK = PCLK1 = PCLK2 = 170 MHz */
    RCC->CFGR = (RCC->CFGR & ~RCC_CFGR_HPRE_Msk) | RCC_CFGR_HPRE_DIV1;

    /* 13. Update CMSIS global SystemCoreClock variable */
    SystemCoreClockUpdate();
    return CLK_OK;
}
```

### 8.1b Profile B: Clean Integer 160 MHz Range 1 Boost Clock Configuration (`HSE 24 MHz`: `M=3, N=40, R=2`)

> **Verification Status:** **Compile-Verified (`arm-none-eabi-gcc 14.3.1`, `-O2 -Wall -Wextra -Werror -Wconversion -Wdouble-promotion -Wvla`)** against `stm32g474xx.h` (Not Tested on Physical Hardware). Notice that 160 MHz is STILL `> 150 MHz`, so `PWR->CR5.R1MODE = 0`, `4 WS`, and intermediate `HPRE = /2` (80 MHz) remain mandatory!

```c
bool SystemClock_Config_160MHz_HSE(void)
{
    uint32_t timeout;

    RCC->APB1ENR1 |= RCC_APB1ENR1_PWREN;
    (void)RCC->APB1ENR1;

    /* Range 1 (VOS=01b) + Boost Mode (R1MODE=0 REQUIRED since 160 MHz > 150 MHz!) */
    PWR->CR1 = (PWR->CR1 & ~PWR_CR1_VOS_Msk) | PWR_CR1_VOS_0;
    timeout = 100000UL;
    while ((PWR->SR2 & PWR_SR2_VOSF) != 0UL) { if (timeout-- == 0UL) { return false; } }
    PWR->CR5 &= ~PWR_CR5_R1MODE;

    /* 4 Wait States (RM0440 Table 9: > 136 MHz up to 170 MHz in Boost = 4 WS) + ART */
    FLASH->ACR = (FLASH->ACR & ~FLASH_ACR_LATENCY_Msk)
               | FLASH_ACR_LATENCY_4WS
               | FLASH_ACR_PRFTEN | FLASH_ACR_ICEN | FLASH_ACR_DCEN;
    timeout = 10000UL;
    while ((FLASH->ACR & FLASH_ACR_LATENCY_Msk) != FLASH_ACR_LATENCY_4WS) {
        if (timeout-- == 0UL) { return false; }
    }

    RCC->CR &= ~RCC_CR_HSEBYP;
    RCC->CR |=  RCC_CR_HSEON;
    timeout = 200000UL;
    while ((RCC->CR & RCC_CR_HSERDY) == 0UL) { if (timeout-- == 0UL) { return false; } }

    /* PLL: HSE 24 MHz / M(3) = 8 MHz -> * N(40) = 320 MHz VCO -> / R(2) = 160.0 MHz
     * (HSI16 equivalent: PLLSRC_HSI, M=4 [PLLM=3], N=80, R=2 -> 320 / 2 = 160.0 MHz) */
    RCC->CR &= ~RCC_CR_PLLON;
    timeout = 100000UL;
    while ((RCC->CR & RCC_CR_PLLRDY) != 0UL) { if (timeout-- == 0UL) { return false; } }

    RCC->PLLCFGR = RCC_PLLCFGR_PLLSRC_HSE
                 | ((3UL - 1UL) << RCC_PLLCFGR_PLLM_Pos)
                 | (40UL << RCC_PLLCFGR_PLLN_Pos)
                 | (0UL  << RCC_PLLCFGR_PLLR_Pos)    /* R = /2 -> 160.0 MHz SYSCLK */
                 | (3UL  << RCC_PLLCFGR_PLLQ_Pos)    /* Q = /8 -> 40.0 MHz */
                 | (8UL  << RCC_PLLCFGR_PLLPDIV_Pos) /* P = /8 -> 40.0 MHz ADC */
                 | RCC_PLLCFGR_PLLREN | RCC_PLLCFGR_PLLPEN;

    RCC->CR |= RCC_CR_PLLON;
    timeout = 200000UL;
    while ((RCC->CR & RCC_CR_PLLRDY) == 0UL) { if (timeout-- == 0UL) { return false; } }

    /* RM0440 §6.1.5 MANDATORY intermediate step (>150 MHz): HPRE = /2 (80 MHz) */
    RCC->CFGR = (RCC->CFGR & ~(RCC_CFGR_HPRE_Msk | RCC_CFGR_PPRE1_Msk | RCC_CFGR_PPRE2_Msk))
              | RCC_CFGR_HPRE_DIV2 | RCC_CFGR_PPRE1_DIV1 | RCC_CFGR_PPRE2_DIV1;

    RCC->CFGR = (RCC->CFGR & ~RCC_CFGR_SW_Msk) | RCC_CFGR_SW_PLL;
    timeout = 100000UL;
    while ((RCC->CFGR & RCC_CFGR_SWS_Msk) != RCC_CFGR_SWS_PLL) {
        if (timeout-- == 0UL) { return false; }
    }

    for (volatile uint32_t i = 0UL; i < 100UL; ++i) { __NOP(); } /* >= 1 us @ 80 MHz */

    /* Final switch: HCLK = PCLK1 = PCLK2 = 160.0 MHz */
    RCC->CFGR = (RCC->CFGR & ~RCC_CFGR_HPRE_Msk) | RCC_CFGR_HPRE_DIV1;
    return true;
}
```

### 8.2 MCO Clock Verification on `PA8` & Reset Cause Diagnostics
```c
void RCC_Enable_MCO_SYSCLK_Div16(void)
{
    RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN;
    (void)RCC->AHB2ENR;

    /* PA8 -> Alternate Function AF0 (MCO), Very High Speed */
    GPIOA->MODER   = (GPIOA->MODER   & ~(3U << 16U)) | (2U << 16U);
    GPIOA->OSPEEDR = (GPIOA->OSPEEDR & ~(3U << 16U)) | (3U << 16U);
    GPIOA->AFR[1] &= ~(0xFU << 0U); /* AFRH[3:0] = AF0 */

    /* MCOSEL = 0001 (SYSCLK), MCOPRE = 100 (/16 -> 10.625 MHz @ 170 MHz) */
    RCC->CFGR = (RCC->CFGR & ~(RCC_CFGR_MCOSEL_Msk | RCC_CFGR_MCOPRE_Msk))
              | (1U << RCC_CFGR_MCOSEL_Pos)
              | (4U << RCC_CFGR_MCOPRE_Pos);
}

uint32_t RCC_ReadAndClearResetFlags(void)
{
    uint32_t flags = RCC->CSR & (RCC_CSR_LPWRRSTF | RCC_CSR_WWDGRSTF | RCC_CSR_IWDGRSTF |
                                 RCC_CSR_SFTRSTF  | RCC_CSR_BORRSTF  | RCC_CSR_PINRSTF  |
                                 RCC_CSR_OBLRSTF);
    RCC->CSR |= RCC_CSR_RMVF;
    RCC->CSR &= ~RCC_CSR_RMVF;
    return flags;
}
```

---

## 9. Practical Laboratory Experiments & Debugging Checkpoints

- **Primary Lab:** [`lab-03-rcc-pll-170mhz-clock-switch`](../18-practical-laboratory/lab-03-rcc-pll-170mhz-clock-switch/README.md) (executed *after* Basic GPIO Labs 01–02).
- **GDB / SVD Verification Checkpoint:**
  ```text
  (gdb) x/4wx 0x40021000   # Inspect RCC->CR (HSION b8, HSERDY b17, PLLRDY b25), CFGR (SWS=0xC), PLLCFGR
  (gdb) x/1wx 0x40007080   # Verify PWR->CR5 bit 8 (R1MODE) == 0 (Range 1 Boost)
  (gdb) x/1wx 0x40007014   # Verify PWR->SR2 bit 10 (VOSF) == 0
  (gdb) x/1wx 0x40022000   # Verify FLASH->ACR bits [3:0] == 4 (4 WS) and bits [10:8] == 0x7 (ART ON)
  (gdb) p SystemCoreClock  # Confirm 170000000
  ```
- **Oscilloscope Checkpoint:** Probe `PA8` (`MCO`) with a 10x compensated probe and short ground spring; verify **$10.625\text{ MHz} \pm 0.01\%$** square wave (`SYSCLK / 16`).

---

## 10. Common Engineering Mistakes, Hardware Traps & Silicon Errata (ES0430)

1. **STM32F4 Bit-Position Copy-Paste Trap:** On STM32G4, `RCC->CR.HSION` is **Bit 8** (`HSIRDY` = **Bit 10**) and `RCC->CFGR.SW` uses **`01=HSI16`, `10=HSE`, `11=PLL`** (NOT `00=HSI` as on STM32F4!). Writing `SW = 00` is reserved and silently fails!
2. **Wrong `PWR->CR5` / `PWR->SR2` Offsets:** `PWR->CR5` lives at **`0x80`** (NOT `0x10`), and `VOSF` lives at bit 10 of **`PWR->SR2` (`0x14`)** (NOT `PWR->SR1`). Always use CMSIS `PWR->CR5` and `PWR->SR2`.
3. **Omitting the RM0440 §6.1.5 `HPRE = /2` Step:** Switching directly from 16 MHz to 170 MHz with `HPRE = /1` causes core voltage droop; always step through 85 MHz for $\ge 1\ \mu\text{s}$.
4. **Assuming HSE Is ST-LINK MCO:** On NUCLEO-G474RE (MB1367), HSE defaults to onboard **24 MHz crystal `X3`** (`HSEBYP = 0`). Setting `HSEBYP = 1` without reworking solder bridges prevents X3 from oscillating and hangs `HSERDY` forever!
5. **Forgetting `SystemClock_Config_170MHz()` in Labs:** `SystemInit()` boots at **16 MHz HSI16**. Hardcoding 170 MHz UART/TIM/I2C divisors without switching clocks creates a **10.625× timing error**!
6. **Forgetting `HSI16` During Flash Programming:** Turning off `HSI16` after switching to HSE+PLL breaks Flash erase/program operations (RM0440 requires HSI16 ON for Flash state machine).
7. **ES0430 §2.2.4 Erratum:** Do not clock RTC from LSI when LSE CSS is enabled; use LSE directly for RTC.

---

## 11. Progressive Coding Exercises & Design Challenges

1. **[Level 1]** Write a clock-gating helper macro/inline function that enables any `AHB2` or `APB1` peripheral clock and automatically performs the mandatory 2-cycle readback synchronization.
2. **[Level 2]** Derive PLL parameters ($M, N, R, P, Q$) from 24 MHz HSE to produce **150 MHz Range 1 Normal** (`R1MODE=1`, 4 WS) AND simultaneously produce a valid **50 MHz `PLL_P`** for multi-ADC operation ($\le 52\text{ MHz}$).
3. **[Level 2]** Implement an `NMI_Handler` that detects `RCC->CIFR & RCC_CIFR_CSSF`, clears `RCC->CICR = RCC_CICR_CSSC`, logs the clock fault to a `.noinit` flag, and verifies `SWS == RCC_CFGR_SWS_HSI`.
4. **[Level 3]** Implement dynamic frequency scaling transitioning safely down from 170 MHz Range 1 Boost $\to$ 24 MHz Range 2 (`VOS=10`, 1 WS) and back up, ensuring Flash latency is adjusted in the opposite order during step-down vs step-up.

---

## 12. Self-Assessment Mastery Questions

1. **Q:** Why must `FLASH->ACR.LATENCY` be increased *before* switching `SYSCLK` to 170 MHz, but decreased *after* switching back to 16 MHz?  
   *Hint:* What happens to instruction fetches if the CPU clocks faster than Flash access time even for a single cycle?
2. **Q:** Given 24 MHz HSE, is `M = 12, N = 170, R = 2` legal on STM32G474RE?  
   *Answer Check:* **NO!** $f_{\text{PLL\_IN}} = 24 / 12 = 2.0\text{ MHz} < 2.66\text{ MHz}$ minimum (and $N = 170 > 127$ max!).
3. **Q:** If `SYSCLK = 170 MHz`, `HPRE = /1`, and `PPRE1 = /4`, what are `HCLK`, `PCLK1`, and `TIM2CLK`?  
   *Answer Check:* `HCLK = 170 MHz`, `PCLK1 = 42.5 MHz`, `TIM2CLK = 2 * PCLK1 = 85.0 MHz`.
4. **Q:** Why cannot `PLL_Q` generate an exact 48 MHz USB clock when `SYSCLK = 170 MHz` ($f_{\text{VCO}} = 340\text{ MHz}$), and what STM32G4 hardware solves this?  
   *Answer Check:* $340 / 48 = 7.083$ (not an integer in $\{2,4,6,8\}$). Solution: **`HSI48` + `CRS`** (`CLK48SEL = 00b`).
5. **Q:** Why must `PWR->SR2 & PWR_SR2_VOSF` be polled instead of `PWR->SR1`?  
   *Answer Check:* Verified in `stm32g474xx.h`: `VOSF` is bit 10 of `PWR->SR2` (`0x40007014`).
6. **Q:** Does running the STM32G474RE at **160 MHz** allow you to skip Range 1 Boost Mode (`PWR->CR5.R1MODE = 0`), drop Flash latency to 3 WS, or omit the RM0440 §6.1.5 `HPRE = /2` transition?  
   *Answer Check:* **NO to all three!** Range 1 Normal maxes out at **150 MHz**, and Boost Mode 3 WS maxes out at **136 MHz** (RM0440 Table 9). Thus 160 MHz STILL requires `R1MODE = 0`, **4 WS**, and `HPRE = /2` (80 MHz) for $\ge 1\ \mu\text{s}$ — while giving clean 40.0 MHz ADC, 10.000 MHz MCO/16, `PSC=159`, `DAC HFSEL=01b`, and `USART BRR=1389` (`-0.008%` error)!

---

## 13. Measurable Mastery & Completion Criteria

| Concept ID | Pass / Fail Verification Criterion | Measurement Tool |
|---|---|---|
| `STM32-RCC-001` | Output `HSI16`, `HSE` (24 MHz X3), and `HSI48` divided on `PA8` (`MCO`) and verify frequencies within DS12288 tolerances | Oscilloscope (`PA8`) |
| `STM32-RCC-002` | Execute full RM0440 §6.1.5 170 MHz sequence (`VOSF==0`, `R1MODE=0`, `4 WS`, `HPRE=/2` + $\ge 1\ \mu\text{s}$ + `HPRE=/1`); measure `PA8` = $10.625\text{ MHz} \pm 0.01\%$ | Oscilloscope + GDB (`x/4wx 0x40021000`) |
| `STM32-RCC-003` | Decode `RCC->CSR` flags after Pin Reset vs Software Reset (`NVIC_SystemReset()`) vs IWDG Reset, and clear via `RMVF` | GDB / UART boot log |
| `STM32-PWR-001` / `STM32-FLASH-001` | Confirm `PWR->CR5.R1MODE == 0`, `PWR->SR2.VOSF == 0`, `FLASH->ACR.LATENCY == 4`, and `ART` enabled; zero unbounded `while` polls | Static review + GDB |
