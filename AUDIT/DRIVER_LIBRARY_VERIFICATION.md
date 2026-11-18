# STM32G474RE Bare-Metal Driver Library Verification Matrix

**Repository:** `embedded-c-master-learning-hub` (`feature/complete-remediation`)  
**Driver Directory:** `23-code-library/Drivers/`  
**Target MCU / Board:** `STM32G474RET6` (LQFP64, Cortex-M4F @ 170 MHz / 160 MHz) on `NUCLEO-G474RE` (`MB1367 Rev C`)  
**CMSIS Ground Truth:** `23-code-library/Drivers/CMSIS/Device/ST/STM32G4xx/Include/stm32g474xx.h`  
**ST/ARM Specifications Checked:** RM0440, DS12288, ES0430, PM0214, UM2505, NXP UM10204

> **Honest Verification Boundary:**
> - **Cross-Compile Verified (`arm-none-eabi-gcc 14.3.1`):** All **12 drivers** compile with **0 warnings, 0 errors** in both `BUILD_TYPE=debug` (`-Og -g3`) and `BUILD_TYPE=release` (`-O2 -g`) under `-std=c11 -Wall -Wextra -Werror -Wpedantic -Wshadow -Wconversion -Wdouble-promotion -Wundef`, and link cleanly both with `--gc-sections` and without `--gc-sections`.
> - **Pure Calculation Helpers Host-Verified (`gcc 15.2.0`):** All pure helper functions compile cleanly under `-DDRIVER_HOST_TEST` and pass unit assertions on host x86_64.
> - **Physical NUCLEO-G474RE Hardware Status (Agent 4, `MB1367 Rev C`, `COM8`, `VDD = 3.28 V`):**
>   - **10 / 12 Drivers `ONBOARD HARDWARE TESTED` (`PASS`):** `clock_g4` (both `160 MHz` & `170 MHz` Boost + `DWT->CYCCNT`), `gpio_g4`, `ring_buffer` (live UART RX `'s'` cmd), `fault_handler` (live `.noinit` `g_crash_log` capture verified via SWD SRAM read + automated `DIV0` reboot persistence), `exti_g4` (`EXTI13` `SWIER1` callback), `tim_g4` (`TIM2` 100 Hz IRQ + `20 kHz` PWM + `TIM3` TRGO), `usart_g4` (`115200 8N1` TX & RX IRQ at both `160`/`170 MHz`), `adc_g4` (`VREFINT` `VDDA=3288 mV` + calibrated `Die Temp=32.91 C`), `dma_g4` (`MEM2MEM` 256/256 words + `ADC1->DMA1_Ch1` circular `HT`/`TC`), and `iwdg_g4` (`2000 ms`, `DBG_IWDG_STOP=1`, clean kicks).
>   - **1 / 12 Driver Partially Tested Onboard (`i2c_g4`):** Bounded `NACKF`/absent-slave detection verified live on silicon in Capstone Projects 02 & 03 (`PA15`/`PB7`, OLED absent without hang); live external slave ACK, Repeated START `i2c_read_reg()`, and `i2c_recover_bus_9clks()` remain **`EXTERNAL HARDWARE PENDING`**.
>   - **1 / 12 Driver `EXTERNAL HARDWARE PENDING` (`spi_g4`):** Cross-compiled + host-tested; physical `PB4 <-> PB5` loopback / external 3.3 V SPI slave check queued in [`HARDWARE_VALIDATION/PENDING_EXTERNAL_CONNECTIONS.md`](../HARDWARE_VALIDATION/PENDING_EXTERNAL_CONNECTIONS.md).
>   - Full verbatim logs and register readbacks: [`AUDIT/HARDWARE_VALIDATION_SUMMARY.md`](HARDWARE_VALIDATION_SUMMARY.md).

---

## 1. Master 12-Driver Inventory & API Matrix

