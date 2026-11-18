# Stage 10: Serial & Industrial Communication Protocols (USART, SPI, I2C, FDCAN, USB, UCPD, SAI, QUADSPI & Framing/CRC)

> **Sequential Navigation (Step 11 of 17 — Canonical Acyclic Order):**  
> **Home:** [`START_HERE.md`](../START_HERE.md) | **Roadmap:** [`learning-sequence.md`](../01-master-concept-map/learning-sequence.md) | **Prev:** [Stage 10 (08) — Hardware Timers & PWM (`09-timers/`)](../09-timers/README.md) | **Next:** [Stage 12 (10) — Mixed-Signal Analog: ADC/DAC/OPAMP/COMP (`12-adc-dac-analog/`)](../12-adc-dac-analog/README.md)  
> **Prerequisites:** [`08-gpio/`](../08-gpio/README.md), [`07-rcc-and-clocks/`](../07-rcc-and-clocks/README.md), [`10-interrupts/`](../10-interrupts/README.md), [`09-timers/`](../09-timers/README.md)

> **Directory:** `11-communication/` | **Target Platform:** STM32G474RET6 (LQFP64) on NUCLEO-G474RE (MB1367)  
> **Code Verification Notice:** All bare-metal C snippets in this module are **Statically Reviewed against `stm32g474xx.h`, RM0440, DS12288, and UM2505 (Not Tested on Physical Hardware)**.

---

## 1. Learning Objectives & Concept Scope

| Concept ID | Concept Name | Architectural Scope | Mastery Tier |
|---|---|---|---|
| `PERIPH-UART-001` / `002` / `003` / `004`, `COMM-LP-001` | `USART1–3`, `UART4–5`, `LPUART1`: 16x/8x `BRR`, `RDR`/`TDR`, Lock-Free SPSC Ring Buffer, `IDLE`+DMA RX, `ORE`/`FE`/`NE`/`PE`, RS-485 `DEM`, & **Stop-Mode Autonomous Wakeup (`CR1.UESM`, `CR3.WUS`/`WUFIE`, `RCC->CR.HSIKERON`)** | `[STM32G474RE Exact]` | Level 1–3 (Essential–Specialist) |
| `PERIPH-SPI-001` / `002` / `003` | `SPI1–3` / `I2S2–3` (`CPOL`/`CPHA`, **Mandatory `FRXTH=1` 8-bit FIFO**, NSS, Full-Duplex DMA, `PA5/LD2` Warning; `SPI4` unbonded on LQFP64) | `[STM32G474RE Exact]` | Level 2 (Production) |
| `PERIPH-I2C-001` / `002` / `003` | `I2C1–4` v2 (`TIMINGR`, `CR2` `NBYTES`/`AUTOEND`/`RELOAD`, `RXDR`/`TXDR`, `NACKF`, 9-Clock Bus-Stuck Recovery, `PB8-BOOT0` Trap) | `[STM32G474RE Exact]` | Level 2 (Production) |
| `PERIPH-CAN-001` / `002`, `G4-FDCAN-001`, `COMM-CAN-002` | **`FDCAN1–3`**: Nominal/Data Bit Timing (`NBTP`/`DBTP`), BRS, Shared Message RAM (`0x4000A400`), **Transmitter Delay Compensation (`TDCR`/`PSR.TDCV`)**, **TX Event FIFO (`TXEFS`/`TXEFA`)**, & **Deterministic Bus-Off (`PSR.BO`) Recovery** | `[STM32G474RE Exact]` | Level 3 (Specialist) |
| `PERIPH-USB-001`, `COMM-USB-002`, `PERIPH-UCPD-001` | **USB FS Device** (1 KB `PMA @ 0x40006000` **16-Bit Halfword Write Quirk — NO Byte Writes!**, `USB_EPxR` Toggle Semantics, Double-Buffered Ping-Pong, Crystal-Less **`CRS` + `HSI48`**) & **`UCPD1`** | `[STM32G474RE Exact]` | Level 3 (Specialist) |
| `PERIPH-SAI-001` / `PERIPH-QSPI-001` | **`SAI1`** (Audio I2S/TDM/PDM) & **`QUADSPI`** (`0xA0001000`, Memory-Mapped XIP `0x90000000`, Bank 1 LQFP64) | `[STM32G474RE Exact]` | Level 3 (Specialist) |
| `PERIPH-PROTO-001` / `PERIPH-CRC-001` | Packet Framing (**SLIP**, **COBS**, **TLV**), Stream FSM Parsers, & Hardware **`CRC`** (`0x40023000`, Programmable 7/8/16/32-bit) | `[General C]` / `[STM32G474RE Exact]` | Level 2 (Production) |

---

## 2. Prerequisites & Curriculum Positioning

### Prerequisite Concept IDs (Zero Self-Referential Dependencies!)
- **Stage 06 (`07-rcc-and-clocks/`):** `STM32-RCC-001`, `STM32-RCC-002`, `STM32-RCC-003` (170 MHz `PCLK1`/`PCLK2`, `RCC->CCIPR`/`CCIPR2` kernel clocks, `HSI48`).
- **Stage 07 (`08-gpio/`):** `PERIPH-GPIO-001`, `PERIPH-GPIO-002` (Alternate Function `AFR[0..1]`, Open-Drain `OTYPER`, `OSPEEDR`, `PB8-BOOT0` & `PA9/10` traps).
- **Stage 08 (`10-interrupts/`):** `RT-IRQ-001`, `RT-IRQ-002` (NVIC priorities, `rc_w1`/`ICR` flag clearing, `__DSB()`).
- **For DMA-Driven Serial Labs Only:** Stage 12 (`13-dma/`): `PERIPH-DMA-001`, `PERIPH-DMA-002` (`DMAMUX1` request IDs & circular buffers).

---

## 3. Core Technical Theory & Engineering Principles

