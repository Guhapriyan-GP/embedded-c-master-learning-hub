# Sequential Learning Roadmap & Advancement Criteria

This document defines the canonical, strictly non-cyclical order of study for bare-metal **STM32G474RE (NUCLEO-G474RE MB1367)** firmware engineering. Every stage maps directly to its **Mastery Tier (Level 1 / Level 2 / Level 3)**, **Source-of-Truth Concept IDs**, physical repository directory, laboratory, and measurable pass criteria.

> **Important Pedagogical Order vs. Physical Folder Note:**  
> To avoid circular dependencies, learners MUST follow the bring-up order below:
> 1. **Basic GPIO on reset 16 MHz HSI16** (`08-gpio/` + Labs 01–02) **BEFORE** full **170 MHz PWR Boost / Flash 4WS / RCC PLL** (`07-rcc-and-clocks/` + Lab 03, which uses GPIO `PA5`/`PA8` MCO to verify 170 MHz).
> 2. **Interrupts, SysTick, NVIC & EXTI** (`10-interrupts/` + Labs 04–05) **BEFORE** **Timers & PWM Interrupts** (`09-timers/` + Labs 06–07, which immediately require `TIM2_IRQHandler` and `NVIC_EnableIRQ`).

---

### Mastery Tier Legend
- **Level 1 (Foundational):** Mandatory core mental model and safe bring-up prerequisites.
- **Level 2 (Production Competency):** Full bare-metal peripheral drivers, RTOS concurrency, debugging, and defensive C engineering.
- **Level 3 (Specialist / STM32G474RE Mastery):** High-resolution digital power (`HRTIM1`, `FMAC`, `CORDIC`), `FDCAN`, `MPU`, Bootloaders, Hardware Security (`RDP`/`PCROP`/`SEC1R`/`TAMP`), and Automated CI/HIL Verification.

---

### Stage 00: Digital Electronics & Hardware Interfacing Primer *(Level 1)*
- **Concept IDs:** `DIG-ELEC-001`, `DIG-ELEC-002`, `HWSW-PCB-001`, `HWSW-LAB-001..003`
- **Core Topics:** Ohm's Law, LED current-limiting resistor sizing, RC time constants ($\tau = RC$), CMOS logic thresholds ($V_{\text{IL}}/V_{\text{IH}}/V_{\text{OL}}/V_{\text{OH}}$), Push-Pull vs. Open-Drain + I2C pull-up sizing, Schmitt trigger hysteresis, NUCLEO-G474RE (`UM2505` MB1367) jumpers (`JP5` 5V select vs. `JP6` $I_{DD}$), solder bridges (`SB6`, `SB17`/`SB23`), and oscilloscope/logic-analyzer probe basics.
- **Completion Criteria:** Calculate exact LED series resistance and I2C pull-up bounds; verify `JP6` ($I_{DD}$) vs. `JP5` and Active-HIGH `B1` (`PC13`) on MB1367 schematic.

---

### Stage 01: Computer Science & C Language Foundations *(Level 1)*
- **Folder:** `02-c-fundamentals/` | **Concept IDs:** `C-TYPE-*`, `C-BIT-*`, `C-PTR-*`, `C-STRUCT-*`, `C-QUAL-*`, `C-FPTR-*`
- **Core Topics:** Two's complement, `<stdint.h>` fixed-width types, integer promotions & usual arithmetic conversions, bitwise masking (`&`, `|`, `^`, `~`, `<<`, `>>`, `U` suffixes), pointers & pointer arithmetic, function pointers, structs/padding/alignment/unions, storage classes (`static`, `extern`), and Undefined Behavior (UB) hazards.
- **Verification:** Complete Stage 01 Mastery Exam (`24-assessments/stage-01-c-mastery-exam.md`, $\ge 85\%$) and compile bit-packing exercises cleanly under `-Wall -Wextra -Werror -Wconversion`.

---

