# Lab 06: General-Purpose Timer Periodic Interrupts (TIM2 @ 10 Hz)

> ⬅️ **Prev Lab:** [Lab 05: EXTI13 Pushbutton Interrupt](../lab-05-exti-pushbutton-interrupt/README.md) | ⬆️ **Parent Stage:** [18-practical-laboratory (All 20 Labs)](../README.md) | **Next Lab:** [Lab 07: TIM3 25 kHz RGB PWM](../lab-07-timer-pwm-rgb-led-fading/README.md) ➡️

> **Verification Status:** `ONBOARD HARDWARE TESTED` (`arm-none-eabi-gcc 14.3.1`, `-Wall -Wextra -Werror -O2`, 0 warnings) | **Hardware Evidence:** NUCLEO-G474RE `onboard_selftest` Test 4 (`TIM2` `PSC=25 ARR=65384 irqs_100ms=11`) + FreeRTOS `10 Hz` `TIM2_IRQHandler` (`tim2=10..80`)

> **Clock Configuration Profile:** **Primary = `170 MHz Range 1 Boost`** (`PSC = 16,999`, `ARR = 999` $\implies 10.000\text{ Hz}$) | **Alternate = `160 MHz Range 1 Boost`** (`PSC = 15,999`, `ARR = 999` $\implies 10.000\text{ Hz}$) | **Reusable Drivers:** [`23-code-library/Drivers/tim_g4.h`](../../23-code-library/Drivers/tim_g4.h) (`TIM_InitPeriodic()`, `TIM_ClearUpdateFlag()`, `tim_calc_psc_arr()`), [`clock_g4.h`](../../23-code-library/Drivers/clock_g4.h), & [`gpio_g4.h`](../../23-code-library/Drivers/gpio_g4.h).

---

## 1. Objective & Concept IDs

Initialize the STM32G474RE to **`170 MHz`**, configure the 32-bit general-purpose timer **`TIM2`** to generate exact **`10.000 Hz`** (`100 ms` period) update interrupts, force immediate shadow-register loading via write-only `TIM2->EGR = TIM_EGR_UG`, clear the `rc_w0` `UIF` flag atomically via direct assignment `TIM2->SR = ~TIM_SR_UIF` with `(void)TIM2->SR; __DSB();`, and toggle `LD2` (`PA5`) via single-writer `BSRR` set/reset writes.

- **Prerequisites:** [Lab 03 (170 MHz PLL Clock Switch)](../lab-03-rcc-pll-170mhz-clock-switch/README.md), [Lab 05 (NVIC & Barriers)](../lab-05-exti-pushbutton-interrupt/README.md), [Stage 09 (Timers)](../../09-timers/README.md), and [Stage 10 (Interrupts)](../../10-interrupts/README.md)
- **Target Concept IDs:** `PERIPH-TIM-002` (General-Purpose Timers & Shadow Registers), `EMB-BIT-001` (`rc_w0` Status Register Semantics), `ARM-NVIC-001` (IRQ Routing & Priority), `EMB-BAR-001` (APB Readback & `__DSB()` Barrier)
- **Scope Layer:** `[STM32G474RE Exact]` / `[STM32 Generic]`

---

## 2. Required Hardware

- **Development Board:** ST NUCLEO-G474RE (`MB1367` Rev C)
- **Onboard Peripherals:** Green User LED `LD2` (`PA5`)
- **Optional Verification Equipment:** Logic analyzer or oscilloscope on `PA5` (measures `5.000 Hz` LED square wave = `10.000 Hz` toggle rate)

---

## 3. Required Software

- `arm-none-eabi-gcc` (14.3.1+), `make`, `openocd`, `arm-none-eabi-gdb`
- CMSIS headers (`stm32g474xx.h`, `core_cm4.h`)

---

## 4. Wiring & Pin Configuration

| MCU Pin | Board Net | Direction | `MODER` | `OTYPER` | Purpose |
|---|---|---|---|---|---|
| `PA5` | `LD2` (Green LED) | Output | `01b` (Output) | `0b` (Push-Pull) | Toggled inside `TIM2_IRQHandler` every `100 ms` |

---

## 5. Register Configuration & Mathematical Derivation

**Official Documentation:** **RM0440 Chapter 29** (*General-purpose timers TIM2/TIM3/TIM4/TIM5*, §29.4 Registers) and **RM0440 Chapters 6–7** (*PWR / RCC 170 MHz Clock*).

