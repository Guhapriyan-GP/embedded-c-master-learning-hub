# Practical Laboratory Roadmap for STM32G474RE (Labs 01–20)

> **Verification Transparency (`v2.2.0-remediation`, branch `feature/complete-remediation`):**
> - **`CROSS-COMPILED` (`arm-none-eabi-gcc 14.3.1`, `-Wall -Wextra -Werror -O2`, 0 warnings / 0 errors):** All **18 bare-metal STM32 labs (`Labs 01–14, 16–19`)** against official STM32G474RE CMSIS headers (`stm32g474xx.h`) **AND** **`Lab 15`** via the pinned official **FreeRTOS-Kernel `V11.1.0`** project in [`23-code-library/freertos/`](../23-code-library/freertos/README.md) (`build/nucleo_g474re_freertos.elf`, `SVC`/`PendSV`/`SysTick` vector bindings verified via `arm-none-eabi-nm` & `objdump`).
> - **`HOST TESTED` (`gcc 15.2.0`):** **`Lab 20`** compiles cleanly and passes 100% of host unit assertions (exit code `0`) alongside `tests/test_c_foundations.c` and `tests/unit/test_ring_buffer.c`.
> - **Physical Hardware Validation Status (Agent 4 on NUCLEO-G474RE `MB1367` Rev C, `STM32G474RET6`, STLINK-V3E `V3J17M11`, `COM8`, `VDD = 3.28 V` — see [`AUDIT/HARDWARE_VALIDATION_SUMMARY.md`](../AUDIT/HARDWARE_VALIDATION_SUMMARY.md)):**
>   - **13 / 19 STM32 Target Labs `ONBOARD HARDWARE TESTED`:** **`Lab 15`** (FreeRTOS `V11.1.0` standalone ELF, `6/6 ALL_PASSED` + 8 s `[TELEM]`) + **`Labs 01, 02, 03, 04, 05, 06, 08, 10, 13, 17, 18, 19`** (all core silicon peripherals, drivers, dual `160`/`170 MHz` Boost clocks, `CORDIC`/`FMAC`, `FDCAN1`/`CRC-32`, `MPU`/`.noinit` `DIV0` reboot, and `IWDG` physically executed via [`HARDWARE_VALIDATION/onboard_selftest/`](../HARDWARE_VALIDATION/onboard_selftest/Src/main.c) `9/9 PASS` + Capstones 01–03).
>   - **1 / 20 Host-Only Lab (`HOST TESTED`):** `Lab 20` (`gcc 15.2.0`, zero MCU hardware required).
>   - **6 / 19 STM32 Target Labs `EXTERNAL HARDWARE PENDING`:** `Labs 07, 09, 11, 12, 16` (external breadboard components/oscilloscope) + `Lab 14` (Stop 0 manual `B1` press & `JP6` µA ammeter) — consolidated bench procedures in [`HARDWARE_VALIDATION/PENDING_EXTERNAL_CONNECTIONS.md`](../HARDWARE_VALIDATION/PENDING_EXTERNAL_CONNECTIONS.md).
> - **Safety Notice:** Always verify jumper positions (**`JP6` for `IDD`, NEVER `JP5`**) and voltage limits ($V_{\text{DD}} \le 3.3\text{ V}$) before wiring any external hardware.

This directory contains **20 progressive, standards-compliant laboratory experiments** targeting the **ST NUCLEO-G474RE (`MB1367` Rev C, STM32G474RET6 LQFP64)**. Every lab follows the mandatory 12-section engineering specification (Objective + Concept IDs, Required Hardware, Required Software, Wiring & Pin Config, Register Config & Math with **explicit 170 MHz vs 160 MHz Clock Profile Declaration**, Code Implementation, Expected Output, Debugging Procedure, Common Errors, Verification Procedure, Extension Exercises, and an individual **Standardized Verification Record**).

---

## Clock Configuration Profiles & Reusable Driver Library (`23-code-library/Drivers/`)

To eliminate silent `16 MHz` vs `160 MHz` vs `170 MHz` timing bugs (`SysTick`, `TIM`, `USART BRR`, `SPI BR`, `I2C TIMINGR`, `ADC CKMODE`, `DAC HFSEL`, `FDCAN NBTP/DBTP`), every lab explicitly declares its primary clock profile AND provides exact **`160 MHz Range 1 Boost`** register values alongside **`170 MHz Range 1 Boost`**:

