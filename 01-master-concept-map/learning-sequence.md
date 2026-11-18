# Sequential Learning Roadmap & Advancement Criteria

This document defines the canonical, strictly non-cyclical order of study for bare-metal **STM32G474RE (NUCLEO-G474RE MB1367)** firmware engineering across all **17 logical learning stages (`Step 01`–`Step 17`)**. Every step maps directly to its **Mastery Tier (Level 1 / Level 2 / Level 3)**, **Source-of-Truth Concept IDs**, physical repository directory, exact stage `README.md` H1 title, practical laboratories, and measurable pass criteria.

> **Important Pedagogical Order vs. Physical Folder Note:**  
> To avoid circular hardware dependencies, learners MUST follow the bring-up order below:
> 1. **Basic GPIO on reset 16 MHz HSI16** ([`08-gpio/`](../08-gpio/README.md) — Step 07, Labs 01–02) **BEFORE** full **170 MHz / 160 MHz PWR Boost / Flash 4WS / RCC PLL** ([`07-rcc-and-clocks/`](../07-rcc-and-clocks/README.md) — Step 08, Lab 03, which uses GPIO `PA5`/`PA8` MCO to verify the switched SYSCLK).
> 2. **Interrupts, SysTick, NVIC & EXTI** ([`10-interrupts/`](../10-interrupts/README.md) — Step 09, Labs 04–05) **BEFORE** **Timers & PWM Interrupts** ([`09-timers/`](../09-timers/README.md) — Step 10, Labs 06–07, which immediately require `TIM2_IRQHandler` and `NVIC_EnableIRQ`).

---

## Canonical 17-Stage Mapping Table (`Step 01–17` ↔ Directory ↔ Exact Stage README H1 Title)

| Step | Physical Directory | Exact Stage README H1 Title | Mastery Tier |
|---:|---|---|---|
| **Step 01** | [`01b-digital-electronics-and-hw-integration/`](../01b-digital-electronics-and-hw-integration/README.md) | `Stage 01b: Digital Electronics, NUCLEO-G474RE Hardware Integration & Lab Equipment Mastery` | Level 1 |
| **Step 02** | [`02-c-fundamentals/`](../02-c-fundamentals/README.md) *(+ [`advanced-c-ub-memory-model.md`](../02-c-fundamentals/advanced-c-ub-memory-model.md))* | `Stage 02: C Language Foundations for Bare-Metal Embedded Systems` | Level 1 |
| **Step 03** | [`03-embedded-c/`](../03-embedded-c/README.md) | `Stage 03: Embedded C, Concurrency Primitives & Production Firmware Architecture` | Level 1 |
| **Step 04** | [`04-arm-cortex-m/`](../04-arm-cortex-m/README.md) | `Stage 04: ARM Cortex-M4F (ARMv7E-M) Microarchitecture & Core Peripherals` | Level 1–2 |
| **Step 05** | [`05-toolchain-and-debugging/`](../05-toolchain-and-debugging/README.md) | `Stage 05: GNU ARM Toolchain, Linker Scripts, Startup Code & Hardware Debugging` | Level 1–2 |
| **Step 06** | [`06-stm32-fundamentals/`](../06-stm32-fundamentals/README.md) | `Stage 06: STM32 Platform Architecture, STM32G474RET6 Silicon & ES0430 Errata` *(Part A)* | Level 1 |
| **Step 07** | [`08-gpio/`](../08-gpio/README.md) *(before `07-rcc-and-clocks/`!)* | `Stage 07: General-Purpose I/O (GPIO) Hardware Engine` | Level 1 |
| **Step 08** | [`07-rcc-and-clocks/`](../07-rcc-and-clocks/README.md) | `Stage 06: Power, Reset & Clock Control (RCC) Engine` *(Part B; legacy H1 prefix `Stage 06`)* | Level 2 |
| **Step 09** | [`10-interrupts/`](../10-interrupts/README.md) *(before `09-timers/`!)* | `Stage 08: Interrupts, Exceptions, NVIC, SysTick & EXTI` | Level 2 |
| **Step 10** | [`09-timers/`](../09-timers/README.md) | `Stage 09: Hardware Timers (Basic, General-Purpose, Advanced, LPTIM1 & HRTIM1)` | Level 2–3 |
| **Step 11** | [`11-communication/`](../11-communication/README.md) | `Stage 10: Serial & Industrial Communication Protocols (USART, SPI, I2C, FDCAN, USB, UCPD, SAI, QUADSPI & Framing/CRC)` | Level 2–3 |
| **Step 12** | [`12-adc-dac-analog/`](../12-adc-dac-analog/README.md) | `Stage 11: Mixed-Signal Analog Subsystem (ADC1–5, DAC1–4, COMP1–7, OPAMP1–6, VREFBUF) & Hardware Math Accelerators (CORDIC & FMAC)` | Level 2–3 |
| **Step 13** | [`13-dma/`](../13-dma/README.md) | `Stage 12: Direct Memory Access (DMA1/DMA2) & DMAMUX1 Architecture` | Level 2–3 |
| **Step 14** | [`14-memory-and-storage/`](../14-memory-and-storage/README.md) | `Stage 13: Embedded Memory Subsystems, Dual-Bank Flash, ECC & Bootloaders` | Level 2–3 |
| **Step 15** | [`15-power-management/`](../15-power-management/README.md) | `Stage 14: System Reliability, Watchdogs (IWDG/WWDG), RTC/TAMP & Low-Power Modes` | Level 2 |
| **Step 16** | [`16-rtos/`](../16-rtos/README.md) | `Stage 15: Real-Time Operating Systems (FreeRTOS on ARM Cortex-M4F) & Concurrency` | Level 2–3 |
| **Step 17** | [`17-advanced-firmware-engineering/`](../17-advanced-firmware-engineering/README.md) | `Stage 16: Production Firmware Engineering, Verification, Optimization, Security & Fault Tolerance` | Level 2–3 |

