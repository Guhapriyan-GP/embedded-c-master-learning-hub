# Lab 10: Multichannel ADC1 (Internal `VREFINT` + Die Temp) + Circular Ping-Pong DMA1

> **Verification Status:** `CROSS-COMPILED` (`arm-none-eabi-gcc 14.3.1`, `-Wall -Wextra -Werror -O2`, 0 warnings) | **Hardware Queue:** `ONBOARD — READY FOR AGENT 4` *(Not Yet Executed on Physical NUCLEO-G474RE Hardware)*

> **Clock Configuration Profile:** **Primary = `170 MHz Range 1 Boost`** (`CKMODE = 11b` $\implies f_{\text{ADC}} = 42.5\text{ MHz} \le 60\text{ MHz}$, `SMP16`/`SMP18 = 640.5` cyc $= 15.07\ \mu\text{s}$) | **Alternate = `160 MHz Range 1 Boost`** (`CKMODE = 11b` $\implies f_{\text{ADC}} = 40.0\text{ MHz} \le 60\text{ MHz}$, `SMP16`/`SMP18 = 640.5` cyc $= 16.01\ \mu\text{s}$) | **Reusable Drivers:** [`23-code-library/Drivers/adc_g4.h`](../../23-code-library/Drivers/adc_g4.h) (`adc_init_single_ended()`, `adc_calc_vdda_mv()`, `adc_temp_mdegc()`), [`dma_g4.h`](../../23-code-library/Drivers/dma_g4.h) (`dma_config_p2m_circular()`, `dma_clear_flags()`, `DMAMUX_REQ_ADC1`), & [`clock_g4.h`](../../23-code-library/Drivers/clock_g4.h).

---

## 1. Objective & Concept IDs

Continuously scan **two self-contained onboard internal channels** on **`ADC1`** — Channel 18 (**Internal Reference Voltage `VREFINT`**, $\approx 1.212\text{ V}$) and Channel 16 (**Internal Die Temperature Sensor**) — into a `volatile` SRAM ping-pong circular buffer via **`DMA1 Channel 1`** and **`DMAMUX1 Channel 0`**, requiring **zero external wiring**! Enforce all **DS12288** silicon constraints (`CKMODE = 11b` $\implies 42.5\text{ MHz} \le 60\text{ MHz}$, `SMPR2.SMP18 = SMP16 = 640.5` cycles $= 15.07\ \mu\text{s} \ge 5\ \mu\text{s}$, and $\ge 120\ \mu\text{s}$ temperature sensor startup $t_{\text{START}}$), compute true calibrated **`g_vdda_mv`** and $V_{\text{DDA}}$-compensated **`g_temp_deci_c`** from factory ROM calibration constants, and process each half-buffer deterministically via **`HTIE`/`TCIE`** interrupts. *(Optional: add external `PA0` `ADC1_IN1` as a 3rd scan channel — see Extension Exercise 2.)*

- **Target Concept IDs:** `PERIPH-DMA-001` (Circular Ping-Pong DMA & DMAMUX1), `PERIPH-DMA-002` (DMAMUX Request Routing), `PERIPH-ADC-001` (Scan Mode, `VREFINT` & Internal Temp Sensor $t_S$), `EMB-VOL-001` (`volatile` DMA Buffers), `EMB-BAR-001` (`__DMB()`/`__DSB()` DMA Ownership Handoff), `STM32-ERRATA-001` (ES0430 §2.3.1 Individual `IFCR` Clear)
- **Scope Layer:** `[STM32G474RE Exact]`

---

## 2. Required Hardware

- **Development Board:** ST NUCLEO-G474RE (`MB1367` Rev C) — **100% self-contained onboard experiment** (both `VREFINT` on `ADC1_IN18` and Die Temperature Sensor on `ADC1_IN16` are internal to the STM32G474RET6 silicon; zero external wiring required!)
- **Optional External Expansion:** `10 kΩ` potentiometer on `PA0` (`ADC1_IN1`) if extending to a 3-channel scan (Extension Exercise 2)

---

## 3. Required Software

- `arm-none-eabi-gcc` (14.3.1+), `make`, `openocd`, `arm-none-eabi-gdb`
- CMSIS headers (`stm32g474xx.h`, `core_cm4.h`)

