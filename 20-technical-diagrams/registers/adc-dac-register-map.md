# ADC & DAC (High-Speed Analog Engine) Register Map

**Target MCU:** STM32G474RE (LQFP64 — **5× 12-bit 4 MSPS SAR ADCs**, **4× 12-bit DACs [7 channels]**)  
**ADC Bases (AHB2):** `ADC1` `0x50000000` | `ADC2` `0x50000100` | `ADC12_COMMON` `0x50000300` | `ADC3` `0x50000400` | `ADC4` `0x50000500` | `ADC5` `0x50000600` | `ADC345_COMMON` `0x50000700`  
**DAC Bases (AHB2):** `DAC1` `0x50000800` (2 ext ch: `PA4`/`PA5`) | `DAC2` `0x50000C00` (1 ext ch: `PA6`) | `DAC3` `0x50001000` (2 internal-only ch) | `DAC4` `0x50001400` (2 internal-only ch)  
**Reference Manual:** RM0440, Chapter 21 (*ADC*) & Chapter 22 (*DAC*) | DS12288 §5.3.19–5.3.20  
**Concept IDs:** `PERIPH-ADC-001`, `PERIPH-DAC-001`  
**Verification Status:** Every register offset, reset value, and clock spec verified against `stm32g474xx.h` & DS12288; C code snippets Compile-Verified (`arm-none-eabi-gcc 14.3.1`, `-Wall -Wextra -Werror`) (Not Tested on Physical Hardware).

---

### 1. 12-Bit SAR ADC Register Overview (`ADC_TypeDef` & `ADC_Common_TypeDef`)

| Register | Offset | Reset Value | Access | Description |
|---|---|---|---|---|
| `ADC_ISR` | `0x000` | `0x00000000` | **rc_w1** | Status (`ADRDY`, `EOSMP`, `EOC`, `EOS`, `OVR`, `JEOC`, `JEOS`, `AWD1–3`). **Write `=` directly — NEVER `|=`!** In ISRs follow with `(void)ADCx->ISR; __DSB();` (ARM AN321 §4.9 & ES0430 §2.1.3) |
| `ADC_IER` | `0x004` | `0x00000000` | RW | Interrupt enable (`ADRDYIE`, `EOCIE`, `EOSIE`, `OVRIE`, `AWD1IE–3IE`) |
| **`ADC_CR`** | `0x008` | **`0x20000000`** | RW | Control (**`DEEPPWD = 1` at bit 29 on reset!** `ADVREGEN` b28, `ADCALDIF` b30, `ADCAL` b31, `ADEN` b0, `ADSTART` b2) |
| `ADC_CFGR` | `0x00C` | `0x80000000` | RW | Config (`DMAEN`, `DMACFG`, `RES[4:3]`, `EXTSEL`, `EXTEN`, `OVRMOD`, `CONT`, `AUTDLY`) |
| `ADC_CFGR2` | `0x010` | `0x00000000` | RW | Hardware oversampling (`ROVSE`, `OVSR[4:2]` ×2..×256, `OVSS[8:5]`, `GCOMP` gain compensation) |
| `ADC_SMPR1/2` | `0x014/0x018` | `0x00000000` | RW | Channel sample times (3 bits/ch: `000`=2.5, `001`=6.5, `010`=12.5, `011`=24.5, `100`=47.5, `101`=92.5, `110`=247.5, `111`=640.5 cycles) |
| `ADC_TR1..3` | `0x020..0x028` | `0x0FFF0000` | RW | Analog Watchdog 1–3 thresholds |
| `ADC_SQR1..4` | `0x030..0x03C` | `0x00000000` | RW | Regular conversion sequence (`SQR1.L[3:0]` = length − 1; `SQ1..SQ16`) |
| `ADC_DR` | `0x040` | `0x00000000` | RO | Regular conversion data register (Reading clears `EOC`) |
| `ADC_JSQR` | `0x04C` | `0x00000000` | RW | Injected conversion sequence (up to 4 preemptive channels → `JDR1..4` @ `0x80..0x8C`) |
| `ADC_DIFSEL` | `0x0B0` | `0x00000000` | RW | Differential mode selection (configure only when `ADEN = 0`) |
| `ADC_CALFACT` | `0x0B4` | `0x00000000` | RW | Calibration factors (Single-ended `CALFACT_S[6:0]` & Differential `CALFACT_D[22:16]`) |
| `ADC_GCOMP` | `0x0C0` | `0x00000000` | RW | Hardware gain compensation coefficient |
| `ADCx_COMMON->CSR` | `+0x000` | `0x00000000` | RO | Common master/slave status |
| **`ADCx_COMMON->CCR`** | `+0x008` | `0x00000000` | RW | Common control (`DUAL[4:0]`, **`CKMODE[17:16]`**, `PRESC[21:18]`, `VREFEN` b22, `VSENSESEL` b23, `VBATSEL` b24) |

