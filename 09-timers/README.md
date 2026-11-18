# Stage 09: Hardware Timers (Basic, General-Purpose, Advanced, LPTIM1 & HRTIM1)

> **Directory:** `09-timers/` *(directory name preserved for link stability; logically **Stage 09** — Timer Interrupts in Lab 06 strictly follow Stage 08 Interrupts/NVIC in `10-interrupts/`, while Polling-Mode Basic Timers can precede interrupts!)*  
> **Target Platform:** STM32G474RET6 (14 Hardware Timers: `TIM6/7`, `TIM2/3/4/5/15/16/17`, `TIM1/8/20`, `LPTIM1`, **`HRTIM1`**)  
> **Code Verification Notice:** All bare-metal C snippets in this module are **Statically Reviewed against `stm32g474xx.h`, RM0440, and AN4539 (Not Tested on Physical Hardware)**.

---

## 1. Learning Objectives & Concept Scope

| Concept ID | Concept Name | Architectural Scope | Mastery Tier |
|---|---|---|---|
| `PERIPH-TIM-001` | Basic Timers (`TIM6`, `TIM7`) — Shadow Preload (`PSC`/`ARR`/`EGR.UG`), `rc_w0` Status Clearing (`SR = ~UIF`), & DAC `TRGO` Triggering | `[STM32G474RE Exact]` | Level 1 (Essential) |
| `PERIPH-TIM-002` | General-Purpose Timers (`TIM2/5` 32-bit; `TIM3/4/15/16/17` 16-bit) & APB $\times 2$ Clock Rule | `[STM32G474RE Exact]` | Level 1 (Essential) |
| `PERIPH-PWM-001` | Edge-Aligned vs Center-Aligned PWM (`CMS`), Preload (`ARPE`/`OCxPE`), & Glitch-Free Duty Updates | `[STM32 Generic]` | Level 2 (Production) |
| `PERIPH-IC-001` / `002` | Input Capture (`ICxF`), Overflow Race Prevention, & Hardware PWM Input Mode (`SMS=0100` Reset) | `[STM32 Generic]` | Level 2 (Production) |
| `PERIPH-OC-001` / `PERIPH-ENC-001` | Output Compare, One-Pulse Mode (`OPM`), & Quadrature Encoder Interface (X1/X2/X4 + 32-bit Rollover) | `[STM32G474RE Exact]` | Level 2 (Production) |
| `PERIPH-SYNC-001` | Master/Slave Timer Synchronization (`CR2.MMS` `TRGO` $\to$ `SMCR.TS` `ITRx`) & Autonomous ADC/DAC Triggering | `[STM32G474RE Exact]` | Level 2 (Production) |
| `PERIPH-TIM-003` / `PERIPH-PWM-002` / `PERIPH-BDTR-001` / `002` | Advanced Timers (`TIM1/8/20`): Complementary `CHx`/`CHxN`, `RCR`, **`BDTR.DTG` 4-Range Dead-Time**, `MOE`, & Hardware **`BKIN`/`BKIN2` Safety Break** | `[STM32G474RE Exact]` | Level 3 (Specialist) |
| `PERIPH-LPTIM-001` | Low-Power Asynchronous Timer (`LPTIM1`) — Autonomous Stop 0 / Stop 1 Wakeup & Tickless Timebase | `[STM32G474RE Exact]` | Level 3 (Specialist) |
| `PERIPH-TIM-004` (`PERIPH-HRTIM-001`) | **High-Resolution Timer (`HRTIM1`)** — **184 ps DLL** (`5.44 GHz` equiv.), Master + Timers A–F, 12 Outputs, Faults & Dead-Time | `[STM32G474RE Exact]` | Level 3 (Specialist) |

---

## 2. Prerequisites & Curriculum Positioning

### Prerequisite Concept IDs
- **For Level 1 Polling-Mode Basic Timers (`TIM6/7`, `TIM2` Free-Running Counter):**
  - **Stage 06 (`07-rcc-and-clocks/`):** `STM32-RCC-001`, `STM32-RCC-002` (170 MHz `SYSCLK` and APB timer $\times 2$ multiplier rule).
  - **Stage 07 (`08-gpio/`):** `PERIPH-GPIO-001`, `PERIPH-GPIO-002` (GPIO Alternate Function `AFR[0..1]` and `OSPEEDR`).
- **For Level 2/3 Timer Interrupts (`Lab 06`), Input Capture ISRs, & Break ISRs:**
  - **Stage 08 (`10-interrupts/`):** `RT-IRQ-001`, `RT-IRQ-002` (NVIC `PRIGROUP`, `NVIC_EnableIRQ()`, and `__DSB()` barrier).

