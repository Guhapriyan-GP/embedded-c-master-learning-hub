# Lab 14: Low-Power Stop 0 Mode & EXTI13 Wakeup with Full 170 MHz Clock Restore

> **Verification Status:** Compile-Verified (`arm-none-eabi-gcc 14.3.1`) / Statically Reviewed (Not Tested on Physical NUCLEO-G474RE Hardware)

---

## 1. Objective & Concept IDs

Place the STM32G474RE safely into **Stop 0** low-power mode (`PWR->CR1.LPMS = 000b`, `SCB->SCR.SLEEPDEEP = 1`, unused GPIOs parked in Analog mode, SysTick suspended), measure true MCU supply current across the correct **`JP6` (`IDD`)** jumper (fixing the dangerous **`JP5`** error!), wake cleanly via an Active-HIGH Rising-Edge **`EXTI13` (`PC13`)** interrupt with a real `EXTI15_10_IRQHandler()`, clear `SLEEPDEEP`, and restore the **`170 MHz PLL`** clock tree after wakeup.

- **Target Concept IDs:** `STM32-PWR-001` (Stop 0 / Stop 1 Modes & Clock Re-Init), `PERIPH-EXTI-001` (Async Stop Wakeup), `ARM-CORE-001` (`SLEEPDEEP`, `__DSB()`/`__ISB()`), `DIG-ELEC-005` (Low-Power Current Measurement & GPIO Leakage)
- **Scope Layer:** `[STM32G474RE Exact]`

---

## 2. Required Hardware

- **Development Board:** ST NUCLEO-G474RE (`MB1367` Rev C)
- **Measurement Equipment:** Digital Multimeter (DMM) in microamp/milliamp (`µA`/`mA`) range with micro-grabber leads (or ST X-NUCLEO-LPM01A power shield)

---

## 3. Required Software

- `arm-none-eabi-gcc` (14.3.1+), `make`, `openocd`, `arm-none-eabi-gdb`
- CMSIS headers (`stm32g474xx.h`, `core_cm4.h`)

---

## 4. Wiring & Pin Configuration

> [!CAUTION]
> **CRITICAL HARDWARE SAFETY FIX — USE `JP6`, NEVER `JP5` (ST UM2505, MB1367 Schematic)!**  
> - **`JP6` (`IDD`)** is the dedicated **MCU $V_{\text{DD}}$ current measurement jumper** on NUCLEO-G474RE. Remove the shunt on **`JP6`** and connect your ammeter in series across `JP6` pins 1–2!
> - **`JP5` (`5V_SEL`)** is the **5 V power-source selector** (`STLK` / `VIN` / `E5V`)! Connecting a DMM in current mode across `JP5` measures the entire 5 V rail (including ST-LINK/V3E) or risks shorting power supplies!

| Hardware Item | Setting / Connection | Notes |
|---|---|---|
| **`JP6` (`IDD`)** | Remove shunt; DMM in series (`mA`/`µA`) | Measures STM32G474RE $I_{\text{DD}}$ directly |
| `PC13` (`B1` Button) | `MODER = 00b`, `PUPDR = 00b`, `EXTI13` Rising Edge | Active-HIGH press (`3.3 V`) wakes MCU from Stop 0 |
| `PA5` (`LD2` LED) | `MODER = 01b` (active), driven LOW before Stop 0 | Blinks 3 times on wakeup, then turns OFF before sleeping |
| All other unused pins | `MODER = 11b` (Analog Mode, default at reset) | **CRITICAL:** Eliminates floating Schmitt-trigger static leakage current! |

> [!NOTE]
> **STM32G4 Stop Modes (RM0440 §6.3.6):** STM32G474RE implements **Stop 0** (`LPMS = 000b`, main regulator ON, fast wakeup, ~150–400 µA typ) and **Stop 1** (`LPMS = 001b`, low-power regulator, lower current). **Stop 2 does NOT exist** on STM32G4 (it is an STM32L4/U5 mode). Full SRAM1/SRAM2/CCM-SRAM and all peripheral register contents are retained across Stop 0.