| Clock Profile | Reusable API (`clock_g4.h`) | PLL Setup (`HSI16` / `HSE24`) | `PWR` / `FLASH` | Mandatory Ramp (RM0440 §6.1.5) |
|---|---|---|---|---|
| **`170 MHz Range 1 Boost`** | `Clock_Init_170MHz()` / `Clock_Init_170MHz_HSE24()` | `16 MHz / 4 × 85 / 2` (`VCO = 340 MHz`) | `VOS=01b`, `R1MODE=0`, `4 WS` | `HPRE=/2 (85 MHz)` $\to \ge 1\ \mu\text{s} \to$ `HPRE=/1` |
| **`160 MHz Range 1 Boost`** | `Clock_Init_160MHz()` | `16 MHz / 4 × 80 / 2` (`VCO = 320 MHz`) | `VOS=01b`, `R1MODE=0`, `4 WS` | `HPRE=/2 (80 MHz)` $\to \ge 1\ \mu\text{s} \to$ `HPRE=/1` |
| **`16 MHz HSI16` (Reset)** | Default out of reset (`CLOCK_PROFILE_HSI16`) | Direct `HSI16` (no PLL) | `VOS=01b`, `R1MODE=1`, `0 WS` | None |

All 12 compile-verified bare-metal drivers in [`23-code-library/Drivers/`](../23-code-library/Drivers/) (`clock_g4`, `gpio_g4`, `exti_g4`, `tim_g4`, `usart_g4`, `spi_g4`, `i2c_g4`, `adc_g4`, `dma_g4`, `iwdg_g4`, `ring_buffer`, `fault_handler`) read CMSIS `SystemCoreClock` dynamically so baud rates, PWM frequencies, and timings remain exact across all three clock profiles (see [`AUDIT/DRIVER_LIBRARY_VERIFICATION.md`](../AUDIT/DRIVER_LIBRARY_VERIFICATION.md)).

---

## Master 20-Lab Verification Matrix

### Group A — Self-Contained Onboard Labs (13 `ONBOARD HARDWARE TESTED` + Lab 14 `EXTERNAL HARDWARE PENDING` for `JP6` Meter / Manual `B1` Wake)

