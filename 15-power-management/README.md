# Stage 14: System Reliability, Watchdogs (`IWDG`/`WWDG`), `RTC`/`TAMP` & Low-Power Modes

> **Sequential Navigation (Step 15 of 17 — Canonical Acyclic Order):**  
> **Home:** [`START_HERE.md`](../START_HERE.md) | **Roadmap:** [`learning-sequence.md`](../01-master-concept-map/learning-sequence.md) | **Prev:** [Stage 14 (12) — Dual-Bank Flash, ECC & Bootloaders (`14-memory-and-storage/`)](../14-memory-and-storage/README.md) | **Next:** [Stage 16 (14) — FreeRTOS `ARM_CM4F` & Real-Time Scheduling (`16-rtos/`)](../16-rtos/README.md)  
> **Prerequisites:** [`07-rcc-and-clocks/`](../07-rcc-and-clocks/README.md), [`10-interrupts/` (`PERIPH-EXTI-*`)](../10-interrupts/README.md), [`09-timers/`](../09-timers/README.md), [`14-memory-and-storage/`](../14-memory-and-storage/README.md)

> **Directory:** `15-power-management/` | **Target Platform:** STM32G474RET6 on NUCLEO-G474RE (MB1367 — IDD Jumper **`JP6`**, NOT `JP5`!)  
> **Code Verification Notice:** All bare-metal C snippets in this module are **Statically Reviewed against `stm32g474xx.h`, RM0440, DS12288, and UM2505 (Not Tested on Physical Hardware)**.

---

## 1. Learning Objectives & Concept Scope

| Concept ID | Concept Name | Architectural Scope | Mastery Tier |
|---|---|---|---|
| `REL-PWR-001` (`STM32-PWR-002`, `RT-PWR-001`) | Exact STM32G474RE Power Modes: Run, Low-Power Run ($\le 2\text{ MHz}$), Sleep, LP-Sleep, **Stop 0 & Stop 1 (NO Stop 2!)**, Standby (`RRS` SRAM2 Retention), Shutdown | `[STM32G474RE Exact]` | Level 2 (Production) |
| `PROF-PWR-001` / `002` (`RT-PWR-002`) | NUCLEO-G474RE **`JP6`** IDD Current Profiling (NOT `JP5`!), Floating GPIO Analog Parking (`MODER=11b`), `RCC->*SMENR` Clock Gating, & Duty-Cycle Battery Math | `[STM32G474RE Exact]` | Level 2 (Production) |
| `PERIPH-RTC-001` | `RTC` Calendar/Alarms/Wakeup & `TAMP` (`BKP0R..31R` Backup Domain Persistence across Standby/Shutdown) | `[STM32G474RE Exact]` | Level 2 (Production) |
| `PERIPH-WDG-001` / `002` (`RT-WDG-001` / `002`, `REL-WDG-001`) | `IWDG` (RM0440 Ch. 35, `PR=100b=4`, `WINR` Window Mode) & `WWDG` (RM0440 Ch. 36, `WDGTB /1../128`, `EWI` IRQ 0) + **`DBGMCU->APB1FZR1` Freeze** | `[STM32G474RE Exact]` | Level 2 (Production) |
| `REL-BOR-001` (`STM32-PWR-003`) | Brown-Out Reset (`FLASH->OPTR.BOR_LEV 0..4`), Power Voltage Detector (`PVD`, EXTI16, `PVD_PVM_IRQn=1`), & PVM1/3/4 | `[STM32G474RE Exact]` | Level 2 (Production) |

---

## 2. Prerequisites & Curriculum Positioning

### Prerequisite Concept IDs (Corrected — Unified Stage Numbering!)
- **Stage 06 (`07-rcc-and-clocks/`):** `STM32-RCC-001`, `STM32-RCC-002`, `STM32-RCC-003` (170 MHz PLL re-init after Stop wakeup, `LSI`/`LSE`, `RCC->CSR` reset flags).
- **Stage 07 (`08-gpio/`):** `PERIPH-GPIO-001` (STM32G4 Analog mode `MODER=11b`, Active-HIGH `PC13` `B1`).
- **Stage 08 (`10-interrupts/`):** `PERIPH-EXTI-001`, `PERIPH-EXTI-003`, `RT-IRQ-001` (EXTI wakeup ISRs/events, `__WFI()`/`__WFE()`, `__DSB()`).
- **Stage 09 (`09-timers/`):** `PERIPH-LPTIM-001` (`LPTIM1` autonomous Stop 0/1 wakeup).

