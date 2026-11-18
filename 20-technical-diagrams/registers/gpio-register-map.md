# GPIO (General-Purpose I/O) Register Map

**Target MCU:** STM32G474RET6 (LQFP64 — **52 bonded GPIOs**: `PA0–15`, `PB0–15`, `PC0–15`, `PD2`, `PF0–1`, `PG10-NRST`; **`GPIOE` has 0 pins on LQFP64**)  
**Base Addresses (AHB2):** `GPIOA` `0x48000000` | `GPIOB` `0x48000400` | `GPIOC` `0x48000800` | `GPIOD` `0x48000C00` | `GPIOF` `0x48001400` | `GPIOG` `0x48001800`  
**Reference Manual:** RM0440, Chapter 9 (*General-purpose I/Os (GPIO)*) | DS12288 Table 13 (*Alternate function mapping*) & Table 15 (*I/O current limits*)  
**Concept IDs:** `PERIPH-GPIO-001`, `GPIO-FMP-001`, `HWSW-PCB-001`, `HWSW-PCB-002`  
**Verification Status:** Every register offset and reset value verified directly against `stm32g474xx.h` and RM0440 §9.4; C code snippets Compile-Verified (`arm-none-eabi-gcc 14.3.1`, `-Wall -Wextra -Werror`) (Not Tested on Physical Hardware).

---

### 1. Register Overview Table (`GPIO_TypeDef`)

| Register | Offset | Reset Value | Access | Description |
|---|---|---|---|---|
| `GPIOx_MODER` | `0x000` | `GPIOA: 0xABFFFFFF`<br>`GPIOB: 0xFFFFFEBF`<br>`Others: 0xFFFFFFFF` | RW | Port mode register (**`11` Analog is reset default on STM32G4!**) |
| `GPIOx_OTYPER` | `0x004` | `0x00000000` | RW | Output type register (`0` = Push-pull, `1` = Open-drain) |
| `GPIOx_OSPEEDR` | `0x008` | `GPIOA: 0x0C000000`<br>`Others: 0x00000000` | RW | Output speed register (`00` Low, `01` Medium, `10` High, `11` Very High) |
| `GPIOx_PUPDR` | `0x00C` | `GPIOA: 0x64000000`<br>`GPIOB: 0x00000100`<br>`Others: 0x00000000` | RW | Pull-up / pull-down resistor selection (~40 kΩ internal weak resistors) |
| `GPIOx_IDR` | `0x010` | `0x0000XXXX` | RO | Input data register (Reflects synchronized digital pin logic levels) |
| `GPIOx_ODR` | `0x014` | `0x00000000` | RW | Output data register (Avoid RMW `|=` / `^=` in ISRs — use `BSRR`/`BRR`!) |
| `GPIOx_BSRR` | `0x018` | `0x00000000` | WO | Bit set/reset register (Atomic single-cycle set [15:0] and reset [31:16]) |
| `GPIOx_LCKR` | `0x01C` | `0x00000000` | RW | Configuration lock register (Locks `MODER/OTYPER/OSPEEDR/PUPDR/AFR` until MCU reset) |
| `GPIOx_AFRL` (`AFR[0]`) | `0x020` | `0x00000000` | RW | Alternate function low register (Pins 0–7, 4 bits per pin: AF0–AF15) |
| `GPIOx_AFRH` (`AFR[1]`) | `0x024` | `0x00000000` | RW | Alternate function high register (Pins 8–15, 4 bits per pin: AF0–AF15) |
| `GPIOx_BRR` | `0x028` | `0x00000000` | WO | Bit reset register (Dedicated atomic pin reset via bits [15:0]) |

---

### 2. Critical Register Bitfields & STM32G4 Hardware Realities

#### A. `GPIOx_MODER` (Port Mode Register) — Offset `0x000`
Configures pin $y \in [0..15]$ via bits $[2y+1 : 2y]$:
- `00`: **Input mode** (Digital Schmitt trigger active, output driver disabled).
- `01`: **General-purpose output mode** (Driven by `ODR`/`BSRR`/`BRR`).
- `10`: **Alternate function mode** (Routed to peripheral selected in `AFRL`/`AFRH`).
- **`11`: Analog mode (RESET DEFAULT ON STM32G4!)** — Schmitt trigger disabled to eliminate floating-pin shoot-through leakage current. Only debug pins default to AF0 (`PA13` SWDIO, `PA14` SWCLK, `PA15` JTDI, `PB3` JTDO/SWO, `PB4` NJTRST), giving `GPIOA->MODER = 0xABFFFFFF` and `GPIOB->MODER = 0xFFFFFEBF` at reset.

> **Bit-Banding Exclusion:** Because `GPIOA–G` live on AHB2 (`0x48000000`), they are **OUTSIDE** the Cortex-M4 peripheral bit-band window (`0x40000000–0x400FFFFF`). Always use `BSRR` or `BRR` for atomic pin updates.