---

### Mastery Tier Legend
- **Level 1 (Foundational):** Mandatory core mental model and safe zero-wire bring-up prerequisites.
- **Level 2 (Production Competency):** Full bare-metal peripheral drivers, RTOS concurrency, debugging, and defensive C engineering.
- **Level 3 (Specialist / STM32G474RE Mastery):** High-resolution digital power (`HRTIM1`, `FMAC`, `CORDIC`), `FDCAN`, `MPU`, Bootloaders, Hardware Security (`RDP`/`PCROP`/`SEC1R`/`TAMP`), and Automated CI/HIL Verification.

---

### Step 01 — Stage 01b (`01b-digital-electronics-and-hw-integration/`): Digital Electronics, NUCLEO-G474RE Hardware Integration & Lab Equipment Mastery *(Level 1)*
- **Folder:** [`01b-digital-electronics-and-hw-integration/`](../01b-digital-electronics-and-hw-integration/README.md) | **Concept IDs:** `DIG-ELEC-001`, `DIG-ELEC-002`, `HWSW-PCB-001`, `HWSW-ANA-001`, `HWSW-LAB-001..003`
- **Core Topics:** Ohm's Law, LED current-limiting resistor sizing, RC time constants ($\tau = RC$), CMOS logic thresholds ($V_{\text{IL}}/V_{\text{IH}}/V_{\text{OL}}/V_{\text{OH}}$), Push-Pull vs. Open-Drain + I2C pull-up sizing, Schmitt trigger hysteresis, NUCLEO-G474RE (`UM2505` MB1367) jumpers (`JP5` 5V select vs. `JP6` $I_{DD}$), solder bridges (`SB6`, `SB17`/`SB23`), `VDDA`/`VREF+` analog rail decoupling, and oscilloscope/logic-analyzer probe basics.
- **Completion Criteria:** Calculate exact LED series resistance and I2C pull-up bounds; verify `JP6` ($I_{DD}$) vs. `JP5` and Active-HIGH `B1` (`PC13`) on MB1367 schematic.

---

