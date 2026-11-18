# Learner Progress Dashboard (NUCLEO-G474RE Bare-Metal Track)

Use this dashboard as your personal engineering logbook. Check off each box (`[ ]` → `[x]`) independently across all **7 separate mastery states** — never treat clean compilation as physical hardware verification!

> **Authoritative Metrics Source (`AUDIT/release_metrics.json`):**  
> `350` Source-of-Truth concepts | `27` domains (`0` DAG cycles) | `17` logical stages (`Step 01`–`Step 17`) | `20` labs (`13` onboard-tested, `6` external hardware pending, `1` host-only) | `12` STM32G474RE bare-metal drivers | `3` capstone projects (onboard-tested) | `70` live-verified videos (`100%` HTTP 200) | `594/594` host C11 assertions (`96.04%` `gcov -b` line coverage across 10 host-linked drivers) | `0` fabricated physical results.

> **Hardware Honesty Legend:**
> - **Host Only:** Runs natively on host PC via `gcc` (`make -C tests test`).
> - **Onboard Hardware Tested:** Proven on physical NUCLEO-G474RE (`MB1367 Rev C`, `COM8` `115200 8N1`, `VDD = 3.28 V`) with verbatim log evidence in [`AUDIT/HARDWARE_VALIDATION_SUMMARY.md`](../AUDIT/HARDWARE_VALIDATION_SUMMARY.md) and [`AUDIT/LAB_VERIFICATION_MATRIX.md`](../AUDIT/LAB_VERIFICATION_MATRIX.md).
> - **`PENDING — USER CONNECTION REQUIRED`:** Requires external breadboard wiring, external sensors, manual Stop 0 `B1` wakeup, or bench instruments (potentiometer, RGB LED, SPI sensor, I2C OLED, `JP6` µA meter, oscilloscope) per [`HARDWARE_VALIDATION/PENDING_EXTERNAL_CONNECTIONS.md`](../HARDWARE_VALIDATION/PENDING_EXTERNAL_CONNECTIONS.md).

---

## The 7 Independent Mastery States (Mandatory Columns)

Every curriculum stage, laboratory, and capstone row below tracks all **7 separate engineering checkpoints**:

| # | Mastery State | Column Header | Completion Criterion |
|---:|---|---|---|
| 1 | **Theory Understood** | `Theory Understood` | Studied stage `README.md`, hardware architecture, formulas, and RM0440/DS12288 register semantics. |
| 2 | **Code Written** | `Code Written` | Authored bare-metal C driver, lab firmware, or host unit-test code cleanly without copy-paste blind spots. |
| 3 | **Code Compiled** | `Code Compiled` | Built cleanly with **0 warnings / 0 errors** (`-Wall -Wextra -Werror`) under `arm-none-eabi-gcc 14.3.1` or host `gcc 15.2.0`. |
| 4 | **Host Tests Passed** | `Host Tests Passed` | Associated host unit tests, pure math helpers, or CMSIS register mock assertions pass (`make -C tests test`, `594/594`). |
| 5 | **Target Upload Completed** | `Target Upload Completed` | Programmed `.elf`/`.bin` to STM32G474RE Flash (`0x08000000`) via SWD (`STM32_Programmer_CLI` or OpenOCD) with verify pass. |
| 6 | **Physical Behavior Verified** | `Physical Behavior Verified` | Observed real silicon behavior on NUCLEO-G474RE (`COM8` VCP telemetry, GDB register inspection, `DWT->CYCCNT`, LED/pin state, or external scope/logic capture). |
| 7 | **Assessment Passed** | `Assessment Passed` | Answered stage self-assessment questions and passed the aligned Mastery Exam (`24-assessments/`) with $\ge 85\%$. |

---

## Quick Status Summary (Update As You Go)

