# Watchdog Peripherals (IWDG & WWDG) Register Map

**Target MCU:** STM32G474RE (LQFP64)  
**IWDG Base:** `0x40003000` (`APB1PERIPH_BASE + 0x3000UL`) — LSI ~32 kHz clock (autonomous)  
**WWDG Base:** `0x40002C00` (`APB1PERIPH_BASE + 0x2C00UL`) — PCLK1 clock (`RCC->APB1ENR1.WWDGEN`, bit 11)  
**DBGMCU Base:** `0xE0042000` (`DBGMCU->APB1FZR1` freezes watchdogs during GDB halt)  
**Reference Manual:** RM0440, **Chapter 35** (*Independent watchdog (IWDG)*) & **Chapter 36** (*System window watchdog (WWDG)*)  
**Concept IDs:** `PERIPH-WDG-001`, `PERIPH-WDG-002`, `REL-WDG-001`, `STM32-DBGMCU-001`  
**Verification Status:** Every register offset and bit position verified directly against `stm32g474xx.h`; code snippets Statically Reviewed (Not Tested on Physical Hardware).

---

### 1. IWDG (Independent Watchdog) Register Map (`IWDG_TypeDef` @ `0x40003000`)

Clocked by the dedicated ~32 kHz **LSI** RC oscillator (survives HSE/PLL failure and Stop/Standby modes). STM32G4 adds a **Window register (`IWDG_WINR`)** preventing premature refreshes.

| Register | Offset | Reset Value | Access | Description |
|---|---|---|---|---|
| `IWDG_KR` | `0x000` | `0x00000000` | WO | Key register (`0xCCCC`=Start, `0x5555`=Unlock `PR`/`RLR`/`WINR`, `0xAAAA`=Reload/Kick) |
| `IWDG_PR` | `0x004` | `0x00000000` | RW | Prescaler (`PR[2:0]`: `000`=`/4`, `001`=`/8`, `010`=`/16`, `011`=`/32`, **`100`=`/64`**, `101`=`/128`, `110`/`111`=`/256`) |
| `IWDG_RLR` | `0x008` | `0x00000FFF` | RW | 12-bit reload counter (`0..4095`) |
| `IWDG_SR` | `0x00C` | `0x00000000` | RO | Status flags (`PVU` bit 0, `RVU` bit 1, **`WVU` bit 2** — poll until `SR == 0`) |
| `IWDG_WINR` | `0x010` | `0x00000FFF` | RW | 12-bit window threshold (Refresh while `CNT > WINR` triggers reset! Writing `WINR` also reloads counter.) |

#### IWDG Timeout Formula & Worked Example (1.0 s)
$$T_{\text{IWDG}} = \frac{(\text{RLR} + 1) \times \text{Prescaler}}{f_{\text{LSI}}}$$
- With $f_{\text{LSI}} \approx 32{,}000\ \text{Hz}$ and Prescaler = `/64` (**`PR = 100b = 4`**): tick period = $64 / 32{,}000 = 2.0\ \text{ms}$.
- For $T_{\text{IWDG}} = 1.0\ \text{s}$: $\text{RLR} = (1000 / 2) - 1 = 499$ (`0x01F3`).
- Optional window: Setting `WINR = 250` rejects refreshes occurring earlier than ~500 ms!

---

### 2. WWDG (Window Watchdog) Register Map (`WWDG_TypeDef` @ `0x40002C00`)

Clocked by `PCLK1 / 4096 / 2^WDGTB`. Resets the MCU if: (a) counter decrements from `0x40` to `0x3F`, **OR** (b) software refreshes `WWDG_CR` while `T[6:0] > W[6:0]`. Triggers **`WWDG_IRQn = 0`** (`EWI`) when `T == 0x40` to allow emergency safe-state logging before reset!

