# Practical Laboratory 6-State Verification Matrix (Labs 01–20)

> **Date:** 2026-11-18 | **Branch:** `feature/complete-remediation`  
> **Target Platform:** ST **NUCLEO-G474RE** (`MB1367 Rev C`, `STM32G474RET6`, `VDD = 3.28–3.29 V`, STLINK-V3E `V3J17M11`, `COM8` `115200 8N1`)  
> **Toolchain:** `arm-none-eabi-gcc 14.3.1 20250623 (GNU Tools for STM32 14.3.rel1)` (`-std=c11 -mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard -DSTM32G474xx -Wall -Wextra -Werror -O2`) & Host `gcc 15.2.0` (`MSYS2 UCRT64`)  
> **Primary Sources:** [`AUDIT/HARDWARE_VALIDATION_SUMMARY.md`](HARDWARE_VALIDATION_SUMMARY.md), [`AUDIT/HARDWARE_VERIFICATION_MATRIX.md`](HARDWARE_VERIFICATION_MATRIX.md), [`HARDWARE_VALIDATION/PENDING_EXTERNAL_CONNECTIONS.md`](../HARDWARE_VALIDATION/PENDING_EXTERNAL_CONNECTIONS.md)

---

## 1. Summary Totals Across All 6 Independent Verification States

| Verification State | PASS | PARTIAL | N/A | `PENDING — USER CONNECTION REQUIRED` | Total Labs |
|---|---:|---:|---:|---:|---:|
| **1. Static review** | 20 | 0 | 0 | 0 | **20** |
| **2. Host test passed** | 13 | 0 | 7 | 0 | **20** |
| **3. Cross-compilation passed** | 19 | 0 | 1 *(Lab 20 host-only)* | 0 | **20** |
| **4. On-target upload succeeded** | 13 | 0 | 1 *(Lab 20 host-only)* | 6 *(Labs 07, 09, 11, 12, 14, 16)* | **20** |
| **5. Physical behavior verified** | 13 | 0 | 1 *(Lab 20 host-only)* | 6 *(Labs 07, 09, 11, 12, 14, 16)* | **20** |
| **6. Measurement verified** | 11 | 3 *(Labs 01, 02, 05)* | 0 | 6 *(Labs 07, 09, 11, 12, 14, 16)* | **20** |

> [!NOTE]
> **Strict Honesty Policy:** No lab is labeled "physically verified" based on compilation alone. Every `PASS` in Columns 4–6 corresponds to recorded `COM8` / SWD evidence on physical NUCLEO-G474RE silicon ([`HARDWARE_VALIDATION_SUMMARY.md`](HARDWARE_VALIDATION_SUMMARY.md)). Every experiment requiring external wiring, sensors, manual Stop 0 `B1`/`JP6` current profiling, or an oscilloscope is explicitly marked **`PENDING — USER CONNECTION REQUIRED`**.

---

## 2. Master 20-Lab × 6-State Granular Matrix

