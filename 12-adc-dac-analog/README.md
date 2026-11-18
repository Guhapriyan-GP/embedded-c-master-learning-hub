# Stage 11: Mixed-Signal Analog Subsystem (ADC1–5, DAC1–4, COMP1–7, OPAMP1–6, VREFBUF) & Hardware Math Accelerators (CORDIC & FMAC)

> **Directory:** `12-adc-dac-analog/` | **Target Platform:** STM32G474RET6 (5× 4 MSPS SAR ADCs, 4× DACs / 7 Ch, 7× COMP, 6× OPAMP, VREFBUF, CORDIC, FMAC)  
> **Code Verification Notice:** All bare-metal C snippets in this module are **Statically Reviewed against `stm32g474xx.h`, RM0440, DS12288, AN2834, AN5305, AN5325, and ES0430 (Not Tested on Physical Hardware)**.

---

## 1. Learning Objectives & Concept Scope

| Concept ID | Concept Name | Architectural Scope | Mastery Tier |
|---|---|---|---|
| `PERIPH-ADC-001` | 5× 12-Bit 4 MSPS SAR ADCs (`ADC1–5`), **Max 60 MHz / 52 MHz Clock Spec (`CKMODE=11` `HCLK/4`)**, Startup (`DEEPPWD` $\to$ `ADVREGEN` $\to 20\ \mu\text{s} \to$ `ADCAL` $\to$ `ADEN`), `rc_w1` `ISR`, & `ADDIS` | `[STM32G474RE Exact]` | Level 1–2 (Essential) |
| `PERIPH-ADC-002` | `SMPR` Source-Impedance RC Settling Formula ($< 0.5\text{ LSB}$), Internal Temp Sensor ($t_S \ge 5\ \mu\text{s}$, $t_{\text{START}} = 120\ \mu\text{s}$, `TS_CAL1/2`), & `VREFINT_CAL` | `[STM32G474RE Exact]` | Level 2 (Production) |
| `PERIPH-ADC-003` / `004` | Single, Scan, Continuous, Injected (`JSQR`/`JDR`), **Dual Simultaneous (`CCR.DUAL`, `CDR`)**, Hardware Oversampling (`CFGR2`), & `AWD1–3` | `[STM32G474RE Exact]` | Level 2–3 (Production) |
| `PERIPH-DAC-001` | **`DAC1–4`** (`AHB2` `0x50000800`: 3 External + 4 Internal 15 MSPS Channels, Mandatory **`MCR.HFSEL = 10b`** $>160\text{ MHz}$, Sawtooth/DMA) | `[STM32G474RE Exact]` | Level 2 (Production) |
| `PERIPH-COMP-001` / `PERIPH-OPAMP-001` / `PERIPH-VREFBUF-001` | **`COMP1–7`** (16.7 ns, Blanking, Direct `TIM1_BKIN` Trip), **`OPAMP1–6`** (PGA $\times 2..\times 64$, `TCMR`, Internal $\to$ ADC), & **`VREFBUF`** (`2.048/2.5/2.9 V`) | `[STM32G474RE Exact]` | Level 3 (Specialist) |
| `PERIPH-SIG-001` / `PERIPH-SENS-001` / `002` | Nyquist Anti-Aliasing Filters, Moving Average / Q15 EMA / Median Filters, & Fixed-Point Calibration | `[General C]` / `[STM32G474RE Exact]` | Level 2 (Production) |
| `PERIPH-CORDIC-001` / `PERIPH-FMAC-001` (`PROF-PERF-003`) | **STM32G4 Hardware Math Accelerators:** **`CORDIC`** (`0x40020C00`, 3–6 Cycle `sin/cos/atan2/sqrt`) & **`FMAC`** (`0x40021400`, 170 MMAC/s FIR/IIR/3p3z) | `[STM32G474RE Exact]` | Level 3 (Specialist) |

---

## 2. Prerequisites & Curriculum Positioning