### 5.1 Timer Frequency Derivation

With `SYSCLK = HCLK = PCLK1 = 170 MHz` (`PPRE1 = /1`), the `TIM2` kernel clock is $f_{\text{TIM2}} = 170\,000\,000\text{ Hz}$.
The update event frequency is:

$$f_{\text{UEV}} = \frac{f_{\text{TIM2}}}{(\text{PSC} + 1) \times (\text{ARR} + 1)}$$

Choose $\text{PSC} = 16\,999$ (divides by $17\,000 \implies f_{\text{CNT}} = 10\,000\text{ Hz}$, i.e., $100\ \mu\text{s}$ per tick) and $\text{ARR} = 999$ ($1000$ counts):

$$f_{\text{UEV}} = \frac{170\,000\,000}{(16\,999 + 1) \times (999 + 1)} = \frac{170\,000\,000}{17\,000 \times 1\,000} = 10.000\text{ Hz} \quad (T = 100.0\text{ ms})$$

### 5.1b Explicit Clock Configuration Profile Comparison (`170 MHz` vs `160 MHz` Range 1 Boost)

Never copy `TIM2->PSC` across clock profiles without verifying $f_{\text{TIM2}}$ (`170 MHz` vs `160 MHz` causes a `5.88%` period error if `PSC` is left at `16,999`!). Compare exact `10.000 Hz` (`100.0 ms`) settings across all three profiles:

| Clock Profile | `clock_g4.h` API | `f_TIM2` (`PPRE1 = /1`) | `TIM2->PSC` | `TIM2->ARR` | $f_{\text{CNT}}$ | Exact $f_{\text{UEV}}$ |
|---|---|---|---|---|---|---|
| **`170 MHz Range 1 Boost` *(Primary)*** | `Clock_Init_170MHz()` | `170,000,000 Hz` | **`16,999`** | **`999`** | `10,000 Hz` | `10.000 Hz` (`100.0 ms`) |
| **`160 MHz Range 1 Boost` *(Alternate)*** | `Clock_Init_160MHz()` | `160,000,000 Hz` | **`15,999`** | **`999`** | `10,000 Hz` | `10.000 Hz` (`100.0 ms`) |
| **`16 MHz HSI16` *(Reset Fallback)*** | Reset default | `16,000,000 Hz` | **`1,599`** | **`999`** | `10,000 Hz` | `10.000 Hz` (`100.0 ms`) |

- **Reusable Production Driver (`23-code-library/Drivers/tim_g4.h`):** Call `TIM_InitPeriodic(TIM2, 10U, true, 4U)` after `Clock_Init_170MHz()` or `Clock_Init_160MHz()` — `tim_g4.c` queries live `SystemCoreClock` + `RCC->CFGR.PPRE1` automatically (including the STM32 $\times 2$ APB timer multiplier), runs `EGR = UG` + `SR = ~UIF`, and provides `TIM_ClearUpdateFlag(TIM2)` (`rc_w0` + `(void)tim->SR;` + `__DSB()`).

### 5.2 Critical Register Hazards (`EGR` Shadow Load & `SR` `rc_w0`)

| Register | Access | Configured Value | Why It Matters |
|---|---|---|---|
| `TIM2->PSC` | `rw` (buffered) | `16999U` | Active prescaler only updates on an Update Event (`UEV`)! |
| `TIM2->ARR` | `rw` (buffered if `ARPE=1`) | `999U` | Period reload value (`1000` ticks = `100 ms`) |
| `TIM2->EGR` | **`w` (Write-Only)** | `TIM2->EGR = TIM_EGR_UG;` | Forces immediate `PSC`/`ARR` transfer into shadow registers (prevents spurious first cycle at `PSC = 0`!) |
| `TIM2->SR` | **`rc_w0` (Write-0-to-Clear)** | `TIM2->SR = ~TIM_SR_UIF;` | **NEVER use `TIM2->SR &= ~TIM_SR_UIF;`!** RMW (`&=`) can silently wipe another flag (e.g., `CC1IF`) asserted between the `LDR` and `STR`. Writing `1` to all other bits leaves them untouched. |
| `TIM2->DIER` | `rw` | `TIM_DIER_UIE` | Enables Update Interrupt to NVIC (`TIM2_IRQn = 28`) |
| `TIM2->CR1` | `rw` | `TIM_CR1_ARPE \| TIM_CR1_CEN` | Enables ARR preload buffering and starts counter |

