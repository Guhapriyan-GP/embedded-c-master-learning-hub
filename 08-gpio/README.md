# Stage 07: General-Purpose I/O (GPIO) Hardware Engine

## 1. Prerequisites
Stage 06 (RCC & Clock tree).

## 2. Core Technical Theory & Engineering Principles
Every physical pin on an STM32 microcontroller is controlled by a flexible GPIO hardware driver capable of digital input, digital output, alternate peripheral routing, and analog connection.

### 1. Internal Pin Circuitry
A GPIO pin circuit contains:
- **Schmitt Trigger:** Digitizes input voltages; disabled in Analog mode to eliminate static leakage current.
- **Weak Pull-Up / Pull-Down Resistors:** Integrated $pprox 40\ 	ext{k}\Omega$ resistors configurable in software.
- **Complementary Output Driver:** P-MOS and N-MOS transistors driving the pin in Push-Pull or Open-Drain mode.
- **Slew Rate / Speed Control:** Adjusts the rise/fall edge speeds to minimize electromagnetic interference (EMI) or support high-frequency signals.

### 2. Atomic Pin Control via BSRR
Modifying output states using the Output Data Register (`ODR`) requires multi-cycle Read-Modify-Write instructions that are vulnerable to interrupt race conditions.
The **Bit Set/Reset Register (`BSRR`)** allows setting or resetting any combination of 16 pins in a single atomic instruction:
- Writing `1` to bits [15:0] sets the pin HIGH.
- Writing `1` to bits [31:16] resets the pin LOW.
- Writing `0` leaves the pin state unchanged.

## 3. Practical Coding Exercises & Challenges
1. Configure multiple GPIO pins in a single atomic BSRR write.
2. Implement an open-drain output with an external pull-up resistor.
3. Measure the maximum achievable square wave frequency on PA5 using direct register writes.

## 4. Expected Learning Outcomes & Mastery Criteria
Complete mastery of digital I/O circuitry, atomic register manipulation, and alternate function multiplexing.
