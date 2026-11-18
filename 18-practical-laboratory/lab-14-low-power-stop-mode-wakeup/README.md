# Lab 14: Low-Power Stop 0 Mode & EXTI13 Wakeup with Full 170 MHz Clock Restore

> ⬅️ **Prev Lab:** [Lab 13: IWDG Hardware Watchdog](../lab-13-iwdg-hardware-watchdog/README.md) | ⬆️ **Parent Stage:** [18-practical-laboratory (All 20 Labs)](../README.md) | **Next Lab:** [Lab 15: FreeRTOS Multi-Tasking](../lab-15-freertos-multitask-producer-consumer/README.md) ➡️

> **Verification Status:** `CROSS-COMPILED` (`arm-none-eabi-gcc 14.3.1`, `-Wall -Wextra -Werror -O2`, 0 warnings) | **Hardware Status:** `PENDING — USER CONNECTION REQUIRED` *(Requires manual physical `B1` wakeup press + precision µA ammeter across `JP6` `IDD` — NEVER `JP5`!)*

> **Clock Configuration Profile:** **Primary = `170 MHz Range 1 Boost`** (Stop 0 halts PLL, wakes on `16 MHz HSI16`, restores `170 MHz` via `Clock_Init_170MHz()`) | **Alternate = `160 MHz Range 1 Boost`** (restores `160 MHz` via `Clock_Init_160MHz()`) | **Reusable Drivers:** [`23-code-library/Drivers/clock_g4.h`](../../23-code-library/Drivers/clock_g4.h) (`Clock_Init_170MHz()`, `Clock_Init_160MHz()`), [`exti_g4.h`](../../23-code-library/Drivers/exti_g4.h), & [`gpio_g4.h`](../../23-code-library/Drivers/gpio_g4.h) (`GPIO_ConfigAnalog()`).

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

### 5.1 Explicit Clock Configuration Profile Declaration (`170 MHz` vs `160 MHz` Post-Stop Restore)

Entering **Stop 0** (`LPMS = 000b`) unconditionally stops the Main PLL (`RCC->CR.PLLON = 0`) and switches `SYSCLK` back to **`16 MHz HSI16`** on wakeup! Failing to call the exact matching Boost-mode restore function causes all timers/UARTs to run `10.0×` (at `160 MHz`) or `10.625×` (at `170 MHz`) too slow after wakeup:

| Active Run Profile | Post-Stop Restore API (`clock_g4.h`) | Wakeup Clock Before Restore | Restored `PLLM`/`PLLN`/`PLLR` | `PWR->CR5.R1MODE` / `FLASH->ACR` | Intermediate `HPRE = /2` Step |
|---|---|---|---|---|---|
| **`170 MHz Range 1 Boost` *(Primary)*** | `Clock_Init_170MHz()` | `16 MHz HSI16` (`SWS=01b`) | `3` (`/4`) / `85` / `0` (`/2`) | `0` (Boost) / `4 WS` | `85 MHz` ($\ge 1\ \mu\text{s}$) $\to 170\text{ MHz}$ |
| **`160 MHz Range 1 Boost` *(Alternate)*** | `Clock_Init_160MHz()` | `16 MHz HSI16` (`SWS=01b`) | `3` (`/4`) / `80` / `0` (`/2`) | `0` (Boost) / `4 WS` | `80 MHz` ($\ge 1\ \mu\text{s}$) $\to 160\text{ MHz}$ |

- **Reusable Production Drivers (`23-code-library/Drivers/`):** Call `Clock_Init_170MHz()` or `Clock_Init_160MHz()` ([`clock_g4.h`](../../23-code-library/Drivers/clock_g4.h)) immediately after `__WFI(); __ISB(); SCB->SCR &= ~SCB_SCR_SLEEPDEEP_Msk;`, configure `EXTI13` via `EXTI_ConfigLine()` ([`exti_g4.h`](../../23-code-library/Drivers/exti_g4.h)), and park unused GPIOs with `GPIO_ConfigAnalog()` ([`gpio_g4.h`](../../23-code-library/Drivers/gpio_g4.h)).

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
        (void)EXTI->PR1;            /* Flush AHB-to-APB2 posted-write bridge */
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

---

## 12. Standardized Verification Record