### Prerequisite Concept IDs
- **Stage 01 (`02-c-fundamentals/`):** `DIG-ELEC-001` (RC time constant $\tau = RC$, voltage dividers, Nyquist sampling theorem).
- **Stage 06 (`07-rcc-and-clocks/`):** `STM32-RCC-001`, `STM32-RCC-002` (170 MHz `HCLK`, `PLL_P`, `RCC->AHB2ENR`, `RCC->CCIPR`).
- **Stage 07 (`08-gpio/`):** `PERIPH-GPIO-001` (Analog pin mode `MODER = 11b`, `PUPDR = 00b`).
- **Stage 08 (`10-interrupts/`):** `RT-IRQ-001`, `RT-IRQ-002` (`ADC1_2_IRQn`, `COMP1_2_3_IRQn`, `__DSB()`).
- **Stage 09 (`09-timers/`):** `PERIPH-SYNC-001` (`TIMx_TRGO`/`TRGO2` hardware triggers, `TIM1_BKIN` break).

---

## 3. Core Technical Theory & Engineering Principles

### 3.1 Five 12-Bit 4 MSPS SAR ADCs (`ADC1–5`) & Strict 60 MHz Clock Limit `[STM32G474RE Exact]`
STM32G474RE packs **5 independent 12-bit SAR ADCs** on **AHB2**: `ADC1` (`0x50000000`), `ADC2` (`0x50000100`), `ADC12_COMMON` (`0x50000300`), `ADC3` (`0x50000400`), `ADC4` (`0x50000500`), `ADC5` (`0x50000600`), `ADC345_COMMON` (`0x50000700`).

- **CRITICAL DS12288 CLOCK LIMIT (NEVER OVERCLOCK!):**
  - Maximum $f_{\text{ADC}} = \mathbf{60\text{ MHz}}$ (single ADC, $V_{\text{DDA}} \ge 2.7\text{ V}$) and **$\mathbf{52\text{ MHz}}$** (concurrent multi-ADC operation)!
  - At $f_{\text{HCLK}} = 170\text{ MHz}$, synchronous `CCR.CKMODE`:
    - `01b` (`HCLK / 1 = 170 MHz`) $\to$ **2.83× ILLEGAL OVERCLOCK!**
    - `10b` (`HCLK / 2 = 85 MHz`) $\to$ **ILLEGAL OVERCLOCK!**
    - **`11b` (`HCLK / 4 = 42.5 MHz`)** $\to$ **LEGAL & RECOMMENDED** for deterministic synchronous timing ($\le 52\text{ MHz}$ multi-ADC spec!).
    - Or use Asynchronous mode (`CKMODE = 00b` + `RCC->CCIPR.ADC12SEL = 01b` `PLL_P <= 60/52 MHz`).

- **Mandatory STM32G4 ADC Startup, Calibration & Shutdown Sequence:**
  1. Notice **`ADC->CR` resets to `0x20000000`** (`DEEPPWD = 1` at bit 29 — deep power-down is ON at reset!).
  2. Exit deep power-down: `ADC1->CR &= ~ADC_CR_DEEPPWD;`
  3. Enable internal voltage regulator: `ADC1->CR |= ADC_CR_ADVREGEN;`
  4. **Wait $\ge 20\ \mu\text{s}$** ($t_{\text{ADCVREG\_STUP}}$ per DS12288) — skipping this corrupts calibration!
  5. Ensure ADC is disabled (`ADEN == 0`), select single-ended (`ADCALDIF = 0`) or differential (`ADCALDIF = 1`), launch calibration `ADC1->CR |= ADC_CR_ADCAL;`, and wait until `ADCAL == 0`.
  6. Clear `ADRDY` via **direct `rc_w1` write**: **`ADC1->ISR = ADC_ISR_ADRDY;`** (NEVER `|=`!).
  7. Enable ADC: `ADC1->CR |= ADC_CR_ADEN;` and wait until `ADC1->ISR & ADC_ISR_ADRDY`.
  8. **Disabling ADC:** Writing `ADC1->CR &= ~ADC_CR_ADEN` does **NOTHING** (hardware ignores clearing `ADEN`)! You MUST set **`ADC1->CR |= ADC_CR_ADDIS`** and poll `while (ADC1->CR & ADC_CR_ADEN) {}`.

