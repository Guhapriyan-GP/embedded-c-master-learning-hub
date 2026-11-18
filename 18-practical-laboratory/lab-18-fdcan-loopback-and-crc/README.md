# Lab 18: FDCAN1 Internal Loopback (500 kbps / 2 Mbps BRS) & Hardware CRC-32 Integrity

> **Verification Status:** `CROSS-COMPILED` (`arm-none-eabi-gcc 14.3.1`, `-Wall -Wextra -Werror -O2`, 0 warnings) | **Hardware Queue:** `ONBOARD — READY FOR AGENT 4` *(Not Yet Executed on Physical NUCLEO-G474RE Hardware)*

> **Clock Configuration Profile:** **Primary = `170 MHz Range 1 Boost`** (`PCLK1 = 170 MHz`: `NBTP` `NBRP=9, NTSEG1=26, NTSEG2=5` = `500 kbps`; `DBTP` `DBRP=4, DTSEG1=11, DTSEG2=3` = `2.00 Mbps`) | **Alternate = `160 MHz Range 1 Boost`** (`PCLK1 = 160 MHz`: `NBTP` `NBRP=9, NTSEG1=24, NTSEG2=5` = `500 kbps`; `DBTP` `DBRP=4, DTSEG1=10, DTSEG2=3` = `2.00 Mbps`) | **Reusable Driver:** [`23-code-library/Drivers/clock_g4.h`](../../23-code-library/Drivers/clock_g4.h) (`Clock_Init_170MHz()`, `Clock_Init_160MHz()`, `Clock_Init_170MHz_HSE24()`).

---

## 1. Objective & Concept IDs

Configure **`FDCAN1`** (`0x40006400`) and its fixed-layout **Message RAM (`SRAMCAN_BASE = 0x4000A400`)** in **Internal Loopback Test Mode** (`500 kbps` nominal arbitration / `2.000 Mbps` CAN-FD data phase with Bit Rate Switching `BRS` — requiring **no external CAN transceiver**!), transmit a **64-byte CAN-FD frame** containing a hardware-computed **IEEE 802.3 CRC-32** checksum (`CRC` @ `0x40023000`), receive it via **RX FIFO 0**, and verify byte-exact payload + CRC-32 integrity (`0xCBF43926` self-test on `"123456789"`!).

- **Target Concept IDs:** `PERIPH-FDCAN-001` (Bosch M_CAN Bit Timing, Message RAM & CAN-FD BRS), `PERIPH-CRC-001` (Hardware Programmable CRC-32), `EMB-PROTO-001` (End-to-End Frame Integrity)
- **Scope Layer:** `[STM32G474RE Exact]`

---

## 2. Required Hardware

- **Development Board:** ST NUCLEO-G474RE (`MB1367` Rev C) — Internal Loopback mode loops `TX` directly into `RX` inside the Bosch M_CAN silicon (no external CAN transceiver or 120 Ω bus required!)

---

## 3. Required Software

- `arm-none-eabi-gcc` (14.3.1+), `make`, `openocd`, `arm-none-eabi-gdb`
- CMSIS headers (`stm32g474xx.h`, `core_cm4.h`)

---

## 4. Wiring & Pin Configuration

No external wiring required in Internal Loopback Mode (`CCCR.TEST = 1`, `TEST.LBCK = 1`, `CCCR.MON = 1`).  
*(When moving to a physical CAN bus, connect `PA11` (`FDCAN1_RX`, `AF9`) and `PA12` (`FDCAN1_TX`, `AF9`) to a 3.3 V CAN-FD transceiver such as `TCAN330`/`MCP2542` and switch RCC to the **24 MHz HSE crystal `X3`** for ±50 ppm clock tolerance!)*

---

## 5. Register Configuration & Mathematical Derivation

**Official Documentation:** **RM0440 Chapter 44** (*FDCAN*, §44.3.3 Message RAM), **RM0440 Chapter 16** (*CRC*), **AN5348** (*FDCAN peripheral on STM32*).

### 5.1 Exact Bit Timing @ `f_FDCAN = 170 MHz` (`RCC->CCIPR.FDCANSEL = 10b` = `PCLK1`, `CKDIV = 0`)

