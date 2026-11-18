# Lab 04: SysTick Accurate Millisecond Delay & Non-Blocking Timer Generator

> **Verification Status:** Compile-Verified (`arm-none-eabi-gcc 14.3.1`) / Statically Reviewed (Not Tested on Physical NUCLEO-G474RE Hardware)

---

## 1. Objective & Concept IDs

Initialize the STM32G474RE system clock to **`170 MHz`** (eliminating the classic `16 MHz` vs `170 MHz` 10.625× timing mismatch), configure the ARM Cortex-M4 24-bit **`SysTick`** core timer for exact `1.000 ms` interrupts, and implement both a wraparound-safe blocking `delay_ms()` and a non-blocking `millis()` periodic scheduler.

- **Target Concept IDs:** `PERIPH-SYSTICK-001` (24-bit SysTick Timer), `ARM-EXCEP-001` (Core Exceptions), `EMB-VOL-001` (`volatile` ISR-Shared Counters), `FW-EXEC-002` (Wraparound-Safe Unsigned Tick Math)
- **Scope Layer:** `[Cortex-M4 Specific]` / `[STM32G474RE Exact]`

---

## 2. Required Hardware

- **Development Board:** ST NUCLEO-G474RE (`MB1367` Rev C)
- **Onboard Peripherals:** Green LED `LD2` (`PA5`)
- **Optional Verification Equipment:** Logic analyzer or oscilloscope to verify exact `500.0 ms` HIGH / `500.0 ms` LOW pulse widths on `PA5`

---

## 3. Required Software

- `arm-none-eabi-gcc` (14.3.1+), `make`, `openocd`, `arm-none-eabi-gdb`
- CMSIS headers (`stm32g474xx.h`, `core_cm4.h`)

---

## 4. Wiring & Pin Configuration

| MCU Pin | Board Net | Direction | `MODER` | `OTYPER` | Purpose |
|---|---|---|---|---|---|
| `PA5` | `LD2` (Green LED) | Output | `01b` (Output) | `0b` (Push-Pull) | Visual & logic-analyzer 1.000 Hz timing probe |

---

## 5. Register Configuration & Mathematical Derivation

**Official Documentation:** **ARM PM0214 §4.5** (*SysTick Timer*) and **RM0440 Chapters 6–7** (*PWR / RCC 170 MHz Configuration*).

| Register | Address | Fields | Value @ 170 MHz | Purpose |
|---|---|---|---|---|
| `SysTick->LOAD` | `0xE000E014` | `RELOAD[23:0]` | `169999` (`0x0002980F`) | Reload count for 1 ms period |
| `SysTick->VAL` | `0xE000E018` | `CURRENT[23:0]` | Write `0` | Clears current counter and `COUNTFLAG` |
| `SysTick->CTRL` | `0xE000E010` | `CLKSOURCE` (b2), `TICKINT` (b1), `ENABLE` (b0) | `0x00000007` | HCLK (170 MHz), IRQ enabled, timer running |

### Timing & Wraparound Math

1. **Reload Calculation:** Since SysTick counts down from `LOAD` to `0` inclusive ($N = \text{LOAD} + 1$ clock ticks per period):
   $$\text{LOAD} = \frac{f_{\text{HCLK}}}{f_{\text{tick}}} - 1 = \frac{170\,000\,000\text{ Hz}}{1000\text{ Hz}} - 1 = 169\,999 = \text{0x02980F}$$
   Check 24-bit limit: $169\,999 \le 2^{24} - 1 = 16\,777\,215\ \checkmark$.
2. **32-Bit Rollover Safety:** A `uint32_t` millisecond counter rolls over after $2^{32}\text{ ms} \approx 49.71\text{ days}$. Comparing `systick_ms < (start + delay_ms)` **BREAKS** across rollover! Using unsigned modular subtraction:
   ```c
   (uint32_t)(systick_ms - start) < delay_ms
   ```
   remains mathematically exact across the `0xFFFFFFFF -> 0x00000000` wraparound boundary (for any `delay_ms < 2^31`) by ISO C unsigned modulo-$2^{32}$ arithmetic.

---

## 6. Code Implementation

