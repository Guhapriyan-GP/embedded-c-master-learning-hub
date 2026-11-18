# RCC (Reset and Clock Control) Register Map

**Target MCU:** STM32G474RE (LQFP64, NUCLEO-G474RE MB1367)  
**Base Address:** `0x40021000` (`AHB1PERIPH_BASE + 0x1000UL`)  
**Bus:** AHB1  
**Reference Manual:** RM0440, Chapter 7 (*Reset and clock control (RCC)*) | DS12288 §5.3.9–5.3.10  
**Concept IDs:** `STM32-RCC-001`, `STM32-RCC-002`, `STM32-RCC-003`  
**Verification Status:** Every register offset and bit position verified directly against `stm32g474xx.h`; C code snippets Compile-Verified (`arm-none-eabi-gcc 14.3.1`, `-Wall -Wextra -Werror`) (Not Tested on Physical Hardware).

---

### 1. Register Overview Table (`RCC_TypeDef` @ `0x40021000`)

| Register | Offset | Reset Value | Access | Description |
|---|---|---|---|---|
| `RCC_CR` | `0x000` | `0x00000100` (`0x00000500` once `HSIRDY=1`) | RW | Clock control register (`HSION` bit 8, `HSIRDY` bit 10, `HSEON`/`HSERDY`, `CSSON`, `PLLON`/`PLLRDY`) |
| `RCC_ICSCR` | `0x004` | `0x40XX0000` | RW | Internal clock sources calibration register (`HSICAL`, `HSITRIM`) |
| `RCC_CFGR` | `0x008` | `0x00000005` | RW | Clock configuration register (`SW`, `SWS`, `HPRE`, `PPRE1`, `PPRE2`, `MCOSEL`, `MCOPRE`) |
| `RCC_PLLCFGR` | `0x00C` | `0x00001000` | RW | Main PLL configuration register (`PLLSRC`, `PLLM`, `PLLN`, `PLLP`/`PLLPDIV`, `PLLQ`, `PLLR`) |
| `RCC_CIER` | `0x018` | `0x00000000` | RW | Clock interrupt enable register |
| `RCC_CIFR` | `0x01C` | `0x00000000` | RO | Clock interrupt flag register (`CSSF`, `LSECSSF`, `PLLRDYF`, `HSERDYF`, `HSIRDYF`, `HSI48RDYF`) |
| `RCC_CICR` | `0x020` | `0x00000000` | WO | Clock interrupt clear register (write `1` to clear flags) |
| `RCC_AHB1RSTR` | `0x028` | `0x00000000` | RW | AHB1 peripheral reset register |
| `RCC_AHB2RSTR` | `0x02C` | `0x00000000` | RW | AHB2 peripheral reset register |
| `RCC_AHB3RSTR` | `0x030` | `0x00000000` | RW | AHB3 peripheral reset register |
| `RCC_APB1RSTR1` | `0x038` | `0x00000000` | RW | APB1 peripheral reset register 1 |
| `RCC_APB1RSTR2` | `0x03C` | `0x00000000` | RW | APB1 peripheral reset register 2 |
| `RCC_APB2RSTR` | `0x040` | `0x00000000` | RW | APB2 peripheral reset register |
| `RCC_AHB1ENR` | `0x048` | `0x00000100` | RW | AHB1 peripheral clock enable (`DMA1`, `DMA2`, `DMAMUX1`, `CORDIC`, `FMAC`, `FLASH`[reset=1], `CRC`) |
| `RCC_AHB2ENR` | `0x04C` | `0x00000000` | RW | AHB2 peripheral clock enable (`GPIOA–G`, `ADC12`, `ADC345`, `DAC1–4`, `RNG`) |
| `RCC_AHB3ENR` | `0x050` | `0x00000000` | RW | AHB3 peripheral clock enable (`QSPIEN` bit 8; `FMCEN` bit 0 unbonded on LQFP64) |
| `RCC_APB1ENR1` | `0x058` | `0x00000400` | RW | APB1 peripheral clock enable 1 (`TIM2–7`, `CRS`, `RTCAPB`[reset=1], `WWDG`, `SPI2–3`, `USART2–3`, `UART4–5`, `I2C1–3`, `USB`, `FDCAN`, `PWR`, `LPTIM1`) |
| `RCC_APB1ENR2` | `0x05C` | `0x00000000` | RW | APB1 peripheral clock enable 2 (`LPUART1EN` bit 0, `I2C4EN` bit 1, `UCPD1EN` bit 8) |
| `RCC_APB2ENR` | `0x060` | `0x00000000` | RW | APB2 peripheral clock enable (`SYSCFG`, `TIM1`, `SPI1`, `TIM8`, `USART1`, `TIM15–17`, `TIM20`, `SAI1`, `HRTIM1`) |
| `RCC_CCIPR` | `0x088` | `0x00000000` | RW | Peripherals independent clock configuration register 1 (USART/UART/LPUART/I2C1–3/LPTIM1/SAI1/I2S23/FDCAN/CLK48/ADC12/ADC345) |
| `RCC_BDCR` | `0x090` | `0x00000000` | RW | RTC backup domain control register (`LSEON`, `LSERDY`, `LSECSSON`, `RTCSEL`, `RTCEN`, `BDRST`; requires `PWR_CR1.DBP=1`) |
| `RCC_CSR` | `0x094` | `0x0C000000` | RW | Control/status register (`LSION`, `LSIRDY`, `RMVF`, and reset cause flags) |
| `RCC_CRRCR` | `0x098` | `0x0000XX00` | RW | Clock recovery RC register (`HSI48ON` bit 0, `HSI48RDY` bit 1, `HSI48CAL[15:7]`) |
| `RCC_CCIPR2` | `0x09C` | `0x00000000` | RW | Peripherals independent clock configuration register 2 (`I2C4SEL[1:0]`, `QSPISEL[21:20]`) |

