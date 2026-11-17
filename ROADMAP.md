# Curriculum Roadmap & Milestone Schedule

This roadmap outlines the complete learning journey through the Embedded C Master Learning Hub, organized into four progressive phases comprising 16 sequential stages and 15 practical laboratories.

---

### Phase 1: Foundations (Stages 01 - 04)
- **Milestone 1.1:** C syntax, pointer mechanics, struct memory layouts, and bitwise manipulation.
- **Milestone 1.2:** Volatile memory access, memory-mapped I/O, and atomic bit setting via registers.
- **Milestone 1.3:** ARMv7-M architecture, core registers, stack pointer operation, and exception handling.
- **Milestone 1.4:** Building bare-metal firmware using `arm-none-eabi-gcc`, custom linker scripts, and startup code.
- **Phase 1 Laboratory:** Complete standalone build and flashing of bare-metal blinky on NUCLEO-G474RE.

---

### Phase 2: Core Platform & Power (Stages 05 - 08)
- **Milestone 2.1:** Microcontroller architecture, multi-layer bus matrix, and documentation navigation (RM0440, DS12288).
- **Milestone 2.2:** System clock synthesis, PLL multipliers/dividers, Flash wait states, and 170 MHz Range 1 Boost Mode.
- **Milestone 2.3:** Digital I/O configuration, push-pull vs open-drain, pull-ups, and atomic BSRR control.
- **Milestone 2.4:** External interrupts (EXTI), SYSCFG multiplexing, and NVIC priority grouping.
- **Phase 2 Laboratories:** Labs 01 through 05 (GPIO, debouncing, 170 MHz clocks, SysTick, EXTI).

---

### Phase 3: Peripherals & Data Transport (Stages 09 - 12)
- **Milestone 3.1:** Hardware timers, prescalers, auto-reload, periodic interrupts, and 25 kHz PWM generation.
- **Milestone 3.2:** Asynchronous and synchronous serial communications: USART with retargeted printf, SPI master, and I2C master.
- **Milestone 3.3:** High-speed 12-bit SAR ADC acquisition, internal voltage regulator power-up, and hardware self-calibration.
- **Milestone 3.4:** Direct Memory Access (DMA) streaming, DMAMUX request routing, and double-buffering ping-pong buffers.
- **Phase 3 Laboratories:** Labs 06 through 12 (Timers, PWM, USART, ADC, DMA, SPI, I2C).

---

### Phase 4: Systems, Concurrency & Production Architecture (Stages 13 - 16)
- **Milestone 4.1:** Embedded Flash page programming, wear leveling, and custom secondary bootloader architecture.
- **Milestone 4.2:** System reliability, Independent Watchdog (IWDG) timeout management, and low-power Stop modes.
- **Milestone 4.3:** FreeRTOS multitasking kernel, task control blocks, context switching, queues, semaphores, and mutexes with priority inheritance.
- **Milestone 4.4:** Layered firmware architecture, table-driven finite state machines, static analysis, and MISRA C:2012 compliance.
- **Phase 4 Laboratories:** Labs 13 through 15 (IWDG, Stop mode, FreeRTOS multitasking) and Capstone Projects 01 - 03.
