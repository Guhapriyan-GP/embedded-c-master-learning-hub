# Capstone Projects Overview (STM32G474RE / NUCLEO-G474RE)

All three capstone projects in this directory are **complete, standalone, reproducibly cross-compiled STM32G474RE firmware projects** (`Inc/`, `Src/`, `Makefile`, architecture documentation, wiring specifications, and honest verification records) built atop the repository's shared verified bare-metal drivers (`23-code-library/Drivers/`), linker/startup templates (`23-code-library/templates/`), and official pinned **`FreeRTOS-Kernel V11.1.0`** (`23-code-library/freertos/FreeRTOS-Kernel/`).

> **Honest Verification Policy:** Every project below is **`CROSS-COMPILED`** with `arm-none-eabi-gcc 14.3.1` (`0` errors, `0` warnings under `-std=c11 -Wall -Wextra -Werror -Wpedantic -Wshadow -Wconversion -Wdouble-promotion -Wundef`) and designed with a **zero-external-wire onboard test mode** (`READY FOR AGENT 4 ONBOARD TEST`). Physical bench validation on the NUCLEO-G474RE board is tracked separately by the hardware test harness; external add-on components (potentiometer, SSD1306 OLED, external power stage) are explicitly marked `EXTERNAL HARDWARE PENDING`.

---

## Capstone Project Matrix

| Project Directory | Title | Tier | Key Peripherals & RTOS Stack | Debug Footprint (`arm-none-eabi-size`) | Verification Status |
|---|---|---|---|---|---|
| [`01-beginner-smart-instrument-panel/`](01-beginner-smart-instrument-panel/README.md) | **Smart Instrument Panel & Telemetry Unit** | Level 1–2 | `170 MHz` / `160 MHz` (`-DUSE_CLOCK_160MHZ`), SysTick 4-state `B1` debouncer FSM, `ADC1` (`VREFINT` + Temp + `PA0`), `TIM2_CH2` (`PA1` AF1 PWM), `USART2` JSON + `ring_buffer_t` RX parser, `IWDG` | Flash: `12520 B` (`2.39%`) / RAM: `2232 B` (`2.27%`) | `CROSS-COMPILED` / `READY FOR AGENT 4 ONBOARD TEST` / `EXTERNAL HARDWARE PENDING` (`PA0` pot, `PA1` load) |
| [`02-intermediate-dma-data-logger/`](02-intermediate-dma-data-logger/README.md) | **High-Speed Multichannel DMA Data Logger** | Level 2 | `170 MHz`, `TIM3` TRGO (`100 Hz`), `ADC1` scan (`VREFINT` + Temp `640.5` cyc) $\to$ `DMA1_Ch1` (`DMAMUX1 Req 5`, ES0430-safe `CHTIF1`/`CTCIF1`), hardware IEEE 802.3 `CRC-32`, `USART2`, optional `I2C1` (`PA15`/`PB7`) OLED, Flash Bank 2 (`0x08040000`) 64-bit dry-run staging | Flash: `11588 B` (`2.21%`) / RAM: `2296 B` (`2.34%`) | `CROSS-COMPILED` / `READY FOR AGENT 4 ONBOARD TEST` / `EXTERNAL HARDWARE PENDING` (`I2C1` OLED) |
| [`03-advanced-freertos-digital-power-controller/`](03-advanced-freertos-digital-power-controller/README.md) | **FreeRTOS Digital Power & Motor Controller** | Level 3 | **FreeRTOS V11.1.0** (`ARM_CM4F`, `heap_4.c`, `PRIGROUP=0`), `TIM1` complementary PWM (`PA8`/`PB13`, `50 kHz`, `500 ns` dead-time), async `< 50 ns` `COMP1 -> TIM1->AF1.BKCMP1E` hardware break + `xTaskNotifyFromISR()`, 4 static tasks (Prio `4/3/2/1`), Q15 PI with anti-windup, `IWDG` heartbeat supervisor | Flash: `21264 B` (`4.06%`) / RAM: `31552 B` (`32.10%`) | `CROSS-COMPILED` / `READY FOR AGENT 4 ONBOARD TEST` (onboard sim mode) / `EXTERNAL HARDWARE PENDING` (external power stage, OLED) |

---

## Quick Reproducible Build Check

From any capstone directory:

```bash
make clean all                     # Debug build (-Og -g3) -> build/*.elf, *.bin, *.hex
make clean all BUILD_TYPE=release  # Release build (-O2 -g)
```

Additional project-specific options:
- Project 01: `make clean all CLOCK_PROFILE=160` (compiles with `-DUSE_CLOCK_160MHZ` calling `Clock_Init_160MHz()`).
- Project 03: `make clean all POWER_SIM_MODE=0` (configures `PA1` as pure analog shunt sense + `1.65 V` `DAC1_CH1` threshold for external power hardware).

See [`AUDIT/CAPSTONE_IMPLEMENTATION_REPORT.md`](../AUDIT/CAPSTONE_IMPLEMENTATION_REPORT.md) for the complete cross-compilation audit, symbol verification, and hardware handoff record.