---

## 3. Core Technical Theory & Engineering Principles

### 3.1 Complete STM32G474RE Timer Inventory `[STM32G474RE Exact]`
| Timer Class | Instances & Base Addresses | Bus & RCC Enable | Width & Channels | Key Capabilities |
|---|---|---|---|---|
| **Basic** | `TIM6` (`0x40001000`), `TIM7` (`0x40001400`) | APB1 (`APB1ENR1` b4, b5) | 16-bit, **0 I/O ch** | Precision timebases, `TRGO` directly triggering `DAC1–4`; IRQs `54`, `55` (shared with DAC underrun) |
| **General-Purpose (32-bit)** | `TIM2` (`0x40000000`), `TIM5` (`0x40000C00`) | APB1 (`APB1ENR1` b0, b3) | **32-bit**, 4 ch | Long-span µs timestamps (4294 s @ 1 MHz!), PWM, Input Capture, Quadrature Encoder |
| **General-Purpose (16-bit)** | `TIM3` (`0x40000400`), `TIM4` (`0x40000800`) | APB1 (`APB1ENR1` b1, b2) | 16-bit, 4 ch | PWM, Input Capture, Encoder, Master/Slave sync |
| **General-Purpose + Break** | `TIM15` (`0x40014000`), `TIM16` (`0x40014400`), `TIM17` (`0x40014800`) | APB2 (`APB2ENR` b16–18) | 16-bit, 1–2 ch + 1N | Complementary output, Dead-Time, `BDTR.MOE` (MUST set `MOE=1`!), Break |
| **Advanced-Control** | `TIM1` (`0x40012C00`), `TIM8` (`0x40013400`), `TIM20` (`0x40015000`) | APB2 (`APB2ENR` b11, b13, b20) | 16-bit, 4 ch + 4N + `CCR5/6` | Motor FOC / digital power: `RCR`, asymmetric dead-time (`BDTR`+`DTR2`), dual `BKIN`/`BKIN2` $\leftarrow$ `COMP1–7`, `TRGO2` $\to$ `ADC1–5` |
| **Low-Power** | `LPTIM1` (`0x40007C00`) | APB1 (`APB1ENR1` b31) + `CCIPR` | 16-bit async | Runs from LSE/LSI in **Stop 0 / Stop 1** (`LPTIM1_IRQn = 49`) |
| **High-Resolution** | **`HRTIM1`** (`0x40016800`, Master + `TIMA..F` + `COMMON`) | APB2 (`APB2ENR` b26) | 16-bit × 7 units, **12 outputs** | **184 ps DLL resolution (`5.44 GHz` equiv.)**, crossbar Set/Reset matrices, 10 EEV, 6 Faults, Burst Mode, 10 ADC triggers! |

### 3.2 Shadow Registers, Preload (`ARPE`/`OCxPE`), `EGR.UG` & `rc_w0` Clearing `[STM32 Generic]`
- **Shadow Registers:** `PSC` is **always** buffered; `ARR` and `CCRx` are buffered when `CR1.ARPE = 1` and `CCMRx.OCxPE = 1`. Writing `PSC`/`ARR`/`CCRx` updates the *preload* register, which copies into the active *shadow* register only on an **Update Event (UEV)**.
- **Mandatory `EGR = UG` Initialization Rule:** Before enabling `DIER.UIE` and `CR1.CEN`, force an immediate shadow load:
  ```c
  TIM2->EGR = TIM_EGR_UG; /* Loads PSC & ARR shadows immediately (also sets SR.UIF!) */
  TIM2->SR  = 0U;         /* Clear the spurious UIF caused by UG before enabling UIE! */
  ```
  Omitting `EGR.UG` leaves `PSC = 0` for the very first period — causing an immediate phantom glitch interrupt!
- **`rc_w0` Status Register Rule (`TIMx->SR`):** Unlike `EXTI->PR1` (`rc_w1`), `TIMx->SR` flags are **write-0-to-clear (`rc_w0`)**! Never use read-modify-write `TIM2->SR &= ~TIM_SR_UIF;` (if `CC1IF` sets between the read and write, `&=` accidentally writes `0` to `CC1IF` and drops it!). Always use direct write:
  ```c
  TIM2->SR = ~TIM_SR_UIF; /* Writes 0 ONLY to UIF; writes 1 to all other bits (no effect!) */
  __DSB();
  ```