---

## 4. Wiring & Pin Configuration

Zero external pins are required for the primary 2-channel self-contained onboard scan:

| Regular Rank | ADC1 Channel | Signal | Internal Enable Bit | Sampling Time Configured |
|---|---|---|---|---|
| `SQ1` | `ADC1_IN18` | Internal `VREFINT` ($\approx 1.212\text{ V}$) | `ADC12_COMMON->CCR.VREFEN` (b22) | **`640.5` cycles (`15.07 µs >= 4.0 µs` DS12288 min)** |
| `SQ2` | `ADC1_IN16` | Internal Die Temp Sensor | `ADC12_COMMON->CCR.VSENSESEL` (b23) | **`640.5` cycles (`15.07 µs >= 5.0 µs` DS12288 min)** |

*(Optional 3rd channel: connect a `10 kΩ` potentiometer wiper to `PA0` (`ADC1_IN1`, `MODER = 11b`, `PUPDR = 00b`, `SMP1 = 47.5` cycles) and increment `SQR1.L` to `2`.)*

---

## 5. Register Configuration & Mathematical Derivation

**Official Documentation:** **RM0440 Chapter 11/12** (*DMA* & *DMAMUX*, Table 91 Request IDs), **RM0440 Chapter 21** (*ADC*, §21.4.32), **DS12288 §3.14 & §5.3.19–5.3.24** (*Calibration ROM, ADC, VREFINT & Temp Sensor*), and **ST ES0430 §2.3.1** (*DMA `CGIFx` errata*).

### 5.1 Explicit Clock Configuration Profile Comparison (`170 MHz` vs `160 MHz` Range 1 Boost)

Both `170 MHz` and `160 MHz` exceed `2 × 60 MHz = 120 MHz`, so **BOTH MUST use `CKMODE = 11b` (`HCLK / 4`)** to stay safely inside DS12288's $60\text{ MHz}$ ADC ceiling:

| Clock Profile | `clock_g4.h` API | `ADC12_COMMON->CCR.CKMODE` | $f_{\text{ADC}}$ | `SMP18`/`SMP16` (`640.5` cyc) $t_S$ | Single-Pair Scan Period (`2 × 653` cyc) | Ping-Pong Half-Buffer (`16` Pairs) Rate |
|---|---|---|---|---|---|---|
| **`170 MHz Range 1 Boost` *(Primary)*** | `Clock_Init_170MHz()` | **`11b`** (`HCLK / 4`) | **`42.5 MHz`** ($\le 60\text{ MHz}$) | **`15.07 µs`** ($\ge 5\ \mu\text{s}$) | `30.73 µs` (`32.54 kHz`) | `2.03 kHz` (`491.7 µs` / half) |
| **`160 MHz Range 1 Boost` *(Alternate)*** | `Clock_Init_160MHz()` | **`11b`** (`HCLK / 4`) | **`40.0 MHz`** ($\le 60\text{ MHz}$) | **`16.01 µs`** ($\ge 5\ \mu\text{s}$) | `32.65 µs` (`30.63 kHz`) | `1.91 kHz` (`522.4 µs` / half) |
| **`16 MHz HSI16` *(Reset Fallback)*** | Reset default | `01b` (`HCLK / 1`) | `16.0 MHz` | `40.03 µs` ($\ge 5\ \mu\text{s}$) | `81.63 µs` (`12.25 kHz`) | `765.6 Hz` (`1.306 ms` / half) |

- **Reusable Production Drivers (`23-code-library/Drivers/`):** Combine `Clock_Init_170MHz()` / `Clock_Init_160MHz()` ([`clock_g4.h`](../../23-code-library/Drivers/clock_g4.h)), `dma_config_p2m_circular(DMA1, 1U, DMAMUX_REQ_ADC1, &ADC1->DR, adc_dma_buffer, FULL_BUF_LEN, 2U, true, 3U)` + `dma_clear_flags()` ([`dma_g4.h`](../../23-code-library/Drivers/dma_g4.h)), and `adc_init_single_ended()` / `adc_calc_vdda_mv()` / `adc_temp_mdegc()` ([`adc_g4.h`](../../23-code-library/Drivers/adc_g4.h)).

