# Stage 10: Serial Communication Protocols (UART, SPI, I2C)

## 1. Prerequisites
Stage 09 (Timers) & Stage 10 (Interrupts).

## 2. Core Technical Theory & Engineering Principles
Embedded systems communicate with external sensors, actuators, and host computers using industry-standard serial protocols.

### 1. Universal Synchronous/Asynchronous Receiver Transmitter (USART/UART)
- **Framing:** Asynchronous transmission using Start bit, 8 data bits, optional Parity bit, and 1-2 Stop bits.
- **Baud Rate Calculation:** Programmed into `USART_BRR`:
  $$	ext{BRR} = rac{f_{	ext{CK}}}{	ext{Baud Rate}}$$
- **Buffering:** Transmit Data Register (`TDR`) and Receive Data Register (`RDR`) interface with internal shift registers.

### 2. Serial Peripheral Interface (SPI)
- **Topology:** High-speed, full-duplex synchronous bus using SCK (Clock), MOSI (Master Out Slave In), MISO (Master In Slave Out), and CS (Chip Select).
- **Modes:** Determined by Clock Polarity (`CPOL`: idle clock state) and Clock Phase (`CPHA`: capture clock edge). Supports data rates exceeding 20 Mbps.

### 3. Inter-Integrated Circuit (I2C)
- **Topology:** Two-wire, multi-master, open-drain bus with pull-up resistors using SCL (Clock) and SDA (Data).
- **Protocol:** Start condition (SDA drops while SCL is HIGH), 7-bit slave address, Read/Write bit, ACK/NACK bit, 8-bit data transfers, and Stop condition.
- **Timing:** Configured via `I2C_TIMINGR` on STM32G4 to establish Standard Mode (100 kHz) and Fast Mode (400 kHz).

## 3. Practical Coding Exercises & Challenges
1. Retarget C `printf` to USART with a circular ring buffer.
2. Write a driver to read sensor data over SPI Mode 0 and Mode 3.
3. Implement an I2C bus scanner that probes all 127 addresses and prints detected devices.

## 4. Expected Learning Outcomes & Mastery Criteria
Ability to implement production-grade serial communication drivers from scratch at register level.
