# USART / UART / LPUART Register Map & Baud Rate Engine

**Target MCU:** STM32G474RE (LQFP64 — 6 instances: `USART1`, `USART2`, `USART3`, `UART4`, `UART5`, `LPUART1`)  
**Base Addresses:** `USART1` `0x40013800` (APB2) | `USART2` `0x40004400` (APB1) | `USART3` `0x40004800` (APB1) | `UART4` `0x40004C00` (APB1) | `UART5` `0x40005000` (APB1) | `LPUART1` `0x40008000` (APB1)  
**NUCLEO-G474RE ST-LINK VCP:** `PA2` (TX) / `PA3` (RX) — usable as **`LPUART1` (AF12)** or **`USART2` (AF7)** (UM2505 §6.8)  
**Reference Manual:** RM0440, Chapter 37 (Rev 8) / Chapter 40 (Rev 9) (*USART*) & Chapter 38 / 41 (*LPUART*)  
**Concept IDs:** `PERIPH-UART-001`, `COMM-LP-001`  
**Verification Status:** Every register offset and bitfield verified directly against `stm32g474xx.h`; C code snippets Compile-Verified (`arm-none-eabi-gcc 14.3.1`, `-Wall -Wextra -Werror`) (Not Tested on Physical Hardware).

---

### 1. Register Overview Table (`USART_TypeDef`)

> **STM32G4 vs. STM32F4 Architecture Note:** STM32G4 USART uses **separate `RDR` (`0x24`) and `TDR` (`0x28`) registers** (NO legacy `DR`), **`ISR` (`0x1C`) / `ICR` (`0x20`)** status/clear registers, an input clock prescaler **`PRESC` (`0x2C`)**, and optional **8-byte TX/RX hardware FIFOs** (`CR1.FIFOEN`).

| Register | Offset | Reset Value | Access | Description |
|---|---|---|---|---|
| `USART_CR1` | `0x000` | `0x00000000` | RW | Control 1 (`UE`, `UESM`, `RE`, `TE`, `IDLEIE`, `RXNEIE_RXFNEIE`, `TCIE`, `TXEIE_TXFNFIE`, `OVER8`, `FIFOEN`) |
| `USART_CR2` | `0x004` | `0x00000000` | RW | Control 2 (`STOP[13:12]`, `SWAP` TX/RX pin swap, `TXINV`/`RXINV`, `ABREN` auto-baud, `RTOEN`) |
| `USART_CR3` | `0x008` | `0x00000000` | RW | Control 3 (`EIE`, `HDSEL`, `DMAR`, `DMAT`, `RTSE`/`CTSE`, `ONEBIT`, `OVRDIS`, `DEM` RS-485 DE, **`WUS[21:20]`** Stop wakeup source, **`WUFIE` b22** Stop wakeup IE) |
| `USART_BRR` | `0x00C` | `0x00000000` | RW | Baud rate register (16-bit for USART1–5; **20-bit** for LPUART1) |
| `USART_GTPR` | `0x010` | `0x00000000` | RW | Guard time and prescaler register (Smartcard / IrDA) |
| `USART_RTOR` | `0x014` | `0x00000000` | RW | Receiver timeout register (Modbus / idle frame detection) |
| `USART_RQR` | `0x018` | `0x00000000` | WO | Request register (`RXFRQ` flush RX, `TXFRQ` flush TX, `ABRRQ`, `SBKRQ`) |
| `USART_ISR` | `0x01C` | `0x020000C0` | RO | Interrupt & status (`PE`, `FE`, `NE`, `ORE`, `IDLE`, `RXNE_RXFNE`, `TC`, `TXE_TXFNF`, `BUSY`, **`WUF` b20** Stop wakeup flag, `TEACK`, `REACK`) |
| `USART_ICR` | `0x020` | `0x00000000` | **WO** | Interrupt flag clear (`PECF`, `FECF`, `NECF`, `ORECF`, `IDLECF`, `TCCF`, `RTOCF`, **`WUCF` b20** — write `1` directly) |
| `USART_RDR` | `0x024` | `0x00000000` | RO | Receive data register (Reading automatically clears `RXNE` when FIFO disabled) |
| `USART_TDR` | `0x028` | `0x00000000` | RW | Transmit data register (Writing automatically clears `TXE` when FIFO disabled) |
| `USART_PRESC` | `0x02C` | `0x00000000` | RW | Clock prescaler (`PRESCALER[3:0]`: `0000`=`/1`, `0001`=`/2`, `0010`=`/4`, `0011`=`/6`, `0100`=`/8`, `0101`=`/10`, `0110`=`/12`, `0111`=`/16`, `1000`=`/32`, `1001`=`/64`, `1010`=`/128`, `1011`=`/256`) |

**IRQ Numbers (`stm32g474xx.h`):** `USART1_IRQn = 37`, `USART2_IRQn = 38`, `USART3_IRQn = 39`, `UART4_IRQn = 52`, `UART5_IRQn = 53`, `LPUART1_IRQn = 91`.  
**DMAMUX1 Request IDs (RM0440 Table 91):** `USART1` RX=`24`/TX=`25` | `USART2` RX=`26`/TX=`27` | `USART3` RX=`28`/TX=`29` | `UART4` RX=`30`/TX=`31` | `UART5` RX=`32`/TX=`33` | `LPUART1` RX=`34`/TX=`35`.

---

### 2. Baud Rate Divisor Mathematics