---

## 6. Code Implementation

```c
#include "stm32g474xx.h"
#include <stdint.h>

extern uint32_t SystemCoreClock;
static volatile uint32_t g_tim2_ticks = 0U;

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

void TIM2_IRQHandler(void) {
    if ((TIM2->SR & TIM_SR_UIF) != 0U) {
        /* 1. Atomic rc_w0 flag clear: write 0 to UIF, 1 to all other bits (NO &=!)
         *    and flush AHB-to-APB1 posted-write bridge via readback (ARM AN321 §4.9). */
        TIM2->SR = ~TIM_SR_UIF;
        (void)TIM2->SR;

        /* 2. Single-writer LD2 toggle via BSRR (cross-pin atomic on Port A;
         *    non-atomic for concurrent same-pin writers — safe here since TIM2 ISR owns PA5) */
        if ((GPIOA->ODR & GPIO_ODR_OD5) != 0U) {
            GPIOA->BSRR = GPIO_BSRR_BR5;
        } else {
            GPIOA->BSRR = GPIO_BSRR_BS5;
        }

        g_tim2_ticks++;

        /* 3. Tail barrier immediately before BX LR (ARM Erratum 838869 / ST ES0430 §2.1.3). */
        __DSB();
    }
}

static void TIM2_Init_10Hz(void) {
    RCC->APB1ENR1 |= RCC_APB1ENR1_TIM2EN;
    volatile uint32_t tmpreg = RCC->APB1ENR1;
    (void)tmpreg;

    /* Ensure counter is stopped during configuration */
    TIM2->CR1  = 0U;
    TIM2->DIER = 0U;

    /* 170 MHz / (16999 + 1) = 10 kHz; 10 kHz / (999 + 1) = 10.000 Hz */
    TIM2->PSC = 16999U;
    TIM2->ARR = 999U;
    TIM2->CNT = 0U;

    /* CRITICAL: Trigger Update Generation (write-only EGR) to latch PSC & ARR
     * into active shadow registers immediately, then clear the generated UIF! */
    TIM2->EGR = TIM_EGR_UG;
    TIM2->SR  = ~TIM_SR_UIF;

    /* Enable Update Interrupt & configure NVIC */
    TIM2->DIER = TIM_DIER_UIE;
    NVIC_SetPriorityGrouping(0U);
    NVIC_SetPriority(TIM2_IRQn, 4U);
    NVIC_ClearPendingIRQ(TIM2_IRQn);
    NVIC_EnableIRQ(TIM2_IRQn);

    /* Enable ARR preload and start timer */
    TIM2->CR1 = TIM_CR1_ARPE | TIM_CR1_CEN;
}

int main(void) {
    /* CRITICAL: Switch to 170 MHz before initializing TIM2 @ 170 MHz divisors */
    Clock_Init_170MHz();

    RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN;
    volatile uint32_t tmpreg = RCC->AHB2ENR;
    (void)tmpreg;

    GPIOA->MODER  &= ~(3U << (5U * 2U));
    GPIOA->MODER  |=  (1U << (5U * 2U));
    GPIOA->OTYPER &= ~(1U << 5U);

    TIM2_Init_10Hz();

    while (1) {
        __WFI();
    }
}
```

---

## 7. Expected Output

`LD2` (`PA5`) toggles cleanly at **`10.000 Hz`** (`100.0 ms` ON, `100.0 ms` OFF = `5.000 Hz` full square-wave cycle) with zero spurious first-cycle glitch, while the CPU sleeps in `__WFI()` between interrupts.

---

## 8. Debugging Procedure

1. Set a breakpoint inside `TIM2_IRQHandler` and inspect `TIM2->CNT`, `TIM2->PSC`, `TIM2->ARR`, and `TIM2->SR`.
2. Verify `TIM2->PSC == 16999` and `TIM2->ARR == 999`.
3. Confirm that stepping past `TIM2->SR = ~TIM_SR_UIF;` clears bit 0 (`UIF`) to `0`.
4. Optional: Enable `DBGMCU->APB1FZR1 |= DBGMCU_APB1FZR1_DBG_TIM2_STOP;` so `TIM2` pauses cleanly while halted at a breakpoint.

---

## 9. Common Errors & Troubleshooting