### 3.3 Edge-Aligned vs Center-Aligned PWM, Input Capture, OPM & Quadrature Encoder
- **Edge-Aligned PWM Mode 1 (`OCxM = 0110b`, `CMS = 00b`):** Output is active while `CNT < CCRx`; frequency $f_{\text{PWM}} = \frac{f_{\text{CNT}}}{\text{ARR}+1}$.
- **Center-Aligned PWM (`CMS = 01/10/11b`):** Counter counts `0 -> ARR -> 0` symmetrically; halves switching harmonics and centers ADC current sampling (`TIM1_TRGO2`) cleanly away from MOSFET switching edges! Frequency $f_{\text{PWM}} = \frac{f_{\text{CNT}}}{2 \times \text{ARR}}$.
- **Input Capture & PWM Input Mode:** Latches `CNT` into `CCRx` on filtered external edges (`ICxF[3:0]`). In **PWM Input Mode**, route one pin (`TI1`) to both `CC1` (rising edge $\to$ Period) and `CC2` (falling edge $\to$ Pulse Width), and set Slave Reset Mode (`SMCR.TS = 101b` `TI1FP1`, `SMS = 0100b` Reset) so hardware measures period and duty cycle simultaneously with zero CPU polling!
- **One-Pulse Mode (`CR1.OPM = 1`):** Automatically clears `CEN` after a single update event — emits a hardware-deterministic delay (`CCR1`) and pulse width (`ARR - CCR1`) from an external trigger.
- **Quadrature Encoder Interface (`SMS = 001/010/011b`):** Decodes 90°-shifted A/B signals on `TI1`/`TI2` in hardware (X2 on TI1, X2 on TI2, or **X4** on both edges of both inputs); `CR1.DIR` indicates CW/CCW direction. Extend 16-bit `TIM3->CNT` to signed 32/64-bit in software by accumulating `(int16_t)(now - prev)`.

### 3.4 Master/Slave Synchronization (`TRGO`/`ITRx`) & Advanced Timer Safety (`BDTR`/`BKIN`)
- **Master/Slave Chaining:** A master timer outputs `TRGO` (`CR2.MMS = 010b` Update, `011b` Compare Pulse, etc.) over internal silicon interconnects (`ITR0..ITR11` per RM0440 Table 241) to clock/reset/gate a slave timer (e.g., cascading two 16-bit timers into a 32-bit counter) or trigger `ADC1–5`/`DAC1–4` with sub-nanosecond hardware determinism.
- **Complementary Outputs & Dead-Time (`TIM1/8/20`):** Drives high-side (`CHx`) and low-side (`CHxN`) power MOSFETs. Hardware inserts programmable dead-time (`BDTR.DTG[7:0]` + STM32G4 asymmetric `DTR2.DTGF`) so both FETs are NEVER simultaneously ON (preventing destructive shoot-through!).
- **Hardware Break (`BKIN` / `BKIN2`):** Asynchronous hardware fault path wired from external pin OR internal **`COMP1–7`** (`TIM1->AF1.BKCMP1E=1`) OR `SYSCFG->CFGR2` (CPU lockup/PVD/SRAM parity). Asserting Break clears `BDTR.MOE = 0` in **< 50 ns** — even if the CPU is frozen in a HardFault or halted in GDB!

### 3.5 STM32G474RE Flagship: High-Resolution Timer (`HRTIM1`, 184 ps) `[STM32G474RE Exact]`
`HRTIM1` is the signature differentiator of the STM32G474 vs G431/G473:
- **Architecture:** 1 **Master Timer** (`HRTIM1` @ `0x40016800`) + **6 independent 16-bit Timer Units (`TIMA..TIMF`**, stride `0x80` starting `0x40016880`) + **Common Control** (`HRTIM1_COMMON` @ `0x40016B80`), driving **12 outputs (`CHA1/2..CHF1/2`)** — all 12 bonded out on LQFP64 (`PA8–PA11`, `PB12–PB15`, `PC6–PC9`, AF13/AF3)!
- **184 ps Delay-Locked Loop (DLL):** Multiplies $f_{\text{HRTIM}} = 170\text{ MHz}$ by **$\times 32$** to an equivalent **$5.44\text{ GHz}$** clock ($t_{\text{res}} = \frac{1}{5.44\text{ GHz}} = \mathbf{183.8\text{ ps}}$!).
- **Mandatory DLL Calibration:** Before enabling any HRTIM counter, trigger DLL calibration (`HRTIM1_COMMON->DLLCR = HRTIM_DLLCR_CAL | HRTIM_DLLCR_CALEN | ...`) and **poll `HRTIM1_COMMON->ISR & HRTIM_ISR_DLLRDY`**. Periodic auto-calibration (`CALEN=1`) compensates thermal/voltage drift automatically.
- **Crossbar Event Matrices (`SETx1R`/`RSTx1R`):** Any output can be Set or Reset by up to 32 independent sources (Period, CMP1–4, Master CMP1–4, other Timer Units, or 10 External Events `EEV1–10` from `COMP1–7` for cycle-by-cycle peak current-mode control!).

