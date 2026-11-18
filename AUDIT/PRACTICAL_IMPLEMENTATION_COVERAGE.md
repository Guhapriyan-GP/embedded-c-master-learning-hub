# Practical Implementation & Verification Coverage Report

> **Strict Hardware Verification Honesty (`v2.2.0-remediation`, branch `feature/complete-remediation`, base commit `585dc4fa`):**
> - **`CROSS-COMPILED` (`arm-none-eabi-gcc 14.3.1`, `-Wall -Wextra -Werror -O2`, 0 warnings / 0 errors):**
>   - Starter template (`BUILD_TYPE=debug` and `BUILD_TYPE=release`) and all **12 reusable bare-metal drivers** (`clock_g4`, `gpio_g4`, `exti_g4`, `tim_g4`, `usart_g4`, `spi_g4`, `i2c_g4`, `adc_g4`, `dma_g4`, `iwdg_g4`, `ring_buffer`, `fault_handler` — see [`AUDIT/DRIVER_LIBRARY_VERIFICATION.md`](DRIVER_LIBRARY_VERIFICATION.md)).
>   - All **18 bare-metal practical laboratories (`Labs 01–14, 16–19`)** against official STM32G474RE CMSIS headers (`stm32g474xx.h`), each declaring explicit **`170 MHz` vs `160 MHz` Range 1 Boost** (or `16 MHz HSI16`) clock profiles and an individual Standardized Verification Record.
>   - **`Lab 15` (FreeRTOS)** via the standalone pinned official **FreeRTOS-Kernel `V11.1.0`** (`ARM_CM4F`) project in [`23-code-library/freertos/`](../23-code-library/freertos/README.md) (`build/nucleo_g474re_freertos.elf`, 0 errors / 0 warnings, `SVC`/`PendSV`/`SysTick` vector bindings verified via `arm-none-eabi-nm` & `objdump` — see [`AUDIT/FREERTOS_VERIFICATION_REPORT.md`](FREERTOS_VERIFICATION_REPORT.md)).
> - **`HOST TESTED` (`gcc 15.2.0`):** **`Lab 20`** (`test_embedded_drivers.c`, exit `0`), `tests/test_c_foundations.c` (**35/35 assertions passed**), `tests/unit/test_ring_buffer.c` (**78/78 assertions passed**), and `-DDRIVER_HOST_TEST` pure driver helpers across all 8 new peripheral drivers.
> - **Physical Hardware Validation Summary (Agent 4 on NUCLEO-G474RE `MB1367` Rev C, `STM32G474RET6`, STLINK-V3E `V3J17M11`, `COM8`, `VDD = 3.28 V` — see [`AUDIT/HARDWARE_VALIDATION_SUMMARY.md`](HARDWARE_VALIDATION_SUMMARY.md)):**
>   - **13 / 19 STM32 Target Labs `ONBOARD HARDWARE TESTED`:** **`Lab 15`** (FreeRTOS `V11.1.0` standalone ELF, `6/6 ALL_PASSED` + 8 s `[TELEM]`) + **`Labs 01, 02, 03, 04, 05, 06, 08, 10, 13, 17, 18, 19`** (all core silicon peripherals, drivers, dual `160`/`170 MHz` Boost clocks, `CORDIC`/`FMAC`, `FDCAN1`/`CRC-32`, `MPU`/`.noinit` `DIV0` reboot, and `IWDG` physically verified via `HARDWARE_VALIDATION/onboard_selftest/build/onboard_selftest.elf` `9/9 PASS` + Capstones 01–03).
>   - **1 / 20 Lab `HOST TESTED`:** **`Lab 20`** (`gcc 15.2.0`, zero MCU hardware required).
>   - **6 / 19 STM32 Target Labs `EXTERNAL HARDWARE PENDING`:** **`Labs 07, 09, 11, 12, 16`** (external breadboard components/oscilloscope) + **`Lab 14`** (Stop 0 manual `B1` wake & `JP6` µA ammeter) — consolidated bench schedule in [`HARDWARE_VALIDATION/PENDING_EXTERNAL_CONNECTIONS.md`](../HARDWARE_VALIDATION/PENDING_EXTERNAL_CONNECTIONS.md).
>   - **3 / 3 Capstones (`Projects 01–03`):** **`ONBOARD HARDWARE TESTED`** in safe zero-wire modes (`COM8`, after Agent 4 caught & fixed missing `GPIO_EnableClock()` calls); optional external peripherals remain **`EXTERNAL HARDWARE PENDING`**.

