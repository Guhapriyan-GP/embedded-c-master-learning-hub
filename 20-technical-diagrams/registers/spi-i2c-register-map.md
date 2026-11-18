# SPI & I2C Register Maps

**Target MCU:** STM32G474RET6 (LQFP64)  
**SPI Instances:** `SPI1` (`0x40013000`, APB2), `SPI2/I2S2` (`0x40003800`, APB1), `SPI3/I2S3` (`0x40003C00`, APB1) *(Note: `SPI4` @ `0x40013C00` is unbonded on LQFP64 — all pins reside on Port E!)*  
**I2C Instances:** `I2C1` (`0x40005400`), `I2C2` (`0x40005800`), `I2C3` (`0x40007800`), `I2C4` (`0x40008400`) (all APB1)  
**Reference Manual:** RM0440, Chapters 39–42 (Rev 8: I2C Ch 39, SPI Ch 40; Rev 9: I2C Ch 41, SPI Ch 42)  
**Concept IDs:** `PERIPH-SPI-001`, `PERIPH-I2C-001`  
**Verification Status:** Every register offset and bit position verified directly against `stm32g474xx.h`; code snippets Statically Reviewed (Not Tested on Physical Hardware).

---

### 1. SPI / I2S Register Map (`SPI_TypeDef`)

| Register | Offset | Reset Value | Access | Description |
|---|---|---|---|---|
| `SPI_CR1` | `0x000` | `0x00000000` | RW | Control 1 (`CPHA`, `CPOL`, `MSTR`, `BR[5:3]`, `SPE`, `LSBFIRST`, `SSI`, `SSM`, `CRCEN`) |
| `SPI_CR2` | `0x004` | `0x00000700` | RW | Control 2 (`RXDMAEN`, `TXDMAEN`, `SSOE`, `NSSP`, `DS[11:8]` 4–16 bit, **`FRXTH` bit 12**) |
| `SPI_SR` | `0x008` | `0x00000002` | RO/rc_w0 | Status (`RXNE`, `TXE`, `CRCERR`, `MODF`, `OVR`, `BSY`, `FRLVL[10:9]`, `FTLVL[12:11]`) |
| `SPI_DR` | `0x00C` | `0x00000000` | RW | Data register (32-bit TX/RX FIFO interface; access width controls FIFO packing!) |
| `SPI_CRCPR` | `0x010` | `0x00000007` | RW | Hardware CRC polynomial register |
| `SPI_RXCRCR` | `0x014` | `0x00000000` | RO | Computed RX CRC register |
| `SPI_TXCRCR` | `0x018` | `0x00000000` | RO | Computed TX CRC register |
| `SPI_I2SCFGR` | `0x01C` | `0x00000000` | RW | I2S configuration (`SPI2` & `SPI3` only) |
| `SPI_I2SPR` | `0x020` | `0x00000002` | RW | I2S prescaler (`SPI2` & `SPI3` only) |

#### Critical STM32G4 SPI FIFO Rules (`SPI_CR2.FRXTH` — Bit 12)
STM32G4 implements **32-bit TX/RX FIFOs** with FIFO packing:
1. **MANDATORY for ≤ 8-bit frames (`DS = 0111b`):** Set **`SPI_CR2.FRXTH = 1`** (bit 12 = 1/4 FIFO = 8-bit RX threshold). If `FRXTH = 0` (reset default!), `RXNE` will **NOT** assert after a single 8-bit transfer — it waits for 16 bits, causing polling loops to **hang forever**!
2. **8-Bit Pointer Access:** Always read/write 8-bit bytes via `*(__IO uint8_t *)&SPIx->DR`. Writing a 16-bit value to `SPIx->DR` pushes **two** bytes into the TX FIFO (FIFO packing).
3. **SPI Modes:** Mode 0 (`CPOL=0, CPHA=0`), Mode 1 (`0,1`), Mode 2 (`1,0`), Mode 3 (`1,1`).
4. **NUCLEO-G474RE Warning:** `PA5` (`SPI1_SCK`) is connected via `SB6` to onboard green LED `LD2`; account for LED loading at >10 MHz or route `SPI1_SCK` to `PB3` (AF5) if SWO is unused.