---

## 5. Register Configuration & Mathematical Derivation

**Official Documentation:** **RM0440 Chapter 6** (*Power control — PWR*, §6.3.6 Stop 0 mode), **RM0440 Chapter 10** (`SYSCFG`), **RM0440 Chapter 15** (`EXTI`), **ARM PM0214 §4.4.6** (`SCB->SCR`), and **ST UM2505** (`JP6` IDD).

| Step | Register Operation | Why It Is Mandatory |
|---|---|---|
| 1 | `RCC->APB1ENR1 \|= RCC_APB1ENR1_PWREN;` | Without `PWREN`, writes to `PWR->CR1` are silently ignored! |
| 2 | `RCC->APB2ENR \|= RCC_APB2ENR_SYSCFGEN;` + `GPIOC->MODER &= ~(3U<<26)` | Exits Analog reset mode and routes `PC13` to `EXTI13` |
| 3 | `EXTI->RTSR1 \|= EXTI_RTSR1_RT13;` + `EXTI->PR1 = EXTI_PR1_PIF13;` | Active-HIGH rising-edge wakeup; clears stale flag before sleep |
| 4 | Suspend `SysTick` (`TICKINT = 0`) | Prevents 1 ms SysTick interrupt from immediately aborting `__WFI()` |
| 5 | `PWR->CR1 = (PWR->CR1 & ~PWR_CR1_LPMS) \| PWR_CR1_LPMS_STOP0;` | Selects Stop 0 mode (`LPMS = 000b`) |
| 6 | `SCB->SCR \|= SCB_SCR_SLEEPDEEP_Msk; __DSB(); __WFI(); __ISB();` | Enters deep sleep deterministically |
| 7 | `SCB->SCR &= ~SCB_SCR_SLEEPDEEP_Msk;` | Prevents subsequent normal `__WFI()` calls from re-entering Stop 0! |
| 8 | `Clock_Init_170MHz();` | **Stop 0 halts the PLL and wakes on `16 MHz HSI16`!** Must re-enable 170 MHz PLL. |

---

## 6. Code Implementation