| Lab | Lab Title & Link | Clock Profile (`160`/`170 MHz`) | 1. Static Review | 2. Host Test Passed | 3. Cross-Compilation Passed | 4. On-Target Upload Succeeded | 5. Physical Behavior Verified | 6. Measurement Verified | Primary Evidence / Artifact |
|---|---|---|---|---|---|---|---|---|---|
| **01** | [Bare-Metal GPIO LED Blinky (3-Way)](../18-practical-laboratory/lab-01-baremetal-gpio-led-blinky/README.md) | `16 MHz` (`160`/`170` alt) | `PASS` | `PASS` (`test_gpio_mock`) | `PASS` (`0 warn`, Appr. A) | `PASS` | `PASS` | `PARTIAL` *(scope `OSPEEDR` opt.)* | `onboard_selftest` Test 2 (`PA5` `BSRR`, `MODER=0xABFFF7AF`) + restored `firmware.elf`; Appr. B/C LL & HAL statically reviewed |
| **02** | [4-State SysTick FSM Debouncer](../18-practical-laboratory/lab-02-gpio-pushbutton-debouncing/README.md) | `16 MHz` (`160`/`170` alt) | `PASS` | `PASS` (`test_gpio_mock`) | `PASS` (`0 warn`) | `PASS` | `PASS` | `PARTIAL` *(manual `B1` opt.)* | `onboard_selftest` Test 2 (`PC13_MODER=0, IDR=0, GPIOC_MODER=0xF3FFFFFF`) + Project 01 FSM |
| **03** | [RCC PLL 170/160 MHz Boost Switch](../18-practical-laboratory/lab-03-rcc-pll-170mhz-clock-switch/README.md) | `170 MHz` (`160 MHz` alt) | `PASS` | `N/A` *(silicon PLL)* | `PASS` (`0 warn`) | `PASS` | `PASS` | `PASS` | `onboard_selftest` Test 3A (`1,599,997` cyc/10 ms `-0.0002%`) & Test 3B (`1,699,989` cyc/10 ms `-0.0006%`), `PWR_CR5=0`, `FLASH_ACR=0x00040704` |
| **04** | [SysTick Accurate 1 ms Delay & Scheduler](../18-practical-laboratory/lab-04-systick-accurate-delay-generator/README.md) | `170 MHz` (`160 MHz` alt) | `PASS` | `N/A` *(silicon SysTick)* | `PASS` (`0 warn`) | `PASS` | `PASS` | `PASS` | `onboard_selftest` Test 3A/3B (`<0.001%` vs `DWT`) + Project 01 exact `500 ms` JSON cadence |
| **05** | [EXTI13 Rising-Edge Interrupt (3-Way)](../18-practical-laboratory/lab-05-exti-pushbutton-interrupt/README.md) | `16 MHz` (`160`/`170` alt) | `PASS` | `PASS` (`test_drivers_math`) | `PASS` (`0 warn`, Appr. A) | `PASS` | `PASS` | `PARTIAL` *(manual `B1` opt.)* | `onboard_selftest` Test 4 (`EXTI13 swier_cb=3/3, SYSCFG_EXTICR4=0x20`); Appr. A/B/C all complete w/ `(void)EXTI->PR1; __DSB();` |
| **06** | [TIM2 10 Hz Periodic Interrupt](../18-practical-laboratory/lab-06-general-purpose-timer-periodic-interrupt/README.md) | `170 MHz` (`160 MHz` alt) | `PASS` | `PASS` (`test_drivers_math`) | `PASS` (`0 warn`) | `PASS` | `PASS` | `PASS` | `onboard_selftest` Test 4 (`TIM2` `100 Hz` `11` IRQs/`100 ms`) + FreeRTOS `10 Hz` (`80` IRQs/`8 s`) |
| **07** | [TIM3 25 kHz RGB Hardware PWM](../18-practical-laboratory/lab-07-timer-pwm-rgb-led-fading/README.md) | `170 MHz` (`160 MHz` alt) | `PASS` | `PASS` (`test_drivers_math`) | `PASS` (`0 warn`) | `PENDING — USER CONNECTION REQUIRED` | `PENDING — USER CONNECTION REQUIRED` | `PENDING — USER CONNECTION REQUIRED` | External RGB LED + 3 × `330 Ω` on `PA6`/`PA7`/`PB0` + scope `25.00 kHz` check |
| **08** | [USART2 SPSC Ring Buffer & `printf` (3-Way)](../18-practical-laboratory/lab-08-usart-baremetal-driver-printf/README.md) | `170 MHz` (`160 MHz` alt) | `PASS` | `PASS` (`test_ring_buffer` + math) | `PASS` (`0 warn`, Appr. A) | `PASS` | `PASS` | `PASS` | Live `COM8` TX (`BRR=1389` @ 160 MHz, `1476` @ 170 MHz) + Project 01 RX `'s'` cmd (`rx_cmds: 0 -> 1`); Appr. B/C LL & HAL statically reviewed |
| **09** | [ADC1 Single-Channel Potentiometer](../18-practical-laboratory/lab-09-adc-single-channel-potentiometer/README.md) | `170 MHz` (`160 MHz` alt) | `PASS` | `PASS` (`test_drivers_math`) | `PASS` (`0 warn`) | `PENDING — USER CONNECTION REQUIRED` | `PENDING — USER CONNECTION REQUIRED` | `PENDING — USER CONNECTION REQUIRED` | External `10 kΩ` pot on `PA0` (`0..3.3 V`) + DMM check *(core `adc_g4.c` init verified in Test 5B)* |
| **10** | [ADC1 Internal `VREFINT`/Temp + Circular DMA1](../18-practical-laboratory/lab-10-adc-dma-multichannel-circular-buffer/README.md) | `170 MHz` (`160 MHz` alt) | `PASS` | `PASS` (`test_drivers_math`) | `PASS` (`0 warn`) | `PASS` | `PASS` | `PASS` | `onboard_selftest` Test 5B/5C (`VDDA=3288 mV`, `32.91 °C`, `HT=20/TC=20/TE=0`) + Project 02 `25×` `$LOG2` frames |
| **11** | [SPI1 Full-Duplex Master Sensor](../18-practical-laboratory/lab-11-spi-master-sensor-interfacing/README.md) | `170 MHz` (`160 MHz` alt) | `PASS` | `PASS` (`test_drivers_math`) | `PASS` (`0 warn`) | `PENDING — USER CONNECTION REQUIRED` | `PENDING — USER CONNECTION REQUIRED` | `PENDING — USER CONNECTION REQUIRED` | External `PB4 <-> PB5` jumper or `3.3 V` SPI Mode 3 slave + logic analyzer |
| **12** | [I2C1 400 kHz & SSD1306 OLED](../18-practical-laboratory/lab-12-i2c-master-ssd1306-oled/README.md) | `170 MHz` (`160 MHz` alt) | `PASS` | `PASS` (`test_drivers_math`) | `PASS` (`0 warn`) | `PENDING — USER CONNECTION REQUIRED` | `PENDING — USER CONNECTION REQUIRED` | `PENDING — USER CONNECTION REQUIRED` | External `3.3 V` SSD1306 OLED (`0x3C`) + `4.7 kΩ` pull-ups on `PA15`/`PB7` *(absent-slave `NACKF` verified in Project 02)* |
| **13** | [Independent Watchdog (`IWDG`) Safe Test](../18-practical-laboratory/lab-13-iwdg-hardware-watchdog/README.md) | `16 MHz` + `32 kHz LSI` | `PASS` | `PASS` (`test_drivers_math`) | `PASS` (`0 warn`) | `PASS` | `PASS` | `PASS` | `onboard_selftest` Test 9 (`PR=2 RLR=3999 DBGMCU_APB1FZR1=0x1800 kicks=6/6`) + FreeRTOS `IWDG` (`wdg=2..9`) |
| **14** | [Stop 0 Low-Power Mode & EXTI13 Wakeup](../18-practical-laboratory/lab-14-low-power-stop-mode-wakeup/README.md) | `170 MHz` (`160 MHz` alt) | `PASS` | `N/A` *(silicon PWR)* | `PASS` (`0 warn`) | `PENDING — USER CONNECTION REQUIRED` | `PENDING — USER CONNECTION REQUIRED` | `PENDING — USER CONNECTION REQUIRED` | Requires manual physical `B1` wakeup press + standalone µA DMM across `JP6` (`IDD`, NEVER `JP5`) |
| **15** | [FreeRTOS `V11.1.0` Producer-Consumer](../18-practical-laboratory/lab-15-freertos-multitask-producer-consumer/README.md) | `170 MHz` (`160` dyn.) | `PASS` | `N/A` *(CM4F RTOS)* | `PASS` (`nucleo_g474re_freertos.elf`, `0 warn`) | `PASS` | `PASS` | `PASS` | `nucleo_g474re_freertos.elf`: **`6/6 ALL_PASSED`** + `8.0 s` live `COM8` `[TELEM]` (`fpu_err=0`, `heap=21104/21104`) |
| **16** | [DAC1 + OPAMP1 PGA + COMP1 → `TIM1_BKIN`](../18-practical-laboratory/lab-16-dac-opamp-comp-hardware-break/README.md) | `170 MHz` (`160 MHz` alt) | `PASS` | `N/A` *(mixed-signal)* | `PASS` (`0 warn`) | `PENDING — USER CONNECTION REQUIRED` | `PENDING — USER CONNECTION REQUIRED` | `PENDING — USER CONNECTION REQUIRED` | External `PA1` `0..3.0 V` stimulus + `>= 100 MHz` scope on `PA8`/`PA4` *(digital `TIM1`/`COMP1` arming verified in Project 03)* |
| **17** | [CORDIC & FMAC Hardware Math vs FPU](../18-practical-laboratory/lab-17-cordic-fmac-hardware-math/README.md) | `170 MHz` (`160 MHz` alt) | `PASS` | `N/A` *(silicon math)* | `PASS` (`0 warn`) | `PASS` | `PASS` | `PASS` | `onboard_selftest` Test 6A (`CORDIC` `20.9×`/`9.72×`, `max_err=2e-6`) & Test 6B (`FMAC` `2.32×`, `0 LSB` diff) |
| **18** | [FDCAN1 Internal Loopback & CRC-32](../18-practical-laboratory/lab-18-fdcan-loopback-and-crc/README.md) | `170 MHz` (`160 MHz` alt) | `PASS` | `N/A` *(silicon CAN/CRC)* | `PASS` (`0 warn`) | `PASS` | `PASS` | `PASS` | `onboard_selftest` Test 7 (`CRC-32=0xCBF43926`, `FDCAN1` 16 B BRS `MATCH=1`, `ECR=0`) |
| **19** | [MPU Stack Guard & Naked-ASM HardFault](../18-practical-laboratory/lab-19-hardfault-mpu-and-crash-diagnostics/README.md) | `16 MHz` (`160`/`170` alt) | `PASS` | `PASS` (`test_gpio_mock`) | `PASS` (`0 warn`) | `PASS` | `PASS` | `PASS` | `onboard_selftest` Test 8A/8B (`MPU_CTRL=0x5`, `DIV0` `.noinit` `magic=0xFA017C0D`, `CFSR=0x02000000` across reset) |
| **20** | [Host TDD, Fake CMSIS Mocks & `-fstack-usage`](../18-practical-laboratory/lab-20-host-unit-testing-and-driver-mocking/README.md) | Host PC (`gcc 15.2.0`) | `PASS` | `PASS` (**594/594**) | `N/A` *(ARM `.su` verified)* | `N/A` *(Host PC)* | `N/A` *(Host PC)* | `PASS` | Lab 20 standalone exit `0` + `make -C tests clean test` (**594/594** assertions PASS) |

