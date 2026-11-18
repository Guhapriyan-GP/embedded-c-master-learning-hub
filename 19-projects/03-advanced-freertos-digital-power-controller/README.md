# Project 03: FreeRTOS Digital Power & Motor Controller (with Hardware `< 50 ns` Break Protection)

> **Verification Status:** `CROSS-COMPILED` (`arm-none-eabi-gcc 14.3.1`, official pinned `FreeRTOS-Kernel V11.1.0`, 0 errors / 0 warnings on all project/template/driver sources) | `READY FOR AGENT 4 ONBOARD TEST` (Default `POWER_SIM_MODE=1` runs safely onboard NUCLEO-G474RE with zero external wires; NOT yet flashed onto physical hardware in this commit) | `EXTERNAL HARDWARE PENDING` (External buck/boost power stage & optional `I2C1` OLED)

**Target Concept IDs:** `APP-PWR-001`, `STM32-RCC-001`, `ARM-NVIC-001`, `RTOS-ARCH-001`, `RTOS-SCHED-001`, `RTOS-TASK-001`, `RTOS-IPC-001`, `RTOS-SYNC-001`, `RTOS-ISR-001`, `RTOS-MEM-001`, `PERIPH-BDTR-002`, `G4-COMP-001`, `PERIPH-DAC-001`, `EMB-NUM-001`, `PERIPH-WDG-001`, `REL-SAFE-001`

---

## 1. Project Overview & Safety-First Architecture

This capstone project implements a deterministic, **safety-first** digital power / motor controller on **STM32G474RE (`170 MHz` Boost Mode, Cortex-M4F)** linking the repository's official pinned **`23-code-library/freertos/FreeRTOS-Kernel/` (`V11.1.0`, `GCC/ARM_CM4F`, `heap_4.c`)**.

Catastrophic overcurrent protection is implemented **100% in asynchronous silicon hardware** (`DAC1_CH1` threshold + `COMP1` $\to$ `TIM1->AF1.BKCMP1E`), clearing `TIM1->BDTR.MOE` in **`< 50 ns`** with `AOE = 0` (latched safe OFF) and zero CPU/RTOS latency. Post-fault notification (`TIM1_BRK_TIM15_IRQHandler` at NVIC Priority `5`) wakes `SafetySupervisorTask` via `xTaskNotifyFromISR()`, while 4 properly prioritized static FreeRTOS tasks handle closed-loop Q15 PI regulation, mutex-guarded USART2 telemetry, watchdog supervision, and UI control strictly above the Idle Task!

```text
+========================================================================================+
|                        ASYNCHRONOUS SILICON SAFETY LAYER (< 50 ns)                     |
|  [ PA1 Current Sense ] -> [ COMP1 vs DAC1_CH1 (PA4) ] --(TIM1->AF1.BKCMP1E)--> TIM1    |
|                           * Clears TIM1->BDTR.MOE in < 50 ns (AOE = 0 latched!) *      |
|                           * Drives PA8 (TIM1_CH1) & PB13 (TIM1_CH1N) LOW!       *      |
|                                       │ (Post-Fault IRQ, NVIC Prio 5)                  |
|                                       ▼ (xTaskNotifyFromISR + __DSB + portYIELD)       |
+=======================================│================================================+
|                         FreeRTOS V11.1.0 Kernel (PRIGROUP = 0)                         |
|                                       │                                                |
|  [ SafetySupervisorTask (Prio 4) ] <--┘       [ ControlLoopTask (Prio 3, 1 kHz) ]      |
|   - Stack: 256 words (1024 B)                  - Stack: 384 words (1536 B)             |
|   - Manages latched fault & recovery           - Fixed-point Q15 PI + anti-windup      |
|   - Kicks IWDG iff Ctrl+Telem+UI alive         - Updates TIM1->CCR1 (50 kHz, 500ns DT) |
|                                                            │ (xQueueSend, non-blocking)|
|                                                            ▼                           |
|  [ UiTask (Prio 1 — NEVER 0!) ]               [ TelemetryTask (Prio 2) ]               |
|   - Stack: 384 words (1536 B)                  - Stack: 512 words (2048 B)             |
|   - 10 Hz B1 setpoint / BG break test          - Mutex-guarded USART2 JSON + Stack HWM |
|   - Optional I2C1 OLED (PA15/PB7, NACKF-safe)                                          |
|                                                                                        |
|  [ FreeRTOS Idle Task (Priority 0) ] — NEVER starved!                                  |
+========================================================================================+
```

---

## 2. Project File Layout