---

## 1. Template Firmware, FreeRTOS & Reusable Driver Verification Matrix (`23-code-library/`)

| Artifact | Path | Verification Taxonomy Status | Measured Build / Test Result |
|---|---|---|---|
| **Starter Firmware Template (Debug & Release)** | [`23-code-library/templates/Makefile`](../23-code-library/templates/Makefile) | **`ONBOARD HARDWARE TESTED`** + `CROSS-COMPILED` | Debug: `FLASH = 2,752 B` (`0.52%`), `RAM = 1,704 B` (`1.73%`); Release: `FLASH = 2,616 B` (`0.50%`), `.isr_vector` = **472 B @ `0x08000000`**; flashed & verified clean on NUCLEO-G474RE (`3.28 V`) |
| **CMake + Ninja Cross-Build** | [`23-code-library/templates/CMakeLists.txt`](../23-code-library/templates/CMakeLists.txt) | `STATICALLY REVIEWED` | Lists all 12 driver `.c` sources; exports `compile_commands.json` |
| **12 Reusable Bare-Metal Drivers** (`clock_g4`, `gpio_g4`, `ring_buffer`, `fault_handler`, `exti_g4`, `tim_g4`, `usart_g4`, `spi_g4`, `i2c_g4`, `adc_g4`, `dma_g4`, `iwdg_g4`) | [`23-code-library/Drivers/`](../23-code-library/Drivers/) | **`ONBOARD HARDWARE TESTED` (10/12 + `i2c_g4` NACKF)** + `CROSS-COMPILED` + `HOST TESTED` | All 12 compile with **0 warnings / 0 errors**; 10/12 drivers verified live on NUCLEO-G474RE (`onboard_selftest` + Capstones); `spi_g4` & `i2c_g4` external slave remain `EXTERNAL HARDWARE PENDING` (see [`DRIVER_LIBRARY_VERIFICATION.md`](DRIVER_LIBRARY_VERIFICATION.md)) |
| **Pinned FreeRTOS `V11.1.0` Reference Project** | [`23-code-library/freertos/`](../23-code-library/freertos/README.md) | **`ONBOARD HARDWARE TESTED`** (`6/6 ALL_PASSED`) | `build/nucleo_g474re_freertos.elf` (`FLASH = 21,480 B`, `RAM = 35,728 B`), 0 warnings; physically run 8 s on `COM8`: 6/6 self-tests PASS + live `[TELEM]` + IWDG (see [`FREERTOS_VERIFICATION_REPORT.md`](FREERTOS_VERIFICATION_REPORT.md)) |
| **Onboard Hardware Self-Test Suite (9 Silicon Tests)** | [`HARDWARE_VALIDATION/onboard_selftest/`](../HARDWARE_VALIDATION/onboard_selftest/Src/main.c) | **`ONBOARD HARDWARE TESTED`** (`9/9 ALL_PASSED`) | `FLASH = 23,344 B`, `RAM = 10,752 B`, 0 warnings; verified `160`/`170 MHz` Boost, `TIM2`, `EXTI13`, `DMA1`, `ADC1`, `CORDIC`, `FMAC`, `FDCAN1`+`CRC-32`, `MPU`+`.noinit` `DIV0` reboot, `IWDG` (see [`HARDWARE_VALIDATION_SUMMARY.md`](HARDWARE_VALIDATION_SUMMARY.md)) |
| **Stage 01 C Foundations & Ring Buffer Host Suites** | [`tests/test_c_foundations.c`](../tests/test_c_foundations.c), [`tests/unit/test_ring_buffer.c`](../tests/unit/test_ring_buffer.c) | `HOST TESTED` (`gcc 15.2.0`, `-Werror -Wpedantic`) | **113/113 assertions passed** (35/35 C foundations + 78/78 SPSC ring buffer) |

---

## 2. Master 20-Lab Verification Coverage Matrix (`18-practical-laboratory/Labs 01–20`)

