# Practical Laboratory Roadmap for STM32G474RE (Labs 01–20)

> **Verification Transparency:** All bare-metal C implementations (`Labs 01–14, 16–19`) are **Compile-Verified (`arm-none-eabi-gcc 14.3.1`, `-Wall -Wextra -Werror`)** against official STM32G474RE CMSIS headers (`stm32g474xx.h`); `Lab 15` (FreeRTOS) is statically reviewed against FreeRTOS `ARM_CM4F` rules; `Lab 20` (Host Unit Testing) is **Compile- & Execution-Verified (`gcc 15.2.0`)**. **None of the STM32 target labs have been executed on physical NUCLEO-G474RE hardware yet** — always verify jumper positions (`JP6` for `IDD`, never `JP5`) and voltage limits ($V_{\text{DD}} \le 3.3\text{ V}$) before powering hardware.

This directory contains **20 progressive, standards-compliant laboratory experiments** targeting the **ST NUCLEO-G474RE (`MB1367` Rev C, STM32G474RET6 LQFP64)**. Every lab follows the mandatory 11-section engineering specification (Objective + Concept IDs, Required Hardware, Required Software, Wiring & Pin Config, Register Config & Math, Code Implementation, Expected Output, Debugging Procedure, Common Errors, Verification Procedure, Extension Exercises).

---

## Tier 1 — Bare-Metal Core, Clocking & Interrupt Foundations (`Labs 01–06`)

| Lab Directory | Experiment Title | Key Hardware, Peripherals & Patterns |
|---|---|---|
| [`lab-01-baremetal-gpio-led-blinky/`](lab-01-baremetal-gpio-led-blinky/README.md) | **Lab 01: Bare-Metal GPIO LED Blinky (3-Way API)** | `LD2` (`PA5`), `BSRR`, STM32G4 Analog `MODER=11b` reset, Register vs LL vs HAL |
| [`lab-02-gpio-pushbutton-debouncing/`](lab-02-gpio-pushbutton-debouncing/README.md) | **Lab 02: Non-Blocking 4-State SysTick FSM Debouncer** | Active-HIGH `B1` (`PC13`, UM2505 pull-down), 4-state FSM, atomic `BSRR` |
| [`lab-03-rcc-pll-170mhz-clock-switch/`](lab-03-rcc-pll-170mhz-clock-switch/README.md) | **Lab 03: RCC PLL 170 MHz Boost Mode Switch** | `PWR_SR2.VOSF`, `PWR_CR5@0x80.R1MODE=0`, 4WS, RM0440 §6.1.5 `HPRE=/2` ramp, `MCO` |
| [`lab-04-systick-accurate-delay-generator/`](lab-04-systick-accurate-delay-generator/README.md) | **Lab 04: SysTick Accurate 1 ms Delay & Scheduler** | 170 MHz `SysTick`, wraparound-safe `(uint32_t)(now - start) < delay`, `millis()` |
| [`lab-05-exti-pushbutton-interrupt/`](lab-05-exti-pushbutton-interrupt/README.md) | **Lab 05: EXTI13 Rising-Edge Interrupt & NVIC (3-Way)** | `SYSCFGEN`, `EXTI->RTSR1`, `rc_w1` `PR1`, `__DSB()`, Register vs LL vs HAL |
| [`lab-06-general-purpose-timer-periodic-interrupt/`](lab-06-general-purpose-timer-periodic-interrupt/README.md) | **Lab 06: TIM2 10 Hz Periodic Interrupt** | 170 MHz `TIM2`, `EGR=UG` shadow load, atomic `rc_w0` `SR = ~UIF`, `__DSB()` |

---

## Tier 2 — Peripherals, Serial Buses, DMA & Analog (`Labs 07–12`)