### Step 02 — Stage 02 (`02-c-fundamentals/`): C Language Foundations for Bare-Metal Embedded Systems *(Level 1)*
- **Folder:** [`02-c-fundamentals/`](../02-c-fundamentals/README.md) *(+ [`advanced-c-ub-memory-model.md`](../02-c-fundamentals/advanced-c-ub-memory-model.md))* | **Concept IDs:** `C-TYPE-*`, `C-BIT-*`, `C-PTR-*`, `C-STRUCT-*`, `C-QUAL-*`, `C-FPTR-*`, `C-UB-*`, `C-ATOM-*`
- **Core Topics:** Two's complement, `<stdint.h>` fixed-width types, integer promotions & usual arithmetic conversions, bitwise masking (`&`, `|`, `^`, `~`, `<<`, `>>`, `U` suffixes), pointers & pointer arithmetic, function pointers, structs/padding/alignment/unions, storage classes (`static`, `extern`), and Undefined Behavior (UB) hazards.
- **Verification:** Complete Stage 01 Mastery Exam ([`24-assessments/stage-01-c-mastery-exam.md`](../24-assessments/stage-01-c-mastery-exam.md), $\ge 85\%$) and pass `make -C tests test` (`tests/test_c_foundations.c`: **`116/116`** assertions; full suite **`594/594`**) cleanly under `-Wall -Wextra -Werror -Wconversion`.

---

### Step 03 — Stage 03 (`03-embedded-c/`): Embedded C, Concurrency Primitives & Production Firmware Architecture *(Level 1)*
- **Folder:** [`03-embedded-c/`](../03-embedded-c/README.md) | **Concept IDs:** `EMB-MMIO-001`, `EMB-VOL-001`, `EMB-ATOM-001`, `EMB-CMSIS-001`, `FW-*`
- **Core Topics:** Memory-mapped I/O struct overlays, `volatile` semantics and compiler reordering, Read-Modify-Write (RMW) hazards vs. atomic `BSRR`/`BRR`, `rc_w1` / `rc_w0` / write-only hardware register rules, and CMSIS device header anatomy (`stm32g474xx.h`).
- **Verification:** Pass Stage 02 Mastery Exam ([`24-assessments/stage-02-embedded-c-register-exam.md`](../24-assessments/stage-02-embedded-c-register-exam.md), $\ge 85\%$).

---

### Step 04 — Stage 04 (`04-arm-cortex-m/`): ARM Cortex-M4F (ARMv7E-M) Microarchitecture & Core Peripherals *(Level 1–2)*
- **Folder:** [`04-arm-cortex-m/`](../04-arm-cortex-m/README.md) | **Concept IDs:** `ARM-CORE-001..005`, `ARM-ISA-001..002`, `ARM-MEM-001..002`, `ARM-ABI-001..002`, `ARM-EXC-001..002`, `ARM-FPU-001`, `ARM-FLT-001`, `ARM-SYNC-001..002`, `ARM-MPU-001` *(L3)*, `ARM-DSP-001` *(L3)*
- **Core Topics:** **ARMv7E-M** 3-stage pipeline, Harvard buses (`I-Code`, `D-Code`, `S-Bus`, `PPB`), `R0–R15` (`PC` reads current+4, `bit[0]=0`), `xPSR` (`N,Z,C,V,Q,GE[3:0]`), `PRIMASK`/`FAULTMASK`/`BASEPRI`/`CONTROL`, banked `MSP`/`PSP`, Thread vs. Handler mode, Thumb-2 ISA, AAPCS (`-mfloat-abi=hard`), 8-word vs. 26-word FPU stack frames, `EXC_RETURN`, lazy stacking (`FPCCR.LSPEN`), `LDREX`/`STREX`, `DMB`/`DSB`/`ISB`, Fault registers (`CFSR`/`HFSR`/`MMFAR`/`BFAR`), and 8-region MPU.
- **Verification:** Pass Stage 03 Mastery Exam ([`24-assessments/stage-03-arm-cortex-m-architecture-exam.md`](../24-assessments/stage-03-arm-cortex-m-architecture-exam.md), $\ge 85\%$).

---