---

### 2. I2C Register Map (`I2C_TypeDef`)

> **Register Name Note:** STM32G4 I2C uses **`I2C_RXDR` (`0x24`)** and **`I2C_TXDR` (`0x28`)** — NOT `DR`, `RDR`, or `TDR`!

| Register | Offset | Reset Value | Access | Description |
|---|---|---|---|---|
| `I2C_CR1` | `0x000` | `0x00000000` | RW | Control 1 (`PE`, `TXIE`, `RXIE`, `NACKIE`, `STOPIE`, `TCIE`, `DNF[11:8]`, `ANFOFF`, `TXDMAEN`, `RXDMAEN`, `NOSTRETCH`) |
| `I2C_CR2` | `0x004` | `0x00000000` | RW | Control 2 (`SADD[9:0]`, `RD_WRN` b10, `ADD10` b11, `START` b13, `STOP` b14, **`NBYTES[23:16]`**, **`RELOAD` b24**, **`AUTOEND` b25**) |
| `I2C_OAR1/2` | `0x008/0x00C` | `0x00000000` | RW | Own address 1 & 2 (Slave mode) |
| `I2C_TIMINGR` | `0x010` | `0x00000000` | RW | Bus timing (`SCLL[7:0]`, `SCLH[15:8]`, `SDADEL[19:16]`, `SCLDEL[23:20]`, `PRESC[31:28]`) — configure only when `PE=0` |
| `I2C_TIMEOUTR` | `0x014` | `0x00000000` | RW | Hardware SMBus/clock-stretching timeout register |
| `I2C_ISR` | `0x018` | `0x00000001` | RO/RW | Status (`TXE`, `TXIS`, `RXNE`, `NACKF`, `STOPF`, `TC`, `TCR`, `BERR`, `ARLO`, `OVR`, `BUSY`) |
| `I2C_ICR` | `0x01C` | `0x00000000` | WO | Interrupt clear (`ADDRCF`, `NACKCF`, `STOPCF`, `BERRCF`, `ARLOCF`, `OVRCF`) |
| `I2C_PECR` | `0x020` | `0x00000000` | RO | Hardware SMBus Packet Error Checking register |
| `I2C_RXDR` | `0x024` | `0x00000000` | RO | Receive data register (Reading clears `RXNE`) |
| `I2C_TXDR` | `0x028` | `0x00000000` | RW | Transmit data register (Writing clears `TXIS`) |

#### `I2C_CR2` Autonomous Transaction Control (`NBYTES`, `AUTOEND`, `RELOAD`)
- **`NBYTES[23:16]`:** Number of bytes (1–255) for the current transfer phase.
- **`AUTOEND = 1` (Bit 25):** Hardware automatically transmits `STOP` after `NBYTES` complete (use for single-phase writes and the final read phase).
- **`AUTOEND = 0`:** Hardware holds `SCL` LOW and sets `ISR.TC = 1` after `NBYTES` complete — ready for a **Repeated START** (`START = 1` with `RD_WRN = 1`).
- **`RELOAD = 1` (Bit 24):** Used for transfers > 255 bytes; hardware sets `ISR.TCR = 1` when `NBYTES` reaches 0, letting software reload `NBYTES` seamlessly without a `STOP`.
- **NUCLEO-G474RE Hardware Warning:** `PB8` (`I2C1_SCL`) is shared with `PB8-BOOT0` (external 10 kΩ pull-down `R36` on MB1367). Always use **external 2.2–4.7 kΩ pull-ups** to 3.3 V on `SCL`/`SDA` (or route `I2C1` to `PA15`/`PB7` AF4). Also note `RCC->CCIPR.I2C1SEL` can select `HSI16` (16 MHz) for stable, SYSCLK-independent `I2C_TIMINGR` values!

---

### 3. CMSIS Code Examples

> **Status:** Statically Reviewed (Not Tested on Physical Hardware).

