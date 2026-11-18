# Lab 13: Independent Watchdog (IWDG) System Health Protection & Fault Recovery

> **Verification Status:** Compile-Verified (`arm-none-eabi-gcc 14.3.1`) / Statically Reviewed (Not Tested on Physical NUCLEO-G474RE Hardware)

---

## 1. Objective & Concept IDs

Configure the hardware **Independent Watchdog (`IWDG`)** clocked from the internal `32 kHz LSI` oscillator for a `1.0 s` fail-safe reset window (`PR = 100b = 4` $\implies /64$, `RLR = 499`), freeze `IWDG` during GDB halts via **`DBGMCU->APB1FZR1`**, fix the **double fatal boot-loop bug** (explicitly clearing `GPIOC->MODER[27:26]` from STM32G4 reset Analog mode + reading **Active-HIGH `B1` (`PC13`)** per UM2505), and detect post-watchdog recovery via `RCC->CSR.IWDGRSTF`.

- **Target Concept IDs:** `PERIPH-WDG-001` (IWDG LSI Clocking & Key Protocol), `STM32-DBGMCU-001` (Debug Freeze Control), `STM32-RESET-001` (`RCC->CSR` Reset Cause Diagnostics), `PERIPH-GPIO-001` (STM32G4 Analog Reset Mode)
- **Scope Layer:** `[STM32G474RE Exact]`

---

## 2. Required Hardware

- **Development Board:** ST NUCLEO-G474RE (`MB1367` Rev C)
- **Onboard Peripherals:** Blue Pushbutton `B1` (`PC13`, Active-HIGH) to inject an intentional software hang; Green LED `LD2` (`PA5`) to indicate normal heartbeat vs watchdog-reset recovery

---

## 3. Required Software

- `arm-none-eabi-gcc` (14.3.1+), `make`, `openocd`, `arm-none-eabi-gdb`
- CMSIS headers (`stm32g474xx.h`, `core_cm4.h`)

---

## 4. Wiring & Pin Configuration

| MCU Pin | Board Net | `MODER` | `PUPDR` | Active Level | Role |
|---|---|---|---|---|---|
| `PC13` | `B1` (Blue Button) | **`00b` (Digital Input — MUST clear from `11b` reset!)** | `00b` (No Pull — external pull-down) | **HIGH (`1`) = Pressed** | Press to inject simulated deadlock (`while(1);`) |
| `PA5` | `LD2` (Green LED) | `01b` (Output Push-Pull) | `00b` (No Pull) | `1` = ON | 5 fast blinks on `IWDGRSTF` boot; steady toggle while healthy |

> [!CAUTION]
> **Why Unfixed STM32G4 Code Boot-Loops Permanently:**  
> 1. On STM32G4, `GPIOC->MODER[27:26]` resets to **`11b` (Analog Mode)**, disconnecting the input Schmitt trigger so `GPIOC->IDR` bit 13 **always reads `0`**.  
> 2. On NUCLEO-G474RE (UM2505), `B1` is **Active-HIGH** (`0` = unpressed, `1` = pressed).  
> If you omit clearing `GPIOC->MODER[27:26]` OR test `(IDR & (1<<13)) == 0`, the MCU immediately enters `while(1);` on boot and resets every second forever!

---

## 5. Register Configuration & Mathematical Derivation

**Official Documentation:** **RM0440 Chapter 35** (*Independent Watchdog — IWDG*), **RM0440 Chapter 7** (*RCC*, §7.4.21 `RCC_CSR`), **RM0440 Chapter 47** (*DBGMCU*, `DBGMCU_APB1FZR1`), **DS12288** (*LSI oscillator tolerance*).

### 5.1 Watchdog Timeout Math (Accounting for LSI ±5% Tolerance)

- Nominal $f_{\text{LSI}} = 32\,000\text{ Hz}$ (per DS12288, STM32G4 LSI is factory-calibrated to $32\text{ kHz} \pm 5\%$, i.e., $30.4\text{ kHz}\text{–}33.6\text{ kHz}$).
- Prescaler divider `/64` is **`PR = 100b = 4U` (`IWDG_PR_PR_2`)** (NOT `010b`!):
  $$f_{\text{IWDG}} = \frac{32\,000\text{ Hz}}{64} = 500\text{ Hz} \quad (2.0\text{ ms per tick})$$
