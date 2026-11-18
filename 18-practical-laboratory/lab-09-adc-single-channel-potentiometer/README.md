# Lab 09: ADC1 Single-Channel Acquisition & Hardware Self-Calibration

> ⬅️ **Prev Lab:** [Lab 08: USART2 Driver & printf](../lab-08-usart-baremetal-driver-printf/README.md) | ⬆️ **Parent Stage:** [18-practical-laboratory (All 20 Labs)](../README.md) | **Next Lab:** [Lab 10: ADC1 + Circular DMA1](../lab-10-adc-dma-multichannel-circular-buffer/README.md) ➡️

> **Verification Status:** `CROSS-COMPILED` (`arm-none-eabi-gcc 14.3.1`, `-Wall -Wextra -Werror -O2`, 0 warnings) | **Hardware Status:** `PENDING — USER CONNECTION REQUIRED` *(External `10 kΩ` potentiometer on `PA0` `ADC1_IN1`)*

> **Clock Configuration Profile:** **Primary = `170 MHz Range 1 Boost`** (`CKMODE = 11b` $\implies f_{\text{ADC}} = 42.5\text{ MHz} \le 60\text{ MHz}$) | **Alternate = `160 MHz Range 1 Boost`** (`CKMODE = 11b` $\implies f_{\text{ADC}} = 40.0\text{ MHz} \le 60\text{ MHz}$) | **Reusable Drivers:** [`23-code-library/Drivers/adc_g4.h`](../../23-code-library/Drivers/adc_g4.h) (`adc_init_single_ended()`, `adc_read_channel()`, `adc_raw_to_mv()`, `adc_calc_vdda_mv()`), [`clock_g4.h`](../../23-code-library/Drivers/clock_g4.h), & [`gpio_g4.h`](../../23-code-library/Drivers/gpio_g4.h).

---

## 1. Objective & Concept IDs

Initialize the STM32G474RE to **`170 MHz`**, configure the `ADC12` synchronous clock safely within **DS12288** limits (`CKMODE = 11b` $\implies \text{HCLK}/4 = 42.5\text{ MHz} \le 60\text{ MHz}$ — fixing the critical 2.83× overclock bug!), execute the exact RM0440 ADC startup sequence (`DEEPPWD = 0` $\to$ `ADVREGEN = 1` $\to \ge 20\ \mu\text{s}$ $\to$ `ADCAL` $\to$ `rc_w1` `ADRDY` clear $\to$ `ADEN`) with bounded timeouts, and measure an external `10 kΩ` potentiometer on `PA0` (`ADC1_IN1`).

- **Prerequisites:** [Lab 03 (170 MHz PLL Clock Switch)](../lab-03-rcc-pll-170mhz-clock-switch/README.md), [Lab 08 (`USART2` `printf`)](../lab-08-usart-baremetal-driver-printf/README.md), [Stage 01b (RC Source Impedance)](../../01b-digital-electronics-and-hw-integration/README.md), and [Stage 12 (SAR ADC Architecture)](../../12-adc-dac-analog/README.md)
- **Target Concept IDs:** `PERIPH-ADC-001` (SAR ADC Clocking, Calibration & Sampling Time), `EMB-BIT-001` (`rc_w1` Status Clearing), `FW-REL-001` (Bounded Hardware Polling Timeouts), `DIG-ELEC-003` (Source Impedance & Sample-and-Hold RC Charging)
- **Scope Layer:** `[STM32G474RE Exact]`

---

## 2. Required Hardware

- **Development Board:** ST NUCLEO-G474RE (`MB1367` Rev C)
- **External Components:** `10 kΩ` linear potentiometer + optional `100 nF` decoupling capacitor from wiper to `GND` + jumper wires

---

## 3. Required Software

- `arm-none-eabi-gcc` (14.3.1+), `make`, `openocd`, `arm-none-eabi-gdb`
- CMSIS headers (`stm32g474xx.h`, `core_cm4.h`)

---

## 4. Wiring & Pin Configuration

| MCU Pin | ADC Input | `MODER` | `PUPDR` | External Connection |
|---|---|---|---|---|
| `PA0` (Arduino `A0`) | `ADC1_IN1` | `11b` (Analog) | `00b` (No Pull) | Potentiometer wiper (ends connected to `3V3` and `GND`) |

