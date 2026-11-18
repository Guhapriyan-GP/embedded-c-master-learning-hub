# Project 02: High-Speed Multichannel DMA Data Logger with CRC-32 & Dual-Bank Flash Storage

> **Verification Status:** Statically Reviewed Architecture & Hardware Specification (Integration Blueprint — Refer to Compile-Verified Labs 03, 10, 12, 18 for Individual Peripheral Drivers)

---

## 1. Project Overview & Architecture

The High-Speed Multichannel DMA Data Logger streams three analog channels autonomously from `ADC1` (`42.5 MHz`) into a **`volatile` SRAM circular ping-pong buffer** via `DMA1 Channel 1` (`HTIE` + `TCIE`), computes statistical metrics (Min, Max, Mean, factory-calibrated die temperature), frames records with a **hardware IEEE 802.3 `CRC-32` checksum**, displays live telemetry on an `I2C1` SSD1306 OLED (`400 kHz`), and commits wear-leveled log pages to **Flash Bank 2 (`0x08040000`)** using STM32G4 64-bit double-word programming without stalling Flash Bank 1 code execution!

```text
[ PA0 (Ch1), PA1 (Ch2), Internal Temp (Ch16) ]
                     │
                     ▼
[ ADC1 Scan Engine @ 42.5 MHz (CKMODE=11b) ] ──(DMAMUX1 Ch0, ID=5)──> [ volatile uint16_t ping_pong[192] ]
                                                                                │
                                                                    ┌───────────┴───────────┐
                                                                    ▼                       ▼
                                                           [ HTIE ISR (Half 0) ]   [ TCIE ISR (Half 1) ]
                                                                    └───────────┬───────────┘
                                                                                ▼
                                                                 [ Background DSP & CRC-32 Framing ]
                                                                    │                           │
                                                                    ▼                           ▼
                                                       [ I2C1 OLED (400 kHz) ]     [ Flash Bank 2 Log (64-bit) ]
                                                       (DMAMUX1 ID=17 I2C1_TX)      (0x08040000, 2 KB Pages)
```

---

## 2. Verified Hardware Subsystems & Exact STM32G474RE Parameters

| Subsystem | Peripheral | Exact STM32G474RE Configuration | Verified Constraints (RM0440 / DS12288) |
|---|---|---|---|
| **Clock Tree** | `RCC`/`PWR`/`FLASH` | `170 MHz` Range 1 Boost (`PWR_CR5.R1MODE=0`, `4WS`) | Mandatory RM0440 §6.1.5 `HPRE=/2 -> >=1 us -> /1` |
| **Data Acquisition** | `ADC1` (`Ch1`, `Ch2`, `Ch16`) | **`CKMODE = 11b` (`42.5 MHz <= 60 MHz`)**, `VSENSESEL = 1`, wait `>= 120 us` $t_{\text{START}}$ | **`SMPR2.SMP16 = 640.5` cycles (`15.07 us >= 5.0 us` DS12288 min!)**; `SMP1/SMP2 = 47.5` cycles; factory `TS_CAL1` (`0x1FFF75A8`) & `TS_CAL2` (`0x1FFF75CA`) |
| **DMA & DMAMUX1** | `DMA1_Ch1` (ADC), `DMA1_Ch2` (I2C TX), `DMA1_Ch3` (UART TX) | **`DMAMUX1` Verified Request IDs (RM0440 Table 91):** `ADC1 = 5`, `I2C1_TX = 17`, `USART2_TX = 27` | `volatile` circular ping-pong buffer (`HTIE \| TCIE \| TEIE`); clear **individual** flags `CHTIFx`/`CTCIFx` (NEVER `CGIFx` per ES0430 §2.3.1!) |
| **Display Output** | `I2C1` (`400 kHz` Fast Mode) | `TIMINGR = 0x30612144` @ 170 MHz; `PA15`/`PB7` (`AF4`) recommended *(or `PB8`/`PB9` with external `2.2 kΩ` pull-ups overriding `R36` `BOOT0` 10 kΩ pull-down!)* | Bounded `NACKF`/timeout + 9-clock bus recovery + `I2C_CR2_RELOAD` for 1024-byte framebuffer |
| **Integrity Engine** | `CRC` (`0x40023000`, AHB1) | `POL = 0x04C11DB7`, `INIT = 0xFFFFFFFF`, `REV_IN=01`, `REV_OUT=1` | Hardware IEEE 802.3 `CRC-32` appended to every 64-byte log record |
| **Non-Volatile Storage** | `FLASH` Bank 2 (`0x08040000`) | Dual-Bank mode (`FLASH_OPTR_DBANK = 1`), `2 KB` pages, **mandatory 64-bit aligned double-word writes** (`PG = 1`) with 8-bit ECC | Reading/executing code from Bank 1 (`0x08000000`) while erasing/programming Bank 2 avoids Flash bus stalls during active DMA! |

---

## 3. Ping-Pong Buffer & Log Record Format

### 3.1 Zero-Copy Ping-Pong Ownership Protocol

The DMA operates over `static volatile uint16_t adc_ping_pong[2 * 3 * 32]` (`192` halfwords = `2` halves × `32` scans × `3` channels):
- **Phase 1 (`HTIF1`):** DMA finishes writing `[0 .. 95]` and immediately continues into `[96 .. 191]`. `DMA1_Channel1_IRQHandler` clears `CHTIF1`, sets `g_half_ready = 1`, executes `__DSB()`, and wakes the main loop to process `[0 .. 95]` after a `__DMB()` barrier.
- **Phase 2 (`TCIF1`):** DMA finishes writing `[96 .. 191]` and wraps to `[0 .. 95]`. `DMA1_Channel1_IRQHandler` clears `CTCIF1`, sets `g_full_ready = 1`, executes `__DSB()`, and main loop processes `[96 .. 191]`.

### 3.2 64-Bit Aligned Flash Log Record (32 Bytes = 4 Double-Words)

```c
typedef struct __attribute__((aligned(8))) {
    uint32_t magic;        /* 0x4C4F4732 ("LOG2") */
    uint32_t sequence_id;  /* Monotonic record counter for power-loss recovery */
    uint32_t timestamp_ms; /* SysTick timestamp */
    uint16_t ch1_min_mv, ch1_mean_mv, ch1_max_mv;
    uint16_t ch2_mean_mv;
    int16_t  die_temp_deci_c; /* Tenths of deg C via TS_CAL1/TS_CAL2 */
    uint16_t dma_overruns;
    uint32_t crc32_ieee;   /* Hardware CRC-32 over bytes [0..27] */
} flash_log_record_t;      /* Exact sizeof == 32 bytes (4 x 64-bit Flash writes!) */
```

---

## 4. Verification & Testing Checklist

- [ ] Confirm `ADC12_COMMON->CCR.CKMODE == 11b` (`42.5 MHz`) and `ADC1->SMPR2.SMP16 == 111b` (`640.5` cycles = `15.07 µs`).
- [ ] Verify `DMAMUX1_Channel0->CCR == 5U` (`ADC1`), `HTIE` & `TCIE` fire strictly alternately with `TEIF1 == 0` and `dma_overruns == 0`.
- [ ] Verify OLED displays live Min/Mean/Max and die temperature at `400 kHz` without stalling if OLED is unplugged (`NACKF` handled).
- [ ] Power-cycle board and confirm Flash Bank 2 records pass hardware `CRC-32` verification with zero ECC errors (`FLASH->ECCR`).
