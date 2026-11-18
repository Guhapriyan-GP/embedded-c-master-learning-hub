# DMA & DMAMUX Register Map

**Target MCU:** STM32G474RE (LQFP64 — `DMA1` 8 channels + `DMA2` 8 channels = **16 channels**, `DMAMUX1` 16 channels + 4 Request Generators)  
**Base Addresses (AHB1):** `DMA1` `0x40020000` | `DMA2` `0x40020400` | `DMAMUX1` `0x40020800`  
**Bus:** AHB1 (`RCC->AHB1ENR`: `DMA1EN` b0, `DMA2EN` b1, **`DMAMUX1EN` b2 — mandatory!**)  
**Reference Manual:** RM0440, Chapter 12 (*DMA*) & Chapter 13 (*DMAMUX*, Table 91) | ES0430 §2.3–2.4  
**Concept IDs:** `PERIPH-DMA-001`, `TIM-DMA-001`, `STM32-SRAM-001`, `STM32-ERRATA-001`  
**Verification Status:** Every register offset, IRQ number, and DMAMUX Request ID verified directly against `stm32g474xx.h` and RM0440 Table 91; C code snippets Compile-Verified (`arm-none-eabi-gcc 14.3.1`, `-Wall -Wextra -Werror`) (Not Tested on Physical Hardware).

---

### 1. DMA Controller & Channel Register Map (`DMA_TypeDef` & `DMA_Channel_TypeDef`)

| Register | Offset | Reset Value | Access | Description |
|---|---|---|---|---|
| `DMA_ISR` | `0x000` | `0x00000000` | RO | Interrupt status (`GIFx`, `TCIFx`, `HTIFx`, `TEIFx` for channels `x = 1..8`) |
| `DMA_IFCR` | `0x004` | `0x00000000` | **WO** | Interrupt flag clear (Direct `=` write, NEVER `|=`! **ES0430 §2.3.1: Clear individual `CTCIFx`/`CHTIFx`/`CTEIFx` — NEVER `CGIFx`!**) |
| `DMA_CCRx` | `0x008 + 0x14*(x-1)` | `0x00000000` | RW | Channel `x` config (`EN`, `TCIE`, `HTIE`, `TEIE`, `DIR`, `CIRC`, `PINC`, `MINC`, `PSIZE`, `MSIZE`, `PL`, `MEM2MEM`) |
| `DMA_CNDTRx` | `0x00C + 0x14*(x-1)` | `0x00000000` | RW | Remaining transfer count (16-bit: `0..65535`; write only when `EN = 0`) |
| `DMA_CPARx` | `0x010 + 0x14*(x-1)` | `0x00000000` | RW | Peripheral 32-bit address register |
| `DMA_CMARx` | `0x014 + 0x14*(x-1)` | `0x00000000` | RW | Memory 32-bit address register (**For CCM-SRAM, MUST use S-Bus alias `0x20018000`–`0x2001FFFF`, NOT `0x10000000`!**) |

**IRQ Numbers (`stm32g474xx.h`):**
- `DMA1_Channel1..7_IRQn = 11..17`, **`DMA1_Channel8_IRQn = 96`**
- `DMA2_Channel1..5_IRQn = 56..60`, **`DMA2_Channel6..8_IRQn = 97..99`**
- **`DMAMUX_OVR_IRQn = 94`** (DMAMUX synchronization / request generator overrun)

---

### 2. DMAMUX1 Register Map (`0x40020800`)

`DMAMUX1_Channel0..7` route to `DMA1_Channel1..8`; `DMAMUX1_Channel8..15` route to `DMA2_Channel1..8`.

| Register Block | Base Offset | Access | Description |
|---|---|---|---|
| `DMAMUX1_Channel0..15->CCR` | `0x000..0x03C` | RW | `DMAREQ_ID[6:0]`, `SOIE` (b8 sync overrun IE), `EGE` (b9 event gen), `SE` (b16 sync enable), `SPOL[18:17]`, `NBREQ[23:19]`, `SYNC_ID[28:24]` |
| `DMAMUX1_ChannelStatus->CSR` | `0x080` | RO | Synchronization overrun flags `SOF0..SOF15` |
| `DMAMUX1_ChannelStatus->CFR` | `0x084` | WO | Clear synchronization overrun flags `CSOF0..CSOF15` |
| `DMAMUX1_RequestGenerator0..3->RGCR` | `0x100..0x10C` | RW | Autonomous DMA request generator (`SIG_ID[4:0]`, `OIE` b8, `GE` b16, `GPOL[18:17]`, `GNBREQ[23:19]`) |
| `DMAMUX1_RequestGenStatus->RGSR/RGCFR` | `0x140/0x144` | RO/WO | Request generator overrun status / clear flags `OF0..OF3` |

#### Verified DMAMUX1 Request IDs (`DMAREQ_ID`, RM0440 Table 91)

