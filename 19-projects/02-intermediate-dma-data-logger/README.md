# Project 02: High-Speed Multichannel DMA Data Logger

## 1. Project Overview & Architecture
The High-Speed DMA Data Logger streams analog sensor data autonomously from the SAR ADC into an SRAM circular ping-pong buffer via Direct Memory Access (DMA), processes statistical summaries (Min, Max, Mean) in background software, and displays live metrics on an I2C OLED display while logging historical samples to embedded Flash memory.

```
[ Sensor Inputs (PA0, PA1, Temp) ]
               │
               ▼
[ ADC1 Multichannel Scan Engine ] ──(Hardware DMA Stream)──> [ SRAM Double Buffer (64 Samples) ]
                                                                             │
                                                                             ├─> Half-Transfer ISR
                                                                             └─> Transfer-Complete ISR
                                                                                     │
                                                                                     ▼
                                                                           [ Background Processing ]
                                                                             │             │
                                                                             ▼             ▼
                                                                        [ I2C OLED ]  [ Flash Log ]
```

## 2. Hardware Subsystems
- **Data Acquisition:** ADC1 configured for continuous scan sequence across Channel 1 (PA0), Channel 2 (PA1), and Channel 16 (Internal Temperature Sensor).
- **Data Transport:** DMA1 Channel 1 mapped via DMAMUX to ADC1 in Circular Mode.
- **Display Output:** SSD1306 128x64 OLED display driven over I2C1 (PB8/PB9) at 400 kHz Fast Mode.
- **Storage Subsystem:** High-density embedded Flash page erase/write routines storing historical records.

## 3. Ping-Pong Buffer Architecture
The DMA operates over a circular array of size $2N$ samples:
- **Phase 1 (First Half):** While the DMA fills buffer elements $[0 \dots N-1]$, the CPU processes elements $[N \dots 2N-1]$.
- **Phase 2 (Second Half):** While the DMA fills buffer elements $[N \dots 2N-1]$, the CPU processes elements $[0 \dots N-1]$.
This guarantees zero data loss and zero sampling jitter, completely decoupling hardware data acquisition from software processing.