| Register | Offset | Reset Value | Access | Description |
|---|---|---|---|---|
| `WWDG_CR` | `0x000` | `0x0000007F` | RW | Control (`T[6:0]` 7-bit down-counter `0x40..0x7F`; `WDGA` bit 7 activation — write-1-only until reset) |
| `WWDG_CFR` | `0x004` | `0x0000007F` | RW | Config (`W[6:0]` window value, `EWI` bit 9 Early Wakeup IRQ enable, **`WDGTB[13:11]`** prescaler `/1`..`/128`!) |
| `WWDG_SR` | `0x008` | `0x00000000` | rc_w0 | Status (`EWIF` bit 0; write `0` to clear) |

> **STM32G4 Enhancement:** `WWDG_CFR.WDGTB[13:11]` is **3 bits wide** on STM32G4 (`000`=`/1`, `001`=`/2`, `010`=`/4`, `011`=`/8`, `100`=`/16`, `101`=`/32`, `110`=`/64`, **`111`=`/128`** — older STM32F4 only supported `/1`..`/8`!).

$$T_{\text{WWDG}} = \frac{4096 \times 2^{\text{WDGTB}} \times (T[5:0] + 1)}{f_{\text{PCLK1}}}$$
- Example at $f_{\text{PCLK1}} = 170\ \text{MHz}$, `WDGTB = 7` (`/128`), `T = 0x7F` (`T[5:0] = 63`):
  $$T_{\text{max}} = \frac{4096 \times 128 \times 64}{170{,}000{,}000} \approx 197.4\ \text{ms}$$

---

### 3. GDB Debug Freeze (`DBGMCU_APB1FZR1` @ `0xE0042008`)

Without freezing watchdogs during debug halts, hitting a GDB breakpoint triggers a watchdog reset. Always enable debug freeze during development:
- **`DBGMCU_APB1FZR1_DBG_WWDG_STOP`** (Bit 11): Halts WWDG counter when Cortex-M4 core is halted.
- **`DBGMCU_APB1FZR1_DBG_IWDG_STOP`** (Bit 12): Halts IWDG counter when Cortex-M4 core is halted.

---

### 4. CMSIS Code Examples

> **Status:** Statically Reviewed (Not Tested on Physical Hardware).

```c
#include "stm32g474xx.h"

void IWDG_Init_1s_Windowed(void) {
    /* Optional (dev builds): Freeze IWDG when halted in GDB */
    DBGMCU->APB1FZR1 |= DBGMCU_APB1FZR1_DBG_IWDG_STOP;

    /* 1. Start IWDG (auto-enables LSI) & unlock PR/RLR/WINR */
    IWDG->KR = 0x0000CCCCU;
    IWDG->KR = 0x00005555U;

    /* 2. Prescaler /64 (PR = 100b = 4), Reload = 499 (~1.0 s), Window = 4095 (disabled) */
    IWDG->PR   = IWDG_PR_PR_2; /* 4U = 0b100 = /64 */
    IWDG->RLR  = 499U;
    while (IWDG->SR != 0U) { } /* Wait for PVU, RVU, WVU to clear */

    /* Writing WINR (or KR = 0xAAAA) refreshes counter and locks registers */
    IWDG->WINR = 0x0FFFU;
}

void WWDG_Init_With_EWI(void) {
    DBGMCU->APB1FZR1 |= DBGMCU_APB1FZR1_DBG_WWDG_STOP;
    RCC->APB1ENR1    |= RCC_APB1ENR1_WWDGEN;
    (void)RCC->APB1ENR1;

    /* WDGTB = 7 (/128), Window W = 0x60 (~101 ms open window), enable EWI IRQ */
    WWDG->CFR = (7U << WWDG_CFR_WDGTB_Pos) | WWDG_CFR_EWI | 0x60U;
    WWDG->SR  = 0U; /* Clear EWIF (rc_w0) */

    NVIC_SetPriority(WWDG_IRQn, 0U);
    NVIC_EnableIRQ(WWDG_IRQn);

    /* Activate WWDG with initial counter T = 0x7F (~197 ms timeout @ 170 MHz) */
    WWDG->CR = WWDG_CR_WDGA | 0x7FU;
}

void WWDG_IRQHandler(void) {
    if (WWDG->SR & WWDG_SR_EWIF) {
        WWDG->SR = 0U; /* Log emergency state to TAMP->BKPxR before reset */
        __DSB();
    }
}
```