### 5.2 Silicon Spec Compliance & Factory Calibration Math

1. **ADC Clock:** `CKMODE = 11b` $\implies f_{\text{ADC}} = 170\text{ MHz} / 4 = 42.5\text{ MHz} \le 60\text{ MHz}$ (`40.0 MHz` @ `160 MHz`).
2. **Sampling Time ($t_{S,\text{VREFINT}} \ge 4\ \mu\text{s}$, $t_{S,\text{temp}} \ge 5\ \mu\text{s}$ per DS12288):**
   - Reset `SMPR2` (`2.5` cycles = `58.8 ns`) violates the $5\ \mu\text{s}$ minimum by **85×**!
   - Setting **`SMPR2.SMP18 = 111b`** and **`SMPR2.SMP16 = 111b`** (`640.5` cycles) yields:
     $$t_{S} = \frac{640.5}{42.5\text{ MHz}} = 15.07\ \mu\text{s} \ge 5.0\ \mu\text{s} \quad (16.01\ \mu\text{s}\text{ at }160\text{ MHz})\ \checkmark$$
3. **Startup Delay ($t_{\text{START}} = 120\ \mu\text{s}$ max per DS12288):** Wait $\ge 150\ \mu\text{s}$ after asserting `ADC_CCR_VREFEN | ADC_CCR_VSENSESEL` before setting `ADSTART`.
4. **Factory-Calibrated $V_{\text{DDA}}$ & Die Temperature Formulas (DS12288 §3.14, RM0440 §21.4.32):**
   - `TS_CAL1` (`0x1FFF75A8`, 16-bit): measured at $30\ ^\circ\text{C}$, $V_{\text{DDA}} = 3.0\text{ V}$.
   - `VREFINT_CAL` (`0x1FFF75AA`, 16-bit): measured at $30\ ^\circ\text{C}$, $V_{\text{DDA}} = 3.0\text{ V}$.
   - `TS_CAL2` (`0x1FFF75CA`, 16-bit): measured at $130\ ^\circ\text{C}$, $V_{\text{DDA}} = 3.0\text{ V}$.
   - Actual supply voltage from `ADC1_IN18`:
     $$V_{\text{DDA}}\ (\text{mV}) = \frac{3000 \times \text{VREFINT\_CAL}}{\text{avg\_vrefint}}$$
   - True $V_{\text{DDA}}$-compensated temperature from `ADC1_IN16`:
     $$\text{TS\_DATA}_{3.0\text{V}} = \text{avg\_temp} \times \frac{V_{\text{DDA}}}{3000}, \qquad T\ (^\circ\text{C}) = 30 + \frac{100 \times (\text{TS\_DATA}_{3.0\text{V}} - \text{TS\_CAL1})}{\text{TS\_CAL2} - \text{TS\_CAL1}}$$

### 5.3 Ping-Pong Buffer Geometry & DMAMUX Routing

- `NUM_CHANNELS = 2`, `PAIRS_PER_HALF = 16` $\implies$ Half buffer = `32` halfwords (`16` pairs `[VREFINT, Temp]`), Full buffer = `64` halfwords (`128` bytes in DMA-accessible SRAM1 — never native CCM-SRAM `0x10000000`!).
- **`DMAMUX1_Channel0->CCR = 5U`** maps `ADC1` (Request ID `5`) to `DMA1_Channel1`.
- **`DMA1_Channel1->CCR`** enables `CIRC | MINC | PSIZE=16b | MSIZE=16b | HTIE | TCIE | TEIE`.
- **ES0430 §2.3.1 Compliance:** Inside `DMA1_Channel1_IRQHandler`, clear **individual** flags (`DMA_IFCR_CHTIF1`, `DMA_IFCR_CTCIF1`, `DMA_IFCR_CTEIF1`) — NEVER write `DMA_IFCR_CGIF1`!

---

## 6. Code Implementation