```c
#include "stm32g474xx.h"
#include <stdint.h>

extern uint32_t SystemCoreClock;

static volatile uint32_t g_wakeup_count = 0U;

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

    if ((RCC->CR & RCC_CR_PLLON) != 0U) {
        RCC->CR &= ~RCC_CR_PLLON;
        while ((RCC->CR & RCC_CR_PLLRDY) != 0U) {}
    }

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

/* CRITICAL: Must implement EXTI15_10_IRQHandler or wakeup jumps to Default_Handler! */
void EXTI15_10_IRQHandler(void) {
    if ((EXTI->PR1 & EXTI_PR1_PIF13) != 0U) {
        EXTI->PR1 = EXTI_PR1_PIF13; /* Clear rc_w1 pending bit */
        g_wakeup_count++;
        __DSB();
    }
}

static void EXTI13_Wakeup_Init(void) {
    RCC->AHB2ENR |= RCC_AHB2ENR_GPIOCEN | RCC_AHB2ENR_GPIOAEN;
    RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN;
    volatile uint32_t tmpreg = RCC->APB2ENR;
    (void)tmpreg;

    /* PA5 (LD2) Output Push-Pull, initially OFF */
    GPIOA->MODER  &= ~(3U << (5U * 2U));
    GPIOA->MODER  |=  (1U << (5U * 2U));
    GPIOA->OTYPER &= ~(1U << 5U);
    GPIOA->BSRR    =  GPIO_BSRR_BR5;

    /* CRITICAL: Clear PC13 out of STM32G4 reset Analog mode (11b -> 00b Digital Input) */
    GPIOC->MODER &= ~(3U << (13U * 2U));
    GPIOC->PUPDR &= ~(3U << (13U * 2U)); /* External pull-down on NUCLEO-G474RE */

    /* Clear EXTICR[3] nibble [7:4] before mapping Port C (0010b) to EXTI13 */
    SYSCFG->EXTICR[3] &= ~SYSCFG_EXTICR4_EXTI13_Msk;
    SYSCFG->EXTICR[3] |=  SYSCFG_EXTICR4_EXTI13_PC;

    /* Active-HIGH B1 press = Rising Edge trigger */
    EXTI->RTSR1 |=  EXTI_RTSR1_RT13;
    EXTI->FTSR1 &= ~EXTI_FTSR1_FT13;
    EXTI->PR1    =  EXTI_PR1_PIF13; /* Clear stale flag */
    EXTI->IMR1  |=  EXTI_IMR1_IM13;

    NVIC_SetPriorityGrouping(0U);
    NVIC_SetPriority(EXTI15_10_IRQn, 2U);
    NVIC_ClearPendingIRQ(EXTI15_10_IRQn);
    NVIC_EnableIRQ(EXTI15_10_IRQn);
}

static void Enter_Stop0_Mode(void) {
    /* 1. Ensure PWR clock is enabled before writing PWR->CR1 */
    RCC->APB1ENR1 |= RCC_APB1ENR1_PWREN;
    volatile uint32_t tmpreg = RCC->APB1ENR1;
    (void)tmpreg;

    /* 2. Turn LED OFF to minimize board current draw; unused pins remain in reset Analog (11b) */
    GPIOA->BSRR = GPIO_BSRR_BR5;

    /* 3. Suspend SysTick if active so periodic ticks do not abort Stop 0 */
    uint32_t systick_ctrl_saved = SysTick->CTRL;
    SysTick->CTRL &= ~SysTick_CTRL_TICKINT_Msk;

    /* 4. Clear any pending EXTI13 flag prior to sleeping */
    EXTI->PR1 = EXTI_PR1_PIF13;
    NVIC_ClearPendingIRQ(EXTI15_10_IRQn);

    /* 5. Select Stop 0 mode (LPMS = 000b) in PWR->CR1 */
    PWR->CR1 = (PWR->CR1 & ~PWR_CR1_LPMS) | PWR_CR1_LPMS_STOP0;

    /* 6. Set SLEEPDEEP, execute barriers, and enter Stop 0 via WFI */
    SCB->SCR |= SCB_SCR_SLEEPDEEP_Msk;
    __DSB();
    __WFI();
    __ISB();

    /* 7. Immediately clear SLEEPDEEP after wakeup so normal WFI stays in Sleep mode */
    SCB->SCR &= ~SCB_SCR_SLEEPDEEP_Msk;
    __DSB();

    /* 8. Stop 0 wakes on 16 MHz HSI16: restore full 170 MHz PLL clock tree! */
    Clock_Init_170MHz();

    /* 9. Restore SysTick interrupt state */
    SysTick->CTRL = systick_ctrl_saved;
}

static void blink_active_indicator(void) {
    for (uint32_t i = 0U; i < 3U; i++) {
        GPIOA->BSRR = GPIO_BSRR_BS5;
        for (volatile uint32_t d = 0U; d < 4000000U; d++) { __NOP(); }
        GPIOA->BSRR = GPIO_BSRR_BR5;
        for (volatile uint32_t d = 0U; d < 4000000U; d++) { __NOP(); }
    }
}

int main(void) {
    /* Unused GPIO pins remain in STM32G4 default Analog Mode (MODER = 11b),
     * preventing floating Schmitt-trigger leakage during Stop 0! */
    Clock_Init_170MHz();
    EXTI13_Wakeup_Init();

    while (1) {
        /* Active 170 MHz burst (~30 mA across JP6) */
        blink_active_indicator();

        /* Enter Stop 0 (~hundreds of uA across JP6; retains all SRAM) */
        Enter_Stop0_Mode();
    }
}
```

---

## 7. Expected Output

