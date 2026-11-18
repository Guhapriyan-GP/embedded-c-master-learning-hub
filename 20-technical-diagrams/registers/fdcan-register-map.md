# FDCAN1–3 (Flexible Data-Rate CAN) & Message RAM Register Map

**Target MCU:** STM32G474RET6 (LQFP64 — **All 3 FDCAN controllers accessible!**)  
**Base Addresses (APB1):**
- `FDCAN1`: `0x40006400` | `FDCAN_CONFIG`: `0x40006500` | `FDCAN2`: `0x40006800` | `FDCAN3`: `0x40006C00`
- **Dedicated FDCAN Message RAM (`SRAMCAN_BASE`):** `0x4000A400` (Fixed partition: **212 × 32-bit words = 848 B (`0x350` B) per instance**: `FDCAN1` @ `0x4000A400`, `FDCAN2` @ `0x4000A750`, `FDCAN3` @ `0x4000AAA0`)  
**RCC Clock Enable:** `RCC->APB1ENR1 |= RCC_APB1ENR1_FDCANEN` (Bit 25 — single shared enable for FDCAN1–3) | Kernel Clock: `RCC->CCIPR.FDCANSEL[25:24]` (`00`=HSE, `01`=PLLQ, `10`=PCLK1)  
**IRQ Numbers (`stm32g474xx.h`):** `FDCAN1_IT0=21/IT1=22`, `FDCAN2_IT0=86/IT1=87`, `FDCAN3_IT0=88/IT1=89`  
**Reference Manual:** RM0440, Chapter 44 (*FDCAN*) | AN5348 (*FDCAN peripheral on STM32*)  
**Concept IDs:** `G4-FDCAN-001`, `PERIPH-CAN-001`, `COMM-CAN-002`  
**Verification Status:** Every base address, struct offset, and IRQ verified directly against `stm32g474xx.h`; C code snippets Compile-Verified (`arm-none-eabi-gcc 14.3.1`, `-Wall -Wextra -Werror`) (Not Tested on Physical Hardware).

---

### 1. FDCAN Global & Config Register Map (`FDCAN_GlobalTypeDef` & `FDCAN_Config_TypeDef`)

| Register | Offset | Reset Value | Access | Description |
|---|---|---|---|---|
| `CREL` / `ENDN` | `0x000` / `0x004` | `0x32141218` / `0x87654321` | RO | Core release & endianness constant (`0x87654321`) |
| `DBTP` | `0x00C` | `0x00000A33` | RW (`CCE=1`) | **Data-phase bit timing** (`DSJW`, `DTSEG2`, `DTSEG1`, `DBRP`, `TDC` b23) for CAN-FD BRS up to 8 Mbps |
| `TEST` | `0x010` | `0x00000000` | RW (`TEST=1`) | Test register (**`LBCK` bit 4** loopback mode, `TX[6:5]`, `RX` b7) |
| **`CCCR`** | `0x018` | `0x00000001` | RW | CC Control (**`INIT` b0**, **`CCE` b1** config enable, `ASM`, `MON` b5 bus monitoring, `DAR`, `TEST` b7, **`FDOE` b8** FD enable, **`BRSE` b9** bit-rate switch, `PXHD`) |
| `NBTP` | `0x01C` | `0x06000A03` | RW (`CCE=1`) | **Nominal (arbitration) bit timing** (`NTSEG2[6:0]`, `NTSEG1[15:8]`, `NBRP[24:16]`, `NSJW[31:25]`) |
| `TSCC` / `TSCV` | `0x020` / `0x024` | `0x00000000` | RW | Timestamp counter configuration & current 16-bit value |
| `ECR` | `0x040` | `0x00000000` | RO | Error counter (`TEC[7:0]` transmit error, `REC[14:8]` receive error, `RP` passive) |
| `PSR` | `0x044` | `0x00000707` | RO | Protocol status (`LEC[2:0]`, `ACT[4:3]`, `EP` b5, `EW` b6, **`BO` b7** bus-off, `DLEC[10:8]`, **`TDCV[22:16]`** measured transceiver delay) |
| `TDCR` | `0x048` | `0x00000000` | RW (`CCE=1`) | Transmitter delay compensation (`TDCF[6:0]` filter window, **`TDCO[14:8]`** SSP offset = `(DBRP+1)*(DTSEG1+2)`, requires `DBTP.TDC=1` & `DBRP<=1`) |
| `IR` / `IE` | `0x050` / `0x054` | `0x00000000` | **rc_w1** / RW | Interrupt flags (`IR`: direct `FDCANx->IR = mask;`, NEVER `|=`; in ISRs follow with `(void)FDCANx->IR; __DSB();` per ARM AN321 §4.9 & ES0430 §2.1.3) & enables (`RF0N`, `RF0F`, `RF0L`, `TC`, `BO`, `EW`, `EP`, `PED`, `PEA`) |
| `ILS` / `ILE` | `0x058` / `0x05C` | `0x00000000` | RW | Interrupt line select (`0`=IT0, `1`=IT1) & line enables (`EINT0` b0, `EINT1` b1) |
| `RXGFC` | `0x080` | `0x00000000` | RW (`CCE=1`) | Global filter config (`RRFE`, `RRFS`, `ANFE[3:2]`, `ANFS[5:4]`, `LSS[20:16]` std filter count, `LSE[27:24]` ext count) |
| `XIDAM` | `0x084` | `0x1FFFFFFF` | RW (`CCE=1`) | Extended ID AND mask |
| `RXF0S` / `RXF0A` | `0x090` / `0x094` | `0x00000000` | RO / RW | RX FIFO 0 status (`F0FL` fill level, `F0GI` get index, `F0PI` put index) & Acknowledge (`F0AI`) |
| `RXF1S` / `RXF1A` | `0x098` / `0x09C` | `0x00000000` | RO / RW | RX FIFO 1 status & Acknowledge |
| `TXBC` / `TXFQS` | `0x0C0` / `0x0C4` | `0x00000000` | RW / RO | TX buffer config (`TFQM`: `0`=FIFO, `1`=Queue) & TX FIFO/Queue status (`TFFL`, `TFGI`, `TFQPI`, `TFQF`) |
| `TXBRP` / **`TXBAR`** | `0x0C8` / `0x0CC` | `0x00000000` | RO / RW | TX buffer request pending & **TX buffer add request** (bits `[2:0]` trigger TX!) |
| `TXBCR` / `TXBTO` | `0x0D0` / `0x0D4` | `0x00000000` | RW / RO | TX cancellation request & transmission occurred flags |
| `TXEFS` / `TXEFA` | `0x0E4` / `0x0E8` | `0x00000000` | RO / RW | TX Event FIFO status (`EFFL[2:0]` fill, `EFGI[9:8]` get idx, `EFPI[17:16]` put idx, `EFF` b24, `TEFL` b25) & acknowledge (`EFAI[1:0]`) |
| `FDCAN_CONFIG->CKDIV` | `+0x000` (`0x40006500`) | `0x00000000` | RW | Shared clock divider (`PDIV[3:0]`: `/1`, `/2`, `/4`, ..., `/30`) across FDCAN1–3 |