```c
#include "stm32g474xx.h"
#include <stdint.h>

extern uint32_t SystemCoreClock;

#define NUM_CHANNELS        (2U)
#define PAIRS_PER_HALF      (16U)
#define HALF_BUF_LEN        (NUM_CHANNELS * PAIRS_PER_HALF) /* 32 halfwords */
#define FULL_BUF_LEN        (HALF_BUF_LEN * 2U)             /* 64 halfwords */

/* Factory calibration addresses for STM32G474RE (DS12288 §3.14) */
#define TS_CAL1_ADDR        ((const volatile uint16_t *)0x1FFF75A8UL) /* 30 C @ 3.0 V  */
#define VREFINT_CAL_ADDR    ((const volatile uint16_t *)0x1FFF75AAUL) /* 30 C @ 3.0 V  */
#define TS_CAL2_ADDR        ((const volatile uint16_t *)0x1FFF75CAUL) /* 130 C @ 3.0 V */

/* CRITICAL: volatile prevents compiler from hoisting/caching DMA-written SRAM */
static volatile uint16_t adc_dma_buffer[FULL_BUF_LEN];

/* Ping-pong ownership flags set by DMA1_Channel1_IRQHandler */
static volatile uint32_t g_half_ready = 0U;
static volatile uint32_t g_full_ready = 0U;
static volatile uint32_t g_dma_errors = 0U;

/* Self-contained onboard outputs inspectable in GDB (zero external wiring!) */
static volatile uint32_t g_vdda_mv     = 0U; /* Expected ~3250..3350 mV on NUCLEO-G474RE */
static volatile int32_t  g_temp_deci_c = 0;  /* Tenths of deg C (e.g., 285 = 28.5 C)     */
static volatile uint32_t g_blocks_done = 0U;

static void Clock_Init_170MHz(void) {
    RCC->APB1ENR1 |= RCC_APB1ENR1_PWREN;
    volatile uint32_t tmpreg = RCC->APB1ENR1;
    (void)tmpreg;

    PWR->CR1 = (PWR->CR1 & ~PWR_CR1_VOS) | PWR_CR1_VOS_0;
    while ((PWR->SR2 & PWR_SR2_VOSF) != 0U) {}

    PWR->CR5 &= ~PWR_CR5_R1MODE;

    FLASH->ACR = (FLASH->ACR & ~FLASH_ACR_LATENCY) |
                 FLASH_ACR_LATENCY_4WS             |
                 FLASH_ACR_PRFTEN | FLASH_ACR_ICEN | FLASH_ACR_DCEN;
    while ((FLASH->ACR & FLASH_ACR_LATENCY) != FLASH_ACR_LATENCY_4WS) {}

    RCC->CR |= RCC_CR_HSION;
    while ((RCC->CR & RCC_CR_HSIRDY) == 0U) {}

    RCC->PLLCFGR = RCC_PLLCFGR_PLLSRC_HSI              |
                   ((4U - 1U) << RCC_PLLCFGR_PLLM_Pos) |
                   (85U       << RCC_PLLCFGR_PLLN_Pos) |
                   (0U        << RCC_PLLCFGR_PLLR_Pos) |
                   RCC_PLLCFGR_PLLREN;

    RCC->CR |= RCC_CR_PLLON;
    while ((RCC->CR & RCC_CR_PLLRDY) == 0U) {}

    RCC->CFGR = (RCC->CFGR & ~(RCC_CFGR_HPRE | RCC_CFGR_PPRE1 | RCC_CFGR_PPRE2)) |
                RCC_CFGR_HPRE_DIV2;
    RCC->CFGR = (RCC->CFGR & ~RCC_CFGR_SW) | RCC_CFGR_SW_PLL;
    while ((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_PLL) {}

    for (volatile uint32_t i = 0U; i < 120U; i++) {
        __NOP();
    }

    RCC->CFGR &= ~RCC_CFGR_HPRE;
    SystemCoreClock = 170000000UL;
}

static void delay_us_170mhz(uint32_t us) {
    for (uint32_t u = 0U; u < us; u++) {
        for (volatile uint32_t i = 0U; i < 60U; i++) {
            __NOP();
        }
    }
}

void DMA1_Channel1_IRQHandler(void) {
    uint32_t isr = DMA1->ISR;

    if ((isr & DMA_ISR_HTIF1) != 0U) {
        DMA1->IFCR = DMA_IFCR_CHTIF1; /* Clear individual HT flag (never CGIF1 — ES0430 §2.3.1!) */
        g_half_ready = 1U;
    }
    if ((isr & DMA_ISR_TCIF1) != 0U) {
        DMA1->IFCR = DMA_IFCR_CTCIF1; /* Clear individual TC flag */
        g_full_ready = 1U;
    }
    if ((isr & DMA_ISR_TEIF1) != 0U) {
        DMA1->IFCR = DMA_IFCR_CTEIF1;
        g_dma_errors++;
    }

    __DSB();
}

static void process_half_buffer(uint32_t offset) {
    uint32_t sum_vref = 0U;
    uint32_t sum_temp = 0U;

    __DMB(); /* Ensure DMA writes to SRAM are visible before CPU reads */
    for (uint32_t i = 0U; i < PAIRS_PER_HALF; i++) {
        sum_vref += adc_dma_buffer[offset + (i * 2U) + 0U]; /* SQ1: ADC1_IN18 (VREFINT) */
        sum_temp += adc_dma_buffer[offset + (i * 2U) + 1U]; /* SQ2: ADC1_IN16 (Temp)    */
    }

    uint32_t avg_vref = sum_vref / PAIRS_PER_HALF;
    uint32_t avg_temp = sum_temp / PAIRS_PER_HALF;

    /* 1. Compute true VDDA in mV using factory VREFINT_CAL (@ 3.0 V) */
    uint32_t vref_cal = (uint32_t)(*VREFINT_CAL_ADDR);
    uint32_t vdda_mv  = (avg_vref > 0U) ? ((3000UL * vref_cal) / avg_vref) : 3300UL;
    g_vdda_mv = vdda_mv;

    /* 2. Scale measured Temp raw value by actual VDDA/3.0 V and interpolate TS_CAL1..TS_CAL2 */
    int32_t ts_cal1   = (int32_t)(*TS_CAL1_ADDR);
    int32_t ts_cal2   = (int32_t)(*TS_CAL2_ADDR);
    int32_t ts_scaled = (int32_t)((avg_temp * vdda_mv) / 3000UL);
    if (ts_cal2 > ts_cal1) {
        g_temp_deci_c = 300 + ((1000 * (ts_scaled - ts_cal1)) / (ts_cal2 - ts_cal1));
    }

    g_blocks_done++;
}

static void DMA_ADC1_Init(void) {
    /* 1. Enable DMA1, DMAMUX1 (AHB1) and ADC12 (AHB2) clocks — zero external GPIO pins needed! */
    RCC->AHB1ENR |= RCC_AHB1ENR_DMA1EN | RCC_AHB1ENR_DMAMUX1EN;
    RCC->AHB2ENR |= RCC_AHB2ENR_ADC12EN;
    volatile uint32_t tmpreg = RCC->AHB2ENR;
    (void)tmpreg;

    /* 2. Configure DMAMUX1 Channel 0 -> Request ID 5 (ADC1) */
    DMAMUX1_Channel0->CCR = (5U << DMAMUX_CxCR_DMAREQ_ID_Pos);

    /* 3. Configure DMA1 Channel 1: Circular Ping-Pong with HTIE + TCIE + TEIE */
    DMA1_Channel1->CCR &= ~DMA_CCR_EN;
    while ((DMA1_Channel1->CCR & DMA_CCR_EN) != 0U) {}

    DMA1->IFCR = DMA_IFCR_CTCIF1 | DMA_IFCR_CHTIF1 | DMA_IFCR_CTEIF1;
    DMA1_Channel1->CPAR  = (uint32_t)&ADC1->DR;
    DMA1_Channel1->CMAR  = (uint32_t)adc_dma_buffer;
    DMA1_Channel1->CNDTR = FULL_BUF_LEN;
    DMA1_Channel1->CCR   = DMA_CCR_CIRC | DMA_CCR_MINC |
                           (1U << DMA_CCR_PSIZE_Pos)   | /* 16-bit peripheral */
                           (1U << DMA_CCR_MSIZE_Pos)   | /* 16-bit memory */
                           (2U << DMA_CCR_PL_Pos)      | /* High priority */
                           DMA_CCR_HTIE | DMA_CCR_TCIE | DMA_CCR_TEIE;

    NVIC_SetPriorityGrouping(0U);
    NVIC_SetPriority(DMA1_Channel1_IRQn, 3U);
    NVIC_ClearPendingIRQ(DMA1_Channel1_IRQn);
    NVIC_EnableIRQ(DMA1_Channel1_IRQn);

    DMA1_Channel1->CCR |= DMA_CCR_EN;

    /* 4. Ensure ADC1 disabled properly via ADDIS */
    if ((ADC1->CR & ADC_CR_ADEN) != 0U) {
        ADC1->CR |= ADC_CR_ADDIS;
        while ((ADC1->CR & ADC_CR_ADEN) != 0U) {}
    }

    /* 5. CRITICAL: CKMODE = 11b (HCLK/4 = 42.5 MHz @ 170 MHz / 40.0 MHz @ 160 MHz <= 60 MHz)
     *    + Enable BOTH internal VREFINT (VREFEN) and Die Temp Sensor (VSENSESEL) */
    ADC12_COMMON->CCR = (ADC12_COMMON->CCR & ~ADC_CCR_CKMODE) |
                        (3U << ADC_CCR_CKMODE_Pos)            |
                        ADC_CCR_VREFEN                        |
                        ADC_CCR_VSENSESEL;

    /* 6. Exit DEEPPWD, enable ADVREGEN, wait >= 120 us (covers 20 us VREG & 120 us Temp t_START!) */
    ADC1->CR &= ~ADC_CR_DEEPPWD;
    ADC1->CR |=  ADC_CR_ADVREGEN;
    delay_us_170mhz(150U);

    /* 7. Calibrate ADC1 (single-ended) */
    ADC1->CR &= ~ADC_CR_ADCALDIF;
    ADC1->CR |=  ADC_CR_ADCAL;
    while ((ADC1->CR & ADC_CR_ADCAL) != 0U) {}

    /* 8. Enable ADC1 using direct rc_w1 clear (NEVER |=) */
    ADC1->ISR = ADC_ISR_ADRDY;
    ADC1->CR |= ADC_CR_ADEN;
    while ((ADC1->ISR & ADC_ISR_ADRDY) == 0U) {}

    /* 9. Sampling times:
     *    Ch 18 (VREFINT): 640.5 cycles (15.07 us >= 4.0 us DS12288 minimum)
     *    Ch 16 (Temp):    640.5 cycles (15.07 us >= 5.0 us DS12288 minimum) */
    ADC1->SMPR2 = (ADC1->SMPR2 & ~(ADC_SMPR2_SMP16 | ADC_SMPR2_SMP18)) |
                  (7U << ADC_SMPR2_SMP16_Pos)                          |
                  (7U << ADC_SMPR2_SMP18_Pos);

    /* 10. Regular Sequence: L = 1 (2 conversions), SQ1 = Ch 18 (VREFINT), SQ2 = Ch 16 (Temp) */
    ADC1->SQR1 = (1U  << ADC_SQR1_L_Pos)   |
                 (18U << ADC_SQR1_SQ1_Pos) |
                 (16U << ADC_SQR1_SQ2_Pos);

    /* 11. Continuous scan + circular DMA requests */
    ADC1->CFGR = (ADC1->CFGR & ~(ADC_CFGR_RES | ADC_CFGR_ALIGN)) |
                 ADC_CFGR_CONT | ADC_CFGR_DMAEN | ADC_CFGR_DMACFG;

    /* 12. Start continuous ADC-to-DMA acquisition */
    ADC1->CR |= ADC_CR_ADSTART;
}

int main(void) {
    Clock_Init_170MHz();
    DMA_ADC1_Init();

    while (1) {
        if (g_half_ready != 0U) {
            g_half_ready = 0U;
            /* DMA is now filling [32..63]; CPU safely processes [0..31] */
            process_half_buffer(0U);
        }
        if (g_full_ready != 0U) {
            g_full_ready = 0U;
            /* DMA is now filling [0..31]; CPU safely processes [32..63] */
            process_half_buffer(HALF_BUF_LEN);
        }
        __WFI();
    }
}
```