### 3.1 Asynchronous Serial: `USART1–3`, `UART4–5`, `LPUART1` `[STM32G474RE Exact]`
STM32G474RE integrates **6 UART engines** — all accessible on LQFP64: `USART1` (`0x40013800`, APB2), `USART2` (`0x40004400`, APB1), `USART3` (`0x40004800`), `UART4` (`0x40004C00`), `UART5` (`0x40005000`), and **`LPUART1`** (`0x40008000`, `APB1ENR2` bit 0).
- **NUCLEO-G474RE ST-LINK VCP (`PA2` TX / `PA3` RX):** Wired via `SB17`/`SB23` so it operates as **either `USART2` (AF7) OR `LPUART1` (AF12)**!
- **STM32G4 Register Names (NOT STM32F4!):** STM32G4 uses **`RDR`** (`0x24`) for receive, **`TDR`** (`0x28`) for transmit, **`ISR`** (`0x1C`) for status, **`ICR`** (`0x20`) for write-1-to-clear flags, and **`PRESC`** (`0x2C`) for input prescaling (`/1../256`). There is **NO `DR` register**.
- **16x vs 8x Oversampling (`CR1.OVER8`):** `OVER8 = 0` (16x) provides superior noise immunity; `OVER8 = 1` (8x) doubles maximum baud rate (up to $f_{\text{CK}}/8$) but uses a special `BRR[3:0]` encoding where `BRR[3] = 0` and `BRR[2:0] = USARTDIV[3:0] >> 1`.
- **Error Flags (`ORE`, `FE`, `NE`, `PE`):** Overrun Error (`ORE`) occurs if a new byte arrives before `RDR` is read — **if unhandled, `ORE` stalls RX interrupts!** Always inspect `ISR` and clear via **`ICR = USART_ICR_ORECF | USART_ICR_FECF | USART_ICR_NECF | USART_ICR_PECF`**.
- **Lock-Free SPSC Ring Buffer & `IDLE` + DMA RX:**
  - *Interrupt Tier:* Producer (RX ISR) updates `head`, consumer (`main()`) updates `tail` on a power-of-2 buffer (zero locks needed on single-core Cortex-M4 with `__DMB()`!). TX enables `TXEIE` when enqueuing and disables `TXEIE` inside ISR when empty.
  - *DMA Tier:* Enable circular DMA RX (`CR3.DMAR`) + **`CR1.IDLEIE`** (fires when line stays idle for 1 frame duration) to capture **arbitrary variable-length packets** with zero per-byte CPU interrupts! Compute received byte count from `buf_len - DMA_Ch->CNDTR`.
- **Hardware RS-485 Driver Enable (`CR3.DEM`):** Automatically asserts/deasserts `DE` pin (`RTS`) around transmission with programmable assertion/deassertion lead/lag times (`CR1.DEAT[4:0]`, `DEDT[4:0]`).
- **`LPUART1` / `USART` Autonomous Wakeup from Stop 0 / Stop 1 Mode (`COMM-LP-001`):**
  Both `LPUART1` (`LPUART1_IRQn = 91`) and `USART1–3` can autonomously monitor the `RX` pin and wake the STM32G474RE from Stop 0 or Stop 1 mode (when `PCLK1`/`PCLK2` are completely halted!) via internal direct EXTI lines (pre-unmasked at reset in `EXTI_IMR1`) using this exact RM0440 Ch. 39/40 + Ch. 7 sequence:
  1. **Select a Stop-Capable Independent Kernel Clock (`RCC->CCIPR`):** Route **HSI16** (`10b`) or **LSE 32.768 kHz** (`11b`) to `LPUART1SEL[11:10]` or `USART2SEL[3:2]`. *(Never select `PCLK` or `SYSCLK` — they stop in Stop 0/1!)*
  2. **Enable Kernel Oscillator in Stop Mode (`RCC->CR.HSIKERON`, bit 9):** When using HSI16, set `RCC->CR |= RCC_CR_HSION | RCC_CR_HSIKERON;` so the hardware clock controller wakes/maintains HSI16 for the UART kernel on RX activity. *(Not needed when using LSE at $\le 9600$ baud on `LPUART1`, `BRR = 874`, where LSE runs continuously at sub-µA draw!)*
  3. **Select Wakeup Event While `UE == 0` (`USART_CR3.WUS[21:20]` & `WUFIE` bit 22):** `00b` = Address Match (`CR2.ADD`), **`10b` = Start-Bit Detection** (fastest wakeup), `11b` = `RXNE`/`RXFNE` received byte. Set `CR3.WUFIE = 1` and clear `ICR = USART_ICR_WUCF` (bit 20).
  4. **Set `USART_CR1.UESM` (bit 1) & Check Pre-Stop Invariants:** Enable `UE | RE | UESM`, wait for `ISR.REACK == 1`, and before calling `__WFI()` with `PWR_CR1_LPMS_STOP1` + `SLEEPDEEP = 1`, verify **`(ISR & USART_ISR_BUSY) == 0`** so an in-progress frame is not truncated!
  5. **HSI16 Wakeup Latency Caveat:** Waking HSI16 + regulator from Stop 1 takes ~5–15 µs. At 115,200 baud (1 bit = 8.68 µs), the first wake byte can suffer framing error unless either (a) a dummy preamble byte wakes the MCU, (b) `LPUART1` runs continuously on 32.768 kHz `LSE` at 9600 baud (zero lost bits!), or (c) `HSI16` stays on in Stop 0. Always clear `SCB->SCR &= ~SCB_SCR_SLEEPDEEP_Msk;` and restore your 170/160 MHz PLL upon exit!

### 3.2 Synchronous Serial: `SPI1–3` & `I2S2–3` (`SPI4` Unbonded!) `[STM32G474RE Exact]`
- **LQFP64 Availability:** **`SPI1`** (`0x40013000`, APB2), **`SPI2/I2S2`** (`0x40003800`, APB1), and **`SPI3/I2S3`** (`0x40003C00`, APB1) are fully bonded out. **`SPI4`** (`0x40013C00`, `SPI4_IRQn=84`) exists on silicon die only — ALL SPI4 pins are on Port E (unbonded on LQFP64)!
- **SPI Modes 0–3 (`CPOL`/`CPHA`):** `CPOL` selects idle clock polarity (`0`=LOW, `1`=HIGH); `CPHA` selects capture edge (`0`=1st edge, `1`=2nd edge).
- **CRITICAL STM32G4 8-Bit FIFO Gotcha (`CR2.FRXTH = 1` & 8-Bit Cast!):** STM32G4 has 32-bit TX/RX FIFOs with default 16-bit threshold (`FRXTH = 0`). If you perform 8-bit transfers without setting **`SPIx->CR2 |= SPI_CR2_FRXTH`** (bit 12 = 1/4 FIFO = 8-bit threshold), **`RXNE` never asserts on a single byte and your driver hangs forever!** Furthermore, write/read `DR` via **`*(__IO uint8_t *)&SPIx->DR`** — a 16-bit write packs and transmits *two* bytes!
- **NSS Management & Clean CS Deassertion:** Use software slave management (`CR1.SSM | CR1.SSI`) for multi-slave buses (avoids Mode Fault `MODF`). Before raising CS HIGH, ALWAYS wait until `TXE == 1`, `FTLVL == 0`, AND **`(SPIx->SR & SPI_SR_BSY) == 0`**.
- **Full-Duplex DMA:** Always pair **two** DMA channels (TX + RX) — even for read-only sensor bursts, TX DMA (`MINC=0`, dummy `0xFF`) generates `SCK` clocks while RX DMA captures incoming bytes.
- **NUCLEO-G474RE `PA5/LD2` Warning:** `PA5` (`SPI1_SCK`) drives onboard LED `LD2` (`SB6`). For clean $>10\text{ MHz}$ SCK edges, account for loading or remap `SPI1` to `PB3/PB4/PB5` (or use `SPI2`).