### Step 05 — Stage 05 (`05-toolchain-and-debugging/`): GNU ARM Toolchain, Linker Scripts, Startup Code & Hardware Debugging *(Level 1–2)*
- **Folder:** [`05-toolchain-and-debugging/`](../05-toolchain-and-debugging/README.md) | **Concept IDs:** `TOOL-GCC-001`, `TOOL-LD-001..004`, `TOOL-OCD-001`, `ARM-EXC-003`, `ARM-DBG-001`, `PROF-STK-001`, `PROF-WCET-001`
- **Core Topics:** `arm-none-eabi-gcc` pipeline, `-fstack-usage`, ELF inspection (`readelf`, `objdump`, `nm`, `size`), linker scripts (`FLASH`, `SRAM1`, `SRAM2`, `CCMRAM (xrw)` + `_siccmram`, `.noinit (NOLOAD)`), 118-entry STM32G474RE vector table (`IRQ 0..101`), `Reset_Handler`, OpenOCD + ST-LINK/V3E SWD, GDB register/memory/SVD inspection, `DWT->CYCCNT` cycle profiling, ITM/SWO on `PB3`, and `DBGMCU` freeze registers.
- **Verification:** Compile and inspect `.elf`/`.map` from [`23-code-library/templates/`](../23-code-library/templates/) ([library guide](../23-code-library/README.md)) with zero warnings under `-Wall -Wextra -Werror`.

---

### Step 06 — Stage 06 [Part A] (`06-stm32-fundamentals/`): STM32 Platform Architecture, STM32G474RET6 Silicon & ES0430 Errata *(Level 1)*
- **Folder:** [`06-stm32-fundamentals/`](../06-stm32-fundamentals/README.md) | **Concept IDs:** `STM32-ARCH-001`, `STM32-SRAM-001`, `STM32-BOOT-001`, `STM32-ERRATA-001`, `HWSW-PCB-002`
- **Core Topics:** Multi-layer AHB BusMatrix, 4 GB memory map (including CCM-SRAM `0x10000000` native vs. `0x20018000` DMA alias and AHB2 `DAC1–4`), LQFP64 52-pin exact availability (no Port E, no SPI4, no FMC, no AES), `RM0440`/`DS12288`/`UM2505`/`ES0430` navigation, and pin conflict avoidance.

---

### Step 07 — Stage 07 (`08-gpio/`): General-Purpose I/O (GPIO) Hardware Engine *(Level 1)*
- **Folder:** [`08-gpio/`](../08-gpio/README.md) *(study before `07-rcc-and-clocks/`!)* | **Concept IDs:** `PERIPH-GPIO-001..005`, `GPIO-FMP-001`
- **Core Topics:** Minimal clock gating (`RCC->AHB2ENR`), STM32G4 reset default **`11` Analog** (`GPIOA=0xABFFFFFF`), Push-Pull vs. Open-Drain, `OSPEEDR`, `PUPDR`, atomic `BSRR`/`BRR`, `LCKR`, `AFR[0..1]`, Active-HIGH `B1` (`PC13`), and `PWR_CR3.UCPD_DBDIS` on `PA9`/`PA10`/`PB6`/`PB4`.
- **Laboratories:** [`lab-01-baremetal-gpio-led-blinky`](../18-practical-laboratory/lab-01-baremetal-gpio-led-blinky/README.md) & [`lab-02-gpio-pushbutton-debouncing`](../18-practical-laboratory/lab-02-gpio-pushbutton-debouncing/README.md).

---