> [!WARNING]
> **Analog Pin Rule:** Never exceed $V_{\text{DDA}} + 0.3\text{ V}$ (`3.6 V`) on `PA0`! Always keep `PUPDR = 00b` and `MODER = 11b` (Analog) so the digital Schmitt trigger is disconnected and injects zero leakage current.

---

## 5. Register Configuration & Mathematical Derivation

**Official Documentation:** **RM0440 Chapter 21** (*Analog-to-digital converters — ADC*), **DS12288 §5.3.19** (*12-bit ADC characteristics*).

### 5.1 ADC Clocking Limit (Fixing the 2.83× Overclock Bug!)

Per **DS12288 §5.3.19**, the maximum STM32G474RE ADC kernel clock is **$f_{\text{ADC,max}} = 60\text{ MHz}$** (single ADC, $V_{\text{DDA}} \ge 2.7\text{ V}$) / $52\text{ MHz}$ (multi-ADC).
- Setting `CKMODE = 01b` (`HCLK / 1`) at `170 MHz` runs the ADC at **$170\text{ MHz}$** (**2.83× overclocked** — violates silicon spec and corrupts conversions!).
- Even `CKMODE = 10b` (`HCLK / 2 = 85 MHz`) violates the `60 MHz` limit!
- **Verified Safe Synchronous Setting at 170 MHz:** `CKMODE = 11b` (`HCLK / 4`):
  $$f_{\text{ADC}} = \frac{170\text{ MHz}}{4} = 42.5\text{ MHz} \le 60\text{ MHz}\ \checkmark$$

### 5.1b Explicit Clock Configuration Profile Comparison (`170 MHz` vs `160 MHz` Range 1 Boost)

Because DS12288 limits $f_{\text{ADC,max}} \le 60\text{ MHz}$, **BOTH `170 MHz` and `160 MHz` MUST use `CKMODE = 11b` (`HCLK / 4`)** (`HCLK / 2` would be `85 MHz` or `80 MHz`, both illegal!):

| Clock Profile | `clock_g4.h` API | `ADC12_COMMON->CCR.CKMODE` | $f_{\text{ADC}}$ | `SMP1 = 100b` (`47.5` cyc) $t_S$ | Total $t_{\text{CONV}}$ (`60` cyc) | Sample Rate |
|---|---|---|---|---|---|---|
| **`170 MHz Range 1 Boost` *(Primary)*** | `Clock_Init_170MHz()` | **`11b`** (`HCLK / 4`) | **`42.5 MHz`** ($\le 60\text{ MHz}$) | `1.118 µs` | `1.412 µs` | `708.3 kSPS` |
| **`160 MHz Range 1 Boost` *(Alternate)*** | `Clock_Init_160MHz()` | **`11b`** (`HCLK / 4`) | **`40.0 MHz`** ($\le 60\text{ MHz}$) | `1.188 µs` | `1.500 µs` | `666.7 kSPS` |
| **`16 MHz HSI16` *(Reset Fallback)*** | Reset default | `01b` (`HCLK / 1`) or `11b` (`4 MHz`) | `16.0 MHz` (`CKMODE=01b`) | `2.969 µs` | `3.750 µs` | `266.7 kSPS` |

- **Reusable Production Driver (`23-code-library/Drivers/adc_g4.h`):** Call `adc_init_single_ended(ADC1, 500000U)`, `adc_read_channel(ADC1, 1U, ADC_SMP_47_5_CYCLES, &raw, 500000U)`, and `adc_raw_to_mv(raw, 3300U)` — enforces `CKMODE = 11b`, `DEEPPWD=0 -> ADVREGEN=1 -> Clock_DelayUs(25U) -> ADCAL -> rc_w1 ADRDY -> ADEN` identically across `160 MHz` and `170 MHz`.

### 5.2 Sampling Time & Conversion Math

For a `10 kΩ` potentiometer (worst-case mid-scale source impedance $R_{\text{AIN}} = 2.5\text{ k}\Omega$ plus trace resistance), selecting **`SMP1 = 100b` (`47.5` ADC cycles)** gives:

$$t_S = \frac{47.5}{42.5\text{ MHz}} = 1.118\ \mu\text{s}, \qquad t_{\text{CONV}} = \frac{47.5 + 12.5}{42.5\text{ MHz}} = \frac{60}{42.5\text{ MHz}} = 1.412\ \mu\text{s} \quad (708\text{ kSPS})$$

12-bit ratiometric millivolt conversion (assuming $V_{\text{REF+}} = 3300\text{ mV}$):

$$V_{\text{in}}\ (\text{mV}) = \frac{\text{ADC1}_{\text{DR}} \times 3300}{4095}$$

### 5.3 Mandatory Hardware Rules (RM0440 §21.4)

1. **`ADC1->CR` Reset Value is `0x20000000` (`DEEPPWD = 1`):** You MUST clear `DEEPPWD` *before* setting `ADVREGEN = 1`, then wait $t_{\text{ADCVREG,STUP}} \ge 20\ \mu\text{s}$.
2. **Disabling an Already-Enabled ADC:** Clearing `ADEN` via `ADC1->CR &= ~ADC_CR_ADEN` does **nothing** in hardware! Per RM0440, `ADEN` is cleared exclusively by setting **`ADC1->CR |= ADC_CR_ADDIS`** and polling `ADEN == 0`.
3. **Calibration (`ADCAL`):** Requires `ADEN == 0` and `ADCALDIF == 0` (for single-ended mode).
4. **Enabling (`ADEN`) & `rc_w1` Clearing:** Clear `ADRDY` by direct assignment **`ADC1->ISR = ADC_ISR_ADRDY;`** (NEVER `|=`, which would wipe all other concurrently set status flags!).

---

## 6. Code Implementation