### 3.3 STM32G4 v2 `I2C1–4` Engine `[STM32G474RE Exact]`
All **4 I2C controllers** (`I2C1 @ 0x40005400`, `I2C2 @ 0x40005800`, `I2C3 @ 0x40007800`, `I2C4 @ 0x40008400`) are accessible on LQFP64 and support Standard (100 kHz), Fast (400 kHz), and **Fast-Mode Plus (1 MHz)** (`SYSCFG->CFGR1.I2Cx_FMP`).
- **Register Names (Verified in `stm32g474xx.h`):** Data registers are **`RXDR` (`0x24`)** and **`TXDR` (`0x28`)** (NOT `RDR`/`TDR`!).
- **Autonomous `CR2` Transfer State Machine:**
  - Write `SADD` (7-bit address shifted `<< 1`), `RD_WRN` (`0`=Write, `1`=Read), `NBYTES[7:0]`, `AUTOEND` (`1`=auto STOP after `NBYTES`, `0`=hold bus with `TC` flag for **Repeated START**!), `RELOAD` (`1` for transfers **$> 255$ bytes**, reloading `NBYTES` on `TCR`!), and `START = 1`.
- **Mandatory NACK Handling & 9-Clock Bus-Stuck Recovery:**
  - Always poll `(ISR & (TXIS | NACKF))` with a finite timeout; if `NACKF` sets, clear `ICR = I2C_ICR_NACKCF`, wait for `STOPF`, and abort cleanly.
  - If the MCU resets mid-read while a slave holds `SDA` LOW, reconfiguring `I2C1` shows `BUSY = 1` forever. **Recovery:** Before enabling `I2C1` AF, temporarily configure `SCL`/`SDA` as Open-Drain GPIO outputs, toggle `SCL` up to **9 clock pulses** until `SDA` reads HIGH, emit a manual **STOP** (`SCL=1`, `SDA: 0 -> 1`), then switch pins to I2C AF!
- **`PB8-BOOT0` Trap:** On NUCLEO-G474RE, `PB8` has an external 10 kΩ pull-down (`R36`) — always attach external `2.2–4.7 kΩ` pull-ups on `SCL`/`SDA` or avoid `PB8`.

### 3.4 Industrial & High-Speed Interfaces on STM32G474RE `[STM32G474RE Exact]`
1. **`FDCAN1–3` (3× ISO 11898-1:2015 CAN-FD Controllers):**
   - `FDCAN1` (`0x40006400`), `FDCAN2` (`0x40006800`), `FDCAN3` (`0x40006C00`), all enabled by single `RCC->APB1ENR1.FDCANEN` (b25), clocked via `RCC->CCIPR.FDCANSEL` (`HSE` crystal strongly recommended over `HSI16` for bit-timing tolerance!).
   - **Shared Fixed-Partition Message RAM (`SRAMCAN_BASE = 0x4000A400`):** Exactly **212 × 32-bit words (`0x350` bytes)** per instance (`FDCAN1`: `0x4000A400`, `FDCAN2`: `0x4000A750`, `FDCAN3`: `0x4000AAA0`) holding up to 28 standard filters, 8 extended filters, two 3-element RX FIFOs, and a 3-element TX FIFO/Queue.
   - Supports **64-byte payloads** & up to **8 Mbps** data-phase **Bit Rate Switching (`BRS`)** configured via **`NBTP`** (nominal arbitration phase) and **`DBTP`** (data phase + Transmitter Delay Compensation `TDCR`).
   - **Transmitter Delay Compensation (`COMM-CAN-002`, `DBTP.TDC` b23, `TDCR @ 0x048`, `PSR.TDCV[22:16]`):** At CAN-FD data rates $\ge 2\text{--}5\text{ Mbps}$ ($t_{\text{bit}} = 200\text{--}500\text{ ns}$), external CAN transceiver loop delay ($t_{\text{loop}} \approx 120\text{--}250\text{ ns}$) approaches a full bit period, causing the transmitting node's own RX comparator to read the *previous* bit and trigger Bit Errors! Enabling **`DBTP.TDC = 1`** (requires `DBRP <= 1`!) makes hardware measure actual TX$\to$RX transceiver delay on the FDF-to-res dominant-to-recessive edge (readable in **`PSR.TDCV[22:16]`** in `fdcan_ker_ck` cycles) and place a **Secondary Sample Point (SSP)** at $\text{SSP} = \text{TDCV} + \text{TDCR.TDCO}[14:8]$ (with noise filter **`TDCR.TDCF[6:0]`**):
     $$\text{TDCO} = (\text{DBRP} + 1) \times (\text{DTSEG1} + 2) \qquad (\text{clamped to } 0\dots 127)$$
   - **TX Event FIFO (`TXEFS @ 0x0E4`, `TXEFA @ 0x0E8`, Message RAM `+0x260`, 3 × 8 B elements):** Whenever a TX buffer transmits with `EFC = 1`, hardware logs its 16-bit transmission timestamp (`TXTS[15:0]`) and 8-bit Message Marker (`MM[31:24]`) into the TX Event FIFO. Read fill level `TXEFS.EFFL[2:0]`, fetch the 2-word entry at `instance_ram_base + 0x260 + 8 * TXEFS.EFGI[9:8]`, and acknowledge via `FDCANx->TXEFA = efgi;`!
   - **Deterministic Bus-Off Recovery (`PSR.BO` b7, `IR.BO` b19, `IE.BOE` b19):** When `TEC > 255`, hardware sets `PSR.BO = 1`, raises `IR.BO = 1`, and forces **`CCCR.INIT = 1`** (isolating the node from the bus). Production firmware must **NOT** blindly hammer `CCCR.INIT = 0` inside an ISR (which floods a shorted bus!): clear `IR = FDCAN_IR_BO`, wait a bounded backoff delay (e.g., 100–500 ms), clear `CCCR &= ~FDCAN_CCCR_INIT` so hardware counts **128 × 11 consecutive recessive bits** per ISO 11898-1, and trip a persistent fault state if retries exceed `max_retries`!