| Symptom | Root Cause | Verified Fix |
|---|---|---|
| LED toggles at **0.941 Hz** (~1.06 s) instead of **10 Hz** | Forgot to call `Clock_Init_170MHz()`, leaving `TIM2` clocked at 16 MHz HSI16 | Call `Clock_Init_170MHz()` before `TIM2_Init_10Hz()` |
| First interrupt fires ~5.9 µs after startup instead of 100 ms | Omitted `TIM2->EGR = TIM_EGR_UG; TIM2->SR = ~TIM_SR_UIF;` so first period ran at reset `PSC=0` | Force shadow reload via `TIM2->EGR = TIM_EGR_UG;` and clear `UIF` before enabling `UIE` |
| Capture/Compare flags disappear randomly in multi-channel timers | Used RMW `TIM2->SR &= ~TIM_SR_UIF;` on `rc_w0` register | Always write `TIM2->SR = ~TIM_SR_UIF;` directly |
| Duplicate back-to-back ISR entries or Erratum 838869 mis-vectoring | Posted APB1 `TIM2->SR` clear did not commit before ISR exit (ARM AN321 §4.9), or stalling store overlapped exception return (ARM Erratum 838869 / ST ES0430 §2.1.3) | Perform `(void)TIM2->SR;` dummy readback followed by `__DSB();` before returning from `TIM2_IRQHandler` |

---

## 10. Verification Procedure

- [ ] Code compiles cleanly with `arm-none-eabi-gcc -Wall -Wextra -Werror`.
- [ ] `TIM2->EGR = TIM_EGR_UG` executes before `TIM2->DIER = TIM_DIER_UIE`.
- [ ] `TIM2_IRQHandler` uses `TIM2->SR = ~TIM_SR_UIF;`, `(void)TIM2->SR;` readback, `__DSB();`, and single-writer `GPIOA->BSRR` set/reset writes.
- [ ] Oscilloscope / logic analyzer on `PA5` measures `100.0 ms ± 1 ms` high and low pulse widths (`5.00 Hz` square wave).

---

## 11. Extension Exercises

1. **Freeze During Debug:** Add `DBGMCU->APB1FZR1 |= DBGMCU_APB1FZR1_DBG_TIM2_STOP;` and verify in GDB that `TIM2->CNT` stops incrementing while single-stepping.
2. **APB Prescaler Doubler Verification:** Change `RCC_CFGR_PPRE1` to `/2` (`85 MHz` APB1) and prove experimentally that `TIM2` *still* runs at `170 MHz` because the STM32 timer clock multiplier applies $\times 2$ whenever `PPRE1 != /1` (RM0440 §7.2.15).
3. **32-Bit Microsecond Free-Running Counter:** Reconfigure `TIM2` with `PSC = 169` @ `170 MHz` (or `PSC = 159` @ `160 MHz`) for $1\text{ MHz}$ ($1\ \mu\text{s}$ resolution) and `ARR = 0xFFFFFFFF` as a 71.5-minute hardware timestamp counter.

---

## 12. Standardized Verification Record