---

## 3. 3-Way API Comparison Labs (`Lab 01`, `Lab 05`, `Lab 08`) — Approach Parity Audit

| Lab | Approach A (Bare-Metal CMSIS) | Approach B (STM32CubeG4 LL) | Approach C (STM32CubeG4 HAL) |
|---|---|---|---|
| **Lab 01** (`GPIO PA5`) | Complete `main()` + `delay_cycles()` — **Compile-Verified** (`arm-none-eabi-gcc 14.3.1`, 0 warn) | Complete `Lab01_LL_GPIO_Init()` + `delay_cycles()` + `main()` — **Statically Reviewed** *(requires CubeG4 LL headers)* | Complete `HAL_Init()` + `SysTick_Handler()` + `Lab01_HAL_GPIO_Init()` + `main()` — **Statically Reviewed** *(requires CubeG4 HAL)* |
| **Lab 05** (`EXTI13 -> PA5`) | Complete `main()` + `EXTI15_10_IRQHandler()` w/ `EXTI->PR1 = EXTI_PR1_PIF13; (void)EXTI->PR1; __DSB();` — **Compile-Verified** (0 warn) | Complete `PA5`+`PC13`+`EXTI13`+`NVIC` init + `EXTI15_10_IRQHandler()` (`LL_EXTI_ClearFlag_0_31` + `(void)EXTI->PR1; __DSB();` + `LL_GPIO_TogglePin`) + `main()` — **Statically Reviewed** *(requires CubeG4 LL headers)* | Complete `HAL_Init()` + `SysTick_Handler()` + `PA5`+`PC13` init + `EXTI15_10_IRQHandler()` -> `HAL_GPIO_EXTI_IRQHandler()` -> `HAL_GPIO_EXTI_Callback()` + `main()` — **Statically Reviewed** *(requires CubeG4 HAL)* |
| **Lab 08** (`USART2` `115200`) | Complete `Clock_Init_170MHz()` + `USART2_Init()` + SPSC `USART2_IRQHandler()` + `_write()` + `main()` — **Compile-Verified** (0 warn) | Complete `PA2`/`PA3` `AF7` + `LL_USART_*` + SPSC `USART2_IRQHandler()` + bounded TX + `main()` — **Statically Reviewed** *(requires CubeG4 LL headers)* | Complete `HAL_Init()` + `SysTick_Handler()` + `PA2`/`PA3` `AF7` + `HAL_UART_Init()` + `USART2_IRQHandler()` -> `HAL_UART_RxCpltCallback()` + `main()` — **Statically Reviewed** *(requires CubeG4 HAL)* |