- [ ] **Environment Setup Complete** ([`BEGINNER_SETUP_WINDOWS11.md`](BEGINNER_SETUP_WINDOWS11.md) — `python scripts/validate_repo.py --skip-builds`, `make -C tests test` **`594/594 PASS`**, `make -C 23-code-library/templates` 0 warnings)
- [ ] **Level 1 (Foundational) Mastery Gate Passed** (`Steps 01–07`: Stages `01b`, `02`, `03`, `04`, `05`, `06 [Part A]`, `07 [08-gpio]` + Labs `01–02` + Exams `01–03`)
- [ ] **Level 2 (Intermediate) Mastery Gate Passed** (`Steps 08–16`: Stages `06 [Part B] [07-rcc]`, `08 [10-interrupts]`, `09 [09-timers]`, `10–15` + Core Labs + Exam `04`)
- [ ] **Level 3 (Specialist) Mastery Gate Passed** (`Step 17`: `Stage 16 [17-advanced-firmware-engineering]`, `HRTIM1`, `FDCAN`, `OPAMP`/`COMP`, `CORDIC`/`FMAC`, Flash/Bootloader/Security, `MPU`, TDD/CI + Exam `05` + Capstones `01–03`)

---

## Canonical 17-Stage 7-State Tracker (`Step 01`–`Step 17`, Strictly Acyclic Order)

> **Stage-Numbering Alignment:** Each row matches [`ROADMAP.md`](../ROADMAP.md), [`01-master-concept-map/learning-sequence.md`](../01-master-concept-map/learning-sequence.md), and the **exact H1 heading** of each stage `README.md` (`08-gpio/` precedes `07-rcc-and-clocks/`, and `10-interrupts/` precedes `09-timers/`).

### Phase 0 & Level 1 — Onboarding & Essential Foundations (`Steps 00–07`)

| Step, Directory & Exact Stage README H1 Title | Theory Understood | Code Written | Code Compiled | Host Tests Passed | Target Upload Completed | Physical Behavior Verified | Assessment Passed | Checkpoint / Hardware Tier |
|---|:---:|:---:|:---:|:---:|:---:|:---:|:---:|---|
| **Step 00 — Onboarding (`BEGINNER_SETUP_WINDOWS11.md`)**<br>[Windows 11 Setup Guide](BEGINNER_SETUP_WINDOWS11.md) | [ ] | [ ] | [ ] | [ ] | [ ] | [ ] | [ ] | `validate_repo.py`, `594/594` host tests, SWD probe |
| **Step 01 — [`01b-digital-electronics-and-hw-integration/`](../01b-digital-electronics-and-hw-integration/README.md)**<br>`Stage 01b: Digital Electronics, NUCLEO-G474RE Hardware Integration & Lab Equipment Mastery` | [ ] | [ ] | [ ] | [ ] *(N/A)* | [ ] | [ ] | [ ] | Onboard `JP5`/`JP6`/`LD2`/`B1` + RC/I2C $R_p$ math |
| **Step 02 — [`02-c-fundamentals/`](../02-c-fundamentals/README.md)** *(+ [02B](../02-c-fundamentals/advanced-c-ub-memory-model.md))*<br>`Stage 02: C Language Foundations for Bare-Metal Embedded Systems` | [ ] | [ ] | [ ] | [ ] | [ ] *(Host)* | [ ] *(Host)* | [ ] | Host Only (`test_c_foundations`: `116/116`) + [Exam 01](../24-assessments/stage-01-c-mastery-exam.md) |
| **Step 03 — [`03-embedded-c/`](../03-embedded-c/README.md)**<br>`Stage 03: Embedded C, Concurrency Primitives & Production Firmware Architecture` | [ ] | [ ] | [ ] | [ ] | [ ] | [ ] | [ ] | Onboard (`PA5` `BSRR`) + [Worked Ex. 1](WORKED_EXAMPLES.md) + [Exam 02](../24-assessments/stage-02-embedded-c-register-exam.md) |
| **Step 04 — [`04-arm-cortex-m/`](../04-arm-cortex-m/README.md)**<br>`Stage 04: ARM Cortex-M4F (ARMv7E-M) Microarchitecture & Core Peripherals` | [ ] | [ ] | [ ] | [ ] | [ ] | [ ] | [ ] | Onboard (GDB `x/8wx $sp`, `fault_regs`) + [Worked Ex. 6](WORKED_EXAMPLES.md) |
| **Step 05 — [`05-toolchain-and-debugging/`](../05-toolchain-and-debugging/README.md)**<br>`Stage 05: GNU ARM Toolchain, Linker Scripts, Startup Code & Hardware Debugging` | [ ] | [ ] | [ ] | [ ] | [ ] | [ ] | [ ] | Onboard (`templates/`, SVD, `.map`/`.su`) + [Exam 03](../24-assessments/stage-03-arm-cortex-m-architecture-exam.md) |
| **Step 06 — [`06-stm32-fundamentals/`](../06-stm32-fundamentals/README.md)**<br>`Stage 06: STM32 Platform Architecture, STM32G474RET6 Silicon & ES0430 Errata` *(Part A)* | [ ] | [ ] | [ ] | [ ] *(N/A)* | [ ] | [ ] | [ ] | Onboard (`DBGMCU->IDCODE=0x469`, CCM alias `0x20018000`) |
| **Step 07 — [`08-gpio/`](../08-gpio/README.md)** *(Study BEFORE `07-rcc-and-clocks/`!)*<br>`Stage 07: General-Purpose I/O (GPIO) Hardware Engine` | [ ] | [ ] | [ ] | [ ] | [ ] | [ ] | [ ] | Onboard (`gpio_g4.c`, `test_gpio_mock` `82/82`, Labs 01–02) |

