# Stage 12: Direct Memory Access (`DMA1`/`DMA2`) & `DMAMUX1` Architecture

> **Sequential Navigation (Step 13 of 17 — Canonical Acyclic Order):**  
> **Home:** [`START_HERE.md`](../START_HERE.md) | **Roadmap:** [`learning-sequence.md`](../01-master-concept-map/learning-sequence.md) | **Prev:** [Stage 12 (10) — Mixed-Signal Analog (`12-adc-dac-analog/`)](../12-adc-dac-analog/README.md) | **Next:** [Stage 14 (12) — Dual-Bank Flash, ECC & Bootloaders (`14-memory-and-storage/`)](../14-memory-and-storage/README.md)  
> **Prerequisites:** [`06-stm32-fundamentals/` (`STM32-SRAM-001`)](../06-stm32-fundamentals/README.md), [`10-interrupts/`](../10-interrupts/README.md), [`11-communication/`](../11-communication/README.md), [`12-adc-dac-analog/`](../12-adc-dac-analog/README.md)

> **Directory:** `13-dma/` | **Target Platform:** STM32G474RET6 (`DMA1` 8 Ch + `DMA2` 8 Ch + `DMAMUX1` 16 Ch + 4 Request Generators)  
> **Code Verification Notice:** All bare-metal C snippets in this module are **Statically Reviewed against `stm32g474xx.h`, RM0440, and ES0430 (Not Tested on Physical Hardware)**.

---

## 1. Learning Objectives & Concept Scope

| Concept ID | Concept Name | Architectural Scope | Mastery Tier |
|---|---|---|---|
| `PERIPH-DMA-001` | `DMA1` (8 Ch) & `DMA2` (8 Ch) Architecture (`CPAR`, `CMAR`, `CNDTR`, `PSIZE`/`MSIZE`, `PINC`/`MINC`, `DIR`, `PL`, `MEM2MEM`) | `[STM32G474RE Exact]` | Level 2 (Production) |
| `PERIPH-DMA-002` | Normal vs Circular vs **Ping-Pong Double Buffering (`HTIE`/`TCIE`, `TEIE`)**, `volatile` Buffers, & **ES0430 §2.3.1 `CGIFx` Erratum** | `[STM32G474RE Exact]` / `[ARM Cortex-M4F]` | Level 2 (Production) |
| `PERIPH-DMA-003` | **`DMAMUX1`** Full Crossbar Routing (RM0440 Table 91), **Request Generators (`RG0..3`)**, **Synchronization (`SE`/`SYNC_ID`)**, & AHB Bus-Matrix Arbitration | `[STM32G474RE Exact]` | Level 3 (Specialist) |
| `STM32-SRAM-001` | **CCM-SRAM DMA Access via `0x20018000` S-Bus Alias (NOT `0x10000000`!)** & Zero-Contention Bus Partitioning | `[STM32G474RE Exact]` | Level 2 (Production) |

---

## 2. Prerequisites & Curriculum Positioning

### Prerequisite Concept IDs (Zero Self-Referential Dependencies!)
- **Stage 05 (`06-stm32-fundamentals/`):** `STM32-ARCH-001` (Multi-layer AHB BusMatrix and SRAM1/SRAM2/CCM-SRAM address map).
- **Stage 06 (`07-rcc-and-clocks/`):** `STM32-RCC-001` (`RCC->AHB1ENR` clocking for `DMA1EN`, `DMA2EN`, and **`DMAMUX1EN`**).
- **Stage 08 (`10-interrupts/`):** `RT-IRQ-001`, `RT-IRQ-002` (NVIC priorities, `__DSB()`, and `volatile` memory ordering).
- **Stage 10 (`11-communication/`) & Stage 11 (`12-adc-dac-analog/`):** `PERIPH-UART-001`, `PERIPH-SPI-001`, `PERIPH-ADC-001`, `PERIPH-DAC-001` (peripheral `DMAEN`/`DMAR`/`DMAT` bits).

---

## 3. Core Technical Theory & Engineering Principles

