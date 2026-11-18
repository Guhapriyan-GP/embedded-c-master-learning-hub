# Lab 10: Multichannel ADC1 + Circular Ping-Pong DMA & Internal Temperature Sensor

> **Verification Status:** Compile-Verified (`arm-none-eabi-gcc 14.3.1`) / Statically Reviewed (Not Tested on Physical NUCLEO-G474RE Hardware)

---

## 1. Objective & Concept IDs

Continuously scan two channels on **`ADC1`** — Channel 1 (`PA0` potentiometer) and Channel 16 (**Internal Die Temperature Sensor**) — into a `volatile` SRAM ping-pong circular buffer via **`DMA1 Channel 1`** and **`DMAMUX1 Channel 0`**, while enforcing **DS12288** silicon constraints (`CKMODE = 11b` $\implies 42.5\text{ MHz}$, `SMPR2.SMP16 = 640.5` cycles $= 15.07\ \mu\text{s} \ge 5\ \mu\text{s}$, and $120\ \mu\text{s}$ temperature sensor startup $t_{\text{START}}$) and processing each half-buffer deterministically via **`HTIE`/`TCIE`** interrupts.

- **Target Concept IDs:** `PERIPH-DMA-001` (Circular Ping-Pong DMA & DMAMUX1), `PERIPH-ADC-001` (Scan Mode & Internal Temp Sensor $t_S$), `EMB-VOL-001` (`volatile` DMA Buffers), `EMB-BAR-001` (`__DMB()`/`__DSB()` DMA Ownership Handoff)
- **Scope Layer:** `[STM32G474RE Exact]`

---

## 2. Required Hardware

- **Development Board:** ST NUCLEO-G474RE (`MB1367` Rev C)
- **External Components:** `10 kΩ` potentiometer wired to `PA0` (`ADC1_IN1`), `3V3`, and `GND` (Internal Temperature Sensor requires no external wiring)

---

## 3. Required Software

- `arm-none-eabi-gcc` (14.3.1+), `make`, `openocd`, `arm-none-eabi-gdb`
- CMSIS headers (`stm32g474xx.h`, `core_cm4.h`)

---

## 4. Wiring & Pin Configuration

| Channel | Signal | Physical Pin | `MODER` | Sampling Time Configured |
|---|---|---|---|---|
| `ADC1_IN1` | Potentiometer Wiper | `PA0` (Arduino `A0`) | `11b` (Analog) | `47.5` cycles (`1.12 µs` @ 42.5 MHz) |
| `ADC1_IN16` | Internal Die Temp Sensor | Internal (`VSENSESEL = 1`) | N/A | **`640.5` cycles (`15.07 µs >= 5 µs` DS12288 min!)** |

---

## 5. Register Configuration & Mathematical Derivation

**Official Documentation:** **RM0440 Chapter 12** (*DMA*), **RM0440 Chapter 13** (*DMAMUX*, Table 91 Request IDs), **RM0440 Chapter 21** (*ADC*), **DS12288 §5.3.24** (*Temperature sensor characteristics*), and **ST ES0430 §2.3.1** (*DMA `CGIFx` errata*).

### 5.1 Silicon Spec Compliance Checks

1. **ADC Clock:** `CKMODE = 11b` $\implies f_{\text{ADC}} = 170\text{ MHz} / 4 = 42.5\text{ MHz} \le 60\text{ MHz}\ \checkmark$.
2. **Temperature Sensor Sampling Time ($t_{S\_\text{temp}} \ge 5\ \mu\text{s}$ per DS12288):**
   - Reset `SMPR2.SMP16 = 000b` (`2.5` cycles = `58.8 ns`) violates the $5\ \mu\text{s}$ minimum by **85×**!
   - Setting **`SMPR2.SMP16 = 111b` (`640.5` cycles)** gives:
     $$t_{S,\text{Ch16}} = \frac{640.5}{42.5\text{ MHz}} = 15.07\ \mu\text{s} \ge 5.0\ \mu\text{s}\ \checkmark$$
3. **Temperature Sensor Startup Time ($t_{\text{START}} = 120\ \mu\text{s}$ max per DS12288):** After setting `ADC12_COMMON->CCR |= ADC_CCR_VSENSESEL;`, firmware MUST wait $\ge 120\ \mu\text{s}$ before starting conversions.
4. **Factory-Calibrated Temperature Formula (RM0440 §21.4.32, DS12288):**
   - `TS_CAL1` (`0x1FFF75A8`, 16-bit): measured at $30\ ^\circ\text{C}$, $V_{\text{DDA}} = 3.0\text{ V}$.
   - `TS_CAL2` (`0x1FFF75CA`, 16-bit): measured at $130\ ^\circ\text{C}$, $V_{\text{DDA}} = 3.0\text{ V}$.
   - Scaling raw 3.3 V reading to 3.0 V reference: $\text{TS\_DATA}_{3.0\text{V}} = \text{raw}_{16} \times \frac{3300}{3000}$, then:
     $$T\ (^\circ\text{C}) = 30 + \frac{100 \times (\text{TS\_DATA}_{3.0\text{V}} - \text{TS\_CAL1})}{\text{TS\_CAL2} - \text{TS\_CAL1}}$$

