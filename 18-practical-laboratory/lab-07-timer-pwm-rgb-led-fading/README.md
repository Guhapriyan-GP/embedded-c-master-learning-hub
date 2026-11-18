# Lab 07: Hardware PWM Output & RGB LED Fading (TIM3 @ 25 kHz)

> ⬅️ **Prev Lab:** [Lab 06: TIM2 Periodic Interrupt](../lab-06-general-purpose-timer-periodic-interrupt/README.md) | ⬆️ **Parent Stage:** [18-practical-laboratory (All 20 Labs)](../README.md) | **Next Lab:** [Lab 08: USART2 Driver & printf](../lab-08-usart-baremetal-driver-printf/README.md) ➡️

> **Verification Status:** `CROSS-COMPILED` (`arm-none-eabi-gcc 14.3.1`, `-Wall -Wextra -Werror -O2`, 0 warnings) | **Hardware Status:** `PENDING — USER CONNECTION REQUIRED` *(External RGB LED + 3 × `330 Ω` resistors on `PA6`/`PA7`/`PB0`)*

> **Clock Configuration Profile:** **Primary = `170 MHz Range 1 Boost`** (`PSC = 0`, `ARR = 6,799` $\implies 25.000\text{ kHz}$, `SysTick->LOAD = 169,999`) | **Alternate = `160 MHz Range 1 Boost`** (`PSC = 0`, `ARR = 6,399` $\implies 25.000\text{ kHz}$, `SysTick->LOAD = 159,999`) | **Reusable Drivers:** [`23-code-library/Drivers/tim_g4.h`](../../23-code-library/Drivers/tim_g4.h) (`TIM_InitPwm()`, `TIM_SetPwmDutyPermille()`, `tim_calc_pwm_ccr()`), [`clock_g4.h`](../../23-code-library/Drivers/clock_g4.h), & [`gpio_g4.h`](../../23-code-library/Drivers/gpio_g4.h).

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

### 5.1b Explicit Clock Configuration Profile Comparison (`170 MHz` vs `160 MHz` Range 1 Boost)

When switching between `170 MHz` and `160 MHz`, update **`TIM3->ARR`**, **`max_duty`**, and **`SysTick->LOAD`** together so both the `25.000 kHz` PWM carrier and the `5 ms` fade cadence stay exact:

| Clock Profile | `clock_g4.h` API | `f_TIM3` | `TIM3->PSC` | `TIM3->ARR` (`25.000 kHz`) | `50%` `CCR` | `SysTick->LOAD` (`1 ms`) |
|---|---|---|---|---|---|---|
| **`170 MHz Range 1 Boost` *(Primary)*** | `Clock_Init_170MHz()` | `170 MHz` | `0` | **`6,799`** (`max_duty = 6799`) | `3,400` | `169,999` (`0x2980F`) |
| **`160 MHz Range 1 Boost` *(Alternate)*** | `Clock_Init_160MHz()` | `160 MHz` | `0` | **`6,399`** (`max_duty = 6399`) | `3,200` | `159,999` (`0x270FF`) |
| **`16 MHz HSI16` *(Reset Fallback)*** | Reset default | `16 MHz` | `0` | **`639`** (`max_duty = 639`) | `320` | `15,999` (`0x03E7F`) |

- **Reusable Production Driver (`23-code-library/Drivers/tim_g4.h`):** Use `GPIO_ConfigAF(GPIOA, 6U, 2U, ...)` + `TIM_InitPwm(TIM3, 1U, 25000U, 0U)` and `TIM_SetPwmDutyPermille(TIM3, 1U, duty_permille)` — auto-computes `PSC`/`ARR`/`CCR` from live `SystemCoreClock` and clamps `100%` duty cleanly against 16-bit `0xFFFF` wraparound!

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

---

## 12. Standardized Verification Record

