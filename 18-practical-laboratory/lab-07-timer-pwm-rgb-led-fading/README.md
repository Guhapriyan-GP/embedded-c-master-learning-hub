# Lab 07: Hardware PWM Output & RGB LED Fading (TIM3 @ 25 kHz)

> **Verification Status:** Compile-Verified (`arm-none-eabi-gcc 14.3.1`) / Statically Reviewed (Not Tested on Physical NUCLEO-G474RE Hardware)

---

## 1. Objective & Concept IDs

Initialize the STM32G474RE to **`170 MHz`**, configure **`TIM3`** Channels 1, 2, and 3 in **PWM Mode 1** with shadow preloading (`OCxPE`, `ARPE`) and `TIM3->EGR = TIM_EGR_UG` initialization to generate flicker-free **`25.000 kHz`** PWM for smooth RGB (or single-channel) LED fading — while eliminating the classic **unsigned integer underflow infinite-loop bug** (`uint32_t duty >= 0` / `duty -= 50`).

- **Target Concept IDs:** `PERIPH-TIM-002` (Hardware PWM Mode 1 & Shadow Preload), `C-TYPE-002` (Signed vs Unsigned Underflow Bugs), `PERIPH-GPIO-001` (Alternate Function Multiplexing `AFR`), `STM32-RCC-001` (170 MHz Clock Sync)
- **Scope Layer:** `[STM32G474RE Exact]` / `[General C]`

---

## 2. Required Hardware

- **Development Board:** ST NUCLEO-G474RE (`MB1367` Rev C)
- **External Components:** Common-cathode RGB LED (or 1–3 discrete LEDs) + three `330 Ω` current-limiting resistors + breadboard & jumper wires
- **Optional Verification Equipment:** Oscilloscope on `PA6` (`TIM3_CH1`) to confirm exact `25.00 kHz` carrier frequency and smooth `0%`–`100%` duty sweep

---

## 3. Required Software

- `arm-none-eabi-gcc` (14.3.1+), `make`, `openocd`, `arm-none-eabi-gdb`
- CMSIS headers (`stm32g474xx.h`, `core_cm4.h`)

---

## 4. Wiring & Pin Configuration

Per **DS12288 Table 13** (*Alternate function mapping*):

| MCU Pin | Timer Channel | `AFR` Value | Breadboard Wiring |
|---|---|---|---|
| `PA6` (Arduino `D12`) | `TIM3_CH1` (Red) | `AF2` (`AFR[0]` `[27:24]`) | `PA6 -> 330 Ω -> Red Anode -> Cathode -> GND` |
| `PA7` (Arduino `D11`) | `TIM3_CH2` (Green) | `AF2` (`AFR[0]` `[31:28]`) | `PA7 -> 330 Ω -> Green Anode -> Cathode -> GND` |
| `PB0` (Arduino `A3`) | `TIM3_CH3` (Blue) | `AF2` (`AFR[0]` `[3:0]`) | `PB0 -> 330 Ω -> Blue Anode -> Cathode -> GND` |

*(If you only have a single external LED, connect it to `PA6` — Channels 2 and 3 operate independently without interfering with Channel 1.)*

---

## 5. Register Configuration & Mathematical Derivation

**Official Documentation:** **RM0440 Chapter 29** (*General-purpose timers TIM2–TIM5*, §29.3.9 PWM Mode) and **DS12288 Table 13**.

### 5.1 PWM Carrier Frequency & Duty Cycle Math

With `f_TIM3 = 170 MHz` and `PSC = 0` (maximum PWM resolution):

$$\text{ARR} = \frac{f_{\text{TIM3}}}{f_{\text{PWM}}} - 1 = \frac{170\,000\,000\text{ Hz}}{25\,000\text{ Hz}} - 1 = 6\,800 - 1 = 6\,799$$

$$\text{Duty Cycle (\%)} = \frac{\text{CCR}x}{\text{ARR} + 1} \times 100\% = \frac{\text{CCR}x}{6\,800} \times 100\%$$

- `CCR1 = 0` $\implies 0.0\%$ duty (LED OFF).
- `CCR1 = 3400` $\implies 50.0\%$ duty.
- `CCR1 = 6800` (or `6799` for 99.99%) $\implies 100.0\%$ duty (LED full ON).

### 5.2 Critical Bug Analysis: Unsigned Integer Underflow Trap