- **Operating Modes:** Single, Scan (`SQR1..4` up to 16 steps), Continuous (`CONT`), Discontinuous, **Injected (`JSQR`/`JDR1..4`, preempts regular scan for urgent phase-current capture)**, **Dual Simultaneous (`ADC12_COMMON->CCR.DUAL = 00110b`, packed 32-bit `CDR` via 1 DMA channel!)**, **Hardware Oversampling (`CFGR2.ROVSE`, $\times 2..\times 256$ up to 16-bit effective resolution)**, and **3 Analog Watchdogs (`TR1..3`)**.

### 3.2 `DAC1–4` (AHB2 — External & 15 MSPS Internal Channels) `[STM32G474RE Exact]`
**Correction to legacy STM32 assumptions:** On STM32G474RE, **`DAC1–4` reside on AHB2** (`RCC->AHB2ENR` bits 16–19, `0x50000800..0x50001400`), NOT APB1!
- **`DAC1` (`0x50000800`):** 2 channels — `PA4` (`DAC1_OUT1`) & `PA5` (`DAC1_OUT2`, shares pin with `LD2`!), external buffered (1 MSPS) + internal.
- **`DAC2` (`0x50000C00`):** 1 channel — `PA6` (`DAC2_OUT1`), external + internal.
- **`DAC3` (`0x50001000`) & `DAC4` (`0x50001400`):** 2 channels each (**4 internal-only channels** running up to **15 MSPS** directly into `COMP1–7` & `OPAMP1–6` for slope-compensated peak current-mode power control using the built-in **Sawtooth Generator `STR1/2`**!).
- **CRITICAL 170 MHz Requirement (`MCR.HFSEL[15:14]`):** When `f_HCLK > 160 MHz`, you **MUST set `DACx->MCR |= (2U << DAC_MCR_HFSEL_Pos)`** (`10b` = High-Frequency Interface Mode $> 160\text{ MHz}$)!

### 3.3 `COMP1–7`, `OPAMP1–6` & `VREFBUF` (All Clocked via `SYSCFGEN`!) `[STM32G474RE Exact]`
All three analog subsystems live on APB2 and share **`RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN`** (bit 0):
- **`COMP1–7` (`0x40010200..0x40010218`):** 16.7 ns rail-to-rail comparators with programmable hysteresis (`0..70 mV`), inverting inputs from `VREFINT` fractions or `DAC1–4`, **PWM Blanking (`BLANKSEL`)** to ignore MOSFET switching spikes, direct hardware trip into **`TIM1/8/15/16/17/20` `BKIN`** & **`HRTIM1` Faults/EEVs**, and safety one-way `LOCK` (bit 31).
- **`OPAMP1–6` (`0x40010300..0x40010314`):** 13 MHz GBW rail-to-rail op-amps with built-in **PGA gains ($\times 2, \times 4, \times 8, \times 16, \times 32, \times 64$ non-inverting; $-1..-63$ inverting)**, **`OPAINTOEN = 1`** routing output directly inside silicon to `ADC1–5` (zero external pins!), and **Timer-Controlled Multiplexer (`TCMR`)** switching inputs synchronously with PWM.
- **`VREFBUF` (`0x40010030`):** Precision internal voltage reference selectable to **`2.048 V` (`VRS=00`)**, **`2.500 V` (`VRS=01`)**, or **`2.900 V` (`VRS=10`)** (`ENVR=1, HIZ=0`, poll `VRR == 1`). **NUCLEO-G474RE Warning:** On MB1367, `VREF+` is shorted to `3V3` (`VDDA`) by default via solder bridge — open the `VREF+` bridge before enabling `VREFBUF` at `2.048 V`/`2.5 V` so it does not short against the 3.3 V rail!

### 3.4 Signal Conditioning & Fixed-Point Sensor DSP `[General C]`
- **Anti-Aliasing Filter:** Place a passive RC low-pass filter ($f_c = \frac{1}{2\pi RC} \le f_s / 10$) ahead of the ADC pin to attenuate frequencies above Nyquist ($f_s / 2$), while keeping $R$ within the ADC settling impedance budget (or buffer via internal `OPAMP` Voltage Follower!).
- **Digital Filtering:**
  - *O(1) Moving Average:* Circular sum buffer for white Gaussian noise reduction ($\sigma$ drops by $\sqrt{N}$).
  - *Fixed-Point Q15 EMA (1st-order IIR):* $y[n] = y[n-1] + \alpha(x[n] - y[n-1])$ using bit-shift $\alpha = 2^{-k}$ (zero float division!).
  - *3/5-Tap Median Filter:* Rejects impulsive EMI spikes without smearing step edges.