### 5.2 Ping-Pong Buffer Geometry & DMAMUX Routing

- `NUM_CHANNELS = 2`, `SAMPLES_PER_HALF = 16` $\implies$ Half buffer = `32` halfwords (`16` pairs `[PA0, Temp]`), Full buffer = `64` halfwords (`128` bytes).
- **`DMAMUX1_Channel0->CCR = 5U`** maps `ADC1` (Request ID `5`) to `DMA1_Channel1`!
- **`DMA1_Channel1->CCR`** enables `CIRC | MINC | PSIZE=16b | MSIZE=16b | HTIE | TCIE`.
- **ES0430 §2.3.1 Warning:** Inside `DMA1_Channel1_IRQHandler`, clear **individual** flags (`DMA_IFCR_CHTIF1`, `DMA_IFCR_CTCIF1`) — NEVER write `DMA_IFCR_CGIF1`, which can race with concurrent flags on the same channel!

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

/* Factory calibration addresses for STM32G474RE (DS12288) */
#define TS_CAL1_ADDR        ((const volatile uint16_t *)0x1FFF75A8UL) /* 30 C @ 3.0V  */
#define TS_CAL2_ADDR        ((const volatile uint16_t *)0x1FFF75CAUL) /* 130 C @ 3.0V */

/* CRITICAL: volatile prevents compiler from hoisting/caching DMA-written SRAM */
static volatile uint16_t adc_dma_buffer[FULL_BUF_LEN];

/* Ping-pong ownership flags set by DMA1_Channel1_IRQHandler */
static volatile uint32_t g_half_ready = 0U;
static volatile uint32_t g_full_ready = 0U;
static volatile uint32_t g_dma_errors = 0U;

/* Processed outputs inspectable in GDB */
static volatile uint32_t g_pot_mv        = 0U;
static volatile int32_t  g_temp_deci_c   = 0; /* Tenths of deg C (e.g., 265 = 26.5 C) */
static volatile uint32_t g_blocks_done   = 0U;

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
        DMA1->IFCR = DMA_IFCR_CHTIF1; /* Clear individual HT flag (never CGIF1!) */
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
    uint32_t sum_pot  = 0U;
    uint32_t sum_temp = 0U;

    __DMB(); /* Ensure DMA writes to SRAM are visible before CPU reads */
    for (uint32_t i = 0U; i < PAIRS_PER_HALF; i++) {
        sum_pot  += adc_dma_buffer[offset + (i * 2U) + 0U];
        sum_temp += adc_dma_buffer[offset + (i * 2U) + 1U];
    }

    uint32_t avg_pot  = sum_pot  / PAIRS_PER_HALF;
    uint32_t avg_temp = sum_temp / PAIRS_PER_HALF;

    g_pot_mv = (avg_pot * 3300UL) / 4095UL;

    /* Scale 3.3 V ADC reading to 3.0 V calibration reference */
    int32_t ts_cal1   = (int32_t)(*TS_CAL1_ADDR);
    int32_t ts_cal2   = (int32_t)(*TS_CAL2_ADDR);
    int32_t ts_scaled = (int32_t)((avg_temp * 3300UL) / 3000UL);
    if (ts_cal2 > ts_cal1) {
        g_temp_deci_c = 300 + ((1000 * (ts_scaled - ts_cal1)) / (ts_cal2 - ts_cal1));
    }

    g_blocks_done++;
}

