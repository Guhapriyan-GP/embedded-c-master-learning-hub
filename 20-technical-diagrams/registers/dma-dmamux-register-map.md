# DMA & DMAMUX Register Map

**Target MCU:** STM32G474RE  
**DMA1 Base:** `0x40020000` (`AHB1PERIPH_BASE + 0x0000UL`)  
**DMA2 Base:** `0x40020400` (`AHB1PERIPH_BASE + 0x0400UL`)  
**DMAMUX1 Base:** `0x40020800` (`AHB1PERIPH_BASE + 0x0800UL`)  
**Bus:** AHB1  
**Reference Manual:** RM0440, Chapter 11 (*DMA*) & Chapter 12 (*DMAMUX*)

---

### 1. DMA Channel Register Overview
Each DMA controller features 8 independent channels capable of managing transfers between memory and peripherals without CPU intervention.

| Register | Offset | Reset Value | Description |
|---|---|---|---|
| `DMA_ISR` | `0x00` | `0x00000000` | Interrupt status register (TCIFx transfer complete, HTIFx half transfer) |
| `DMA_IFCR` | `0x04` | `0x00000000` | Interrupt flag clear register (**Write `1` to clear flags!**) |
| `DMA_CCRx` | `0x08 + 0x14*(x-1)` | `0x00000000` | Channel $x$ configuration register (EN, DIR, CIRC, PINC, MINC, PSIZE, MSIZE) |
| `DMA_CNDTRx`| `0x0C + 0x14*(x-1)` | `0x00000000` | Channel $x$ number of data items register (Remaining transfers) |
| `DMA_CPARx` | `0x10 + 0x14*(x-1)` | `0x00000000` | Channel $x$ peripheral address register (Physical 32-bit register address) |
| `DMA_CMARx` | `0x14 + 0x14*(x-1)` | `0x00000000` | Channel $x$ memory address register (Physical 32-bit RAM buffer address) |

---

### 2. Critical Bitfields in `DMA_CCRx`
- **Bit 0 (`EN`):** Channel enable (1 = start DMA; must be `0` when configuring `CNDTR`, `CPAR`, `CMAR`).
- **Bit 1 (`TCIE`):** Transfer Complete interrupt enable.
- **Bit 2 (`HTIE`):** Half Transfer interrupt enable (Enables double-buffering ping-pong pipelines).
- **Bit 4 (`DIR`):** Data transfer direction:
  - `0`: Read from peripheral, write to memory (Peripheral-to-Memory, e.g., ADC, UART RX).
  - `1`: Read from memory, write to peripheral (Memory-to-Peripheral, e.g., DAC, UART TX).
- **Bit 5 (`CIRC`):** Circular mode (When enabled, `CNDTR` auto-reloads on completion; continuous streaming).
- **Bit 6 (`PINC`):** Peripheral increment mode (0 = fixed address, typical for hardware registers).
- **Bit 7 (`MINC`):** Memory increment mode (1 = increment memory pointer after each transfer).
- **Bits [9:8] (`PSIZE`):** Peripheral data size (`00`: 8-bit, `01`: 16-bit, `10`: 32-bit).
- **Bits [11:10] (`MSIZE`):** Memory data size (`00`: 8-bit, `01`: 16-bit, `10`: 32-bit).
- **Bits [13:12] (`PL`):** Channel priority level (`00`: Low, `01`: Medium, `10`: High, `11`: Very High).

---

### 3. DMAMUX (DMA Request Multiplexer) on STM32G4
Unlike older STM32 chips (F1/F4) with fixed hardware channels, the STM32G4 uses a **DMAMUX** crossbar switch. Any peripheral request ID can be routed to any DMA channel:
- `DMAMUX1_Channel0` corresponds to `DMA1_Channel1`.
- `DMAMUX1_Channel0->CCR` Bits [6:0] (`DMAREQ_ID`):
  - `5`: `ADC1` request
  - `24`: `USART1_TX` request
  - `25`: `USART1_RX` request
  - `26`: `USART2_TX` request

---

### 4. CMSIS Code Example: Circular DMA1 Channel 1 for ADC1 Stream
```c
#define BUFFER_SIZE 64
uint16_t adc_buffer[BUFFER_SIZE];

void DMA1_ADC1_Init(void) {
    /* 1. Enable DMA1 and DMAMUX1 bus clocks on AHB1 */
    RCC->AHB1ENR |= RCC_AHB1ENR_DMA1EN | RCC_AHB1ENR_DMAMUX1EN;

    /* 2. Route ADC1 Request (ID = 5) to DMA1 Channel 1 via DMAMUX1 Channel 0 */
    DMAMUX1_Channel0->CCR = (5U << DMAMUX_CxCR_DMAREQ_ID_Pos);

    /* 3. Ensure DMA1 Channel 1 is disabled before configuration */
    DMA1_Channel1->CCR &= ~DMA_CCR_EN;

    /* 4. Configure Peripheral Address (ADC1->DR) and Memory Address (adc_buffer) */
    DMA1_Channel1->CPAR = (uint32_t)&(ADC1->DR);
    DMA1_Channel1->CMAR = (uint32_t)adc_buffer;

    /* 5. Set Number of Data Items */
    DMA1_Channel1->CNDTR = BUFFER_SIZE;

    /* 6. Configure Channel 1:
     *    - Direction: Peripheral-to-Memory (DIR = 0)
     *    - Circular Mode (CIRC = 1)
     *    - Memory Increment (MINC = 1)
     *    - Peripheral Fixed (PINC = 0)
     *    - Data Size: 16-bit Peripheral and 16-bit Memory (PSIZE=01, MSIZE=01)
     *    - High Priority (PL = 10b)
     *    - Transfer Complete Interrupt Enable (TCIE = 1) */
    DMA1_Channel1->CCR = DMA_CCR_CIRC |
                         DMA_CCR_MINC |
                         (1U << DMA_CCR_PSIZE_Pos) | /* 16-bit */
                         (1U << DMA_CCR_MSIZE_Pos) | /* 16-bit */
                         (2U << DMA_CCR_PL_Pos)    | /* High */
                         DMA_CCR_TCIE;

    /* 7. Enable DMA1 Channel 1 */
    DMA1_Channel1->CCR |= DMA_CCR_EN;
    
    /* 8. Enable DMA in ADC1 CFGR */
    ADC1->CFGR |= ADC_CFGR_DMAEN | ADC_CFGR_DMACFG; /* Circular DMA */
}
```