---

## 7. Expected Output

`DMA1_Channel1_IRQHandler` alternates cleanly between Half-Transfer (`HTIF1`) and Transfer-Complete (`TCIF1`) interrupts with zero external wiring attached, keeping `g_blocks_done` incrementing continuously, `g_dma_errors == 0`, `g_vdda_mv` reporting the real NUCLEO-G474RE supply rail (`~3250..3350 mV`), and `g_temp_deci_c` reporting calibrated STM32G474RE die temperature (typically `240..450`, i.e., `24.0 °C–45.0 °C` at room ambient).

---

## 8. Debugging Procedure

1. Set breakpoints on `process_half_buffer(0U);` and `process_half_buffer(HALF_BUF_LEN);` to verify strict ping-pong alternation.
2. In GDB, inspect:
   ```gdb
   (gdb) p/x ADC12_COMMON->CCR   # CKMODE == 11b (0x30000), VREFEN (b22) & VSENSESEL (b23) == 1
   (gdb) p/x ADC1->SMPR2         # SMP16[20:18] == 111b and SMP18[26:24] == 111b (640.5 cycles)
   (gdb) p/x DMAMUX1_Channel0->CCR # Request ID == 5 (ADC1)
   (gdb) p g_vdda_mv             # ~3250..3350 mV
   (gdb) p g_temp_deci_c         # ~240..450 (24.0..45.0 C)
   (gdb) p g_blocks_done
   (gdb) p g_dma_errors          # MUST be 0
   ```