Consider the broken naive decrement loop:
```c
/* BUGGY CODE — DO NOT USE! */
for (uint32_t duty = 6799; duty > 0; duty -= 50) { ... }
```
Because $6799 \bmod 50 = 49$, `duty` reaches `49`, then `duty -= 50` **wraps around modulo $2^{32}$** to `0xFFFFFFFF` (`4,294,967,295`)! The condition `duty > 0` remains `true` for **85.9 million iterations (~119 hours at 5 ms/step)** while saturating `TIM3->CCR1`!  
**Verified Fix:** Use signed `int32_t duty` with `duty >= 0` (and step cleanly down to `0`).

---

## 6. Code Implementation

```c
#include "stm32g474xx.h"
#include <stdint.h>

extern uint32_t SystemCoreClock;
static volatile uint32_t g_ms_ticks = 0U;

void SysTick_Handler(void) {
    g_ms_ticks++;
}

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

static void SysTick_Init_1ms(void) {
    SysTick->CTRL = 0U;
    SysTick->LOAD = (170000000UL / 1000UL) - 1UL;
    SysTick->VAL  = 0U;
    SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk |
                    SysTick_CTRL_TICKINT_Msk   |
                    SysTick_CTRL_ENABLE_Msk;
}

static void delay_ms(uint32_t ms) {
    uint32_t start = g_ms_ticks;
    while ((uint32_t)(g_ms_ticks - start) < ms) {
        __WFI();
    }
}

static void TIM3_PWM_RGB_Init_25kHz(void) {
    RCC->APB1ENR1 |= RCC_APB1ENR1_TIM3EN;
    RCC->AHB2ENR  |= RCC_AHB2ENR_GPIOAEN | RCC_AHB2ENR_GPIOBEN;
    volatile uint32_t tmpreg = RCC->AHB2ENR;
    (void)tmpreg;

    /* PA6 (TIM3_CH1) and PA7 (TIM3_CH2) -> AF2, High Speed */
    GPIOA->MODER   = (GPIOA->MODER   & ~((3U << 12U) | (3U << 14U))) | ((2U << 12U) | (2U << 14U));
    GPIOA->OSPEEDR = (GPIOA->OSPEEDR & ~((3U << 12U) | (3U << 14U))) | ((2U << 12U) | (2U << 14U));
    GPIOA->AFR[0]  = (GPIOA->AFR[0]  & ~((0xFU << 24U) | (0xFU << 28U))) | ((2U << 24U) | (2U << 28U));

    /* PB0 (TIM3_CH3) -> AF2, High Speed */
    GPIOB->MODER   = (GPIOB->MODER   & ~(3U << 0U))  | (2U << 0U);
    GPIOB->OSPEEDR = (GPIOB->OSPEEDR & ~(3U << 0U))  | (2U << 0U);
    GPIOB->AFR[0]  = (GPIOB->AFR[0]  & ~(0xFU << 0U)) | (2U << 0U);

    TIM3->CR1  = 0U;
    TIM3->PSC  = 0U;
    TIM3->ARR  = 6799U; /* 170 MHz / 6800 = 25.000 kHz */
    TIM3->CCR1 = 0U;
    TIM3->CCR2 = 0U;
    TIM3->CCR3 = 0U;

    /* Clear OCxM fields before configuring PWM Mode 1 (0110b = 6U) + Preload Enable (OCxPE) */
    TIM3->CCMR1 &= ~(TIM_CCMR1_OC1M | TIM_CCMR1_OC1PE | TIM_CCMR1_OC2M | TIM_CCMR1_OC2PE);
    TIM3->CCMR1 |=  (6U << TIM_CCMR1_OC1M_Pos) | TIM_CCMR1_OC1PE |
                    (6U << TIM_CCMR1_OC2M_Pos) | TIM_CCMR1_OC2PE;

    TIM3->CCMR2 &= ~(TIM_CCMR2_OC3M | TIM_CCMR2_OC3PE);
    TIM3->CCMR2 |=  (6U << TIM_CCMR2_OC3M_Pos) | TIM_CCMR2_OC3PE;

    /* Enable Channel 1, 2, 3 outputs (Active-HIGH polarity) */
    TIM3->CCER |= TIM_CCER_CC1E | TIM_CCER_CC2E | TIM_CCER_CC3E;

    /* Enable Auto-Reload Preload (ARPE), force shadow transfer via UG, then start */
    TIM3->CR1 |= TIM_CR1_ARPE;
    TIM3->EGR  = TIM_EGR_UG;
    TIM3->CR1 |= TIM_CR1_CEN;
}

int main(void) {
    Clock_Init_170MHz();
    SysTick_Init_1ms();
    TIM3_PWM_RGB_Init_25kHz();

    const int32_t max_duty = 6799;

    while (1) {
        /* Fade IN: 0 -> 6799 (signed int32_t prevents underflow!) */
        for (int32_t duty = 0; duty <= max_duty; duty += 50) {
            TIM3->CCR1 = (uint32_t)duty;               /* Red ramps up */
            TIM3->CCR2 = (uint32_t)(max_duty - duty);  /* Green ramps down */
            TIM3->CCR3 = (uint32_t)(duty / 2);         /* Blue half-intensity */
            delay_ms(5U);
        }

        /* Fade OUT: 6799 -> 0 (FIXED: signed int32_t terminates cleanly when duty < 0!) */
        for (int32_t duty = max_duty; duty >= 0; duty -= 50) {
            TIM3->CCR1 = (uint32_t)duty;
            TIM3->CCR2 = (uint32_t)(max_duty - duty);
            TIM3->CCR3 = (uint32_t)(duty / 2);
            delay_ms(5U);
        }
        TIM3->CCR1 = 0U;
    }
}
```

