# Project 01: Smart Instrument Panel & Telemetry Unit

> **Verification Status:** `CROSS-COMPILED` (`arm-none-eabi-gcc 14.3.1`, 0 errors / 0 warnings under strict C11 flags) | `READY FOR AGENT 4 ONBOARD TEST` (Self-contained `MODE_INTERNAL_TELEM` requires zero external wires; NOT yet flashed onto physical NUCLEO-G474RE hardware in this commit) | `EXTERNAL HARDWARE PENDING` (External `PA0` potentiometer & `PA1` PWM load)

**Target Concept IDs:** `STM32-RCC-001`, `STM32-PWR-001`, `STM32-FLASH-001`, `PERIPH-GPIO-001`, `ARM-SYST-001`, `FW-FSM-001`, `PERIPH-ADC-001`, `PERIPH-PWM-001`, `PERIPH-UART-001`, `EMB-CONC-001`, `PERIPH-WDG-001`, `REL-FAULT-001`

---

## 1. Project Overview & Architecture

The Smart Instrument Panel is a standalone, bare-metal STM32G474RE firmware application (`170 MHz SYSCLK` Boost Mode default, or `160 MHz` via `-DUSE_CLOCK_160MHZ`) built directly atop the repository's shared verified drivers in `23-code-library/Drivers/`.

It boots into **`MODE_INTERNAL_TELEM`** — a **100% self-contained onboard test mode requiring zero external wiring** — measuring internal `VREFINT` (`VDDA`) and factory-calibrated internal die temperature (`TS_CAL1`/`TS_CAL2`), driving `TIM2_CH2` hardware PWM on `PA1` (`AF1`), streaming structured JSON telemetry over `USART2` (`PA2`/`PA3` ST-LINK VCP, `115200 8N1`), and accepting non-blocking RX commands (`'m'`, `'s'`, `'h'`) via a lock-free C11 SPSC `ring_buffer_t`.

```text
[ Pushbutton B1 (PC13, Active-HIGH) ] ──> [ 4-State SysTick FSM Debouncer (25 ms) ] ─┐
[ USART2 RX (PA3, 115200 IRQ)       ] ──> [ Lock-Free SPSC ring_buffer_t ('m','s') ] ┼─> [ Non-Blocking Superloop ]
[ ADC1 @ 42.5 MHz (VREFINT/Temp/PA0)] ──> [ 50 ms Calibrated Acquisition           ] ┘      │         │        │
                                                                                            │         │        └──> [ IWDG (1.0 s) ]
                                                                    [ TIM2_CH2 PWM (PA1) ] <┘         ▼
                                                                    [ LD2 Mode LED (PA5) ]   [ USART2 JSON (PA2) ]
```

---

## 2. Project File Layout

| File | Role |
|---|---|
| `Inc/app_panel.h` | Mode enum (`MODE_IDLE`, `MODE_INTERNAL_TELEM`, `MODE_EXT_POT_PWM`), 4-state SysTick FSM debouncer, telemetry snapshot, PWM scaling helpers |
| `Src/main.c` | Application startup (`Clock_Init_170MHz()` / `Clock_Init_160MHz()`), `SysTick_Handler`, `USART2_IRQHandler`, non-blocking cooperative superloop |
| `Makefile` | Reproducible cross-compilation linking `23-code-library/Drivers/` & `23-code-library/templates/` (`CLOCK_PROFILE=170\|160`, `BUILD_TYPE=debug\|release`) |

---

## 3. Verified Conflict-Free Hardware Subsystems & Wiring