- Reload value `RLR = 499U` ($500$ ticks):
  $$T_{\text{nom}} = \frac{64 \times (499 + 1)}{32\,000} = 1.000\text{ s}, \qquad T_{\text{min (+5\% LSI)}} = 0.952\text{ s}, \qquad T_{\text{max (-5\% LSI)}} = 1.053\text{ s}$$
- Therefore, firmware **MUST** kick (`IWDG->KR = 0xAAAAU`) comfortably faster than $952\text{ ms}$ (e.g., every **`250 ms`** via deterministic SysTick!).

| Register | Value | Meaning |
|---|---|---|
| `DBGMCU->APB1FZR1` | `\|= DBGMCU_APB1FZR1_DBG_IWDG_STOP` (bit 12) | Freezes IWDG counter while CPU is halted in GDB |
| `IWDG->KR` | `0xCCCC` / `0x5555` / `0xAAAA` | Start / Unlock `PR`+`RLR` / Reload (kick) |
| `IWDG->PR` | `4U` (`100b` = `IWDG_PR_PR_2`) | Prescaler `/64` |
| `IWDG->RLR` | `499U` (`0x01F3`) | 500 ticks = `1.000 s` nominal |
| `RCC->CSR` | `RCC_CSR_IWDGRSTF` (b29), `RCC_CSR_RMVF` (b23) | Detect and clear IWDG reset flag |

---

## 6. Code Implementation

```c
#include "stm32g474xx.h"
#include <stdbool.h>
#include <stdint.h>

#define SYSCLK_HZ_HSI16     (16000000UL)
#define IWDG_SYNC_TIMEOUT   (500000UL)

static volatile uint32_t g_ms_ticks = 0U;

void SysTick_Handler(void) {
    g_ms_ticks++;
}

static void SysTick_Init_1ms(void) {
    SysTick->CTRL = 0U;
    SysTick->LOAD = (SYSCLK_HZ_HSI16 / 1000UL) - 1UL; /* 1.000 ms @ 16 MHz HSI16 */
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

static inline void led_toggle_atomic(void) {
    if ((GPIOA->ODR & GPIO_ODR_OD5) != 0U) {
        GPIOA->BSRR = GPIO_BSRR_BR5;
    } else {
        GPIOA->BSRR = GPIO_BSRR_BS5;
    }
}

static bool IWDG_Init_1s(void) {
    /* 1. Freeze IWDG during GDB halts so breakpoints do not trigger resets */
    DBGMCU->APB1FZR1 |= DBGMCU_APB1FZR1_DBG_IWDG_STOP;

    /* 2. Start IWDG (automatically enables LSI) & unlock PR/RLR */
    IWDG->KR = 0xCCCCU;
    IWDG->KR = 0x5555U;

    /* 3. PR = 100b = 4U (/64 -> 500 Hz); RLR = 499U (500 ticks = 1.000 s nominal) */
    IWDG->PR  = IWDG_PR_PR_2; /* 0x4U */
    IWDG->RLR = 499U;

    /* 4. Wait for register synchronization across LSI clock domain with bounded timeout */
    uint32_t timeout = IWDG_SYNC_TIMEOUT;
    while (IWDG->SR != 0U) {
        if (--timeout == 0U) {
            return false;
        }
    }

    /* 5. Reload counter with 499 and lock PR/RLR */
    IWDG->KR = 0xAAAAU;
    return true;
}

int main(void) {
    SysTick_Init_1ms();

    /* 1. Enable GPIOA (LD2) and GPIOC (B1) clocks + sync readback */
    RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN | RCC_AHB2ENR_GPIOCEN;
    volatile uint32_t tmpreg = RCC->AHB2ENR;
    (void)tmpreg;

    /* 2. PA5 -> Push-Pull Output */
    GPIOA->MODER  &= ~(3U << (5U * 2U));
    GPIOA->MODER  |=  (1U << (5U * 2U));
    GPIOA->OTYPER &= ~(1U << 5U);

    /* 3. CRITICAL: Clear PC13 out of STM32G4 reset Analog mode (11b -> 00b Input)
     *    and leave PUPDR = 00b (external pull-down on NUCLEO-G474RE) */
    GPIOC->MODER &= ~(3U << (13U * 2U));
    GPIOC->PUPDR &= ~(3U << (13U * 2U));

    /* 4. Check Reset Cause in RCC->CSR before starting watchdog */
    if ((RCC->CSR & RCC_CSR_IWDGRSTF) != 0U) {
        RCC->CSR |= RCC_CSR_RMVF; /* Clear all reset cause flags */

        /* Rapidly blink LD2 5 times (10 toggles @ 80 ms) to signal IWDG recovery */
        for (uint32_t i = 0U; i < 10U; i++) {
            led_toggle_atomic();
            delay_ms(80U);
        }
        GPIOA->BSRR = GPIO_BSRR_BR5;
    } else {
        RCC->CSR |= RCC_CSR_RMVF; /* Clean slate on power-on reset */
    }

    /* 5. Arm 1.0 s Hardware Independent Watchdog */
    (void)IWDG_Init_1s();

    while (1) {
        /* Kick watchdog every 250 ms (safe margin below 952 ms worst-case LSI timeout) */
        IWDG->KR = 0xAAAAU;
        led_toggle_atomic();

        /* Active-HIGH B1 (PC13 == 1 when pressed per UM2505): simulate software hang! */
        if ((GPIOC->IDR & GPIO_IDR_ID13) != 0U) {
            /* Deliberate deadlock: IWDG resets MCU after ~1.0 s! */
            while (1) {
                __NOP();
            }
        }

        delay_ms(250U);
    }
}
```