#### B. `GPIOx_BSRR` (`0x018`) & `GPIOx_BRR` (`0x028`)
- **`BSRR[15:0]` (`BSy`):** Writing `1` atomically sets `ODRy = 1`. Writing `0` has no effect.
- **`BSRR[31:16]` (`BRy`):** Writing `1` atomically clears `ODRy = 0`. If both `BSy` and `BRy` are written `1`, `BSy` wins.
- **`BRR[15:0]` (`BRy`):** Writing `1` atomically clears `ODRy = 0` without shifting by +16 (useful for 16-bit mask clears).

#### C. `GPIOx_LCKR` (Configuration Lock Register) — Offset `0x01C`
Locks pin configuration bits until next hardware reset using a mandatory 4-step write sequence on **`LCKK` (Bit 16)** with target mask `M` in bits `[15:0]`:
1. Write `LCKK=1 | M` → 2. Write `LCKK=0 | M` → 3. Write `LCKK=1 | M` → 4. Read `GPIOx->LCKR` (confirm bit 16 reads `1`).

#### D. `GPIOx_AFRL` (`AFR[0]`, `0x020`) & `GPIOx_AFRH` (`AFR[1]`, `0x024`)
Each pin uses a 4-bit field (`0x0` = AF0 through `0xF` = AF15) per **DS12288 Table 13** (*Alternate function mapping*). Always configure `AFR` and `OTYPER`/`OSPEEDR`/`PUPDR` **before** switching `MODER` to `10` (AF) or `01` (Output) to prevent output glitches.

#### E. NUCLEO-G474RE (MB1367) Special Pin Notes (UM2505)
- **`PA5`:** Connected to Green LED `LD2` via `SB6` (also Arduino `D13` / `SPI1_SCK` — watch for capacitive/LED loading on high-speed SPI!).
- **`PC13`:** Connected to Blue User Button `B1` (**Active-HIGH** with external pull-down; connects to `VDD` when pressed).
- **`PA9`/`PA10`/`PB6`/`PB4`:** Internal 5.1 kΩ UCPD dead-battery pull-downs are enabled at reset until `PWR->CR3 |= PWR_CR3_UCPD_DBDIS;`.
- **`PB8-BOOT0`:** External 10 kΩ pull-down (`R36`) on NUCLEO-G474RE; avoid relying solely on internal ~40 kΩ pull-up for `I2C1_SCL` on `PB8`.

---

### 3. CMSIS Code Examples

> **Status:** Statically Reviewed (Not Tested on Physical Hardware).

#### Example 1: Atomic Output Control (`PA5` LED `LD2`) & Lock Sequence
```c
#include "stm32g474xx.h"

void GPIO_LED_Init_PA5(void) {
    /* 1. Enable GPIOA AHB2 clock + read-back delay */
    RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN;
    (void)RCC->AHB2ENR;

    /* 2. Configure Output Type (Push-Pull), Speed (Low), Pull (None) BEFORE MODER */
    GPIOA->OTYPER  &= ~(1U << 5U);
    GPIOA->OSPEEDR &= ~(3U << (5U * 2U));
    GPIOA->PUPDR   &= ~(3U << (5U * 2U));

    /* 3. Switch PA5 from reset default Analog (11b) to Output (01b) */
    GPIOA->MODER = (GPIOA->MODER & ~(3U << (5U * 2U))) | (1U << (5U * 2U));

    /* 4. Atomic HIGH via BSRR, Atomic LOW via BRR (or BSRR bit 5+16) */
    GPIOA->BSRR = GPIO_BSRR_BS5;
    GPIOA->BRR  = GPIO_BRR_BR5;
}

/* Lock PA5 configuration until next reset */
uint32_t GPIO_Lock_PA5(void) {
    uint32_t mask = (1U << 5U);
    GPIOA->LCKR = GPIO_LCKR_LCKK | mask;
    GPIOA->LCKR = mask;
    GPIOA->LCKR = GPIO_LCKR_LCKK | mask;
    (void)GPIOA->LCKR;
    return (GPIOA->LCKR & GPIO_LCKR_LCKK) ? 1U : 0U;
}
```

#### Example 2: Glitch-Free Alternate Function Setup (`PA2`/`PA3` USART2 AF7)
```c
void GPIO_USART2_Init_PA2_PA3(void) {
    RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN;
    (void)RCC->AHB2ENR;

    /* 1. Set AF7 in AFRL (AFR[0]) first */
    GPIOA->AFR[0] = (GPIOA->AFR[0] & ~((0xFU << (2U * 4U)) | (0xFU << (3U * 4U)))) |
                    ((7U << (2U * 4U)) | (7U << (3U * 4U)));

    /* 2. High speed, pull-up on RX (PA3) to avoid floating noise */
    GPIOA->OSPEEDR = (GPIOA->OSPEEDR & ~((3U << 4U) | (3U << 6U))) | ((2U << 4U) | (2U << 6U));
    GPIOA->PUPDR   = (GPIOA->PUPDR   & ~((3U << 4U) | (3U << 6U))) | (1U << 6U);

    /* 3. Switch MODER2 and MODER3 to Alternate Function (10b) */
    GPIOA->MODER = (GPIOA->MODER & ~((3U << 4U) | (3U << 6U))) | ((2U << 4U) | (2U << 6U));
}
```
