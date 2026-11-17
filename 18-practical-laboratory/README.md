# Practical Laboratory Roadmap for STM32G474RE

This directory contains **15 structured, reproducible hands-on bare-metal laboratory experiments** targeting the **NUCLEO-G474RE** evaluation board.

Every experiment includes exact register definitions, mathematical derivations, wiring guides, full C source code, GDB debugging steps, and common beginner pitfalls.

| Lab Directory | Experiment Title | Primary Hardware & Peripherals |
|---|---|---|
| [`lab-01-baremetal-gpio-led-blinky/`](lab-01-baremetal-gpio-led-blinky/README.md) | **Lab 01: Bare-Metal GPIO LED Blinky** | Onboard Green LED LD2 (PA5) |
| [`lab-02-gpio-pushbutton-debouncing/`](lab-02-gpio-pushbutton-debouncing/README.md) | **Lab 02: GPIO Pushbutton Debouncing** | Onboard Blue Button B1 (PC13) |
| [`lab-03-rcc-pll-170mhz-clock-switch/`](lab-03-rcc-pll-170mhz-clock-switch/README.md) | **Lab 03: RCC PLL 170 MHz Clock Switch** | Core Boost Mode, Flash Wait States |
| [`lab-04-systick-accurate-delay-generator/`](lab-04-systick-accurate-delay-generator/README.md) | **Lab 04: SysTick Accurate Millisecond Delay** | Core SysTick Timer |
| [`lab-05-exti-pushbutton-interrupt/`](lab-05-exti-pushbutton-interrupt/README.md) | **Lab 05: EXTI Pushbutton Interrupt & NVIC** | SYSCFG, EXTI13, NVIC Priority |
| [`lab-06-general-purpose-timer-periodic-interrupt/`](lab-06-general-purpose-timer-periodic-interrupt/README.md) | **Lab 06: General-Purpose Timer Interrupts (TIM2)** | 32-bit Timer TIM2, Periodic ISR |
| [`lab-07-timer-pwm-rgb-led-fading/`](lab-07-timer-pwm-rgb-led-fading/README.md) | **Lab 07: Hardware PWM Output & Fading (TIM3)** | 25 kHz PWM, CCR1 Preload, PA6 |
| [`lab-08-usart-baremetal-driver-printf/`](lab-08-usart-baremetal-driver-printf/README.md) | **Lab 08: USART2 Bare-Metal Driver & printf** | 115200 Baud, ST-LINK VCP (PA2/PA3) |
| [`lab-09-adc-single-channel-potentiometer/`](lab-09-adc-single-channel-potentiometer/README.md) | **Lab 09: ADC1 Single-Channel Calibration** | 12-bit SAR ADC, Calibration, PA0 |
| [`lab-10-adc-dma-multichannel-circular-buffer/`](lab-10-adc-dma-multichannel-circular-buffer/README.md) | **Lab 10: Multichannel ADC with DMA Circular Buffer** | DMA1 Ch1, DMAMUX, Scan Mode |
| [`lab-11-spi-master-sensor-interfacing/`](lab-11-spi-master-sensor-interfacing/README.md) | **Lab 11: SPI Master Interfacing with Sensor** | SPI1 Mode 3, 10.6 MHz, PA5/PA6/PA7 |
| [`lab-12-i2c-master-ssd1306-oled/`](lab-12-i2c-master-ssd1306-oled/README.md) | **Lab 12: I2C Master Interfacing with SSD1306 OLED** | 400 kHz Fast Mode, TIMINGR, PB8/PB9 |
| [`lab-13-iwdg-hardware-watchdog/`](lab-13-iwdg-hardware-watchdog/README.md) | **Lab 13: Independent Watchdog (IWDG) Protection** | 32 kHz LSI, Fail-safe Reset |
| [`lab-14-low-power-stop-mode-wakeup/`](lab-14-low-power-stop-mode-wakeup/README.md) | **Lab 14: Low-Power Stop Mode with EXTI Wakeup** | Stop 0 Mode, Microamp Sleep, Wakeup |
| [`lab-15-freertos-multitask-producer-consumer/`](lab-15-freertos-multitask-producer-consumer/README.md) | **Lab 15: FreeRTOS Multitasking Producer-Consumer** | Preemptive Tasks, Queues, Mutexes |