### Stage 02: Embedded C & Memory-Mapped Architecture *(Level 1)*
- **Folder:** `03-embedded-c/` | **Concept IDs:** `EMB-MMIO-001`, `EMB-VOL-001`, `EMB-ATOM-001`, `EMB-CMSIS-001`
- **Core Topics:** Memory-mapped I/O struct overlays, `volatile` semantics and compiler reordering, Read-Modify-Write (RMW) hazards vs. atomic `BSRR`/`BRR`, `rc_w1` / `rc_w0` / write-only hardware register rules, and CMSIS device header anatomy (`stm32g474xx.h`).
- **Verification:** Pass Stage 02 Mastery Exam (`24-assessments/stage-02-embedded-c-register-exam.md`, $\ge 85\%$).

---

### Stage 03: ARM Cortex-M4F Microarchitecture & Assembly *(Level 1–2)*
- **Folder:** `04-arm-cortex-m/` | **Concept IDs:** `ARM-CORE-001..005`, `ARM-ISA-001..002`, `ARM-MEM-001..002`, `ARM-ABI-001..002`, `ARM-EXC-001..002`, `ARM-FPU-001`, `ARM-FLT-001`, `ARM-SYNC-001..002`, `ARM-MPU-001` *(L3)*, `ARM-DSP-001` *(L3)*
- **Core Topics:** **ARMv7E-M** 3-stage pipeline, Harvard buses (`I-Code`, `D-Code`, `S-Bus`, `PPB`), `R0–R15` (`PC` reads current+4, `bit[0]=0`), `xPSR` (`N,Z,C,V,Q,GE[3:0]`), `PRIMASK`/`FAULTMASK`/`BASEPRI`/`CONTROL`, banked `MSP`/`PSP`, Thread vs. Handler mode, Thumb-2 ISA, AAPCS (`-mfloat-abi=hard`), 8-word vs. 26-word FPU stack frames, `EXC_RETURN`, lazy stacking (`FPCCR.LSPEN`), `LDREX`/`STREX`, `DMB`/`DSB`/`ISB`, Fault registers (`CFSR`/`HFSR`/`MMFAR`/`BFAR`), and 8-region MPU.
- **Verification:** Pass Stage 03 Mastery Exam (`24-assessments/stage-03-arm-cortex-m-architecture-exam.md`, $\ge 85\%$).

---

### Stage 04: GNU Toolchain, Linker Scripts, Startup & Debugging *(Level 1–2)*
- **Folder:** `05-toolchain-and-debugging/` | **Concept IDs:** `TOOL-GCC-001`, `TOOL-LD-001`, `TOOL-OCD-001`, `ARM-EXC-003`, `ARM-DBG-001`, `PROF-STK-001`, `PROF-WCET-001`
- **Core Topics:** `arm-none-eabi-gcc` pipeline, `-fstack-usage`, ELF inspection (`readelf`, `objdump`, `nm`, `size`), linker scripts (`FLASH`, `SRAM1`, `SRAM2`, `CCMRAM (xrw)` + `_siccmram`, `.noinit (NOLOAD)`), 118-entry STM32G474RE vector table (`IRQ 0..101`), `Reset_Handler`, OpenOCD + ST-LINK/V3E SWD, GDB register/memory/SVD inspection, `DWT->CYCCNT` cycle profiling, ITM/SWO on `PB3`, and `DBGMCU` freeze registers.
- **Verification:** Compile and inspect `.elf`/`.map` from `23-code-library/templates/` with zero warnings under `-Wall -Wextra -Werror`.

---

### Stage 05: STM32G474RE Platform Architecture & Documentation Navigation *(Level 1)*
- **Folder:** `06-stm32-fundamentals/` | **Concept IDs:** `STM32-ARCH-001`, `STM32-SRAM-001`, `STM32-BOOT-001`, `STM32-ERRATA-001`, `HWSW-PCB-002`
- **Core Topics:** Multi-layer AHB BusMatrix, 4 GB memory map (including CCM-SRAM `0x10000000` native vs. `0x20018000` DMA alias and AHB2 `DAC1–4`), LQFP64 52-pin exact availability (no Port E, no SPI4, no FMC, no AES), `RM0440`/`DS12288`/`UM2505`/`ES0430` navigation, and pin conflict avoidance.

---

