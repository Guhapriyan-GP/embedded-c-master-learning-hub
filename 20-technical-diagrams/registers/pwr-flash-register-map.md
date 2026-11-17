# PWR & FLASH Register Maps (Boost Mode & Latency)

**Target MCU:** STM32G474RE  
**PWR Base Address:** `0x40007000` (`APB1PERIPH_BASE + 0x7000UL`)  
**FLASH Interface Base:** `0x40022000` (`AHB1PERIPH_BASE + 0x2000UL`)  
**Reference Manual:** RM0440, Section 5 (*PWR*) & Section 3 (*Embedded Flash*)

---

### 1. PWR (Power Control) Engine & Core Voltage Scaling

Operating the STM32G4 at its maximum rated frequency of 170 MHz requires configuring the core internal voltage regulator into **Range 1 Boost Mode**. Without this, the maximum allowed clock in Range 1 normal mode is 150 MHz, and exceeding it causes CPU lockup.

#### A. Key PWR Registers
| Register | Offset | Reset Value | Description |
|---|---|---|---|
| `PWR_CR1` | `0x00` | `0x00000200` | Power control register 1 (Low-power mode selection, VOS voltage scaling) |
| `PWR_CR5` | `0x10` | `0x00000100` | Power control register 5 (Range 1 Boost Mode enable) |
| `PWR_SR1` | `0x14` | `0x00000000` | Power status register 1 (VOSF voltage scaling flag) |

#### B. PWR Critical Bitfields
- **`PWR_CR1` Bits [10:9] (`VOS`):** Voltage scaling range selection (RW).
  - `01`: Range 1 (High performance, up to 150 MHz normal or 170 MHz in Boost mode).
  - `10`: Range 2 (Low-power, up to 26 MHz).
- **`PWR_CR5` Bit 8 (`R1MODE`):** Main regulator Range 1 Boost Mode (RW).
  - `0`: Range 1 Boost Mode enabled (VCORE boosted for up to 170 MHz operation).
  - `1`: Range 1 normal mode (Reset default).
- **`PWR_SR1` Bit 10 (`VOSF`):** Voltage service flag (RO).
  - `0`: Regulator is ready at selected voltage range.
  - `1`: Voltage regulator transition in progress.

---

### 2. FLASH Memory Interface & Wait State Configuration

Flash memory access time is slower than the 170 MHz core clock. The CPU must insert wait states (`LATENCY`) so that memory read data is stable before the bus cycle completes.

#### A. Key FLASH Registers
| Register | Offset | Reset Value | Description |
|---|---|---|---|
| `FLASH_ACR` | `0x00` | `0x00000600` | Access control register (Latency wait states, Prefetch, Instruction/Data cache) |

#### B. `FLASH_ACR` Bitfields
- **Bits [3:0] (`LATENCY`):** Flash memory read access latency (RW).
  - `0000` (0 WS): $0 < \text{HCLK} \le 34 \text{ MHz}$
  - `0001` (1 WS): $34 < \text{HCLK} \le 68 \text{ MHz}$
  - `0010` (2 WS): $68 < \text{HCLK} \le 102 \text{ MHz}$
  - `0011` (3 WS): $102 < \text{HCLK} \le 136 \text{ MHz}$
  - `0100` (4 WS): $136 < \text{HCLK} \le 170 \text{ MHz}$ (Mandatory for 170 MHz Boost Mode!)
- **Bit 8 (`PRFTEN`):** Prefetch buffer enable (RW). Set to `1` to enable instruction lookahead.
- **Bit 9 (`ICEN`):** Instruction cache enable (RW). Set to `1` to eliminate wait-state overhead on loops.
- **Bit 10 (`DCEN`):** Data cache enable (RW). Set to `1` for high-speed literal pool and constant reads.

---

### 3. CMSIS Code Example: Pre-Clock Boost Mode Setup
```c
/* 1. Enable PWR peripheral clock on APB1 */
RCC->APB1ENR1 |= RCC_APB1ENR1_PWREN;

/* 2. Configure Core Voltage Scaling to Range 1 */
PWR->CR1 &= ~PWR_CR1_VOS;
PWR->CR1 |= (1U << PWR_CR1_VOS_Pos); /* Range 1 */

/* 3. Enable Range 1 Boost Mode in PWR_CR5 (Clear bit 8) */
PWR->CR5 &= ~PWR_CR5_R1MODE;

/* 4. Configure Flash Latency: 4 Wait States, enable Prefetch and Caches */
FLASH->ACR &= ~FLASH_ACR_LATENCY;
FLASH->ACR |= FLASH_ACR_LATENCY_4WS;
FLASH->ACR |= (FLASH_ACR_PRFTEN | FLASH_ACR_ICEN | FLASH_ACR_DCEN);

/* 5. Verify Flash Latency status */
while ((FLASH->ACR & FLASH_ACR_LATENCY) != FLASH_ACR_LATENCY_4WS);
```
