# Capstone Implementation & Cross-Compilation Report

**Date:** 2026-11-18  
**Owner:** Agent 9 — Capstone & Project Implementation  
**Target Hardware:** ST NUCLEO-G474RE (`MB1367`, `STM32G474RET6`, ARM Cortex-M4F, `512 KB` Flash, `96 KB` SRAM1+2, `32 KB` CCM-SRAM)  
**Toolchain:** `arm-none-eabi-gcc 14.3.1` (`GNU Tools for STM32 14.3.rel1.20251027-0700`), GNU Make `4.4.1`  
**Board Lock Compliance:** Physical NUCLEO-G474RE board was **NOT** touched or flashed during this wave (exclusive hardware lock held by Agent 4).

---

## 1. Executive Summary

All three capstone blueprints in `19-projects/` have been implemented as **complete, clean, reproducibly cross-compiled STM32G474RE firmware projects** reusing the repository's shared verified bare-metal drivers (`23-code-library/Drivers/`), CMSIS headers (`stm32g474xx.h`), startup/linker templates (`23-code-library/templates/`), and official pinned **`FreeRTOS-Kernel V11.1.0`** (`23-code-library/freertos/FreeRTOS-Kernel/`).

- **Compilation Status:** **3 / 3 projects (`100%`)** pass `make clean all` with **`0` errors and `0` warnings** under strict C11 diagnostics (`-std=c11 -Wall -Wextra -Werror -Wpedantic -Wshadow -Wconversion -Wdouble-promotion -Wundef`) across all project, template, and driver sources.
- **Additional Profile Verification:** Project 01 verified under both `170 MHz` default and `160 MHz` (`-DUSE_CLOCK_160MHZ`); Project 03 verified under both `POWER_SIM_MODE=1` (safe onboard simulation) and `POWER_SIM_MODE=0` (external hardware power stage); all 3 projects verified under both `BUILD_TYPE=debug` (`-Og -g3`) and `BUILD_TYPE=release` (`-O2 -g`).
- **Honest Hardware Status (Updated by Agent 4 Physical Bring-Up):** All **3 / 3 projects** are now **`ONBOARD HARDWARE TESTED` (`PASS`)** on physical **NUCLEO-G474RE** (`MB1367 Rev C`, `COM8`, `115200 8N1`) in their safe zero-external-wire modes — **after Agent 4 caught and fixed a genuine real-silicon defect** (missing `GPIO_EnableClock()` calls in all 3 capstones' `Src/main.c` that initially silenced `PA2` `USART2_TX` on hardware!). External bench peripherals (potentiometer, PWM load, SSD1306 I2C1 OLED, external buck/boost power stage) remain explicitly labeled **`EXTERNAL HARDWARE PENDING`** (see [`HARDWARE_VALIDATION/PENDING_EXTERNAL_CONNECTIONS.md`](../HARDWARE_VALIDATION/PENDING_EXTERNAL_CONNECTIONS.md)).

---

## 2. Exact Compiled Footprints (`arm-none-eabi-size` & `--print-memory-usage`)

| Project | Target ELF | Build Profile | `text` (B) | `data` (B) | `bss` (B) | Flash Used (512 KB) | RAM Used (96 KB) | Strict Warnings / Errors |
|---|---|---|---:|---:|---:|---:|---:|---|
| **01** (`01-beginner-smart-instrument-panel`) | `build/smart_instrument_panel.elf` | Debug (`-Og -g3`, `170 MHz`) | `12420` | `92` | `2148` | `12520 B` (`2.39%`) | `2232 B` (`2.27%`) | `0` / `0` |
| **01** (`01-beginner-smart-instrument-panel`) | `build/smart_instrument_panel.elf` | Debug (`-Og -g3`, `160 MHz`) | `12420` | `92` | `2148` | `12520 B` (`2.39%`) | `2232 B` (`2.27%`) | `0` / `0` |
| **01** (`01-beginner-smart-instrument-panel`) | `build/smart_instrument_panel.elf` | Release (`-O2 -g`, `170 MHz`) | `11800` | `92` | `2152` | `11900 B` (`2.27%`) | `2240 B` (`2.28%`) | `0` / `0` |
| **02** (`02-intermediate-dma-data-logger`) | `build/dma_data_logger.elf` | Debug (`-Og -g3`, `170 MHz`) | `11488` | `92` | `2208` | `11588 B` (`2.21%`) | `2296 B` (`2.34%`) | `0` / `0` |
| **02** (`02-intermediate-dma-data-logger`) | `build/dma_data_logger.elf` | Release (`-O2 -g`, `170 MHz`) | `10388` | `92` | `2208` | `10488 B` (`2.00%`) | `2296 B` (`2.34%`) | `0` / `0` |
| **03** (`03-advanced-freertos-digital-power-controller`) | `build/freertos_digital_power_ctrl.elf` | Debug (`-Og -g3`, `POWER_SIM_MODE=1`) | `21156` | `100` | `31460` | `21264 B` (`4.06%`) | `31552 B` (`32.10%`) | `0` / `0` |
| **03** (`03-advanced-freertos-digital-power-controller`) | `build/freertos_digital_power_ctrl.elf` | Release (`-O2 -g`, `POWER_SIM_MODE=1`) | `20656` | `100` | `31456` | `20764 B` (`3.96%`) | `31552 B` (`32.10%`) | `0` / `0` |

*(Note: Project 03 RAM usage includes the `20 KB` `heap_4.c` FreeRTOS heap plus statically allocated TCBs and stacks for Idle, Timer, and all 4 application tasks. Every project's default `build/` directory currently holds the fresh `debug` `.elf`, `.bin`, `.hex`, and `.map` artifacts ready for Agent 4 flashing.)*

---

## 3. Per-Project Engineering & Silicon Compliance Audit

### Project 01 — `19-projects/01-beginner-smart-instrument-panel/`
- **Files Created/Updated:** `Inc/app_panel.h`, `Src/main.c`, `Makefile`, `README.md`
- **Key Verified Details:**
  - Dual clock profile support: `Clock_Init_170MHz()` default + compile-time `-DUSE_CLOCK_160MHZ` (`make CLOCK_PROFILE=160`) calling `Clock_Init_160MHz()`.
  - Non-blocking 4-state SysTick FSM debouncer (`25 ms`) on Active-HIGH `PC13` (`B1`, UM2505 external pull-down `R31`) cycling `MODE_IDLE`, `MODE_INTERNAL_TELEM` (default zero-wire boot mode), and `MODE_EXT_POT_PWM`.
  - Conflict-free pin assignment: `ADC1_IN1` on `PA0` (optional external pot), `TIM2_CH2` `20.000 kHz` PWM on `PA1` (`AF1`), `USART2` on `PA2`/`PA3` (`AF7`, `115200 8N1`), `LD2` on `PA5`.
  - Non-blocking RX command parser (`'m'` mode cycle, `'s'` status dump, `'h'` help) consuming lock-free C11 SPSC `ring_buffer_t` populated in `USART2_IRQHandler`.
  - `IWDG` (`1000 ms`, `DBG_IWDG_STOP = 1`) + `FaultHandler_EnableDetailedFaults(true)`.

### Project 02 — `19-projects/02-intermediate-dma-data-logger/`
- **Files Created/Updated:** `Inc/logger_packet.h`, `Src/main.c`, `Makefile`, `README.md`
- **Key Verified Details:**
  - `170 MHz` Boost Mode + exact `100.000 Hz` `TIM3` TRGO (`PSC=1699`, `ARR=999`, `TIM3->CR2 = TIM_CR2_MMS_1`).
  - `ADC1` (`CKMODE=11b` = `42.5 MHz <= 60 MHz`) hardware-triggered on rising edge of `TIM3_TRGO` (`EXTSEL = 4` verified against RM0440 Table 163 / `LL_ADC_REG_TRIG_EXT_TIM3_TRGO`, `EXTEN = 01b`), scanning `VREFINT` (`Ch18`) + Internal Temp Sensor (`Ch16`) with `SMP16 = SMP18 = 640.5` cycles (`15.07 us >= 5.0 us` DS12288 min) and `150 us` `t_START` stabilization delay.
  - `DMA1_Channel1` (`DMAMUX1_Channel0`, `DMAMUX_REQ_ADC1 = 5`) circular ping-pong into `volatile uint16_t g_adc_ping_pong[64]` (`HTIE | TCIE | TEIE`); ISR clears ONLY individual `CHTIF1`/`CTCIF1`/`CTEIF1` flags via `dma_clear_flags()` (strictly avoiding `CGIF1` per ES0430 §2.3.1!) and hands off halves across `__DMB()`.
  - Hardware IEEE 802.3 `CRC-32` (`CRC` @ `0x40023000`, `POL = 0x04C11DB7`, `INIT = 0xFFFFFFFF`, `REV_IN = 01`, `REV_OUT = 1`, final `^ 0xFFFFFFFF`) verified at boot against `"123456789" -> 0xCBF43926` and applied to every 32-byte `logger_frame_t` (`_Static_assert(sizeof(logger_frame_t) == 32U && (sizeof(logger_frame_t) % 8U) == 0U)`).
  - Optional `I2C1` SSD1306 OLED hook routed to `PA15`/`PB7` (`AF4`, avoiding the `PB8-BOOT0` bootloader trap!) using `i2c_g4.c` bounded `NACKF`/timeout detection so absent display never blocks logging.
  - Read-only `FLASH->OPTR.DBANK` check + 64-bit aligned Bank 2 (`0x08040000`) dry-run RAM staging helper (`memcpy`, zero option-byte writes!).

### Project 03 — `19-projects/03-advanced-freertos-digital-power-controller/`
- **Files Created/Updated:** `Inc/FreeRTOSConfig.h`, `Inc/power_ctrl.h`, `Src/main.c`, `Makefile`, `README.md`
- **Key Verified Details:**
  - Links official pinned `23-code-library/freertos/FreeRTOS-Kernel/` (`V11.1.0`, `GCC/ARM_CM4F`, `heap_4.c`).
  - Symbol verification (`arm-none-eabi-nm -n build/freertos_digital_power_ctrl.elf`) confirmed all required vectors and task entry points are linked:
    - `SVC_Handler` (`0x08003da0`), `PendSV_Handler` (`0x08003df0`), `SysTick_Handler` (`0x08003f44`)
    - `TIM1_BRK_TIM15_IRQHandler` (`0x08000d6c`), `vTaskStartScheduler` (`0x0800229c`)
    - `vSafetySupervisorTask` (`0x08000c10`), `vControlLoopTask` (`0x080007d0`), `vTelemetryTask` (`0x08000b40`), `vUiTask` (`0x08000a44`)
  - Mandatory `NVIC_SetPriorityGrouping(0U)` called before scheduler start; `TIM1_BRK_TIM15_IRQn` set to NVIC Priority `5` (`>= configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY = 5`) so `xTaskNotifyFromISR()` is legally callable.
  - `TIM1` `50.000 kHz` center-aligned complementary PWM (`PA8` AF6 `TIM1_CH1`, `PB13` AF6 `TIM1_CH1N`, `ARR=1700`, `BDTR.DTG=85` = `500.0 ns` dead-time) protected asynchronously in silicon by `DAC1_CH1` (`HFSEL = 10b` @ `170 MHz`) + `COMP1` $\to$ `TIM1->AF1 = TIM1_AF1_BKCMP1E` (`BKE | BKP`, `AOE = 0` latched safe OFF!).
  - 4 static FreeRTOS tasks strictly above `tskIDLE_PRIORITY` (`0`): `SafetySupervisorTask` (Prio `4`, conditional `IWDG` feed + `DBG_IWDG_STOP`), `ControlLoopTask` (Prio `3`, `1 kHz` fixed-point Q15 PI regulator with anti-windup clamping), `TelemetryTask` (Prio `2`, `512`-word stack, priority-inheritance `g_uart_mutex` + live stack HWM reporting), `UiTask` (Prio `1`, `10 Hz` `B1` setpoint/fault-reset/software `TIM1->EGR=BG` break test + optional NACKF-safe `I2C1` OLED).

---

## 4. Physical NUCLEO-G474RE Onboard Hardware Verification Results (Agent 4)

**Status:** **3 / 3 Capstones `ONBOARD HARDWARE TESTED` (`PASS` on safe onboard modes; external peripherals remain `EXTERNAL HARDWARE PENDING`)**  
**Silicon Defect Caught & Fixed During Bring-Up:** Initial flashing of Project 01 produced `0` bytes on `COM8` because all three capstones called `GPIO_Config*()` without calling `GPIO_EnableClock(GPIOA/GPIOB/GPIOC)` first (leaving `RCC->AHB2ENR` port clocks disabled). Explicit `GPIO_EnableClock()` calls were added to `01/Src/main.c`, `02/Src/main.c`, and `03/Src/main.c`, rebuilt cleanly (`0` warnings / `0` errors), and verified live on `COM8` (`115200 8N1`):

### 4.1 Project 01 (`smart_instrument_panel.elf`) — Live `COM8` Capture + Interactive `'s'` Command
- Flashed (`12.34 KB`, verified), sent `'s'` at `t = 1.8 s` -> immediately parsed via `USART2_IRQHandler` + SPSC `ring_buffer_t` (`rx_cmds: 0 -> 1`):
```text
--- [PANEL STATUS] ---
  SYSCLK      : 170000000 Hz
  Mode        : INTERNAL_TELEM (B1 or UART 'm')
  VDDA        : 3288 mV (VREFINT)
  Die Temp    : 33.2 C (TS_CAL1/2)
  Ext Pot PA0 : raw=0 (0 mV) [EXT PENDING]
  PWM PA1 AF1 : 33.8% @ 20000 Hz (ARR=8499)
  B1 Presses  : 0 | RX Cmds: 1 | ADC Err: 0
  Prev Reset  : IWDG=0 | HardFaultLog=0
----------------------
{"tick_ms":2027,"clk_mhz":170,"mode":"INTERNAL_TELEM","vdda_mv":3286,"temp_c":32.7,"pot_raw":0,"pot_mv":0,"pwm_permille":328,"rx_cmds":1}
{"tick_ms":4027,"clk_mhz":170,"mode":"INTERNAL_TELEM","vdda_mv":3286,"temp_c":33.0,"pot_raw":0,"pot_mv":0,"pwm_permille":334,"rx_cmds":1}
```

### 4.2 Project 02 (`dma_data_logger.elf`) — Live `COM8` Capture (4.0 s, 25 Ping-Pong Frames)
- Flashed (`11.43 KB`, verified), exact `160 ms` frame cadence (`ovr=0`, `te=0`, `ok=1`), hardware `CRC-32 = 0xCBF43926`, clean absent-OLED `NACKF`:
```text
[BOOT] SYSCLK=170000000 Hz | CRC32_TEST=0xCBF43926 (PASS)
[BOOT] FLASH_OPTR.DBANK=1 (dry-run Bank 2 0x08040000 staging)
[BOOT] I2C1 OLED (PA15/PB7 0x3C): ABSENT (EXTERNAL HARDWARE PENDING)
$LOG2,seq=0,t_ms=192,half=0,vdda_mv=3286,temp=32.3C,raw=[954,954,956],ovr=0,te=0,b2=0x08040000,crc32=0xDBB3E973,ok=1
$LOG2,seq=1,t_ms=352,half=1,vdda_mv=3288,temp=32.5C,raw=[954,954,956],ovr=0,te=0,b2=0x08040020,crc32=0xDD6FFABA,ok=1
... (25 consecutive frames @ exact 160 ms cadence) ...
$LOG2,seq=24,t_ms=4032,half=0,vdda_mv=3286,temp=33.0C,raw=[956,956,957],ovr=0,te=0,b2=0x08040300,crc32=0x3CCCB8CF,ok=1
```

### 4.3 Project 03 (`freertos_digital_power_ctrl.elf`) — Live `COM8` Capture (5.2 s, FreeRTOS V11.1.0)
- Flashed (`20.89 KB`, verified), 4 static tasks running cleanly at `170 MHz`, Q15 PI regulator tracking `5000 mV` (`4989..5021 mV`), `TIM1`/`COMP1` armed (`moe:1, fault:0`), `wdg: 1 -> 19`, steady stack HWM:
```text
  Clock : 170 MHz Boost | PRIGROUP=0 | FreeRTOS V11.1.0
  PWM   : TIM1 PA8/PB13 50 kHz Complementary, 500 ns dead-time
  Break : COMP1 -> TIM1->AF1.BKCMP1E (<50 ns MOE trip, AOE=0)
  Mode  : ONBOARD SAFE SIMULATION (Ext Power Stage PENDING)
  OLED  : ABSENT (EXTERNAL HARDWARE PENDING)
{"t_ms":502,"sim":1,"vref_mv":5000,"vout_mv":5020,"duty_pm":209,"ccr1":355,"moe":1,"fault":0,"brk":0,"wdg":1,"hwm":[217,337,398,351]}
{"t_ms":2002,"sim":1,"vref_mv":5000,"vout_mv":4989,"duty_pm":209,"ccr1":355,"moe":1,"fault":0,"brk":0,"wdg":7,"hwm":[217,337,325,351]}
{"t_ms":5002,"sim":1,"vref_mv":5000,"vout_mv":5021,"duty_pm":209,"ccr1":355,"moe":1,"fault":0,"brk":0,"wdg":19,"hwm":[217,337,325,351]}
```

Full verbatim captures are archived in [`AUDIT/HARDWARE_VALIDATION_SUMMARY.md`](HARDWARE_VALIDATION_SUMMARY.md); external peripheral bring-up procedures are listed in [`HARDWARE_VALIDATION/PENDING_EXTERNAL_CONNECTIONS.md`](../HARDWARE_VALIDATION/PENDING_EXTERNAL_CONNECTIONS.md).