static void DMA_ADC1_Init(void) {
    /* 1. Enable DMA1, DMAMUX1 (AHB1) and ADC12, GPIOA (AHB2) clocks */
    RCC->AHB1ENR |= RCC_AHB1ENR_DMA1EN | RCC_AHB1ENR_DMAMUX1EN;
    RCC->AHB2ENR |= RCC_AHB2ENR_ADC12EN | RCC_AHB2ENR_GPIOAEN;
    volatile uint32_t tmpreg = RCC->AHB2ENR;
    (void)tmpreg;

    /* PA0 -> Analog (11b), No Pull */
    GPIOA->MODER |=  (3U << 0U);
    GPIOA->PUPDR &= ~(3U << 0U);

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

    /* 5. CRITICAL: CKMODE = 11b (HCLK/4 = 42.5 MHz <= 60 MHz) + Enable Temp Sensor (VSENSESEL) */
    ADC12_COMMON->CCR = (ADC12_COMMON->CCR & ~ADC_CCR_CKMODE) |
                        (3U << ADC_CCR_CKMODE_Pos)            |
                        ADC_CCR_VSENSESEL;

    /* 6. Exit DEEPPWD, enable ADVREGEN, wait >= 120 us (covers both 20 us VREG & 120 us Temp t_START!) */
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
     *    Ch 1  (PA0):  47.5 cycles (1.12 us @ 42.5 MHz)
     *    Ch 16 (Temp): 640.5 cycles (15.07 us >= 5.0 us DS12288 minimum!) */
    ADC1->SMPR1 = (ADC1->SMPR1 & ~ADC_SMPR1_SMP1)  | (4U << ADC_SMPR1_SMP1_Pos);
    ADC1->SMPR2 = (ADC1->SMPR2 & ~ADC_SMPR2_SMP16) | (7U << ADC_SMPR2_SMP16_Pos);

    /* 10. Regular Sequence: L = 1 (2 conversions), SQ1 = Ch 1, SQ2 = Ch 16 */
    ADC1->SQR1 = (1U << ADC_SQR1_L_Pos)   |
                 (1U << ADC_SQR1_SQ1_Pos) |
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

`DMA1_Channel1_IRQHandler` alternates cleanly between Half-Transfer (`HTIF1`) and Transfer-Complete (`TCIF1`) interrupts, keeping `g_blocks_done` incrementing continuously, `g_dma_errors == 0`, `g_pot_mv` tracking `PA0` (`0..3300 mV`), and `g_temp_deci_c` reporting realistic STM32G474RE die temperature (typically `250..400`, i.e., `25.0 °C–40.0 °C` at room ambient).

---

## 8. Debugging Procedure

1. Set breakpoints on `process_half_buffer(0U);` and `process_half_buffer(HALF_BUF_LEN);` to verify strict ping-pong alternation.
2. In GDB, inspect:
   ```gdb
   (gdb) p/x ADC12_COMMON->CCR   # CKMODE == 11b (0x30000), VSENSESEL == 1 (0x800000)
   (gdb) p/x ADC1->SMPR2         # SMP16[20:18] == 111b (640.5 cycles)
   (gdb) p g_pot_mv
   (gdb) p g_temp_deci_c
   (gdb) p g_dma_errors          # MUST be 0
   ```

---

## 9. Common Errors & Troubleshooting

| Symptom | Root Cause | Verified Fix |
|---|---|---|
| Temperature reads garbage or freezes around wrong offset | `SMPR2.SMP16` left at reset `2.5` cycles (`58 ns` vs `5 µs` min!) or skipped `120 µs` $t_{\text{START}}$ | Set `SMP16 = 7U` (`640.5` cycles = `15.07 µs`) and wait `>= 120 µs` after `VSENSESEL` |
| CPU hangs in `__WFI()` and never processes samples | Forgot `DMA_CCR_HTIE \| DMA_CCR_TCIE` or `NVIC_EnableIRQ(DMA1_Channel1_IRQn)` | Enable both interrupts and implement `DMA1_Channel1_IRQHandler` |
| Works at `-O0` but freezes at `-O2` | `adc_dma_buffer` missing `volatile` qualifier | Declare `static volatile uint16_t adc_dma_buffer[FULL_BUF_LEN];` |
| DMA stops after first 64 samples | Set `ADC_CFGR_DMAEN` without `ADC_CFGR_DMACFG` (One-Shot mode instead of Circular) | Set both `ADC_CFGR_DMAEN \| ADC_CFGR_DMACFG` |

---

## 10. Verification Procedure

- [ ] Code compiles cleanly with `arm-none-eabi-gcc -Wall -Wextra -Werror`.
- [ ] `ADC12_COMMON->CCR` has `CKMODE = 11b` (`42.5 MHz`) and `VSENSESEL = 1`.
- [ ] `ADC1->SMPR2` has `SMP16 = 111b` (`640.5` cycles = `15.07 µs >= 5 µs`).
- [ ] `DMA1_Channel1_IRQHandler` clears individual `CHTIF1`/`CTCIF1` flags (never `CGIF1`) and executes `__DSB()`.
- [ ] `g_blocks_done` increments monotonically with `g_dma_errors == 0`.

---

## 11. Extension Exercises

1. **Deterministic Timer-Triggered Sampling:** Replace `ADC_CFGR_CONT` with `TIM6_TRGO` hardware triggering (`EXTEN = 01b`, `EXTSEL = 13` for `TIM6_TRGO`) to lock the sampling rate to an exact `10.000 kHz` DSP cadence.
2. **Add `VREFINT` (`Ch 18`):** Expand to a 3-channel scan (`PA0`, `Temp`, `VREFINT`) so temperature calculation dynamically compensates for actual $V_{\text{DDA}}$ instead of assuming `3.300 V`.
3. **Stream Processed Stats via USART2:** Print `g_pot_mv` and `g_temp_deci_c` every 500 ms over USART2 (Lab 08) without stalling the DMA pipeline.