---

## 3. Core Technical Theory & Engineering Principles

### 3.1 Exact STM32G474RE Low-Power Mode Matrix (**NO Stop 2 on STM32G4!**) `[STM32G474RE Exact]`
> **CRITICAL HARDWARE FACT:** Unlike STM32L4/U5 microcontrollers, **STM32G474RE does NOT have a "Stop 2" mode!** Verified in `stm32g474xx.h` (`PWR_CR1_LPMS`) and RM0440: STM32G4 implements **Stop 0** (`LPMS=000b`) and **Stop 1** (`LPMS=001b`), followed directly by **Standby** (`011b`) and **Shutdown** (`1xxb`).

| Mode | Entry Mechanism (`PWR->CR1` + `SCB->SCR`) | Clocks Active | Retention | Wakeup Latency (DS12288) | Wakeup Sources |
|---|---|---|---|---|---|
| **Run** | Default (`170 MHz` Range 1 Boost) | All enabled | Full SRAM + Regs | — | — |
| **Low-Power Run** | `HCLK <= 2 MHz` + `PWR->CR1.LPR = 1` (poll `SR2.REGLPF==1`) | $\le 2\text{ MHz}$ | Full SRAM + Regs | — | Exit via `LPR=0` |
| **Sleep** | `SLEEPDEEP = 0`, `__WFI()` / `__WFE()` | CPU halted; peripherals ON | Full SRAM + Regs | **11–12 CPU cycles** (DS12288) | Any NVIC IRQ / Event |
| **Low-Power Sleep** | From LP-Run (`LPR=1`), `SLEEPDEEP = 0`, `__WFI()` | Peripherals $\le 2\text{ MHz}$ | Full SRAM + Regs | Fast (~µs) | Any NVIC IRQ / Event |
| **Stop 0** | `LPMS = 000b` (Main Regulator ON), `SLEEPDEEP = 1`, `__WFI()` | HSI/HSE/PLL OFF; LSI/LSE ON | **Full 128 KB SRAM + Regs** | ~5–10 µs (wakes on **HSI16**!) | EXTI, LPTIM1, LPUART1, I2C, RTC, PVD |
| **Stop 1** | `LPMS = 001b` (Low-Power Regulator ON), `SLEEPDEEP = 1`, `__WFI()` | HSI/HSE/PLL OFF; LSI/LSE ON | **Full 128 KB SRAM + Regs** | ~10–15 µs (wakes on **HSI16**!) | EXTI, LPTIM1, LPUART1, I2C, RTC, PVD |
| **Standby** | `LPMS = 011b`, `SLEEPDEEP = 1`, `__WFI()` | Main $V_{\text{CORE}}$ **OFF** | Optional **16 KB SRAM2 (`PWR->CR3.RRS=1`)** + `TAMP->BKPxR` | Reset boot (~µs–ms) | `WKUP1..5` (`PA0`, `PC13`, `PA2`, `PC5`), RTC, TAMP, IWDG, NRST |
| **Shutdown** | `LPMS = 1xxb`, `SLEEPDEEP = 1`, `__WFI()` | $V_{\text{CORE}}$ & **BOR OFF** | `TAMP->BKPxR` only (LSE optional) | Full POR boot | `WKUP1..5`, RTC, TAMP, NRST |

### 3.2 Mandatory Stop-Mode Entry & Exit Hygiene (`PWREN` & `SLEEPDEEP`)
1. **ALWAYS enable `RCC->APB1ENR1 |= RCC_APB1ENR1_PWREN;`** (+ dummy read) before writing `PWR->CR1`; otherwise `LPMS` writes are silently ignored!
2. **Park ALL Unused GPIOs to Analog Mode (`MODER = 11b`, `PUPDR = 00b`):** A single floating digital input near $V_{\text{DD}}/2$ draws $100–500\ \mu\text{A}$ of Schmitt-trigger shoot-through current — dwarfing Stop 1's entire silicon current!
3. **Disable SysTick (`SysTick->CTRL &= ~SysTick_CTRL_TICKINT_Msk`)** before Sleep/Stop if you do not want a 1 ms tick immediately waking the CPU; clear stale `EXTI->PR1` flags before `__WFI()`.
4. **Post-Stop Wakeup Cleanup:** Immediately after `__WFI()` returns from Stop 0/1:
   - Clear **`SCB->SCR &= ~SCB_SCR_SLEEPDEEP_Msk;`** (otherwise the next accidental `__WFI()` re-enters Stop!).
   - Re-run **`SystemClock_Config_170MHz_HSE()`** because Stop mode automatically disables HSE/PLL and wakes on **16 MHz HSI16**!