| # | Driver Files | Status | Concept IDs | Public APIs | Error / Timeout Model | Max Static Stack (`.su`) |
|---|---|---|---|---|---|---|
| 1 | `clock_g4.h` / `.c` | Enhanced | `STM32-RCC-001`, `STM32-PWR-001`, `STM32-FLASH-001`, `TOOL-DBG-003` | `Clock_Init_170MHz()`, `Clock_Init_160MHz()`, `Clock_Init_170MHz_HSE24()`, `Clock_GetSysClkHz()`, `Clock_GetProfile()`, `Clock_DWT_Init()`, `Clock_GetCycles()`, `Clock_DelayUs()`, `Clock_DelayMs()` | `bool` return; bounded `CLOCK_READY_TIMEOUT` (1,000,000 iters) on `VOSF`, `FLASH_ACR`, `HSIRDY`, `HSERDY`, `PLLRDY`, `SWS` | 16 B (`Clock_DelayMs`) |
| 2 | `gpio_g4.h` / `.c` | Enhanced | `PERIPH-GPIO-001`, `PERIPH-GPIO-002`, `PERIPH-GPIO-003`, `PERIPH-GPIO-005`, `EMB-MMIO-001` | `GPIO_EnableClock()`, `GPIO_ConfigPin()`, `GPIO_ConfigInput()`, `GPIO_ConfigOutput()`, `GPIO_ConfigAF()`, `GPIO_ConfigAnalog()`, `GPIO_LockPin()`, `GPIO_SetPin()`, `GPIO_ResetPin()`, `GPIO_WritePin()`, `GPIO_TogglePin()`, `GPIO_ReadPin()` | `bool` return; NULL / pin `>15` / AF `>15` validation; branchless atomic `BSRR` toggle | 16 B (`GPIO_ConfigPin`) |
| 3 | `ring_buffer.h` / `.c` | Preserved | `EMB-CONC-001`, `C-MEM-001`, `FW-ARCH-001` | `rb_init()`, `rb_reset()`, `rb_count()`, `rb_free()`, `rb_is_empty()`, `rb_is_full()`, `rb_push()`, `rb_pop()`, `rb_peek()`, `rb_write()`, `rb_read()` | `bool` / count returns; power-of-2 validation; C11 `<stdatomic.h>` `acquire`/`release` SPSC (zero UB, zero malloc) | 20 B (`rb_write`) |
| 4 | `fault_handler.h` / `.c` | Preserved | `ARM-EXCP-001`, `TOOL-DBG-002`, `REL-FAULT-001` | `FaultHandler_EnableDetailedFaults()`, `FaultHandler_HasValidCrashLog()`, `FaultHandler_ClearCrashLog()`, `HardFault_Handler_C()` | `.noinit` `g_crash_log`; SRAM1+SRAM2 (`0x20000000..0x20017FFF`) & CCM-SRAM (`0x10000000..0x10007FFF`) stacked-SP bounds check prevents secondary BusFault lockup | 8 B (`HardFault_Handler_C`) |
| 5 | `exti_g4.h` / `.c` | **NEW** | `PERIPH-EXTI-001`, `PERIPH-EXTI-002`, `PERIPH-EXTI-003`, `PERIPH-SYSCFG-001`, `ARM-NVIC-001` | `exti_calc_exticr_index_shift()`, `EXTI_ConfigLine()`, `EXTI_DisableLine()`, `EXTI_RegisterCallback()`, `EXTI_TriggerSoftware()`, `EXTI_ClearPending()`, `EXTI_DispatchRange()` | `exti_status_t` (`EXTI_OK`, `EXTI_ERR_INVALID_ARG`); direct `EXTI->PR1 = mask; __DSB();` (ES0430 §2.1.3) | 32 B (`EXTI_DispatchRange`) |
| 6 | `tim_g4.h` / `.c` | **NEW** | `PERIPH-TIM-001`, `PERIPH-TIM-002`, `PERIPH-PWM-001` | `tim_calc_psc_arr()`, `tim_calc_pwm_ccr()`, `TIM_GetKernelClockHz()`, `TIM_InitPeriodic()`, `TIM_InitPwm()`, `TIM_SetPwmDutyPermille()`, `TIM_SetPwmCompare()`, `TIM_ClearUpdateFlag()` | `tim_status_t` (`TIM_OK`, `INVALID_ARG`, `UNSUPPORTED_TIM`, `FREQ_OUT_OF_RANGE`); `ARPE`/`OCxPE` + `EGR=UG`; `rc_w0` `SR = ~UIF` | 40 B (`tim_calc_psc_arr`) |
| 7 | `usart_g4.h` / `.c` | **NEW** | `PERIPH-UART-001`, `PERIPH-UART-002`, `PERIPH-UART-004` | `usart_calc_brr()`, `lpuart_calc_brr()`, `usart_init()`, `usart_write_byte()`, `usart_write_buf()`, `usart_write_str()`, `usart_flush_tx()`, `usart_clear_errors()`, `usart_rx_irq_handler()` | `usart_status_t` (`USART_OK`, `INVALID_ARG`, `BAUD_RANGE`, `TIMEOUT`); bounded `TXE`/`TC` waits; `ORE`/`FE`/`NE`/`PE` cleared via `ICR` | 24 B (`usart_init`) |
| 8 | `spi_g4.h` / `.c` | **NEW** | `PERIPH-SPI-001` | `spi_calc_br_div()`, `spi_init_master()`, `spi_transfer_byte()`, `spi_transfer_buf()`, `spi_wait_idle()` | `spi_status_t` (`SPI_OK`, `INVALID_ARG`, `TIMEOUT`); bounded `TXE`/`RXNE`/`FTLVL`/`BSY` waits; `SPI_CR2_FRXTH` + 8-bit `DR` access | 20 B (`spi_transfer_buf`) |
| 9 | `i2c_g4.h` / `.c` | **NEW** | `PERIPH-I2C-001`, `PERIPH-I2C-002`, `PERIPH-I2C-003` | `i2c_select_timingr()`, `i2c_init_master()`, `i2c_write()`, `i2c_write_reg()`, `i2c_read_reg()`, `i2c_recover_bus_9clks()` | `i2c_status_t` (`I2C_OK`, `INVALID_ARG`, `NACK`, `BUS`, `ARLO`, `BUSY`, `TIMEOUT`, `UNSUP_CLK`); checks `NACKF`/`BERR`/`ARLO` on every phase; `RELOAD` >255 B | 48 B (`i2c_write_stream`) |
| 10 | `adc_g4.h` / `.c` | **NEW** | `PERIPH-ADC-001` | `adc_raw_to_mv()`, `adc_calc_vdda_mv()`, `adc_temp_mdegc()`, `adc_init_single_ended()`, `adc_read_channel()`, `adc_read_vrefint_raw()`, `adc_read_temp_raw()`, `adc_read_temp_mdegc()` | `adc_status_t` (`ADC_OK`, `INVALID_ARG`, `TIMEOUT`, `NOT_READY`); bounded `ADDIS`/`ADCAL`/`ADRDY`/`EOC` waits; `CKMODE=11` (`HCLK/4` <= 60 MHz) | 32 B (`adc_read_temp_mdegc`) |
| 11 | `dma_g4.h` / `.c` | **NEW** | `PERIPH-DMA-001`, `PERIPH-DMA-002`, `STM32-ERRATA-001` | `dma_dmamux_channel_index()`, `dma_enable_clocks()`, `dma_clear_flags()`, `dma_config_p2m_circular()`, `dma_mem2mem_sync()` | `dma_status_t` (`DMA_OK`, `INVALID_ARG`, `TRANSFER`, `TIMEOUT`); alignment checks; individual `CTCIF`/`CHTIF`/`CTEIF` clear (ES0430 §2.3.1) | 56 B (`dma_config_p2m_circular`) |
| 12 | `iwdg_g4.h` / `.c` | **NEW** | `PERIPH-WDG-001`, `STM32-RESET-001`, `STM32-DBGMCU-001` | `iwdg_calc_prescaler_reload()`, `iwdg_init_ms()`, `iwdg_kick()`, `iwdg_was_reset_cause()`, `iwdg_clear_reset_flags()` | `iwdg_status_t` (`IWDG_OK`, `INVALID_ARG`, `TIMEOUT`); bounded `LSIRDY` & `IWDG->SR` (`PVU`/`RVU`) waits; optional `DBG_IWDG_STOP` freeze | 56 B (`iwdg_init_ms`) |