### Stage 06A: General-Purpose I/O (GPIO) on Reset 16 MHz HSI16 *(Level 1)*
- **Folder:** `08-gpio/` | **Concept IDs:** `PERIPH-GPIO-001`
- **Core Topics:** Minimal clock gating (`RCC->AHB2ENR`), STM32G4 reset default **`11` Analog** (`GPIOA=0xABFFFFFF`), Push-Pull vs. Open-Drain, `OSPEEDR`, `PUPDR`, atomic `BSRR`/`BRR`, `LCKR`, `AFR[0..1]`, Active-HIGH `B1` (`PC13`), and `PWR_CR3.UCPD_DBDIS` on `PA9`/`PA10`/`PB6`/`PB4`.
- **Laboratories:** `lab-01-baremetal-gpio-led-blinky` & `lab-02-gpio-pushbutton-debouncing`.

---

### Stage 06B: Power Scaling (PWR), Flash Latency & 170 MHz Clock Tree (RCC) *(Level 2)*
- **Folder:** `07-rcc-and-clocks/` | **Concept IDs:** `STM32-RCC-001..003`, `STM32-PWR-001`, `STM32-FLASH-001`
- **Core Topics:** HSI16, onboard **24 MHz HSE crystal X3**, HSI48+CRS, LSI/LSE, PLL limits ($f_{\text{PLL\_IN}} \in [2.66, 16]\ \text{MHz}$, $f_{\text{VCO}} \in [96, 344]\ \text{MHz}$), `PWR_CR5` (`0x80`) Range 1 Boost + `PWR_SR2.VOSF` poll, `FLASH_ACR` 4 Wait States + ART caches, mandatory **RM0440 §6.1.5 `HPRE=/2` (≥ 1 µs) intermediate step**, `CCIPR`/`CCIPR2`, CSS NMI, and `MCO` verification on `PA8`.
- **Laboratory:** `lab-03-rcc-pll-170mhz-clock-switch`.

---

### Stage 07: Interrupts, Exceptions, SysTick, NVIC & EXTI *(Level 2)*
- **Folder:** `10-interrupts/` | **Concept IDs:** `ARM-NVIC-001`, `ARM-SYST-001`, `PERIPH-EXTI-001`
- **Core Topics:** SysTick 24-bit timer, NVIC `ISER`/`ICER`/`IP` (4 priority bits on G474RE), `PRIGROUP` preemption vs. subpriority, `BASEPRI` vs. `PRIMASK`, 12-cycle latency / 6-cycle tail-chaining / late arrival, `SYSCFG->EXTICR`, `EXTI` rising/falling triggers (`RTSR1` for Active-HIGH `PC13`), direct `EXTI->PR1 = mask` clearing, and `__DSB()` before ISR exit (`ES0430 §2.1.3`).
- **Laboratories:** `lab-04-systick-accurate-delay-generator` & `lab-05-exti-pushbutton-interrupt`.

---

### Stage 08: General-Purpose, Advanced & High-Resolution Timers (`TIMx` & `HRTIM1`) *(Level 2–3)*
- **Folder:** `09-timers/` | **Concept IDs:** `PERIPH-TIM-001..003`, `PERIPH-LPTIM-001`, `PERIPH-HRTIM-001` *(L3)*
- **Core Topics:** Basic (`TIM6`/`7`), GP (`TIM2`/`5` 32-bit, `TIM3`/`4`, `TIM15–17`), Advanced (`TIM1`/`8`/`20`: complementary PWM, `BDTR` dead-time + `MOE` + `BKIN` overcurrent break), Input Capture, Encoder mode, write-only `EGR = UG` & `rc_w0` `SR = ~flag`, `LPTIM1`, and flagship **`HRTIM1`** (184 ps DLL, Master + TIMA–F) *(Level 3)*.
- **Laboratories:** `lab-06-general-purpose-timer-periodic-interrupt` & `lab-07-timer-pwm-rgb-led-fading`.

---

