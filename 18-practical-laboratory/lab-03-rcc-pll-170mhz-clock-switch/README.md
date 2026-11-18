# Lab 03: RCC PLL 170 MHz Clock Switch in Range 1 Boost Mode

> ⬅️ **Prev Lab:** [Lab 02: GPIO Pushbutton Debouncing](../lab-02-gpio-pushbutton-debouncing/README.md) | ⬆️ **Parent Stage:** [18-practical-laboratory (All 20 Labs)](../README.md) | **Next Lab:** [Lab 04: SysTick Accurate Delay](../lab-04-systick-accurate-delay-generator/README.md) ➡️

> **Verification Status:** `ONBOARD HARDWARE TESTED` (`arm-none-eabi-gcc 14.3.1`, `-Wall -Wextra -Werror -O2`, 0 warnings) | **Hardware Evidence:** NUCLEO-G474RE `onboard_selftest` Test 3A (`160 MHz`: `1,599,997` cyc/10 ms) & Test 3B (`170 MHz`: `1,699,989` cyc/10 ms), `PWR_CR5=0x0`, `FLASH_ACR=0x00040704`

> **Clock Configuration Profile:** **Primary = `170 MHz Range 1 Boost` (`M=4, N=85, R=2`, `R1MODE=0`, `4WS`, `MCO/16 = 10.625 MHz`)** | **Alternate = `160 MHz Range 1 Boost` (`M=4, N=80, R=2`, `R1MODE=0`, `4WS`, `MCO/16 = 10.000 MHz`)** | **Reusable Drivers:** [`23-code-library/Drivers/clock_g4.h`](../../23-code-library/Drivers/clock_g4.h) (`Clock_Init_170MHz()`, `Clock_Init_160MHz()`, `Clock_Init_170MHz_HSE24()`, `Clock_GetSysClkHz()`) & [`gpio_g4.h`](../../23-code-library/Drivers/gpio_g4.h).

---

## 1. Objective & Concept IDs

Safely transition the STM32G474RE from its reset `16 MHz HSI16` clock to its maximum rated **`170 MHz SYSCLK`** in **Voltage Scaling Range 1 Boost Mode** by following the exact mandatory **RM0440 §6.1.5** sequence: `PWR_CR1.VOS` stabilization (`PWR_SR2.VOSF`), `PWR_CR5.R1MODE = 0`, 4 Flash Wait States, PLL lock, and the mandatory intermediate `HPRE = /2` (85 MHz) $\to \ge 1\ \mu\text{s} \to$ `HPRE = /1` (170 MHz) ramp.

- **Prerequisites:** [Lab 01 (Bare-Metal GPIO & `AF`)](../lab-01-baremetal-gpio-led-blinky/README.md), [Stage 06 (STM32 Architecture)](../../06-stm32-fundamentals/README.md), [Stage 07 (RCC & Clock Tree)](../../07-rcc-and-clocks/README.md), and [Stage 15 (PWR Voltage Scaling)](../../15-power-management/README.md)
- **Target Concept IDs:** `STM32-RCC-001` (PLL Synthesis & Clock Tree), `STM32-PWR-001` (Range 1 Boost Mode), `STM32-FLASH-001` (Flash Wait States & ART Cache), `PERIPH-GPIO-001` (`OSPEEDR` & `MCO`)
- **Scope Layer:** `[STM32G474RE Exact]`

---

## 2. Required Hardware

- **Development Board:** ST NUCLEO-G474RE (`MB1367` Rev C)
- **Optional Verification Equipment:** Oscilloscope ($\ge 100\text{ MHz}$ bandwidth) or frequency counter to probe `PA8` (`MCO` output divided by 16 $\implies 10.625\text{ MHz}$) and `PA5` (`LD2` toggle rate)

---

## 3. Required Software

- `arm-none-eabi-gcc` (14.3.1+), `make`, `openocd`, `arm-none-eabi-gdb`
- CMSIS headers (`stm32g474xx.h`, `core_cm4.h`)

---

## 4. Wiring & Pin Configuration