---

### 2. Critical Register Bitfields

#### A. `RCC_CR` (Clock Control Register) — Offset `0x000`
> **STM32G4 vs. STM32F4 Warning:** On STM32G4, `HSION` is **Bit 8** and `HSIRDY` is **Bit 10** (NOT Bits 0 and 1 as on STM32F4). Always use CMSIS bit masks (`RCC_CR_HSION`, `RCC_CR_HSIRDY`).

- **Bit 8 (`HSION`):** HSI16 clock enable (RW). Reset: `1` (HSI16 is ON after reset).
- **Bit 9 (`HSIKERON`):** HSI16 always enable for peripheral kernels in Stop mode (RW).
- **Bit 10 (`HSIRDY`):** HSI16 clock ready flag (RO). Set by hardware when HSI16 (16 MHz ±1%) is stable.
- **Bit 16 (`HSEON`):** HSE crystal oscillator enable (RW). Reset: `0`. On NUCLEO-G474RE (MB1367), a **24 MHz crystal (`X3`)** is populated by default on `PF0`/`PF1`.
- **Bit 17 (`HSERDY`):** HSE clock ready flag (RO).
- **Bit 18 (`HSEBYP`):** HSE crystal oscillator bypass (RW). Set `1` only if driving `OSC_IN` from an external clock source (e.g., ST-LINK MCO).
- **Bit 19 (`CSSON`):** Clock Security System enable (RW, write-once until reset). Monitors HSE; failure switches SYSCLK to HSI16 and triggers **NMI** (`RCC_CIFR.CSSF = 1`).
- **Bit 24 (`PLLON`):** Main PLL enable (RW). Must be `0` (`PLLRDY == 0`) while modifying `RCC_PLLCFGR`.
- **Bit 25 (`PLLRDY`):** Main PLL clock ready flag (RO). Set by hardware when PLL locks.

#### B. `RCC_CFGR` (Clock Configuration Register) — Offset `0x008`
> **STM32G4 vs. STM32F4 Warning:** On STM32G4, `SW = 00` is **Reserved**! Reset value of `RCC_CFGR` is `0x00000005` (`SW = 01`, `SWS = 01`).

- **Bits [1:0] (`SW`):** System clock switch (RW):
  - `00`: **Reserved** (do not write)
  - `01`: **HSI16** selected as SYSCLK (`RCC_CFGR_SW_HSI = 0x1`)
  - `10`: **HSE** selected as SYSCLK (`RCC_CFGR_SW_HSE = 0x2`)
  - `11`: **PLL "R"** clock selected as SYSCLK (`RCC_CFGR_SW_PLL = 0x3`)