| Lab | Title & Link | Primary Clock (`160 MHz` Alt?) | Reusable Drivers | Build / Verification Status | Physical Hardware Evidence |
|---|---|---|---|---|---|
| **Lab 01** | [Bare-Metal GPIO LED Blinky (3-Way)](lab-01-baremetal-gpio-led-blinky/README.md) | `16 MHz HSI16` (160/170 MHz documented) | `gpio_g4`, `clock_g4` | **`ONBOARD HARDWARE TESTED`** *(via `onboard_selftest` + template)* | `PA5` atomic `BSRR` set/rst/tog verified |
| **Lab 02** | [4-State SysTick FSM Debouncer](lab-02-gpio-pushbutton-debouncing/README.md) | `16 MHz HSI16` (`LOAD=15999`; `159999`/`169999` alt) | `gpio_g4`, `clock_g4` | **`ONBOARD HARDWARE TESTED`** *(via `onboard_selftest` + Project 01)* | `PC13` `MODER=00b` idle `0` + SysTick FSM *(manual `B1` touch optional)* |
| **Lab 03** | [RCC PLL 170 MHz / 160 MHz Boost Switch](lab-03-rcc-pll-170mhz-clock-switch/README.md) | `170 MHz Boost` (`N=85`; `N=80` @ `160 MHz`) | `clock_g4`, `gpio_g4` | **`ONBOARD HARDWARE TESTED`** *(via `onboard_selftest` Test 3A/3B)* | `160 MHz` (`1,599,997` cyc/10 ms) & `170 MHz` (`1,699,989` cyc/10 ms) |
| **Lab 04** | [SysTick Accurate 1 ms Delay & Scheduler](lab-04-systick-accurate-delay-generator/README.md) | `170 MHz Boost` (`LOAD=169999`; `159999` @ `160 MHz`) | `clock_g4`, `gpio_g4` | **`ONBOARD HARDWARE TESTED`** *(via `onboard_selftest` + Project 01)* | `SysTick` vs `DWT->CYCCNT` verified (`<0.001%`) |
| **Lab 05** | [EXTI13 Rising-Edge Interrupt (3-Way)](lab-05-exti-pushbutton-interrupt/README.md) | `16 MHz HSI16` (identical @ 160/170 MHz) | `exti_g4`, `gpio_g4`, `clock_g4` | **`ONBOARD HARDWARE TESTED`** *(via `onboard_selftest` Test 4)* | `EXTI13` `SWIER1` callback 3/3 (`SYSCFG_EXTICR4=0x20`) |
| **Lab 06** | [TIM2 10 Hz Periodic Interrupt](lab-06-general-purpose-timer-periodic-interrupt/README.md) | `170 MHz Boost` (`PSC=16999`; `15999` @ `160 MHz`) | `tim_g4`, `clock_g4`, `gpio_g4` | **`ONBOARD HARDWARE TESTED`** *(via `onboard_selftest` + FreeRTOS)* | `TIM2` `100 Hz` & `10 Hz` IRQs (`rc_w0` clear) verified |
| **Lab 08** | [USART2 SPSC Ring Buffer & `printf` (3-Way)](lab-08-usart-baremetal-driver-printf/README.md) | `170 MHz Boost` (`BRR=1476`; `1389` @ `160 MHz`) | `usart_g4`, `ring_buffer`, `clock_g4`, `gpio_g4` | **`ONBOARD HARDWARE TESTED`** *(via `onboard_selftest` + Project 01)* | Live `COM8` TX (`160`/`170 MHz`) + RX `'s'` cmd (`rx_cmds=1`) |
| **Lab 10** | [ADC1 Internal `VREFINT`/Temp + Circular DMA1](lab-10-adc-dma-multichannel-circular-buffer/README.md) | `170 MHz Boost` (`CKMODE=11b`: `42.5` / `40.0 MHz`) | `adc_g4`, `dma_g4`, `clock_g4` | **`ONBOARD HARDWARE TESTED`** *(via `onboard_selftest` + Project 02)* | `VDDA=3288 mV`, `32.91 °C`, `DMA1_Ch1` `HT=20/TC=20/TE=0` |
| **Lab 13** | [Independent Watchdog (`IWDG`) Safe Test](lab-13-iwdg-hardware-watchdog/README.md) | `16 MHz HSI16` + `32 kHz LSI` (`PR=4, RLR=499`) | `iwdg_g4`, `clock_g4`, `gpio_g4` | **`ONBOARD HARDWARE TESTED`** *(via `onboard_selftest` + FreeRTOS)* | `IWDG` (`PR=2, RLR=3999`, `DBG_IWDG_STOP=1`) `6/6` kicks |
| **Lab 14** | [Stop 0 Low-Power Mode & EXTI13 Wakeup](lab-14-low-power-stop-mode-wakeup/README.md) | `170 MHz Boost` (160 MHz restore documented) | `clock_g4`, `exti_g4`, `gpio_g4` | `CROSS-COMPILED` / **`EXTERNAL HARDWARE PENDING`** | Requires manual physical `B1` press + `JP6` µA ammeter |
| **Lab 15** | [FreeRTOS `V11.1.0` Producer-Consumer](lab-15-freertos-multitask-producer-consumer/README.md) | `170 MHz Boost` (`SystemCoreClock` dynamic @ 160/170) | [`23-code-library/freertos/`](../23-code-library/freertos/README.md) | **`ONBOARD HARDWARE TESTED`** (`nucleo_g474re_freertos.elf`) | **6/6 PASS** + 8 s `COM8` `[TELEM]` stream (`fpu_err=0`) |
| **Lab 17** | [CORDIC & FMAC Hardware Math vs FPU](lab-17-cordic-fmac-hardware-math/README.md) | `170 MHz Boost` (identical regs @ `160 MHz`) | `clock_g4` (`Clock_DWT_Init`) | **`ONBOARD HARDWARE TESTED`** *(via `onboard_selftest` Test 6A/6B)* | `CORDIC` `9.72×` faster (`2e-6` err); `FMAC` `0 LSB` diff |
| **Lab 18** | [FDCAN1 Internal Loopback & CRC-32](lab-18-fdcan-loopback-and-crc/README.md) | `170 MHz Boost` (exact `160 MHz` `NBTP`/`DBTP` alt) | `clock_g4` | **`ONBOARD HARDWARE TESTED`** *(via `onboard_selftest` Test 7)* | `CRC-32=0xCBF43926`, `FDCAN1` 16 B BRS loopback (`ECR=0`) |
| **Lab 19** | [MPU Stack Guard & Naked-ASM HardFault](lab-19-hardfault-mpu-and-crash-diagnostics/README.md) | `16 MHz HSI16` (identical @ 160/170 MHz) | `fault_handler`, `gpio_g4`, `clock_g4` | **`ONBOARD HARDWARE TESTED`** *(via `onboard_selftest` Test 8A/8B)* | `MPU_CTRL=0x5` + `DIV0` `.noinit` (`CFSR=0x02000000`) across reboot |

### Group B — Host-Tested Lab (1 Lab — Zero MCU Hardware Required)

| Lab | Title & Link | Execution Target | Reusable Drivers | Build / Verification Status | Hardware Queue |
|---|---|---|---|---|---|
| **Lab 20** | [Host TDD, Fake CMSIS Mocks & `-fstack-usage`](lab-20-host-unit-testing-and-driver-mocking/README.md) | Host PC (`gcc 15.2.0`) + ARM `.su` | `ring_buffer` + `-DDRIVER_HOST_TEST` helpers | **`HOST TESTED`** (`gcc 15.2.0`, 0 warn, exit `0`) | `HOST PC ONLY — NO MCU HARDWARE REQUIRED` |

