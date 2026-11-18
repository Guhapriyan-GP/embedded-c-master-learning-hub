# Stage 07: General-Purpose I/O (GPIO) Hardware Engine

> **Directory:** `08-gpio/` | **Target Platform:** STM32G474RET6 (LQFP64, 52 GPIOs) on NUCLEO-G474RE (MB1367)  
> **Code Verification Notice:** All bare-metal C snippets in this module are **Statically Reviewed against `stm32g474xx.h`, RM0440, DS12288, and UM2505 (Not Tested on Physical Hardware)**.

---

## 1. Learning Objectives & Concept Scope

| Concept ID | Concept Name | Architectural Scope | Mastery Tier |
|---|---|---|---|
| `PERIPH-GPIO-001` | STM32G4 GPIO Architecture, 52-Pin LQFP64 Map (No Port E), **Analog Reset Default (`MODER=11b`)**, Input/Push-Pull/Open-Drain/AF Modes | `[STM32G474RE Exact]` | Level 1 (Essential) |
| `PERIPH-GPIO-002` | Output Slew-Rate / Speed Control (`OSPEEDR`) & EMI / Signal-Integrity Trade-Offs | `[STM32G474RE Exact]` | Level 1 (Essential) |
| `PERIPH-GPIO-003` | Single-Cycle Atomic Pin Control (`BSRR` & `BRR`) vs Non-Atomic `ODR` Read-Modify-Write Race Conditions | `[STM32 Generic]` | Level 1 (Essential) |
| `PERIPH-GPIO-004` | Mechanical Switch Debouncing (Hardware RC + Schmitt Trigger vs Non-Blocking 4-State Software FSM) | `[General C]` / `[STM32G474RE Exact]` | Level 1 (Essential) |
| `PERIPH-GPIO-005` | Hardware Configuration Lock (`LCKR` 4-Step Sequence) & Board Hardware Traps (`PB8-BOOT0`, `PA9/PA10` UCPD Dead-Battery, `PA5/LD2`) | `[STM32G474RE Exact]` | Level 2 (Production) |
| `HWSW-PCB-001` | NUCLEO-G474RE (MB1367 / UM2505) Physical Pin Polarity (`B1 PC13` Active-HIGH!) & Electrical Current Limits | `[STM32G474RE Exact]` | Level 1 (Essential) |
| `GPIO-FMP-001` | `SYSCFG->CFGR1` I2C Fast-Mode Plus (1 MHz, 20 mA Sink) High-Drive Activation (`I2C_PB6..9_FMP` & `I2C1..4_FMP`) on `FT_f` Pads | `[STM32G474RE Exact]` | Level 2 (Production) |

---

## 2. Prerequisites & Curriculum Positioning