| ID (Dec) | ID (Hex) | Peripheral Request | ID (Dec) | ID (Hex) | Peripheral Request |
|---:|---:|---|---:|---:|---|
| `1..4` | `0x01..0x04` | `DMAMUX_REQ_GEN0..3` | **`24`** | `0x18` | **`USART1_RX`** |
| **`5`** | `0x05` | **`ADC1`** | **`25`** | `0x19` | **`USART1_TX`** |
| `6 / 7` | `0x06 / 0x07` | `DAC1_CH1` / `DAC1_CH2` | **`26`** | `0x1A` | **`USART2_RX`** |
| `8 / 9` | `0x08 / 0x09` | `TIM6_UP` / `TIM7_UP` | **`27`** | `0x1B` | **`USART2_TX`** |
| **`10`** | `0x0A` | **`SPI1_RX`** | `28 / 29` | `0x1C / 0x1D` | `USART3_RX` / `USART3_TX` |
| **`11`** | `0x0B` | **`SPI1_TX`** | `34 / 35` | `0x22 / 0x23` | `LPUART1_RX` / `LPUART1_TX` |
| `12 / 13` | `0x0C / 0x0D` | `SPI2_RX` / `SPI2_TX` | `36..39` | `0x24..0x27` | `ADC2` / `ADC3` / `ADC4` / `ADC5` |
| `14 / 15` | `0x0E / 0x0F` | `SPI3_RX` / `SPI3_TX` | `40 / 41` | `0x28 / 0x29` | `QUADSPI` / `DAC2_CH1` |
| **`16`** | `0x10` | **`I2C1_RX`** | `95..101` | `0x5F..0x65` | `HRTIM1_M` / `HRTIM1_A..F` |
| **`17`** | `0x11` | **`I2C1_TX`** | `108 / 109` | `0x6C / 0x6D` | `SAI1_A` / `SAI1_B` |
| `18..23` | `0x12..0x17` | `I2C2..4_RX/TX` | `110 / 111` | `0x6E / 0x6F` | `FMAC_READ` / `FMAC_WRITE` |
| `42..92` | `0x2A..0x5C` | `TIM1/8/2/3/4/5/15/16/17/20` (incl. **`46` / `0x2E` = `TIM1_UP`** for `TIM-DMA-001` burst!) | `112 / 113` | `0x70 / 0x71` | `CORDIC_READ` / `CORDIC_WRITE` |
| `102..105` | `0x66..0x69` | `DAC3_CH1/2`, `DAC4_CH1/2` | `114 / 115` | `0x72 / 0x73` | `UCPD1_RX` / `UCPD1_TX` |

---

### 3. CMSIS Code Example: Circular Ping-Pong `DMA1_Channel1` for `ADC1` (ES0430-Safe)

> **Status:** Compile-Verified (`arm-none-eabi-gcc 14.3.1`, `-O2 -Wall -Wextra -Werror`; Not Tested on Physical Hardware).

```c
#include "stm32g474xx.h"

#define ADC_BUF_LEN 64U
static volatile uint16_t adc_buf[ADC_BUF_LEN];

void DMA1_Ch1_ADC1_Init(void) {
    /* 1. Enable BOTH DMA1 and DMAMUX1 clocks on AHB1 */
    RCC->AHB1ENR |= RCC_AHB1ENR_DMA1EN | RCC_AHB1ENR_DMAMUX1EN;
    (void)RCC->AHB1ENR;

    /* 2. Disable DMA1 Channel 1 and wait for EN == 0 */
    DMA1_Channel1->CCR &= ~DMA_CCR_EN;
    while (DMA1_Channel1->CCR & DMA_CCR_EN) { }

    /* 3. ES0430 §2.3.1: Clear INDIVIDUAL channel flags — NEVER DMA_IFCR_CGIF1! */
    DMA1->IFCR = DMA_IFCR_CTCIF1 | DMA_IFCR_CHTIF1 | DMA_IFCR_CTEIF1;

    /* 4. Route ADC1 (DMAMUX Request ID = 5) to DMAMUX1_Channel0 (-> DMA1_Channel1) */
    DMAMUX1_Channel0->CCR = (5U << DMAMUX_CxCR_DMAREQ_ID_Pos);

    /* 5. Program addresses & count (Peripheral-to-Memory, 16-bit, Circular, HT+TC IRQs) */
    DMA1_Channel1->CPAR  = (uint32_t)&(ADC1->DR);
    DMA1_Channel1->CMAR  = (uint32_t)adc_buf;
    DMA1_Channel1->CNDTR = ADC_BUF_LEN;
    DMA1_Channel1->CCR   = DMA_CCR_CIRC | DMA_CCR_MINC |
                           (1U << DMA_CCR_PSIZE_Pos) | (1U << DMA_CCR_MSIZE_Pos) |
                           (2U << DMA_CCR_PL_Pos)    |
                           DMA_CCR_HTIE | DMA_CCR_TCIE | DMA_CCR_TEIE;

    NVIC_SetPriority(DMA1_Channel1_IRQn, 3U);
    NVIC_EnableIRQ(DMA1_Channel1_IRQn);

    /* 6. Enable DMA channel, then enable ADC1 circular DMA mode */
    DMA1_Channel1->CCR |= DMA_CCR_EN;
    ADC1->CFGR         |= ADC_CFGR_DMAEN | ADC_CFGR_DMACFG;
}

void DMA1_Channel1_IRQHandler(void) {
    uint32_t isr = DMA1->ISR;
    if (isr & DMA_ISR_HTIF1) {
        DMA1->IFCR = DMA_IFCR_CHTIF1; /* Process first half adc_buf[0..31] */
    }
    if (isr & DMA_ISR_TCIF1) {
        DMA1->IFCR = DMA_IFCR_CTCIF1; /* Process second half adc_buf[32..63] */
    }
    if (isr & DMA_ISR_TEIF1) {
        DMA1->IFCR = DMA_IFCR_CTEIF1; /* Handle transfer error */
    }
    /* Case 2 (short/leaf ISR before BX LR — see 10-interrupts §3.6):
     * Dummy readback confirms flag clear on AHB1 (ARM AN321 §4.9); tail __DSB() guards ES0430 §2.1.3 / ARM 838869 */
    (void)DMA1->ISR;
    __DSB();
}
```