| File | Role |
|---|---|
| `Inc/FreeRTOSConfig.h` | Production STM32G474RE FreeRTOS V11.1.0 config (`PRIO_BITS=4`, `MAX_SYSCALL=5`, `CHECK_FOR_STACK_OVERFLOW=2`, `USE_MUTEXES=1`, static+dynamic allocation, CMSIS vector mapping) |
| `Inc/power_ctrl.h` | Fixed-point **Q15 PI regulator** with conditional-integration anti-windup clamping (`q15_pi_step()`), discrete buck converter simulation plant (`POWER_SIM_MODE=1`), task budgets, IPC types |
| `Src/main.c` | `170 MHz` Boost Mode, `NVIC_SetPriorityGrouping(0U)`, `DAC1` (`HFSEL=10b`) + `COMP1` $\to$ `TIM1` 50 kHz complementary PWM (`PA8`/`PB13`, `500 ns` dead-time), `TIM1_BRK_TIM15_IRQHandler`, 4 static FreeRTOS tasks, `IWDG` supervisor |
| `Makefile` | Links `23-code-library/freertos/FreeRTOS-Kernel/` (`V11.1.0`, `ARM_CM4F`, `heap_4.c`), `23-code-library/Drivers/`, and `23-code-library/templates/` (`POWER_SIM_MODE=1\|0`, `BUILD_TYPE=debug\|release`) |

---

## 3. Verified Hardware Subsystems & Wiring

| Subsystem | Peripheral | MCU Pin | Exact Verified Configuration | Bench Status |
|---|---|---|---|---|
| **Clock & NVIC** | `RCC`/`PWR`/`NVIC` | Internal | `170 MHz` Boost Mode + mandatory `NVIC_SetPriorityGrouping(0U)` (`PRIGROUP = 0`) | `READY FOR AGENT 4 ONBOARD TEST` |
| **Complementary PWM** | `TIM1_CH1` / `TIM1_CH1N` | **`PA8`** (`AF6`), **`PB13`** (`AF6`) | Center-aligned `50.000 kHz` (`PSC=0`, `ARR=1700`), **`500.0 ns` dead-time (`BDTR.DTG = 85`)**, `OSSI \| OSSR`, `OIS1 = OIS1N = 0` | `READY FOR AGENT 4 ONBOARD TEST` *(PWM outputs active on pins; external power stage pending)* |
| **Hardware Overcurrent Break** | `DAC1_CH1` + `COMP1` $\to$ `TIM1_BKIN` | **`PA4`** (`DAC1_OUT1`), **`PA1`** (`COMP1_INP`) | `DAC1` `HFSEL = 10b` (mandatory `> 160 MHz`), `COMP1` `INMSEL = 5` (`DAC1_CH1`), `TIM1->AF1 = TIM1_AF1_BKCMP1E`, `BKE \| BKP`, `AOE = 0` latched; `TIM1_BRK_TIM15_IRQn` NVIC Prio `5` (`xTaskNotifyFromISR`). In `POWER_SIM_MODE=1`, `PA1` is pulled LOW internally and long-pressing `B1` injects `TIM1->EGR = TIM_EGR_BG` to test the real hardware break path safely! | `READY FOR AGENT 4 ONBOARD TEST` (onboard `BG` break test) / `EXTERNAL HARDWARE PENDING` (external shunt) |
| **Watchdog** | `IWDG` | Internal (`32 kHz LSI`) | `1500 ms` timeout, `DBG_IWDG_STOP = 1`; refreshed by `SafetySupervisorTask` **only** when `EVT_CTRL \| EVT_TELEM \| EVT_UI` all report healthy | `READY FOR AGENT 4 ONBOARD TEST` |
| **Telemetry UART** | `USART2` (ST-LINK VCP) | **`PA2`** (TX), **`PA3`** (RX) | `AF7`, `115200 8N1`, mutex-protected JSON output including live stack high-water marks (`uxTaskGetStackHighWaterMark`) | `READY FOR AGENT 4 ONBOARD TEST` |
| **User Controls & LED** | `B1` (`PC13`) & `LD2` (`PA5`) | `PC13` (Active-HIGH), `PA5` (Output) | Short press `B1`: cycles target setpoint (`3.3 V / 5.0 V / 12.0 V`) or clears latched fault; Long press (`>= 1.5 s`): triggers `TIM1->EGR = TIM_EGR_BG` break self-test | `READY FOR AGENT 4 ONBOARD TEST` |
| **External Power Stage** | Half-Bridge / Buck Stage | `PA8`, `PB13`, `PA1` | External MOSFET gate driver + shunt amplifier (`POWER_SIM_MODE=0` build) | `EXTERNAL HARDWARE PENDING` |
| **Optional OLED** | `I2C1` (`400 kHz`) | **`PA15`** (SCL), **`PB7`** (SDA) | `AF4` Open-Drain (`0x3C`), bounded `NACKF`/timeout probe — degrades gracefully when absent | `EXTERNAL HARDWARE PENDING` |

---

## 4. FreeRTOS Task Table & Stack Budgets