---

## 2. Silicon-Specific RM0440 / DS12288 / ES0430 Safeguards Audited

1. **Dynamic Clock Coherency (`clock_g4` + `tim_g4` / `usart_g4` / `spi_g4` / `i2c_g4`):**
   - Both **`Clock_Init_160MHz()`** (`HSI16 / 4 * 80 / 2`, VCO = 320 MHz, Boost `R1MODE=0`, 4 WS, two-step `HPRE=/2 -> >=1 us -> HPRE=/1`) and **`Clock_Init_170MHz()`** (`HSI16 / 4 * 85 / 2`, VCO = 340 MHz) update CMSIS `SystemCoreClock` via `SystemCoreClockUpdate()`.
   - Every timing-sensitive peripheral driver reads `SystemCoreClock` and `RCC->CFGR` (`PPRE1`/`PPRE2`) at runtime — eliminating silent 16 MHz / 160 MHz / 170 MHz baud-rate and PWM mismatches.
2. **I2C v2 `TIMINGR` Compliance (NXP UM10204 + RM0440 §39.4.9):**
   - `I2C_TIMINGR_170MHZ_100KHZ = 0x70C45D6CUL`: $t_{\text{PRESC}} = 47.06\text{ ns}$, $t_{\text{SCLL}} = 5.13\,\mu\text{s} \ge 4.7\,\mu\text{s}$, $t_{\text{SCLH}} = 4.42\,\mu\text{s} \ge 4.0\,\mu\text{s}$.
   - `I2C_TIMINGR_160MHZ_100KHZ = 0x70C45866UL`: $t_{\text{PRESC}} = 50.00\text{ ns}$, $t_{\text{SCLL}} = 5.15\,\mu\text{s} \ge 4.7\,\mu\text{s}$, $t_{\text{SCLH}} = 4.45\,\mu\text{s} \ge 4.0\,\mu\text{s}$.
   - `I2C_TIMINGR_170MHZ_400KHZ = 0x30B21F3BUL`: $t_{\text{PRESC}} = 23.53\text{ ns}$, $t_{\text{SCLL}} = 1.41\,\mu\text{s} \ge 1.3\,\mu\text{s}$, $t_{\text{SCLH}} = 0.75\,\mu\text{s} \ge 0.6\,\mu\text{s}$.
   - `I2C_TIMINGR_160MHZ_400KHZ = 0x30B21D38UL`: $t_{\text{PRESC}} = 25.00\text{ ns}$, $t_{\text{SCLL}} = 1.43\,\mu\text{s} \ge 1.3\,\mu\text{s}$, $t_{\text{SCLH}} = 0.75\,\mu\text{s} \ge 0.6\,\mu\text{s}$.