### Step 08 — Stage 06 [Part B] (`07-rcc-and-clocks/`): Power, Reset & Clock Control (RCC) Engine *(Level 2)*
- **Folder:** [`07-rcc-and-clocks/`](../07-rcc-and-clocks/README.md) *(README H1 carries legacy prefix `Stage 06:`; logically Step 08 after `08-gpio/`!)* | **Concept IDs:** `STM32-RCC-001..003`, `STM32-PWR-001`, `STM32-FLASH-001`, `CLK-PLL-001`
- **Core Topics:** HSI16, onboard **24 MHz HSE crystal X3**, HSI48+CRS, LSI/LSE, PLL limits ($f_{\text{PLL-IN}} \in [2.66, 16]\ \text{MHz}$, $f_{\text{VCO}} \in [96, 344]\ \text{MHz}$), both **170 MHz maximum-performance** ($f_{\text{VCO}} = 340\ \text{MHz}$, $M=6/N=85/R=2$ from HSE or $M=4/N=85/R=2$ from HSI16) and **160 MHz clean-integer** ($f_{\text{VCO}} = 320\ \text{MHz}$, $M=3/N=40/R=2$ or $M=6/N=80/R=2$ from HSE, giving exact $40\ \text{MHz}$ ADC `HCLK/4` and $10.000\ \text{MHz}$ `MCO /16`) profiles — both requiring `PWR_CR5` (`0x80`) Range 1 Boost (`R1MODE=0`) + `PWR_SR2.VOSF` poll, `FLASH_ACR` 4 Wait States + ART caches, and the mandatory **RM0440 §6.1.5 `HPRE=/2` ($\ge 1\ \mu\text{s}$) intermediate step**, plus `CCIPR`/`CCIPR2`, CSS NMI, and `MCO` verification on `PA8`.
- **Laboratory:** [`lab-03-rcc-pll-170mhz-clock-switch`](../18-practical-laboratory/lab-03-rcc-pll-170mhz-clock-switch/README.md).

---

### Step 09 — Stage 08 (`10-interrupts/`): Interrupts, Exceptions, NVIC, SysTick & EXTI *(Level 2)*
- **Folder:** [`10-interrupts/`](../10-interrupts/README.md) *(study before `09-timers/`!)* | **Concept IDs:** `ARM-NVIC-001`, `ARM-SYST-001`, `PERIPH-SYSTICK-001`, `PERIPH-EXTI-001..003`, `RT-IRQ-001..002`, `RT-TIME-002`
- **Core Topics:** SysTick 24-bit timer, NVIC `ISER`/`ICER`/`IP` (4 priority bits on G474RE), `PRIGROUP` preemption vs. subpriority, `BASEPRI` vs. `PRIMASK`, 12-cycle latency / 6-cycle tail-chaining / late arrival, `SYSCFG->EXTICR`, `EXTI` rising/falling triggers (`RTSR1` for Active-HIGH `PC13`), direct `EXTI->PR1 = mask` clearing, and the 2-mechanism synchronization pattern before ISR exit: (1) peripheral status-register dummy readback (`(void)EXTI->PR1;`) per **ARM AN321 §4.9** so the AHB-to-APB2 posted-write bridge commits the clear at the EXTI slave (preventing spurious duplicate re-entry), followed by (2) `__DSB();` to drain the CPU write buffer and mitigate **ARM Cortex-M4 r0p1 Erratum 838869 / ST ES0430 §2.1.3** (preventing a stalling store-immediate overlapping exception return from vectoring to an incorrect interrupt).
- **Laboratories:** [`lab-04-systick-accurate-delay-generator`](../18-practical-laboratory/lab-04-systick-accurate-delay-generator/README.md) & [`lab-05-exti-pushbutton-interrupt`](../18-practical-laboratory/lab-05-exti-pushbutton-interrupt/README.md).

---

### Step 10 — Stage 09 (`09-timers/`): Hardware Timers (Basic, General-Purpose, Advanced, LPTIM1 & HRTIM1) *(Level 2–3)*
- **Folder:** [`09-timers/`](../09-timers/README.md) | **Concept IDs:** `PERIPH-TIM-001..004`, `PERIPH-LPTIM-001`, `PERIPH-HRTIM-001` *(L3)*, `TIM-DMA-001`
- **Core Topics:** Basic (`TIM6`/`7`), GP (`TIM2`/`5` 32-bit, `TIM3`/`4`, `TIM15–17`), Advanced (`TIM1`/`8`/`20`: complementary PWM, `BDTR` dead-time + `MOE` + `BKIN` overcurrent break), Input Capture, Encoder mode, write-only `EGR = UG` & `rc_w0` `SR = ~flag`, `LPTIM1`, and flagship **`HRTIM1`** (184 ps DLL, Master + TIMA–F) *(Level 3)*.
- **Laboratories:** [`lab-06-general-purpose-timer-periodic-interrupt`](../18-practical-laboratory/lab-06-general-purpose-timer-periodic-interrupt/README.md) & [`lab-07-timer-pwm-rgb-led-fading`](../18-practical-laboratory/lab-07-timer-pwm-rgb-led-fading/README.md).