| Lab Directory | Experiment Title | Key Hardware, Peripherals & Patterns |
|---|---|---|
| [`lab-07-timer-pwm-rgb-led-fading/`](lab-07-timer-pwm-rgb-led-fading/README.md) | **Lab 07: TIM3 25 kHz RGB Hardware PWM Fading** | `PA6`/`PA7`/`PB0` `AF2`, `OCxPE`/`ARPE`, `EGR=UG`, signed `int32_t` anti-underflow |
| [`lab-08-usart-baremetal-driver-printf/`](lab-08-usart-baremetal-driver-printf/README.md) | **Lab 08: USART2 SPSC Ring Buffer & `printf` (3-Way)** | `115200 8N1` @ 170 MHz, bounded TX timeout, lock-free RX ISR, Register/LL/HAL |
| [`lab-09-adc-single-channel-potentiometer/`](lab-09-adc-single-channel-potentiometer/README.md) | **Lab 09: ADC1 Calibration & Single-Channel Read** | `CKMODE=11b` (`42.5 MHz <= 60 MHz`), `DEEPPWD -> ADVREGEN -> ADCAL -> ADEN`, `ADDIS` |
| [`lab-10-adc-dma-multichannel-circular-buffer/`](lab-10-adc-dma-multichannel-circular-buffer/README.md) | **Lab 10: Multichannel ADC1 + Circular Ping-Pong DMA** | `DMA1_Ch1`, `DMAMUX` ID `5`, `SMP16=640.5` (`15.07 µs`), `HTIE`/`TCIE`, `volatile` |
| [`lab-11-spi-master-sensor-interfacing/`](lab-11-spi-master-sensor-interfacing/README.md) | **Lab 11: SPI1 Full-Duplex Master Sensor Driver** | `10.625 MHz` Mode 3, 2-byte read protocol, `FRXTH` 8-bit FIFO, `PA5/LD2` warning |
| [`lab-12-i2c-master-ssd1306-oled/`](lab-12-i2c-master-ssd1306-oled/README.md) | **Lab 12: I2C1 400 kHz Fast Mode & SSD1306 OLED** | `TIMINGR=0x30612144`, `PB8-BOOT0` trap, `NACKF` + 9-clock recovery, `RELOAD` > 255 B |

---

## Tier 3 — Reliability, Low-Power & RTOS (`Labs 13–15`)

| Lab Directory | Experiment Title | Key Hardware, Peripherals & Patterns |
|---|---|---|
| [`lab-13-iwdg-hardware-watchdog/`](lab-13-iwdg-hardware-watchdog/README.md) | **Lab 13: Independent Watchdog (IWDG) Protection** | `PR=100b` (`/64`), `DBGMCU_APB1FZR1_DBG_IWDG_STOP`, Active-HIGH `PC13`, `IWDGRSTF` |
| [`lab-14-low-power-stop-mode-wakeup/`](lab-14-low-power-stop-mode-wakeup/README.md) | **Lab 14: Stop 0 Low-Power Mode & EXTI13 Wakeup** | **`JP6` (`IDD`)** current measurement, `PWREN`, `EXTI15_10_IRQHandler`, 170 MHz restore |
| [`lab-15-freertos-multitask-producer-consumer/`](lab-15-freertos-multitask-producer-consumer/README.md) | **Lab 15: FreeRTOS Producer-Consumer & Static Tasks** | `NVIC_SetPriorityGrouping(0)`, `FreeRTOSConfig.h`, 512-word stack, `xTaskCreateStatic`, `FromISR` |

---

## Tier 4 — STM32G474RE Advanced Mixed-Signal, Math, Automotive, Safety & TDD (`Labs 16–20`)

| Lab Directory | Experiment Title | Key Hardware, Peripherals & Patterns |
|---|---|---|
| [`lab-16-dac-opamp-comp-hardware-break/`](lab-16-dac-opamp-comp-hardware-break/README.md) | **Lab 16: DAC1 + OPAMP1 PGA + COMP1 → `TIM1_BKIN`** | `DAC1` `HFSEL=10b`, `OPAMP1` PGA ×8 internal, `COMP1` `< 50 ns` hardware PWM trip |
| [`lab-17-cordic-fmac-hardware-math/`](lab-17-cordic-fmac-hardware-math/README.md) | **Lab 17: CORDIC & FMAC Hardware Math Accelerators** | `q1.31` trig & `q1.15` FIR filter benchmarked vs Cortex-M4F FPU via `DWT->CYCCNT` |
| [`lab-18-fdcan-loopback-and-crc/`](lab-18-fdcan-loopback-and-crc/README.md) | **Lab 18: FDCAN1 Loopback (500k/2M BRS) & CRC-32** | Bosch M_CAN Message RAM (`0x4000A400`), 64-byte CAN-FD frame, IEEE 802.3 `CRC-32` |
| [`lab-19-hardfault-mpu-and-crash-diagnostics/`](lab-19-hardfault-mpu-and-crash-diagnostics/README.md) | **Lab 19: MPU Stack Guard & Naked-ASM HardFault** | 32-byte No-Access MPU guard, `CFSR`/`HFSR`/`MMFAR`/`BFAR` decoder, `.noinit` recovery |
| [`lab-20-host-unit-testing-and-driver-mocking/`](lab-20-host-unit-testing-and-driver-mocking/README.md) | **Lab 20: Host TDD, Fake CMSIS Mocks & `-fstack-usage`** | Host `gcc` unit testing (`tests/unit/test_ring_buffer.c`), `ASan`/`UBSan`, `.su` audit |