### 3.3 NUCLEO-G474RE Hardware Current Profiling (`JP6`, NOT `JP5`!) `[STM32G474RE Exact]`
Per **ST UM2505 (MB1367 §6.4)**:
- **`JP6`** is the MCU **`IDD`** current measurement jumper! Remove `JP6` and insert a calibrated ammeter, Nordic PPK2, or $1\ \Omega$ shunt + differential oscilloscope probe to measure true STM32G474RE $I_{\text{DD}}$.
- **`JP5` is the 5V Power-Source Selector** (`5V_USB_STLK` / `5V_VIN` / `E5V`) — **NEVER** put an ammeter across `JP5`!
- **Back-Powering & Debug Current Trap:** Disconnect ST-LINK SWD (`CN4` / debugger session) and clear `DBGMCU->CR` stop-debug bits during microamp Stop/Standby tests; otherwise ST-LINK I/O pins and internal debug clocks add `500 µA–2 mA` of phantom current! Also beware multimeter burden voltage on µA ranges causing brown-out during 30 mA active wakeups (use a PPK2 or autoranging shunt!).

### 3.4 `RTC` & `TAMP` (Backup Domain & Wakeup) `[STM32G474RE Exact]`
STM32G4 separates **`RTC`** (`0x40002800`) and **`TAMP`** (`0x40002400`, holding **32 × 32-bit Backup Registers `BKP0R..BKP31R`** at `0x40002500..0x4000257C`):
- Write access requires `RCC->APB1ENR1 |= RCC_APB1ENR1_PWREN | RCC_APB1ENR1_RTCAPBEN;` + **`PWR->CR1 |= PWR_CR1_DBP;`**.
- Clocked by onboard 32.768 kHz `LSE` crystal `X2` (`PREDIV_A = 127`, `PREDIV_S = 255` $\to 1\text{ Hz}$); RTC Alarm (`RTC_Alarm_IRQn = 41`) and Periodic Wakeup Timer (`RTC_WKUP_IRQn = 3`) wake the MCU cleanly from Stop 0/1, Standby, and Shutdown!

### 3.5 Hardware Watchdogs: `IWDG`, `WWDG` & `DBGMCU` Freeze `[STM32G474RE Exact]`
1. **Independent Watchdog (`IWDG` @ `0x40003000`, RM0440 Ch. 35):**
   - 12-bit down-counter clocked by dedicated **~32 kHz LSI** ($\pm 5\%$ tolerance: `30.4–33.6 kHz`) — survives HSE/PLL failure! Once started (`KR = 0xCCCC`), **cannot be stopped except by reset**.
   - Prescaler `IWDG->PR[2:0]`: `000b=/4`, `001b=/8`, `010b=/16`, `011b=/32`, **`100b (4) = /64`**, `101b=/128`, `110b=/256`.
   - **STM32G4 Window Mode (`IWDG->WINR @ 0x10`):** Resets if refreshed *too early* (`counter > WINR`), catching runaway infinite kick loops!
2. **Window Watchdog (`WWDG` @ `0x40002C00`, RM0440 Ch. 36):**
   - 7-bit down-counter (`0x7F -> 0x40`, reset at `0x3F`) clocked by `PCLK1 / 4096 / 2^WDGTB` where STM32G4 extends **`WDGTB[2:0]` to `/1 .. /128`** (`0..7`).
   - Enforces BOTH minimum refresh delay (`CNT <= W[6:0]`) and maximum expiry (`CNT >= 0x40`), plus **Early Wakeup Interrupt (`EWI`, `WWDG_IRQn = 0`)** firing at `CNT == 0x40` — giving firmware one final ISR to save crash context into `TAMP->BKP0R` before hardware reset!
3. **Mandatory Debug Freeze (`DBGMCU->APB1FZR1 @ 0xE0042008`):**
   - Without freezing watchdogs during GDB breakpoints, stepping through code triggers continuous watchdog resets! Set:
     ```c
     DBGMCU->APB1FZR1 |= DBGMCU_APB1FZR1_DBG_IWDG_STOP | DBGMCU_APB1FZR1_DBG_WWDG_STOP;
     ```