- **Bits [3:2] (`SWS`):** System clock switch status (RO). Mirrors `SW` once clock switch completes (`01` = HSI16, `10` = HSE, `11` = PLL).
- **Bits [7:4] (`HPRE`):** AHB prescaler (RW). `0xxx`: `/1`, `1000`: `/2`, `1001`: `/4`, `1010`: `/8`, `1011`: `/16`, `1100`: `/64`, `1101`: `/128`, `1110`: `/256`, `1111`: `/512`.
- **Bits [10:8] (`PPRE1`):** APB1 prescaler (RW). `0xx`: `/1`, `100`: `/2`, `101`: `/4`, `110`: `/8`, `111`: `/16`.
- **Bits [13:11] (`PPRE2`):** APB2 prescaler (RW). Same encoding as `PPRE1`.
- **Bits [27:24] (`MCOSEL`):** Microcontroller clock output selection (`0000`=Disabled, `0001`=SYSCLK, `0011`=HSI16, `0100`=HSE, `0101`=PLL, `0110`=LSI, `0111`=LSE, `1000`=HSI48) routed to `PA8` (AF0) or `PG10`.
- **Bits [30:28] (`MCOPRE`):** MCO prescaler (`000`=`/1`, `001`=`/2`, `010`=`/4`, `011`=`/8`, `100`=`/16`).

#### C. `RCC_PLLCFGR` (PLL Configuration Register) — Offset `0x00C`
Per **DS12288**, valid STM32G474RE PLL limits are:
- PLL input frequency: $f_{\text{PLL\_IN}} = \frac{f_{\text{SRC}}}{M} \in [2.66,\ 16.0]\ \text{MHz}$
- VCO output frequency: $f_{\text{VCO}} = f_{\text{PLL\_IN}} \times N \in [96,\ 344]\ \text{MHz}$ (Range 1) or $[96,\ 128]\ \text{MHz}$ (Range 2)

- **Bits [1:0] (`PLLSRC`):** `00` = No clock, `10` = HSI16, `11` = HSE.
- **Bits [7:4] (`PLLM`):** Input divider $M = \text{PLLM} + 1$ (`0000` = `/1` to `1111` = `/16`).
- **Bits [14:8] (`PLLN`):** VCO multiplier $N$ (Valid range: `8` to `127`).
- **Bit 16 (`PLLPEN`):** Main PLL "P" clock output enable (for ADC1–5).
- **Bit 17 (`PLLP`):** Divider for PLL_P when `PLLPDIV == 0` (`0` = `/7`, `1` = `/17`).
- **Bit 20 (`PLLQEN`):** Main PLL "Q" clock output enable (for FDCAN, QSPI, SAI1, USB/RNG).
- **Bits [22:21] (`PLLQ`):** Divider for PLL_Q (`00` = `/2`, `01` = `/4`, `10` = `/6`, `11` = `/8`).
- **Bit 24 (`PLLREN`):** Main PLL "R" clock output enable (**must be `1` to drive SYSCLK**).
- **Bits [26:25] (`PLLR`):** Divider for PLL_R (`00` = `/2`, `01` = `/4`, `10` = `/6`, `11` = `/8`).
- **Bits [31:27] (`PLLPDIV`):** Programmable divider for PLL_P (`00000` = controlled by `PLLP` bit; `00010`..`11111` = `/2`..`/31`).

#### D. `RCC_CCIPR` (`0x088`) & `RCC_CCIPR2` (`0x09C`) — Independent Kernel Clocks
- **`RCC_CCIPR`:** `USART1SEL[1:0]`, `USART2SEL[3:2]`, `USART3SEL[5:4]`, `UART4SEL[7:6]`, `UART5SEL[9:8]`, `LPUART1SEL[11:10]`, `I2C1SEL[13:12]`, `I2C2SEL[15:14]`, `I2C3SEL[17:16]`, `LPTIM1SEL[19:18]`, `SAI1SEL[21:20]`, `I2S23SEL[23:22]`, `FDCANSEL[25:24]` (`00`=HSE, `01`=PLLQ, `10`=PCLK1), `CLK48SEL[27:26]` (`00`=HSI48, `10`=PLLQ), `ADC12SEL[29:28]` (`00`=None, `01`=PLLP, `10`=SYSCLK), `ADC345SEL[31:30]`.
- **`RCC_CCIPR2`:** `I2C4SEL[1:0]`, `QSPISEL[21:20]` (`00`=SYSCLK, `01`=HSI16, `10`=PLLQ).