2. **USB 2.0 Full-Speed Device (`0x40005C00`, 1 KB `PMA @ 0x40006000`) + `CRS` (`0x40002000`) (`COMM-USB-002`):**
   - 8 bidirectional endpoints on `PA11`/`PA12` with internal 1.5 kΩ D+ pull-up (`BCDR.DPPU`). Uses **`HSI48` + `CRS`** (`CLK48SEL = 00b`, `CRS->CFGR.SYNCSRC = 10b` USB SOF, `RELOAD = 47999`) for **crystal-less 48 MHz ±0.25%** operation even when `SYSCLK = 170 MHz` or `160 MHz`!
   - **CRITICAL STM32G4 16-Bit Halfword PMA Access Quirk (RM0440 Ch. 43):** Unlike legacy STM32F103 (32-bit stride with 16-bit gaps), STM32G4 maps its 1024-byte Packet Memory Area (`USB_PMAADDR = 0x40006000`) **contiguously as 512 × 16-bit halfwords** — **HOWEVER, the APB1-to-PMA hardware bridge FORBIDS 8-bit byte writes (`STRB`)!** Writing a single byte (`*(uint8_t *)pma = x;`) or calling Newlib `memcpy()` corrupts the adjacent byte in the same 16-bit halfword! Always copy to/from PMA exclusively via aligned **`volatile uint16_t *`** halfword loads/stores!
   - **`USB->EPnR` Invariant-Preserving Toggle Semantics (`rw` + `rc_w0` + `t`):** Each 16-bit `EPnR` packs normal `rw` bits (`EA`, `EP_TYPE`, `EP_KIND`), write-0-to-clear flags (`CTR_RX` b15, `CTR_TX` b7 — **write `1` to preserve!**), and **toggle-on-1** fields (`STAT_RX[13:12]`, `DTOG_RX` b14, `STAT_TX[5:4]`, `DTOG_TX` b6 — **write `1` ONLY to bits you want to flip, `0` to keep!**). Never do naive `EPnR |= ...`!
   - **Double-Buffered Bulk Ping-Pong (`EP_TYPE = USB_EP_BULK`, `EP_KIND = 1`):** Allocates BOTH BDT descriptors (`ADDR_TX`/`COUNT_TX` as Buffer 0, `ADDR_RX`/`COUNT_RX` as Buffer 1) to a single unidirectional Bulk endpoint. For Bulk IN, hardware tracks its active buffer with `DTOG_TX` while software tracks its buffer with `SW_BUF` (repurposed **`DTOG_RX`** bit!) — while hardware transmits Buffer 0 to the host, CPU fills Buffer 1 and toggles `SW_BUF`, sustaining near-wire-rate Full-Speed throughput without NAK bubbles!
3. **`UCPD1` (`0x4000A000`, USB Type-C & Power Delivery 3.0):**
   - Hardware 300 kbps BMC PHY + 4b5b + CRC32 clocked from **HSI16** on `PB6` (`CC1`), `PB4` (`CC2`), `PA9/PA10` (`DBCC1/2`). Remember `PWR->CR3 |= PWR_CR3_UCPD_DBDIS` when disabling dead-battery pull-downs!