```c
#include "stm32g474xx.h"
#include <stdint.h>

extern uint32_t SystemCoreClock;

static volatile uint32_t systick_ms = 0U;

void SysTick_Handler(void) {
    systick_ms++;
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

    /* Mandatory RM0440 §6.1.5 intermediate HPRE = /2 step */
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
    SysTick->LOAD = (170000000UL / 1000UL) - 1UL; /* 169,999 ticks = 1.000 ms @ 170 MHz */
    SysTick->VAL  = 0U;
    NVIC_SetPriority(SysTick_IRQn, 15U);          /* Lowest priority core tick */
    SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk |
                    SysTick_CTRL_TICKINT_Msk   |
                    SysTick_CTRL_ENABLE_Msk;
}

static inline uint32_t millis(void) {
    /* Aligned 32-bit load on Cortex-M4 is single-instruction atomic */
    return systick_ms;
}

static void delay_ms(uint32_t delay) {
    uint32_t start = millis();
    while ((uint32_t)(systick_ms - start) < delay) {
        __WFI(); /* Save power between 1 ms SysTick interrupts */
    }
}

int main(void) {
    /* CRITICAL: Must initialize 170 MHz PLL before configuring SysTick for 170 MHz! */
    Clock_Init_170MHz();
    SysTick_Init_1ms();

    RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN;
    volatile uint32_t tmpreg = RCC->AHB2ENR;
    (void)tmpreg;

    GPIOA->MODER &= ~(3U << (5U * 2U));
    GPIOA->MODER |=  (1U << (5U * 2U));

    /* Demonstration 1: 3 blocking blinks using wraparound-safe delay_ms() */
    for (uint32_t i = 0U; i < 3U; i++) {
        GPIOA->BSRR = GPIO_BSRR_BS5;
        delay_ms(200U);
        GPIOA->BSRR = GPIO_BSRR_BR5;
        delay_ms(200U);
    }

    /* Demonstration 2: Drift-free non-blocking periodic toggle (500 ms ON / 500 ms OFF) */
    uint32_t next_toggle_ms = millis() + 500U;
    uint32_t led_on = 0U;

    while (1) {
        if ((int32_t)(millis() - next_toggle_ms) >= 0) {
            next_toggle_ms += 500U; /* Periodic phase-locked schedule (no drift) */
            led_on ^= 1U;
            GPIOA->BSRR = (led_on != 0U) ? GPIO_BSRR_BS5 : GPIO_BSRR_BR5;
        }
    }
}
```

---

## 7. Expected Output

`LD2` (`PA5`) flashes rapidly 3 times (`200 ms` ON / `200 ms` OFF), then transitions into an exact, drift-free **`1.000 Hz` blink** (`500 ms` ON / `500 ms` OFF, $\pm 1\%$ HSI16 tolerance).

---

## 8. Debugging Procedure

1. Set a breakpoint inside `SysTick_Handler`. Confirm it fires repeatedly and `systick_ms` increments by 1 each hit.
2. Inspect `SysTick->LOAD` (`0x0002980F` = `169999`) and `SysTick->VAL` (counting downwards toward `0`).
3. To test 32-bit rollover safety in GDB without waiting 49.7 days:
   ```gdb
   (gdb) set systick_ms = 0xFFFFFFF0
   (gdb) continue
   ```
   Verify `delay_ms()` and the periodic toggle continue seamlessly across `0xFFFFFFFF -> 0x00000000`.

---

## 9. Common Errors & Troubleshooting

| Symptom | Root Cause | Verified Fix |
|---|---|---|
| LED blinks every **10.625 seconds** instead of **1.0 second** | Forgot to call `Clock_Init_170MHz()`; MCU stayed at `16 MHz HSI16` while `LOAD` assumed `170 MHz` | Always call `Clock_Init_170MHz()` (or derive `LOAD` from `SystemCoreClock`) |
| `delay_ms()` hangs forever under `-O2` | `systick_ms` not declared `volatile` (compiler hoists load outside loop) | Declare `static volatile uint32_t systick_ms;` |
| `SysTick_Handler` never fires | Startup vector table symbol mismatch or global IRQs disabled (`PRIMASK=1`) | Verify `SysTick_Handler` matches vector table entry #15 and `TICKINT=1` |
| Delay breaks after 49.7 days | Wrote `while (systick_ms < start + delay)` | Always use unsigned difference `(uint32_t)(systick_ms - start) < delay` |

---

## 10. Verification Procedure

- [ ] Code compiles cleanly with `arm-none-eabi-gcc -Wall -Wextra -Werror`.
- [ ] `RCC->CFGR & RCC_CFGR_SWS == RCC_CFGR_SWS_PLL` (`170 MHz`) confirmed before `SysTick_Init_1ms()`.
- [ ] `SysTick->LOAD == 169999U` and `SysTick->CTRL == 0x7U` (plus `COUNTFLAG` bit 16 pulsing).
- [ ] Logic analyzer on `PA5` measures $T_{\text{HIGH}} = 500\text{ ms} \pm 5\text{ ms}$ (HSI16 ±1% spec).
- [ ] GDB rollover injection (`systick_ms = 0xFFFFFFF0`) produces zero stall or glitch.

---

## 11. Extension Exercises

1. **Microsecond `delay_us()` Using `SysTick->VAL`:** Combine `systick_ms` and the down-counting `SysTick->VAL` register (handling the double-read race condition when `VAL` wraps during the read!) to build a $1\ \mu\text{s}$-resolution timestamp function.
2. **Multi-Rate Software Timers:** Add a second independent LED blinking at `333 ms` in the same non-blocking `while(1)` loop without modifying `delay_ms()`.
3. **Compare with `DWT->CYCCNT`:** Enable `CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk` and `DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk` to measure exact cycle jitter of `SysTick_Handler`.