### 3.6 Brown-Out Reset (`BOR`) & Power Voltage Detector (`PVD`)
- **`FLASH->OPTR.BOR_LEV[2:0]`:** 5 hardware reset thresholds (`BOR_LEV_0` ~1.7 V up to `BOR_LEV_4` ~2.8 V). Keeps the CPU cleanly in reset during supply brown-out before Flash/SRAM corrupts!
- **`PWR->CR2` (`PVDE`, `PLS[3:1]`):** Programmable early-warning supply comparator routed to **EXTI Line 16 (`PVD_PVM_IRQn = 1`)** — triggers an ISR as $V_{\text{DD}}$ crosses e.g. `2.9 V` so firmware can disable power actuators and flush state to `TAMP->BKPxR` milliseconds before `BOR` triggers!

---

## 4. Core Terminology & Acronyms

| Term | Engineering Definition |
|---|---|
| **Stop 0 / Stop 1** | Deep-sleep modes preserving full 128 KB SRAM & registers with Main Regulator (Stop 0) or Low-Power Regulator (Stop 1). **STM32G4 has NO Stop 2.** |
| **`RRS` (SRAM2 Retention)** | `PWR->CR3` bit 8 preserving 16 KB `SRAM2` (`0x20014000`) contents across Standby power-down. |
| **`JP6` (`IDD`) vs `JP5` (`5V_SEL`)** | NUCLEO-G474RE (UM2505) MCU current measurement jumper (`JP6`) vs 5V rail selector (`JP5`). |
| **`IWDG` / `WWDG` / `EWI`** | LSI-clocked Independent Watchdog (with `WINR`) / PCLK1-clocked Window Watchdog with Early Wakeup Interrupt (`IRQ 0`). |
| **`BOR` / `PVD`** | Hardware Brown-Out Reset (`FLASH->OPTR`) / Early-Warning Power Voltage Detector (`PWR->CR2` + EXTI16). |

---

## 5. Hardware Register Architecture & Bitfield Reference

Verified directly against `stm32g474xx.h`:

| Register | Base + Offset | Key Bitfields |
|---|---|---|
| **`PWR->CR1`** | `0x40007000` | `LPMS[2:0]` (`000=Stop0`, `001=Stop1`, `011=Standby`, `1xx=Shutdown`), `DBP` (b8), `VOS[10:9]`, `LPR` (b14) |
| **`PWR->CR2` / `CR3`** | `+0x04` / `+0x08` | `PVDE`, `PLS[3:1]`, `PVME1/3/4`; `EWUP1..5`, **`RRS` (b8)**, `UCPD_STDBY` (b13), `UCPD_DBDIS` (b14) |
| **`PWR->SR1` / `SR2` / `SCR`** | `+0x10 / 14 / 18` | `WUF1..5`, `SBF` (b8); `REGLPF` (b9), `VOSF` (b10), `PVDO` (b11); `CWUF1..5`, `CSBF` |
| **`IWDG` (`0x40003000`)** | `KR@0x00, PR@0x04, RLR@0x08, SR@0x0C, WINR@0x10` | `KR`: `0xCCCC` start, `0x5555` unlock, `0xAAAA` reload; **`PR = 0b100 (4)` = `/64`** |
| **`WWDG` (`0x40002C00`)** | `CR@0x00, CFR@0x04, SR@0x08` | `CR`: `T[6:0]`, `WDGA` (b7); `CFR`: `W[6:0]`, `EWI` (b9), `WDGTB[13:11]` (`/1../128`); `SR`: `EWIF` (b0) |
| **`DBGMCU->APB1FZR1`** | `0xE0042008` | **`DBG_WWDG_STOP` (b11)**, **`DBG_IWDG_STOP` (b12)**, `DBG_RTC_STOP` (b10), `DBG_TIM2..7_STOP` |

---

## 6. Engineering Mathematics & Worked Formulas

### 6.1 `IWDG` Timeout Equation (Accounting for $\pm 5\%$ LSI Drift!)
$$t_{\text{IWDG}} = \frac{4 \times 2^{\text{PR}} \times (\text{RLR} + 1)}{f_{\text{LSI}}}$$
For `PR = 4` (`100b` $\to$ divider `/64`) and `RLR = 499` (`500` ticks):
- Nominal ($f_{\text{LSI}} = 32.0\text{ kHz}$): $t_{\text{nom}} = \frac{64 \times 500}{32{,}000} = \mathbf{1.000\text{ s}}$.
- Worst-case fast LSI ($33.6\text{ kHz}$): $t_{\min} = \mathbf{0.952\text{ s}}$ $\implies$ design firmware refresh interval $\le 750\text{ ms}$!

