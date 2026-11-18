# Project 02: High-Speed Multichannel DMA Data Logger with CRC-32 & Dual-Bank Flash Staging

> **Verification Status:** `CROSS-COMPILED` (`arm-none-eabi-gcc 14.3.1`, 0 errors / 0 warnings under strict C11 flags) | `READY FOR AGENT 4 ONBOARD TEST` (Self-contained TIM3 -> ADC1 -> DMA1 -> CRC-32 -> USART2 pipeline requires zero external wires; NOT yet flashed onto physical NUCLEO-G474RE hardware in this commit) | `EXTERNAL HARDWARE PENDING` (Optional `I2C1` SSD1306 OLED on `PA15`/`PB7`)

**Target Concept IDs:** `STM32-RCC-001`, `PERIPH-SYNC-001`, `PERIPH-ADC-001`, `PERIPH-DMA-001`, `PERIPH-DMA-002`, `STM32-ERRATA-001`, `PERIPH-CRC-001`, `PERIPH-I2C-001`, `STM32-FLASH-001`, `EMB-VOL-001`, `EMB-BAR-001`, `PERIPH-WDG-001`, `REL-FAULT-001`

---

## 1. Project Overview & Architecture

The High-Speed Multichannel DMA Data Logger autonomously acquires two internal STM32G474RE analog channels (`ADC1_IN18` `VREFINT` + `ADC1_IN16` calibrated Die Temperature Sensor) triggered at an exact **`100.000 Hz`** rate by **`TIM3` TRGO**, streams samples into a **`volatile` SRAM circular ping-pong buffer** via **`DMA1_Channel1`** (`DMAMUX1 Req 5`, `HTIE | TCIE | TEIE`), computes statistics and factory-calibrated `VDDA` / die temperature, seals every 64-bit-aligned binary frame with a **hardware IEEE 802.3 `CRC-32`** (`0x40023000`), stages 4 × 64-bit double-words for **Flash Bank 2 (`0x08040000`)** in RAM dry-run mode (never modifying Flash option bytes!), streams framed telemetry over **`USART2`**, and drives an optional **`I2C1` SSD1306 OLED** on `PA15`/`PB7` (`AF4`) with bounded `NACKF`/timeout detection so disconnected external hardware never stalls acquisition.

```text
[ TIM3 TRGO (100.000 Hz, MMS=010b) ]
                 │ (EXTSEL=4, EXTEN=01b Rising Edge)
                 ▼
[ ADC1 Scan @ 42.5 MHz: VREFINT (Ch18) + Temp (Ch16, 640.5 cyc) ]
                 │ (DMAMUX1 Ch0, Req ID=5)
                 ▼
[ volatile uint16_t g_adc_ping_pong[64] (2 halves x 16 scans x 2 ch) ]
         ┌───────┴───────┐
         ▼               ▼
   [ HTIF1 Half 0 ] [ TCIF1 Half 1 ]   (Individual CHTIF1/CTCIF1 clear per ES0430 §2.3.1!)
         └───────┬───────┘
                 ▼ (__DMB() ownership handoff)
[ Background DSP + Hardware IEEE 802.3 CRC-32 (0x40023000) ]
         ┌───────┼───────────────┐
         ▼       ▼               ▼
[ USART2 VCP ] [ Bank 2 64-bit ] [ Optional I2C1 OLED ]
(115200 8N1)   (Dry-Run Staging) (PA15/PB7, NACKF-safe)
```

---

## 2. Project File Layout

| File | Role |
|---|---|
| `Inc/logger_packet.h` | 64-bit-aligned 32-byte `logger_frame_t` (`_Static_assert(sizeof == 32 && sizeof % 8 == 0)`), ping-pong geometry, Bank 2 dry-run 64-bit staging helper |
| `Src/main.c` | `170 MHz` clock, hardware `CRC-32` known-answer self-test (`"123456789" -> 0xCBF43926`), `TIM3` TRGO, `ADC1`+`DMA1_Ch1` ping-pong, `USART2` streaming, optional `I2C1` OLED hook |
| `Makefile` | Reproducible cross-compilation linking `23-code-library/Drivers/` & `23-code-library/templates/` |

---

## 3. Verified Hardware Subsystems & Wiring