### Level 2 — Core Platform Bring-Up, Peripherals, DMA, Low-Power & RTOS (`Steps 08–16`)

| Step, Directory & Exact Stage README H1 Title | Theory Understood | Code Written | Code Compiled | Host Tests Passed | Target Upload Completed | Physical Behavior Verified | Assessment Passed | Checkpoint / Hardware Tier |
|---|:---:|:---:|:---:|:---:|:---:|:---:|:---:|---|
| **Step 08 — [`07-rcc-and-clocks/`](../07-rcc-and-clocks/README.md)**<br>`Stage 06: Power, Reset & Clock Control (RCC) Engine` *(Part B; legacy H1 prefix `Stage 06`)* | [ ] | [ ] | [ ] | [ ] *(N/A)* | [ ] | [ ] | [ ] | Onboard (`clock_g4.c` 160/170 MHz Boost, Lab 03) |
| **Step 09 — [`10-interrupts/`](../10-interrupts/README.md)** *(Study BEFORE `09-timers/`!)*<br>`Stage 08: Interrupts, Exceptions, NVIC, SysTick & EXTI` | [ ] | [ ] | [ ] | [ ] | [ ] | [ ] | [ ] | Onboard (`exti_g4.c`, AN321 + `__DSB()`, Labs 04–05) |
| **Step 10 — [`09-timers/`](../09-timers/README.md)**<br>`Stage 09: Hardware Timers (Basic, General-Purpose, Advanced, LPTIM1 & HRTIM1)` | [ ] | [ ] | [ ] | [ ] | [ ] | [ ] | [ ] | Onboard (`tim_g4.c`, Lab 06) / Ext. Pending (Lab 07 RGB) |
| **Step 11 — [`11-communication/`](../11-communication/README.md)**<br>`Stage 10: Serial & Industrial Communication Protocols (USART, SPI, I2C, FDCAN, USB, UCPD, SAI, QUADSPI & Framing/CRC)` | [ ] | [ ] | [ ] | [ ] | [ ] | [ ] | [ ] | Onboard (Labs 08, 18) / Ext. Pending (Labs 11, 12) |
| **Step 12 — [`12-adc-dac-analog/`](../12-adc-dac-analog/README.md)**<br>`Stage 11: Mixed-Signal Analog Subsystem (ADC1–5, DAC1–4, COMP1–7, OPAMP1–6, VREFBUF) & Hardware Math Accelerators (CORDIC & FMAC)` | [ ] | [ ] | [ ] | [ ] | [ ] | [ ] | [ ] | Onboard (`VREFINT`/Temp) / Ext. Pending (Labs 09, 16) |
| **Step 13 — [`13-dma/`](../13-dma/README.md)**<br>`Stage 12: Direct Memory Access (DMA1/DMA2) & DMAMUX1 Architecture` | [ ] | [ ] | [ ] | [ ] | [ ] | [ ] | [ ] | Onboard (`dma_g4.c`, Labs 10, 17) + [Exam 04](../24-assessments/stage-04-stm32-peripherals-exam.md) |
| **Step 14 — [`14-memory-and-storage/`](../14-memory-and-storage/README.md)**<br>`Stage 13: Embedded Memory Subsystems, Dual-Bank Flash, ECC & Bootloaders` | [ ] | [ ] | [ ] | [ ] *(N/A)* | [ ] | [ ] | [ ] | Onboard (`DBANK=1`, CRC-32; never set `RDP` L2 `0xCC`!) |
| **Step 15 — [`15-power-management/`](../15-power-management/README.md)**<br>`Stage 14: System Reliability, Watchdogs (IWDG/WWDG), RTC/TAMP & Low-Power Modes` | [ ] | [ ] | [ ] | [ ] | [ ] | [ ] | [ ] | Onboard (`iwdg_g4.c`, Lab 13) / Ext. Pending (Lab 14 `JP6`) |
| **Step 16 — [`16-rtos/`](../16-rtos/README.md)**<br>`Stage 15: Real-Time Operating Systems (FreeRTOS on ARM Cortex-M4F) & Concurrency` | [ ] | [ ] | [ ] | [ ] *(N/A)* | [ ] | [ ] | [ ] | Onboard (`23-code-library/freertos/` V11.1.0, Lab 15 `6/6 PASS`) |