---

### Step 11 — Stage 10 (`11-communication/`): Serial & Industrial Communication Protocols (USART, SPI, I2C, FDCAN, USB, UCPD, SAI, QUADSPI & Framing/CRC) *(Level 2–3)*
- **Folder:** [`11-communication/`](../11-communication/README.md) | **Concept IDs:** `PERIPH-UART-001`, `PERIPH-SPI-001`, `PERIPH-I2C-001`, `PERIPH-FDCAN-001` *(L3)*, `PERIPH-USB-001` *(L3)*, `PERIPH-UCPD-001` *(L3)*, `PERIPH-SAI-001` *(L3)*, `PERIPH-QSPI-001` *(L3)*
- **Core Topics:** `USART1–3`/`UART4–5`/`LPUART1` (`RDR`/`TDR`, `PRESC`, FIFO, `IDLE`+DMA), `SPI1–3` (`CR2.FRXTH=1` mandatory for 8-bit, 8-bit `DR` access, `PA5`/`LD2` caveat), `I2C1–4` (`TIMINGR`, `RXDR`/`TXDR`, Repeated START, `AUTOEND`/`RELOAD`, NACK timeout, `PB8-BOOT0` pull-down caveat), plus Level 3 **`FDCAN1–3`** (shared Message RAM `0x4000A400`) and crystal-less **USB FS + CRS**.
- **Laboratories:** [`lab-08-usart-baremetal-driver-printf`](../18-practical-laboratory/lab-08-usart-baremetal-driver-printf/README.md), [`lab-11-spi-master-sensor-interfacing`](../18-practical-laboratory/lab-11-spi-master-sensor-interfacing/README.md), [`lab-12-i2c-master-ssd1306-oled`](../18-practical-laboratory/lab-12-i2c-master-ssd1306-oled/README.md), [`lab-18-fdcan-loopback-and-crc`](../18-practical-laboratory/lab-18-fdcan-loopback-and-crc/README.md).

---

### Step 12 — Stage 11 (`12-adc-dac-analog/`): Mixed-Signal Analog Subsystem (ADC1–5, DAC1–4, COMP1–7, OPAMP1–6, VREFBUF) & Hardware Math Accelerators (CORDIC & FMAC) *(Level 2–3)*
- **Folder:** [`12-adc-dac-analog/`](../12-adc-dac-analog/README.md) | **Concept IDs:** `PERIPH-ADC-001..004`, `PERIPH-DAC-001`, `PERIPH-OPAMP-001` *(L3)*, `PERIPH-COMP-001` *(L3)*, `PERIPH-VREFBUF-001` *(L3)*
- **Core Topics:** SAR ADC clock compliance ($\le 60\ \text{MHz}$ single / $\le 52\ \text{MHz}$ multi → `CKMODE=11` `HCLK/4=42.5 MHz` @ 170 MHz or `40.0 MHz` @ 160 MHz), `DEEPPWD` exit (`0x20000000`), calibration, RC source-impedance settling & temp sensor $t_S \ge 5\ \mu\text{s}$, oversampling, AHB2 `DAC1–4` (`MCR.HFSEL=10` >160 MHz, sawtooth wave), plus Level 3 `OPAMP1–6` PGA→ADC, `COMP1–7`→`TIM1/HRTIM` hardware break, and `VREFBUF`.
- **Laboratories:** [`lab-09-adc-single-channel-potentiometer`](../18-practical-laboratory/lab-09-adc-single-channel-potentiometer/README.md) & [`lab-16-dac-opamp-comp-hardware-break`](../18-practical-laboratory/lab-16-dac-opamp-comp-hardware-break/README.md).