### 6.2 `WWDG` Window Timing Equations (STM32G4 `WDGTB = 0..7`)
$$t_{\text{WWDG,max}} = \frac{4096 \times 2^{\text{WDGTB}} \times (T[5:0] + 1)}{f_{\text{PCLK1}}}, \qquad t_{\text{WWDG,min}} = \frac{4096 \times 2^{\text{WDGTB}} \times (T[6:0] - W[6:0])}{f_{\text{PCLK1}}}$$
At $f_{\text{PCLK1}} = 170\text{ MHz}$, `WDGTB = 7` (`/128`): 1 tick $= \frac{4096 \times 128}{170\text{ MHz}} = 3.084\text{ ms}$. With `T = 0x7F` (`64` ticks down to `0x3F`) and `W = 0x5F`:
- Minimum refresh wait: $(127 - 95) \times 3.084\text{ ms} = \mathbf{98.7\text{ ms}}$; Maximum timeout: $64 \times 3.084\text{ ms} = \mathbf{197.4\text{ ms}}$!

### 6.3 Duty-Cycle Average Current & Battery Life Estimation
$$I_{\text{avg}} = \frac{I_{\text{run}}\,t_{\text{run}} + I_{\text{stop}}\,t_{\text{stop}}}{T}, \qquad L_{\text{hours}} = \frac{C_{\text{battery}}\,(\text{mA}\cdot\text{h}) \times 0.85}{I_{\text{avg}}\,(\text{mA})}$$
Example: 1 Hz sensor waking for $t_{\text{run}} = 5\text{ ms}$ at $25\text{ mA}$ and sleeping $995\text{ ms}$ in Stop 1 at $0.15\text{ mA}$:
$$I_{\text{avg}} = \frac{25(0.005) + 0.15(0.995)}{1.0} = 0.274\text{ mA} \implies \text{1000 mAh LiPo} \approx \mathbf{3{,}100\text{ hours}\ (129\text{ days})}$$

---

## 7. Official Documentation & Standards References

| Document | Exact Sections |
|---|---|
| **ST RM0440** | Ch. 5/6 (*PWR* — Stop 0/1, Standby, Shutdown, PVD), Ch. 34 (*RTC*), Ch. 35 (*IWDG*), Ch. 36 (*WWDG*), Ch. 38 (*TAMP*), Ch. 47 (*DBGMCU*) |
| **ST DS12288** | §5.3.5 (*Supply Current Characteristics*: Run, Sleep 11–12 cycles wakeup, Stop 0/1, Standby, Shutdown) |
| **ST UM2505** | §6.3 (`JP5` 5V Select), **§6.4 (`JP6` IDD Current Measurement)**, §6.6 (`B1` PC13 Active-HIGH) |
| **ST AN4899** | *STM32 GPIO Settings & Low-Power Consumption* |

---

## 8. Production-Grade Bare-Metal C Code Examples

> **Verification Status:** Statically Reviewed against `stm32g474xx.h`, RM0440, and UM2505 (Not Tested on Physical Hardware).

### 8.1 Clean Stop 1 Entry, Active-HIGH `PC13` EXTI Wakeup & 170 MHz Clock Recovery
```c
#include "stm32g474xx.h"
#include <stdint.h>

extern void SystemClock_Config_170MHz_HSE(void);

void Enter_Stop1_And_Recover(void)
{
    /* 1. MANDATORY: Enable PWR clock before touching PWR->CR1! */
    RCC->APB1ENR1 |= RCC_APB1ENR1_PWREN;
    (void)RCC->APB1ENR1;

    /* 2. Suspend SysTick IRQ so 1 ms tick does not immediately wake Stop mode */
    SysTick->CTRL &= ~SysTick_CTRL_TICKINT_Msk;

    /* 3. Select Stop 1 mode (LPMS = 001b; Stop 0 = 000b; NO Stop 2 on G4!) */
    PWR->CR1 = (PWR->CR1 & ~PWR_CR1_LPMS_Msk) | PWR_CR1_LPMS_STOP1;

    /* 4. Clear stale EXTI13 flag before entering Stop */
    EXTI->PR1 = EXTI_PR1_PIF13;

    /* 5. Set SLEEPDEEP, synchronize pipeline, wait for EXTI13 wakeup interrupt */
    SCB->SCR |= SCB_SCR_SLEEPDEEP_Msk;
    __DSB();
    __WFI();
    __ISB();

    /* 6. POST-WAKEUP HYGIENE: Clear SLEEPDEEP & restore 170 MHz PLL (Stop wakes on HSI16!) */
    SCB->SCR &= ~SCB_SCR_SLEEPDEEP_Msk;
    SystemClock_Config_170MHz_HSE();

    /* 7. Re-enable SysTick IRQ */
    SysTick->CTRL |= SysTick_CTRL_TICKINT_Msk;
}
```

