# Stage 06: Power, Reset & Clock Control (RCC) Engine

## 1. Prerequisites
Stage 05 (STM32 platform architecture).

## 2. Core Technical Theory & Engineering Principles
The Reset and Clock Control (RCC) subsystem synthesizes and distributes clock signals to the CPU core and all peripheral blocks.

### 1. Clock Sources
- **HSI16 (High-Speed Internal):** 16 MHz internal RC oscillator ($\pm 1\%$ accuracy). Default SYSCLK source on reset.
- **HSE (High-Speed External):** 4 to 48 MHz external crystal or ceramic resonator (NUCLEO boards typically provide 24 MHz from ST-LINK MCO).
- **LSI (Low-Speed Internal):** $pprox 32\ 	ext{kHz}$ internal low-power oscillator for the Independent Watchdog (IWDG).
- **LSE (Low-Speed External):** 32.768 kHz precision crystal for the Real-Time Clock (RTC).

### 2. Main Phase-Locked Loop (PLL) Mathematics
The PLL multiplies an input reference clock ($f_{	ext{IN}}$) to produce high-frequency core and peripheral clocks:
$$f_{	ext{VCO}} = f_{	ext{IN}} 	imes \left( rac{N}{M} ight)$$
$$f_{	ext{PLL\_R}} (	ext{SYSCLK}) = rac{f_{	ext{VCO}}}{R}$$

**Operating Constraints on STM32G4:**
- Input clock to PLL ($f_{	ext{IN}} / M$): Must be between $4\ 	ext{MHz}$ and $16\ 	ext{MHz}$ (4 MHz recommended).
- Internal VCO frequency ($f_{	ext{VCO}}$): Must be between $64\ 	ext{MHz}$ and $344\ 	ext{MHz}$.
- Target SYSCLK: Up to 170 MHz in Range 1 Boost Mode.

### 3. Core Voltage Scaling & Flash Latency
- When operating above 150 MHz up to 170 MHz, the core voltage regulator must be placed in **Range 1 Boost Mode** (`PWR_CR5.R1MODE = 0`).
- Flash memory access requires **4 wait states** at 170 MHz (`FLASH_ACR_LATENCY_4WS`).

## 3. Practical Coding Exercises & Challenges
1. Calculate PLL M, N, R values to achieve 150 MHz from a 24 MHz HSE crystal.
2. Verify clock switching by outputting SYSCLK on the MCO pin (PA8) and measuring with an oscilloscope.
3. Implement software timeout loops when waiting for clock ready flags.

## 4. Expected Learning Outcomes & Mastery Criteria
Ability to configure complex microcontroller clock trees and maximize system throughput safely.