#### E. `RCC_CSR` (Control/Status Register) — Offset `0x094`
- **Bit 0 (`LSION`):** LSI ~32 kHz RC oscillator enable (RW).
- **Bit 1 (`LSIRDY`):** LSI ready flag (RO).
- **Bit 23 (`RMVF`):** Remove reset flags (RW; set `1` to clear bits [31:25], then clear to `0`).
- **Bits [31:25] (RO Reset Cause Flags):** `OBLRSTF` (b25), `PINRSTF` (b26), `BORRSTF` (b27), `SFTRSTF` (b28), `IWDGRSTF` (b29), `WWDGRSTF` (b30), `LPWRRSTF` (b31).

---

### 3. Dual Production Clock Profiles (170 MHz Boost vs 160 MHz Boost vs 150 MHz Normal)

> **CRITICAL SILICON RULE (RM0440 §6.1.5 & Table 9):** **160 MHz is STILL $> 150\text{ MHz}$**, so Profile B STILL requires **Range 1 Boost (`PWR->CR5.R1MODE = 0`)**, **4 Flash Wait States (`FLASH_ACR_LATENCY_4WS`)**, and the **intermediate `HPRE = /2` (80 MHz) $\ge 1\ \mu\text{s}$ transition**! Only $\le 150\text{ MHz}$ (Profile C) permits Range 1 Normal (`R1MODE = 1`).

| Metric | **Profile A: 170 MHz Max (Boost)** | **Profile B: 160 MHz Clean Integer (Boost)** | **Profile C: 150 MHz Normal** |
|---|---|---|---|
| `PWR->CR5.R1MODE` / `FLASH` WS | `0` (Boost) / **`4 WS`** | `0` (Boost — `>150 MHz`!) / **`4 WS`** | `1` (Normal) / **`4 WS`** (`>120 MHz`) |
| RM0440 §6.1.5 `HPRE=/2` Step | **Yes** (`85 MHz` $\ge 1\ \mu\text{s}$) | **Yes** (`80 MHz` $\ge 1\ \mu\text{s}$) | Not required |
| `HSI16` (16 MHz) PLL | `M=4 (PLLM=3), N=85, R=2` (`VCO=340`) | `M=4 (PLLM=3), N=80, R=2` (`VCO=320`) | `M=4 (PLLM=3), N=75, R=2` (`VCO=300`) |
| `HSE` (24 MHz X3) PLL | `M=6 (PLLM=5), N=85, R=2` (`VCO=340`) | `M=3 (PLLM=2), N=40, R=2` (or `M=6, N=80, R=2`) | `M=6 (PLLM=5), N=75, R=2` (or `M=2, N=25, R=2`) |
| `ADC` (`CKMODE=11`, `HCLK/4`) | `42.5 MHz` ($\le 60/52\text{ MHz}$) | **`40.000 MHz`** (exact integer MHz!) | `37.5 MHz` (`PLL_P = 50.0 MHz`) |
| `DAC->MCR.HFSEL` | **`10b`** (`> 160 MHz`) | **`01b`** ($\le 160\text{ MHz}$) | **`01b`** ($\le 160\text{ MHz}$) |
| 1 µs Timer `PSC` / `MCO/16` | `PSC = 169` / `10.625 MHz` | **`PSC = 159`** / **`10.000 MHz`** | `PSC = 149` / `9.375 MHz` |
| `USART` 115200 `BRR` / Error | `1476` (`0x05C4`, `-0.021%`) | **`1389`** (`0x056D`, **`-0.008%`**) | `1302` (`0x0516`, `+0.006%`) |
| SWO 2 MHz NRZ (`TPI->ACPR`) | `84` (`0x54`) | **`79`** (`0x4F`) (`39` @ 4 MHz) | `74` (`0x4A`) |

---