3. **ADC Clock Limit & Internal Channels (DS12288 §5.3.19 + RM0440 §21.4.6):**
   - Synchronous `CKMODE = 0b11` (`HCLK / 4`) yields **42.5 MHz @ 170 MHz** and **40.0 MHz @ 160 MHz** — strictly within DS12288 max $f_{\text{ADC}} = 60\text{ MHz}$.
   - Full startup sequence: `ADDIS` (if active) -> `DEEPPWD = 0` -> `ADVREGEN = 1` -> `25 us` delay (`>= 20 us`) -> `ADCALDIF = 0` -> `ADCAL` -> `ADC->ISR = ADC_ISR_ADRDY` (direct `rc_w1`) -> `ADEN = 1`.
   - Internal Temperature Sensor enables `VSENSESEL`, waits `125 us` (`>= 120 us` $t_{\text{START}}$), samples `ADC1_IN16` at **640.5 cycles** (`15.1 us >= 5 us`), and interpolates using factory `TS_CAL1` (`0x1FFF75A8`), `TS_CAL2` (`0x1FFF75CA`), and `VREFINT_CAL` (`0x1FFF75AA`).
4. **SPI 8-Bit FIFO Packing (RM0440 §42.5.9):**
   - Explicitly sets `SPI_CR2_DS = 0b0111` (8-bit) **and `SPI_CR2_FRXTH = 1`** (8-bit RXNE threshold) and accesses `DR` strictly via `*(__IO uint8_t *)&spi->DR`.