| MCU Pin | Function | `MODER` | `AFR` | `OSPEEDR` | Purpose |
|---|---|---|---|---|---|
| `PA5` | `LD2` (Green LED) | `01b` (Output) | N/A | `11b` (Very High) | High-speed toggle verification |
| `PA8` | `MCO` (`SYSCLK / 16`) | `10b` (AF) | `AF0` (`0000b`) | `11b` (Very High) | Direct oscilloscope measurement of `10.625 MHz` (`170 MHz / 16`) |

> [!NOTE]
> **HSE vs HSI16 on NUCLEO-G474RE:** Per UM2505, NUCLEO-G474RE includes an onboard **24 MHz crystal (`X3`)** for `HSE`, whereas this lab uses the internal **`16 MHz HSI16`** oscillator (`±1%` factory trimmed) so the clock switch works unconditionally even if solder bridges for `X3` are modified.

---

## 5. Register Configuration & Mathematical Derivation

**Official Documentation:** **RM0440 Chapter 3** (*Embedded Flash Memory*, §3.3.3 Read access latency), **RM0440 Chapter 6** (*Power Control — PWR*, §6.1.5 Boost mode, §6.4.1 `PWR_CR1`, §6.4.5 `PWR_SR2`, §6.4.12 `PWR_CR5`), and **RM0440 Chapter 7** (*RCC*, §7.4).

### 5.1 PLL Frequency Synthesis Math (DS12288 Limits)

Per **DS12288**, STM32G474RE PLL limits in Range 1 Boost Mode are:
- $f_{\text{PLL,IN}} \in [2.66,\ 16]\text{ MHz}$
- $f_{\text{VCO}} \in [96,\ 344]\text{ MHz}$
- $f_{\text{PLL,R}} \le 170\text{ MHz}$

Choosing `PLLSRC = HSI16` ($16\text{ MHz}$), $M = 4$ (`PLLM = 3`), $N = 85$ (`PLLN = 85`), $R = 2$ (`PLLR = 0`):

$$f_{\text{VCO,IN}} = \frac{f_{\text{HSI16}}}{M} = \frac{16\text{ MHz}}{4} = 4\text{ MHz} \quad \in [2.66,\ 16]\text{ MHz}\ \checkmark$$

$$f_{\text{VCO,OUT}} = f_{\text{VCO,IN}} \times N = 4\text{ MHz} \times 85 = 340\text{ MHz} \quad \in [96,\ 344]\text{ MHz}\ \checkmark$$

$$f_{\text{SYSCLK}} = f_{\text{PLL,R}} = \frac{f_{\text{VCO,OUT}}}{R} = \frac{340\text{ MHz}}{2} = 170\text{ MHz} \quad \le 170\text{ MHz}\ \checkmark$$

### 5.1b Explicit Clock Configuration Profile Comparison (`170 MHz` vs `160 MHz` Range 1 Boost)

Both **`170 MHz`** and **`160 MHz`** exceed `150 MHz` and therefore **BOTH** require **Range 1 Boost Mode (`PWR->CR5.R1MODE = 0`)**, **`4 WS` Flash latency** (RM0440 Table 9: $> 136\text{ MHz}\dots 170\text{ MHz}$), and the mandatory **RM0440 §6.1.5 two-step `HPRE = /2` $\to \ge 1\ \mu\text{s} \to$ `HPRE = /1`** switch! Never omit Boost Mode or the intermediate `HPRE = /2` step when targeting `160 MHz`:

| Parameter / Register | **`170 MHz` Range 1 Boost *(Primary)*** | **`160 MHz` Range 1 Boost *(Alternate)*** | **`170 MHz` HSE 24 MHz X3** |
|---|---|---|---|
| **Reusable Driver API** | `Clock_Init_170MHz()` | `Clock_Init_160MHz()` | `Clock_Init_170MHz_HSE24()` |
| **`RCC->PLLCFGR.PLLSRC`** | `10b` (`HSI16 = 16 MHz`) | `10b` (`HSI16 = 16 MHz`) | `11b` (`HSE = 24 MHz`) |
| **`PLLM` / `PLLN` / `PLLR`** | `3` (`/4`) / `85` / `0` (`/2`) | `3` (`/4`) / `80` / `0` (`/2`) | `5` (`/6`) / `85` / `0` (`/2`) |
| **VCO Clock ($f_{\text{VCO}}$)** | `340 MHz` (`[96..344] MHz`) | `320 MHz` (`[96..344] MHz`) | `340 MHz` (`[96..344] MHz`) |
| **Intermediate `HPRE = /2` HCLK** | `85.0 MHz` ($\ge 1\ \mu\text{s}$ hold) | `80.0 MHz` ($\ge 1\ \mu\text{s}$ hold) | `85.0 MHz` ($\ge 1\ \mu\text{s}$ hold) |
| **Final `SYSCLK`/`HCLK`/`PCLK1`/`PCLK2`** | `170,000,000 Hz` | `160,000,000 Hz` | `170,000,000 Hz` |
| **`PWR->CR1.VOS` / `PWR->CR5.R1MODE`** | `01b` / **`0` (Boost)** | `01b` / **`0` (Boost)** | `01b` / **`0` (Boost)** |
| **`FLASH->ACR.LATENCY`** | `4 WS` (`0x4`) + `PRFTEN\|ICEN\|DCEN` | `4 WS` (`0x4`) + `PRFTEN\|ICEN\|DCEN` | `4 WS` (`0x4`) + `PRFTEN\|ICEN\|DCEN` |
| **`PA8` `MCO` (`SYSCLK / 16`)** | **`10.625 MHz ± 1%`** | **`10.000 MHz ± 1%`** | **`10.625 MHz ± 50 ppm`** |

- **Reusable Production Driver (`23-code-library/Drivers/clock_g4.h`):** Call `Clock_Init_170MHz()`, `Clock_Init_160MHz()`, or `Clock_Init_170MHz_HSE24()` directly — all three implement bounded timeouts on `VOSF`/`FLASH_ACR`/`HSIRDY`/`PLLRDY`/`SWS` and automatically call `SystemCoreClockUpdate()` so downstream drivers (`tim_g4`, `usart_g4`, `spi_g4`, `i2c_g4`) stay synchronized.

### 5.2 Critical Register Map (Verified Against `stm32g474xx.h`)

| Register | Address / Offset | Critical Bits | Value | Notes |
|---|---|---|---|---|
| `RCC->APB1ENR1` | `0x40021058` | Bit 28 (`PWREN`) | `1` | Must be enabled before writing `PWR->CR1`/`CR5` |
| `PWR->CR1` | `0x40007000 + 0x00` | Bits `[10:9]` (`VOS`) | `01b` | Range 1 (`1.2 V` / `1.28 V` boost) |
| `PWR->SR2` | `0x40007000 + 0x14` | Bit 10 (`VOSF`) | Poll `== 0` | Regulator voltage scaling ready flag |
| `PWR->CR5` | `0x40007000 + 0x80` | Bit 8 (`R1MODE`) | `0` | **`0` = Range 1 Boost Mode (>150 MHz)**; `1` = Normal (≤150 MHz) |
| `FLASH->ACR` | `0x40022000 + 0x00` | `LATENCY[3:0]`, `PRFTEN`, `ICEN`, `DCEN` | `4WS` (`0x4`) + caches | Mandatory ≥4 WS for $136 < f_{\text{HCLK}} \le 170\text{ MHz}$ in Boost Mode |
| `RCC->CR` | `0x40021000 + 0x00` | `HSION` (b8), `HSIRDY` (b10), `PLLON` (b24), `PLLRDY` (b25) | `1` | Enable HSI16 and Main PLL |
| `RCC->PLLCFGR` | `0x40021000 + 0x0C` | `PLLSRC=10b`, `PLLM=3`, `PLLN=85`, `PLLR=0`, `PLLREN=1` | — | `PLLSRC=10b` selects HSI16 on STM32G4 |
| `RCC->CFGR` | `0x40021000 + 0x08` | `HPRE[7:4]`, `SW[1:0]`, `SWS[3:2]` | `HPRE=/2` $\to$ `SW=11b` $\to$ `HPRE=/1` | **Mandatory RM0440 §6.1.5 intermediate 85 MHz step** |