| Field | Record |
|---|---|
| **Verification Status** | `CROSS-COMPILED` / `PENDING — USER CONNECTION REQUIRED` |
| **Objective** | Enter STM32G474RE **Stop 0** low-power mode (`PWR->CR1.LPMS = 000b`, `SLEEPDEEP = 1`, SysTick suspended), wake cleanly via rising-edge `EXTI13` (`PC13` `B1`) with full SRAM retention, clear `SLEEPDEEP`, and restore the `170 MHz` (or `160 MHz`) PLL clock tree. |
| **Firmware Version** | `v2.2.0-remediation` (branch `feature/complete-remediation`, base commit `585dc4fa`) |
| **Compiler Version** | `arm-none-eabi-gcc 14.3.1 20250623 (GNU Tools for STM32 14.3.rel1.20251027-0700)` |
| **Build Result** | **PASS** — `arm-none-eabi-gcc -std=c11 -mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard -DSTM32G474xx -Wall -Wextra -Werror -O2 -c` succeeded with **0 warnings, 0 errors** (`1,848 B` object file). |
| **Test Environment** | Windows 11 + ARM GNU Toolchain 14.3.rel1 + NUCLEO-G474RE (`MB1367` Rev C, `STM32G474RET6`, STLINK-V3E `V3J17M11`, `COM8`) |
| **Required Peripherals** | **Onboard Only (Functional Wakeup & Clock Restore):** `PWR`, `RCC`, `SYSCFG`, `EXTI13`, `SCB`, `GPIOA` (`PA5` `LD2`), `GPIOC` (`PC13` `B1`). *(Optional external DMM in series across `JP6` `IDD` — NEVER `JP5` — for absolute µA validation.)* |
| **Hardware Availability** | `PENDING — USER CONNECTION REQUIRED` *(Manual `B1` press + µA DMM across `JP6` `IDD` with SWD detached)* |
| **Pin Configuration** | `PC13` (`B1`): `MODER[27:26] = 00b`, `PUPDR = 00b`, `EXTI13` Rising Edge; `PA5` (`LD2`): `MODER = 01b` (driven LOW prior to Stop 0); all other GPIOs kept in reset Analog (`11b`) for minimum leakage. |
| **Expected Behavior** | Blinks `LD2` 3 times at `170 MHz`, enters Stop 0 (`LD2` OFF); pressing `B1` wakes MCU via `EXTI15_10_IRQHandler`, increments retained `g_wakeup_count`, clears `SLEEPDEEP`, restores `170 MHz` (`SWS = PLL`), and repeats. |
| **Actual Observations** | `PENDING — USER CONNECTION REQUIRED` *(cross-compiled cleanly with 0 warnings; Stop 0 physical wakeup requires manual physical `B1` actuation and true microamp current measurement requires disconnecting SWD and inserting a µA DMM across `JP6` `IDD` per `HARDWARE_VALIDATION/PENDING_EXTERNAL_CONNECTIONS.md`)*. |
| **Register Values** | Reset: `PWR->CR1.LPMS = 000b`, `SCB->SCR = 0`. Pre-Stop: `RCC->APB1ENR1[28] (PWREN) = 1`, `PWR->CR1.LPMS = 000b` (Stop 0), `SCB->SCR[2] (SLEEPDEEP) = 1`. Post-Wake: `SCB->SCR[2] = 0`, `RCC->CFGR.SWS = 11b` (PLL `170 MHz` or `160 MHz`), `g_wakeup_count >= 1`. |
| **Debugging Procedure** | Flash via `STM32_Programmer_CLI -c port=SWD -w firmware.bin 0x08000000 -v -rst` or OpenOCD (set `DBGMCU->CR |= DBGMCU_CR_DBG_STOP` only during GDB sessions, clear for true current check); inspect `p g_wakeup_count`, `p/x SCB->SCR`, `p/x RCC->CFGR`. |
| **Remaining Limitations** | Active SWD probe keeps debug domains powered and inflates `JP6` current above datasheet Stop 0 typ (`~160–350 µA` per DS12288 Table 31); measure true microamp draw standalone with DMM across `JP6` (`IDD`, NEVER `JP5`). |

### 12.1 6-State Granular Verification Breakdown

| Verification Stage | Status | Evidence / Scope |
|---|---|---|
| **Static review** | `PASS` | `PWR_CR1_LPMS_STOP0`, `SCB_SCR_SLEEPDEEP_Msk`, SysTick suspend/resume, `EXTI13` wakeup + `(void)EXTI->PR1; __DSB();`, and `170`/`160 MHz` PLL restore verified against RM0440 Ch. 5. |
| **Host test passed** | `N/A` | Hardware low-power Stop 0 silicon state machine. |
| **Cross-compilation passed** | `PASS` | Bare-metal CMSIS program compiles cleanly (`arm-none-eabi-gcc 14.3.1`, `-Wall -Wextra -Werror -O2`, 0 warnings / 0 errors). |
| **On-target upload succeeded** | `PENDING — USER CONNECTION REQUIRED` | Deferred because Stop 0 halts SWD unless `DBG_STOP` is set and requires physical `B1` press + `JP6` ammeter. |
| **Physical behavior verified** | `PENDING — USER CONNECTION REQUIRED` | Awaiting manual physical `B1` (`PC13`) wakeup & `170 MHz` PLL restore test on bench. |
| **Measurement verified** | `PENDING — USER CONNECTION REQUIRED` | Awaiting standalone µA DMM / PPK2 current measurement across `JP6` (`IDD`, NEVER `JP5`) (`HWSW-PWR-001`, `RT-PWR-002`). |

---

> ⬅️ **Prev Lab:** [Lab 13: IWDG Hardware Watchdog](../lab-13-iwdg-hardware-watchdog/README.md) | ⬆️ **Parent Stage:** [18-practical-laboratory (All 20 Labs)](../README.md) | **Next Lab:** [Lab 15: FreeRTOS Multi-Tasking](../lab-15-freertos-multitask-producer-consumer/README.md) ➡️