---

## 4. Core Terminology & Acronyms

| Term | Engineering Definition |
|---|---|
| **`PSC` / `ARR` / `CNT` / `CCR`** | Prescaler (`1..65536`), Auto-Reload Register (period), live Counter, Capture/Compare Register (duty or timestamp). |
| **UEV (`UIF`) / `EGR.UG`** | Update Event (counter overflow/underflow) / Software Update Generation loading shadow registers. |
| **`ARPE` / `OCxPE`** | Auto-Reload Preload Enable / Output Compare Preload Enable — prevents mid-cycle PWM glitches. |
| **`BDTR.MOE` / `DTG` / `BKIN`** | Main Output Enable (required on `TIM1/8/15/16/17/20`), Dead-Time Generator, Hardware Safety Break Input. |
| **`HRTIM1` DLL** | Delay-Locked Loop synthesizing 32 phase-shifted taps per 170 MHz clock cycle (184 ps resolution). |
| **`DITHEN` (STM32G4)** | Hardware 4-bit fractional dithering (`CR1.DITHEN`) adding 4 bits of effective PWM frequency/duty resolution on standard timers. |

---

## 5. Hardware Register Architecture & Bitfield Reference

Verified directly against `TIM_TypeDef`, `LPTIM_TypeDef`, and `HRTIM_*` in `stm32g474xx.h`:

| Register | Offset (`TIMx`) | Key Bitfields & STM32G4 Notes |
|---|---|---|
| **`CR1`** | `0x00` | `CEN` (b0), `UDIS` (b1), `URS` (b2), `OPM` (b3), `DIR` (b4), `CMS[6:5]`, `ARPE` (b7), `CKD[9:8]`, `UIFREMAP` (b11), `DITHEN` (b12) |
| **`CR2`** | `0x04` | `MMS[6:4]` (`010`=Update `TRGO`), `OIS1..4`, `MMS2[23:20]` (`TRGO2` $\to$ ADC on TIM1/8/20) |
| **`SMCR`** | `0x08` | `SMS[2:0]` + `SMS[16]` (Encoder `001..011`, Reset `100`, Trigger `110`, ExtClk `111`), `TS` (`ITRx`/`TI1FP1`) |
| **`DIER`** / **`SR`** / **`EGR`** | `0x0C` / `0x10` / `0x14` | `UIE`/`UDE`/`CCxIE`; **`SR` is `rc_w0`** (`TIMx->SR = ~flag;`); `EGR.UG` (b0) |
| **`CCMR1/2`** / **`CCER`** | `0x18/1C` / `0x20` | `OCxM` (`0110`=PWM1), `OCxPE`, `CCxS`, `ICxF`; `CCxE`, `CCxP`, `CCxNE`, `CCxNP` |
| **`CNT` / `PSC` / `ARR` / `RCR`** | `0x24 / 28 / 2C / 30` | 32-bit `CNT`/`ARR` on `TIM2/5`; 16-bit on others; 16-bit `RCR` on `TIM1/8/20` |
| **`CCR1..4`** | `0x34..0x40` | Capture/Compare channels 1–4 (`CCR5/6` @ `0x48/4C` on TIM1/8/20) |
| **`BDTR`** | `0x44` | `DTG[7:0]`, `LOCK[9:8]`, `OSSI`, `OSSR`, `BKE` (b12), `BKP`, `AOE`, **`MOE` (b15)**, `BK2E` (b24) |
| **`AF1` / `AF2`** | `0x60` / `0x64` | Routes `COMP1–7` outputs directly to `BKIN` / `BKIN2` (`BKCMP1E..7E`) |
| **`DCR` / `DMAR`** | **`0x3DC` / `0x3E0`** | STM32G4 moved DMA Burst offsets (NOT `0x48/4C` as on STM32F4!) |