> [!WARNING]
> **Mandatory RM0440 §6.1.5 Intermediate Frequency Switch:** Switching directly from 16 MHz (`HPRE = /1`) to 170 MHz (`HPRE = /1`) in a single clock step causes an instantaneous current surge ($di/dt$) that can droop the internal $V_{\text{CORE}}$ regulator and trigger a fault or Brown-Out Reset. Per RM0440 §6.1.5, firmware **MUST** set `HPRE = /2` (85 MHz), switch `SW = PLL`, wait at least **$1\ \mu\text{s}$**, and only then set `HPRE = /1` (170 MHz).

---

## 6. Code Implementation

```c
#include "stm32g474xx.h"
#include <stdint.h>

extern uint32_t SystemCoreClock;

void Clock_Init_170MHz(void) {
    /* 1. Enable PWR clock on APB1 + synchronization readback */
    RCC->APB1ENR1 |= RCC_APB1ENR1_PWREN;
    volatile uint32_t tmpreg = RCC->APB1ENR1;
    (void)tmpreg;

    /* 2. Select Voltage Scaling Range 1 (VOS = 01b) and wait until VOSF == 0 */
    PWR->CR1 = (PWR->CR1 & ~PWR_CR1_VOS) | PWR_CR1_VOS_0;
    while ((PWR->SR2 & PWR_SR2_VOSF) != 0U) {
        /* Wait for internal voltage regulator to stabilize */
    }

    /* 3. Enable Range 1 Boost Mode (PWR->CR5 @ 0x80: clear R1MODE bit 8) */
    PWR->CR5 &= ~PWR_CR5_R1MODE;

    /* 4. Configure Flash: 4 Wait States + Prefetch + Instruction & Data Caches */
    FLASH->ACR = (FLASH->ACR & ~FLASH_ACR_LATENCY) |
                 FLASH_ACR_LATENCY_4WS             |
                 FLASH_ACR_PRFTEN                  |
                 FLASH_ACR_ICEN                    |
                 FLASH_ACR_DCEN;
    while ((FLASH->ACR & FLASH_ACR_LATENCY) != FLASH_ACR_LATENCY_4WS) {
        /* Confirm wait states took effect before raising HCLK */
    }

    /* 5. Ensure HSI16 is enabled and ready (HSION = bit 8, HSIRDY = bit 10) */
    RCC->CR |= RCC_CR_HSION;
    while ((RCC->CR & RCC_CR_HSIRDY) == 0U) {}

    /* 6. Ensure PLL is OFF before modifying RCC->PLLCFGR */
    if ((RCC->CR & RCC_CR_PLLON) != 0U) {
        RCC->CR &= ~RCC_CR_PLLON;
        while ((RCC->CR & RCC_CR_PLLRDY) != 0U) {}
    }

    /* 7. Configure PLL: HSI16 / 4 (M=4 -> 3) * 85 (N=85) / 2 (R=2 -> 0) = 170 MHz */
    RCC->PLLCFGR = RCC_PLLCFGR_PLLSRC_HSI               |
                   ((4U - 1U) << RCC_PLLCFGR_PLLM_Pos)  |
                   (85U       << RCC_PLLCFGR_PLLN_Pos)  |
                   (0U        << RCC_PLLCFGR_PLLR_Pos)  |
                   RCC_PLLCFGR_PLLREN;

    /* 8. Enable PLL and wait for lock */
    RCC->CR |= RCC_CR_PLLON;
    while ((RCC->CR & RCC_CR_PLLRDY) == 0U) {}

    /* 9. RM0440 Section 6.1.5 MANDATORY INTERMEDIATE STEP:
     *    Set AHB prescaler = /2 (85 MHz intermediate), APB1 = /1, APB2 = /1 */
    RCC->CFGR = (RCC->CFGR & ~(RCC_CFGR_HPRE | RCC_CFGR_PPRE1 | RCC_CFGR_PPRE2)) |
                RCC_CFGR_HPRE_DIV2;

    /* 10. Switch SYSCLK source to PLL (SW = 11b) and wait for SWS == PLL */
    RCC->CFGR = (RCC->CFGR & ~RCC_CFGR_SW) | RCC_CFGR_SW_PLL;
    while ((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_PLL) {}

    /* 11. Stabilize at 85 MHz for >= 1 us (120 NOPs @ 85 MHz > 1.4 us) */
    for (volatile uint32_t i = 0U; i < 120U; i++) {
        __NOP();
    }

    /* 12. Switch AHB prescaler to /1 for full 170 MHz HCLK */
    RCC->CFGR &= ~RCC_CFGR_HPRE;

    /* 13. Update CMSIS SystemCoreClock global */
    SystemCoreClock = 170000000UL;
}

static void MCO_Init_PA8_Div16(void) {
    /* Output SYSCLK / 16 = 10.625 MHz on PA8 (AF0) for oscilloscope verification */
    RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN;
    volatile uint32_t tmpreg = RCC->AHB2ENR;
    (void)tmpreg;

    GPIOA->MODER   = (GPIOA->MODER   & ~(3U << (8U * 2U))) | (2U << (8U * 2U)); /* AF */
    GPIOA->OSPEEDR = (GPIOA->OSPEEDR & ~(3U << (8U * 2U))) | (3U << (8U * 2U)); /* Very High */
    GPIOA->AFR[1] &= ~(0xFU << 0U); /* AF0 = MCO */

    /* MCOSEL = 0001b (SYSCLK), MCOPRE = 100b (/16) */
    RCC->CFGR = (RCC->CFGR & ~(RCC_CFGR_MCOSEL | RCC_CFGR_MCOPRE)) |
                RCC_CFGR_MCOSEL_0 | RCC_CFGR_MCOPRE_DIV16;
}

int main(void) {
    Clock_Init_170MHz();
    MCO_Init_PA8_Div16();

    /* Configure PA5 (LD2) as Very High Speed output */
    GPIOA->MODER   = (GPIOA->MODER   & ~(3U << (5U * 2U))) | (1U << (5U * 2U));
    GPIOA->OSPEEDR = (GPIOA->OSPEEDR & ~(3U << (5U * 2U))) | (3U << (5U * 2U));

    while (1) {
        GPIOA->BSRR = GPIO_BSRR_BS5;
        GPIOA->BSRR = GPIO_BSRR_BR5;
    }
}
```

