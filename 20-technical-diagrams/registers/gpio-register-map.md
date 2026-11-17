# GPIO (General-Purpose I/O) Register Map

**Target MCU:** STM32G474RE  
**GPIOA Base Address:** `0x48000000` (`AHB2PERIPH_BASE + 0x0000UL`)  
**GPIOB Base:** `0x48000400` | **GPIOC Base:** `0x48000800`  
**Bus:** AHB2  
**Reference Manual:** RM0440, Section 9.4 (*GPIO registers*)

---

### 1. Register Overview Table

| Register | Offset | Reset Value | Description |
|---|---|---|---|
| `GPIOx_MODER` | `0x00` | Device Specific | Port mode register (Input, Output, Alternate Function, Analog) |
| `GPIOx_OTYPER` | `0x04` | `0x00000000` | Output type register (Push-pull vs Open-drain) |
| `GPIOx_OSPEEDR`| `0x08` | Device Specific | Output speed register (Low, Medium, High, Very High) |
| `GPIOx_PUPDR` | `0x0C` | Device Specific | Pull-up / pull-down resistor selection register |
| `GPIOx_IDR` | `0x10` | Read Only | Input data register (Reflects physical pin logic levels) |
| `GPIOx_ODR` | `0x14` | `0x00000000` | Output data register (Sets output logic levels when in output mode) |
| `GPIOx_BSRR` | `0x18` | `0x00000000` | Bit set/reset register (Atomic single-cycle pin manipulation) |
| `GPIOx_LCKR` | `0x1C` | `0x00000000` | Configuration lock register |
| `GPIOx_AFRL` | `0x20` | `0x00000000` | Alternate function low register (Pins 0 to 7, 4 bits per pin) |
| `GPIOx_AFRH` | `0x24` | `0x00000000` | Alternate function high register (Pins 8 to 15, 4 bits per pin) |
| `GPIOx_BRR` | `0x28` | `0x00000000` | Bit reset register (Dedicated atomic pin reset) |

---

### 2. Critical Register Bitfields & Encodings

#### A. `GPIOx_MODER` (Port Mode Register) — Offset `0x00`
Configures the functional state of pin $y$ ($y \in [0..15]$) using bits $[2y+1 : 2y]$:
- `00`: **Input mode** (Reset default for most pins; digital input buffer active, output buffer disabled).
- `01`: **General-purpose output mode** (Push-pull or Open-drain driven by `ODR`/`BSRR`).
- `10`: **Alternate function mode** (Pin is routed to internal peripheral such as USART, SPI, TIM).
- `11`: **Analog mode** (Schmitt trigger input disabled to save power; pin routed directly to ADC/DAC/OPAMP).

#### B. `GPIOx_OTYPER` (Output Type Register) — Offset `0x04`
- **Bit $y$ (`OTy`):**
  - `0`: **Output push-pull** (Actively drives pin HIGH to VDD and LOW to VSS).
  - `1`: **Output open-drain** (Actively drives pin LOW to VSS; floats when HIGH; requires pull-up resistor; mandatory for I2C).

#### C. `GPIOx_BSRR` (Bit Set / Reset Register) — Offset `0x18`
Provides atomic, interrupt-safe control of output pins without requiring Read-Modify-Write instructions:
- **Bits [15:0] (`BSy`):** Bit Set pin $y$. Writing `1` sets the corresponding `ODRy` bit to `1` (Pin goes HIGH). Writing `0` has no effect.
- **Bits [31:16] (`BRy`):** Bit Reset pin $y$. Writing `1` clears the corresponding `ODRy` bit to `0` (Pin goes LOW). Writing `0` has no effect.
*(Note: If both `BSy` and `BRy` are written simultaneously, `BSy` has priority).*

#### D. `GPIOx_AFR` (Alternate Function Registers) — Offsets `0x20`, `0x24`
Pins 0 to 7 are configured in `AFR[0]` (`AFRL`), pins 8 to 15 in `AFR[1]` (`AFRH`). Each pin occupies 4 bits:
- `0000` (AF0) to `1111` (AF15): Corresponds to peripheral multiplexing documented in Datasheet DS12288 Table 14.

---

### 3. CMSIS Code Examples

#### Example 1: Configure User LED (Green on PA5, NUCLEO-G474RE)
```c
/* 1. Enable GPIOA bus clock in AHB2 */
RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN;

/* 2. Configure PA5 as Output (MODER5 = 01b)
 *    Clear bits [11:10], then set bit 10 */
GPIOA->MODER &= ~(3U << (5U * 2U));
GPIOA->MODER |=  (1U << (5U * 2U));

/* 3. Configure Push-Pull, Low-Speed, No-Pull */
GPIOA->OTYPER &= ~(1U << 5U);
GPIOA->OSPEEDR &= ~(3U << (5U * 2U));
GPIOA->PUPDR   &= ~(3U << (5U * 2U));

/* 4. Atomic Turn ON using BSRR (Bit 5) */
GPIOA->BSRR = (1U << 5U);

/* 5. Atomic Turn OFF using BSRR (Bit 21 = 5 + 16) */
GPIOA->BSRR = (1U << (5U + 16U));
```

#### Example 2: Configure USART2 TX on PA2 (Alternate Function AF7)
```c
/* 1. Set PA2 mode to Alternate Function (MODER2 = 10b) */
GPIOA->MODER &= ~(3U << (2U * 2U));
GPIOA->MODER |=  (2U << (2U * 2U));

/* 2. Map PA2 to AF7 in AFRL (Bits [11:8]) */
GPIOA->AFR[0] &= ~(0xFU << (2U * 4U));
GPIOA->AFR[0] |=  (7U   << (2U * 4U));
```