### Stage 09: Communication Protocols (`USART`, `LPUART`, `SPI`, `I2C`, `FDCAN`, `USB`) *(Level 2–3)*
- **Folder:** `11-communication/` | **Concept IDs:** `PERIPH-UART-001`, `PERIPH-SPI-001`, `PERIPH-I2C-001`, `PERIPH-FDCAN-001` *(L3)*, `PERIPH-USB-001` *(L3)*, `PERIPH-UCPD-001` *(L3)*, `PERIPH-SAI-001` *(L3)*, `PERIPH-QSPI-001` *(L3)*
- **Core Topics:** `USART1–3`/`UART4–5`/`LPUART1` (`RDR`/`TDR`, `PRESC`, FIFO, `IDLE`+DMA), `SPI1–3` (`CR2.FRXTH=1` mandatory for 8-bit, 8-bit `DR` access, `PA5`/`LD2` caveat), `I2C1–4` (`TIMINGR`, `RXDR`/`TXDR`, Repeated START, `AUTOEND`/`RELOAD`, NACK timeout, `PB8-BOOT0` pull-down caveat), plus Level 3 **`FDCAN1–3`** (shared Message RAM `0x4000A400`) and crystal-less **USB FS + CRS**.
- **Laboratories:** `lab-08-usart-baremetal-driver-printf`, `lab-11-spi-master-sensor-interfacing`, `lab-12-i2c-master-ssd1306-oled`.

---

### Stage 10: High-Speed Mixed-Signal Analog (`ADC1–5`, `DAC1–4`, `OPAMP1–6`, `COMP1–7`, `VREFBUF`) *(Level 2–3)*
- **Folder:** `12-adc-dac-analog/` | **Concept IDs:** `PERIPH-ADC-001`, `PERIPH-DAC-001`, `PERIPH-OPAMP-001` *(L3)*, `PERIPH-COMP-001` *(L3)*, `PERIPH-VREFBUF-001` *(L3)*
- **Core Topics:** SAR ADC clock compliance ($\le 60\ \text{MHz}$ single / $\le 52\ \text{MHz}$ multi → `CKMODE=11` `HCLK/4=42.5 MHz`), `DEEPPWD` exit (`0x20000000`), calibration, RC source-impedance settling & temp sensor $t_S \ge 5\ \mu\text{s}$, oversampling, AHB2 `DAC1–4` (`MCR.HFSEL=10` >160 MHz, sawtooth wave), plus Level 3 `OPAMP1–6` PGA→ADC, `COMP1–7`→`TIM1/HRTIM` hardware break, and `VREFBUF`.
- **Laboratory:** `lab-09-adc-single-channel-potentiometer`.

---

### Stage 11: DMA1/2, DMAMUX1 & Hardware Math Accelerators (`CORDIC`, `FMAC`) *(Level 2–3)*
- **Folder:** `13-dma/` | **Concept IDs:** `PERIPH-DMA-001`, `PERIPH-CORDIC-001` *(L3)*, `PERIPH-FMAC-001` *(L3)*, `PROF-PERF-001..003`
- **Core Topics:** 16-channel `DMA1`/`DMA2`, `DMAMUX1` Request IDs (RM0440 Table 91), Synchronization & Request Generators, circular ping-pong (`HTIF`/`TCIF`), ES0430 §2.3.1 individual flag clearing (never `CGIFx`), CCM-SRAM DMA alias `0x20018000`, and Level 3 `CORDIC` (`q1.31` trig) + `FMAC` (170 MMAC/s FIR/IIR).
- **Laboratory:** `lab-10-adc-dma-multichannel-circular-buffer` + Stage 04 Mastery Exam (`24-assessments/stage-04-stm32-peripherals-exam.md`, $\ge 85\%$).

---

### Stage 12: Embedded Memory Subsystems, Dual-Bank Flash, Bootloaders & Security *(Level 2–3)*
- **Folder:** `14-memory-and-storage/` | **Concept IDs:** `STM32-FLASH-002`, `BOOT-FLASH-001`, `BOOT-JUMP-001`, `BOOT-VERIF-001` *(L3)*, `BOOT-UPD-001` *(L3)*, `SEC-RDP-001` *(L3)*, `SEC-MEM-001` *(L3)*, `SEC-BOOT-001` *(L3)*, `SEC-TAMP-001` *(L3)*, `SEC-CRYPTO-001` *(L3)*, `PERIPH-CRC-001`, `PERIPH-RNG-001`
- **Core Topics:** Dual-Bank (`DBANK=1`, 2 KB pages, RWW) vs. Single-Bank (`DBANK=0`, 4 KB pages), mandatory 64-bit aligned writes + 8-bit ECC (`FLASH_ECCR`), safe 9-step bootloader handoff (`SCB->VTOR` 512-byte aligned), hardware `CRC`/`RNG`, and Level 3 `RDP` (0/1, never L2 on dev boards!), `WRP`, `PCROP`, Securable Memory (`SEC1R`), and `TAMP` 32 backup registers.