5. **DMA / DMAMUX1 & Errata ES0430 §2.3.1:**
   - Enables `DMAMUX1EN` together with `DMA1EN`/`DMA2EN`; uses explicit CMSIS channel pointer arrays (avoiding the `0x14`-byte stride vs 16-byte struct size hazard); clears ONLY individual `CTCIFx`/`CHTIFx`/`CTEIFx` bits in `IFCR` (never `CGIFx`).
6. **Cortex-M4 Posted-Write Barrier (ES0430 §2.1.3 / ARM Errata 838869):**
   - `EXTI_ClearPending()`, `TIM_ClearUpdateFlag()`, `usart_rx_irq_handler()`, and `dma_clear_flags()` issue `__DSB()` immediately after clearing peripheral interrupt flags.

---

## 3. Compilation & Link Verification Proof (`arm-none-eabi-gcc 14.3.1`)

**Toolchain:** `arm-none-eabi-gcc.exe (GNU Tools for STM32 14.3.rel1.20251027-0700) 14.3.1 20250623`  
**Mandatory Flags:** `-std=c11 -mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard -Wall -Wextra -Werror -Wpedantic -Wshadow -Wconversion -Wdouble-promotion -Wundef -ffunction-sections -fdata-sections -fstack-usage`  
**Build Systems Updated:** `23-code-library/templates/Makefile` and `23-code-library/templates/CMakeLists.txt` (all 12 driver `.c` sources listed).

| Build Profile | Command | Compiler Warnings / Errors | FLASH Used | RAM Used | CCMRAM Used | Result |
|---|---|---|---|---|---|---|
| **Debug (`-Og -g3`)** | `make clean && make BUILD_TYPE=debug` | **0 warnings / 0 errors** | 2,752 B (0.52%) | 1,704 B (1.73%) | 12 B (0.04%) | **PASS (exit 0)** |
| **Release (`-O2 -g -DNDEBUG`)** | `make clean && make BUILD_TYPE=release` | **0 warnings / 0 errors** | 2,616 B (0.50%) | 1,704 B (1.73%) | 12 B (0.04%) | **PASS (exit 0)** |
| **Full No-GC Link Check (all 12 `.o` linked without `--gc-sections`)** | `arm-none-eabi-gcc <all 15 .o> -T stm32g474re.ld --specs=nano.specs --specs=nosys.specs` | **0 undefined refs / 0 errors** | 13,992 B (2.67%) | 2,272 B (2.31%) | 12 B (0.04%) | **PASS (exit 0)** |

> *Note:* Standard newlib `--specs=nosys.specs` linker notices (`_close/_lseek/_read/_write is not implemented`) and `RWX` LOAD segment note (due to executable `CCMRAM (xrw)`) are expected toolchain linker notes, with **zero compiler warnings** under `-Werror`.

---