### Level 3 — Production Firmware Engineering, Standards, TDD & CI (`Step 17`)

| Step, Directory & Exact Stage README H1 Title | Theory Understood | Code Written | Code Compiled | Host Tests Passed | Target Upload Completed | Physical Behavior Verified | Assessment Passed | Checkpoint / Hardware Tier |
|---|:---:|:---:|:---:|:---:|:---:|:---:|:---:|---|
| **Step 17 — [`17-advanced-firmware-engineering/`](../17-advanced-firmware-engineering/README.md)**<br>`Stage 16: Production Firmware Engineering, Verification, Optimization, Security & Fault Tolerance` | [ ] | [ ] | [ ] | [ ] | [ ] | [ ] | [ ] | Onboard (Lab 19) + Host TDD (Lab 20 `594/594`) + [Exam 05](../24-assessments/stage-05-rtos-and-advanced-firmware-exam.md) |

---

## All 20 Practical Laboratories & 3 Capstones — 7-State Tracker

Every laboratory and capstone project tracks all **7 separate mastery states** aligned with [`AUDIT/LAB_VERIFICATION_MATRIX.md`](../AUDIT/LAB_VERIFICATION_MATRIX.md) and [`AUDIT/HARDWARE_VALIDATION_SUMMARY.md`](../AUDIT/HARDWARE_VALIDATION_SUMMARY.md):

