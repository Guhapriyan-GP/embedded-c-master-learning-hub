# SPI & I2C Register Maps

**Target MCU:** STM32G474RE  
**SPI1 Base:** `0x40013000` (`APB2PERIPH_BASE + 0x3000UL`)  
**I2C1 Base:** `0x40005400` (`APB1PERIPH_BASE + 0x5400UL`)  
**Reference Manual:** RM0440, Chapter 35 (*SPI*) & Chapter 36 (*I2C*)

---

### 1. SPI (Serial Peripheral Interface) Register Map

| Register | Offset | Reset Value | Description |
|---|---|---|---|
| `SPI_CR1` | `0x00` | `0x00000000` | Control register 1 (MSTR master mode, CPOL, CPHA, BR baud prescaler, SPE enable) |
| `SPI_CR2` | `0x04` | `0x00000700` | Control register 2 (DS data size: 4-16 bits, FRXTH FIFO reception threshold) |
| `SPI_SR` | `0x08` | `0x00000002` | Status register (TXE transmit empty, RXNE receive not empty, BSY busy flag) |
| `SPI_DR` | `0x0C` | `0x00000000` | Data register (Transmit and receive FIFO buffer interface) |

#### SPI Mode Matrix
- **Mode 0 (`CPOL = 0, CPHA = 0`):** SCK idle LOW; data captured on first (rising) clock edge.
- **Mode 1 (`CPOL = 0, CPHA = 1`):** SCK idle LOW; data captured on second (falling) clock edge.
- **Mode 2 (`CPOL = 1, CPHA = 0`):** SCK idle HIGH; data captured on first (falling) clock edge.
- **Mode 3 (`CPOL = 1, CPHA = 1`):** SCK idle HIGH; data captured on second (rising) clock edge.

---

### 2. I2C (Inter-Integrated Circuit) Register Map

The STM32G4 features an advanced I2C peripheral that decouples bus timing from core clock frequency via `I2C_TIMINGR`.

| Register | Offset | Reset Value | Description |
|---|---|---|---|
| `I2C_CR1` | `0x00` | `0x00000000` | Control register 1 (PE peripheral enable, ANFOFF analog filter, NOSTRETCH) |
| `I2C_CR2` | `0x04` | `0x00000000` | Control register 2 (SADD slave address, NBYTES count, START, STOP, AUTOEND) |
| `I2C_TIMINGR`| `0x10`| `0x00000000` | Timing register (PRESC prescaler, SCLDEL, SDADEL, SCLH high time, SCLL low time) |
| `I2C_ISR` | `0x18` | `0x00000001` | Interrupt and status register (TXIS, RXNE, NACKF, STOPF, TC, BSY) |
| `I2C_ICR` | `0x1C` | `0x00000000` | Interrupt clear register |
| `I2C_RXDR` | `0x24` | `0x00000000` | Receive data register |
| `I2C_TXDR` | `0x28` | `0x00000000` | Transmit data register |

---

### 3. CMSIS Code Examples

#### Example 1: Full-Duplex SPI1 Master Transmit/Receive
```c
uint8_t SPI1_Transfer(uint8_t tx_data) {
    /* 1. Wait until Transmit buffer is empty (TXE = 1) */
    while (!(SPI1->SR & SPI_SR_TXE));
    
    /* 2. Write 8-bit data to Data Register (8-bit access via uint8_t pointer) */
    *(__IO uint8_t *)&SPI1->DR = tx_data;
    
    /* 3. Wait until Receive buffer is not empty (RXNE = 1) */
    while (!(SPI1->SR & SPI_SR_RXNE));
    
    /* 4. Read received byte */
    return *(__IO uint8_t *)&SPI1->DR;
}
```

#### Example 2: I2C1 Master Single-Byte Write
```c
void I2C1_WriteByte(uint8_t dev_addr, uint8_t reg_addr, uint8_t data) {
    /* 1. Configure Slave Address, Transfer 2 Bytes (reg + data), Generate START */
    I2C1->CR2 = (dev_addr & 0xFE) | (2U << I2C_CR2_NBYTES_Pos) | I2C_CR2_AUTOEND | I2C_CR2_START;
    
    /* 2. Send Register Address */
    while (!(I2C1->ISR & I2C_ISR_TXIS));
    I2C1->TXDR = reg_addr;
    
    /* 3. Send Data Byte */
    while (!(I2C1->ISR & I2C_ISR_TXIS));
    I2C1->TXDR = data;
    
    /* 4. Wait for STOP flag (AUTOEND generated STOP automatically) */
    while (!(I2C1->ISR & I2C_ISR_STOPF));
    I2C1->ICR = I2C_ICR_STOPCF; /* Clear STOP flag */
}
```