## 4. Host Unit-Test Verification (`-DDRIVER_HOST_TEST`)

All 8 new drivers guard CMSIS hardware sections behind `#if !defined(DRIVER_HOST_TEST)` so every pure calculation helper can be compiled natively on a host PC alongside `ring_buffer.c`:

- **Host Compiler:** `gcc.exe (MSYS2 UCRT64) 15.2.0`
- **Flags:** `-std=c11 -DDRIVER_HOST_TEST -Wall -Wextra -Werror -Wpedantic -Wshadow -Wconversion -Wdouble-promotion -Wundef`
- **Verified Pure Helpers (0 warnings, 100% assertions passed):**
  - `exti_calc_exticr_index_shift()` — pins `0, 5, 13, 15`, out-of-range `16`, NULL guard
  - `tim_calc_psc_arr()` — 170 MHz @ 25 kHz / 1 kHz / 1 Hz, 160 MHz @ 10 kHz, invalid args
  - `tim_calc_pwm_ccr()` — `0%`, `50%`, `100%`, and `arr = 0xFFFF` 16-bit wraparound clamp
  - `usart_calc_brr()` & `lpuart_calc_brr()` — 170 MHz / 160 MHz / 16 MHz @ 115,200 baud
  - `spi_calc_br_div()` — 170 MHz & 160 MHz divider selection and `/256` saturation
  - `i2c_select_timingr()` — all 6 verified profiles (170/160/16 MHz × 100/400 kHz) + unsupported clock
  - `adc_raw_to_mv()`, `adc_calc_vdda_mv()`, `adc_temp_mdegc()` — endpoints, midpoint, 3.0 V / 3.3 V scaling, invalid cal guard
  - `dma_dmamux_channel_index()` — DMA1 Ch1..8 (`0..7`), DMA2 Ch1..8 (`8..15`), out-of-range (`-1`)
  - `iwdg_calc_prescaler_reload()` — 1000 ms (`PR=1, RLR=3999`), 5000 ms (`PR=4, RLR=2499`), max 32768 ms (`PR=6, RLR=4095`), overflow rejection
  - `ring_buffer` — C11 `<stdatomic.h>` SPSC push/pop smoke test

---

## 5. Physical Hardware Verification Status per Driver (NUCLEO-G474RE Bring-Up)

Executed on physical **NUCLEO-G474RE (`MB1367 Rev C`, ST-LINK SN `003700413335510133383531`, `VDD = 3.28 V`, `COM8`)** via [`HARDWARE_VALIDATION/onboard_selftest/`](../HARDWARE_VALIDATION/onboard_selftest/Src/main.c) and Capstones 01–03 (see [`AUDIT/HARDWARE_VALIDATION_SUMMARY.md`](HARDWARE_VALIDATION_SUMMARY.md) & [`HARDWARE_VALIDATION/PENDING_EXTERNAL_CONNECTIONS.md`](../HARDWARE_VALIDATION/PENDING_EXTERNAL_CONNECTIONS.md)):