---

## 9. Common Errors & Troubleshooting

| Symptom | Root Cause | Verified Fix |
|---|---|---|
| Temperature or `VREFINT` reads garbage / wrong offset | `SMP16`/`SMP18` left at reset `2.5` cycles (`58 ns` vs `5 µs` min!) or skipped `120 µs` $t_{\text{START}}$ | Set `SMP16 = 7U`, `SMP18 = 7U` (`640.5` cycles = `15.07 µs`) and wait `>= 120 µs` after `VSENSESEL` |
| DMA never transfers a single sample | Forgot `RCC_AHB1ENR_DMAMUX1EN` or left `DMAMUX1_Channel0->CCR == 0` | Enable `DMAMUX1EN` and program Request ID `5` (`ADC1`) |
| CPU hangs in `__WFI()` and never processes samples | Forgot `DMA_CCR_HTIE \| DMA_CCR_TCIE` or `NVIC_EnableIRQ(DMA1_Channel1_IRQn)` | Enable both interrupts and implement `DMA1_Channel1_IRQHandler` |
| Works at `-O0` but freezes at `-O2` | `adc_dma_buffer` missing `volatile` qualifier | Declare `static volatile uint16_t adc_dma_buffer[FULL_BUF_LEN];` |
| DMA stops after first 64 samples | Set `ADC_CFGR_DMAEN` without `ADC_CFGR_DMACFG` (One-Shot mode instead of Circular) | Set both `ADC_CFGR_DMAEN \| ADC_CFGR_DMACFG` |