1. **Nominal Bit Rate (`500.000 kbps`, `NBTP`):**
   - `NBRP = 9` (divide by `10` $\implies f_{\text{TQ}} = 17.0\text{ MHz}$)
   - `Sync = 1`, `NTSEG1 = 26` (`27` TQ), `NTSEG2 = 5` (`6` TQ), `NSJW = 5` (`6` TQ)
   - $f_{\text{nom}} = \frac{17.0\text{ MHz}}{1 + 27 + 6} = \frac{17.0\text{ MHz}}{34} = \mathbf{500.000\text{ kbps}}$ exact! Sample Point = $\frac{28}{34} = 82.35\%$.

2. **Data Bit Rate (`2.000 Mbps`, `DBTP`):**
   - `DBRP = 4` (divide by `5` $\implies f_{\text{TQ}} = 34.0\text{ MHz}$)
   - `Sync = 1`, `DTSEG1 = 11` (`12` TQ), `DTSEG2 = 3` (`4` TQ), `DSJW = 3` (`4` TQ)
   - $f_{\text{data}} = \frac{34.0\text{ MHz}}{1 + 12 + 4} = \frac{34.0\text{ MHz}}{17} = \mathbf{2.000\text{ Mbps}}$ exact! Sample Point = $\frac{13}{17} = 76.47\%$.

### 5.1b Explicit Clock Configuration Profile Comparison (`170 MHz` vs `160 MHz` FDCAN1 Bit Timing)

Switching `PCLK1` (`FDCANSEL = 10b`, `CKDIV = 0`) from `170 MHz` to `160 MHz` without updating `NBTP`/`DBTP` shifts CAN bit timing by **`-5.88%`** (guaranteed bus error on a physical CAN-FD bus!). Use these exact zero-error bit-timing profiles:

| Clock Profile | `clock_g4.h` API | `NBTP` (`500.000 kbps` Nominal) | Nominal Sample Point | `DBTP` (`2.000 Mbps` Data BRS) | Data Sample Point | `CRC-32` Regs |
|---|---|---|---|---|---|---|
| **`170 MHz Range 1 Boost` *(Primary)*** | `Clock_Init_170MHz()` / `_HSE24()` | `NBRP=9` (`17 MHz`), `NTSEG1=26`, `NTSEG2=5`, `NSJW=5` (`34 TQ`) | `82.35%` (`28/34`) | `DBRP=4` (`34 MHz`), `DTSEG1=11`, `DTSEG2=3`, `DSJW=3` (`17 TQ`) | `76.47%` (`13/17`) | Clock-independent |
| **`160 MHz Range 1 Boost` *(Alternate)*** | `Clock_Init_160MHz()` | `NBRP=9` (`16 MHz`), `NTSEG1=24`, `NTSEG2=5`, `NSJW=5` (`32 TQ`) | `81.25%` (`26/32`) | `DBRP=4` (`32 MHz`), `DTSEG1=10`, `DTSEG2=3`, `DSJW=3` (`16 TQ`) | `75.00%` (`12/16`) | Clock-independent |

- **Physical CAN Bus Note:** Internal Loopback works cleanly on HSI16 (`±1%`), but any physical multi-node CAN-FD bus requires **`Clock_Init_170MHz_HSE24()`** ([`clock_g4.h`](../../23-code-library/Drivers/clock_g4.h)) using the NUCLEO-G474RE onboard `24 MHz` crystal (`X3`, `±50 ppm`).

### 5.2 STM32G474RE Fixed FDCAN1 Message RAM Layout (`SRAMCAN_BASE = 0x4000A400`, `0x350` Bytes)

| Section | Offset from `0x4000A400` | Size | Element Geometry |
|---|---|---|---|
| Standard Filters | `+0x000` | `112 B` | `28 × 4 B` |
| Extended Filters | `+0x070` | `64 B` | `8 × 8 B` |
| **RX FIFO 0** | **`+0x0B0`** | `216 B` | `3 × 72 B` (2 header words + 16 data words = `64 B` payload) |
| RX FIFO 1 | `+0x188` | `216 B` | `3 × 72 B` |
| TX Event FIFO | `+0x260` | `24 B` | `3 × 8 B` |
| **TX Buffers** | **`+0x278`** | `216 B` | `3 × 72 B` (2 header words + 16 data words = `64 B` payload) |