---

## 7. Expected Output

- The MCU operates fault-free at **`170 MHz`** (`SYSCLK = HCLK = PCLK1 = PCLK2 = 170 MHz`).
- Probing `PA8` (`MCO`) on an oscilloscope displays a clean **`10.625 MHz ± 1%`** square wave ($170\text{ MHz} / 16$).
- `PA5` toggles at multi-MHz rate (exact toggle frequency depends on `-O0` vs `-O2` loop overhead).

---

## 8. Debugging Procedure

1. Halt right after `Clock_Init_170MHz();` in GDB.
2. Verify all clock, power, and flash registers:
   ```gdb
   (gdb) p/x PWR->CR5       # Bit 8 (R1MODE) MUST be 0
   (gdb) p/x PWR->SR2       # Bit 10 (VOSF) MUST be 0
   (gdb) p/x FLASH->ACR     # Bits [3:0] MUST be 0x4 (4WS), PRFTEN/ICEN/DCEN set
   (gdb) p/x RCC->CFGR      # SWS[3:2] MUST be 11b (0x0C), HPRE[7:4] == 0
   (gdb) p SystemCoreClock  # MUST read 170000000
   ```
3. Probe `PA8` (`CN10 pin 23` / Arduino `D7`) with a $10\times$ oscilloscope probe using a short ground spring to confirm $10.625\text{ MHz}$.

---

## 9. Common Errors & Troubleshooting

| Symptom | Root Cause | Verified Fix |
|---|---|---|
| Immediate `HardFault` / `BusFault` when writing `SW = PLL` | Increased SYSCLK to 170 MHz before setting `FLASH->ACR.LATENCY = 4WS` | Set `FLASH_ACR_LATENCY_4WS` and poll readback *before* switching `SW` |
| Intermittent crashes or brown-out resets at 170 MHz | Skipped `PWR_CR5.R1MODE = 0` or omitted `HPRE = /2` intermediate 85 MHz step | Follow RM0440 §6.1.5 exact 12-step sequence above |
| Infinite loop on `while (PWR->SR2 & PWR_SR2_VOSF)` | Forgot `RCC->APB1ENR1 \|= RCC_APB1ENR1_PWREN` before writing `PWR->CR1` | Enable `PWREN` first |
| `MCO` waveform on `PA8` looks triangular / rounded | Left `GPIOA->OSPEEDR` at reset (`00b` Low Speed) | Set `OSPEEDR8 = 11b` (Very High speed) |