#### Example 1: 8-Bit `SPI1` Master Init (`FRXTH = 1`) & Timeout-Safe Transfer
```c
#include "stm32g474xx.h"

void SPI1_Master_Init_8Bit(void) {
    RCC->APB2ENR |= RCC_APB2ENR_SPI1EN;
    (void)RCC->APB2ENR;

    SPI1->CR1 = 0U; /* Disable SPE before configuration */
    /* Master, Mode 0 (CPOL=0, CPHA=0), BR = /16 (10.625 MHz @ 170 MHz), Software NSS */
    SPI1->CR1 = SPI_CR1_MSTR | (3U << SPI_CR1_BR_Pos) | SPI_CR1_SSM | SPI_CR1_SSI;
    /* CRITICAL: 8-bit frame (DS = 0111b) AND FRXTH = 1 (8-bit RXNE threshold!) */
    SPI1->CR2 = (7U << SPI_CR2_DS_Pos) | SPI_CR2_FRXTH;
    SPI1->CR1 |= SPI_CR1_SPE;
}

int SPI1_TransferByte(uint8_t tx, uint8_t *rx, uint32_t timeout) {
    uint32_t t = timeout;
    while (!(SPI1->SR & SPI_SR_TXE)) { if (t-- == 0U) { return -1; } }
    *(__IO uint8_t *)&SPI1->DR = tx; /* 8-bit write avoids FIFO packing */

    t = timeout;
    while (!(SPI1->SR & SPI_SR_RXNE)) { if (t-- == 0U) { return -2; } }
    *rx = *(__IO uint8_t *)&SPI1->DR;
    return 0;
}
```

#### Example 2: `I2C1` Master Repeated-START Register Read (NACK & Timeout Protected)
```c
int I2C1_ReadReg(uint8_t addr7, uint8_t reg, uint8_t *buf, uint8_t len, uint32_t timeout) {
    uint32_t t = timeout;
    while (I2C1->ISR & I2C_ISR_BUSY) { if (t-- == 0U) { return -1; } }

    /* Phase 1: Write 1-byte register address, AUTOEND = 0 (hold bus for Repeated START) */
    I2C1->ICR = I2C_ICR_NACKCF | I2C_ICR_STOPCF;
    I2C1->CR2 = ((uint32_t)(addr7 << 1U) & I2C_CR2_SADD_Msk) |
                (1U << I2C_CR2_NBYTES_Pos) | I2C_CR2_START;

    t = timeout;
    while (!(I2C1->ISR & (I2C_ISR_TXIS | I2C_ISR_NACKF))) { if (t-- == 0U) { return -2; } }
    if (I2C1->ISR & I2C_ISR_NACKF) { I2C1->ICR = I2C_ICR_NACKCF; return -3; }
    I2C1->TXDR = reg;

    t = timeout;
    while (!(I2C1->ISR & I2C_ISR_TC)) { if (t-- == 0U) { return -4; } }

    /* Phase 2: Repeated START, RD_WRN = 1, NBYTES = len, AUTOEND = 1 */
    I2C1->CR2 = ((uint32_t)(addr7 << 1U) & I2C_CR2_SADD_Msk) |
                I2C_CR2_RD_WRN | ((uint32_t)len << I2C_CR2_NBYTES_Pos) |
                I2C_CR2_AUTOEND | I2C_CR2_START;

    for (uint8_t i = 0U; i < len; i++) {
        t = timeout;
        while (!(I2C1->ISR & (I2C_ISR_RXNE | I2C_ISR_NACKF))) { if (t-- == 0U) { return -5; } }
        if (I2C1->ISR & I2C_ISR_NACKF) { I2C1->ICR = I2C_ICR_NACKCF; return -7; }
        buf[i] = (uint8_t)I2C1->RXDR;
    }

    t = timeout;
    while (!(I2C1->ISR & I2C_ISR_STOPF)) { if (t-- == 0U) { return -6; } }
    I2C1->ICR = I2C_ICR_STOPCF;
    return 0;
}
```