### 3.1 `DMA1`, `DMA2` & `DMAMUX1` Crossbar Architecture `[STM32G474RE Exact]`
STM32G474RE provides **16 fully flexible DMA channels** across two independent AHB1 bus masters:
- **`DMA1` (`0x40020000`, 8 channels `Ch1..Ch8`)** $\longleftarrow$ fed by **`DMAMUX1_Channel0..7`**.
- **`DMA2` (`0x40020400`, 8 channels `Ch1..Ch8`)** $\longleftarrow$ fed by **`DMAMUX1_Channel8..15`**.
- **CRITICAL CLOCKING RULE:** You MUST enable **`RCC->AHB1ENR |= RCC_AHB1ENR_DMA1EN | RCC_AHB1ENR_DMAMUX1EN`** (bits 0 and 2; `DMA2EN` is bit 1). Forgetting `DMAMUX1EN` silently stalls every peripheral DMA request with zero error flags!
- **Note on High-Numbered IRQs:** `DMA1_Channel1..7_IRQn` are IRQs `11..17`, while **`DMA1_Channel8_IRQn = 96`**, `DMA2_Channel1..5_IRQn = 56..60`, **`DMA2_Channel6..8_IRQn = 97..99`**, and **`DMAMUX_OVR_IRQn = 94`** — ensure your vector table populates all 102 STM32G474RE IRQs!

### 3.2 Transfer Modes: Normal, Circular, Double-Buffer Ping-Pong & Memory-to-Memory
1. **Normal Mode (`CIRC = 0`):** Transfers `CNDTR` items and stops (`TCIF = 1`). Ideal for one-shot UART/SPI TX frames. To re-arm, **disable `CCR.EN = 0`** (and wait `EN == 0`), clear flags, reload `CNDTR`, and set `CCR.EN = 1`.
2. **Circular Mode (`CIRC = 1`):** Automatically reloads `CNDTR` upon reaching `0` for uninterrupted streaming (ADC, DAC, UART `IDLE` RX).
3. **Ping-Pong Double Buffering (`HTIE | TCIE` in Circular Mode):**
   - Allocate a single contiguous `volatile` buffer of `2 * N` elements:
     - When DMA finishes `[0 .. N-1]`, **`HTIF` (Half-Transfer)** fires: CPU processes `[0 .. N-1]` while DMA writes `[N .. 2N-1]`!
     - When DMA finishes `[N .. 2N-1]`, **`TCIF` (Transfer-Complete)** fires: CPU processes `[N .. 2N-1]` while DMA wraps back to `[0 .. N-1]`!
   - **Real-Time Constraint:** CPU processing time $t_{\text{proc}}$ MUST satisfy $t_{\text{proc}} < N \cdot T_{\text{sample}}$ or DMA overtakes the CPU and tears live samples!
4. **Memory-to-Memory (`MEM2MEM = 1`):** Software-triggered bulk transfer (cannot be combined with `CIRC = 1`; ignores `DMAMUX1` request ID). Always verify whether CPU burst `LDMIA`/`STMIA` vs DMA wins — DMA's true strength is **concurrency** (freeing the CPU to run math while DMA moves data!).

### 3.3 `DMAMUX1` Advanced Features: Request Generators & Synchronization `[STM32G474RE Exact]`
Beyond static peripheral routing (`DMAREQ_ID` `0..115`), `DMAMUX1` adds two autonomous hardware engines:
- **Request Generators (`RG0..RG3`, `DMAREQ_ID = 1..4`):** Convert an asynchronous trigger (EXTI0–15, LPTIM1, TIM12/COMP1–7) into `GNBREQ + 1` automatic DMA requests (e.g., triggering a 4-word Memory-to-GPIO `BSRR` waveform burst directly on an EXTI edge while the CPU sleeps in `__WFI()`!).
- **Channel Synchronization (`SE = 1`, `SYNC_ID`, `SPOL`, `NBREQ`):** Gates a peripheral DMA channel so it transfers exactly `NBREQ + 1` items only after receiving a hardware sync pulse (e.g., synchronizing SPI TX bursts to a timer trigger). Overruns flag `DMAMUX1_ChannelStatus->CSR` (`SOFx`) and fire `DMAMUX_OVR_IRQn = 94`.

### 3.4 CCM-SRAM DMA Alias (`0x20018000` vs `0x10000000`) & AHB Bus Contention `[STM32G474RE Exact]`
This is one of the most important architectural subtleties of the STM32G474RE multi-layer AHB BusMatrix:
- **`0x10000000..0x10007FFF` (CCM-SRAM Native):** Connected **ONLY** to the Cortex-M4 **I-Code and D-Code** buses for 0-wait-state execution and data access. **DMA1 and DMA2 CANNOT access `0x10000000`!** Programming `CMAR = 0x10000000` causes a DMA Transfer Error (`TEIF`)!
- **`0x20018000..0x2001FFFF` (CCM-SRAM S-Bus Alias):** STM32G4 maps the exact same 32 KB physical CCM-SRAM contiguously after SRAM2 (`0x20014000..0x20017FFF`) on the **AHB BusMatrix S-Bus** — making it **100% accessible to DMA1 and DMA2**!
- **Zero-Contention Partitioning:** Place time-critical CPU ISR code and stack in native CCM-SRAM (`0x10000000`, I/D-Bus) while DMA1/DMA2 stream ADC/DAC/SPI buffers in **SRAM1 (`0x20000000`)** and **SRAM2 (`0x20014000`)** — CPU and DMA operate on completely independent BusMatrix slaves with **zero AHB arbitration stalls**!