#### Critical ADC Clock Limit (DS12288 — DO NOT OVERCLOCK!)
Per **DS12288 §5.3.19**, maximum STM32G474RE $f_{\text{ADC}}$ is **60 MHz** (single ADC, $V_{\text{DDA}} \ge 2.7\ \text{V}$) or **52 MHz** (multi-ADC concurrent):
- At `HCLK = 170 MHz` (Profile A) or `160 MHz` (Profile B):
  - `CKMODE = 01` (`HCLK/1 = 170 / 160 MHz`) → **ILLEGAL (overclock!)**
  - `CKMODE = 10` (`HCLK/2 = 85 / 80 MHz`) → **ILLEGAL (overclock!)**
  - **`CKMODE = 11` (`HCLK/4 = 42.5 MHz` @ 170 MHz; exact `40.000 MHz` @ 160 MHz!) → VALID ($\le 60\ \text{MHz}$ and $\le 52\ \text{MHz}$)!**
  - Alternatively, use asynchronous mode (`CKMODE = 00`, `RCC->CCIPR.ADC12SEL = 01` PLL_P ≤ 60 MHz).
- Total conversion time: $t_{\text{CONV}} = t_{\text{SMP}} + 12.5\ \text{cycles}$ (12-bit). Internal temp sensor (`ADC1_IN16`) requires $t_S \ge 5\ \mu\text{s}$ ($\ge 247.5$ cycles at 42.5 MHz or 40.0 MHz).

---

### 2. 12-Bit DAC Register Overview (`DAC_TypeDef` — AHB2 Bus!)

> **Bus & High-Speed Warning:** On STM32G474RE, `DAC1–DAC4` reside on **AHB2** (`RCC->AHB2ENR.DAC1EN..DAC4EN`, NOT APB1!). At **170 MHz HCLK**, you **MUST set `DACx->MCR.HFSEL = 10b` (`> 160 MHz`)**; at **160 MHz HCLK** (Profile B), **`HFSEL = 01b` (`> 80 MHz` and $\le 160\text{ MHz}$)** is sufficient!