4. **`SAI1` (`0x40015400`, Block A & B + PDM):**
   - Dual independent audio sub-blocks (I2S, TDM up to 16 slots, AC'97, PDM digital microphone) with 8×32-bit FIFOs and DMAMUX IDs `108`/`109`.
5. **`QUADSPI` (`0xA0001000`, XIP `0x90000000`):**
   - Bank 1 bonded on LQFP64 (AF10: `PB10`/`PA3` CLK, `PB11`/`PA2` NCS, `PB1`/`PB0`/`PA7`/`PA6` IO0–IO3); supports Indirect, Auto-Status-Polling, and **Memory-Mapped XIP** reads directly from `0x90000000`!

### 3.5 Binary Packet Framing & Hardware `CRC` Engine `[General C]` / `[STM32G474RE Exact]`
Raw UART byte streams have no packet boundaries. Production protocols combine framing + integrity:
- **SLIP (RFC 1055):** `0xC0` delimiter + escape stuffing (`0xDB`). Simple, worst-case 2× expansion.
- **COBS (Consistent Overhead Byte Stuffing):** Guarantees **zero `0x00` bytes inside the payload** with deterministic max overhead of **1 byte per 254 bytes** — ideal for DMA+`IDLE` or byte-stream FSM parsing!
- **TLV (Type-Length-Value):** `[Sync][Type][Length][Payload...][CRC-16/32]` — self-describing, extensible, enforces strict length bounds before copying (SEI CERT C `ARR30-C`).
- **STM32G4 Hardware `CRC` (`0x40023000`, `RCC->AHB1ENR.CRCEN` b12):** Unlike older STM32F4, STM32G4 supports **programmable 7/8/16/32-bit polynomials (`CRC->POL`)**, programmable `CRC->INIT`, input/output bit reversal (`CR.REV_IN`, `CR.REV_OUT`), and 8/16/32-bit writes to `CRC->DR`!

---

## 4. Core Terminology & Acronyms

| Term | Engineering Definition |
|---|---|
| **`RDR` / `TDR` vs `RXDR` / `TXDR`** | STM32G4 USART uses `RDR`/`TDR`; STM32G4 I2C uses `RXDR`/`TXDR`; STM32G4 SPI uses `DR`. |
| **`FRXTH` (SPI)** | FIFO Reception Threshold (`CR2` bit 12) — MUST be `1` for 8-bit `RXNE` triggering on STM32G4. |
| **`NBYTES` / `AUTOEND` / `RELOAD`** | STM32G4 I2C v2 hardware byte-counter fields in `CR2` automating STOP, Repeated START, and >255 B transfers. |
| **`NBTP` / `DBTP` / `BRS`** | FDCAN Nominal Bit Timing, Data Bit Timing, and Bit Rate Switch (e.g., 500 kbps arbitration $\to$ 2–8 Mbps payload). |
| **`CRS` (Clock Recovery System)** | Hardware oscillator trimmer locking `HSI48` to host USB 1 kHz SOF frames without a crystal. |
| **COBS / SLIP / TLV** | Deterministic binary packet framing protocols preventing delimiter collisions in byte streams. |

---

## 5. Hardware Register Architecture & Bitfield Reference

Verified directly against `stm32g474xx.h`:

| Peripheral | Base Address | Key Registers & Verified Offsets | `IRQn` | `DMAMUX1` IDs (RX / TX) |
|---|---|---|---|---|
| **`USART2`** | `0x40004400` | `CR1@0x00`, `CR2@0x04`, `CR3@0x08`, `BRR@0x0C`, `ISR@0x1C`, `ICR@0x20`, `RDR@0x24`, `TDR@0x28`, `PRESC@0x2C` | `38` | **`26` / `27`** (`USART1`: **`24`/`25`**) |
| **`LPUART1`** | `0x40008000` | Same layout; **20-bit `BRR = 256*f_CK/Baud`** | **`91`** | `34` / `35` |
| **`SPI1`** | `0x40013000` | `CR1@0x00`, `CR2@0x04` (`DS[11:8]`, **`FRXTH` b12**), `SR@0x08` (`FRLVL`, `FTLVL`, `BSY`), `DR@0x0C` | `35` | `10` / `11` |
| **`I2C1`** | `0x40005400` | `CR1@0x00`, `CR2@0x04`, `TIMINGR@0x10`, `ISR@0x18`, `ICR@0x1C`, **`RXDR@0x24`**, **`TXDR@0x28`** | `31`/`32` | `16` / `17` |
| **`FDCAN1`** | `0x40006400` | `DBTP@0x0C`, `TEST@0x10`, `CCCR@0x18`, `NBTP@0x1C`, `PSR@0x44`, `RXGFC@0x80`, `RXF0S@0x90`, `TXBAR@0xCC`; Msg RAM `0x4000A400` | `21`/`22` | N/A (Msg RAM) |
| **`CRC`** | `0x40023000` | `DR@0x00`, `IDR@0x04`, `CR@0x08` (`RESET`, `POLYSIZE`, `REV_IN`, `REV_OUT`), `INIT@0x10`, `POL@0x14` | None | M2M DMA |

---

## 6. Engineering Mathematics & Worked Formulas

### 6.1 USART & LPUART Baud Rate (`BRR`) Equations
- **Standard USART (`OVER8 = 0`, 16x oversampling):**
  $$\text{BRR} = \text{round}\!\left(\frac{f_{\text{CK}}}{\text{Baud}}\right)$$
  At $f_{\text{CK}} = 170\text{ MHz}$ and $115{,}200\text{ baud}$: $\text{BRR} = \text{round}(170{,}000{,}000 / 115{,}200) = \mathbf{1476}$ (actual baud = $115{,}176$, error = $-0.021\%$ $\checkmark$).
- **LPUART1 (`256×` fractional):**
  $$\text{BRR}_{\text{LPUART}} = \text{round}\!\left(\frac{256 \times f_{\text{CK}}}{\text{Baud}}\right), \qquad 3 \cdot \text{Baud} \le f_{\text{CK}} \le 4096 \cdot \text{Baud}$$
  At $f_{\text{CK}} = 32{,}768\text{ Hz}$ (`LSE`) and $9600\text{ baud}$: $\text{BRR} = \text{round}(256 \times 32768 / 9600) = \mathbf{874}$ (error = $-0.02\%$!).

### 6.2 I2C Pull-Up Resistor Sizing (NXP UM10204)
$$R_{p,\min} = \frac{V_{\text{DD}} - V_{\text{OL,max}}}{I_{\text{OL}}}, \qquad R_{p,\max} = \frac{t_{r,\max}}{0.8473 \cdot C_b}$$
For $V_{\text{DD}} = 3.3\text{ V}$, $V_{\text{OL,max}} = 0.4\text{ V}$, $I_{\text{OL}} = 3\text{ mA}$, Fast Mode $t_{r,\max} = 300\text{ ns}$, $C_b = 100\text{ pF}$:
$$R_{p,\min} = \frac{2.9}{0.003} = 967\ \Omega, \qquad R_{p,\max} = \frac{300\times 10^{-9}}{0.8473 \times 100\times 10^{-12}} = 3.54\text{ k}\Omega \implies \text{Choose } \mathbf{2.2\text{ k}\Omega}$$

### 6.3 FDCAN Nominal & Data Bit Timing
$$f_{\text{bit}} = \frac{f_{\text{FDCAN}}}{(\text{BRP}+1) \times (1 + (\text{TSEG1}+1) + (\text{TSEG2}+1))}, \qquad \text{SP} = \frac{1 + (\text{TSEG1}+1)}{1 + (\text{TSEG1}+1) + (\text{TSEG2}+1)}$$
For $f_{\text{FDCAN}} = 24\text{ MHz}$ (`HSE` direct):
- **500 kbps Nominal (87.5% SP):** `NBRP = 2` (`/3` $\to 8\text{ MHz}$, 16 TQ/bit), `NTSEG1 = 12` (13 TQ), `NTSEG2 = 1` (2 TQ) $\implies 1+13+2 = 16\text{ TQ}$, $\text{SP} = 14/16 = 87.5\%$.
- **2 Mbps Data (75.0% SP):** `DBRP = 0` (`/1` $\to 24\text{ MHz}$, 12 TQ/bit), `DTSEG1 = 7` (8 TQ), `DTSEG2 = 2` (3 TQ) $\implies 1+8+3 = 12\text{ TQ}$, $\text{SP} = 9/12 = 75.0\%$.

---

## 7. Official Documentation & Standards References

| Document | Exact Sections |
|---|---|
| **ST RM0440** | Ch. 8 (*CRS*), Ch. 16 (*CRC*), Ch. 26 (*QUADSPI*), Ch. 37/40 (*USART/LPUART*), Ch. 39/41 (*I2C*), Ch. 40/42 (*SPI/I2S*), Ch. 41 (*SAI*), Ch. 43 (*USB*), Ch. 44 (*FDCAN*), Ch. 45 (*UCPD*) — *note Rev 8 vs Rev 9 chapter numbers* |
| **ST AN5348 / AN5225 / AN4760** | AN5348 (*FDCAN on STM32*), AN5225 (*UCPD USB-C PD*), AN4760 (*QUADSPI*) |
| **NXP UM10204 / ISO 11898-1:2015** | I2C Bus Specification / CAN & CAN-FD Protocol Specification |

---

## 8. Production-Grade Bare-Metal C Code Examples

> **Verification Status:** Statically Reviewed against `stm32g474xx.h` & RM0440 (Not Tested on Physical Hardware).

### 8.1 Lock-Free Power-of-2 SPSC UART Ring Buffer + Error-Safe ISR
```c
#include "stm32g474xx.h"
#include <stdint.h>
#include <stdbool.h>

#define UART_RB_SIZE 256U /* MUST be power of 2 */
#define UART_RB_MASK (UART_RB_SIZE - 1U)

typedef struct {
    volatile uint16_t head;
    volatile uint16_t tail;
    uint8_t           buf[UART_RB_SIZE];
} spsc_rb_t;

static spsc_rb_t g_rx_rb = {0};

static inline bool rb_push_isr(spsc_rb_t *rb, uint8_t byte)
{
    uint16_t next = (uint16_t)((rb->head + 1U) & UART_RB_MASK);
    if (next == rb->tail) { return false; } /* Full */
    rb->buf[rb->head] = byte;
    __DMB(); /* Ensure payload visible before advancing head */
    rb->head = next;
    return true;
}

static inline bool rb_pop_main(spsc_rb_t *rb, uint8_t *out)
{
    if (rb->tail == rb->head) { return false; } /* Empty */
    *out = rb->buf[rb->tail];
    __DMB();
    rb->tail = (uint16_t)((rb->tail + 1U) & UART_RB_MASK);
    return true;
}

void USART2_IRQHandler(void)
{
    uint32_t isr = USART2->ISR;

    /* Handle framing/overrun/noise/parity errors so ORE never stalls RX */
    if ((isr & (USART_ISR_ORE | USART_ISR_FE | USART_ISR_NE | USART_ISR_PE)) != 0U) {
        USART2->ICR = USART_ICR_ORECF | USART_ICR_FECF | USART_ICR_NECF | USART_ICR_PECF;
    }

    if ((isr & USART_ISR_RXNE_RXFNE) != 0U) {
        uint8_t byte = (uint8_t)(USART2->RDR & 0xFFU);
        (void)rb_push_isr(&g_rx_rb, byte);
    }
    __DSB();
}
```

### 8.2 STM32G4 8-Bit SPI Transfer (`FRXTH = 1`, 8-Bit `DR` Access & `!BSY` Check)
```c
uint8_t SPI1_TransferByte(uint8_t tx, bool *timeout_err)
{
    uint32_t t = 100000U;
    while ((SPI1->SR & SPI_SR_TXE) == 0U) {
        if (t-- == 0U) { *timeout_err = true; return 0U; }
    }
    /* CRITICAL STM32G4 RULE: 8-bit write prevents 16-bit FIFO packing of 2 bytes! */
    *(__IO uint8_t *)&SPI1->DR = tx;

    t = 100000U;
    while ((SPI1->SR & SPI_SR_RXNE) == 0U) { /* Requires CR2.FRXTH = 1! */
        if (t-- == 0U) { *timeout_err = true; return 0U; }
    }
    *timeout_err = false;
    return *(__IO uint8_t *)&SPI1->DR;
}
/* Note: Always read sensor register with 2 bytes (address byte + dummy 0x00 byte)
 * and wait for (SPI1->SR & SPI_SR_BSY) == 0U before driving CS HIGH! */
```

### 8.3 STM32G4 Hardware IEEE 802.3 CRC-32 (`"123456789"` $\to$ `0xCBF43926`)
```c
uint32_t HW_CRC32_Compute(const uint8_t *data, uint32_t len)
{
    RCC->AHB1ENR |= RCC_AHB1ENR_CRCEN;
    (void)RCC->AHB1ENR;

    CRC->POL  = 0x04C11DB7UL;
    CRC->INIT = 0xFFFFFFFFUL;
    /* 32-bit poly (00b), Byte-level input bit-reversal (01b), Output bit-reversal (1b), Reset */
    CRC->CR   = CRC_CR_REV_IN_0 | CRC_CR_REV_OUT | CRC_CR_RESET;

    for (uint32_t i = 0U; i < len; ++i) {
        *(__IO uint8_t *)&CRC->DR = data[i];
    }
    return CRC->DR ^ 0xFFFFFFFFUL;
}
```

### 8.4 `LPUART1` Stop 1 Autonomous Wakeup (`COMM-LP-001`), `FDCAN` TDC / `TXEFS` / Bus-Off (`COMM-CAN-002`) & USB 16-Bit PMA / `EPnR` Helpers (`COMM-USB-002`)

> **Verification Status:** **Compile-Verified (`arm-none-eabi-gcc 14.3.1`, `-O2 -Wall -Wextra -Werror -Wconversion -Wdouble-promotion -Wvla`)** against `stm32g474xx.h` (Not Tested on Physical Hardware).

```c
#include "stm32g474xx.h"
#include <stdbool.h>
#include <stdint.h>

/* --- COMM-LP-001: LPUART1 Stop 1 Autonomous Wakeup on HSI16 (UESM + WUS + HSIKERON) --- */
void LPUART1_Stop1_Wakeup_HSI16_Init(void)
{
    RCC->CR       |= RCC_CR_HSION | RCC_CR_HSIKERON;
    while ((RCC->CR & RCC_CR_HSIRDY) == 0UL) { }

    RCC->CCIPR     = (RCC->CCIPR & ~RCC_CCIPR_LPUART1SEL_Msk) | RCC_CCIPR_LPUART1SEL_1; /* HSI16 */
    RCC->APB1ENR2 |= RCC_APB1ENR2_LPUART1EN;
    (void)RCC->APB1ENR2;

    LPUART1->CR1   = 0UL;
    LPUART1->PRESC = 0UL;
    LPUART1->BRR   = 35556UL; /* 256 * 16 MHz / 115200 baud */
    LPUART1->CR3   = (2UL << USART_CR3_WUS_Pos) | USART_CR3_WUFIE; /* Start-bit wakeup */
    LPUART1->ICR   = USART_ICR_WUCF;
    LPUART1->CR1   = USART_CR1_RE | USART_CR1_TE | USART_CR1_UESM | USART_CR1_UE;
    while ((LPUART1->ISR & USART_ISR_REACK) == 0UL) { }

    NVIC_SetPriority(LPUART1_IRQn, 2U);
    NVIC_EnableIRQ(LPUART1_IRQn);
}

void LPUART1_EnterStop1_WaitRx(void)
{
    RCC->APB1ENR1 |= RCC_APB1ENR1_PWREN;
    (void)RCC->APB1ENR1;
    while ((LPUART1->ISR & USART_ISR_BUSY) != 0UL) { }
    LPUART1->ICR = USART_ICR_WUCF;

    PWR->CR1  = (PWR->CR1 & ~PWR_CR1_LPMS_Msk) | PWR_CR1_LPMS_STOP1;
    SCB->SCR |= SCB_SCR_SLEEPDEEP_Msk;
    __DSB();
    __WFI();
    SCB->SCR &= ~SCB_SCR_SLEEPDEEP_Msk; /* Restore PLL after Stop 1 exit! */
}

/* --- COMM-CAN-002: FDCAN 5 Mbps TDC (TDCR), TX Event FIFO (TXEFS) & Bus-Off Recovery --- */
#define FDCAN1_TX_EVENT_FIFO_ADDR (SRAMCAN_BASE + 0x260UL)

typedef struct {
    uint32_t word0;
    uint32_t word1; /* TXTS[15:0], DLC[19:16], MM[31:24] */
} fdcan_tx_event_t;

void FDCAN1_Configure_Data5Mbps_With_TDC(void)
{
    /* Assumes INIT=1, CCE=1, f_fdcan = 40 MHz: DBRP=0 (/1), DTSEG1=4 (5 TQ), DTSEG2=1 (2 TQ) -> 8 TQ = 5 Mbps, 75% SP
     * TDCO = (DBRP + 1) * (DTSEG1 + 2) = 1 * 6 = 6 cycles (150 ns SSP offset) */
    FDCAN1->DBTP = FDCAN_DBTP_TDC
                 | (0UL << FDCAN_DBTP_DBRP_Pos)
                 | (4UL << FDCAN_DBTP_DTSEG1_Pos)
                 | (1UL << FDCAN_DBTP_DTSEG2_Pos)
                 | (1UL << FDCAN_DBTP_DSJW_Pos);
    FDCAN1->TDCR = (6UL << FDCAN_TDCR_TDCO_Pos) | (0UL << FDCAN_TDCR_TDCF_Pos);
}

bool FDCAN1_PopTxEvent(uint8_t *marker_out, uint16_t *tx_timestamp_out)
{
    uint32_t txefs = FDCAN1->TXEFS;
    if (((txefs & FDCAN_TXEFS_EFFL_Msk) >> FDCAN_TXEFS_EFFL_Pos) == 0UL) {
        return false;
    }
    uint32_t efgi = (txefs & FDCAN_TXEFS_EFGI_Msk) >> FDCAN_TXEFS_EFGI_Pos;
    const volatile fdcan_tx_event_t *elem =
        (const volatile fdcan_tx_event_t *)(FDCAN1_TX_EVENT_FIFO_ADDR + (efgi * 8UL));

    uint32_t w1       = elem->word1;
    *marker_out       = (uint8_t)((w1 >> 24U) & 0xFFUL);
    *tx_timestamp_out = (uint16_t)(w1 & 0xFFFFUL);
    FDCAN1->TXEFA     = efgi & FDCAN_TXEFA_EFAI_Msk;
    return true;
}

typedef struct {
    uint32_t bus_off_count;
    uint32_t last_bo_tick_ms;
    bool     recovery_pending;
} fdcan_bo_ctx_t;

void FDCAN1_ServiceBusOff(fdcan_bo_ctx_t *ctx, uint32_t now_ms, uint32_t backoff_ms, uint32_t max_retries)
{
    if ((FDCAN1->IR & FDCAN_IR_BO) != 0UL) {
        FDCAN1->IR = FDCAN_IR_BO; /* rc_w1 clear */
    }
    if ((FDCAN1->PSR & FDCAN_PSR_BO) != 0UL) {
        if (!ctx->recovery_pending) {
            ctx->bus_off_count++;
            ctx->last_bo_tick_ms  = now_ms;
            ctx->recovery_pending = true;
        } else if ((ctx->bus_off_count <= max_retries) &&
                   ((uint32_t)(now_ms - ctx->last_bo_tick_ms) >= backoff_ms)) {
            FDCAN1->CCCR &= ~FDCAN_CCCR_INIT; /* Hardware counts 128 x 11 recessive bits */
            ctx->recovery_pending = false;
        }
    } else {
        ctx->recovery_pending = false;
    }
}

/* --- COMM-USB-002: STM32G4 16-Bit Halfword PMA Access & USB_EPxR Toggle/Double-Buffer Helpers --- */
void USB_PMA_Write16(uint16_t pma_offset_bytes, const uint8_t *src, uint16_t len)
{
    /* NEVER use 8-bit STRB or memcpy() on USB_PMAADDR (0x40006000) — corrupts adjacent byte! */
    volatile uint16_t *dst = (volatile uint16_t *)(USB_PMAADDR + (uint32_t)(pma_offset_bytes & ~1U));
    uint16_t words = (uint16_t)((len + 1U) / 2U);

    for (uint16_t i = 0U; i < words; ++i) {
        uint16_t lo = (uint16_t)src[2U * i];
        uint16_t hi = (((2U * i) + 1U) < len) ? (uint16_t)src[(2U * i) + 1U] : 0U;
        dst[i] = (uint16_t)(lo | (uint16_t)(hi << 8U));
    }
}

void USB_PMA_Read16(uint16_t pma_offset_bytes, uint8_t *dst, uint16_t len)
{
    const volatile uint16_t *src = (const volatile uint16_t *)(USB_PMAADDR + (uint32_t)(pma_offset_bytes & ~1U));
    uint16_t words = (uint16_t)(len / 2U);

    for (uint16_t i = 0U; i < words; ++i) {
        uint16_t w         = src[i];
        dst[2U * i]        = (uint8_t)(w & 0xFFU);
        dst[(2U * i) + 1U] = (uint8_t)(w >> 8U);
    }
    if ((len & 1U) != 0U) {
        dst[len - 1U] = (uint8_t)(src[words] & 0xFFU);
    }
}

static inline volatile uint16_t *usb_epr_ptr(uint8_t ep_num)
{
    return (volatile uint16_t *)(USB_BASE + ((uint32_t)ep_num * 4UL));
}

void USB_EP_SetTxStatus(uint8_t ep_num, uint16_t desired_stat_tx)
{
    volatile uint16_t *epr = usb_epr_ptr(ep_num);
    uint16_t cur = *epr;
    /* Preserve rw bits, write 1 to CTR_RX/CTR_TX (rc_w0 invariant), XOR only STAT_TX toggle bits */
    uint16_t val = (uint16_t)((cur & USB_EPREG_MASK) | USB_EP_CTR_RX | USB_EP_CTR_TX);
    val = (uint16_t)(val | (uint16_t)((cur ^ desired_stat_tx) & USB_EPTX_STAT));
    *epr = val;
}

void USB_EP_ToggleSwBufForBulkIn(uint8_t ep_num)
{
    /* Double-Buffered Bulk IN (EP_TYPE=BULK, EP_KIND=1): DTOG_RX is SW_BUF */
    volatile uint16_t *epr = usb_epr_ptr(ep_num);
    uint16_t cur = *epr;
    *epr = (uint16_t)((cur & USB_EPREG_MASK) | USB_EP_CTR_RX | USB_EP_CTR_TX | USB_EP_DTOG_RX);
}
```

---

## 9. Practical Laboratory Experiments & Debugging Checkpoints

- **Lab 08:** [`lab-08-usart-baremetal-driver-printf`](../18-practical-laboratory/lab-08-usart-baremetal-driver-printf/README.md) — 170 MHz `USART2` / `LPUART1` driver & `printf` retargeting.
- **Lab 11:** [`lab-11-spi-master-sensor-interfacing`](../18-practical-laboratory/lab-11-spi-master-sensor-interfacing/README.md) — 170 MHz `SPI1` with `FRXTH=1`, 2-byte sensor read, and `!BSY` CS timing.
- **Lab 12:** [`lab-12-i2c-master-ssd1306-oled`](../18-practical-laboratory/lab-12-i2c-master-ssd1306-oled/README.md) — `I2C1` v2 with derived `TIMINGR`, `NACKF` timeout, `RELOAD`, and 9-clock recovery.
- **Logic Analyzer Checkpoint (Saleae / PulseView @ $\ge 10\times$ Bus Rate):** Capture SPI Mode 0 vs Mode 3 edges, I2C Repeated-START without intervening STOP ($t_{\text{HIGH}} \ge 600\text{ ns}$ @ 400 kHz), and verify IEEE 802.3 CRC-32 check value `0xCBF43926` in GDB.

---

## 10. Common Engineering Mistakes, Hardware Traps & Silicon Errata (ES0430)

1. **Omitting `SPIx->CR2 |= SPI_CR2_FRXTH`:** 8-bit `RXNE` hangs permanently waiting for 16 bits in the STM32G4 FIFO.
2. **Writing 16-Bit `SPI1->DR = byte`:** Pushes TWO bytes onto MOSI (`byte` followed by `0x00`)! Always cast `*(__IO uint8_t *)&SPI1->DR`.
3. **1-Byte SPI Sensor Read:** Full-duplex SPI returns dummy garbage during byte 1 (address phase); you MUST clock a 2nd dummy byte (`0x00`) to shift in the register value.
4. **Infinite I2C Hang on `TXIS` Without Checking `NACKF`:** A missing slave or wrong 7-bit address (`addr << 1`) stalls forever unless `NACKF` and a timeout abort the loop.
5. **Swapped `USART` DMAMUX IDs:** Verified RM0440 Table 91: **`24 = USART1_RX`, `25 = USART1_TX`, `26 = USART2_RX`, `27 = USART2_TX`** (do NOT swap RX/TX!).
6. **`PB8-BOOT0` & `PA5/LD2` Board Traps:** `PB8` has a 10 kΩ pull-down (`R36`); `PA5` loads `SPI1_SCK` with `LD2` (`SB6`).
7. **Attempting to Use `SPI4` on LQFP64:** `SPI4` pins exist only on unbonded Port E.

---

## 11. Progressive Coding Exercises & Design Challenges

1. **[Level 1]** Upgrade `Lab 08` from polling TX/RX to interrupt-driven SPSC ring buffers and verify zero dropped bytes across a 10 KB burst at 921,600 baud.
2. **[Level 2]** Implement an I2C bus scanner + Repeated-START multi-byte read + 9-clock GPIO bus-stuck recovery routine returning distinct error codes (`I2C_OK`, `I2C_ERR_NACK`, `I2C_ERR_TIMEOUT`, `I2C_ERR_BUS_STUCK`).
3. **[Level 2]** Implement a COBS encoder/decoder framing binary telemetry packets validated by STM32G4 Hardware `CRC` (`CRC-16-CCITT`, `POL = 0x1021`).
4. **[Level 3]** Configure `FDCAN1` in Internal Loopback (`TEST.LBCK=1`, `CCCR.MON=1`, `FDOE=1`, `BRSE=1`), transmit a 64-byte CAN-FD frame (500 kbps / 2 Mbps), and verify byte-exact reception from `SRAMCAN_BASE` RX FIFO 0 with `ECR == 0`.

---

## 12. Self-Assessment Mastery Questions

1. **Q:** Why does `USART2->BRR = 1476` output garbled characters if `SystemClock_Config_170MHz()` was forgotten?  
   *Answer Check:* MCU boots at 16 MHz HSI16, so `16,000,000 / 1476 = 10,840 baud` instead of `115,200 baud` (10.625× slower!).
2. **Q:** Why must `SPI1->SR & SPI_SR_BSY` be polled after `TXE == 1` before raising Chip Select (`CS`) HIGH?  
   *Answer Check:* `TXE` indicates only that the TX FIFO has room; the last byte is still shifting out on `MOSI`/`SCK`! Raising `CS` early truncates the last bits.
3. **Q:** How do you transfer a 1024-byte SSD1306 framebuffer over STM32G4 I2C when `CR2.NBYTES` is only 8 bits wide (`max 255`)?  
   *Answer Check:* Set `CR2.RELOAD = 1` (`AUTOEND = 0`) and `NBYTES = 255`; whenever `ISR.TCR` (Transfer Complete Reload) asserts, write the next chunk count into `NBYTES`, clearing `RELOAD = 0` and setting `AUTOEND = 1` on the final chunk.
4. **Q:** Why does STM32G474RE use `HSI48` + `CRS` for USB FS Device when running `SYSCLK` at `170 MHz`?  
   *Answer Check:* `VCO = 340 MHz` cannot be integer-divided to `48 MHz`; `CRS` continuously trims `HSI48` against 1 kHz host USB SOF frames to within $\pm 0.25\%$.
5. **Q:** (`COMM-LP-001`) Why does `LPUART1` Stop-Mode wakeup fail if `RCC->CCIPR.LPUART1SEL = 00b` (`PCLK1`), even when `CR1.UESM = 1` and `CR3.WUFIE = 1`?  
   *Answer Check:* `PCLK1` stops completely in Stop 0/1 mode! You must route **HSI16** (with `RCC->CR.HSIKERON = 1`) or **LSE** (`32.768 kHz`) to `LPUART1SEL`.
6. **Q:** (`COMM-CAN-002`) Why does a 5 Mbps CAN-FD transmitter suffer Bit Errors without `DBTP.TDC = 1` and `TDCR.TDCO`, and why must Bus-Off recovery (`PSR.BO == 1`) NOT blindly clear `CCCR.INIT` inside an unthrottled tight loop?  
   *Answer Check:* Transceiver loop delay (~150–250 ns) exceeds a 200 ns bit period, so the TX node samples its own previous bit without a TDC Secondary Sample Point. Blindly clearing `CCCR.INIT` on a shorted CAN bus causes continuous error-frame storms; a bounded backoff + retry limit preserves bus integrity.
7. **Q:** (`COMM-USB-002`) What happens if you call standard `memcpy((void *)(USB_PMAADDR + 64), buf, 64)` on STM32G474RE, and why does `USB->EP1R |= USB_EP_TX_VALID;` corrupt endpoint state?  
   *Answer Check:* Byte writes (`STRB`) to `USB_PMAADDR` (`0x40006000`) corrupt adjacent bytes in 16-bit halfwords (must use `volatile uint16_t *`). Naive `|=` on `USB->EP1R` toggles existing `1` bits in toggle fields (`STAT_TX/RX`, `DTOG_TX/RX`) and can accidentally clear `CTR_RX`/`CTR_TX` (`rc_w0`).

---

## 13. Measurable Mastery & Completion Criteria

| Concept ID | Pass / Fail Verification Criterion | Measurement Tool |
|---|---|---|
| `PERIPH-UART-002` / `003` | Stream 10 KB via `USART2`/`LPUART1` SPSC ring buffer or `IDLE`+DMA at 921.6 kbps with `0` `ORE` errors | Logic Analyzer + CRC Check |
| `COMM-LP-001` | Configure `LPUART1`/`USART2` with `UESM=1`, `WUS=10b`, `WUFIE=1`, and `HSIKERON=1` (or `LSE`); wake from Stop 1 on incoming UART frame and restore PLL | GDB + Ammeter (`JP6`) |
| `PERIPH-SPI-001` / `002` | Execute 2-byte SPI sensor read & 64-byte paired TX+RX DMA burst at 10.6–21.25 MHz with `FRXTH=1` and clean `!BSY` CS timing | Logic Analyzer |
| `PERIPH-I2C-002` / `003` | Execute 400 kHz Repeated-START read, >255 B `RELOAD` write, clean `NACKF` abort, and verified 9-clock SDA unlock | Oscilloscope / Logic Analyzer |
| `PERIPH-CAN-002` / `COMM-CAN-002` | `FDCAN1` loopback transfers 64-byte CAN-FD frame (500k/5M BRS + `TDCR.TDCO`), decodes `TXEFS` timestamp/marker, and passes deterministic `PSR.BO` backoff recovery test | GDB + CAN Analyzer |
| `COMM-USB-002` | Verify 16-bit halfword `USB_PMA_Write16`/`Read16` round-trip (odd & even lengths) and invariant-preserving `USB_EP_SetTxStatus`/`SW_BUF` toggle logic | Unit Test / GDB |
| `PERIPH-CRC-001` | Hardware `CRC` produces exact `0xCBF43926` for ASCII `"123456789"` and rejects 100% of bit-flipped COBS/TLV frames | GDB / Unit Test |

---

> **Sequential Navigation (Step 11 of 17):**  
> **Prev:** [Stage 10 (08) — Hardware Timers & PWM (`09-timers/`)](../09-timers/README.md) | **Home:** [`START_HERE.md`](../START_HERE.md) | **Progress:** [`LEARNER_PROGRESS_DASHBOARD.md`](../LEARNERS/LEARNER_PROGRESS_DASHBOARD.md) | **Next:** [Stage 12 (10) — Mixed-Signal Analog: ADC/DAC/OPAMP/COMP (`12-adc-dac-analog/`)](../12-adc-dac-analog/README.md)