1. On boot, `LD2` blinks 3 times at 170 MHz (DMM across **`JP6`** reads ~`25–35 mA`).
2. MCU enters **Stop 0** (`LD2` OFF): DMM across **`JP6`** drops sharply to **sub-milliamp levels** (note: detach active SWD debugger or run standalone power cycle for true datasheet microamp figures, since an attached SWD probe keeps debug domains powered!).
3. Pressing `B1` (`PC13`) wakes the MCU instantly via `EXTI15_10_IRQHandler`, restores `170 MHz`, increments `g_wakeup_count` (retained in SRAM!), blinks `LD2` 3 times, and returns to Stop 0.

---

## 8. Debugging Procedure

1. To debug Stop 0 without losing SWD connection, optionally enable `DBGMCU->CR |= DBGMCU_CR_DBG_STOP;` (remember to disable it when measuring true low-power current on `JP6`!).
2. Place a breakpoint right after `Clock_Init_170MHz();` inside `Enter_Stop0_Mode()`.
3. Press `B1` and verify:
   - `g_wakeup_count` incremented by `1` (SRAM retained).
   - `(SCB->SCR & SCB_SCR_SLEEPDEEP_Msk) == 0`.
   - `(RCC->CFGR & RCC_CFGR_SWS) == RCC_CFGR_SWS_PLL` (`170 MHz` restored!).

---

## 9. Common Errors & Troubleshooting

| Symptom | Root Cause | Verified Fix |
|---|---|---|
| DMM reads ~60–100 mA even in Stop mode | Connected DMM across **`JP5`** (`5V_SEL`!) instead of **`JP6` (`IDD`)**, OR left SWD debugger actively attached | Measure across **`JP6` (`IDD`)** and power-cycle without debugger attached |
| MCU never wakes up when pressing `B1` | Forgot `GPIOC->MODER &= ~(3U << 26U)` (left in Analog `11b`) or forgot `SYSCFGEN` | Clear `PC13` `MODER` to `00b`, enable `SYSCFGEN`, set `EXTI->RTSR1` |
| MCU crashes/freezes right after pressing `B1` | Missing `EXTI15_10_IRQHandler()` — vectored straight to `Default_Handler`! | Implement `EXTI15_10_IRQHandler()` clearing `EXTI->PR1` + `__DSB()` |
| Current barely drops (~10 mA) in Stop mode | Forgot `RCC_APB1ENR1_PWREN` or left floating digital input pins leaking current | Enable `PWREN` before writing `PWR->CR1`, park unused GPIOs in Analog (`11b`) |
| After wakeup, timers/UART run 10.625× too slow | Stop 0 reverted `SYSCLK` to `16 MHz HSI16` and firmware forgot to re-call `Clock_Init_170MHz()` | Call `Clock_Init_170MHz()` immediately after `__WFI()` |

---

## 10. Verification Procedure

- [ ] Code compiles cleanly with `arm-none-eabi-gcc -Wall -Wextra -Werror`.
- [ ] `JP6` (`IDD`) verified in UM2505 (never `JP5`).
- [ ] `RCC_APB1ENR1_PWREN`, `RCC_APB2ENR_SYSCFGEN`, `GPIOC->MODER[27:26] = 00b`, `EXTI->RTSR1`, and `EXTI15_10_IRQHandler` all present.
- [ ] `SCB_SCR_SLEEPDEEP_Msk` cleared after wakeup and `RCC->CFGR.SWS == PLL` verified post-wake.

---

## 11. Extension Exercises

1. **Stop 1 Mode Comparison:** Change `PWR_CR1_LPMS_STOP0` to `PWR_CR1_LPMS_STOP1` (`001b`) and compare `JP6` current draw against Stop 0 (verify DS12288 table values!).
2. **Periodic `LPTIM1` / `RTC` Auto-Wakeup:** Wake automatically every 5 seconds using the Low-Power Timer (`LPTIM1` clocked by `LSI`) without pressing `B1`.
3. **Standby Mode + SRAM2 Retention:** Configure `PWR_CR1_LPMS_STANDBY` (`011b`) and `PWR->CR3 |= PWR_CR3_RRS` to retain `SRAM2` across Standby reset.