### 3.5 `volatile` DMA Buffers & Mandatory ES0430 §2.3.1 `CGIFx` Erratum
- Any RAM buffer written by DMA and read by CPU **MUST be qualified `volatile`** (or read via compiler memory barrier `asm volatile("" ::: "memory")`) so `-O2`/`-O3` cannot hoist or cache stale reads in registers.
- **CRITICAL SILICON ERRATUM (`ES0430 §2.3.1`):** On STM32G4, writing `1` to the global interrupt clear bit **`DMA_IFCR_CGIFx`** simultaneously with an active Transfer Error can clear `TEIFx` without disabling the channel! **RULE:** **NEVER write `CGIFx`!** Always clear individual flags explicitly:
  ```c
  DMA1->IFCR = DMA_IFCR_CTCIF1 | DMA_IFCR_CHTIF1 | DMA_IFCR_CTEIF1; /* NEVER DMA_IFCR_CGIF1! */
  ```

---

## 4. Core Terminology & Acronyms

| Term | Engineering Definition |
|---|---|
| **`DMA1` / `DMA2`** | Dual 8-channel AHB1 bus-master controllers executing autonomous memory/peripheral transfers without CPU cycles. |
| **`DMAMUX1`** | 16-channel programmable request multiplexer routing any of 115 peripheral/generator signals to any DMA1/DMA2 channel. |
| **`HTIF` / `TCIF` / `TEIF`** | Half-Transfer, Transfer-Complete, and Transfer-Error status flags (cleared individually via `DMAx->IFCR`). |
| **Ping-Pong Buffering** | Circular `2*N` buffer alternating CPU processing between lower half (`HTIF`) and upper half (`TCIF`) for zero torn data. |
| **BusMatrix Contention** | Cycle stalls occurring when two AHB masters (CPU + DMA) access the same SRAM bank simultaneously; solved via separate banks/CCM-SRAM. |

---

## 5. Hardware Register Architecture & Bitfield Reference

Verified directly against `stm32g474xx.h` and RM0440 Table 91:

### 5.1 `DMA_TypeDef` (`DMA1=0x40020000`, `DMA2=0x40020400`) & `DMAMUX1` (`0x40020800`)
| Register | Offset | Key Bitfields |
|---|---|---|
| **`DMAx->ISR` / `IFCR`** | `0x00` / `0x04` | 4 bits/ch: `GIFx`, `TCIFx`, `HTIFx`, `TEIFx` (`IFCR` clears via `CTCIFx`/`CHTIFx`/`CTEIFx` — **avoid `CGIFx`!**) |
| **`DMAx_ChN->CCR`** | `+0x08 + 0x14*(N-1)` | `EN` (b0), `TCIE` (b1), `HTIE` (b2), `TEIE` (b3), `DIR` (b4: `0=P2M`, `1=M2P`), `CIRC` (b5), `PINC` (b6), `MINC` (b7), `PSIZE[9:8]`, `MSIZE[11:10]`, `PL[13:12]`, `MEM2MEM` (b14) |
| **`DMAx_ChN->CNDTR`** | `+0x04` | Remaining number of data transfers (`0..65535`; read live to count bytes received in UART `IDLE` mode!) |
| **`DMAx_ChN->CPAR` / `CMAR`** | `+0x08` / `+0x0C` | Peripheral address / Memory address (**use `0x20018000+` alias if targeting CCM-SRAM!**) |
| **`DMAMUX1_Channel(0..15)->CCR`** | `+0x00..0x3C` | `DMAREQ_ID[6:0]`, `SOIE` (b8), `EGE` (b9), `SE` (b16), `SPOL[18:17]`, `NBREQ[23:19]`, `SYNC_ID[28:24]` |
| **`DMAMUX1_RequestGenerator(0..3)->RGCR`** | `+0x100..0x10C` | `SIG_ID[4:0]`, `OIE` (b8), `GE` (b16), `GPOL[18:17]`, `GNBREQ[23:19]` |