| Register | Offset | Reset Value | Access | Description |
|---|---|---|---|---|
| `DAC_CR` | `0x000` | `0x00000000` | RW | Control (`EN1/2`, `TEN1/2`, `TSEL1/2[3:0]`, `WAVE1/2` noise/triangle/sawtooth, `DMAEN1/2`, `DMAUDRIE1/2`, `CEN1/2`) |
| `DAC_SWTRIGR` | `0x004` | `0x00000000` | WO | Software trigger (`SWTRIG1/2`, `SWTRIGB1/2` sawtooth step) |
| `DAC_DHR12R1` | `0x008` | `0x00000000` | RW | Channel 1 12-bit right-aligned data holding register |
| `DAC_DHR12L1` | `0x00C` | `0x00000000` | RW | Channel 1 12-bit left-aligned data holding register |
| `DAC_DHR12R2` | `0x014` | `0x00000000` | RW | Channel 2 12-bit right-aligned data holding register |
| `DAC_DHR12RD` | `0x020` | `0x00000000` | RW | Dual-channel simultaneous 12-bit right-aligned data holding register |
| `DAC_DOR1/2` | `0x02C/0x030` | `0x00000000` | RO | Current DAC output data registers |
| `DAC_SR` | `0x034` | `0x00000000` | RO/**rc_w1** | Status (`DAC1RDY`, `DORSTAT1`, `DMAUDR1/2` [`rc_w1`: direct `DACx->SR = DAC_SR_DMAUDR1;`, NEVER `|=`; follow with `(void)DACx->SR; __DSB();` in ISRs], `CAL_FLAG1`, `BWST1`) |
| `DAC_CCR` | `0x038` | `0x00000000` | RW | Buffer offset calibration control (`OTRIM1[4:0]`, `OTRIM2[20:16]`) |
| **`DAC_MCR`** | `0x03C` | `0x00000000` | RW | Mode control (`MODE1[2:0]`, **`HFSEL[15:14]`**: `00` ≤80 MHz, `01` >80 MHz, **`10` >160 MHz**, `MODE2[18:16]`) |
| `DAC_STR1/2` | `0x058/0x05C` | `0x00000000` | RW | Hardware Sawtooth wave reset value & step increment (for digital power slope compensation!) |
| `DAC_STMODR` | `0x060` | `0x00000000` | RW | Sawtooth trigger selection (reset & increment triggers from TIM/HRTIM) |

---

### 3. CMSIS Code Examples

> **Status:** Statically Reviewed (Not Tested on Physical Hardware).

#### Example 1: Compliant 42.5 MHz `ADC1` Init & Single-Shot Read (`PA0` = `ADC1_IN1`)
```c
#include "stm32g474xx.h"

void ADC1_Init_42p5MHz(void) {
    /* 1. Enable ADC12 & GPIOA clocks on AHB2; PA0 defaults to Analog (11b) */
    RCC->AHB2ENR |= RCC_AHB2ENR_ADC12EN | RCC_AHB2ENR_GPIOAEN;
    (void)RCC->AHB2ENR;
    GPIOA->MODER |= (3U << (0U * 2U)); /* Ensure PA0 Analog */

    /* 2. CRITICAL: Select CKMODE = 11b (HCLK/4 = 42.5 MHz <= 60 MHz spec!) */
    ADC12_COMMON->CCR = (ADC12_COMMON->CCR & ~ADC_CCR_CKMODE_Msk) |
                        (3U << ADC_CCR_CKMODE_Pos);

    /* 3. Exit Deep Power-Down (reset value 0x20000000), enable VREG, wait >= 20 us */
    ADC1->CR &= ~ADC_CR_DEEPPWD;
    ADC1->CR |=  ADC_CR_ADVREGEN;
    for (volatile uint32_t i = 0U; i < 4000U; i++) { __NOP(); }

    /* 4. Single-ended self-calibration (must have ADEN = 0) */
    ADC1->CR &= ~ADC_CR_ADCALDIF;
    ADC1->CR |=  ADC_CR_ADCAL;
    while (ADC1->CR & ADC_CR_ADCAL) { }

    /* 5. Clear ADRDY via direct rc_w1 assignment (NEVER |=), then enable ADC */
    ADC1->ISR = ADC_ISR_ADRDY;
    ADC1->CR |= ADC_CR_ADEN;
    while (!(ADC1->ISR & ADC_ISR_ADRDY)) { }

    /* 6. Sequence length L = 0 (1 conv), SQ1 = Channel 1, Sample time = 47.5 cycles */
    ADC1->SQR1  = (1U << ADC_SQR1_SQ1_Pos);
    ADC1->SMPR1 = (ADC1->SMPR1 & ~ADC_SMPR1_SMP1_Msk) | (4U << ADC_SMPR1_SMP1_Pos);
}

uint16_t ADC1_ReadOnce(void) {
    ADC1->CR |= ADC_CR_ADSTART;
    while (!(ADC1->ISR & ADC_ISR_EOC)) { }
    return (uint16_t)(ADC1->DR & 0x0FFFU);
}
```

#### Example 2: `DAC1` Channel 1 (`PA4`) Init at 170 MHz (`HFSEL = 10b`)
```c
void DAC1_CH1_Init_170MHz(void) {
    RCC->AHB2ENR |= RCC_AHB2ENR_DAC1EN | RCC_AHB2ENR_GPIOAEN;
    (void)RCC->AHB2ENR;
    GPIOA->MODER |= (3U << (4U * 2U)); /* PA4 Analog */

    /* CRITICAL at 170 MHz: HFSEL = 10b (>160 MHz), MODE1 = 000b (external buffered) */
    DAC1->MCR = (2U << DAC_MCR_HFSEL_Pos);
    DAC1->CR  = DAC_CR_EN1;
    while (!(DAC1->SR & DAC_SR_DAC1RDY)) { }

    DAC1->DHR12R1 = 2048U; /* Mid-scale (~VREF+/2) */
}
```