Let $f_{\text{KER\_PRESC}} = f_{\text{KER}} / \text{PRESC}$ (kernel clock selected via `RCC->CCIPR`: PCLK, SYSCLK, HSI16, or LSE).

1. **USART1–3 / UART4–5 (`OVER8 = 0`, 16× oversampling):**
   $$\text{BRR} = \text{round}\!\left(\frac{f_{\text{KER\_PRESC}}}{\text{Baud}}\right)$$
   - At $f_{\text{CK}} = 170\ \text{MHz}$ (Profile A), `PRESC = /1`, target $115{,}200\ \text{bps}$:
     $$\text{BRR} = \frac{170{,}000{,}000}{115{,}200} = 1475.69 \rightarrow 1476\ (\texttt{0x05C4}), \quad \text{Error} = -0.021\%$$
   - At $f_{\text{CK}} = 160\ \text{MHz}$ (Profile B), `PRESC = /1`, target $115{,}200\ \text{bps}$:
     $$\text{BRR} = \frac{160{,}000{,}000}{115{,}200} = 1388.89 \rightarrow 1389\ (\texttt{0x056D}), \quad \text{Error} = -0.008\%$$
2. **LPUART1 (20-bit fractional BRR, valid when $3 \times \text{Baud} \le f_{\text{KER\_PRESC}} \le 4096 \times \text{Baud}$):**
   $$\text{BRR}_{\text{LPUART}} = \text{round}\!\left(\frac{256 \times f_{\text{KER\_PRESC}}}{\text{Baud}}\right)$$
   - Example: $f_{\text{LSE}} = 32{,}768\ \text{Hz}$ at $9{,}600\ \text{bps}$ (Stop 1 wakeup): $\text{BRR} = \frac{256 \times 32768}{9600} = 874$ (`0x036A`). At 170 MHz and 115,200 bps, use `PRESC = /4` ($42.5\ \text{MHz} \le 4096 \times 115200$) so $\text{BRR} = 94{,}444$.

---

### 3. Critical `USART_ISR` Flags & `USART_ICR` Clearing
- **`RXNE` / `RXFNE` (Bit 5):** Data available in `RDR` (or RX FIFO not empty). Cleared by reading `RDR`.
- **`TC` (Bit 6):** Transmission complete (shift register empty). Clear via `USARTx->ICR = USART_ICR_TCCF;`.
- **`TXE` / `TXFNF` (Bit 7):** `TDR` empty (or TX FIFO not full). Cleared by writing `TDR`.
- **`IDLE` (Bit 4):** Idle frame detected after RX activity (ideal with circular DMA RX for variable-length frames!). Clear via `USARTx->ICR = USART_ICR_IDLECF;`.
- **`ORE` (Bit 3):** Overrun error — **blocks further reception until cleared** via `USARTx->ICR = USART_ICR_ORECF;`.

---

### 4. CMSIS Code Example: Polling & Timeout-Safe `USART2` Driver (`PA2`/`PA3`, 115,200 bps @ 170 MHz)

> **Status:** Statically Reviewed (Not Tested on Physical Hardware).

```c
#include "stm32g474xx.h"

void USART2_Init_115200(uint32_t pclk1_hz) {
    /* 1. Enable USART2 (APB1) and GPIOA (AHB2) clocks */
    RCC->APB1ENR1 |= RCC_APB1ENR1_USART2EN;
    RCC->AHB2ENR  |= RCC_AHB2ENR_GPIOAEN;
    (void)RCC->APB1ENR1;

    /* 2. Configure PA2 (TX) and PA3 (RX) as AF7 (USART2) */
    GPIOA->AFR[0] = (GPIOA->AFR[0] & ~((0xFU << 8U) | (0xFU << 12U))) |
                    ((7U << 8U) | (7U << 12U));
    GPIOA->PUPDR  = (GPIOA->PUPDR  & ~(3U << 6U)) | (1U << 6U); /* Pull-up on PA3 RX */
    GPIOA->MODER  = (GPIOA->MODER  & ~((3U << 4U) | (3U << 6U))) |
                    ((2U << 4U) | (2U << 6U));

    /* 3. Disable USART, set PRESC = /1, compute rounded BRR, enable 8-byte FIFOs */
    USART2->CR1   = 0U;
    USART2->PRESC = 0U; /* /1 */
    USART2->BRR   = (pclk1_hz + (115200U / 2U)) / 115200U; /* 1476 @ 170 MHz */
    USART2->CR1   = USART_CR1_FIFOEN | USART_CR1_TE | USART_CR1_RE | USART_CR1_UE;
    while (!(USART2->ISR & (USART_ISR_TEACK | USART_ISR_REACK))) { }
}

int USART2_SendByte(uint8_t byte, uint32_t timeout_iters) {
    while (!(USART2->ISR & USART_ISR_TXE_TXFNF)) {
        if (timeout_iters-- == 0U) { return -1; }
    }
    USART2->TDR = byte;
    return 0;
}

int USART2_ReceiveByte(uint8_t *out, uint32_t timeout_iters) {
    if (USART2->ISR & USART_ISR_ORE) {
        USART2->ICR = USART_ICR_ORECF; /* Recover from overrun */
    }
    while (!(USART2->ISR & USART_ISR_RXNE_RXFNE)) {
        if (timeout_iters-- == 0U) { return -1; }
    }
    *out = (uint8_t)(USART2->RDR & 0xFFU);
    return 0;
}
```