**Selected Verified `IRQn` & `DMAMUX1` IDs (`stm32g474xx.h` / RM0440 Table 91):**
- `TIM2_IRQn = 28`, `TIM3_IRQn = 29`, `TIM4_IRQn = 30`, `TIM5_IRQn = 50`, **`TIM6_DAC_IRQn = 54`**, **`TIM7_DAC_IRQn = 55`**, `TIM1_BRK_TIM15_IRQn = 24`, `TIM1_UP_TIM16_IRQn = 25`, `TIM1_TRG_COM_TIM17_IRQn = 26`, `TIM1_CC_IRQn = 27`, **`LPTIM1_IRQn = 49`**, `HRTIM1_Master_IRQn = 67` .. `HRTIM1_TIMF_IRQn = 74`.
- DMAMUX1: `TIM6_UP = 8`, `TIM7_UP = 9`, `TIM1_UP = 46`, `TIM2_UP = 60`, `HRTIM1_M..F = 95..101`.

---

## 6. Engineering Mathematics & Worked Formulas

### 6.1 Update Event & PWM Frequency Equations
$$f_{\text{CNT}} = \frac{f_{\text{TIMCLK}}}{\text{PSC} + 1}$$
- **Edge-Aligned Update / PWM Frequency:**
  $$f_{\text{UEV}} = f_{\text{PWM,edge}} = \frac{f_{\text{TIMCLK}}}{(\text{PSC} + 1)(\text{ARR} + 1)}, \qquad D = \frac{\text{CCR}x}{\text{ARR} + 1}$$
- **Center-Aligned PWM Frequency (`CMS != 00`):**
  $$f_{\text{PWM,center}} = \frac{f_{\text{TIMCLK}}}{2(\text{PSC} + 1)\,\text{ARR}}, \qquad D = \frac{\text{CCR}x}{\text{ARR}}$$

**Worked Example (25.00 kHz Edge-Aligned PWM at 170 MHz):**
Choose `PSC = 0` ($f_{\text{CNT}} = 170\text{ MHz}$): $\text{ARR} = \frac{170{,}000{,}000}{25{,}000} - 1 = \mathbf{6799}$ (12.7 bits resolution!). For 37.5% duty: $\text{CCR1} = 0.375 \times 6800 = \mathbf{2550}$.

### 6.2 `BDTR.DTG[7:0]` Four-Range Piecewise Dead-Time Formula (RM0440)
Let $t_{\text{DTS}} = \frac{1}{f_{\text{TIMCLK}}} \times 2^{\text{CKD}}$ (for `CKD = 00` at 170 MHz, $t_{\text{DTS}} = \frac{1}{170\text{ MHz}} = 5.8824\text{ ns}$):

| `DTG[7:5]` Prefix | Formula ($t_{\text{DT}}$) | Max Dead-Time @ 170 MHz (`CKD=00`) |
|---|---|---|
| `0xx` (`0..127`) | $t_{\text{DT}} = \text{DTG}[6:0] \times t_{\text{DTS}}$ | $127 \times 5.882\text{ ns} = 747.1\text{ ns}$ |
| `10x` (`128..191`) | $t_{\text{DT}} = (64 + \text{DTG}[5:0]) \times 2\,t_{\text{DTS}}$ | $127 \times 11.765\text{ ns} = 1.494\ \mu\text{s}$ |
| `110` (`192..223`) | $t_{\text{DT}} = (32 + \text{DTG}[4:0]) \times 8\,t_{\text{DTS}}$ | $63 \times 47.059\text{ ns} = 2.965\ \mu\text{s}$ |
| `111` (`224..255`) | $t_{\text{DT}} = (32 + \text{DTG}[4:0]) \times 16\,t_{\text{DTS}}$ | $63 \times 94.118\text{ ns} = 5.929\ \mu\text{s}$ |

**Worked Example (500 ns Dead-Time @ 170 MHz):** $\text{DTG} = \text{round}(500\text{ ns} / 5.8824\text{ ns}) = \mathbf{85}$ (`0x55`) $\implies t_{\text{DT}} = 85 \times 5.8824\text{ ns} = \mathbf{500.0\text{ ns}}$.

### 6.3 `HRTIM1` Period & Resolution Math
$$f_{\text{HRTIM\_EQ}} = 32 \times f_{\text{HRTIM}} = 32 \times 170\text{ MHz} = 5.44\text{ GHz} \implies t_{\text{step}} = \mathbf{183.82\text{ ps}}$$
For a 200 kHz digital power stage: $\text{PER}x\text{R} = \frac{5.44\text{ GHz}}{200\text{ kHz}} = \mathbf{27{,}200}$ (14.73 bits of duty resolution at 200 kHz — impossible on a standard 170 MHz timer which only gets 850 steps = 9.7 bits!).

---

## 7. Official Documentation & Standards References