---

## 10. Verification Procedure

- [ ] Code compiles cleanly with `arm-none-eabi-gcc -Wall -Wextra -Werror`.
- [ ] `ADC12_COMMON->CCR` has `CKMODE = 11b` (`42.5 MHz` @ 170 MHz / `40.0 MHz` @ 160 MHz), `VREFEN = 1`, and `VSENSESEL = 1`.
- [ ] `ADC1->SMPR2` has `SMP16 = 111b` and `SMP18 = 111b` (`640.5` cycles = `15.07 µs >= 5 µs`).
- [ ] `DMA1_Channel1_IRQHandler` clears individual `CHTIF1`/`CTCIF1`/`CTEIF1` flags (never `CGIF1`) and executes `__DSB()`.
- [ ] `g_blocks_done` increments monotonically with `g_dma_errors == 0`, `g_vdda_mv` in `3150..3400 mV`, and `g_temp_deci_c` in realistic ambient range.

---

## 11. Extension Exercises

1. **Deterministic Timer-Triggered Sampling:** Replace `ADC_CFGR_CONT` with `TIM6_TRGO` hardware triggering (`EXTEN = 01b`, `EXTSEL = 13` for `TIM6_TRGO`) to lock the sampling rate to an exact `10.000 kHz` DSP cadence.
2. **Add External Potentiometer (`PA0` / `ADC1_IN1`) as 3rd Scan Channel:** Expand `NUM_CHANNELS` to `3`, set `SQR1.L = 2` (`SQ3 = 1`), configure `SMP1 = 47.5` cycles, and compute true $V_{\text{DDA}}$-compensated potentiometer voltage using `g_vdda_mv`.
3. **Stream Processed Stats via USART2:** Print `g_vdda_mv` and `g_temp_deci_c` every 500 ms over `USART2` ST-LINK VCP (Lab 08) without stalling the DMA pipeline.