---

### Step 13 — Stage 12 (`13-dma/`): Direct Memory Access (`DMA1`/`DMA2`) & `DMAMUX1` Architecture *(Level 2–3)*
- **Folder:** [`13-dma/`](../13-dma/README.md) | **Concept IDs:** `PERIPH-DMA-001..003`, `PERIPH-CORDIC-001` *(L3)*, `PERIPH-FMAC-001` *(L3)*, `PROF-PERF-001..003`
- **Core Topics:** 16-channel `DMA1`/`DMA2`, `DMAMUX1` Request IDs (RM0440 Table 91), Synchronization & Request Generators, circular ping-pong (`HTIF`/`TCIF`), ES0430 §2.3.1 individual flag clearing (never `CGIFx`), CCM-SRAM DMA alias `0x20018000`, and Level 3 `CORDIC` (`q1.31` trig) + `FMAC` (170 MMAC/s FIR/IIR).
- **Laboratories:** [`lab-10-adc-dma-multichannel-circular-buffer`](../18-practical-laboratory/lab-10-adc-dma-multichannel-circular-buffer/README.md), [`lab-17-cordic-fmac-hardware-math`](../18-practical-laboratory/lab-17-cordic-fmac-hardware-math/README.md) + Stage 04 Mastery Exam ([`24-assessments/stage-04-stm32-peripherals-exam.md`](../24-assessments/stage-04-stm32-peripherals-exam.md), $\ge 85\%$).

---

### Step 14 — Stage 13 (`14-memory-and-storage/`): Embedded Memory Subsystems, Dual-Bank Flash, ECC & Bootloaders *(Level 2–3)*
- **Folder:** [`14-memory-and-storage/`](../14-memory-and-storage/README.md) | **Concept IDs:** `STM32-FLASH-002`, `BOOT-FLASH-001`, `BOOT-JUMP-001`, `BOOT-VERIF-001` *(L3)*, `BOOT-UPD-001` *(L3)*, `MEM-OB-001` *(L3)*, `SEC-RDP-001` *(L3)*, `SEC-MEM-001` *(L3)*, `SEC-BOOT-001` *(L3)*, `SEC-TAMP-001` *(L3)*, `SEC-CRYPTO-001` *(L3)*, `PERIPH-CRC-001`, `PERIPH-RNG-001`
- **Core Topics:** Dual-Bank (`DBANK=1`, 2 KB pages, RWW) vs. Single-Bank (`DBANK=0`, 4 KB pages), mandatory 64-bit aligned writes + 8-bit ECC (`FLASH_ECCR`), safe 9-step bootloader handoff (`SCB->VTOR` 512-byte aligned), hardware `CRC`/`RNG`, and Level 3 `RDP` (0/1, never L2 on dev boards!), `WRP`, `PCROP`, Securable Memory (`SEC1R`), and `TAMP` 32 backup registers.

---

### Step 15 — Stage 14 (`15-power-management/`): System Reliability, Watchdogs (`IWDG`/`WWDG`), `RTC`/`TAMP` & Low-Power Modes *(Level 2)*
- **Folder:** [`15-power-management/`](../15-power-management/README.md) | **Concept IDs:** `STM32-PWR-002..003`, `PERIPH-WDG-001..002`, `PERIPH-RTC-001`, `REL-BOR-001`, `REL-WDG-001`, `ARM-PWR-001`, `PROF-PWR-001..002`
- **Core Topics:** `IWDG` (LSI + `WINR` window mode + `DBGMCU` freeze), `WWDG` (`WDGTB` `/1../128`, `EWI` IRQ 0), `BOR_LEV` & `PVD`, `RCC->CSR` reset cause decoding, and exact **STM32G474RE Low-Power Modes: Sleep (11–12 cycle wakeup), Low-Power Run, Low-Power Sleep, Stop 0, Stop 1, Standby (with optional SRAM2 `RRS`), and Shutdown** *(Note: Stop 2 does NOT exist on STM32G4!)*. Current measurement on **`JP6`** (`IDD`), NOT `JP5`.
- **Laboratories:** [`lab-13-iwdg-hardware-watchdog`](../18-practical-laboratory/lab-13-iwdg-hardware-watchdog/README.md) & [`lab-14-low-power-stop-mode-wakeup`](../18-practical-laboratory/lab-14-low-power-stop-mode-wakeup/README.md).