### 8.2 `IWDG` Init (`PR = 100b = /64`, `DBGMCU` Freeze) & `WWDG` `EWI` Crash Logger
```c
void IWDG_Init_1s_DebugSafe(void)
{
    /* Freeze IWDG & WWDG when CPU is halted in GDB! */
    DBGMCU->APB1FZR1 |= DBGMCU_APB1FZR1_DBG_IWDG_STOP | DBGMCU_APB1FZR1_DBG_WWDG_STOP;

    IWDG->KR  = 0x0000CCCCUL; /* Start IWDG (auto-enables LSI) */
    IWDG->KR  = 0x00005555UL; /* Unlock PR, RLR, WINR */
    IWDG->PR  = 4U;           /* 100b = /64 prescaler (NOT 010b!) */
    IWDG->RLR = 499U;         /* 500 ticks @ 500 Hz = ~1.000 s nominal */
    while (IWDG->SR != 0U) {} /* Wait for LSI clock-domain sync (PVU/RVU/WVU == 0) */
    IWDG->KR  = 0x0000AAAAUL; /* Initial refresh */
}

void WWDG_IRQHandler(void)
{
    if ((WWDG->SR & WWDG_SR_EWIF) != 0U) {
        WWDG->SR = 0U; /* Clear EWI flag */
        /* Emergency crash stamp into TAMP->BKP0R before imminent WWDG reset at T==0x3F */
        RCC->APB1ENR1 |= RCC_APB1ENR1_PWREN | RCC_APB1ENR1_RTCAPBEN;
        PWR->CR1      |= PWR_CR1_DBP;
        TAMP->BKP0R    = 0xBAD00001UL;
        __DSB();
    }
}
```

---

## 9. Practical Laboratory Experiments & Debugging Checkpoints

- **Lab 13:** [`lab-13-iwdg-hardware-watchdog`](../18-practical-laboratory/lab-13-iwdg-hardware-watchdog/README.md) — Active-HIGH `PC13` digital input (`MODER=00b`), `IWDG` `PR=4`, `DBGMCU` freeze, `RCC->CSR.IWDGRSTF` diagnostics.
- **Lab 14:** [`lab-14-low-power-stop-mode-wakeup`](../18-practical-laboratory/lab-14-low-power-stop-mode-wakeup/README.md) — `PWREN`, `JP6` IDD profiling, `EXTI15_10_IRQHandler`, `SLEEPDEEP=0`, 170 MHz recovery.
- **Hardware Profiling Checkpoint:** Remove **`JP6`** on NUCLEO-G474RE (never `JP5`!), disconnect ST-LINK SWD, and verify Run (`~25–35 mA`) $\to$ Stop 0 $\to$ Stop 1 (`< 200 µA`) $\to$ Standby (`< 5 µA`) matching DS12288 §5.3.5.

---

## 10. Common Engineering Mistakes, Hardware Traps & Silicon Errata (ES0430)

1. **Measuring $I_{\text{DD}}$ Across `JP5` Instead of `JP6`:** `JP5` is the 5V source jumper; **`JP6`** is the MCU `IDD` jumper (UM2505).
2. **Claiming "Stop 2" Mode Exists on STM32G4:** STM32G4 only has **Stop 0** and **Stop 1** (`LPMS = 000b / 001b`).
3. **Forgetting `RCC->APB1ENR1 |= RCC_APB1ENR1_PWREN`:** `PWR->CR1` writes silently fail, and the MCU enters Sleep instead of Stop!
4. **Leaving `PC13` in Analog Reset Mode (`MODER = 11b`) or Omitting `EXTI15_10_IRQHandler`:** Permanently locks the MCU in Stop mode or jumps to `Default_Handler` on wakeup.
5. **Forgetting `DBGMCU->APB1FZR1` Freeze Bits:** GDB single-stepping triggers continuous IWDG/WWDG resets.
6. **Writing `IWDG->PR = 0b010` Thinking It Is `/64`:** `0b010 = 2` is `/16` (4× too fast!); `/64` is **`0b100 = 4`**.