### Prerequisite Concept IDs
- **Stage 01 (`02-c-fundamentals/`):** `C-BIT-001` (Bitwise shifts/masks) and `DIG-ELEC-001` (Ohm's Law, pull-up/pull-down voltage dividers, CMOS Push-Pull vs Open-Drain).
- **Stage 02 (`03-embedded-c/`):** `EMB-MMIO-001` (`volatile` struct pointers, atomic vs RMW register access).
- **Stage 05 (`06-stm32-fundamentals/`):** `STM32-ARCH-001` (AHB2 bus address map and DS12288 pin tables).
- **Stage 06 Phase A (`07-rcc-and-clocks/`):** Minimal clock gating (`RCC->AHB2ENR |= RCC_AHB2ENR_GPIOxEN` on default 16 MHz HSI16).

### Pedagogical Sequencing Note (Practice Basic GPIO BEFORE 170 MHz PLL!)
- **Level 1 Basic GPIO (`Labs 01–02`)** is practiced **first** on the reset-default **16 MHz HSI16** clock (requiring only `RCC->AHB2ENR`).
- Once you can toggle `PA5` (`LD2`) and read `PC13` (`B1`) cleanly, return to **[Stage 06 Phase B (`07-rcc-and-clocks/`)](../07-rcc-and-clocks/README.md)** to configure the **170 MHz PLL + Range 1 Boost (`Lab 03`)** and verify `SYSCLK/16` via `PA8` (`MCO` AF0)!

---

## 3. Core Technical Theory & Engineering Principles

### 3.1 Exact STM32G474RET6 (LQFP64) 52-Pin Port Inventory `[STM32G474RE Exact]`
Do not confuse silicon die declarations in `stm32g474xx.h` with physical LQFP64 package pins! Per **DS12288 Table 2 & Table 12**, STM32G474RET6 exposes exactly **52 GPIOs** across **6 ports**:

| Port | Base Address (AHB2) | `RCC->AHB2ENR` Bit | Bonded Pins on LQFP64 | `MODER` Reset Value | Special / Reserved Pins |
|---|---|---|---|---|---|
| **`GPIOA`** | `0x48000000` | Bit 0 (`GPIOAEN`) | `PA0–PA15` (16 pins) | **`0xABFFFFFF`** | `PA13/PA14` (SWDIO/SWCLK AF0), `PA15` (JTDI AF0), `PA5` (`LD2` via `SB6`), `PA2/PA3` (ST-LINK VCP), `PA9/PA10` (`UCPD1_DBCC1/2` 5.1 kΩ trap!) |
| **`GPIOB`** | `0x48000400` | Bit 1 (`GPIOBEN`) | `PB0–PB15` (16 pins) | **`0xFFFFFEBF`** | `PB3/PB4` (JTAG AF0), **`PB8-BOOT0`** (external 10 kΩ PD `R36`), `PB4/PB6` (`UCPD1_CC2/CC1`) |
| **`GPIOC`** | `0x48000800` | Bit 2 (`GPIOCEN`) | `PC0–PC15` (16 pins) | **`0xFFFFFFFF`** | **`PC13`** (User Button `B1` **Active-HIGH**), `PC14/PC15` (`X2` 32.768 kHz LSE) |
| **`GPIOD`** | `0x48000C00` | Bit 3 (`GPIODEN`) | **`PD2` only** (1 pin) | **`0xFFFFFFFF`** | `PD0–PD1`, `PD3–PD15` not bonded out |
| **`GPIOE`** | `0x48001000` | Bit 4 (`GPIOEEN`) | **0 pins (UNBONDED!)** | `0xFFFFFFFF` | Declared in CMSIS header for LQFP100+, **inaccessible on NUCLEO-G474RE** |
| **`GPIOF`** | `0x48001400` | Bit 5 (`GPIOFEN`) | **`PF0–PF1` only** (2 pins) | **`0xFFFFFFFF`** | `PF0/PF1` wired to **24 MHz HSE crystal `X3`** by default |
| **`GPIOG`** | `0x48001800` | Bit 6 (`GPIOGEN`) | **`PG10-NRST` only** (1 pin) | **`0xFFFFFFFF`** | Reset pin by default (configurable via `FLASH->OPTR.NRST_MODE`) |

### 3.2 Critical STM32G4 Reset Trap: `MODER = 11b` (Analog Mode) `[STM32G474RE Exact]`
Unlike legacy STM32F4 microcontrollers (where GPIO pins reset to `00b` Digital Input), **STM32G4 resets all non-debug pins to `11b` (Analog Mode, `0xFFFFFFFF`)**.
- In Analog Mode, the input **Schmitt trigger is physically disconnected** to eliminate CMOS shoot-through current on floating pins.
- **Consequence:** Reading `GPIOC->IDR` without first clearing `GPIOC->MODER[27:26]` to `00b` **ALWAYS returns `0`** and prevents EXTI from ever firing! This exact mistake caused fatal boot-loops in legacy Labs 13 and 14.

### 3.3 Internal Pin Circuitry, Output Drivers & Slew-Rate EMI Control
1. **Input Path:** Protection clamp diodes (`FT` = 5 V-tolerant when $V_{\text{DD}} > 0$; `TT`/`TTa` = $V_{\text{DDA}}$-limited per DS12288) $\to$ selectable ~40 kΩ internal pull-up/pull-down (`PUPDR`) $\to$ **Schmitt Trigger** (hysteresis rejects slow-edge noise) $\to$ `IDR` (sampled every AHB2 cycle).
2. **Output Driver (`OTYPER`):**
   - **Push-Pull (`0`):** Complementary P-MOS (drives `VDD`) and N-MOS (drives `VSS`). Actively drives both HIGH and LOW; NEVER connect two push-pull outputs together (bus contention short!).
   - **Open-Drain (`1`):** P-MOS permanently disabled; N-MOS pulls LOW (`0`), writing `1` releases the pin to **Hi-Z**, relying on an external pull-up resistor (mandatory for I2C wired-AND arbitration and level shifting).
3. **Slew Rate (`OSPEEDR`):** `00` Low ($\le 5\text{ MHz}$), `01` Medium ($\le 25\text{ MHz}$), `10` High ($\le 50\text{ MHz}$), `11` Very High ($\le 100+\text{ MHz}$). Choose the **slowest speed that meets your signal rise-time requirement** — excessively fast edges on long jumper wires induce ground bounce, overshoot, and radiated EMI.
4. **Electrical Limits (DS12288 Table 15):** Max $\pm 20\text{ mA}$ per standard I/O pin (recommend $\le 8\text{ mA}$ for clean $V_{\text{OL}}/V_{\text{OH}}$), max $160\text{ mA}$ combined $I_{\text{VDD}}/I_{\text{VSS}}$ across all I/Os.

#### 3.3.1 `SYSCFG->CFGR1` I2C Fast-Mode Plus (Fm+, 1 MHz / 20 mA) High-Current Drive (`GPIO-FMP-001`) `[STM32G474RE Exact]`

Standard I2C (`100 kHz`) and Fast Mode (`400 kHz`) sink $I_{OL} = 3\text{ mA}$ at $V_{OL} \le 0.4\text{ V}$. At **1 MHz Fast-Mode Plus (Fm+)**, **NXP UM10204** tightens maximum rise time to **$t_{r,\max} = 120\text{ ns}$** (for $C_b$ up to $550\text{ pF}$), demanding strong pull-up resistors ($R_p \approx 330\text{--}680\ \Omega$) and a **$20\text{ mA}$ open-drain sink driver** (`V_OL <= 0.4 V` at `I_OL = 20 mA`, **DS12288 Table 54**).

On STM32G474RE, only **`FT_f`** pads physically implement the 20 mA Fm+ pull-down transistor, and it is **disabled at reset** until software unlocks **`SYSCFG`** (`RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN; (void)RCC->APB2ENR;`) and sets the corresponding control bit in **`SYSCFG->CFGR1`** (offset `0x004`, base `0x40010000`, verified in `stm32g474xx.h`):

| `SYSCFG->CFGR1` Bit | CMSIS Macro (`stm32g474xx.h`) | Scope & Hardware Behavior (RM0440 §10.2.2) |
|---|---|---|
| **Bit 16** | `SYSCFG_CFGR1_I2C_PB6_FMP` | Forces 20 mA Fm+ drive on **`PB6`** (even in GPIO output mode, not only I2C AF!) |
| **Bit 17** | `SYSCFG_CFGR1_I2C_PB7_FMP` | Forces 20 mA Fm+ drive on **`PB7`** (I2C1_SDA AF4 or high-sink GPIO!) |
| **Bit 18** | `SYSCFG_CFGR1_I2C_PB8_FMP` | Forces 20 mA Fm+ drive on **`PB8`** *(beware MB1367 `R36` 10 kΩ `BOOT0` pull-down!)* |
| **Bit 19** | `SYSCFG_CFGR1_I2C_PB9_FMP` | Forces 20 mA Fm+ drive on **`PB9`** |
| **Bit 20** | `SYSCFG_CFGR1_I2C1_FMP` | Enables 20 mA Fm+ drive on whichever `FT_f` pins are AF-mapped to **`I2C1`** (e.g., `PA15`/`PB7`) |
| **Bit 21** | `SYSCFG_CFGR1_I2C2_FMP` | Enables 20 mA Fm+ drive on `FT_f` pins AF-mapped to **`I2C2`** |
| **Bit 22** | `SYSCFG_CFGR1_I2C3_FMP` | Enables 20 mA Fm+ drive on `FT_f` pins AF-mapped to **`I2C3`** |
| **Bit 23** | `SYSCFG_CFGR1_I2C4_FMP` | Enables 20 mA Fm+ drive on `FT_f` pins AF-mapped to **`I2C4`** |

> **Fm+ Pull-Up Calculation & Current-Budget Warning:** For $V_{DD} = 3.3\text{ V}$, $V_{OL(\max)} = 0.4\text{ V}$, $I_{OL} = 20\text{ mA}$, $C_b = 150\text{ pF}$, $t_{r,\max} = 120\text{ ns}$:
> $$R_{p,\min} = \frac{3.3 - 0.4}{0.020} = \mathbf{145\ \Omega}, \qquad R_{p,\max} = \frac{120\times 10^{-9}}{0.8473 \times 150\times 10^{-12}} = \mathbf{944\ \Omega} \implies \text{Choose } \mathbf{470\ \Omega\text{ or }680\ \Omega}$$
> Always confirm the pin's DS12288 Table 12 type is **`FT_f`** (standard `FT`/`TT_a` pins cannot drive 20 mA at $V_{OL} \le 0.4\text{ V}$) and account for 20 mA sinks against the package $\sum I_{VSS} \le 160\text{ mA}$ budget!

### 3.4 Atomic Pin Manipulation (`BSRR` & `BRR`) vs `ODR` Race Conditions
Executing `GPIOA->ODR ^= (1U << 5)` compiles into a **3-instruction non-atomic Read-Modify-Write (RMW)** sequence (`LDR` $\to$ `EOR` $\to$ `STR`). If an ISR preempts between `LDR` and `STR` and modifies `PA6`, the main loop's `STR` overwrites and destroys the ISR's update!
- **`GPIOx->BSRR` (`0x18`, Write-Only):** Single-cycle atomic write. Bits `[15:0]` (`BSy=1`) set pins HIGH; bits `[31:16]` (`BRy=1`) reset pins LOW; writing `0` leaves the pin untouched. If both `BSy` and `BRy` are written `1`, **Set (`BSy`) has priority**.
- **`GPIOx->BRR` (`0x28`, Write-Only):** STM32G4 dedicated 16-bit atomic reset register (`BRy` in bits `[15:0]`).

### 3.5 NUCLEO-G474RE (MB1367) Critical Board Hardware Traps `[STM32G474RE Exact]`
1. **User Button B1 (`PC13`) is ACTIVE-HIGH (UM2505):** Unlike older MB1136 Nucleo boards (STM32F4), NUCLEO-G474RE wires `B1` between `VDD` and `PC13` with an **external pull-down resistor**. Unpressed = `0` (`LOW`); Pressed = `1` (`HIGH`). Configure `PUPDR = 00` (No Pull — enabling internal pull-up creates a voltage divider fighting the external pull-down!).
2. **`PB8-BOOT0` 10 kΩ Pull-Down Trap (`R36`):** `PB8` has an external 10 kΩ pull-down on NUCLEO-G474RE for `BOOT0`. Using `PB8` as I2C1 SCL with only the weak ~40 kΩ internal pull-up forms a divider $3.3\text{ V} \times \frac{10}{10+40} = 0.66\text{ V}$ — **permanently stuck LOW!** Always add external `2.2–4.7 kΩ` pull-ups or use alternate I2C pins (`PA15`/`PB7`).
3. **`PA9` / `PA10` UCPD Dead-Battery 5.1 kΩ Pull-Down Trap:** After reset, STM32G4 enables **5.1 kΩ USB-C dead-battery pull-downs** on `PA9`/`PA10` (and `PB6`/`PB4` CC lines). Unless using USB-C PD, you MUST execute:
   ```c
   RCC->APB1ENR1 |= RCC_APB1ENR1_PWREN;
   (void)RCC->APB1ENR1;
   PWR->CR3 |= PWR_CR3_UCPD_DBDIS; /* Bit 14: Disable 5.1 kOhm dead-battery pull-downs */
   ```
4. **`PA5` / `LD2` Shared Loading Trap:** `PA5` drives the green LED `LD2` (via `SB6` / transistor buffer on MB1367) and is also `SPI1_SCK` and `DAC1_OUT2`. Account for LED loading or use `PB3/PB4/PB5` for high-speed SPI1.

### 3.6 Non-Blocking Switch Debouncing & API Comparison (Register vs LL vs HAL)
Mechanical contacts bounce for `5–20 ms`. Never call blocking `delay_ms(20)` inside `main()` or an ISR! Instead, sample at `1–5 ms` ticks using a **4-state FSM** (`IDLE` $\to$ `DEBOUNCE_PRESS` $\to$ `PRESSED` $\to$ `DEBOUNCE_RELEASE`) or a shift-register integrator (`hist = (hist << 1) | raw;`).

| Approach | Code Style (`PA5` Set HIGH) | Flash / Cycle Cost | Recommended Production Role |
|---|---|---|---|
| **Direct CMSIS Register** | `GPIOA->BSRR = GPIO_BSRR_BS5;` | 1–2 instructions (~2 cycles) | Primary learning target, hard-RT ISRs, full silicon clarity |
| **STM32Cube LL** | `LL_GPIO_SetOutputPin(GPIOA, LL_GPIO_PIN_5);` | Identical (`static inline`) | Production sweet spot for motor control, digital power, tight ISRs |
| **STM32Cube HAL** | `HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_SET);` | Function call + branch (~15–25 cycles) | Rapid prototyping, low-frequency control, complex peripherals (`USB`/`FDCAN`) |

---

## 4. Core Terminology & Acronyms

| Term | Engineering Meaning |
|---|---|
| **Push-Pull / Open-Drain** | Active HIGH+LOW complementary CMOS driver vs active LOW-only + Hi-Z driver requiring external pull-up. |
| **Schmitt Trigger** | Input buffer with dual switching thresholds ($V_{\text{IT}+} > V_{\text{IT}-}$) providing noise hysteresis; OFF in Analog mode. |
| **Atomic Access (`BSRR`/`BRR`)** | Single-instruction hardware register write immune to ISR preemption races. |
| **Alternate Function (`AF0–AF15`)** | 4-bit per-pin multiplexer (`AFR[0]` pins 0–7, `AFR[1]` pins 8–15) connecting internal peripherals to physical pins. |
| **`FT` / `TTa` I/O Structure** | DS12288 pin tolerance codes: Five-volt Tolerant (`FT`) vs Three-volt Tolerant analog (`TTa`, never exceed $V_{\text{DDA}} + 0.3\text{ V}$!). |
| **Dead-Battery `Rd` (`UCPD_DBDIS`)** | Hardware 5.1 kΩ USB-C pull-down enabled by default after reset on `PA9`/`PA10` until disabled in `PWR->CR3`. |

---

## 5. Hardware Register Architecture & Bitfield Reference

Verified directly against `GPIO_TypeDef` in `stm32g474xx.h` (`GPIOA = 0x48000000`, stride `0x0400` per port):

| Register | Offset | Access | Bit Width / Pin | Function & Reset Value |
|---|---|---|---|---|
| **`MODER`** | `0x00` | R/W | 2 bits `[2n+1:2n]` | `00`=Input, `01`=Output, `10`=AF, **`11`=Analog (STM32G4 Reset Default!)** |
| **`OTYPER`** | `0x04` | R/W | 1 bit `[n]` | `0`=Push-Pull (reset), `1`=Open-Drain |
| **`OSPEEDR`** | `0x08` | R/W | 2 bits `[2n+1:2n]` | `00`=Low, `01`=Medium, `10`=High, `11`=Very High |
| **`PUPDR`** | `0x0C` | R/W | 2 bits `[2n+1:2n]` | `00`=No Pull, `01`=Pull-Up (~40 kΩ), `10`=Pull-Down (~40 kΩ), `11`=Reserved |
| **`IDR`** | `0x10` | RO | 1 bit `[n]` | Synchronized digital pin input state (reads `0` when `MODER=11b`!) |
| **`ODR`** | `0x14` | R/W | 1 bit `[n]` | Output data latch (avoid RMW `^=` if shared with ISRs!) |
| **`BSRR`** | `0x18` | WO | `[15:0]` Set / `[31:16]` Reset | Single-cycle atomic Set/Reset (`BSy` wins if both `BSy` and `BRy` = 1) |
| **`LCKR`** | `0x1C` | R/W | `[15:0]` pins + `[16]` `LCKK` | Locks `MODER/OTYPER/OSPEEDR/PUPDR/AFR` until next MCU reset |
| **`AFR[0]` (`AFRL`)** | `0x20` | R/W | 4 bits per pin `0..7` | Alternate Function `0..15` per DS12288 Table 13 |
| **`AFR[1]` (`AFRH`)** | `0x24` | R/W | 4 bits per pin `8..15` | Alternate Function `0..15` per DS12288 Table 13 |
| **`BRR`** | `0x28` | WO | 1 bit `[n]` (`[15:0]`) | Single-cycle atomic pin Reset |

**Safe Configuration Order (Zero Output Glitch):** Enable `RCC->AHB2ENR` + dummy read $\to$ pre-load `BSRR` (initial output state) $\to$ configure `OTYPER`, `OSPEEDR`, `PUPDR`, `AFR` $\to$ write `MODER` **last**.

---

## 6. Engineering Mathematics & Worked Formulas

### 6.1 LED Series Current-Limiting Resistor Sizing
$$R_{\text{LED}} = \frac{V_{\text{DD}} - V_{F,\text{LED}} - V_{\text{OL/drop}}}{I_{\text{LED}}}$$
For $V_{\text{DD}} = 3.3\text{ V}$, Green LED $V_F = 2.1\text{ V}$, target $I_{\text{LED}} = 5\text{ mA}$ (well below DS12288 $8\text{ mA}$ linear rating):
$$R_{\text{LED}} = \frac{3.3 - 2.1}{0.005} = 240\ \Omega \quad (\text{select standard } 270\ \Omega \text{ or } 330\ \Omega)$$

### 6.2 `PB8-BOOT0` Voltage Divider Failure Proof
When external `R36` ($10\text{ k}\Omega$ pull-down) fights an internal weak pull-up ($R_{\text{PU}} \approx 40\text{ k}\Omega$):
$$V_{\text{PB8}} = 3.3\text{ V} \times \frac{10\text{ k}\Omega}{10\text{ k}\Omega + 40\text{ k}\Omega} = 0.66\text{ V} < V_{\text{IL,max}} \approx 0.99\text{ V}$$
The pin never reaches CMOS logic HIGH ($V_{\text{IH,min}} = 0.7 \times V_{\text{DD}} = 2.31\text{ V}$). With an external $2.2\text{ k}\Omega$ pull-up: $V_{\text{PB8}} = 3.3 \times \frac{10}{12.2} = 2.70\text{ V} > 2.31\text{ V}$ $\checkmark$.

### 6.3 Hardware RC Debounce Filter Time Constant
To reject $t_{\text{bounce}} = 10\text{ ms}$ with a Schmitt-trigger input ($V_{\text{IT}+} \approx 0.65\,V_{\text{DD}}$):
$$t = -R C \ln\!\left(1 - \frac{V_{\text{IT}+}}{V_{\text{DD}}}\right) \approx 1.05\,RC \implies RC \approx 10\text{ ms}\ (\text{e.g., } R = 100\text{ k}\Omega,\ C = 100\text{ nF})$$

---

## 7. Official Documentation & Standards References

| Document | Exact Sections |
|---|---|
| **ST RM0440** | Ch. 9 (*General-purpose I/Os — GPIO*: `MODER`, `BSRR`, `BRR`, `LCKR`, `AFR`); Ch. 5/6 (`PWR->CR3.UCPD_DBDIS`) |
| **ST DS12288** | Table 12 (*STM32G474xE LQFP64 pin definitions*), Table 13 (*Alternate function mapping*), §5.3.14 (*I/O static & AC characteristics*) |
| **ST UM2505** | §6.5 (*LEDs: LD2 on PA5*), §6.6 (*Push-buttons: B1 on PC13 Active-HIGH*), Tables 13–17 (Arduino & Morpho pinouts) |
| **ST AN4899** | *STM32 Microcontroller GPIO Hardware Settings and Low-Power Consumption* |

---

## 8. Production-Grade Bare-Metal C Code Examples

> **Verification Status:** Statically Reviewed against `stm32g474xx.h`, RM0440, and UM2505 (Not Tested on Physical Hardware).

### 8.1 Glitch-Free GPIO Init (`PA5` LD2 + `PC13` Active-HIGH B1) & Atomic Toggle
```c
#include "stm32g474xx.h"
#include <stdint.h>
#include <stdbool.h>

void GPIO_Board_Init(void)
{
    /* Enable GPIOA and GPIOC clocks + mandatory AHB2 sync dummy read */
    RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN | RCC_AHB2ENR_GPIOCEN;
    (void)RCC->AHB2ENR;

    /* PA5 (LD2): Preload LOW -> Push-Pull -> Low Speed -> No Pull -> Output (01b) */
    GPIOA->BSRR    = GPIO_BSRR_BR5;
    GPIOA->OTYPER &= ~GPIO_OTYPER_OT5;
    GPIOA->OSPEEDR = (GPIOA->OSPEEDR & ~GPIO_OSPEEDR_OSPEED5_Msk);
    GPIOA->PUPDR  &= ~GPIO_PUPDR_PUPD5_Msk;
    GPIOA->MODER   = (GPIOA->MODER & ~GPIO_MODER_MODE5_Msk) | GPIO_MODER_MODE5_0;

    /* PC13 (B1 User Button, UM2505 Active-HIGH with external pull-down):
     * MUST clear MODER[27:26] from STM32G4 reset Analog (11b) to Input (00b)!
     * Leave PUPDR = 00b (No Pull) so we do not fight external pull-down. */
    GPIOC->PUPDR &= ~GPIO_PUPDR_PUPD13_Msk;
    GPIOC->MODER &= ~GPIO_MODER_MODE13_Msk;
}

static inline void LD2_Toggle_Atomic(void)
{
    /* Atomic single-pin toggle using BSRR without risking ODR RMW corruption of other pins */
    if ((GPIOA->ODR & GPIO_ODR_OD5) != 0U) {
        GPIOA->BSRR = GPIO_BSRR_BR5;
    } else {
        GPIOA->BSRR = GPIO_BSRR_BS5;
    }
}

static inline bool B1_IsPressed(void)
{
    /* NUCLEO-G474RE (MB1367): Active-HIGH (pressed == 1) */
    return (GPIOC->IDR & GPIO_IDR_ID13) != 0U;
}
```

### 8.2 Non-Blocking 4-State Debounce FSM (1 ms Tick Driven — Zero Blocking Delays)
```c
typedef enum {
    BTN_IDLE = 0,
    BTN_DEBOUNCE_PRESS,
    BTN_PRESSED,
    BTN_DEBOUNCE_RELEASE
} btn_state_t;

typedef struct {
    btn_state_t state;
    uint8_t     ticks;
    uint8_t     threshold_ms;
} btn_fsm_t;

/* Call every 1 ms from non-blocking SysTick/Timer loop; returns true once per clean press */
bool BTN_FSM_Update_1ms(btn_fsm_t *fsm, bool raw_pressed)
{
    bool press_event = false;

    switch (fsm->state) {
        case BTN_IDLE:
            if (raw_pressed) {
                fsm->ticks = 0U;
                fsm->state = BTN_DEBOUNCE_PRESS;
            }
            break;
        case BTN_DEBOUNCE_PRESS:
            if (!raw_pressed) {
                fsm->state = BTN_IDLE;
            } else if (++fsm->ticks >= fsm->threshold_ms) {
                fsm->state  = BTN_PRESSED;
                press_event = true;
            }
            break;
        case BTN_PRESSED:
            if (!raw_pressed) {
                fsm->ticks = 0U;
                fsm->state = BTN_DEBOUNCE_RELEASE;
            }
            break;
        case BTN_DEBOUNCE_RELEASE:
            if (raw_pressed) {
                fsm->state = BTN_PRESSED;
            } else if (++fsm->ticks >= fsm->threshold_ms) {
                fsm->state = BTN_IDLE;
            }
            break;
        default:
            fsm->state = BTN_IDLE;
            break;
    }
    return press_event;
}
```

### 8.3 Hardware Configuration Lock Sequence (`GPIOx->LCKR`)
```c
bool GPIO_LockPins(GPIO_TypeDef *port, uint16_t pin_mask)
{
    uint32_t mask = (uint32_t)pin_mask;
    port->LCKR = GPIO_LCKR_LCKK | mask; /* Step 1: LCKK=1 + mask */
    port->LCKR = mask;                  /* Step 2: LCKK=0 + mask */
    port->LCKR = GPIO_LCKR_LCKK | mask; /* Step 3: LCKK=1 + mask */
    (void)port->LCKR;                   /* Step 4: Read LCKR */
    return (port->LCKR & GPIO_LCKR_LCKK) != 0U;
}
```

### 8.4 I2C1 1 MHz Fast-Mode Plus (20 mA Drive via `SYSCFG->CFGR1`, `GPIO-FMP-001`)

> **Verification Status:** **Compile-Verified (`arm-none-eabi-gcc 14.3.1`, `-O2 -Wall -Wextra -Werror -Wconversion -Wdouble-promotion -Wvla`)** against `stm32g474xx.h` (Not Tested on Physical Hardware). Uses `PA15` (`I2C1_SCL`, AF4) and `PB7` (`I2C1_SDA`, AF4) — both `FT_f` pads — avoiding the NUCLEO-G474RE `PB8-BOOT0` 10 kΩ pull-down!

```c
#include "stm32g474xx.h"

void I2C1_FastModePlus_Init_PA15_PB7(void)
{
    /* 1. Enable SYSCFG (APB2), GPIOA/GPIOB (AHB2), and I2C1 (APB1) + sync readbacks */
    RCC->APB2ENR  |= RCC_APB2ENR_SYSCFGEN;
    (void)RCC->APB2ENR;
    RCC->AHB2ENR  |= RCC_AHB2ENR_GPIOAEN | RCC_AHB2ENR_GPIOBEN;
    (void)RCC->AHB2ENR;
    RCC->APB1ENR1 |= RCC_APB1ENR1_I2C1EN;
    (void)RCC->APB1ENR1;

    /* 2. Enable 20 mA Fast-Mode Plus drive on I2C1 and PB7 via SYSCFG->CFGR1 */
    SYSCFG->CFGR1 |= SYSCFG_CFGR1_I2C1_FMP | SYSCFG_CFGR1_I2C_PB7_FMP;

    /* 3. PA15 (I2C1_SCL, AF4) & PB7 (I2C1_SDA, AF4): Open-Drain, High Speed, No internal pull */
    GPIOA->OTYPER  |=  GPIO_OTYPER_OT15;
    GPIOA->OSPEEDR  = (GPIOA->OSPEEDR & ~GPIO_OSPEEDR_OSPEED15_Msk) | (2UL << GPIO_OSPEEDR_OSPEED15_Pos);
    GPIOA->PUPDR   &= ~GPIO_PUPDR_PUPD15_Msk;
    GPIOA->AFR[1]   = (GPIOA->AFR[1]  & ~(0xFUL << ((15U - 8U) * 4U))) | (4UL << ((15U - 8U) * 4U));
    GPIOA->MODER    = (GPIOA->MODER   & ~GPIO_MODER_MODE15_Msk) | (2UL << GPIO_MODER_MODE15_Pos);

    GPIOB->OTYPER  |=  GPIO_OTYPER_OT7;
    GPIOB->OSPEEDR  = (GPIOB->OSPEEDR & ~GPIO_OSPEEDR_OSPEED7_Msk) | (2UL << GPIO_OSPEEDR_OSPEED7_Pos);
    GPIOB->PUPDR   &= ~GPIO_PUPDR_PUPD7_Msk;
    GPIOB->AFR[0]   = (GPIOB->AFR[0]  & ~(0xFUL << (7U * 4U))) | (4UL << (7U * 4U));
    GPIOB->MODER    = (GPIOB->MODER   & ~GPIO_MODER_MODE7_Msk) | (2UL << GPIO_MODER_MODE7_Pos);

    /* 4. Clock I2C1 from HSI16 (16 MHz) for 1 MHz Fm+ (PRESC=0, SCLL=9, SCLH=3) */
    RCC->CCIPR = (RCC->CCIPR & ~RCC_CCIPR_I2C1SEL_Msk) | RCC_CCIPR_I2C1SEL_1;
    I2C1->CR1     = 0UL;
    I2C1->TIMINGR = (0UL << I2C_TIMINGR_PRESC_Pos)
                  | (2UL << I2C_TIMINGR_SCLDEL_Pos)
                  | (0UL << I2C_TIMINGR_SDADEL_Pos)
                  | (3UL << I2C_TIMINGR_SCLH_Pos)
                  | (9UL << I2C_TIMINGR_SCLL_Pos);
    I2C1->CR1     = I2C_CR1_PE;
}
```

---

## 9. Practical Laboratory Experiments & Debugging Checkpoints

- **Lab 01 (Level 1, 16 MHz HSI16):** [`lab-01-baremetal-gpio-led-blinky`](../18-practical-laboratory/lab-01-baremetal-gpio-led-blinky/README.md) — `RCC->AHB2ENR` + dummy read, `MODER`, atomic `BSRR` blinky.
- **Lab 02 (Level 1, 16 MHz HSI16):** [`lab-02-gpio-pushbutton-debouncing`](../18-practical-laboratory/lab-02-gpio-pushbutton-debouncing/README.md) — Active-HIGH `PC13` input + non-blocking 4-state debounce FSM.
- **GDB Verification Checkpoint:**
  ```text
  (gdb) x/4wx 0x48000000  # Inspect GPIOA MODER (reset 0xABFFFFFF), OTYPER, OSPEEDR, PUPDR
  (gdb) x/1wx 0x48000810  # Read GPIOC->IDR while holding B1 -> verify bit 13 (0x2000) == 1
  (gdb) x/1wx 0x4800001C  # After GPIO_LockPins(GPIOA, 1<<5), verify LCKR == 0x00010020
  ```
- **Oscilloscope Checkpoint:** Toggle `PA5` at `OSPEEDR = 00` vs `11`; measure 10%–90% rise time ($t_r$) and overshoot/ringing with a 10x probe + short ground spring.

---

## 10. Common Engineering Mistakes, Hardware Traps & Silicon Errata (ES0430)

1. **Forgetting STM32G4 Analog Reset State (`MODER = 11b`):** Trying to read `PC13` without clearing `GPIOC->MODER[27:26]` always reads `0`.
2. **NUCLEO-F4 Active-LOW Button Copy-Paste:** On NUCLEO-G474RE (UM2505), `B1` (`PC13`) is **Active-HIGH** with external pull-down. Inverting polarity triggers on release or boot-loops immediately.
3. **Leaving `PA9`/`PA10` UCPD Dead-Battery Pull-Downs Enabled:** Forgetting `PWR->CR3 |= PWR_CR3_UCPD_DBDIS` loads `PA9`/`PA10` with 5.1 kΩ to GND!
4. **Using `PB8` Open-Drain Without External Pull-Up:** External 10 kΩ `R36` (`BOOT0`) pulls `PB8` down to 0.66 V against internal pull-up.
5. **Overwriting SWD Debug Pins (`PA13`/`PA14`):** Writing `GPIOA->MODER = 0x00000400;` wipes bits `[29:26]` and immediately locks out ST-LINK SWD! Always use read-modify-write masking on `MODER` and never touch `PA13`/`PA14` unless intentional.
6. **ES0430 §2.2.1 Erratum:** Do not rely on full JTAG without `NJTRST` (`PB4`); always use 2-wire **SWD** (`PA13`/`PA14`).

---

## 11. Progressive Coding Exercises & Design Challenges

1. **[Level 1]** Write a single atomic `GPIOA->BSRR` statement that simultaneously sets `PA5` and `PA6` HIGH while forcing `PA7` and `PA8` LOW.
2. **[Level 1]** Configure `PA9` as Push-Pull Output *with* and *without* `PWR->CR3 |= PWR_CR3_UCPD_DBDIS` and measure pin voltage under a `10 kΩ` pull-up in input mode to prove the 5.1 kΩ dead-battery resistor behavior.
3. **[Level 2]** Reproduce an `ODR` RMW race condition between `main()` toggling `PA5` via `GPIOA->ODR ^= (1U<<5)` and a 100 kHz timer ISR toggling `PA6`, then eliminate 100% of dropped toggles using `BSRR`.
4. **[Level 2]** Implement a 16-button parallel vertical-counter debouncer using bitwise operations (`cnt0`, `cnt1`, `state`) running in $< 15$ CPU cycles per tick.

---

## 12. Self-Assessment Mastery Questions

1. **Q:** Why does STM32G4 reset `GPIOx->MODER` to `11b` (Analog) instead of `00b` (Digital Input)?  
   *Answer Check:* Disables the CMOS Schmitt trigger so unconnected floating pins cannot float near $V_{\text{DD}}/2$ and draw hundreds of microamps of shoot-through leakage current.
2. **Q:** Why is `GPIOA->MODER` reset value `0xABFFFFFF` instead of `0xFFFFFFFF`?  
   *Answer Check:* `PA13` (SWDIO), `PA14` (SWCLK), and `PA15` (JTDI) reset to `10b` (Alternate Function AF0) so the SWD/JTAG debugger can attach immediately after reset.
3. **Q:** Can you use `GPIOE` (`PE0–PE15`) on NUCLEO-G474RE?  
   *Answer Check:* **No.** `GPIOE` exists in `stm32g474xx.h` for larger packages (LQFP100+), but STM32G474RET6 (LQFP64) has **0 bonded Port E pins**.
4. **Q:** If bit 5 (`BS5`) and bit 21 (`BR5`) of `GPIOA->BSRR` are both written `1` simultaneously, what is the resulting state of `PA5`?  
   *Answer Check:* **HIGH (`1`)** — Set (`BSy`) has hardware priority over Reset (`BRy`) per RM0440 §9.4.7.
5. **Q:** (`GPIO-FMP-001`) Why does enabling `SYSCFG->CFGR1 |= SYSCFG_CFGR1_I2C1_FMP` silently fail if `RCC->APB2ENR.SYSCFGEN` was not set first, and what is the difference between `I2C_PB7_FMP` (bit 17) and `I2C1_FMP` (bit 20)?  
   *Answer Check:* Without `SYSCFGEN` + readback, APB2 writes to `SYSCFG` are ignored. `I2C_PB7_FMP` forces 20 mA drive on `PB7` regardless of mode (even GPIO output), whereas `I2C1_FMP` enables 20 mA drive on whichever `FT_f` pins are AF-mapped to `I2C1`.

---

## 13. Measurable Mastery & Completion Criteria

| Concept ID | Pass / Fail Verification Criterion | Measurement Tool |
|---|---|---|
| `PERIPH-GPIO-001` | Verify `GPIOA->MODER == 0xABFFFFFF` at reset; configure PP, OD, Input, AF, Analog; confirm `IDR` reads `0` in Analog vs `1` in Input at 3.3 V | GDB (`x/4wx 0x48000000`) |
| `PERIPH-GPIO-002` | Measure 10%–90% rise time & overshoot on `PA5` across `OSPEEDR = 00..11` matching DS12288 AC table within $\pm 20\%$ | Oscilloscope (10x probe) |
| `PERIPH-GPIO-003` | Eliminate 100% of multi-context pin glitches over 10M cycles using `BSRR`/`BRR` | Logic analyzer |
| `PERIPH-GPIO-004` | Non-blocking 4-state debounce FSM produces exactly 1 event per press on Active-HIGH `PC13` with 0 blocking delays | Debugger / Logic analyzer |
| `PERIPH-GPIO-005` | Execute 4-step `LCKR` sequence; verify `LCKR[16]==1` and subsequent `MODER` writes are ignored until reset | GDB |
| `GPIO-FMP-001` | Enable `SYSCFGEN` + `SYSCFG->CFGR1` (`I2C1_FMP`/`I2C_PB7_FMP`); verify `CFGR1` bits `[23:16]` in GDB and measure $V_{OL} \le 0.4\text{ V}$ & $t_r \le 120\text{ ns}$ at 1 MHz Fm+ with $470\text{--}680\ \Omega$ pull-ups | GDB + Oscilloscope |