### 2.1 Self-Contained Onboard Labs (13 `ONBOARD HARDWARE TESTED` + Lab 14 `EXTERNAL HARDWARE PENDING` for `JP6` Meter / Manual `B1` Wake)

| Lab | Title & Link | Primary Clock (`160 MHz` Alt Documented) | Primary Concept IDs | Build / Verification Status | Physical Silicon Evidence (Agent 4) |
|---|---|---|---|---|---|
| **Lab 01** | [Bare-Metal GPIO LED Blinky (3-Way)](../18-practical-laboratory/lab-01-baremetal-gpio-led-blinky/README.md) | `16 MHz HSI16` (`160`/`170 MHz` scaling table) | `PERIPH-GPIO-001`, `EMB-BIT-001`, `STM32-BUS-001` | **`ONBOARD HARDWARE TESTED`** *(via `onboard_selftest` + template)* | `PA5` atomic `BSRR` set/rst/tog verified (`GPIOA->MODER=0xABFFF7AF`) |
| **Lab 02** | [Non-Blocking 4-State SysTick FSM Debouncer](../18-practical-laboratory/lab-02-gpio-pushbutton-debouncing/README.md) | `16 MHz HSI16` (`LOAD=15999`; `159999`/`169999` alt) | `PERIPH-GPIO-001`, `FW-PAT-003`, `FW-EXEC-002` | **`ONBOARD HARDWARE TESTED`** *(via `onboard_selftest` + Project 01)* | `PC13` `MODER=00b` & idle Active-HIGH `0` + 1 ms SysTick FSM live *(manual `B1` touch optional)* |
| **Lab 03** | [RCC PLL 170 MHz / 160 MHz Boost Switch & MCO](../18-practical-laboratory/lab-03-rcc-pll-170mhz-clock-switch/README.md) | `170 MHz Boost` (`N=85`; `N=80` @ `160 MHz`) | `STM32-RCC-001`, `STM32-PWR-001`, `STM32-FLASH-001` | **`ONBOARD HARDWARE TESTED`** *(via `onboard_selftest` Test 3A/3B)* | BOTH `160 MHz` (`1,599,997` cyc/10 ms) & `170 MHz` (`1,699,989` cyc/10 ms), `PWR_CR5=0`, `4 WS` *(MCO scope optional)* |
| **Lab 04** | [SysTick Accurate 1 ms Delay & Scheduler](../18-practical-laboratory/lab-04-systick-accurate-delay-generator/README.md) | `170 MHz Boost` (`LOAD=169999`; `159999` @ `160 MHz`) | `PERIPH-SYSTICK-001`, `EMB-VOL-001`, `FW-EXEC-002` | **`ONBOARD HARDWARE TESTED`** *(via `onboard_selftest` + Project 01)* | `SysTick` vs `DWT->CYCCNT` verified at `160`/`170 MHz` (`<0.001%` delta) |
| **Lab 05** | [EXTI13 Rising-Edge Interrupt & NVIC (3-Way)](../18-practical-laboratory/lab-05-exti-pushbutton-interrupt/README.md) | `16 MHz HSI16` (clock-independent @ `160`/`170 MHz`) | `PERIPH-EXTI-001`, `PERIPH-SYSCFG-001`, `ARM-NVIC-001` | **`ONBOARD HARDWARE TESTED`** *(via `onboard_selftest` Test 4)* | `SYSCFG->EXTICR[3]=0x20`, `EXTI->SWIER1` invoked callback 3/3 times *(manual `B1` touch optional)* |
| **Lab 06** | [TIM2 10 Hz Periodic Interrupt (`rc_w0` & `EGR=UG`)](../18-practical-laboratory/lab-06-general-purpose-timer-periodic-interrupt/README.md) | `170 MHz Boost` (`PSC=16999`; `15999` @ `160 MHz`) | `PERIPH-TIM-002`, `EMB-BIT-001`, `EMB-BAR-001` | **`ONBOARD HARDWARE TESTED`** *(via `onboard_selftest` + FreeRTOS)* | `TIM2` `100 Hz` (`11` IRQs/100 ms) & `10 Hz` (`80` IRQs/8 s in FreeRTOS), `rc_w0` clear |
| **Lab 08** | [USART2 SPSC Ring Buffer & `printf` (3-Way)](../18-practical-laboratory/lab-08-usart-baremetal-driver-printf/README.md) | `170 MHz Boost` (`BRR=1476`; `1389` @ `160 MHz`) | `PERIPH-UART-001`, `FW-PAT-001`, `TOOL-NEWLIB-001` | **`ONBOARD HARDWARE TESTED`** *(via `onboard_selftest` + Project 01)* | Live `COM8` TX (`160`/`170 MHz`) + RX IRQ SPSC `ring_buffer_t` (`'s'` -> `rx_cmds=1`) |
| **Lab 10** | [ADC1 Internal `VREFINT`/Temp + Circular Ping-Pong DMA1](../18-practical-laboratory/lab-10-adc-dma-multichannel-circular-buffer/README.md) | `170 MHz Boost` (`CKMODE=11b`: `42.5` / `40.0 MHz`) | `PERIPH-DMA-001..002`, `PERIPH-ADC-001`, `STM32-ERRATA-001` | **`ONBOARD HARDWARE TESTED`** *(via `onboard_selftest` + Project 02)* | `VDDA=3288 mV`, `Die Temp=32.91 °C`, `DMA1_Ch1` (`DMAMUX0=5`) `HT=20/TC=20/TE=0` & 25 ping-pong frames (`ovr=0`) |
| **Lab 13** | [Independent Watchdog (`IWDG`) & `DBGMCU` Freeze](../18-practical-laboratory/lab-13-iwdg-hardware-watchdog/README.md) | `16 MHz HSI16` + `32 kHz LSI` (`PR=4, RLR=499`) | `PERIPH-WDG-001`, `STM32-DBGMCU-001`, `STM32-RESET-001` | **`ONBOARD HARDWARE TESTED`** *(via `onboard_selftest` + FreeRTOS)* | `IWDG` (`PR=2, RLR=3999`, `DBG_IWDG_STOP=1`) + `6/6` kicks & 8 s FreeRTOS supervisor |
| **Lab 14** | [Stop 0 Low-Power Mode, EXTI13 Wakeup & `JP6` IDD](../18-practical-laboratory/lab-14-low-power-stop-mode-wakeup/README.md) | `170 MHz Boost` (post-stop `160 MHz` restore alt) | `STM32-PWR-001`, `PERIPH-EXTI-001`, `DIG-ELEC-005` | `CROSS-COMPILED` / **`EXTERNAL HARDWARE PENDING`** | Deferred: requires manual physical `B1` press to wake from Stop 0 + `JP6` µA ammeter (see `PENDING_EXTERNAL_CONNECTIONS.md`) |
| **Lab 15** | [FreeRTOS `V11.1.0` Producer-Consumer & Static Tasks](../18-practical-laboratory/lab-15-freertos-multitask-producer-consumer/README.md) | `170 MHz Boost` (`SystemCoreClock` dynamic @ 160/170) | `RTOS-SCHED-001`, `RTOS-SYNC-001`, `RTOS-ISR-001`, `RTOS-MEM-001` | **`ONBOARD HARDWARE TESTED`** (`nucleo_g474re_freertos.elf`) | **6/6 PASS** + 8 s live `COM8` `[TELEM]` stream (`heap=21104/21104`, `fpu_err=0`, `wdg=2..9`) |
| **Lab 17** | [CORDIC & FMAC Hardware Math vs FPU `DWT->CYCCNT`](../18-practical-laboratory/lab-17-cordic-fmac-hardware-math/README.md) | `170 MHz Boost` (identical regs @ `160 MHz`) | `PERIPH-CORDIC-001`, `PERIPH-FMAC-001`, `ARM-DWT-001` | **`ONBOARD HARDWARE TESTED`** *(via `onboard_selftest` Test 6A/6B)* | `CORDIC`: `9.72×` faster (`8,511` vs `82,716` cyc, `2e-6` err); `FMAC`: `0 LSB` diff (`2,055` vs `4,766` cyc) |
| **Lab 18** | [FDCAN1 Loopback (500k/2M BRS) & Hardware CRC-32](../18-practical-laboratory/lab-18-fdcan-loopback-and-crc/README.md) | `170 MHz Boost` (exact `160 MHz` `NBTP`/`DBTP` alt) | `PERIPH-FDCAN-001`, `PERIPH-CRC-001`, `EMB-PROTO-001` | **`ONBOARD HARDWARE TESTED`** *(via `onboard_selftest` Test 7)* | `CRC-32=0xCBF43926`, `FDCAN1` 16 B BRS loopback (`ID=0x123, FDF=1, BRS=1, ECR=0`) |
| **Lab 19** | [8-Region MPU Stack Guard & Naked-ASM HardFault](../18-practical-laboratory/lab-19-hardfault-mpu-and-crash-diagnostics/README.md) | `16 MHz HSI16` (clock-independent @ `160`/`170 MHz`) | `ARM-MPU-001`, `ARM-FAULT-001`, `REL-CRASH-001`, `TOOL-LD-001` | **`ONBOARD HARDWARE TESTED`** *(via `onboard_selftest` Test 8A/8B)* | `MPU_CTRL=0x5` + controlled `DIV0` UsageFault persisted `.noinit` (`CFSR=0x02000000`, `PC=0x08001A60`) across `NVIC_SystemReset()` |