### 5.3 Hardware IEEE 802.3 CRC-32 Configuration

- `CRC->POL = 0x04C11DB7UL`, `CRC->INIT = 0xFFFFFFFFUL`
- `CRC->CR = CRC_CR_REV_IN_0 | CRC_CR_REV_OUT | CRC_CR_RESET` (byte-wise bit reflection in, bit reflection out, 32-bit polynomial)
- Final XOR with `0xFFFFFFFFUL` $\implies$ standard test vector `"123456789"` produces **`0xCBF43926`**!

---

## 6. Code Implementation

```c
#include "stm32g474xx.h"
#include <stdbool.h>
#include <stdint.h>

extern uint32_t SystemCoreClock;

/* Fixed STM32G4 FDCAN1 Message RAM offsets (RM0440 §44.3.3) */
#define FDCAN1_RAM_BASE        (SRAMCAN_BASE)           /* 0x4000A400 */
#define FDCAN1_RX_FIFO0_BASE   (FDCAN1_RAM_BASE + 0x0B0UL)
#define FDCAN1_TX_BUF_BASE     (FDCAN1_RAM_BASE + 0x278UL)
#define FDCAN_ELEM_STRIDE_U32  (18U)                    /* 72 bytes = 18 x uint32_t */

static volatile uint32_t g_crc_selftest_val = 0U;  /* MUST equal 0xCBF43926 */
static volatile uint32_t g_fdcan_rx_id      = 0U;  /* MUST equal 0x123 */
static volatile uint32_t g_fdcan_rx_crc_ok  = 0U;  /* MUST equal 1 */
static volatile uint32_t g_fdcan_pass       = 0U;  /* MUST equal 1 */
static volatile uint32_t g_fdcan_ecr        = 0xFFU;

static void Clock_Init_170MHz(void) {
    RCC->APB1ENR1 |= RCC_APB1ENR1_PWREN;
    volatile uint32_t tmpreg = RCC->APB1ENR1;
    (void)tmpreg;

    PWR->CR1 = (PWR->CR1 & ~PWR_CR1_VOS) | PWR_CR1_VOS_0;
    while ((PWR->SR2 & PWR_SR2_VOSF) != 0U) {}
    PWR->CR5 &= ~PWR_CR5_R1MODE;

    FLASH->ACR = (FLASH->ACR & ~FLASH_ACR_LATENCY) |
                 FLASH_ACR_LATENCY_4WS             |
                 FLASH_ACR_PRFTEN | FLASH_ACR_ICEN | FLASH_ACR_DCEN;
    while ((FLASH->ACR & FLASH_ACR_LATENCY) != FLASH_ACR_LATENCY_4WS) {}

    RCC->CR |= RCC_CR_HSION;
    while ((RCC->CR & RCC_CR_HSIRDY) == 0U) {}

    RCC->PLLCFGR = RCC_PLLCFGR_PLLSRC_HSI              |
                   ((4U - 1U) << RCC_PLLCFGR_PLLM_Pos) |
                   (85U       << RCC_PLLCFGR_PLLN_Pos) |
                   (0U        << RCC_PLLCFGR_PLLR_Pos) |
                   RCC_PLLCFGR_PLLREN;

    RCC->CR |= RCC_CR_PLLON;
    while ((RCC->CR & RCC_CR_PLLRDY) == 0U) {}

    RCC->CFGR = (RCC->CFGR & ~(RCC_CFGR_HPRE | RCC_CFGR_PPRE1 | RCC_CFGR_PPRE2)) |
                RCC_CFGR_HPRE_DIV2;
    RCC->CFGR = (RCC->CFGR & ~RCC_CFGR_SW) | RCC_CFGR_SW_PLL;
    while ((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_PLL) {}

    for (volatile uint32_t i = 0U; i < 120U; i++) { __NOP(); }
    RCC->CFGR &= ~RCC_CFGR_HPRE;
    SystemCoreClock = 170000000UL;
}

static uint32_t CRC32_IEEE8023_Compute(const uint8_t *data, uint32_t len) {
    RCC->AHB1ENR |= RCC_AHB1ENR_CRCEN;
    volatile uint32_t tmpreg = RCC->AHB1ENR;
    (void)tmpreg;

    CRC->POL  = 0x04C11DB7UL;
    CRC->INIT = 0xFFFFFFFFUL;
    /* 32-bit poly (POLYSIZE=00), byte-wise input reverse (REV_IN=01), output reverse (REV_OUT=1) */
    CRC->CR   = CRC_CR_REV_IN_0 | CRC_CR_REV_OUT | CRC_CR_RESET;

    for (uint32_t i = 0U; i < len; i++) {
        *(__IO uint8_t *)&CRC->DR = data[i]; /* 8-bit write streams 1 byte cleanly */
    }
    return CRC->DR ^ 0xFFFFFFFFUL;
}

static bool FDCAN1_Init_InternalLoopback(void) {
    /* 1. Select PCLK1 (170 MHz) as FDCAN kernel clock (FDCANSEL = 10b) and enable FDCANEN */
    RCC->CCIPR    = (RCC->CCIPR & ~RCC_CCIPR_FDCANSEL) | RCC_CCIPR_FDCANSEL_1;
    RCC->APB1ENR1 |= RCC_APB1ENR1_FDCANEN;
    volatile uint32_t tmpreg = RCC->APB1ENR1;
    (void)tmpreg;

    /* Shared prescaler /1 -> 170 MHz */
    FDCAN_CONFIG->CKDIV = 0U;

    /* 2. Zero-initialize FDCAN1 Message RAM (212 words = 848 bytes) */
    volatile uint32_t *ram = (volatile uint32_t *)FDCAN1_RAM_BASE;
    for (uint32_t i = 0U; i < 212U; i++) {
        ram[i] = 0U;
    }

    /* 3. Request Initialization Mode (INIT=1) and Configuration Change Enable (CCE=1) */
    FDCAN1->CCCR |= FDCAN_CCCR_INIT;
    uint32_t timeout = 200000UL;
    while ((FDCAN1->CCCR & FDCAN_CCCR_INIT) == 0U) {
        if (--timeout == 0U) { return false; }
    }
    FDCAN1->CCCR |= FDCAN_CCCR_CCE;

    /* 4. Enable CAN-FD (FDOE), Bit Rate Switching (BRSE), Bus Monitoring (MON), Test Mode (TEST) */
    FDCAN1->CCCR |= FDCAN_CCCR_FDOE | FDCAN_CCCR_BRSE | FDCAN_CCCR_MON | FDCAN_CCCR_TEST;
    FDCAN1->TEST |= FDCAN_TEST_LBCK; /* Internal Loopback */

    /* 5. Nominal 500 kbps @ 170 MHz: NBRP=9 (/10 -> 17 MHz), NTSEG1=26, NTSEG2=5, NSJW=5 */
    FDCAN1->NBTP = (5U  << FDCAN_NBTP_NSJW_Pos)   |
                   (9U  << FDCAN_NBTP_NBRP_Pos)   |
                   (26U << FDCAN_NBTP_NTSEG1_Pos) |
                   (5U  << FDCAN_NBTP_NTSEG2_Pos);

    /* 6. Data 2.000 Mbps @ 170 MHz: DBRP=4 (/5 -> 34 MHz), DTSEG1=11, DTSEG2=3, DSJW=3 */
    FDCAN1->DBTP = (4U  << FDCAN_DBTP_DBRP_Pos)   |
                   (11U << FDCAN_DBTP_DTSEG1_Pos) |
                   (3U  << FDCAN_DBTP_DTSEG2_Pos) |
                   (3U  << FDCAN_DBTP_DSJW_Pos);

    /* 7. Global Filter: Accept non-matching standard/extended frames into RX FIFO 0 */
    FDCAN1->RXGFC = 0U; /* ANFS=00 (Accept into RX FIFO 0), ANFE=00, LSS=0, LSE=0 */

    /* 8. Leave Initialization Mode to start FDCAN1 */
    FDCAN1->CCCR &= ~FDCAN_CCCR_INIT;
    timeout = 200000UL;
    while ((FDCAN1->CCCR & FDCAN_CCCR_INIT) != 0U) {
        if (--timeout == 0U) { return false; }
    }
    return true;
}

int main(void) {
    Clock_Init_170MHz();

    /* Step 1: Hardware CRC-32 standard check vector ("123456789" -> 0xCBF43926) */
    static const uint8_t check_vec[9] = {'1','2','3','4','5','6','7','8','9'};
    g_crc_selftest_val = CRC32_IEEE8023_Compute(check_vec, sizeof(check_vec));

    /* Step 2: Initialize FDCAN1 in 500 kbps / 2 Mbps CAN-FD Internal Loopback */
    if (!FDCAN1_Init_InternalLoopback()) {
        while (1) { __BKPT(0); }
    }

    /* Step 3: Build 64-byte payload (60 data bytes + 4-byte hardware CRC-32 trailer) */
    uint8_t tx_payload[64];
    for (uint32_t i = 0U; i < 60U; i++) {
        tx_payload[i] = (uint8_t)(0xA0U + i);
    }
    uint32_t tx_crc = CRC32_IEEE8023_Compute(tx_payload, 60U);
    tx_payload[60] = (uint8_t)(tx_crc & 0xFFU);
    tx_payload[61] = (uint8_t)((tx_crc >> 8U)  & 0xFFU);
    tx_payload[62] = (uint8_t)((tx_crc >> 16U) & 0xFFU);
    tx_payload[63] = (uint8_t)((tx_crc >> 24U) & 0xFFU);

    /* Step 4: Populate TX Buffer 0 in Message RAM (Std ID = 0x123, DLC = 15 -> 64 B, FDF=1, BRS=1) */
    volatile uint32_t *tx_buf0 = (volatile uint32_t *)FDCAN1_TX_BUF_BASE;
    tx_buf0[0] = (0x123UL << 18U);                     /* T0: Standard 11-bit ID in bits [28:18] */
    tx_buf0[1] = (15UL << 16U) | (1UL << 21U) | (1UL << 20U); /* T1: DLC=15 (64B), FDF=1, BRS=1 */
    for (uint32_t w = 0U; w < 16U; w++) {
        uint32_t word = ((uint32_t)tx_payload[w * 4U + 0U])       |
                        ((uint32_t)tx_payload[w * 4U + 1U] << 8U) |
                        ((uint32_t)tx_payload[w * 4U + 2U] << 16U)|
                        ((uint32_t)tx_payload[w * 4U + 3U] << 24U);
        tx_buf0[2U + w] = word;
    }

    __DSB();
    FDCAN1->TXBAR = (1UL << 0U); /* Trigger transmission on TX Buffer 0 */

    /* Step 5: Poll RX FIFO 0 Fill Level (F0FL > 0) with bounded timeout */
    uint32_t timeout = 500000UL;
    while ((FDCAN1->RXF0S & FDCAN_RXF0S_F0FL) == 0U) {
        if (--timeout == 0U) {
            while (1) { __BKPT(1); }
        }
    }

    /* Step 6: Read received frame from RX FIFO 0 Get Index and verify CRC-32 */
    uint32_t get_idx = (FDCAN1->RXF0S & FDCAN_RXF0S_F0GI) >> FDCAN_RXF0S_F0GI_Pos;
    volatile uint32_t *rx_elem = (volatile uint32_t *)FDCAN1_RX_FIFO0_BASE +
                                 (get_idx * FDCAN_ELEM_STRIDE_U32);

    g_fdcan_rx_id = (rx_elem[0] >> 18U) & 0x7FFU;
    uint8_t rx_payload[64];
    for (uint32_t w = 0U; w < 16U; w++) {
        uint32_t word = rx_elem[2U + w];
        rx_payload[w * 4U + 0U] = (uint8_t)(word & 0xFFU);
        rx_payload[w * 4U + 1U] = (uint8_t)((word >> 8U)  & 0xFFU);
        rx_payload[w * 4U + 2U] = (uint8_t)((word >> 16U) & 0xFFU);
        rx_payload[w * 4U + 3U] = (uint8_t)((word >> 24U) & 0xFFU);
    }

    /* Acknowledge RX FIFO 0 element so hardware frees the slot */
    FDCAN1->RXF0A = get_idx;

    uint32_t rx_crc_calc = CRC32_IEEE8023_Compute(rx_payload, 60U);
    uint32_t rx_crc_recv = ((uint32_t)rx_payload[60])       |
                           ((uint32_t)rx_payload[61] << 8U) |
                           ((uint32_t)rx_payload[62] << 16U)|
                           ((uint32_t)rx_payload[63] << 24U);

    g_fdcan_rx_crc_ok = (rx_crc_calc == rx_crc_recv) ? 1U : 0U;
    g_fdcan_ecr       = FDCAN1->ECR;
    g_fdcan_pass      = ((g_crc_selftest_val == 0xCBF43926UL) &&
                         (g_fdcan_rx_id == 0x123UL)           &&
                         (g_fdcan_rx_crc_ok == 1U)            &&
                         (g_fdcan_ecr == 0U)) ? 1U : 0U;

    while (1) {
        __WFI();
    }
}
```