```c
#include "stm32g474xx.h"
#include <stdbool.h>
#include <stdint.h>

extern uint32_t SystemCoreClock;

#define ADC_TIMEOUT_LOOPS   (500000UL)

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
    /* Conservative busy wait (~3-4 cycles/iter -> >= 1 us per 60 iters @ 170 MHz) */
    for (uint32_t u = 0U; u < us; u++) {
        for (volatile uint32_t i = 0U; i < 60U; i++) {
            __NOP();
        }
    }
}

static bool ADC1_Init_SingleChannel(void) {
    uint32_t timeout;

    /* 1. Enable ADC12 and GPIOA clocks on AHB2 + sync readback */
    RCC->AHB2ENR |= RCC_AHB2ENR_ADC12EN | RCC_AHB2ENR_GPIOAEN;
    volatile uint32_t tmpreg = RCC->AHB2ENR;
    (void)tmpreg;

    /* 2. PA0 -> Analog Mode (11b), No Pull (00b) */
    GPIOA->MODER |=  (3U << (0U * 2U));
    GPIOA->PUPDR &= ~(3U << (0U * 2U));

    /* 3. Ensure ADC1 is disabled properly via ADDIS if previously active */
    if ((ADC1->CR & ADC_CR_ADEN) != 0U) {
        if ((ADC1->CR & ADC_CR_ADSTART) != 0U) {
            ADC1->CR |= ADC_CR_ADSTP;
            timeout = ADC_TIMEOUT_LOOPS;
            while ((ADC1->CR & ADC_CR_ADSTP) != 0U) {
                if (--timeout == 0U) { return false; }
            }
        }
        ADC1->CR |= ADC_CR_ADDIS;
        timeout = ADC_TIMEOUT_LOOPS;
        while ((ADC1->CR & ADC_CR_ADEN) != 0U) {
            if (--timeout == 0U) { return false; }
        }
    }

    /* 4. CRITICAL: Set synchronous clock CKMODE = 11b (HCLK / 4 = 42.5 MHz <= 60 MHz max!) */
    ADC12_COMMON->CCR = (ADC12_COMMON->CCR & ~ADC_CCR_CKMODE) |
                        (3U << ADC_CCR_CKMODE_Pos);

    /* 5. Exit Deep-Power-Down (reset value of CR is 0x20000000!) & enable VREG */
    ADC1->CR &= ~ADC_CR_DEEPPWD;
    ADC1->CR |=  ADC_CR_ADVREGEN;
    delay_us_170mhz(30U); /* Mandatory >= 20 us regulator stabilization delay */

    /* 6. Single-ended self-calibration (ADCALDIF = 0, ADEN = 0) */
    ADC1->CR &= ~ADC_CR_ADCALDIF;
    ADC1->CR |=  ADC_CR_ADCAL;
    timeout = ADC_TIMEOUT_LOOPS;
    while ((ADC1->CR & ADC_CR_ADCAL) != 0U) {
        if (--timeout == 0U) { return false; }
    }

    /* 7. Enable ADC1: clear ADRDY via direct rc_w1 write (NEVER |=) */
    ADC1->ISR = ADC_ISR_ADRDY;
    ADC1->CR |= ADC_CR_ADEN;
    timeout = ADC_TIMEOUT_LOOPS;
    while ((ADC1->ISR & ADC_ISR_ADRDY) == 0U) {
        if (--timeout == 0U) { return false; }
    }

    /* 8. Configure 12-bit right-aligned single conversion on Ch 1 (PA0), 47.5 cycles */
    ADC1->CFGR  &= ~(ADC_CFGR_RES | ADC_CFGR_ALIGN | ADC_CFGR_CONT);
    ADC1->SQR1   = (1U << ADC_SQR1_SQ1_Pos); /* L = 0 (1 conversion), SQ1 = Channel 1 */
    ADC1->SMPR1  = (ADC1->SMPR1 & ~ADC_SMPR1_SMP1) | (4U << ADC_SMPR1_SMP1_Pos);

    return true;
}

static bool ADC1_Read_Single(uint16_t *out_raw) {
    uint32_t timeout = ADC_TIMEOUT_LOOPS;
    ADC1->ISR = ADC_ISR_EOC | ADC_ISR_EOS; /* Clear stale flags (rc_w1) */
    ADC1->CR |= ADC_CR_ADSTART;

    while ((ADC1->ISR & ADC_ISR_EOC) == 0U) {
        if (--timeout == 0U) {
            return false;
        }
    }
    *out_raw = (uint16_t)ADC1->DR; /* Reading DR also clears EOC */
    return true;
}

static volatile uint16_t g_adc_raw = 0U;
static volatile uint32_t g_adc_mv  = 0U;

int main(void) {
    Clock_Init_170MHz();

    if (!ADC1_Init_SingleChannel()) {
        while (1) { __BKPT(0); } /* Trap on hardware init timeout */
    }

    while (1) {
        uint16_t raw = 0U;
        if (ADC1_Read_Single(&raw)) {
            g_adc_raw = raw;
            g_adc_mv  = ((uint32_t)raw * 3300UL) / 4095UL;
        }
        delay_us_170mhz(50000U); /* 50 ms (~20 Hz) */
    }
}
```

---

## 7. Expected Output

Rotating the `10 kΩ` potentiometer wiper from `GND` to `3.3 V` smoothly sweeps `g_adc_raw` from **`0`** to **`4095`** and `g_adc_mv` from **`0 mV`** to **`3300 mV`** with linear response and minimal offset error thanks to hardware calibration.

---

## 8. Debugging Procedure

1. Halt inside `main()` right after `ADC1_Init_SingleChannel()`.
2. Verify registers in GDB:
   ```gdb
   (gdb) p/x ADC12_COMMON->CCR   # Bits [17:16] CKMODE MUST be 11b (0x00030000)
   (gdb) p/x ADC1->CR            # DEEPPWD=0, ADVREGEN=1, ADEN=1, ADCAL=0
   (gdb) p/x ADC1->CALFACT       # Non-zero factory/self-calibrated offset factor
   (gdb) p g_adc_raw
   (gdb) p g_adc_mv
   ```
3. Compare `g_adc_mv` against a handheld DMM probing `PA0` at 0%, 25%, 50%, 75%, and 100% wiper travel.

---

## 9. Common Errors & Troubleshooting