| Document | Exact Sections |
|---|---|
| **ST RM0440** | Ch. 27/28 (*HRTIM1*), Ch. 28/29 (*Advanced TIM1/8/20*), Ch. 29/30 (*GP TIM2–5, TIM15–17*), Ch. 31/33 (*Basic TIM6/7*), Ch. 32 (*LPTIM1*) — *note Rev 8 vs Rev 9 numbering* |
| **ST AN4013** | *STM32 Cross-Series Timer Overview* |
| **ST AN4539** | *HRTIM Cookbook* (essential guide for STM32G474RE power electronics) |
| **ST ES0430** | §2.11 (*HRTIM Errata*) |

---

## 8. Production-Grade Bare-Metal C Code Examples

> **Verification Status:** Statically Reviewed against `stm32g474xx.h` & RM0440 (Not Tested on Physical Hardware).

### 8.1 `TIM2` 10 Hz Periodic Interrupt (`EGR.UG` Shadow Load + `rc_w0` Direct Clear + `__DSB()`)
```c
#include "stm32g474xx.h"
#include <stdint.h>

void TIM2_Init_10Hz_170MHz(void)
{
    RCC->APB1ENR1 |= RCC_APB1ENR1_TIM2EN;
    (void)RCC->APB1ENR1;

    TIM2->CR1  = 0U;
    TIM2->PSC  = 16999U;          /* 170 MHz / 17000 = 10 kHz tick */
    TIM2->ARR  = 999U;            /* 10 kHz / 1000 = 10 Hz UEV */
    TIM2->EGR  = TIM_EGR_UG;      /* Force shadow load of PSC & ARR BEFORE enabling UIE! */
    TIM2->SR   = 0U;              /* Clear UG-generated UIF */
    TIM2->DIER = TIM_DIER_UIE;

    NVIC_SetPriority(TIM2_IRQn, 5U);
    NVIC_ClearPendingIRQ(TIM2_IRQn);
    NVIC_EnableIRQ(TIM2_IRQn);

    TIM2->CR1  = TIM_CR1_ARPE | TIM_CR1_CEN;
}

void TIM2_IRQHandler(void)
{
    if ((TIM2->SR & TIM_SR_UIF) != 0U) {
        TIM2->SR = ~TIM_SR_UIF;   /* rc_w0 direct write: NEVER use '&= ~UIF'! */
        GPIOA->BSRR = ((GPIOA->ODR & GPIO_ODR_OD5) != 0U) ? GPIO_BSRR_BR5 : GPIO_BSRR_BS5;
        __DSB();
    }
}
```

### 8.2 `TIM1` 50 kHz Complementary PWM + 500 ns Dead-Time + `COMP1` Hardware Break (`BKIN`)
```c
void TIM1_Complementary_PWM_50kHz_Init(void)
{
    /* Assumes 170 MHz SYSCLK, PA8 = TIM1_CH1 (AF6), PA7 = TIM1_CH1N (AF6) configured */
    RCC->APB2ENR |= RCC_APB2ENR_TIM1EN;
    (void)RCC->APB2ENR;

    TIM1->CR1   = 0U;
    TIM1->PSC   = 0U;             /* 170 MHz counter clock */
    TIM1->ARR   = 1700U;          /* Center-aligned: 170 MHz / (2 * 1700) = 50.00 kHz */
    TIM1->CCR1  = 850U;           /* 50.0% duty cycle */

    /* PWM Mode 1 (OC1M = 0110b) + Preload Enable (OC1PE = 1) */
    TIM1->CCMR1 = (6U << TIM_CCMR1_OC1M_Pos) | TIM_CCMR1_OC1PE;
    TIM1->CCER  = TIM_CCER_CC1E | TIM_CCER_CC1NE;

    /* Route COMP1_OUT directly into TIM1_BKIN for < 50 ns asynchronous overcurrent trip */
    TIM1->AF1   = TIM1_AF1_BKCMP1E;

    /* DTG = 85 -> 500.0 ns dead-time; OSSR|OSSI safe off-states; BKE=1; AOE=0 (latched fault!) */
    TIM1->BDTR  = (85U << TIM_BDTR_DTG_Pos)
                | TIM_BDTR_OSSR
                | TIM_BDTR_OSSI
                | TIM_BDTR_BKE;

    TIM1->EGR   = TIM_EGR_UG;
    TIM1->SR    = 0U;

    /* Enable Center-Aligned Mode 1 + ARPE + Main Output Enable (MOE) */
    TIM1->CR1   = TIM_CR1_CMS_0 | TIM_CR1_ARPE | TIM_CR1_CEN;
    TIM1->BDTR |= TIM_BDTR_MOE;
}
```