---

## 7. Expected Output

In GDB at `while (1)`:
- `g_crc_selftest_val == 0xCBF43926` (exact IEEE 802.3 standard check value)
- `g_fdcan_rx_id == 0x123`
- `g_fdcan_rx_crc_ok == 1`
- `g_fdcan_ecr == 0` (zero TX/RX CAN errors)
- `g_fdcan_pass == 1`

---

## 8. Debugging Procedure

1. Halt at `while (1)` and inspect:
   ```gdb
   (gdb) p/x g_crc_selftest_val
   (gdb) p/x g_fdcan_rx_id
   (gdb) p g_fdcan_rx_crc_ok
   (gdb) p/x g_fdcan_ecr
   (gdb) p g_fdcan_pass
   ```
2. Dump the TX buffer and RX FIFO 0 directly from Message RAM (`x/18wx 0x4000A678` vs `x/18wx 0x4000A4B0`).

---

## 9. Common Errors & Troubleshooting

| Symptom | Root Cause | Verified Fix |
|---|---|---|
| `FDCAN1->CCCR` `INIT` bit never sets or clears | Forgot `RCC->CCIPR.FDCANSEL` or `RCC->APB1ENR1.FDCANEN` | Select valid kernel clock (`PCLK1` / `HSE` / `PLLQ`) and enable `FDCANEN` |
| CRC-32 does not equal `0xCBF43926` | Used 32-bit writes to `CRC->DR` for unaligned bytes or omitted `REV_IN`/`REV_OUT`/final `^ 0xFFFFFFFF` | Cast `*(__IO uint8_t *)&CRC->DR` and apply IEEE 802.3 reflection + XOR |
| RX FIFO 0 overflows after 3 frames | Forgot `FDCAN1->RXF0A = get_idx;` after reading element | Always acknowledge `RXF0A` with `F0GI` |