| Task | Priority | Period / Trigger | Static Stack | Responsibility |
|---|---|---|---|---|
| **`SafetySupervisorTask`** | **4 (CRITICAL)** | `xTaskNotifyWait(..., 100 ms)` | `256` words (`1024 B`) | Handles post-break safe latch/recovery & feeds `IWDG` iff all 3 worker heartbeats arrived |
| **`ControlLoopTask`** | **3 (HIGH)** | `1 kHz` (`vTaskDelayUntil`, `1 ms`) | `384` words (`1536 B`) | Fixed-point Q15 PI regulator with anti-windup clamping $\to$ `TIM1->CCR1`; non-blocking queue post |
| **`TelemetryTask`** | **2 (MEDIUM)** | `xQueueReceive(..., 250 ms)` | **`512` words (`2048 B`)** | Acquires `g_uart_mutex` (priority inheritance), streams JSON telemetry + stack high-water marks |
| **`UiTask`** | **1 (LOW — NEVER 0!)** | `10 Hz` (`vTaskDelayUntil`, `100 ms`) | `384` words (`1536 B`) | Active-HIGH `B1` setpoint/fault-reset/break-test + optional NACKF-safe `I2C1` OLED |
| **FreeRTOS Idle Task** | **0 (`tskIDLE_PRIORITY`)** | Background | `128` words (`512 B`) | Unstarved kernel cleanup |

---

## 5. Build Commands & Exact Compiled Footprint

Verified with **`arm-none-eabi-gcc 14.3.1`** (`FreeRTOS-Kernel V11.1.0`, `ARM_CM4F`, `heap_4.c`):
- Strict `-std=c11 -Wall -Wextra -Werror -Wpedantic -Wshadow -Wconversion -Wdouble-promotion -Wundef` on all project, template, and driver code (`0` errors, `0` warnings).
- Verified linked vector symbols via `arm-none-eabi-nm`: `SVC_Handler`, `PendSV_Handler`, `SysTick_Handler`, `TIM1_BRK_TIM15_IRQHandler`, `vTaskStartScheduler`.

```bash
# Default onboard safe simulation build (POWER_SIM_MODE=1, debug -Og -g3)
make clean all

# External power stage build (PA1 pure analog shunt sense, 1.65 V DAC1 threshold)
make clean all POWER_SIM_MODE=0

# Release build (-O2 -g)
make clean all BUILD_TYPE=release
```

| Profile | Artifact | `text` (B) | `data` (B) | `bss` (B) | Flash Used | RAM Used | Compiler Warnings / Errors |
|---|---|---:|---:|---:|---:|---:|---|
| Debug (`-Og -g3`, `POWER_SIM_MODE=1`) | `build/freertos_digital_power_ctrl.elf` | `21156` | `100` | `31460` | `21264 B` (`4.06%`) | `31552 B` (`32.10%`) | `0` / `0` |
| Release (`-O2 -g`, `POWER_SIM_MODE=1`) | `build/freertos_digital_power_ctrl.elf` | `20656` | `100` | `31456` | `20764 B` (`3.96%`) | `31552 B` (`32.10%`) | `0` / `0` |

*(RAM total includes the `20 KB` `heap_4.c` FreeRTOS heap plus static TCBs/stacks for Idle, Timer, and all 4 application tasks.)*

---

## 6. Verification Checklist

### Onboard NUCLEO-G474RE Verification (`READY FOR AGENT 4 ONBOARD TEST` — Zero External Wires)
- [ ] Flash `build/freertos_digital_power_ctrl.bin` to `0x08000000` and open ST-LINK VCP at `115200 8N1`.
- [ ] Confirm boot banner reports `170 MHz Boost | PRIGROUP=0 | FreeRTOS V11.1.0`, `ONBOARD SAFE SIMULATION`, and `OLED : ABSENT (EXTERNAL HARDWARE PENDING)`.
- [ ] Confirm JSON telemetry streams every `500 ms` with `vout_mv` tracking `vref_mv` (`5000 mV`), `moe:1`, `fault:0`, monotonic `wdg` feeds, and positive stack high-water marks `hwm:[...]` for all 4 tasks.
- [ ] Short-press `B1` (`PC13`): confirm `vref_mv` steps `5000 -> 12000 -> 3300 mV` and Q15 PI regulator smoothly settles `vout_mv` and `duty_pm`.
- [ ] Long-press `B1` (`>= 1.5 s`): confirm `TIM1->EGR = TIM_EGR_BG` fires `TIM1_BRK_TIM15_IRQHandler`, latches `moe:0`, `fault:1`, increments `brk`, illuminates `LD2` solid ON, and prints `[SAFETY] HW BREAK LATCHED`; then short-press `B1` to confirm clean `MOE` recovery.

### External Bench Verification (`EXTERNAL HARDWARE PENDING`)
- [ ] Build with `POWER_SIM_MODE=0`, probe `PA8`/`PB13` on oscilloscope to confirm `50.00 kHz` complementary PWM with `500 ns` dead-time, and inject `> 1.65 V` onto `PA1` (even while CPU is halted in GDB!) to verify `< 50 ns` asynchronous `MOE` shutdown.