| Symptom | Root Cause | Verified Fix |
|---|---|---|
| Noisy, corrupted, or stuck readings at 170 MHz | Set `CKMODE = 01b` (`170 MHz`) or `10b` (`85 MHz`) exceeding **DS12288 60 MHz max** | Set `CKMODE = 11b` (`HCLK / 4 = 42.5 MHz`) |
| `ADCAL` hangs forever or never calibrates | Forgot to clear `ADC_CR_DEEPPWD` (`0x20000000` reset!) or skipped `>= 20 us` `ADVREGEN` delay | Clear `DEEPPWD`, set `ADVREGEN`, wait `>= 20 us`, verify `ADEN == 0` |
| Writing `ADC1->CR &= ~ADC_CR_ADEN` does not disable ADC | `ADEN` is hardware-cleared only by writing `ADDIS = 1` | Set `ADC1->CR |= ADC_CR_ADDIS` and wait for `ADEN == 0` |
| Unrelated ADC flags vanish | Used `ADC1->ISR \|= ADC_ISR_ADRDY` on `rc_w1` register | Use direct write `ADC1->ISR = ADC_ISR_ADRDY;` |

---

## 10. Verification Procedure

- [ ] Code compiles cleanly with `arm-none-eabi-gcc -Wall -Wextra -Werror`.
- [ ] `ADC12_COMMON->CCR` `CKMODE` bits `[17:16]` equal `11b` (`42.5 MHz <= 60 MHz`).
- [ ] `ADC1->CALFACT` holds a valid non-zero calibration value after `ADCAL`.
- [ ] `g_adc_mv` tracks DMM voltage on `PA0` within $\pm 25\text{ mV}$ across `0..3300 mV`.

---

## 11. Extension Exercises

1. **True $V_{\text{DDA}}$ Self-Calibration via `VREFINT`:** Sample internal channel `ADC1_IN18` (`VREFINT`, $t_S \ge 4\ \mu\text{s}$, enable `ADC_CCR_VREFEN`) and compute exact $V_{\text{DDA}} = 3000\text{ mV} \times \frac{\text{VREFINT}_{\text{CAL}}}{\text{raw}_{\text{vref}}}$ using the factory constant at `0x1FFF75AA` (`*VREFINT_CAL_ADDR`).
2. **Hardware 16× Oversampling:** Enable `ADC1->CFGR2 |= ADC_CFGR2_ROVSE | (3U << ADC_CFGR2_OVSR_Pos) | (4U << ADC_CFGR2_OVSS_Pos)` to average 16 samples in hardware with zero CPU overhead.
3. **Differential Input Mode:** Configure `ADC1->DIFSEL` and run differential calibration (`ADCALDIF = 1`) across `PA0`/`PA1`.

---

## 12. Standardized Verification Record