---

## 12. Standardized Verification Record

| Field | Record |
|---|---|
| **Verification Status** | `CROSS-COMPILED` *(Self-Contained Onboard — Queued for Physical Execution)* |
| **Objective** | Continuously scan onboard internal `ADC1_IN18` (`VREFINT`) and `ADC1_IN16` (Die Temp Sensor) via `DMA1_Channel1` circular ping-pong (`DMAMUX1_Channel0` ID `5`), honoring DS12288 `f_ADC <= 60 MHz`, `640.5`-cycle sampling, and ES0430 §2.3.1 individual `IFCR` flag clearing. |
| **Firmware Version** | `v2.2.0-remediation` (branch `feature/complete-remediation`, base commit `585dc4fa`) |
| **Compiler Version** | `arm-none-eabi-gcc 14.3.1 20250623 (GNU Tools for STM32 14.3.rel1.20251027-0700)` |
| **Build Result** | **PASS** — `arm-none-eabi-gcc -std=c11 -mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard -DSTM32G474xx -Wall -Wextra -Werror -O2 -c` succeeded with **0 warnings, 0 errors** (`2,204 B` object file). |
| **Test Environment** | Windows 11 + ARM GNU Toolchain 14.3.rel1 + NUCLEO-G474RE (`MB1367` Rev C, `STM32G474RET6`, STLINK-V3E `V3J17M11`, `COM8`) |
| **Required Peripherals** | **Onboard Only:** `RCC`, `PWR`, `FLASH`, `ADC1` (`VREFINT` + internal Temp Sensor), `DMA1_Channel1`, `DMAMUX1_Channel0`, `NVIC`. Zero external wiring required. |
| **Hardware Availability** | `ONBOARD — READY FOR AGENT 4` |
| **Pin Configuration** | None required (`ADC1_IN18` and `ADC1_IN16` are internal silicon channels; optional `PA0` `ADC1_IN1` `MODER=11b` for Extension Exercise 2). |
| **Expected Behavior** | Continuous ping-pong `HTIF1`/`TCIF1` interrupts increment `g_blocks_done` with `g_dma_errors == 0`; `g_vdda_mv` settles around `~3280 mV` (`3250..3350 mV`) and `g_temp_deci_c` reports ambient + self-heating die temperature (`~240..450`). |
| **Actual Observations** | Pending Agent 4 Onboard Test *(not yet executed on physical silicon)*. |
| **Register Values** | Reset: `ADC1->CR = 0x20000000`, `ADC12_COMMON->CCR = 0`, `DMAMUX1_Channel0->CCR = 0`. Configured (`170 MHz` / `160 MHz`): `ADC12_COMMON->CCR = 0x00C30000` (`CKMODE=11b`, `VREFEN=1`, `VSENSESEL=1`), `ADC1->SMPR2`: `SMP16=7, SMP18=7`, `ADC1->SQR1`: `L=1, SQ1=18, SQ2=16`, `DMAMUX1_Channel0->CCR = 5`, `DMA1_Channel1->CNDTR = 64`. |
| **Debugging Procedure** | Flash via `STM32_Programmer_CLI -c port=SWD -w firmware.bin 0x08000000 -v -rst` or OpenOCD; in `arm-none-eabi-gdb` verify `p/x ADC12_COMMON->CCR`, `p/x DMAMUX1_Channel0->CCR`, `p g_vdda_mv`, `p g_temp_deci_c`, `p g_blocks_done`, `p g_dma_errors`. |
| **Remaining Limitations** | Uses continuous free-running scan (`ADC_CFGR_CONT`); exact fixed-rate DSP sampling requires `TIM6_TRGO` hardware trigger (Extension Exercise 1). Physical silicon calibration check pending Agent 4. |