### 3.5 STM32G4 Hardware Math Accelerators: `CORDIC` & `FMAC` `[STM32G474RE Exact]`
1. **`CORDIC` (`0x40020C00`, `RCC->AHB1ENR.CORDICEN` b3, `CORDIC_IRQn = 100`):**
   - Fixed-point (`q1.31` / `q1.15`) hardware coprocessor computing **Cosine, Sine, Phase (`atan2`), Modulus (`hypot`), `atan`, `cosh`/`sinh`/`atanh`, `ln`, `sqrt`** in **3–6 CPU cycles** (~10× faster than FPU `sinf()`/`cosf()`!).
   - **Zero-Wait-State Polling Feature:** Reading `CORDIC->RDATA` automatically stalls the AHB bus until the result is ready — eliminating `while (!RRDY)` branch overhead!
2. **`FMAC` (`0x40021400`, `RCC->AHB1ENR.FMACEN` b4, `FMAC_IRQn = 101`):**
   - Hardware DSP filter engine with **256 × 16-bit internal SRAM** partitioned into circular `X1` (input), `X2` (coefficients), and `Y` (output/state) buffers executing **FIR** and **IIR Direct-Form I (Biquad / 3p3z digital power compensator)** at **170 MMAC/s** concurrently with the CPU!

---

## 4. Core Terminology & Acronyms

| Term | Engineering Definition |
|---|---|
| **`DEEPPWD` / `ADVREGEN`** | ADC Deep Power-Down (`CR` b29, `1` at reset!) & internal regulator enable (`CR` b28, needs $\ge 20\ \mu\text{s}$ startup). |
| **`ADCAL` / `ADDIS`** | Self-calibration trigger / Hardware-enforced ADC disable trigger (clearing `ADEN` directly is ignored). |
| **`TS_CAL1` / `TS_CAL2` / `VREFINT_CAL`** | Factory calibration words stored in System ROM (`0x1FFF75A8`, `0x1FFF75CA`, `0x1FFF75AA`) for accurate °C and $V_{\text{DDA}}$ math. |
| **`HFSEL` (DAC)** | DAC High-Frequency Interface Mode (`MCR[15:14]` = `10b` required when `HCLK > 160 MHz`). |
| **`CORDIC` / `FMAC`** | COordinate Rotation DIgital Computer (`0x40020C00`) / Filter Math Accelerator (`0x40021400`) on AHB1. |
| **`q1.31` / `q1.15`** | Signed fixed-point fractional formats representing $[-1.0,\ 1.0 - 2^{-31}]$ or $[-1.0,\ 1.0 - 2^{-15}]$. |

---

## 5. Hardware Register Architecture & Bitfield Reference

Verified directly against `stm32g474xx.h`:

| Peripheral | Base Address | Bus & Enable Bit | Key Registers & Offsets | `DMAMUX1` IDs |
|---|---|---|---|---|
| **`ADC1` / `ADC12_COMMON`** | `0x50000000` / `0x50000300` | AHB2 (`ADC12EN` b13) | `ISR@0x00` (`rc_w1`), `CR@0x08` (reset **`0x20000000`**), `CFGR@0x0C`, `CFGR2@0x10`, `SMPR1/2@0x14/18`, `SQR1..4@0x30..3C`, `DR@0x40`, `JSQR@0x4C`, `CALFACT@0xB4`; Common `CCR@0x08` (`CKMODE[17:16]`, `DUAL`, `VREFEN`, `VSENSESEL`), `CDR@0x0C` | `ADC1=5`, `ADC2=36`, `ADC3..5=37..39` |
| **`DAC1..4`** | `0x50000800..0x50001400` | AHB2 (`DAC1..4EN` b16–19) | `CR@0x00`, `DHR12R1@0x08`, `SR@0x34`, `MCR@0x3C` (**`HFSEL[15:14]`**, `MODE1/2`), `STR1/2@0x58/5C` | `DAC1=6/7`, `DAC2=41`, `DAC3/4=102..105` |
| **`COMP1..7`** | `0x40010200..0x40010218` | APB2 (`SYSCFGEN` b0) | Single `CSR@0x00` (`EN`, `INMSEL`, `INPSEL`, `HYST`, `BLANKSEL`, `VALUE`, `LOCK`) | None |
| **`OPAMP1..6`** | `0x40010300..0x40010314` | APB2 (`SYSCFGEN` b0) | `CSR@0x00` (`OPAEN`, `VM_SEL`, `HIGHSPEEDEN`, `OPAINTOEN`, `PGA_GAIN`), `TCMR@0x18` | None |
| **`VREFBUF`** | `0x40010030` | APB2 (`SYSCFGEN` b0) | `CSR@0x00` (`ENVR`, `HIZ`, `VRR`, `VRS[5:4]`), `CCR@0x04` | None |
| **`CORDIC`** | `0x40020C00` | AHB1 (`CORDICEN` b3) | `CSR@0x00` (`FUNC`, `PRECISION`, `NRES`, `NARGS`, `RRDY`), `WDATA@0x04`, `RDATA@0x08` | `RD=112`, `WR=113` |
| **`FMAC`** | `0x40021400` | AHB1 (`FMACEN` b4) | `X1/X2/YBUFCFG@0x00..08`, `PARAM@0x0C`, `CR@0x10`, `SR@0x14`, `WDATA@0x18`, `RDATA@0x1C` | `RD=110`, `WR=111` |

---

## 6. Engineering Mathematics & Worked Formulas

### 6.1 ADC Conversion Time & Source-Impedance RC Settling Equation (AN2834 / DS12288)
$$t_{\text{CONV}} = t_{\text{SMP}} + 12.5\ \text{ADC cycles}$$
To charge the internal sampling capacitor $C_{\text{ADC}} \approx 5\text{ pF}$ through external source impedance $R_{\text{AIN}}$ plus internal mux resistance $R_{\text{ADC}} \approx 1\text{ k}\Omega$ to within **$< 0.5\text{ LSB}$** at $N = 12$ bits:
$$t_{\text{SMP,min}} \ge (R_{\text{AIN}} + R_{\text{ADC}})\,C_{\text{ADC}}\,\ln\!\left(2^{N+1}\right) = (R_{\text{AIN}} + R_{\text{ADC}})\,C_{\text{ADC}} \times 9.011$$

**Worked Example A ($10\text{ k}\Omega$ Potentiometer at $f_{\text{ADC}} = 42.5\text{ MHz}$, $T_{\text{ADC}} = 23.53\text{ ns}$):**
$$t_{\text{SMP,min}} = (10{,}000 + 1{,}000)\times 5\times 10^{-12}\times 9.011 = 495.6\text{ ns} \implies \frac{495.6\text{ ns}}{23.53\text{ ns}} = 21.1\text{ cycles}$$
Select the next available `SMPR` encoding: **`24.5` cycles (`011b`)** or **`47.5` cycles (`100b`)** for safety margin!

**Worked Example B (Internal Temp Sensor `ADC1_IN16` at $42.5\text{ MHz}$):**
DS12288 mandates **$t_{S\_\text{temp}} \ge 5.0\ \mu\text{s}$** (plus **$120\ \mu\text{s}$** startup wait $t_{\text{START}}$ after setting `VSENSESEL`):
$$\text{Cycles}_{\min} = 5.0\ \mu\text{s} \times 42.5\text{ MHz} = 212.5\text{ cycles} \implies \text{Select } \mathbf{247.5\text{ cycles}}\ (5.82\ \mu\text{s},\ \text{SMP16} = \texttt{110b})\text{ or } 640.5\text{ cycles!}$$