### Group C — External-Hardware-Pending Labs (5 Labs — Cross-Compiled, Wiring & Test Procedures Ready)

| Lab | Title & Link | Primary Clock (`160 MHz` Alt) | Reusable Drivers | Build / Verification Status | External Hardware Required |
|---|---|---|---|---|---|
| **Lab 07** | [TIM3 25 kHz RGB Hardware PWM](lab-07-timer-pwm-rgb-led-fading/README.md) | `170 MHz` (`ARR=6799`) / `160 MHz` (`ARR=6399`) | `tim_g4`, `clock_g4`, `gpio_g4` | `CROSS-COMPILED` / `EXTERNAL HARDWARE PENDING` | External RGB LED + 3 × `330 Ω` resistors on `PA6`/`PA7`/`PB0` |
| **Lab 09** | [ADC1 Single-Channel Potentiometer](lab-09-adc-single-channel-potentiometer/README.md) | `170 MHz` (`42.5 MHz`) / `160 MHz` (`40.0 MHz`) | `adc_g4`, `clock_g4`, `gpio_g4` | `CROSS-COMPILED` / `EXTERNAL HARDWARE PENDING` | External `10 kΩ` potentiometer on `PA0` (`ADC1_IN1`) |
| **Lab 11** | [SPI1 Full-Duplex Master Sensor](lab-11-spi-master-sensor-interfacing/README.md) | `170 MHz` (`10.625 MHz`) / `160 MHz` (`10.000 MHz`) | `spi_g4`, `clock_g4`, `gpio_g4` | `CROSS-COMPILED` / `EXTERNAL HARDWARE PENDING` | External 3.3 V SPI Mode 3 sensor (or MISO-MOSI jumper) on `SPI1` |
| **Lab 12** | [I2C1 400 kHz & SSD1306 OLED](lab-12-i2c-master-ssd1306-oled/README.md) | `170 MHz` (`0x70811021`) / `160 MHz` (`0x70710F1F`) | `i2c_g4`, `clock_g4`, `gpio_g4` | `CROSS-COMPILED` / `EXTERNAL HARDWARE PENDING` | External SSD1306 OLED + `2.2–4.7 kΩ` pull-ups on `PA15`/`PB7` |
| **Lab 16** | [DAC1 + OPAMP1 PGA + COMP1 → `TIM1_BKIN`](lab-16-dac-opamp-comp-hardware-break/README.md) | `170 MHz` (`HFSEL=10b, ARR=3399`) / `160 MHz` (`HFSEL=01b, ARR=3199`) | `clock_g4`, `gpio_g4` | `CROSS-COMPILED` / `EXTERNAL HARDWARE PENDING` | External `PA1` DC/pot stimulus + oscilloscope on `PA8`/`PA4` *(internal `DAC1 -> OPAMP1 -> COMP1` path also observable via GDB)* |

---

## Progressive Learning Path Summary (Tiers 1–4)

- **Tier 1 (`Labs 01–06`) — Bare-Metal Core, Clocking & Interrupt Foundations:** GPIO `MODER=11b` reset & atomic `BSRR`, Active-HIGH `PC13` FSM debouncer, `170 MHz` / `160 MHz` Range 1 Boost PLL switch, `SysTick` wraparound-safe scheduling, `EXTI13` + `__DSB()`, and `TIM2` `rc_w0` periodic interrupts.
- **Tier 2 (`Labs 07–12`) — Peripherals, Serial Buses, DMA & Analog:** `TIM3` 25 kHz RGB PWM (anti-underflow), `USART2` SPSC ring buffer & `printf`, `ADC1` `CKMODE=11b` calibration, `ADC1` internal `VREFINT`/Temp + circular ping-pong `DMA1`, `SPI1` 8-bit `FRXTH`, and `I2C1` 400 kHz on `PA15`/`PB7` (`PB8-BOOT0` safe) + `RELOAD` + 9-clock recovery.
- **Tier 3 (`Labs 13–15`) — Reliability, Low-Power & RTOS:** `IWDG` with `DBG_IWDG_STOP`, Stop 0 low-power wakeup & `JP6` `IDD` measurement, and compiled official **FreeRTOS `V11.1.0`** multi-tasking (`23-code-library/freertos/`).
- **Tier 4 (`Labs 16–20`) — Mixed-Signal Safety, Math Accelerators, CAN-FD, Fault Diagnostics & Host TDD:** Sub-50 ns `DAC1`/`OPAMP1`/`COMP1` $\to$ `TIM1_BKIN` trip, `CORDIC` + `FMAC` vs FPU `DWT->CYCCNT`, `FDCAN1` loopback + `CRC-32`, 8-region `MPU` stack guard + `.noinit` crash recovery, and host `gcc` TDD + `-fstack-usage`.