---

## 7. Expected Output

The RGB LED (or single LED on `PA6`) smoothly breathes from 0% to 100% brightness and back down to 0% continuously with zero visible flicker (25 kHz carrier is above audible and optical flicker bands) and zero loop hang at the bottom of the fade-out ramp.

---

## 8. Debugging Procedure

1. Probe `PA6` (`TIM3_CH1`) with an oscilloscope set to `10 us/div`.
2. Confirm period $T = 40.0\ \mu\text{s}$ ($f = 25.00\text{ kHz}$).
3. In GDB, inspect `TIM3->CCR1`, `TIM3->ARR`, `TIM3->CCMR1` (`OC1PE = 1`, `OC1M = 6`), and `TIM3->CR1` (`ARPE = 1`, `CEN = 1`).
4. Step across the end of the fade-out loop (`duty = 49` $\to$ `duty = -1`) and verify the loop exits cleanly instead of wrapping to `0xFFFFFFFF`.

---

## 9. Common Errors & Troubleshooting

| Symptom | Root Cause | Verified Fix |
|---|---|---|
| LED fades in once, fades out once, then sticks solid ON for hours | `uint32_t duty` underflowed (`49U - 50U = 0xFFFFFFFFU`) | Use signed `int32_t duty` with `duty >= 0` |
| Oscilloscope measures `2.35 kHz` instead of `25.00 kHz` | Forgot `Clock_Init_170MHz()` (16 MHz / 6800 = 2.353 kHz) | Call `Clock_Init_170MHz()` first |
| First PWM cycle glitches for `~385 us` (`ARR = 0xFFFF`) | Omitted `TIM3->EGR = TIM_EGR_UG;` before setting `CEN` | Always write `TIM3->EGR = TIM_EGR_UG;` before enabling `CEN` |
| Pin stays flat `0 V` even though `TIM3->CNT` is running | Forgot `TIM_CCER_CC1E` or wrote wrong `AFR[0]` nibble (`AF2` for `TIM3`) | Verify `CCER` bit 0 and `GPIOA->AFR[0]` bits `[27:24] == 2` |

---

## 10. Verification Procedure

- [ ] Code compiles cleanly with `arm-none-eabi-gcc -Wall -Wextra -Werror`.
- [ ] `TIM3->ARR == 6799`, `TIM3->CCMR1` has `OC1PE=1` & `OC1M=6`, and `TIM3->CR1` has `ARPE=1`.
- [ ] `TIM3->EGR = TIM_EGR_UG;` precedes `TIM3->CR1 |= TIM_CR1_CEN;`.
- [ ] Oscilloscope confirms `25.00 kHz ± 1%` carrier with continuous smooth fade-in and fade-out cycles.

---

## 11. Extension Exercises

1. **Perceptual Gamma Correction ($\gamma = 2.2$):** Human eye brightness perception is logarithmic. Replace linear `duty` steps with a 256-entry `const uint16_t gamma_lut[256]` stored in Flash so fading looks visually uniform at low brightness.
2. **Center-Aligned PWM:** Set `TIM3->CR1 |= TIM_CR1_CMS_0` (Center-Aligned Mode 1) and update `ARR = 3399` to maintain 25 kHz while reducing switching harmonics.
3. **Zero-CPU DMA Waveform:** Stream a breathing lookup table into `TIM3->CCR1` automatically using `TIM3_UP` DMAMUX request (`ID = 65`) with DMA1.