---

### Stage 13: System Reliability, Watchdogs & STM32G4 Low-Power Modes *(Level 2)*
- **Folder:** `15-power-management/` | **Concept IDs:** `STM32-PWR-002..003`, `PERIPH-WDG-001..002`, `PERIPH-RTC-001`, `REL-BOR-001`, `REL-WDG-001`, `ARM-PWR-001`, `PROF-PWR-001..002`
- **Core Topics:** `IWDG` (LSI + `WINR` window mode + `DBGMCU` freeze), `WWDG` (`WDGTB` `/1../128`, `EWI` IRQ 0), `BOR_LEV` & `PVD`, `RCC->CSR` reset cause decoding, and exact **STM32G474RE Low-Power Modes: Sleep (11–12 cycle wakeup), Low-Power Run, Low-Power Sleep, Stop 0, Stop 1, Standby (with optional SRAM2 `RRS`), and Shutdown** *(Note: Stop 2 does NOT exist on STM32G4!)*. Current measurement on **`JP6`** (`IDD`), NOT `JP5`.
- **Laboratories:** `lab-13-iwdg-hardware-watchdog` & `lab-14-low-power-stop-mode-wakeup`.

---

### Stage 14: Real-Time Operating Systems (FreeRTOS `ARM_CM4F`) & Concurrency *(Level 2–3)*
- **Folder:** `16-rtos/` | **Concept IDs:** `RTOS-ARCH-001`, `RTOS-SCHED-001`, `RTOS-CTX-001`, `RTOS-TASK-001..002`, `RTOS-IPC-001..002`, `RTOS-SYNC-001..004`, `RTOS-ISR-001..002`, `RTOS-HAZ-001..002`, `RTOS-MEM-001..002`, `RTOS-PORT-001`, `RTOS-PWR-001` *(L3)*, `RTOS-THEORY-001` *(L3)*
- **Core Topics:** Preemptive vs. cooperative scheduling, `SVC`/`SysTick`/`PendSV` context switching with FPU `S16–S31` lazy stacking, `NVIC_SetPriorityGrouping(0)`, `BASEPRI` & `configMAX_SYSCALL_INTERRUPT_PRIORITY`, Queues, Direct-to-Task Notifications, Binary/Counting Semaphores, Mutexes & Priority Inheritance (distinct from deadlock!), Event Groups, `FromISR` + `portYIELD_FROM_ISR`, static allocation (`xTaskCreateStatic`), `heap_1..heap_5`, Stack High-Water Mark & Overflow Method 2, and Level 3 LPTIM1 Tickless Idle + RMS/RTA schedulability.
- **Laboratory:** `lab-15-freertos-multitask-producer-consumer`.

---

### Stage 15: Defensive Firmware Architecture, Standards, Testing & CI/CD *(Level 2–3)*
- **Folder:** `17-advanced-firmware-engineering/` | **Concept IDs:** `BOOT-ARCH-001`, `BOOT-CFG-001`, `REL-CRASH-001`, `PROF-STD-001..004`, `PROF-SA-001..004`, `PROF-DA-001..004`, `PROF-REV-001`, `TEST-UNIT-001..002`, `TEST-MOCK-001`, `TEST-HIL-001..002`, `PROF-CI-001`, `PROF-GIT-001..002`, `PROF-DOC-001..002`, `PROF-REL-001..002`
- **Core Topics:** Layered architecture & Active Objects, FSM/HSM, lock-free SPSC ring buffers, naked-asm HardFault crash logger to `.noinit` / `TAMP->BKPxR`, MISRA C:2012/2023 + SEI CERT C + Barr Group + Deviation Management, `cppcheck`/`clang-tidy`/`-fanalyzer`, Unity/Ceedling/CMock host unit tests with fake CMSIS registers + ASan/UBSan, Renode emulation, and GitHub Actions CI.
- **Verification:** Complete Stage 05 Mastery Exam (`24-assessments/stage-05-rtos-and-advanced-firmware-exam.md`, $\ge 85\%$) and Capstone Project (`19-projects/`).