### 4. CMSIS Code Examples: Safe 170 MHz & 160 MHz Boost Clock Switches (HSE 24 MHz X3 + RM0440 §6.1.5)

> **Status:** Compile-Verified (`arm-none-eabi-gcc 14.3.1`, `-Wall -Wextra -Werror`) (Not Tested on Physical Hardware). Always configure PWR Range 1 Boost (`PWR_CR5.R1MODE = 0`) and Flash 4 Wait States (`FLASH_ACR.LATENCY = 4WS`) BEFORE switching above 150 MHz!

```c
#include "stm32g474xx.h"

void SystemClock_Config_170MHz_HSE(void) {
    /* 1. Enable PWR clock & select Voltage Scaling Range 1 Boost Mode */
    RCC->APB1ENR1 |= RCC_APB1ENR1_PWREN;
    (void)RCC->APB1ENR1;
    PWR->CR1 = (PWR->CR1 & ~PWR_CR1_VOS_Msk) | PWR_CR1_VOS_0; /* 01b = Range 1 */
    while (PWR->SR2 & PWR_SR2_VOSF) { /* Wait for voltage scaling transition */ }
    PWR->CR5 &= ~PWR_CR5_R1MODE; /* 0 = Range 1 Boost Mode (mandatory > 150 MHz) */

    /* 2. Configure Flash 4 Wait States + Prefetch + Instruction/Data Caches */
    FLASH->ACR = (FLASH->ACR & ~FLASH_ACR_LATENCY_Msk) |
                 FLASH_ACR_LATENCY_4WS |
                 FLASH_ACR_PRFTEN | FLASH_ACR_ICEN | FLASH_ACR_DCEN;
    while ((FLASH->ACR & FLASH_ACR_LATENCY_Msk) != FLASH_ACR_LATENCY_4WS) { }

    /* 3. Enable 24 MHz HSE crystal (X3 on NUCLEO-G474RE) and wait for HSERDY */
    RCC->CR |= RCC_CR_HSEON;
    while (!(RCC->CR & RCC_CR_HSERDY)) { }

    /* 4. Configure PLL (ensure PLL is OFF first):
     *    f_IN = 24 MHz, M = 6 -> VCO_in = 4 MHz (in [2.66, 16] MHz)
     *    N = 85 -> VCO_out = 340 MHz (in [96, 344] MHz)
     *    R = 2 -> SYSCLK = 170 MHz; Q = 8 -> 42.5 MHz; P = 8 -> 42.5 MHz (<= 60 MHz ADC) */
    RCC->CR &= ~RCC_CR_PLLON;
    while (RCC->CR & RCC_CR_PLLRDY) { }

    RCC->PLLCFGR = RCC_PLLCFGR_PLLSRC_HSE                 | /* 11b = HSE */
                   ((6U - 1U) << RCC_PLLCFGR_PLLM_Pos)    | /* M = 6 */
                   (85U << RCC_PLLCFGR_PLLN_Pos)          | /* N = 85 */
                   (0U << RCC_PLLCFGR_PLLR_Pos)           | /* R = 2 (00b) */
                   (3U << RCC_PLLCFGR_PLLQ_Pos)           | /* Q = 8 (11b) */
                   (8U << RCC_PLLCFGR_PLLPDIV_Pos)        | /* P = 8 (42.5 MHz) */
                   RCC_PLLCFGR_PLLREN | RCC_PLLCFGR_PLLPEN;

    /* 5. Enable PLL and wait for lock */
    RCC->CR |= RCC_CR_PLLON;
    while (!(RCC->CR & RCC_CR_PLLRDY)) { }

    /* 6. RM0440 §6.1.5 MANDATORY transition step for >150 MHz:
     *    Step A: Set AHB prescaler HPRE = /2 (85 MHz intermediate) */
    RCC->CFGR = (RCC->CFGR & ~RCC_CFGR_HPRE_Msk) | RCC_CFGR_HPRE_DIV2;

    /*    Step B: Switch SYSCLK to PLL and wait for SWS == PLL */
    RCC->CFGR = (RCC->CFGR & ~RCC_CFGR_SW_Msk) | RCC_CFGR_SW_PLL;
    while ((RCC->CFGR & RCC_CFGR_SWS_Msk) != RCC_CFGR_SWS_PLL) { }

    /*    Step C: Wait >= 1 microsecond at 85 MHz (>= 85 cycles) */
    for (volatile uint32_t i = 0U; i < 100U; i++) { __NOP(); }

    /*    Step D: Set HPRE = /1 for full 170 MHz HCLK, PCLK1 = PCLK2 = 170 MHz */
    RCC->CFGR = (RCC->CFGR & ~(RCC_CFGR_HPRE_Msk | RCC_CFGR_PPRE1_Msk | RCC_CFGR_PPRE2_Msk)) |
                RCC_CFGR_HPRE_DIV1 | RCC_CFGR_PPRE1_DIV1 | RCC_CFGR_PPRE2_DIV1;
}

/* Profile B: 160 MHz Clean Integer Profile (HSE 24 MHz: M=3, N=40, R=2 -> VCO=320 MHz)
 * STILL requires Boost Mode (R1MODE=0), 4 WS, and HPRE=/2 (80 MHz) >= 1 us step! */
void SystemClock_Config_160MHz_HSE(void) {
    RCC->APB1ENR1 |= RCC_APB1ENR1_PWREN;
    (void)RCC->APB1ENR1;
    PWR->CR1 = (PWR->CR1 & ~PWR_CR1_VOS_Msk) | PWR_CR1_VOS_0;
    while (PWR->SR2 & PWR_SR2_VOSF) { }
    PWR->CR5 &= ~PWR_CR5_R1MODE; /* 0 = Boost required for 160 MHz (> 150 MHz) */

    FLASH->ACR = (FLASH->ACR & ~FLASH_ACR_LATENCY_Msk) |
                 FLASH_ACR_LATENCY_4WS |
                 FLASH_ACR_PRFTEN | FLASH_ACR_ICEN | FLASH_ACR_DCEN;
    while ((FLASH->ACR & FLASH_ACR_LATENCY_Msk) != FLASH_ACR_LATENCY_4WS) { }

    RCC->CR |= RCC_CR_HSEON;
    while (!(RCC->CR & RCC_CR_HSERDY)) { }

    RCC->CR &= ~RCC_CR_PLLON;
    while (RCC->CR & RCC_CR_PLLRDY) { }

    /* M=3 (PLLM=2), N=40, R=2 -> 160 MHz; Q=8 -> 40 MHz; P=8 -> 40.0 MHz exact ADC */
    RCC->PLLCFGR = RCC_PLLCFGR_PLLSRC_HSE                 |
                   ((3U - 1U) << RCC_PLLCFGR_PLLM_Pos)    |
                   (40U << RCC_PLLCFGR_PLLN_Pos)          |
                   (0U << RCC_PLLCFGR_PLLR_Pos)           |
                   (3U << RCC_PLLCFGR_PLLQ_Pos)           |
                   (8U << RCC_PLLCFGR_PLLPDIV_Pos)        |
                   RCC_PLLCFGR_PLLREN | RCC_PLLCFGR_PLLPEN;

    RCC->CR |= RCC_CR_PLLON;
    while (!(RCC->CR & RCC_CR_PLLRDY)) { }

    /* RM0440 §6.1.5 intermediate step: HPRE=/2 (80 MHz) -> SW=PLL -> >=1 us -> HPRE=/1 */
    RCC->CFGR = (RCC->CFGR & ~RCC_CFGR_HPRE_Msk) | RCC_CFGR_HPRE_DIV2;
    RCC->CFGR = (RCC->CFGR & ~RCC_CFGR_SW_Msk) | RCC_CFGR_SW_PLL;
    while ((RCC->CFGR & RCC_CFGR_SWS_Msk) != RCC_CFGR_SWS_PLL) { }
    for (volatile uint32_t i = 0U; i < 100U; i++) { __NOP(); }
    RCC->CFGR = (RCC->CFGR & ~(RCC_CFGR_HPRE_Msk | RCC_CFGR_PPRE1_Msk | RCC_CFGR_PPRE2_Msk)) |
                RCC_CFGR_HPRE_DIV1 | RCC_CFGR_PPRE1_DIV1 | RCC_CFGR_PPRE2_DIV1;
}
```