### 2.2 Host-Tested Lab (1 Lab — Zero MCU Hardware Required)

| Lab | Title & Link | Execution Profile | Primary Concept IDs | Build / Verification Status | Hardware Status |
|---|---|---|---|---|---|
| **Lab 20** | [Host TDD, Fake CMSIS Structs & `-fstack-usage`](../18-practical-laboratory/lab-20-host-unit-testing-and-driver-mocking/README.md) | Host PC (`gcc 15.2.0`) + ARM `.su` | `PROF-TEST-001`, `PROF-MOCK-001`, `PROF-QUAL-001`, `FW-PAT-001` | **`HOST TESTED`** (`gcc 15.2.0`, 0 warn, exit `0`) | `HOST PC ONLY — NO MCU HARDWARE REQUIRED` |

### 2.3 External-Hardware-Pending Labs (5 Labs — Cross-Compiled, Wiring & Test Procedures Ready)

| Lab | Title & Link | Primary Clock (`160 MHz` Alt) | Primary Concept IDs | Build / Verification Status | External Hardware Pending |
|---|---|---|---|---|---|
| **Lab 07** | [TIM3 25 kHz 3-Channel RGB Hardware PWM](../18-practical-laboratory/lab-07-timer-pwm-rgb-led-fading/README.md) | `170 MHz` (`ARR=6799`) / `160 MHz` (`ARR=6399`) | `PERIPH-TIM-002`, `C-TYPE-002`, `PERIPH-GPIO-001` | `CROSS-COMPILED` / `EXTERNAL HARDWARE PENDING` | External RGB LED + 3 × `330 Ω` resistors on `PA6`/`PA7`/`PB0` |
| **Lab 09** | [ADC1 42.5 MHz Calibration & Single-Channel Pot](../18-practical-laboratory/lab-09-adc-single-channel-potentiometer/README.md) | `170 MHz` (`42.5 MHz`) / `160 MHz` (`40.0 MHz`) | `PERIPH-ADC-001`, `FW-REL-001`, `DIG-ELEC-003` | `CROSS-COMPILED` / `EXTERNAL HARDWARE PENDING` | External `10 kΩ` potentiometer on `PA0` (`ADC1_IN1`) |
| **Lab 11** | [SPI1 10.625 MHz Full-Duplex Master (`FRXTH=1`)](../18-practical-laboratory/lab-11-spi-master-sensor-interfacing/README.md) | `170 MHz` (`10.625 MHz`) / `160 MHz` (`10.000 MHz`) | `PERIPH-SPI-001`, `DIG-ELEC-006`, `FW-REL-001` | `CROSS-COMPILED` / `EXTERNAL HARDWARE PENDING` | External 3.3 V SPI Mode 3 sensor (or MISO-MOSI jumper) on `SPI1` |
| **Lab 12** | [I2C1 400 kHz Fast Mode, `RELOAD` & Bus Recovery](../18-practical-laboratory/lab-12-i2c-master-ssd1306-oled/README.md) | `170 MHz` (`0x70811021`) / `160 MHz` (`0x70710F1F`) | `PERIPH-I2C-001`, `DIG-ELEC-004`, `FW-REL-001` | `CROSS-COMPILED` / `EXTERNAL HARDWARE PENDING` | External SSD1306 OLED + `2.2–4.7 kΩ` pull-ups on `PA15`/`PB7` |
| **Lab 16** | [DAC1 + OPAMP1 PGA + COMP1 → `TIM1_BKIN` Break](../18-practical-laboratory/lab-16-dac-opamp-comp-hardware-break/README.md) | `170 MHz` (`HFSEL=10b, ARR=3399`) / `160 MHz` (`HFSEL=01b, ARR=3199`) | `PERIPH-DAC-001`, `PERIPH-OPAMP-001`, `PERIPH-COMP-001`, `PERIPH-TIM-003` | `CROSS-COMPILED` / `EXTERNAL HARDWARE PENDING` | External `PA1` DC/pot stimulus + scope on `PA8`/`PA4` *(internal `DAC1 -> OPAMP1 -> COMP1` path also observable via GDB)* |

