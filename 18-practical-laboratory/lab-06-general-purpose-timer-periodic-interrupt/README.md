# Lab 06: General-Purpose Timer Periodic Interrupts (TIM2 @ 10 Hz)

> **Verification Status:** Compile-Verified (`arm-none-eabi-gcc 14.3.1`) / Statically Reviewed (Not Tested on Physical NUCLEO-G474RE Hardware)

---

## 1. Objective & Concept IDs

Initialize the STM32G474RE to **`170 MHz`**, configure the 32-bit general-purpose timer **`TIM2`** to generate exact **`10.000 Hz`** (`100 ms` period) update interrupts, force immediate shadow-register loading via write-only `TIM2->EGR = TIM_EGR_UG`, clear the `rc_w0` `UIF` flag atomically via direct assignment `TIM2->SR = ~TIM_SR_UIF`, and toggle `LD2` (`PA5`) atomically with `BSRR` + `__DSB()`.

- **Target Concept IDs:** `PERIPH-TIM-002` (General-Purpose Timers & Shadow Registers), `EMB-BIT-001` (`rc_w0` Status Register Semantics), `ARM-NVIC-001` (IRQ Routing & Priority), `EMB-BAR-001` (`__DSB()` Barrier)
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
        /* 1. Atomic rc_w0 flag clear: write 0 to UIF, 1 to all other bits (NO &=!) */
        TIM2->SR = ~TIM_SR_UIF;

        /* 2. Atomic LED toggle via BSRR (no non-atomic ODR ^= in ISR) */
        if ((GPIOA->ODR & GPIO_ODR_OD5) != 0U) {
            GPIOA->BSRR = GPIO_BSRR_BR5;
        } else {
            GPIOA->BSRR = GPIO_BSRR_BS5;
        }

        g_tim2_ticks++;

        /* 3. Ensure APB1 SR clear retires before exception return */
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
| Duplicate back-to-back ISR entries | Missing `__DSB()` after APB1 `TIM2->SR` clear | Add `__DSB();` before returning from `TIM2_IRQHandler` |

---

## 10. Verification Procedure

- [ ] Code compiles cleanly with `arm-none-eabi-gcc -Wall -Wextra -Werror`.
- [ ] `TIM2->EGR = TIM_EGR_UG` executes before `TIM2->DIER = TIM_DIER_UIE`.
- [ ] `TIM2_IRQHandler` uses `TIM2->SR = ~TIM_SR_UIF;`, atomic `GPIOA->BSRR`, and `__DSB()`.
- [ ] Oscilloscope / logic analyzer on `PA5` measures `100.0 ms ± 1 ms` high and low pulse widths (`5.00 Hz` square wave).

---

## 11. Extension Exercises

1. **Freeze During Debug:** Add `DBGMCU->APB1FZR1 |= DBGMCU_APB1FZR1_DBG_TIM2_STOP;` and verify in GDB that `TIM2->CNT` stops incrementing while single-stepping.
2. **APB Prescaler Doubler Verification:** Change `RCC_CFGR_PPRE1` to `/2` (`85 MHz` APB1) and prove experimentally that `TIM2` *still* runs at `170 MHz` because the STM32 timer clock multiplier applies $\times 2$ whenever `PPRE1 != /1` (RM0440 §7.2.15).
3. **32-Bit Microsecond Free-Running Counter:** Reconfigure `TIM2` with `PSC = 169` ($1\text{ MHz}$ = $1\ \mu\text{s}$ resolution) and `ARR = 0xFFFFFFFF` as a 71.5-minute hardware timestamp counter.