### 5.2 Verified Key `DMAMUX1` Request IDs (RM0440 Table 91 — RX/TX Verified!)
| Request ID | Peripheral Signal | Request ID | Peripheral Signal | Request ID | Peripheral Signal |
|---:|---|---:|---|---:|---|
| `1..4` | `DMAMUX_REQ_GEN0..3` | `16` / `17` | `I2C1_RX` / `I2C1_TX` | `36..39` | `ADC2` .. `ADC5` |
| **`5`** | **`ADC1`** | **`24`** / **`25`** | **`USART1_RX`** / **`USART1_TX`** | `40` / `41` | `QUADSPI` / `DAC2_CH1` |
| `6` / `7` | `DAC1_CH1` / `DAC1_CH2` | **`26`** / **`27`** | **`USART2_RX`** / **`USART2_TX`** | `95..101` | `HRTIM1_M` / `A..F` |
| `8` / `9` | `TIM6_UP` / `TIM7_UP` | `28` / `29` | `USART3_RX` / `USART3_TX` | `108` / `109` | `SAI1_A` / `SAI1_B` |
| `10` / `11` | `SPI1_RX` / `SPI1_TX` | `34` / `35` | `LPUART1_RX` / `LPUART1_TX` | `110..113` | `FMAC_RD/WR`, `CORDIC_RD/WR` |

---

## 6. Engineering Mathematics & Worked Formulas

### 6.1 Ping-Pong Double-Buffer Real-Time Deadline Constraint
For a circular buffer of $2N$ samples fed at aggregate sample rate $f_s$:
$$t_{\text{half}} = \frac{N}{f_s}, \qquad \text{Requirement: } \text{WCET}_{\text{process}}(N) + t_{\text{ISR ovh}} < t_{\text{half}}$$
**Worked Example:** `ADC1` scanning 2 channels at $20\text{ kHz}$ ($f_s = 40\text{ kS/s}$), buffer `2 * N = 128` samples ($N = 64$):
$$t_{\text{half}} = \frac{64}{40{,}000} = \mathbf{1.600\text{ ms}} \quad (272{,}000\text{ CPU cycles @ 170 MHz})$$

### 6.2 Peak AHB DMA Bandwidth Budget
$$\text{BW}_{\text{DMA}} = f_s \times B_{\text{transfer}} \times 2\ (\text{read} + \text{write})$$
For 4 MSPS 16-bit ADC streaming: $4\times 10^6 \times 2\text{ B} = 8\text{ MB/s}$ (~1.2% of 170 MHz × 4 B = `680 MB/s` theoretical AHB peak!).

---

## 7. Official Documentation & Standards References

| Document | Exact Sections |
|---|---|
| **ST RM0440** | Ch. 2 (*BusMatrix & CCM-SRAM alias `0x20018000`*), Ch. 11/12 (*DMA*), Ch. 12/13 (*DMAMUX*, Table 91) |
| **ST ES0430** | **§2.3.1 (*DMA `CGIFx` clear race — NEVER use `CGIFx`!*)**, §2.4.1 (*DMAMUX overrun clear*) |

---

## 8. Production-Grade Bare-Metal C Code Examples

> **Verification Status:** Statically Reviewed against `stm32g474xx.h`, RM0440, and ES0430 §2.3.1 (Not Tested on Physical Hardware).