### 8.3 `HRTIM1` 184 ps DLL Calibration & Timer A 200 kHz PWM Skeleton
```c
void HRTIM1_TimerA_200kHz_Init(void)
{
    /* NOTE: PA9 (HRTIM1_CHA2) requires PWR->CR3 |= PWR_CR3_UCPD_DBDIS! */
    RCC->APB2ENR |= RCC_APB2ENR_HRTIM1EN;
    (void)RCC->APB2ENR;

    /* 1. Start DLL calibration + periodic auto-calibration (MANDATORY before starting timers!) */
    HRTIM1_COMMON->DLLCR = HRTIM_DLLCR_CAL | HRTIM_DLLCR_CALEN;
    while ((HRTIM1_COMMON->ISR & HRTIM_ISR_DLLRDY) == 0U) {
        /* Add bounded timeout in production */
    }

    /* 2. Timer A: CKPSC = 000 (x32 = 5.44 GHz, 184 ps), Continuous + Preload */
    HRTIM1_TIMA->PERxR  = 27200U; /* 5.44 GHz / 200 kHz = 27200 */
    HRTIM1_TIMA->CMP1xR = 13600U; /* 50% duty */
    HRTIM1_TIMA->SETx1R = HRTIM_SET1R_PER;  /* Set CHA1 on Period */
    HRTIM1_TIMA->RSTx1R = HRTIM_RST1R_CMP1; /* Reset CHA1 on CMP1 */
    HRTIM1_TIMA->TIMxCR = HRTIM_TIMCR_CONT | HRTIM_TIMCR_PREEN;

    /* 3. Enable CHA1 output & start Timer A counter */
    HRTIM1_COMMON->OENR = HRTIM_OENR_TA1OEN;
    HRTIM1->sMasterRegs.MCR = HRTIM_MCR_TACEN;
}
```

---

## 9. Practical Laboratory Experiments & Debugging Checkpoints

- **Lab 06:** [`lab-06-general-purpose-timer-periodic-interrupt`](../18-practical-laboratory/lab-06-general-purpose-timer-periodic-interrupt/README.md) — `TIM2` 10 Hz periodic ISR at 170 MHz (requires Stage 08 NVIC first!).
- **Lab 07:** [`lab-07-timer-pwm-rgb-led-fading`](../18-practical-laboratory/lab-07-timer-pwm-rgb-led-fading/README.md) — `TIM3` 25 kHz PWM with signed/safe duty ramping (zero unsigned underflow!).
- **GDB & Oscilloscope Checkpoints:**
  - Verify `TIMx->PSC`, `ARR`, `CCR1`, and `BDTR.MOE` in GDB (`x/18wx 0x40012C00` for `TIM1`).
  - Freeze timers during GDB breakpoints via `DBGMCU->APB1FZR1 |= DBGMCU_APB1FZR1_DBG_TIM2_STOP;` (`DBGMCU->APB2FZ` for `TIM1/8/15/16/17/20/HRTIM1`) so PWM outputs do not burn power stages while halted!
  - Measure `TIM1_CH1` vs `TIM1_CH1N` dead-time ($500\text{ ns} \pm 10\text{ ns}$) on a dual-channel oscilloscope.

---

## 10. Common Engineering Mistakes, Hardware Traps & Silicon Errata (ES0430)

1. **Forgetting `BDTR.MOE = 1` on `TIM1/8/15/16/17/20`:** Counter runs normally in GDB, `CCER.CC1E = 1`, GPIO AF is correct — yet physical output stays dead flat! Always set `TIMx->BDTR |= TIM_BDTR_MOE;`.
2. **Omitting `EGR = UG` Before `CEN`:** First period runs at reset `PSC = 0` (`ARR = 0xFFFF`), creating an unexpected glitch pulse or premature ISR.
3. **Using `TIMx->SR &= ~TIM_SR_UIF` on an `rc_w0` Register:** Silently clears concurrently asserted `CCxIF`/`BIF` flags! Always write `TIMx->SR = ~TIM_SR_UIF;`.
4. **Unsigned Integer Underflow in PWM Ramping:** `for (uint32_t d = 6799; d > 0; d -= 50)` wraps `49 - 50 -> 0xFFFFFFFF` and hangs for hours! Use signed `int32_t` or clamp before subtracting.
5. **Forgetting `HRTIM1` DLL Calibration (`DLLRDY`) or `PA9` `UCPD_DBDIS`:** Starting HRTIM before `DLLRDY == 1` produces undefined timing; `PA9` (`HRTIM1_CHA2`) carries a 5.1 kΩ pull-down until `PWR->CR3 |= PWR_CR3_UCPD_DBDIS`.
6. **Using Software Polling/RTOS Task for Power-Stage Overcurrent Trip:** Software latency (`10–100 µs`) destroys MOSFETs. Always wire `COMPx_OUT` directly to `TIMx_BKIN` / `HRTIM1` Fault for `< 50 ns` asynchronous hardware shutdown!