### 6.2 Calibrated $V_{\text{DDA}}$ & Die Temperature Equations (RM0440 §21.4.32)
$$V_{\text{DDA}} = 3.0\text{ V} \times \frac{\text{VREFINT\_CAL}}{\text{ADC}_{\text{VREFINT}}}, \qquad T_{\text{die}}(^\circ\text{C}) = \frac{130 - 30}{\text{TS\_CAL2} - \text{TS\_CAL1}} \times \left(\text{ADC}_{\text{TEMP}} \cdot \frac{V_{\text{DDA}}}{3.0\text{ V}} - \text{TS\_CAL1}\right) + 30$$

---

## 7. Official Documentation & Standards References

| Document | Exact Sections |
|---|---|
| **ST RM0440** | Ch. 17 (*CORDIC*), Ch. 18 (*FMAC*), Ch. 21 (*ADC*), Ch. 22 (*DAC*), Ch. 23 (*VREFBUF*), Ch. 24 (*COMP*), Ch. 25 (*OPAMP*) |
| **ST DS12288** | §5.3.19 (*12-bit ADC: 60/52 MHz max, $t_{S\_\text{temp}} \ge 5\ \mu\text{s}$, $t_{\text{START}} = 120\ \mu\text{s}$*), §5.3.20–23 (*DAC/VREFBUF/COMP/OPAMP*) |
| **ST AN2834 / AN5305 / AN5306 / AN5325** | AN2834 (*ADC Accuracy*), AN5305 (*FMAC*), AN5306 (*OPAMP*), AN5325 (*CORDIC*) |

---

## 8. Production-Grade Bare-Metal C Code Examples

> **Verification Status:** Statically Reviewed against `stm32g474xx.h`, RM0440, and DS12288 (Not Tested on Physical Hardware).

### 8.1 Spec-Compliant `ADC1` Bring-Up at 170 MHz (`CKMODE=11` `42.5 MHz`, `rc_w1`, `ADDIS`)
```c
#include "stm32g474xx.h"
#include <stdint.h>

static void delay_us_approx_170mhz(uint32_t us)
{
    for (volatile uint32_t i = 0U; i < (us * 45U); ++i) { __NOP(); }
}

void ADC1_Init_SpecCompliant_170MHz(void)
{
    RCC->AHB2ENR |= RCC_AHB2ENR_ADC12EN | RCC_AHB2ENR_GPIOAEN;
    (void)RCC->AHB2ENR;

    /* PA0 (ADC1_IN1): Analog Mode (11b), No Pull */
    GPIOA->PUPDR &= ~GPIO_PUPDR_PUPD0_Msk;
    GPIOA->MODER |=  GPIO_MODER_MODE0_Msk;

    /* CRITICAL: CKMODE = 11b (HCLK/4 = 42.5 MHz <= 52 MHz multi-ADC / 60 MHz single spec!) */
    ADC12_COMMON->CCR = (ADC12_COMMON->CCR & ~ADC_CCR_CKMODE_Msk)
                      | (3U << ADC_CCR_CKMODE_Pos);

    /* Ensure ADC is disabled using ADDIS (NEVER clear ADEN directly!) */
    if ((ADC1->CR & ADC_CR_ADEN) != 0U) {
        ADC1->CR |= ADC_CR_ADDIS;
        while ((ADC1->CR & ADC_CR_ADEN) != 0U) {}
    }

    /* Exit Deep Power-Down (reset bit 29 = 1) -> Enable Regulator -> Wait >= 20 us */
    ADC1->CR &= ~ADC_CR_DEEPPWD;
    ADC1->CR |=  ADC_CR_ADVREGEN;
    delay_us_approx_170mhz(25U);

    /* Single-Ended Calibration */
    ADC1->CR &= ~ADC_CR_ADCALDIF;
    ADC1->CR |=  ADC_CR_ADCAL;
    while ((ADC1->CR & ADC_CR_ADCAL) != 0U) {}

    /* Clear ADRDY using direct '=' (rc_w1 — NEVER '|='!), then enable ADC */
    ADC1->ISR = ADC_ISR_ADRDY;
    ADC1->CR |= ADC_CR_ADEN;
    while ((ADC1->ISR & ADC_ISR_ADRDY) == 0U) {}

    /* Channel 1 sampling time = 47.5 cycles (1.12 us @ 42.5 MHz, safe for 10 kOhm source) */
    ADC1->SMPR1 = (ADC1->SMPR1 & ~ADC_SMPR1_SMP1_Msk) | (4U << ADC_SMPR1_SMP1_Pos);

    /* Regular sequence length = 1 conversion (L = 0), SQ1 = Channel 1 */
    ADC1->SQR1 = (1U << ADC_SQR1_SQ1_Pos);
}
```