| Lab / Project | Aligned Step | Theory Understood | Code Written | Code Compiled | Host Tests Passed | Target Upload Completed | Physical Behavior Verified | Assessment Passed | Repo Verification Status |
|---|---|:---:|:---:|:---:|:---:|:---:|:---:|:---:|---|
| [**Lab 01:** Bare-Metal GPIO LED Blinky (3-Way)](../18-practical-laboratory/lab-01-baremetal-gpio-led-blinky/README.md) | Step 07 (`08-gpio/`) | [ ] | [ ] | [ ] | [ ] | [ ] | [ ] | [ ] | `ONBOARD HARDWARE TESTED` |
| [**Lab 02:** GPIO Pushbutton 4-State Debouncer](../18-practical-laboratory/lab-02-gpio-pushbutton-debouncing/README.md) | Step 07 (`08-gpio/`) | [ ] | [ ] | [ ] | [ ] | [ ] | [ ] | [ ] | `ONBOARD HARDWARE TESTED` |
| [**Lab 03:** RCC PLL 170/160 MHz Boost Switch](../18-practical-laboratory/lab-03-rcc-pll-170mhz-clock-switch/README.md) | Step 08 (`07-rcc-and-clocks/`) | [ ] | [ ] | [ ] | [ ] *(N/A)* | [ ] | [ ] | [ ] | `ONBOARD HARDWARE TESTED` |
| [**Lab 04:** SysTick 1 ms Delay & Scheduler](../18-practical-laboratory/lab-04-systick-accurate-delay-generator/README.md) | Step 09 (`10-interrupts/`) | [ ] | [ ] | [ ] | [ ] *(N/A)* | [ ] | [ ] | [ ] | `ONBOARD HARDWARE TESTED` |
| [**Lab 05:** EXTI13 Interrupt & 3-Way API](../18-practical-laboratory/lab-05-exti-pushbutton-interrupt/README.md) | Step 09 (`10-interrupts/`) | [ ] | [ ] | [ ] | [ ] | [ ] | [ ] | [ ] | `ONBOARD HARDWARE TESTED` |
| [**Lab 06:** TIM2 10 Hz Periodic Interrupt](../18-practical-laboratory/lab-06-general-purpose-timer-periodic-interrupt/README.md) | Step 10 (`09-timers/`) | [ ] | [ ] | [ ] | [ ] | [ ] | [ ] | [ ] | `ONBOARD HARDWARE TESTED` |
| [**Lab 07:** TIM3 25 kHz RGB Hardware PWM](../18-practical-laboratory/lab-07-timer-pwm-rgb-led-fading/README.md) | Step 10 (`09-timers/`) | [ ] | [ ] | [ ] | [ ] | [ ] *(Ext)* | [ ] *(Ext)* | [ ] | `PENDING — USER CONNECTION REQUIRED` |
| [**Lab 08:** USART2 SPSC Ring Buffer & `printf`](../18-practical-laboratory/lab-08-usart-baremetal-driver-printf/README.md) | Step 11 (`11-communication/`) | [ ] | [ ] | [ ] | [ ] | [ ] | [ ] | [ ] | `ONBOARD HARDWARE TESTED` |
| [**Lab 09:** ADC1 Single-Channel Potentiometer](../18-practical-laboratory/lab-09-adc-single-channel-potentiometer/README.md) | Step 12 (`12-adc-dac-analog/`) | [ ] | [ ] | [ ] | [ ] | [ ] *(Ext)* | [ ] *(Ext)* | [ ] | `PENDING — USER CONNECTION REQUIRED` |
| [**Lab 10:** ADC1 Internal `VREFINT`/Temp + DMA1](../18-practical-laboratory/lab-10-adc-dma-multichannel-circular-buffer/README.md) | Step 13 (`13-dma/`) | [ ] | [ ] | [ ] | [ ] | [ ] | [ ] | [ ] | `ONBOARD HARDWARE TESTED` |
| [**Lab 11:** SPI1 Master Sensor Interfacing](../18-practical-laboratory/lab-11-spi-master-sensor-interfacing/README.md) | Step 11 (`11-communication/`) | [ ] | [ ] | [ ] | [ ] | [ ] *(Ext)* | [ ] *(Ext)* | [ ] | `PENDING — USER CONNECTION REQUIRED` |
| [**Lab 12:** I2C1 400 kHz & SSD1306 OLED](../18-practical-laboratory/lab-12-i2c-master-ssd1306-oled/README.md) | Step 11 (`11-communication/`) | [ ] | [ ] | [ ] | [ ] | [ ] *(Ext)* | [ ] *(Ext)* | [ ] | `PENDING — USER CONNECTION REQUIRED` |
| [**Lab 13:** Independent Watchdog (`IWDG`)](../18-practical-laboratory/lab-13-iwdg-hardware-watchdog/README.md) | Step 15 (`15-power-management/`) | [ ] | [ ] | [ ] | [ ] | [ ] | [ ] | [ ] | `ONBOARD HARDWARE TESTED` |
| [**Lab 14:** Stop 0 Mode & EXTI13 Wakeup (`JP6`)](../18-practical-laboratory/lab-14-low-power-stop-mode-wakeup/README.md) | Step 15 (`15-power-management/`) | [ ] | [ ] | [ ] | [ ] *(N/A)* | [ ] *(Ext)* | [ ] *(Ext)* | [ ] | `PENDING — USER CONNECTION REQUIRED` |
| [**Lab 15:** FreeRTOS `V11.1.0` Producer-Consumer](../18-practical-laboratory/lab-15-freertos-multitask-producer-consumer/README.md) | Step 16 (`16-rtos/`) | [ ] | [ ] | [ ] | [ ] *(N/A)* | [ ] | [ ] | [ ] | `ONBOARD HARDWARE TESTED` (`6/6 PASS`) |
| [**Lab 16:** DAC1 + OPAMP1 + COMP1 → `TIM1_BKIN`](../18-practical-laboratory/lab-16-dac-opamp-comp-hardware-break/README.md) | Step 12 (`12-adc-dac-analog/`) | [ ] | [ ] | [ ] | [ ] *(N/A)* | [ ] *(Ext)* | [ ] *(Ext)* | [ ] | `PENDING — USER CONNECTION REQUIRED` |
| [**Lab 17:** CORDIC & FMAC Hardware Math vs FPU](../18-practical-laboratory/lab-17-cordic-fmac-hardware-math/README.md) | Step 13 (`13-dma/`) | [ ] | [ ] | [ ] | [ ] *(N/A)* | [ ] | [ ] | [ ] | `ONBOARD HARDWARE TESTED` |
| [**Lab 18:** FDCAN1 Internal Loopback & CRC-32](../18-practical-laboratory/lab-18-fdcan-loopback-and-crc/README.md) | Step 11 (`11-communication/`) | [ ] | [ ] | [ ] | [ ] *(N/A)* | [ ] | [ ] | [ ] | `ONBOARD HARDWARE TESTED` |
| [**Lab 19:** MPU Stack Guard & `.noinit` HardFault](../18-practical-laboratory/lab-19-hardfault-mpu-and-crash-diagnostics/README.md) | Step 17 (`17-advanced-firmware-engineering/`) | [ ] | [ ] | [ ] | [ ] | [ ] | [ ] | [ ] | `ONBOARD HARDWARE TESTED` |
| [**Lab 20:** Host TDD, Fake CMSIS Mocks & `.su`](../18-practical-laboratory/lab-20-host-unit-testing-and-driver-mocking/README.md) | Step 17 (`17-advanced-firmware-engineering/`) | [ ] | [ ] | [ ] | [ ] | [ ] *(Host)* | [ ] *(Host)* | [ ] | `HOST TESTED` (`594/594`, `96.04%` `gcov`) |
| [**Capstone 01:** Smart Instrument Panel](../19-projects/01-beginner-smart-instrument-panel/README.md) | Steps 01–12 | [ ] | [ ] | [ ] | [ ] | [ ] | [ ] | [ ] | `ONBOARD HARDWARE TESTED` (Ext pot pending) |
| [**Capstone 02:** High-Speed DMA Data Logger](../19-projects/02-intermediate-dma-data-logger/README.md) | Steps 01–14 | [ ] | [ ] | [ ] | [ ] | [ ] | [ ] | [ ] | `ONBOARD HARDWARE TESTED` (Ext OLED pending) |
| [**Capstone 03:** FreeRTOS Digital Power Controller](../19-projects/03-advanced-freertos-digital-power-controller/README.md) | Steps 01–17 | [ ] | [ ] | [ ] | [ ] | [ ] | [ ] | [ ] | `ONBOARD HARDWARE TESTED` (Ext power stage pending) |

---

> **Navigation:** [`START_HERE.md`](../START_HERE.md) | [`ROADMAP.md`](../ROADMAP.md) | [`learning-sequence.md`](../01-master-concept-map/learning-sequence.md) | [`REVISION_NOTES_AND_MASTERY_GATES.md`](REVISION_NOTES_AND_MASTERY_GATES.md)