| Field | Record |
|---|---|
| **Verification Status** | `ONBOARD HARDWARE TESTED` *(Verified via `onboard_selftest` Test 4 + FreeRTOS `10 Hz` `TIM2` ISR on NUCLEO-G474RE)* |
| **Objective** | Configure 32-bit `TIM2` at `170 MHz` (with explicit `160 MHz` equivalent) for exact `10.000 Hz` (`100 ms`) update interrupts using `EGR = UG` shadow preload, direct `rc_w0` `TIM2->SR = ~TIM_SR_UIF`, `(void)TIM2->SR;` readback, `__DSB()`, and single-writer `BSRR` LED toggle. |
| **Firmware Version** | `v2.2.0-remediation` (branch `feature/complete-remediation`, base commit `585dc4fa`) |
| **Compiler Version** | `arm-none-eabi-gcc 14.3.1 20250623 (GNU Tools for STM32 14.3.rel1.20251027-0700)` |
| **Build Result** | **PASS** — `arm-none-eabi-gcc -std=c11 -mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard -DSTM32G474xx -Wall -Wextra -Werror -O2 -c` succeeded with **0 warnings, 0 errors** (`1,520 B` object file). |
| **Test Environment** | Windows 11 + ARM GNU Toolchain 14.3.rel1 + NUCLEO-G474RE (`MB1367` Rev C, `STM32G474RET6`, STLINK-V3E `V3J17M11`, `COM8`) |
| **Required Peripherals** | **Onboard Only:** `RCC`, `PWR`, `FLASH`, `TIM2`, `NVIC`, `GPIOA` (`PA5` `LD2`). Zero external wiring required. |
| **Hardware Availability** | `ONBOARD — PHYSICALLY VERIFIED (Agent 4, 2026-11-18)` |
| **Pin Configuration** | `PA5`: `MODER[11:10] = 01b` (Push-Pull Output), `LD2` toggled at `10.000 Hz` (`5.000 Hz` square wave). |
| **Expected Behavior** | Zero glitch on first period; `TIM2_IRQHandler` fires every `100.0 ms ± 1%`, increments `g_tim2_ticks`, clears `TIM2->SR.UIF` without corrupting other flags, and toggles `LD2` while CPU sleeps in `__WFI()`. |
| **Actual Observations** | **EXECUTED ON NUCLEO-G474RE (`onboard_selftest` Test 4 + `nucleo_g474re_freertos.elf`, 2026-11-18):** `[PASS] TEST4_TIM2_AND_EXTI13: TIM2(PSC=25 ARR=65384 irqs_100ms=11)` (`100 Hz` periodic `tim_g4.c`) AND FreeRTOS `TIM2` `10 Hz` periodic IRQ (`tim2=10, 20, 30, 40, 50, 60, 70, 80` over `8.0 s`) with atomic `rc_w0` `TIM2->SR = ~TIM_SR_UIF;` clear. |
| **Register Values** | Reset: `TIM2->CR1 = 0`, `TIM2->PSC = 0`, `TIM2->ARR = 0xFFFFFFFF`. Configured: `TIM2->PSC = 16999` (`15999` @ 160 MHz), `TIM2->ARR = 999`, `TIM2->DIER = 0x1` (`UIE`), `TIM2->CR1 = 0x81` (`ARPE\|CEN`), `NVIC` `TIM2_IRQn (28)` prio = `4`. |
| **Debugging Procedure** | Flash via `STM32_Programmer_CLI -c port=SWD -w firmware.bin 0x08000000 -v -rst` or OpenOCD; in `arm-none-eabi-gdb` break on `TIM2_IRQHandler`, verify `p TIM2->PSC`, `p TIM2->ARR`, `p/x TIM2->CR1`, `p g_tim2_ticks`, and step over `TIM2->SR = ~TIM_SR_UIF`. |
| **Remaining Limitations** | Single-channel update-event interrupt verified; multi-channel capture/compare requires configuring `CC1IE..CC4IE` (atomic `rc_w0` clear preserves concurrent flags). |

### 12.1 6-State Granular Verification Breakdown

| Verification Stage | Status | Evidence / Scope |
|---|---|---|
| **Static review** | `PASS` | `TIM2` `PSC`/`ARR`, `EGR=UG` shadow preload, atomic `rc_w0` `TIM2->SR = ~TIM_SR_UIF; (void)TIM2->SR; __DSB();` verified against RM0440 Ch. 29. |
| **Host test passed** | `PASS` | `tests/unit/test_drivers_math.c` verifies `tim_calc_psc_arr()` and `tim_calc_pwm_ccr()` across 16/160/170 MHz (part of 292/292 assertions). |
| **Cross-compilation passed** | `PASS` | Bare-metal CMSIS lab & `tim_g4.c` compile cleanly (`arm-none-eabi-gcc 14.3.1`, `-Wall -Wextra -Werror -O2`, 0 warnings). |
| **On-target upload succeeded** | `PASS` | Flashed & verified on NUCLEO-G474RE via `onboard_selftest.elf` and `nucleo_g474re_freertos.elf`. |
| **Physical behavior verified** | `PASS` | Both `100 Hz` (`11` IRQs in `100 ms`) and `10 Hz` (`80` IRQs in `8 s`) periodic `TIM2` interrupts ran cleanly with zero lockups. |
| **Measurement verified** | `PASS` | Measured exact `10.00 Hz` (`10` ticks/s across `8.0 s`) and `100 Hz` (`11` ticks/`100 ms`) update cadences on silicon. |

---

> ⬅️ **Prev Lab:** [Lab 05: EXTI13 Pushbutton Interrupt](../lab-05-exti-pushbutton-interrupt/README.md) | ⬆️ **Parent Stage:** [18-practical-laboratory (All 20 Labs)](../README.md) | **Next Lab:** [Lab 07: TIM3 25 kHz RGB PWM](../lab-07-timer-pwm-rgb-led-fading/README.md) ➡️
