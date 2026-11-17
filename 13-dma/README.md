# Stage 12: Direct Memory Access (DMA) & DMAMUX Architecture

## 1. Prerequisites
Stage 12 (Analog acquisition) & Stage 10 (Serial protocols).

## 2. Core Technical Theory & Engineering Principles
Direct Memory Access (DMA) offloads data movement across the bus matrix from the CPU core, allowing high-bandwidth data transfers between memory and peripherals with zero CPU overhead.

### 1. DMA Controller Architecture
The STM32G4 includes two DMA controllers (DMA1 and DMA2), each featuring 8 independent channels:
- **Source & Destination:** Configured via Peripheral Address (`CPAR`) and Memory Address (`CMAR`).
- **Data Sizes:** 8-bit, 16-bit, or 32-bit transfers with automatic alignment.
- **Transfer Direction:** Peripheral-to-Memory, Memory-to-Peripheral, or Memory-to-Memory.
- **Circular Mode:** Counter auto-reloads upon completion, creating continuous circular data streams.

### 2. DMAMUX Request Routing
In older STM32 architectures, peripheral requests were hardwired to specific DMA channels. The STM32G4 features a flexible **DMAMUX** crossbar switch that maps any peripheral hardware trigger (ADC, USART, SPI, Timers) to any DMA channel.

### 3. Double Buffering & Half-Transfer Interrupts
Circular DMA buffers generate two critical interrupts:
- **Half Transfer (`HTIF`):** The first half of the buffer is full and ready for CPU processing.
- **Transfer Complete (`TCIF`):** The second half of the buffer is full.
This allows the CPU to process one half of the buffer while the DMA engine autonomously writes into the other half, guaranteeing jitter-free streaming.

## 3. Practical Coding Exercises & Challenges
1. Implement a high-speed memory-to-memory DMA copy and compare throughput against `memcpy()`.
2. Configure a circular DMA buffer streaming ADC samples to memory.
3. Build an interrupt-driven double-buffered UART receiver using DMA.

## 4. Expected Learning Outcomes & Mastery Criteria
Complete mastery of zero-overhead DMA streaming, DMAMUX routing, and double-buffering architectures.