| Subsystem | Peripheral & Channel | MCU Pin | `AFR` / Mode | Exact Verified Configuration | Bench Status |
|---|---|---|---|---|---|
| **System Clock** | `RCC` + `PWR` + `FLASH` | Internal | `170 MHz` (opt. `160 MHz`) | `Clock_Init_170MHz()` (`M=4, N=85, R=2`, `R1MODE=0`, `4WS`, RM0440 §6.1.5 `HPRE=/2 -> >=1 us -> /1`); `-DUSE_CLOCK_160MHZ` calls `Clock_Init_160MHz()` | `READY FOR AGENT 4 ONBOARD TEST` |
| **System Tick** | `SysTick` | Internal | `1.000 kHz` (`1 ms`) | `SysTick_Config(Clock_GetSysClkHz() / 1000U)`, wraparound-safe `uint32_t` delta math | `READY FOR AGENT 4 ONBOARD TEST` |
| **User Button** | `GPIOC` (`B1`) | **`PC13`** | Input (`00b`), No Pull | **Active-HIGH** per UM2505 (external pull-down `R31`); 4-state `25 ms` SysTick FSM debouncer cycles `MODE_IDLE -> MODE_INTERNAL_TELEM -> MODE_EXT_POT_PWM` | `READY FOR AGENT 4 ONBOARD TEST` |
| **Status LED** | `GPIOA` (`LD2`) | **`PA5`** | Output (`01b`), Push-Pull | Atomic `BSRR` indicator: `MODE_IDLE` 1 Hz blink, `MODE_INTERNAL_TELEM` 2 Hz blink, `MODE_EXT_POT_PWM` solid ON | `READY FOR AGENT 4 ONBOARD TEST` |
| **Internal Analog** | `ADC1_IN18` & `ADC1_IN16` | Internal | `42.5 MHz` (`CKMODE=11b`) | `VREFINT` (`VREFINT_CAL`) + Temp Sensor (`VSENSESEL`, `125 us` startup, `SMP=640.5` cycles, `TS_CAL1`/`TS_CAL2`) | `READY FOR AGENT 4 ONBOARD TEST` |
| **Telemetry UART** | `USART2` (ST-LINK VCP) | **`PA2`** (TX), **`PA3`** (RX) | `AF7` (`10b`) | `115200 8N1` dynamically computed BRR (`1476` @ 170 MHz), SPSC `ring_buffer_t` RX parser (`'m'`, `'s'`, `'h'`) | `READY FOR AGENT 4 ONBOARD TEST` |
| **Watchdog** | `IWDG` | Internal (`32 kHz LSI`) | `1000 ms` timeout | `DBGMCU_APB1FZR1_DBG_IWDG_STOP` enabled for GDB; kicked only when ADC/subsystems report healthy | `READY FOR AGENT 4 ONBOARD TEST` |
| **External Pot** | `ADC1_IN1` | **`PA0`** (Arduino `A0`) | Analog (`11b`) | Optional `10 kΩ` potentiometer (`3V3` – `PA0` – `GND`), `SMP1 = 47.5` cycles, active in `MODE_EXT_POT_PWM` | `EXTERNAL HARDWARE PENDING` |
| **Actuator PWM** | **`TIM2_CH2`** | **`PA1`** (Arduino `A1`) | `AF1` (`10b`) | `20.000 kHz` PWM (`PSC=0, ARR=8499` @ 170 MHz; `ARR=7999` @ 160 MHz), `OC2PE + ARPE + UG`. Zero conflict with `PA0`! | `EXTERNAL HARDWARE PENDING` *(signal generated on pin; external load/scope pending)* |

---

## 4. Build Commands & Exact Compiled Footprint

Verified with **`arm-none-eabi-gcc 14.3.1`** (`GNU Tools for STM32 14.3.rel1`) using strict flags:
`-std=c11 -Wall -Wextra -Werror -Wpedantic -Wshadow -Wconversion -Wdouble-promotion -Wundef`

```bash
# Default 170 MHz debug build (-Og -g3)
make clean all

# 160 MHz clock profile (-DUSE_CLOCK_160MHZ)
make clean all CLOCK_PROFILE=160

# Release build (-O2 -g)
make clean all BUILD_TYPE=release
```

| Profile | Artifact | `text` (B) | `data` (B) | `bss` (B) | Flash Used | RAM Used | Compiler Warnings / Errors |
|---|---|---:|---:|---:|---:|---:|---|
| Debug (`-Og -g3`, `170 MHz`) | `build/smart_instrument_panel.elf` | `12420` | `92` | `2148` | `12520 B` (`2.39%`) | `2232 B` (`2.27%`) | `0` / `0` |
| Debug (`-Og -g3`, `160 MHz`) | `build/smart_instrument_panel.elf` | `12420` | `92` | `2148` | `12520 B` (`2.39%`) | `2232 B` (`2.27%`) | `0` / `0` |
| Release (`-O2 -g`, `170 MHz`) | `build/smart_instrument_panel.elf` | `11800` | `92` | `2152` | `11900 B` (`2.27%`) | `2240 B` (`2.28%`) | `0` / `0` |

---

## 5. Verification Checklist

### Onboard NUCLEO-G474RE Verification (`READY FOR AGENT 4 ONBOARD TEST` — Zero External Wires)
- [ ] Flash `build/smart_instrument_panel.bin` to `0x08000000` and open ST-LINK VCP at `115200 8N1`.
- [ ] Confirm boot banner reports `SYSCLK : 170000000 Hz`, `Mode : INTERNAL_TELEM`, plausible `VDDA` (`~3250..3350 mV`), and plausible die temperature (`~20..45 °C`).
- [ ] Confirm valid JSON telemetry lines arrive every `500 ms` and `LD2` (`PA5`) blinks at `2 Hz`.
- [ ] Send `'m'` over UART (and/or press physical `B1` on `PC13`): confirm clean mode cycling `INTERNAL_TELEM -> EXT_POT_PWM -> IDLE -> INTERNAL_TELEM` with zero double-triggering.
- [ ] Send `'s'` over UART: confirm immediate structured status dump showing `TIM2->ARR == 8499` (`20.000 kHz` @ `170 MHz`).

### External Bench Verification (`EXTERNAL HARDWARE PENDING`)
- [ ] Connect a `10 kΩ` potentiometer to `PA0` and probe `PA1` (`TIM2_CH2`) on an oscilloscope; switch to `MODE_EXT_POT_PWM` and verify `20.00 kHz` PWM duty scales linearly `0%..100%`.