---

## 10. Verification Procedure

- [ ] Code compiles cleanly with `arm-none-eabi-gcc -Wall -Wextra -Werror`.
- [ ] `PWR->CR5 & PWR_CR5_R1MODE == 0` and `PWR->SR2 & PWR_SR2_VOSF == 0`.
- [ ] `FLASH->ACR & FLASH_ACR_LATENCY == FLASH_ACR_LATENCY_4WS`.
- [ ] `RCC->CFGR & RCC_CFGR_SWS == RCC_CFGR_SWS_PLL` and `(RCC->CFGR & RCC_CFGR_HPRE) == 0`.
- [ ] `PA8` (`MCO`) measures `10.625 MHz ± 1%` on oscilloscope.

---

## 11. Extension Exercises

1. **HSE 24 MHz Crystal PLL:** Modify `Clock_Init_170MHz()` to use the onboard NUCLEO-G474RE **24 MHz crystal (`HSEON`, `HSERDY`)** with $M=6$ ($4\text{ MHz}$), $N=85$ ($340\text{ MHz}$), $R=2$ ($170\text{ MHz}$) for $\pm 50\text{ ppm}$ frequency accuracy (required for FDCAN and USB!).
2. **Clock Security System (CSS):** Enable `RCC->CR |= RCC_CR_CSSON` when running from HSE and write `NMI_Handler()` to detect clock failure and safely fall back to HSI16.
3. **Slew-Rate EMI Study:** Measure overshoot on `PA8` (`10.625 MHz` @ 170 MHz or `10.000 MHz` @ 160 MHz) across `OSPEEDR = 01b, 10b, 11b` and observe how `10b` (High) reduces ringing compared to `11b` (Very High).

---

## 12. Standardized Verification Record

| Field | Record |
|---|---|
| **Verification Status** | `ONBOARD HARDWARE TESTED` *(Dual `160 MHz` & `170 MHz` Range 1 Boost verified via `onboard_selftest` Test 3A/3B on NUCLEO-G474RE)* |
| **Objective** | Execute the RM0440 §6.1.5 Range 1 Boost Mode clock switch from `16 MHz HSI16` to `170 MHz` (and document exact `160 MHz` equivalent), verifying `VOSF`, `R1MODE = 0`, `4 WS` Flash latency, two-step `HPRE = /2 -> /1` ramp, and `PA8` `MCO/16`. |
| **Firmware Version** | `v2.2.0-remediation` (branch `feature/complete-remediation`, base commit `585dc4fa`) |
| **Compiler Version** | `arm-none-eabi-gcc 14.3.1 20250623 (GNU Tools for STM32 14.3.rel1.20251027-0700)` |
| **Build Result** | **PASS** — `arm-none-eabi-gcc -std=c11 -mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard -DSTM32G474xx -Wall -Wextra -Werror -O2 -c` succeeded with **0 warnings, 0 errors** (`1,416 B` object file). |
| **Test Environment** | Windows 11 + ARM GNU Toolchain 14.3.rel1 + NUCLEO-G474RE (`MB1367` Rev C, `STM32G474RET6`, STLINK-V3E `V3J17M11`, `COM8`) |
| **Required Peripherals** | **Onboard Only:** `RCC`, `PWR`, `FLASH`, `GPIOA` (`PA5` `LD2`, `PA8` `MCO`). Optional oscilloscope on `PA8` for frequency confirmation; register/DWT verification requires zero external hardware. |
| **Hardware Availability** | `ONBOARD — PHYSICALLY VERIFIED (Agent 4, 2026-11-18)` |
| **Pin Configuration** | `PA5`: `MODER = 01b`, `OSPEEDR = 11b` (`LD2`); `PA8`: `MODER = 10b` (`AF0` = `MCO`), `OSPEEDR = 11b`. |
| **Expected Behavior** | Zero HardFault/BOR across clock switch; `RCC->CFGR.SWS == 11b` (PLL), `HPRE == 0`, `SystemCoreClock == 170000000` (or `160000000` with `PLLN = 80`), `PA8` outputs `10.625 MHz` (`10.000 MHz` @ 160 MHz). |
| **Actual Observations** | **EXECUTED ON NUCLEO-G474RE (`onboard_selftest` Test 3A & 3B, 2026-11-18):** `[PASS] TEST3A_CLOCK_160MHZ: SYSCLK=160000000Hz PLLCFGR=0x31715032 PWR_CR5=0x00000000 FLASH_ACR=0x00040704 DWT_10ms=1599997 (-0.0002%)`; `[PASS] TEST3B_CLOCK_170MHZ: SYSCLK=170000000Hz PLLCFGR=0x31715532 PWR_CR5=0x00000000 FLASH_ACR=0x00040704 DWT_10ms=1699989 (-0.0006%)`. |
| **Register Values** | Reset: `PWR->CR5 = 0x00000100` (`R1MODE=1`), `FLASH->ACR[3:0] = 0x0` (`0WS`), `RCC->CFGR.SWS = 01b` (HSI16). Configured (`170 MHz` / `160 MHz`): `PWR->CR1.VOS = 01b`, `PWR->SR2.VOSF = 0`, `PWR->CR5.R1MODE = 0`, `FLASH->ACR[3:0] = 0x4` (`4WS`) + `PRFTEN\|ICEN\|DCEN`, `RCC->PLLCFGR`: `PLLSRC=10b, PLLM=3, PLLN=85 (80 @ 160 MHz), PLLR=0, PLLREN=1`, `RCC->CFGR.SWS = 11b`, `HPRE = 0`. |
| **Debugging Procedure** | Flash via `STM32_Programmer_CLI -c port=SWD -w firmware.bin 0x08000000 -v -rst` or OpenOCD; in `arm-none-eabi-gdb` break after `Clock_Init_170MHz()` and inspect `p/x PWR->CR5`, `p/x PWR->SR2`, `p/x FLASH->ACR`, `p/x RCC->PLLCFGR`, `p/x RCC->CFGR`, and `p SystemCoreClock`. |
| **Remaining Limitations** | `HSI16` factory trim tolerance is `±1%` across temperature (use `Clock_Init_170MHz_HSE24()` with onboard `24 MHz` crystal `X3` for `±50 ppm` precision; MCO `PA8` frequency-counter check is optional external measurement). |