### 8.2 `CORDIC` Zero-Wait-State Hardware `cos(theta)` & `sin(theta)` (q1.31)
```c
void CORDIC_CosSin_Q31(int32_t angle_rad_div_pi_q31, int32_t *cos_q31, int32_t *sin_q31)
{
    RCC->AHB1ENR |= RCC_AHB1ENR_CORDICEN;
    (void)RCC->AHB1ENR;

    /* FUNC=0 (Cos+Sin), PRECISION=6 (24 iterations -> ~6 cycles, 20-bit accuracy),
     * NRES=1 (2 outputs: cos then sin), NARGS=0 (1 input: angle), 32-bit q1.31 */
    CORDIC->CSR = (0U << CORDIC_CSR_FUNC_Pos)
                | (6U << CORDIC_CSR_PRECISION_Pos)
                | CORDIC_CSR_NRES;

    CORDIC->WDATA = (uint32_t)angle_rad_div_pi_q31;
    /* Hardware zero-wait-state polling: CPU automatically stalls AHB until RDATA ready! */
    *cos_q31 = (int32_t)CORDIC->RDATA;
    *sin_q31 = (int32_t)CORDIC->RDATA;
}
```

---

## 9. Practical Laboratory Experiments & Debugging Checkpoints

- **Lab 09:** [`lab-09-adc-single-channel-potentiometer`](../18-practical-laboratory/lab-09-adc-single-channel-potentiometer/README.md) — Spec-compliant 42.5 MHz `ADC1` bring-up & voltage conversion.
- **Lab 10:** [`lab-10-adc-dma-multichannel-circular-buffer`](../18-practical-laboratory/lab-10-adc-dma-multichannel-circular-buffer/README.md) — Multi-channel ADC + Temp Sensor (`SMP16 = 247.5` cycles + `120 µs` startup) + DMA.
- **GDB Verification Checkpoint:**
  ```text
  (gdb) x/1wx 0x50000308   # Verify ADC12_COMMON->CCR bits [17:16] == 0b11 (CKMODE = HCLK/4)
  (gdb) x/1wx 0x500000B4   # Inspect non-zero ADC1->CALFACT proving calibration succeeded
  (gdb) x/1hx 0x1FFF75AA   # Read factory VREFINT_CAL value directly from System ROM
  ```

---

## 10. Common Engineering Mistakes, Hardware Traps & Silicon Errata (ES0430)

1. **Overclocking ADC at 170 MHz (`CKMODE = 01` or `10`):** Violates DS12288 60 MHz / 52 MHz spec by 2.83×! Always use `CKMODE = 11` (`HCLK/4 = 42.5 MHz`) or `PLL_P <= 52 MHz`.
2. **Sampling Temp Sensor with Default 2.5 Cycles:** Violates DS12288 $t_{S\_\text{temp}} \ge 5\ \mu\text{s}$ by 85× and returns garbage temperature! Also wait $120\ \mu\text{s}$ after setting `VSENSESEL`.
3. **Writing `ADC1->CR &= ~ADC_CR_ADEN` or `ADC1->ISR |= ADC_ISR_ADRDY`:** `ADEN` can only be cleared via `ADDIS`; `ISR` is `rc_w1` so use direct `=`.
4. **Forgetting `DACx->MCR.HFSEL = 10b` at 170 MHz:** Corrupts DAC register synchronization above 160 MHz!
5. **Enabling `VREFBUF` at 2.048 V / 2.5 V While NUCLEO `VREF+` Bridge Is Closed:** Shorts `VREFBUF` output directly against the 3.3 V rail!

---

## 11. Progressive Coding Exercises & Design Challenges