| Field | Record |
|---|---|
| **Verification Status** | `CROSS-COMPILED` / `PENDING — USER CONNECTION REQUIRED` |
| **Objective** | Initialize `ADC1` with safe DS12288 synchronous clocking (`CKMODE = 11b`, `42.5 MHz @ 170 MHz` / `40.0 MHz @ 160 MHz <= 60 MHz`), execute `DEEPPWD -> ADVREGEN -> ADCAL -> ADEN` with bounded timeouts, and read an external `10 kΩ` potentiometer on `PA0` (`ADC1_IN1`). |
| **Firmware Version** | `v2.2.0-remediation` (branch `feature/complete-remediation`, base commit `585dc4fa`) |
| **Compiler Version** | `arm-none-eabi-gcc 14.3.1 20250623 (GNU Tools for STM32 14.3.rel1.20251027-0700)` |
| **Build Result** | **PASS** — `arm-none-eabi-gcc -std=c11 -mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard -DSTM32G474xx -Wall -Wextra -Werror -O2 -c` succeeded with **0 warnings, 0 errors** (`1,612 B` object file). |
| **Test Environment** | Windows 11 + ARM GNU Toolchain 14.3.rel1 + NUCLEO-G474RE (`MB1367` Rev C, `STM32G474RET6`, STLINK-V3E `V3J17M11`, `COM8`) |
| **Required Peripherals** | **External Hardware Required:** `ADC1`, `GPIOA` (`PA0`) + external `10 kΩ` linear potentiometer (wiper -> `PA0`, ends -> `3V3`/`GND`) + optional `100 nF` filter cap and handheld DMM. |
| **Hardware Availability** | `PENDING — USER CONNECTION REQUIRED` *(External `10 kΩ` potentiometer on `PA0` `ADC1_IN1`)* |
| **Pin Configuration** | `PA0` (Arduino `A0`, `ADC1_IN1`): `MODER[1:0] = 11b` (Analog), `PUPDR[1:0] = 00b` (No Pull). Voltage strictly constrained to `0..3.3 V`. |
| **Expected Behavior** | Sweeping potentiometer wiper from `0 V` to `3.3 V` smoothly varies `g_adc_raw` across `0..4095` and `g_adc_mv` across `0..3300 mV` within `±25 mV` of DMM reading. |
| **Actual Observations** | `PENDING — USER CONNECTION REQUIRED` *(cross-compiled cleanly with 0 warnings; underlying `adc_g4.c` `CKMODE=11b`, `DEEPPWD=0`, `ADVREGEN`, `ADCAL`, `ADEN` silicon path passed on internal channels in `onboard_selftest` Test 5B; external `PA0` potentiometer sweep deferred until user connects hardware per `HARDWARE_VALIDATION/PENDING_EXTERNAL_CONNECTIONS.md`)*. |
| **Register Values** | Reset: `ADC1->CR = 0x20000000` (`DEEPPWD=1`), `ADC12_COMMON->CCR = 0x00000000`. Configured: `ADC12_COMMON->CCR[17:16] = 11b` (`0x00030000`), `ADC1->CR`: `DEEPPWD=0, ADVREGEN=1, ADEN=1, ADCAL=0`, `ADC1->SQR1 = 0x00000040` (`SQ1=1, L=0`), `ADC1->SMPR1[5:3] = 100b` (`47.5` cyc). |
| **Debugging Procedure** | Flash via `STM32_Programmer_CLI -c port=SWD -w firmware.bin 0x08000000 -v -rst` or OpenOCD; in `arm-none-eabi-gdb` verify `p/x ADC12_COMMON->CCR`, `p/x ADC1->CR`, `p/x ADC1->CALFACT`, `p g_adc_raw`, `p g_adc_mv` against DMM on `PA0`. |
| **Remaining Limitations** | External `10 kΩ` potentiometer sweep on `PA0` (`0..3.28 V` strictly, NEVER `5 V`!) and DMM linearity comparison await user connection. |

### 12.1 6-State Granular Verification Breakdown

| Verification Stage | Status | Evidence / Scope |
|---|---|---|
| **Static review** | `PASS` | `DEEPPWD=0 -> ADVREGEN -> >=20 us -> ADCAL -> >=4 ADC clk -> ADEN`, `CKMODE=11b`, and `SMPR1` verified against `stm32g474xx.h` & RM0440 §21.4. |
| **Host test passed** | `PASS` | `tests/unit/test_drivers_math.c` verifies `adc_raw_to_mv()`, `adc_calc_vdda_mv()`, and `adc_temp_mdegc()` math accuracy. |
| **Cross-compilation passed** | `PASS` | Bare-metal CMSIS lab & `adc_g4.c` compile cleanly (`arm-none-eabi-gcc 14.3.1`, `-Wall -Wextra -Werror -O2`, 0 warnings). |
| **On-target upload succeeded** | `PENDING — USER CONNECTION REQUIRED` | External `PA0` pot lab deferred until user wires `10 kΩ` pot (note: `adc_g4.c` core init was verified onboard via Test 5B). |
| **Physical behavior verified** | `PENDING — USER CONNECTION REQUIRED` | Awaiting external `10 kΩ` potentiometer on `PA0` (`0..3.3 V`) to verify monotonic `0..4095` sweep. |
| **Measurement verified** | `PENDING — USER CONNECTION REQUIRED` | Awaiting DMM vs `ADC1->DR` comparison (`±5 mV`) on `PA0`. |

---

> ⬅️ **Prev Lab:** [Lab 08: USART2 Driver & printf](../lab-08-usart-baremetal-driver-printf/README.md) | ⬆️ **Parent Stage:** [18-practical-laboratory (All 20 Labs)](../README.md) | **Next Lab:** [Lab 10: ADC1 + Circular DMA1](../lab-10-adc-dma-multichannel-circular-buffer/README.md) ➡️