---

### Step 16 — Stage 15 (`16-rtos/`): Real-Time Operating Systems (FreeRTOS on ARM Cortex-M4F) & Concurrency *(Level 2–3)*
- **Folder:** [`16-rtos/`](../16-rtos/README.md) | **Concept IDs:** `RTOS-ARCH-001`, `RTOS-SCHED-001`, `RTOS-CTX-001`, `RTOS-TASK-001..002`, `RTOS-IPC-001..002`, `RTOS-SYNC-001..004`, `RTOS-ISR-001..002`, `RTOS-HAZ-001..002`, `RTOS-MEM-001..002`, `RTOS-PORT-001`, `RTOS-PWR-001` *(L3)*, `RTOS-THEORY-001` *(L3)*
- **Core Topics:** Preemptive vs. cooperative scheduling, `SVC`/`SysTick`/`PendSV` context switching with FPU `S16–S31` lazy stacking, `NVIC_SetPriorityGrouping(0)` (or `3`), `BASEPRI` & `configMAX_SYSCALL_INTERRUPT_PRIORITY`, Queues, Direct-to-Task Notifications, Binary/Counting Semaphores, Mutexes & Priority Inheritance (distinct from deadlock!), Event Groups, `FromISR` + `portYIELD_FROM_ISR`, static allocation (`xTaskCreateStatic`), `heap_1..heap_5`, Stack High-Water Mark & Overflow Method 2, and Level 3 LPTIM1 Tickless Idle + RMS/RTA schedulability.
- **Laboratory:** [`lab-15-freertos-multitask-producer-consumer`](../18-practical-laboratory/lab-15-freertos-multitask-producer-consumer/README.md) & pinned **`FreeRTOS-Kernel V11.1.0`** project in [`23-code-library/freertos/`](../23-code-library/freertos/README.md).

---

### Step 17 — Stage 16 (`17-advanced-firmware-engineering/`): Production Firmware Engineering, Verification, Optimization, Security & Fault Tolerance *(Level 2–3)*
- **Folder:** [`17-advanced-firmware-engineering/`](../17-advanced-firmware-engineering/README.md) | **Concept IDs:** `BOOT-ARCH-001`, `BOOT-CFG-001`, `REL-CRASH-001`, `PROF-STD-001..004`, `PROF-SA-001..004`, `PROF-DA-001..004`, `PROF-REV-001`, `TEST-UNIT-001..002`, `TEST-MOCK-001`, `TEST-HIL-001..002`, `PROF-CI-001`, `PROF-GIT-001..002`, `PROF-DOC-001..002`, `PROF-REL-001..002`
- **Core Topics:** Layered architecture & Active Objects, FSM/HSM, lock-free SPSC ring buffers, naked-asm HardFault crash logger to `.noinit` / `TAMP->BKPxR`, MISRA C:2012/2023 + SEI CERT C + Barr Group + Deviation Management, `cppcheck`/`clang-tidy`/`-fanalyzer`, Unity/Ceedling/CMock host unit tests with fake CMSIS registers + ASan/UBSan, Renode emulation, and GitHub Actions CI.
- **Verification:** [`lab-19-hardfault-mpu-and-crash-diagnostics`](../18-practical-laboratory/lab-19-hardfault-mpu-and-crash-diagnostics/README.md), [`lab-20-host-unit-testing-and-driver-mocking`](../18-practical-laboratory/lab-20-host-unit-testing-and-driver-mocking/README.md), Stage 05 Mastery Exam ([`24-assessments/stage-05-rtos-and-advanced-firmware-exam.md`](../24-assessments/stage-05-rtos-and-advanced-firmware-exam.md), $\ge 85\%$), and Capstone Projects ([`19-projects/`](../19-projects/README.md)).