| Subsystem | Peripheral | Pin / Address | Exact Verified STM32G474RE Configuration | Bench Status |
|---|---|---|---|---|
| **Clock Tree** | `RCC`/`PWR`/`FLASH` | Internal | `170 MHz` Boost Mode (`Clock_Init_170MHz()`, `R1MODE=0`, `4WS`, RM0440 §6.1.5 `HPRE=/2 -> >=1 us -> /1`) | `READY FOR AGENT 4 ONBOARD TEST` |
| **Sample Timer** | `TIM3` Master TRGO | Internal (`APB1`) | `PSC=1699`, `ARR=999` (`170 MHz / 1700 / 1000 = 100.000 Hz` exact), `CR2.MMS = 010b` (Update -> TRGO) | `READY FOR AGENT 4 ONBOARD TEST` |
| **ADC1 Scan** | `ADC1` (`Ch18`, `Ch16`) | Internal (`VREFEN \| VSENSESEL`) | `CKMODE = 11b` (`42.5 MHz <= 60 MHz`), `150 us` startup wait (`>= 120 us` DS12288), `SMP16 = SMP18 = 640.5` cycles (`15.07 us >= 5 us`), `EXTSEL = 4` (`TIM3_TRGO`), `EXTEN = 01b`, `DMAEN \| DMACFG \| OVRMOD` | `READY FOR AGENT 4 ONBOARD TEST` |
| **Ping-Pong DMA** | `DMA1_Ch1` + `DMAMUX1_Ch0` | SRAM1/2 | `DMAMUX_REQ_ADC1 = 5`, circular 16-bit (`64` words), `HTIE \| TCIE \| TEIE`; clears ONLY individual `CHTIF1`/`CTCIF1`/`CTEIF1` (never `CGIF1` per ES0430 §2.3.1) | `READY FOR AGENT 4 ONBOARD TEST` |
| **Integrity Engine** | `CRC` | `0x40023000` (`AHB1`) | `POL = 0x04C11DB7`, `INIT = 0xFFFFFFFF`, `REV_IN = 01`, `REV_OUT = 1`, final `^ 0xFFFFFFFF`; boot self-test verifies `0xCBF43926` | `READY FOR AGENT 4 ONBOARD TEST` |
| **Telemetry UART** | `USART2` (ST-LINK VCP) | `PA2` (TX), `PA3` (RX) | `AF7`, `115200 8N1`; emits `$LOG2,...` framed records every `160 ms` (`6.25 Hz`) + `PA5` (`LD2`) toggle per frame | `READY FOR AGENT 4 ONBOARD TEST` |
| **Flash Bank 2** | `FLASH` Bank 2 | `0x08040000..0x0807FFFF` | Read-only `FLASH->OPTR.DBANK` check + 64-bit aligned 4-double-word RAM dry-run staging (`memcpy`, CRC re-check). **NEVER writes Flash option bytes!** | `READY FOR AGENT 4 ONBOARD TEST` |
| **Optional OLED** | `I2C1` (`400 kHz`) | **`PA15`** (SCL), **`PB7`** (SDA) | `AF4` Open-Drain (`i2c_g4.c` `TIMINGR = 0x30B21F3B`), address `0x3C`, bounded `NACKF`/timeout probe avoids `PB8-BOOT0` bootloader trap and never hangs if absent | `EXTERNAL HARDWARE PENDING` |

---

## 4. 64-Bit-Aligned Telemetry & Flash Frame (`logger_frame_t`)

Compile-time enforced via C11 `_Static_assert` in `Inc/logger_packet.h`:
- `sizeof(logger_frame_t) == 32U` (exact multiple of `8` bytes = `4` STM32G4 64-bit Flash double-words)
- `offsetof(logger_frame_t, crc32_ieee) == 28U` (hardware IEEE 802.3 CRC-32 computed over bytes `[0..27]`)

---

## 5. Build Commands & Exact Compiled Footprint

Verified with **`arm-none-eabi-gcc 14.3.1`** under `-std=c11 -Wall -Wextra -Werror -Wpedantic -Wshadow -Wconversion -Wdouble-promotion -Wundef`:

```bash
# Default debug build (-Og -g3)
make clean all

# Release build (-O2 -g)
make clean all BUILD_TYPE=release
```

| Profile | Artifact | `text` (B) | `data` (B) | `bss` (B) | Flash Used | RAM Used | Compiler Warnings / Errors |
|---|---|---:|---:|---:|---:|---:|---|
| Debug (`-Og -g3`, `170 MHz`) | `build/dma_data_logger.elf` | `11488` | `92` | `2208` | `11588 B` (`2.21%`) | `2296 B` (`2.34%`) | `0` / `0` |
| Release (`-O2 -g`, `170 MHz`) | `build/dma_data_logger.elf` | `10388` | `92` | `2208` | `10488 B` (`2.00%`) | `2296 B` (`2.34%`) | `0` / `0` |

---

## 6. Verification Checklist

### Onboard NUCLEO-G474RE Verification (`READY FOR AGENT 4 ONBOARD TEST` — Zero External Wires)
- [ ] Flash `build/dma_data_logger.bin` to `0x08000000` and open ST-LINK VCP at `115200 8N1`.
- [ ] Confirm boot banner reports `SYSCLK=170000000 Hz | CRC32_TEST=0xCBF43926 (PASS)`.
- [ ] Confirm boot banner reports `I2C1 OLED (PA15/PB7 0x3C): ABSENT (EXTERNAL HARDWARE PENDING)` cleanly without stalling.
- [ ] Confirm `$LOG2` telemetry frames stream every `~160 ms` alternating `half=0` and `half=1`, with `ovr=0`, `te=0`, `ok=1`, valid `vdda_mv` (`~3300 mV`), and plausible die temperature.
- [ ] Confirm `b2=0x08040000`, `0x08040020`, `0x08040040` increments by exact 32-byte 64-bit-aligned strides.

### External Bench Verification (`EXTERNAL HARDWARE PENDING`)
- [ ] Connect an SSD1306 I2C OLED (`0x3C`) to `PA15` (`SCL`) and `PB7` (`SDA`) with `4.7 kΩ` pull-ups to `3V3` and verify boot detection reports `DETECTED`.