| Field | Record |
|---|---|
| **Verification Status** | `CROSS-COMPILED` / `PENDING — USER CONNECTION REQUIRED` |
| **Objective** | Generate glitch-free `25.000 kHz` 3-channel RGB PWM on `TIM3` (`PA6`/`PA7`/`PB0`, `AF2`) with `ARPE`/`OCxPE` shadow preloading, `EGR = UG`, and signed `int32_t` anti-underflow fading ramp. |
| **Firmware Version** | `v2.2.0-remediation` (branch `feature/complete-remediation`, base commit `585dc4fa`) |
| **Compiler Version** | `arm-none-eabi-gcc 14.3.1 20250623 (GNU Tools for STM32 14.3.rel1.20251027-0700)` |
| **Build Result** | **PASS** — `arm-none-eabi-gcc -std=c11 -mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard -DSTM32G474xx -Wall -Wextra -Werror -O2 -c` succeeded with **0 warnings, 0 errors** (`1,656 B` object file). |
| **Test Environment** | Windows 11 + ARM GNU Toolchain 14.3.rel1 + NUCLEO-G474RE (`MB1367` Rev C, `STM32G474RET6`, STLINK-V3E `V3J17M11`, `COM8`) |
| **Required Peripherals** | **External Hardware Required:** `TIM3` (`CH1`/`CH2`/`CH3`) + `GPIOA`/`GPIOB` + external common-cathode RGB LED (or 3 discrete LEDs) + three `330 Ω` resistors + breadboard/jumpers (+ optional oscilloscope). |
| **Hardware Availability** | `PENDING — USER CONNECTION REQUIRED` *(External RGB LED + 3 × `330 Ω` resistors on `PA6`/`PA7`/`PB0`)* |
| **Pin Configuration** | `PA6` (`D12`, `TIM3_CH1`, `AF2`), `PA7` (`D11`, `TIM3_CH2`, `AF2`), `PB0` (`A3`, `TIM3_CH3`, `AF2`): `MODER = 10b` (AF), `OSPEEDR = 10b` (High). |
| **Expected Behavior** | Continuous smooth RGB breathing with `25.00 kHz` (`40.0 µs` period) carrier on `PA6`/`PA7`/`PB0`, zero mid-cycle glitches, and clean exit at `duty = 0` without unsigned underflow hang. |
| **Actual Observations** | `PENDING — USER CONNECTION REQUIRED` *(cross-compiled cleanly with 0 warnings; `tim_calc_pwm_ccr()` verified on host; physical RGB LED & `25.00 kHz` oscilloscope confirmation deferred until user connects external components per `HARDWARE_VALIDATION/PENDING_EXTERNAL_CONNECTIONS.md`)*. |
| **Register Values** | Reset: `TIM3->CR1 = 0`, `TIM3->ARR = 0xFFFF`, `TIM3->CCER = 0`. Configured (`170 MHz` / `160 MHz`): `TIM3->PSC = 0`, `TIM3->ARR = 6799` (`6399` @ 160 MHz), `TIM3->CCMR1 = 0x6868` (`OC1M=6, OC1PE=1, OC2M=6, OC2PE=1`), `TIM3->CCMR2 = 0x0068` (`OC3M=6, OC3PE=1`), `TIM3->CCER = 0x0111`, `TIM3->CR1 = 0x0081` (`ARPE\|CEN`). |
| **Debugging Procedure** | Flash via `STM32_Programmer_CLI -c port=SWD -w firmware.bin 0x08000000 -v -rst` or OpenOCD; in `arm-none-eabi-gdb` verify `p TIM3->ARR`, `p/x TIM3->CCMR1`, `p/x TIM3->CCER`, `p TIM3->CCR1`; probe `PA6` at `10 us/div` on oscilloscope once wired. |
| **Remaining Limitations** | Requires external common-cathode RGB LED + three `330 Ω` resistors (and optional oscilloscope on `PA6`/`PA7`/`PB0`) for physical verification; linear duty sweep omits perceptual gamma correction (Extension Exercise 1). |

### 12.1 6-State Granular Verification Breakdown

| Verification Stage | Status | Evidence / Scope |
|---|---|---|
| **Static review** | `PASS` | `TIM3` `CH1..3` (`PA6`/`PA7`/`PB0` `AF2`), `ARPE`/`OCxPE`, `EGR=UG`, and signed `int32_t` anti-underflow ramp verified against `stm32g474xx.h` & RM0440. |
| **Host test passed** | `PASS` | `tests/unit/test_drivers_math.c` validates `tim_calc_psc_arr()` (`25 kHz` @ `160`/`170 MHz`) and `tim_calc_pwm_ccr()` edge cases (`0%`, `50%`, `100%`, `0xFFFF` guard). |
| **Cross-compilation passed** | `PASS` | Bare-metal CMSIS program compiles cleanly (`arm-none-eabi-gcc 14.3.1`, `-Wall -Wextra -Werror -O2`, 0 warnings / 0 errors). |
| **On-target upload succeeded** | `PENDING — USER CONNECTION REQUIRED` | Deferred until external RGB LED + `330 Ω` resistors are wired to `PA6`/`PA7`/`PB0` (see `HARDWARE_VALIDATION/PENDING_EXTERNAL_CONNECTIONS.md`). |
| **Physical behavior verified** | `PENDING — USER CONNECTION REQUIRED` | Awaiting external RGB LED connection for optical smooth-fade verification. |
| **Measurement verified** | `PENDING — USER CONNECTION REQUIRED` | Awaiting oscilloscope probe on `PA6`/`PA7`/`PB0` to verify `25.00 kHz` (`40.0 µs`) carrier and glitch-free duty transitions. |

---

> ⬅️ **Prev Lab:** [Lab 06: TIM2 Periodic Interrupt](../lab-06-general-purpose-timer-periodic-interrupt/README.md) | ⬆️ **Parent Stage:** [18-practical-laboratory (All 20 Labs)](../README.md) | **Next Lab:** [Lab 08: USART2 Driver & printf](../lab-08-usart-baremetal-driver-printf/README.md) ➡️