1. **[Level 2]** Sample `VREFINT` (`ADC1_IN18`, $t_S \ge 4\ \mu\text{s}$) and Internal Temp Sensor (`ADC1_IN16`, `SMP16 = 247.5`) to compute real $V_{\text{DDA}}$ and calibrated die temperature in °C using `VREFINT_CAL`, `TS_CAL1`, and `TS_CAL2`.
2. **[Level 2]** Configure `OPAMP1` in internal PGA $\times 8$ mode (`OPAINTOEN = 1` $\to$ `ADC1_IN13`) and amplify a 200 mV input to $1.600\text{ V} \pm 15\text{ mV}$ with zero external op-amps.
3. **[Level 3]** Route `DAC3_CH1` (1.65 V threshold) and `PA1` into `COMP1`, wire `COMP1_OUT` directly to `TIM1_BKIN` (`TIM1->AF1.BKCMP1E=1`), and measure `< 50 ns` hardware PWM trip on oscilloscope.
4. **[Level 3]** Benchmark 1024 `CORDIC` `cos/sin` pairs vs `libm` `cosf()`/`sinf()` using `DWT->CYCCNT` and implement a 32-tap `FMAC` FIR low-pass filter verified against a NumPy `q1.15` reference.

---

## 12. Self-Assessment Mastery Questions

1. **Q:** Why is `ADC1->CR` equal to `0x20000000` immediately after reset?  
   *Answer Check:* Bit 29 (`DEEPPWD`) resets to `1` (Deep Power-Down enabled) to save static analog leakage current.
2. **Q:** At $f_{\text{ADC}} = 42.5\text{ MHz}$, what minimum `SMPR` setting satisfies a $10\text{ k}\Omega$ source ($C_{\text{ADC}} = 5\text{ pF}$, $R_{\text{ADC}} = 1\text{ k}\Omega$, 12-bit $< 0.5\text{ LSB}$)?  
   *Answer Check:* $495.6\text{ ns} \approx 21.1\text{ cycles} \implies$ minimum **`24.5` cycles** (`47.5` cycles recommended).
3. **Q:** Which RCC enable bit clocks `COMP1–7`, `OPAMP1–6`, and `VREFBUF`, and which bus hosts `DAC1–4`?  
   *Answer Check:* `COMP`/`OPAMP`/`VREFBUF` share **`RCC->APB2ENR.SYSCFGEN`**; `DAC1–4` live on **`AHB2`** (`RCC->AHB2ENR.DAC1..4EN`).
4. **Q:** Why does `CORDIC->RDATA` not require a `while (!(CORDIC->CSR & CORDIC_CSR_RRDY))` polling loop?  
   *Answer Check:* `CORDIC` supports AHB zero-wait-state hardware stalling — reading `RDATA` automatically inserts bus wait states until the calculation finishes.

---

## 13. Measurable Mastery & Completion Criteria

| Concept ID | Pass / Fail Verification Criterion | Measurement Tool |
|---|---|---|
| `PERIPH-ADC-001` / `002` | `ADC1` at `42.5 MHz` (`CKMODE=11`), non-zero `CALFACT`, calibrated $V_{\text{DDA}}$ & die temp within $\pm 2^\circ\text{C}$ (`SMP16 >= 5 us`) | GDB + DMM |
| `PERIPH-ADC-003` / `004` | 16× HW Oversampling reduces noise $\sigma$ by $\approx 4\times$; Dual Simultaneous `ADC1+ADC2` packs 32-bit `CDR` via single DMA | GDB / Python FFT |
| `PERIPH-DAC-001` | 10 kHz sine wave on `PA4` (`DAC1_OUT1`, `HFSEL=10`) via `TIM6` + circular DMA; `DAC3` drives `COMP1` internally | Oscilloscope |
| `PERIPH-COMP-001` / `OPAMP-001` | `OPAMP1` PGA $\times 8$ matches $1.600\text{ V} \pm 15\text{ mV}$; `COMP1` $\to$ `TIM1_BKIN` trips in `< 50 ns` | Oscilloscope |
| `PERIPH-CORDIC-001` / `FMAC-001` | `CORDIC` $\le 12$ cycles/pair (err $< 10^{-5}$); `FMAC` 32-tap FIR matches `q1.15` golden ref within $\pm 1\text{ LSB}$ (`SR.SAT==0`) | `DWT->CYCCNT` + GDB |