---

## 11. Progressive Coding Exercises & Design Challenges

1. **[Level 2]** Configure `IWDG->WINR = 250U` with `RLR = 499U` (`PR = 4`), prove that kicking at 100 ms triggers an immediate window reset (`RCC_CSR_IWDGRSTF == 1`), while kicking at 650 ms keeps the system alive.
2. **[Level 2]** Configure `WWDG` with `EWI` (`WWDG_IRQn = 0`) logging a diagnostic word into `TAMP->BKP0R`, let it expire, and read `TAMP->BKP0R` + `RCC->CSR.WWDGRSTF` on reboot.
3. **[Level 2]** Measure `JP6` current in Stop 1 with 10 pins configured as floating Digital Inputs vs parked in Analog Mode (`MODER=11b`, `PUPDR=00b`) and record the microamp reduction.
4. **[Level 3]** Enter Standby mode with **`PWR->CR3 |= PWR_CR3_RRS`** preserving a sentinel in `SRAM2` (`0x20014000`), wake via `RTC` Alarm A (`+5 s`), and verify `PWR->SR1.SBF == 1` and intact `SRAM2` data!

---

## 12. Self-Assessment Mastery Questions

1. **Q:** Why does UART output become garbled immediately after waking from Stop 1 if you do not re-configure RCC?  
   *Answer Check:* Stop mode shuts down HSE and the Main PLL; STM32G4 wakes up clocked by **16 MHz HSI16** instead of 170 MHz, throwing off `USART2->BRR` by 10.625×.
2. **Q:** Which RAM regions retain data in Stop 1 vs Standby (`RRS = 1`) vs Shutdown?  
   *Answer Check:* Stop 1: all 128 KB (`SRAM1+SRAM2+CCM`) + peripheral registers. Standby (`RRS=1`): 16 KB `SRAM2` + `TAMP->BKPxR`. Shutdown: `TAMP->BKPxR` only.
3. **Q:** Why can `WWDG` detect faults that `IWDG` (without `WINR`) cannot, and vice versa?  
   *Answer Check:* `WWDG` enforces a tight lower/upper refresh window and fires `EWI` before reset, but stops if `PCLK1`/HSE fails; `IWDG` runs off an independent 32 kHz `LSI` oscillator so it rescues even a dead main clock!

---

## 13. Measurable Mastery & Completion Criteria

| Concept ID | Pass / Fail Verification Criterion | Measurement Tool |
|---|---|---|
| `REL-PWR-001` / `PROF-PWR-001` | Measure `JP6` $I_{\text{DD}}$ in Run, Stop 0, Stop 1 (`< 200 µA`), and Standby (`RRS=1` preserving `SRAM2`) within $\pm 15\%$ of DS12288 | Ammeter / PPK2 on `JP6` |
| `PERIPH-RTC-001` | Wake from Standby after 5.00 s via `RTC_Alarm_IRQn = 41` with intact `TAMP->BKP0R` | Oscilloscope / GDB |
| `PERIPH-WDG-001` / `002` | Verify `IWDG` (`PR=4`, `WINR`), `WWDG` early/late reset + `EWI` backup log, `RCC->CSR` flags, and `DBGMCU` breakpoint freeze | GDB + UART Log |
| `REL-BOR-001` | Verify `FLASH->OPTR.BOR_LEV >= 3` and `PVD` (EXTI16) ISR saving state to `TAMP->BKP0R` prior to supply collapse | Bench PSU + GDB |

---

> **Sequential Navigation (Step 15 of 17):**  
> **Prev:** [Stage 14 (12) — Dual-Bank Flash, ECC & Bootloaders (`14-memory-and-storage/`)](../14-memory-and-storage/README.md) | **Home:** [`START_HERE.md`](../START_HERE.md) | **Progress:** [`LEARNER_PROGRESS_DASHBOARD.md`](../LEARNERS/LEARNER_PROGRESS_DASHBOARD.md) | **Next:** [Stage 16 (14) — FreeRTOS `ARM_CM4F` & Real-Time Scheduling (`16-rtos/`)](../16-rtos/README.md)