1. **`clock_g4` — `ONBOARD HARDWARE TESTED` (`PASS`):** Verified `Clock_Init_160MHz()` (`DWT_10ms = 1,599,997` cyc, `PWR->CR5 = 0`, `FLASH->ACR = 0x00040704`) and `Clock_Init_170MHz()` (`DWT_10ms = 1,699,989` cyc). *(Optional `Clock_Init_170MHz_HSE24()` + `PA8` MCO scope check: `EXTERNAL HARDWARE PENDING`.)*
2. **`gpio_g4` + `exti_g4` — `ONBOARD HARDWARE TESTED` (`PASS`):** Verified `PA5` (`LD2`) atomic `BSRR` set/reset/toggle readback, `PC13` `MODER=00b` & unpressed Active-HIGH `0`, and `EXTI_TriggerSoftware(13)` invoking registered callback 3/3 times (`SYSCFG->EXTICR[3] = 0x00000020`). Note: Physical bring-up caught missing `GPIO_EnableClock()` calls in Capstones 01–03 (fixed and re-verified).
3. **`tim_g4` — `ONBOARD HARDWARE TESTED` (`PASS`):** Verified `TIM_InitPeriodic(TIM2, 100 Hz)` (`PSC=25, ARR=65384`, `11` IRQs in 100 ms, `rc_w0` clear) and `TIM_InitPwm(TIM2, Ch2, 20 kHz)` (`ARR=8499` in Project 01). *(Oscilloscope PWM edge capture on `PA6`/`PA1`: `EXTERNAL HARDWARE PENDING`.)*
4. **`usart_g4` + `ring_buffer` — `ONBOARD HARDWARE TESTED` (`PASS`):** Verified `USART2` (`PA2`/`PA3` AF7, `115200 8N1`) TX at both `160 MHz` (`BRR=1389`) and `170 MHz` (`BRR=1476`), plus live RX IRQ -> C11 SPSC `ring_buffer_t` command parsing (`'s'` sent over `COM8` in Project 01 incremented `rx_cmds: 0 -> 1`).
5. **`spi_g4` — `EXTERNAL HARDWARE PENDING`:** Cross-compiled + host-tested; physical `PB4 <-> PB5` loopback jumper / external 3.3 V SPI slave check documented in `PENDING_EXTERNAL_CONNECTIONS.md`.
6. **`i2c_g4` — `ONBOARD PARTIAL` (`NACKF` VERIFIED) / `EXTERNAL HARDWARE PENDING`:** Verified bounded absent-slave `NACKF` detection on `I2C1` (`PA15`/`PB7`, `0x3C` OLED absent without hang in Projects 02 & 03); external slave ACK, `i2c_read_reg()` Repeated START, and `i2c_recover_bus_9clks()` pending external slave + `4.7 kΩ` pull-ups.
7. **`adc_g4` — `ONBOARD HARDWARE TESTED` (`PASS`):** Verified `adc_init_single_ended(ADC1)` (`CKMODE=11b`, `CALFACT=0x45`) + `adc_read_temp_mdegc()` (`VREF_CAL=1657`, `VREF_RAW=1513` -> `VDDA=3288 mV` matching ST-LINK `3.28–3.29 V`; `TS_CAL1=1037`, `TS_CAL2=1370`, `TEMP_RAW=956` -> `32.91 °C`).
8. **`dma_g4` — `ONBOARD HARDWARE TESTED` (`PASS`):** Verified `dma_mem2mem_sync()` (`256/256` exact match in `1,977` cycles, `DMA1->ISR = 0x00000000`) and `dma_config_p2m_circular()` (`ADC1 -> DMA1_Ch1`, `DMAMUX0 = 0x5`, `HT=20`, `TC=20`, `TE=0` in 10 ms; plus 25 consecutive `160 ms` ping-pong frames with `ovr=0, te=0` in Project 02).
9. **`iwdg_g4` — `ONBOARD HARDWARE TESTED` (`PASS`):** Verified `iwdg_init_ms(2000, true)` (`PR=2`, `RLR=3999`, `DBGMCU->APB1FZR1 = 0x00001800`) + 6/6 clean kicks over 300 ms and continuous watchdog supervision in FreeRTOS + Capstones 01 & 03 without spurious resets.
10. **`fault_handler` — `ONBOARD HARDWARE TESTED` (`PASS`):** Verified `FaultHandler_EnableDetailedFaults(true)` + live `HardFault_Handler_C()` SRAM `.noinit` capture via SWD readback (`magic=0xFA017C0D`, `pc=0x08001A0C`, `cfsr=0x02000000`, `flags=0x00000084`, `stacked_sp=0x20017F48`) AND automated `.noinit` persistence across `NVIC_SystemReset()` (`FaultHandler_HasValidCrashLog()` / `FaultHandler_ClearCrashLog()`).
