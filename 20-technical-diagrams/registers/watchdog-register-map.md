# Watchdog Peripherals (IWDG & WWDG) Register Map

**Target MCU:** STM32G474RE  
**IWDG Base:** `0x40003000` (`APB1PERIPH_BASE + 0x3000UL`)  
**WWDG Base:** `0x40002C00` (`APB1PERIPH_BASE + 0x2C00UL`)  
**Reference Manual:** RM0440, Chapter 30 (*IWDG*) & Chapter 31 (*WWDG*)

---

### 1. IWDG (Independent Watchdog) Architecture

The Independent Watchdog is clocked from its own dedicated Low-Speed Internal (LSI) RC oscillator running at $\approx 32\ \text{kHz}$. It remains active even if the main CPU crystal or PLL completely fails, providing a fail-safe reset against software deadlocks.

| Register | Offset | Reset Value | Description |
|---|---|---|---|
| `IWDG_KR` | `0x00` | `0x00000000` | Key register (Write key values to unlock, reload, or start) |
| `IWDG_PR` | `0x04` | `0x00000000` | Prescaler register (Divides 32 kHz LSI clock by 4 up to 256) |
| `IWDG_RLR`| `0x08` | `0x00000FFF` | Reload register (12-bit countdown reload value: 0 to 4095) |
| `IWDG_SR` | `0x0C` | `0x00000000` | Status register (PVU prescaler update busy, RVU reload update busy) |

#### Special Key Register (`IWDG_KR`) Command Codes
- **`0xCCCC`:** Starts the IWDG timer (once started, it **cannot be disabled** except by a system reset!).
- **`0x5555`:** Disables register write protection on `IWDG_PR` and `IWDG_RLR`.
- **`0xAAAA`:** "Kicks" (refreshes) the watchdog, reloading the counter from `IWDG_RLR`.

#### Timeout Calculation Formula:

$$T_{\text{TIMEOUT}} = \frac{(\text{RLR} + 1) \times \text{Prescaler}}{f_{\text{LSI}}}$$

**Worked Example for 1-Second Timeout:**
- $f_{\text{LSI}} \approx 32,000\ \text{Hz}$.
- Select Prescaler = 64 (`PR = 010b = 4`). Clock to counter = $32,000 / 64 = 500\ \text{Hz}$ ($2\ \text{ms}$ per tick).
- Target: $1.0\ \text{second} \rightarrow \text{Count} = 1000\ \text{ms} / 2\ \text{ms} = 500$ ticks.
- Set $\text{RLR} = 500 - 1 = 499$ (`0x01F3`).

---

### 2. CMSIS Code Example: IWDG Initialization & Refresh
```c
void IWDG_Init_1Second(void) {
    /* 1. Start IWDG engine */
    IWDG->KR = 0xCCCC;

    /* 2. Unlock Write Protection on PR and RLR */
    IWDG->KR = 0x5555;

    /* 3. Set Prescaler to /64 (PR = 4) */
    IWDG->PR = 4U;

    /* 4. Set Reload Value to 499 for 1-second timeout */
    IWDG->RLR = 499U;

    /* 5. Wait for register updates to take effect in SR */
    while (IWDG->SR != 0);

    /* 6. Initial Kick / Reload */
    IWDG->KR = 0xAAAA;
}

void IWDG_Refresh(void) {
    /* Refresh watchdog counter to prevent system reset */
    IWDG->KR = 0xAAAA;
}
```