---

## 3. Capstone Projects Coverage (`19-projects/`)

| Project | Path | Scope & Remediated Defects | Implementation Status |
|---|---|---|---|
| **Project 01** | [`01-beginner-smart-instrument-panel/README.md`](../19-projects/01-beginner-smart-instrument-panel/README.md) | Resolved `PA0` pin conflict (`PA0=ADC1_IN1`, `PA1=TIM2_CH2`), Active-HIGH `PC13` FSM, `GPIO_EnableClock()` fix | **`ONBOARD HARDWARE TESTED`** (live `COM8` JSON + `'s'` RX cmd); Ext pot/PWM load `EXTERNAL HARDWARE PENDING` |
| **Project 02** | [`02-intermediate-dma-data-logger/README.md`](../19-projects/02-intermediate-dma-data-logger/README.md) | `TS_CAL1`/`TS_CAL2` + `VREFINT_CAL`, `TIM3` TRGO + circular `DMA1`, hardware `CRC-32`, `GPIO_EnableClock()` fix | **`ONBOARD HARDWARE TESTED`** (`CRC=0xCBF43926`, 25 exact `160 ms` frames, `ovr=0`); Ext OLED `EXTERNAL HARDWARE PENDING` |
| **Project 03** | [`03-advanced-freertos-digital-power-controller/README.md`](../19-projects/03-advanced-freertos-digital-power-controller/README.md) | FreeRTOS `V11.1.0` 4 static tasks (Prios 1–4), Q15 PI loop, `COMP1 -> TIM1 BKIN`, `GPIO_EnableClock()` fix | **`ONBOARD HARDWARE TESTED`** (`vout_mv=4989..5021`, `moe:1`, `wdg:1..19`); Ext power stage/OLED `EXTERNAL HARDWARE PENDING` |