---

## 10. Verification Procedure

- [ ] Code compiles cleanly with `arm-none-eabi-gcc -Wall -Wextra -Werror`.
- [ ] `g_crc_selftest_val == 0xCBF43926UL`.
- [ ] `g_fdcan_rx_id == 0x123UL`, `g_fdcan_rx_crc_ok == 1U`, `g_fdcan_ecr == 0U`, `g_fdcan_pass == 1U`.

---

## 11. Extension Exercises

1. **Hardware Standard ID Filter:** Configure `RXGFC` (`LSS = 1`, `ANFS = 2` reject non-matching) and write a Standard Filter Element at `0x4000A400` accepting only ID range `0x100..0x1FF`.
2. **External Loopback (`MON = 0`, `LBCK = 1`):** Observe the actual `500 kbps` arbitration header followed by `2 Mbps` BRS data bits on `PA12` (`AF9`) with an oscilloscope!
3. **512 KB Flash CRC-32 Boot Check:** Stream `0x08000000..0x0807FFFB` through `CRC->DR` via Memory-to-Memory DMA1 to verify firmware image integrity on startup.

---

## 12. Standardized Verification Record

| Field | Record |
|---|---|
| **Verification Status** | `CROSS-COMPILED` *(Self-Contained Onboard — Queued for Physical Execution)* |
| **Objective** | Verify hardware IEEE 802.3 `CRC-32` (`"123456789" -> 0xCBF43926`) and transmit/receive a 64-byte CAN-FD frame (`500 kbps` nominal / `2.000 Mbps` BRS) with embedded CRC-32 trailer over `FDCAN1` Internal Loopback (`SRAMCAN_BASE = 0x4000A400`) with zero external CAN transceiver. |
| **Firmware Version** | `v2.2.0-remediation` (branch `feature/complete-remediation`, base commit `585dc4fa`) |
| **Compiler Version** | `arm-none-eabi-gcc 14.3.1 20250623 (GNU Tools for STM32 14.3.rel1.20251027-0700)` |
| **Build Result** | **PASS** — `arm-none-eabi-gcc -std=c11 -mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard -DSTM32G474xx -Wall -Wextra -Werror -O2 -c` succeeded with **0 warnings, 0 errors** (`2,116 B` object file). |
| **Test Environment** | Windows 11 + ARM GNU Toolchain 14.3.rel1 + NUCLEO-G474RE (`MB1367` Rev C, `STM32G474RET6`, STLINK-V3E `V3J17M11`, `COM8`) |
| **Required Peripherals** | **Onboard Only:** `RCC`, `PWR`, `FLASH`, `CRC` (`0x40023000`), `FDCAN1` (`0x40006400`), `FDCAN1` Message RAM (`0x4000A400`). Zero external wiring required in Internal Loopback Mode. |
| **Hardware Availability** | `ONBOARD — READY FOR AGENT 4` |
| **Pin Configuration** | None required in Internal Loopback (`CCCR.TEST=1, TEST.LBCK=1, CCCR.MON=1`); physical CAN bus uses `PA11`/`PA12` (`AF9`) + external 3.3 V transceiver + `HSE` 24 MHz crystal `X3`. |
| **Expected Behavior** | `g_crc_selftest_val == 0xCBF43926`, `g_fdcan_rx_id == 0x123`, `g_fdcan_rx_crc_ok == 1`, `g_fdcan_ecr == 0` (zero protocol errors), `g_fdcan_pass == 1`. |
| **Actual Observations** | Pending Agent 4 Onboard Test *(not yet executed on physical silicon)*. |
| **Register Values** | Configured (`170 MHz` / `160 MHz`): `RCC->CCIPR.FDCANSEL = 10b` (`PCLK1`), `FDCAN_CONFIG->CKDIV = 0`, `FDCAN1->NBTP = 0x0A091A05` (`0x0A091805` @ 160 MHz), `FDCAN1->DBTP = 0x00040B33` (`0x00040A33` @ 160 MHz), `FDCAN1->TEST.LBCK = 1`, `CRC->POL = 0x04C11DB7`, `CRC->INIT = 0xFFFFFFFF`. |
| **Debugging Procedure** | Flash via `STM32_Programmer_CLI -c port=SWD -w firmware.bin 0x08000000 -v -rst` or OpenOCD; in `arm-none-eabi-gdb` break at `while (1)` and verify `p/x g_crc_selftest_val`, `p/x g_fdcan_rx_id`, `p g_fdcan_rx_crc_ok`, `p/x g_fdcan_ecr`, `p g_fdcan_pass`, plus `x/18wx 0x4000A4B0`. |
| **Remaining Limitations** | Internal Loopback validates Bosch M_CAN controller, Message RAM, BRS, and CRC-32 without exercising physical transceiver dominant/recessive bus arbitration (requires external transceiver + `Clock_Init_170MHz_HSE24()`). Physical silicon test pending Agent 4. |