### 8.1 Ping-Pong Circular `DMA1_Channel1` for `ADC1` (`DMAMUX1` ID = `5`, ES0430-Safe `IFCR`)
```c
#include "stm32g474xx.h"
#include <stdint.h>

#define ADC_HALF_SAMPLES 64U
#define ADC_TOTAL_SAMPLES (ADC_HALF_SAMPLES * 2U)

static volatile uint16_t g_adc_dma_buf[ADC_TOTAL_SAMPLES];
static volatile uint8_t  g_half_ready = 0U;
static volatile uint8_t  g_full_ready = 0U;
static volatile uint32_t g_dma_err_cnt = 0U;

void DMA1_Ch1_ADC1_PingPong_Init(void)
{
    /* CRITICAL: Enable BOTH DMA1EN and DMAMUX1EN! */
    RCC->AHB1ENR |= RCC_AHB1ENR_DMA1EN | RCC_AHB1ENR_DMAMUX1EN;
    (void)RCC->AHB1ENR;

    /* 1. Disable channel & wait until EN == 0 before reconfiguring */
    DMA1_Channel1->CCR &= ~DMA_CCR_EN;
    while ((DMA1_Channel1->CCR & DMA_CCR_EN) != 0U) {}

    /* 2. ES0430 §2.3.1: Clear individual flags ONLY (NEVER DMA_IFCR_CGIF1!) */
    DMA1->IFCR = DMA_IFCR_CTCIF1 | DMA_IFCR_CHTIF1 | DMA_IFCR_CTEIF1;

    /* 3. Route DMAMUX1 Channel 0 (-> DMA1 Channel 1) to Request ID 5 (ADC1) */
    DMAMUX1_Channel0->CCR = (5U << DMAMUX_CxCR_DMAREQ_ID_Pos);

    /* 4. Configure addresses, count, and 16-bit Circular P2M with HT/TC/TE interrupts */
    DMA1_Channel1->CPAR  = (uint32_t)&ADC1->DR;
    DMA1_Channel1->CMAR  = (uint32_t)g_adc_dma_buf; /* SRAM1/SRAM2 or 0x20018000 CCM alias! */
    DMA1_Channel1->CNDTR = ADC_TOTAL_SAMPLES;
    DMA1_Channel1->CCR   = DMA_CCR_PL_1       /* High priority */
                         | DMA_CCR_MSIZE_0    /* 16-bit memory */
                         | DMA_CCR_PSIZE_0    /* 16-bit peripheral */
                         | DMA_CCR_MINC       /* Increment memory pointer */
                         | DMA_CCR_CIRC       /* Circular mode */
                         | DMA_CCR_HTIE       /* Half-transfer IRQ */
                         | DMA_CCR_TCIE       /* Transfer-complete IRQ */
                         | DMA_CCR_TEIE;      /* Transfer-error IRQ */

    NVIC_SetPriority(DMA1_Channel1_IRQn, 5U);
    NVIC_EnableIRQ(DMA1_Channel1_IRQn);

    /* 5. Enable DMA channel BEFORE starting ADC conversions */
    DMA1_Channel1->CCR |= DMA_CCR_EN;
}

void DMA1_Channel1_IRQHandler(void)
{
    uint32_t isr = DMA1->ISR;

    if ((isr & DMA_ISR_HTIF1) != 0U) {
        DMA1->IFCR = DMA_IFCR_CHTIF1;
        g_half_ready = 1U; /* Main processes g_adc_dma_buf[0 .. 63] */
    }
    if ((isr & DMA_ISR_TCIF1) != 0U) {
        DMA1->IFCR = DMA_IFCR_CTCIF1;
        g_full_ready = 1U; /* Main processes g_adc_dma_buf[64 .. 127] */
    }
    if ((isr & DMA_ISR_TEIF1) != 0U) {
        DMA1->IFCR = DMA_IFCR_CTEIF1;
        g_dma_err_cnt++;
    }
    __DSB();
}
```

---

## 9. Practical Laboratory Experiments & Debugging Checkpoints

- **Lab 10:** [`lab-10-adc-dma-multichannel-circular-buffer`](../18-practical-laboratory/lab-10-adc-dma-multichannel-circular-buffer/README.md) — Ping-pong `HTIE`/`TCIE` circular DMA with `volatile` buffer and spec-compliant `ADC1`.
- **GDB Verification Checkpoint:**
  ```text
  (gdb) x/1wx 0x40020800   # Verify DMAMUX1_Channel0->CCR == 5 (ADC1)
  (gdb) x/4wx 0x40020008   # Inspect DMA1_Channel1 CCR, CNDTR (live countdown!), CPAR, CMAR
  (gdb) p/x &g_adc_dma_buf # Confirm address is in 0x20000000..0x2001FFFF (NEVER 0x10000000!)
  ```

---

## 10. Common Engineering Mistakes, Hardware Traps & Silicon Errata (ES0430)

1. **Forgetting `RCC->AHB1ENR |= RCC_AHB1ENR_DMAMUX1EN`:** DMA channel stays permanently idle at initial `CNDTR` with zero errors!
2. **Passing Native CCM-SRAM Address (`0x10000000`) to `DMAx->CMAR`:** Causes immediate `TEIF` bus error! Always convert/link DMA buffers to SRAM1/SRAM2 or the **`0x20018000`** S-Bus CCM alias.
3. **Writing `DMA_IFCR_CGIFx` (ES0430 §2.3.1):** Can swallow `TEIFx` without stopping the channel. Always clear `CTCIFx`, `CHTIFx`, `CTEIFx` individually!
4. **Swapping `USART` RX/TX DMAMUX IDs:** `24 = USART1_RX`, `25 = USART1_TX`, `26 = USART2_RX`, `27 = USART2_TX`.
5. **Omitting `volatile` on DMA Buffers:** Under `-O2`, GCC hoists buffer reads out of `while()` loops and reads stale values forever.
6. **Modifying `CNDTR` While `CCR.EN == 1`:** Writes to `CNDTR`/`CPAR`/`CMAR` are ignored while the channel is enabled!

