# USART / UART Register Map & Baud Rate Engine

**Target MCU:** STM32G474RE  
**USART2 Base Address:** `0x40004400` (`APB1PERIPH_BASE + 0x4400UL`) — *Connected to NUCLEO ST-LINK VCP (PA2/PA3)*  
**USART1 Base Address:** `0x40013800` (`APB2PERIPH_BASE + 0x3800UL`)  
**Reference Manual:** RM0440, Chapter 34 (*USART*)

---

### 1. Register Overview Table

| Register | Offset | Reset Value | Description |
|---|---|---|---|
| `USART_CR1` | `0x00` | `0x00000000` | Control register 1 (UE enable, TE transmit, RE receive, RXNEIE, FIFOEN) |
| `USART_CR2` | `0x04` | `0x00000000` | Control register 2 (STOP bits: 1, 0.5, 2, 1.5) |
| `USART_CR3` | `0x08` | `0x00000000` | Control register 3 (DMAT DMA transmit enable, DMAR DMA receive enable) |
| `USART_BRR` | `0x0C` | `0x00000000` | Baud rate register (Baud rate divisor $\text{USARTDIV}$) |
| `USART_ISR` | `0x1C` | `0x000000C0` | Interrupt & status register (TXE, TC, RXNE, ORE, FE, PE) |
| `USART_ICR` | `0x20` | `0x00000000` | Interrupt flag clear register (Write `1` to clear flags) |
| `USART_RDR` | `0x24` | `0x00000000` | Receive data register (Read data buffer) |
| `USART_TDR` | `0x28` | `0x00000000` | Transmit data register (Write data buffer) |

---

### 2. Baud Rate Divisor Mathematics

The baud rate register (`BRR`) value depends on the USART peripheral clock ($f_{\text{CK}}$) and the oversampling setting (`OVER8` bit in `USART_CR1`).

**Case 1: Standard Oversampling by 16 (`OVER8 = 0` in `USART_CR1`):**

$$\text{BRR} = \frac{f_{\text{CK}}}{\text{Baud Rate}}$$

**Worked Example for 115,200 Baud on NUCLEO-G474RE:**
- Clock Source ($f_{\text{CK}}$): PCLK1 = 170,000,000 Hz.
- Target Baud: 115,200 bps.
- Calculation:
  $$\text{BRR} = \frac{170,000,000}{115,200} \approx 1475.69 \rightarrow \text{Round to } 1476 \text{ (\texttt{0x05C4})}$$
- Actual Baud Rate Achieved:
  $$\text{Actual Baud} = \frac{170,000,000}{1476} \approx 115,176.15 \text{ bps (Error: } -0.02\% \text{ - well within the } \pm 2\% \text{ tolerance limit)}$$

---

### 3. Critical Status Flags in `USART_ISR`
- **Bit 5 (`RXNE`):** Read data register not empty (Set by hardware when a received byte is ready in `RDR`; cleared automatically when `RDR` is read).
- **Bit 6 (`TC`):** Transmission complete (Set when the last frame has completely shifted out of the hardware shift register).
- **Bit 7 (`TXE`):** Transmit data register empty (Set when content of `TDR` has been transferred into the internal shift register; ready for next byte).
- **Bit 3 (`ORE`):** Overrun error (Set when a new byte arrives before the previous byte was read from `RDR`).

---

### 4. CMSIS Code Example: Bare-Metal Polling Driver on USART2 (PA2/PA3)
```c
void USART2_Init_115200(void) {
    /* 1. Enable USART2 clock (APB1) and GPIOA clock (AHB2) */
    RCC->APB1ENR1 |= RCC_APB1ENR1_USART2EN;
    RCC->AHB2ENR  |= RCC_AHB2ENR_GPIOAEN;

    /* 2. Configure PA2 (TX) and PA3 (RX) as Alternate Function AF7 */
    GPIOA->MODER &= ~((3U << (2U*2U)) | (3U << (3U*2U)));
    GPIOA->MODER |=  ((2U << (2U*2U)) | (2U << (3U*2U)));
    
    GPIOA->AFR[0] &= ~((0xFU << (2U*4U)) | (0xFU << (3U*4U)));
    GPIOA->AFR[0] |=  ((7U   << (2U*4U)) | (7U   << (3U*4U)));

    /* 3. Program Baud Rate: 170 MHz / 115200 = 1476 (0x05C4) */
    USART2->BRR = 1476U;

    /* 4. Enable Transmitter (TE), Receiver (RE), and USART (UE) */
    USART2->CR1 = USART_CR1_TE | USART_CR1_RE | USART_CR1_UE;
}

void USART2_SendChar(char c) {
    /* Wait until Transmit Data Register is Empty (TXE flag) */
    while (!(USART2->ISR & USART_ISR_TXE));
    USART2->TDR = (uint8_t)c;
}

char USART2_ReceiveChar(void) {
    /* Wait until data is received (RXNE flag) */
    while (!(USART2->ISR & USART_ISR_RXNE));
    return (char)(USART2->RDR & 0xFF);
}
```
