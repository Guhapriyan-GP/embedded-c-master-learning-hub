# Stage 05: STM32 Platform Architecture & Documentation

## 1. Prerequisites
Stage 04 (Toolchain & build workflow).

## 2. Core Technical Theory & Engineering Principles
The STM32G4 family combines the ARM Cortex-M4 core with advanced analog and digital peripherals interconnected through a multi-layer bus matrix.

### 1. The Multi-Layer AHB Bus Matrix
The bus matrix allows multiple bus masters (CPU I-Code, D-Code, System Bus, DMA1, DMA2) to access different bus slaves (Flash, SRAM1, SRAM2, CCM-SRAM, AHB/APB peripherals) concurrently without blocking, unless two masters contend for the same slave.

### 2. Peripheral Bus Hierarchy
Peripherals are organized across hierarchical buses with different maximum speed ratings:
- **AHB1 / AHB2 (up to 170 MHz):** High-speed peripherals (DMA, DMAMUX, GPIO ports, ADCs).
- **APB1 (up to 170 MHz):** General-purpose timers, UARTs, I2C, SPI, PWR, DACs.
- **APB2 (up to 170 MHz):** High-speed peripherals (TIM1, TIM8, SPI1, USART1, SYSCFG, EXTI).

### 3. Navigating Official Documentation
- **Datasheet (DS12288):** Consult for pin alternate function tables, maximum operating ratings, electrical current draw, and package footprints.
- **Reference Manual (RM0440):** Consult for peripheral block diagrams, register offsets, bit descriptions, and operational sequences.

## 3. Practical Coding Exercises & Challenges
1. Trace an alternate function from a physical pin to a peripheral register using DS12288.
2. Verify the base address of any peripheral in RM0440.
3. Determine the bus speed and prescaler routing for an APB peripheral.

## 4. Expected Learning Outcomes & Mastery Criteria
Fluency in navigating ST technical documentation and mapping physical hardware.