---

### 2. Fixed STM32G4 Message RAM Partition (Per Instance = `0x350` Bytes)

Unlike STM32H7 (where offsets are programmable), **STM32G4 fixes every Message RAM section offset** relative to `INSTANCE_RAM_BASE` (`0x4000A400`, `0x4000A750`, `0x4000AAA0`):

| Section | Offset from Instance Base | Elements | Element Size | Total Bytes |
|---|---:|---:|---:|---:|
| Standard ID Filters (11-bit) | `+0x000` | 28 | 4 B (1 word) | 112 B (`0x070`) |
| Extended ID Filters (29-bit) | `+0x070` | 8 | 8 B (2 words) | 64 B (`0x040`) |
| RX FIFO 0 (Header 2W + 64B Payload) | `+0x0B0` | 3 | 72 B (18 words) | 216 B (`0x0D8`) |
| RX FIFO 1 (Header 2W + 64B Payload) | `+0x188` | 3 | 72 B (18 words) | 216 B (`0x0D8`) |
| TX Event FIFO | `+0x260` | 3 | 8 B (2 words) | 24 B (`0x018`) |
| TX FIFO / Queue Buffers | `+0x278` | 3 | 72 B (18 words) | 216 B (`0x0D8`) |

---

### 3. Bit Timing Formula & CMSIS Internal Loopback Example

$$f_{\text{bit}} = \frac{f_{\text{fdcan,ker}} / \text{PDIV}}{(\text{BRP} + 1) \times (1 + (\text{TSEG1} + 1) + (\text{TSEG2} + 1))}$$

- **Example (HSE = 24 MHz, `PDIV = /1`, Target = 500 kbps nominal, 83.3% sample point):**
  - `NBRP = 2` (`/3` → $8\ \text{MHz}$ TQ), `1 + 12 + 3 = 16 TQ` (`NTSEG1 = 11`, `NTSEG2 = 2`, `NSJW = 2`) → $8\ \text{MHz} / 16 = 500.0\ \text{kbps}$ exact!

> **Status:** Statically Reviewed (Not Tested on Physical Hardware). Always use HSE crystal (not HSI16) for physical CAN buses!

```c
#include "stm32g474xx.h"

void FDCAN1_Init_InternalLoopback_500k(void) {
    /* 1. Select HSE (24 MHz) as FDCAN kernel clock (FDCANSEL = 00b) & enable APB1 clock */
    RCC->CCIPR    &= ~RCC_CCIPR_FDCANSEL_Msk;
    RCC->APB1ENR1 |=  RCC_APB1ENR1_FDCANEN;
    (void)RCC->APB1ENR1;

    /* 2. Enter Initialization Mode (INIT = 1) and unlock Configuration Change (CCE = 1) */
    FDCAN1->CCCR |= FDCAN_CCCR_INIT;
    while (!(FDCAN1->CCCR & FDCAN_CCCR_INIT)) { }
    FDCAN1->CCCR |= FDCAN_CCCR_CCE;

    /* 3. Enable Internal Loopback (TEST + MON + LBCK) and CAN-FD mode (FDOE) */
    FDCAN1->CCCR |= FDCAN_CCCR_TEST | FDCAN_CCCR_MON | FDCAN_CCCR_FDOE;
    FDCAN1->TEST |= FDCAN_TEST_LBCK;

    /* 4. Nominal 500 kbps @ 24 MHz: NBRP=2 (/3), NTSEG1=11 (12 TQ), NTSEG2=2 (3 TQ), NSJW=2 */
    FDCAN_CONFIG->CKDIV = 0U; /* /1 */
    FDCAN1->NBTP = (2U  << FDCAN_NBTP_NSJW_Pos)   |
                   (2U  << FDCAN_NBTP_NBRP_Pos)   |
                   (11U << FDCAN_NBTP_NTSEG1_Pos) |
                   (2U  << FDCAN_NBTP_NTSEG2_Pos);

    /* 5. Accept non-matching standard frames into RX FIFO 0 (ANFS = 00b) */
    FDCAN1->RXGFC = 0U;

    /* 6. Leave Initialization Mode to start FDCAN1 */
    FDCAN1->CCCR &= ~FDCAN_CCCR_INIT;
    while (FDCAN1->CCCR & FDCAN_CCCR_INIT) { }
}
```