---

## 11. Progressive Coding Exercises & Design Challenges

1. **[Level 2]** Benchmark a 16 KB `DMA1` `MEM2MEM` 32-bit transfer vs `memcpy()` using `DWT->CYCCNT` at 170 MHz.
2. **[Level 2]** Implement `USART2` Circular DMA RX (`DMAMUX` ID = `26`) + `USART_CR1_IDLEIE` extracting variable-length packets via `buf_len - DMA1_Channel2->CNDTR`.
3. **[Level 3]** Configure `DMAMUX1` Request Generator 0 (`SIG_ID = EXTI13`, `GNBREQ = 3`) driving `DMA1_Channel3` (`DMAREQ_ID = 1`) to burst 4 words into `GPIOA->BSRR` while the CPU stays asleep in `__WFI()`.
4. **[Level 3]** Intentionally target `0x10000000` (native CCM) vs `0x20018000` (S-Bus alias) in `DMA1_Channel1->CMAR` and verify `TEIF1 == 1` on `0x10000000` vs clean `TCIF1 == 1` on `0x20018000` in GDB!

---

## 12. Self-Assessment Mastery Questions

1. **Q:** Why does `DMA1_Channel1` map to `DMAMUX1_Channel0` while `DMA2_Channel1` maps to `DMAMUX1_Channel8`?  
   *Answer Check:* `DMAMUX1` uses 0-based channel indexing `0..15` across both 8-channel DMA controllers (`0..7 -> DMA1 Ch1..8`, `8..15 -> DMA2 Ch1..8`).
2. **Q:** Why can DMA access CCM-SRAM at `0x20018000` when it faults at `0x10000000`?  
   *Answer Check:* `0x10000000` is wired exclusively to the CPU's private I-Code/D-Code buses, whereas `0x20018000` aliases the exact same physical 32 KB array onto the shared AHB BusMatrix S-Bus reachable by DMA1/DMA2.
3. **Q:** What happens if `main()` takes 2.0 ms to process a half-buffer when `t_half = 1.6 ms`?  
   *Answer Check:* Buffer overrun / torn samples — DMA wraps into the lower half and overwrites data while `main()` is still reading it!

---

## 13. Measurable Mastery & Completion Criteria

| Concept ID | Pass / Fail Verification Criterion | Measurement Tool |
|---|---|---|
| `PERIPH-DMA-001` | Benchmark `MEM2MEM` 16 KB copy & M2P UART TX with clean `EN=0` reload and individual `IFCR` clears (0 `CGIFx` uses) | `DWT->CYCCNT` + Static Audit |
| `PERIPH-DMA-002` | Stream 20 kHz `ADC1` into 128-sample circular `volatile` ping-pong buffer with 0 torn samples and 0 overruns over 1M samples | Logic Analyzer + GDB |
| `PERIPH-DMA-003` | Execute EXTI-triggered `DMAMUX1` Request Generator burst in `__WFI()` sleep with `DMAMUX1_ChannelStatus->CSR == 0` | Logic Analyzer + GDB |
| `STM32-SRAM-001` | Verify DMA fails (`TEIF`) at `0x10000000`, succeeds at `0x20018000`, and achieves 0-contention when CPU executes from CCM while DMA uses SRAM1 | GDB + `DWT->CYCCNT` |

---

> **Sequential Navigation (Step 13 of 17):**  
> **Prev:** [Stage 12 (10) — Mixed-Signal Analog (`12-adc-dac-analog/`)](../12-adc-dac-analog/README.md) | **Home:** [`START_HERE.md`](../START_HERE.md) | **Progress:** [`LEARNER_PROGRESS_DASHBOARD.md`](../LEARNERS/LEARNER_PROGRESS_DASHBOARD.md) | **Next:** [Stage 14 (12) — Dual-Bank Flash, ECC & Bootloaders (`14-memory-and-storage/`)](../14-memory-and-storage/README.md)