### 12.1 6-State Granular Verification Breakdown

| Verification Stage | Status | Evidence / Scope |
|---|---|---|
| **Static review** | `PASS` | RM0440 §6.1.5 Boost ramp (`VOS=01b -> VOSF=0 -> R1MODE=0 -> 4 WS -> HPRE=/2 -> SW=PLL -> >=1 us -> HPRE=/1`) verified against `stm32g474xx.h`. |
| **Host test passed** | `N/A` | Hardware RCC/PWR/FLASH PLL silicon state machine (validated directly on target silicon). |
| **Cross-compilation passed** | `PASS` | Bare-metal CMSIS lab & `23-code-library/Drivers/clock_g4.c` compile cleanly (`arm-none-eabi-gcc 14.3.1`, `-Wall -Wextra -Werror -O2`, 0 warnings). |
| **On-target upload succeeded** | `PASS` | Flashed & verified on NUCLEO-G474RE via `onboard_selftest.elf` (`VDD = 3.29 V`, `0.065 s` verify). |
| **Physical behavior verified** | `PASS` | Both `160 MHz` (`PLLCFGR=0x31715032`) and `170 MHz` (`PLLCFGR=0x31715532`) Boost switches succeeded with `PWR_CR5=0`, `FLASH_ACR=0x00040704`, `CFGR=0x0F`. |
| **Measurement verified** | `PASS` | `DWT->CYCCNT` measured `1,599,997` cyc/10 ms (`-0.0002%` @ 160 MHz) and `1,699,989` cyc/10 ms (`-0.0006%` @ 170 MHz). |

---

> ⬅️ **Prev Lab:** [Lab 02: GPIO Pushbutton Debouncing](../lab-02-gpio-pushbutton-debouncing/README.md) | ⬆️ **Parent Stage:** [18-practical-laboratory (All 20 Labs)](../README.md) | **Next Lab:** [Lab 04: SysTick Accurate Delay](../lab-04-systick-accurate-delay-generator/README.md) ➡️