---

## 4. Explicit Deferred Practical Labs (Honest Backlog)

To maintain strict transparency, the following topics have full Part 4 theory, register maps, and authoritative citations, but do **not** yet have a dedicated standalone compiled lab binary:
1. `G4-HRTIM-001` — Standalone `HRTIM1` 200 kHz DLL-calibrated PWM lab + oscilloscope 184 ps jitter verification.
2. `PERIPH-USB-001` & `CLK-CRS-001` — Full bare-metal USB FS Device HID/CDC enumeration stack with crystal-less HSI48 + CRS.
3. `G4-UCPD-001` — USB Type-C Power Delivery BMC/4b5b protocol stack lab.
4. `PERIPH-SAI-001` & `PERIPH-QSPI-001` — External I2S audio codec (SAI1) and external QSPI NOR Flash XIP (`0x90000000`) labs.
5. `PERIPH-LPTIM-001` & `RTOS-PWR-001` — FreeRTOS `LPTIM1` Stop 1 Tickless Idle lab.
6. `BOOT-VERIF-001` & `BOOT-UPD-001` & `SEC-*` — Dual-image flashed Bank 1/Bank 2 A/B OTA bootloader with `BFB2` swap & Ed25519 signature check.
7. `TEST-HIL-001` & `TEST-HIL-002` — Automated `pytest`+`pyserial`+OpenOCD HIL harness on physical hardware and headless Renode/QEMU CI runner.