---

## 7. Expected Output

- **Normal Operation:** `LD2` blinks steadily every `250 ms` (`2 Hz`) indefinitely; MCU never resets.
- **Fault Injection:** Pressing `B1` (`PC13`) traps the CPU in the simulated deadlock (`LD2` freezes). Exactly **`~1.0 s`** later, `IWDG` asserts a hardware reset; on reboot, `RCC_CSR_IWDGRSTF` triggers 5 rapid recovery blinks before returning to normal operation.

---

## 8. Debugging Procedure

1. Set a breakpoint right after `IWDG_Init_1s();` and inspect `DBGMCU->APB1FZR1` (bit 12 == `1`).
2. Remain halted at the breakpoint for 5+ seconds: confirm the MCU does **NOT** reset because `DBG_IWDG_STOP` froze the watchdog counter!
3. Resume execution, press `B1`, and observe GDB catch the reset event after ~1.0 s with `(RCC->CSR & RCC_CSR_IWDGRSTF) != 0`.

---

## 9. Common Errors & Troubleshooting

| Symptom | Root Cause | Verified Fix |
|---|---|---|
| MCU continuously reboots every 1.0 s without pressing `B1` | Forgot `GPIOC->MODER &= ~(3U << 26U)` (left in Analog `11b`) OR inverted Active-LOW check `(IDR & (1<<13)) == 0` | Clear `GPIOC->MODER[27:26]` and test `(GPIOC->IDR & GPIO_IDR_ID13) != 0U` |
| GDB disconnects / target resets while paused at a breakpoint | Omitted `DBGMCU->APB1FZR1 \|= DBGMCU_APB1FZR1_DBG_IWDG_STOP` | Set `DBG_IWDG_STOP` before starting `IWDG` |
| Timeout is 4× shorter (~250 ms) than expected | Wrote `IWDG->PR = 2U` (`010b` = `/16`) instead of `4U` (`100b` = `/64`) | Use CMSIS constant `IWDG_PR_PR_2` (`4U` = `100b` = `/64`) |
| Watchdog fails to catch main-loop freeze | Kicked `IWDG->KR = 0xAAAA` inside `SysTick_Handler`! | **NEVER** kick a watchdog inside a timer ISR; kick only at the end of a verified main loop |

---

## 10. Verification Procedure

- [ ] Code compiles cleanly with `arm-none-eabi-gcc -Wall -Wextra -Werror`.
- [ ] `GPIOC->MODER[27:26] == 00b` and `IWDG->PR == 4U` (`100b` = `/64`).
- [ ] `DBGMCU->APB1FZR1 & DBGMCU_APB1FZR1_DBG_IWDG_STOP != 0`.
- [ ] MCU runs cleanly without resetting when `B1` is unpressed, and recovers with 5 rapid blinks ~1.0 s after pressing `B1`.

---

## 11. Extension Exercises

1. **Windowed IWDG (`IWDG->WINR`):** Set `IWDG->WINR = 250U` so refreshing *too early* (runaway loop faster than 500 ms) ALSO triggers a watchdog reset!
2. **Window Watchdog (`WWDG`, RM0440 Ch. 36):** Configure `WWDG` with Early Wakeup Interrupt (`EWI`) to log crash telemetry to Backup RAM before reset.
3. **Multi-Task Supervisor:** Require 3 independent software subsystems to each set a health bit in a bitmask before `main()` executes `IWDG->KR = 0xAAAAU`.