---

## 11. Progressive Coding Exercises & Design Challenges

1. **[Level 1]** Configure `TIM2` as a 32-bit `1 MHz` free-running microsecond counter (`PSC = 169` @ 170 MHz) with `APB1 = /2` and prove `TIM2CLK` stays `170 MHz` ($\times 2$ APB timer rule).
2. **[Level 2]** Wire `TIM3_CH1` (`PA6`, 25 kHz PWM) to `TIM2_CH1` (`PA0`) configured in **Hardware PWM Input Mode** (`CC1S=01`, `CC2S=10`, `SMS=0100` Reset) and read exact period (`CCR1`) and duty (`CCR2`) without polling.
3. **[Level 2]** Implement a 32-bit rollover-safe Quadrature Encoder tracker on `TIM3` (`SMS = 011b` X4 mode) with digital input filtering (`IC1F = IC2F = 6`).
4. **[Level 3]** Configure `TIM1` complementary PWM with `DTG = 0xCB` ($2.02\ \mu\text{s}$ dead-time) and verify hardware `BKIN` clears `BDTR.MOE` to `0` even while the CPU is halted at a GDB breakpoint.

---

## 12. Self-Assessment Mastery Questions

1. **Q:** Why must `TIMx->SR = ~TIM_SR_UIF` be used instead of `TIMx->SR &= ~TIM_SR_UIF`?  
   *Answer Check:* `TIMx->SR` is `rc_w0` (write-0-to-clear); RMW `&=` can inadvertently write `0` to another flag that transitioned `0 -> 1` between the read and write.
2. **Q:** What happens if you write a new `TIM3->CCR1` value mid-cycle with `OC1PE = 0` vs `OC1PE = 1`?  
   *Answer Check:* With `OC1PE = 0`, changing `CCR1` across the live `CNT` value causes a missing pulse or 100% glitch pulse; `OC1PE = 1` defers the update cleanly until the next Update Event.
3. **Q:** Calculate `BDTR.DTG` for a $1.00\ \mu\text{s}$ dead-time at $f_{\text{TIM1}} = 170\text{ MHz}$ (`CKD = 00`).  
   *Answer Check:* $1000\text{ ns} > 747\text{ ns}$ (Range 0 max), so use Range 1 (`10x`): $64 + x = \frac{1000}{2 \times 5.8824} = 85 \implies x = 21 \implies \text{DTG} = 0\text{x80} + 21 = \mathbf{0\text{x95}}$ ($t_{\text{DT}} = 1.000\ \mu\text{s}$).
4. **Q:** Why does `HRTIM1` achieve 184 ps resolution from a 170 MHz input clock?  
   *Answer Check:* An internal Delay-Locked Loop (DLL) generates 32 phase-locked delay taps per 170 MHz clock cycle ($170\text{ MHz} \times 32 = 5.44\text{ GHz} \implies 183.8\text{ ps}$).

---

## 13. Measurable Mastery & Completion Criteria

| Concept ID | Pass / Fail Verification Criterion | Measurement Tool |
|---|---|---|
| `PERIPH-TIM-001` / `002` | `TIM2` 1 MHz 32-bit counter + `TIM6` 100 kHz UEV with `EGR.UG` (zero first-cycle glitch) and `SR = ~UIF` | Oscilloscope + GDB |
| `PERIPH-PWM-001` | 25.00 kHz Center-Aligned PWM on `TIM3` with zero mid-cycle glitch during asynchronous `CCR1` updates (`OC1PE=1`) | Oscilloscope |
| `PERIPH-IC-002` / `PERIPH-ENC-001` | Measure PWM frequency/duty to $\pm 1$ tick (5.88 ns @ 170 MHz) via `TIM2` PWM Input Mode; X4 encoder tracks CW/CCW across 16-bit rollover | GDB / Logic Analyzer |
| `PERIPH-BDTR-001` / `002` | `TIM1` complementary PWM with $500\text{ ns} \pm 10\text{ ns}$ dead-time; `COMP1` $\to$ `BKIN` clears `MOE=0` in `< 50 ns` with CPU halted | Dual-Trace Oscilloscope |
| `PERIPH-TIM-004` (`HRTIM1`) | `HRTIM1_COMMON->ISR.DLLRDY == 1`; 200 kHz PWM on `CHA1` with verified sub-2 ns (`+10` count) duty step | High-BW Oscilloscope + GDB |
