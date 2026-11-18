# Physical Hardware Evidence, Binary Traceability & 7-State Verification Audit

> **Audit Date:** 2026-11-18  
> **Repository Root:** `<repo-root>` (`embedded-c-master-learning-hub`, base commit `543d21e`)  
> **Target Hardware:** ST **NUCLEO-G474RE** (`MB1367 Rev C`), `STM32G474RET6` (`Device ID = 0x469`, `Rev X / 0x2003`, `512 KB` Flash, `96 KB` SRAM1+2, `32 KB` CCM-SRAM)  
> **Debug Probe / VCP:** Onboard **STLINK-V3E** (`SN: <redacted-stlink-sn>`, FW `V3J17M11`, SWD `8000 KHz`, VCP `COM8` `115200 8N1`, `VDD = 3.28–3.29 V`)  
> **Cross-Compiler:** `arm-none-eabi-gcc 14.3.1 20250623 (GNU Tools for STM32 14.3.rel1.20251027-0700)`  
> **Sanitization Policy:** Local host filesystem roots are normalized to `<repo-root>` and physical probe serial numbers are redacted to `<redacted-stlink-sn>`.

---

## 1. Strict 7-State Verification Taxonomy

To prevent overclaiming compilation, software triggers, or simulation runs as full physical bench verification, every hardware claim in this repository is strictly separated across **7 independent verification states**:

| State | Label | Exact Engineering Meaning |
|---|---|---|
| **1** | **Static review** | Source code inspected against `stm32g474xx.h`, `core_cm4.h`, RM0440, DS12288, ES0430, PM0214, UM2505, and `gcc -fanalyzer`. |
| **2** | **Host test passed** | Executed natively on host x86_64 via `make -C tests clean test` (**873 / 873 assertions passing, `0` failed**). |
| **3** | **Cross-compilation passed** | Compiled and linked for `cortex-m4` (`-mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard -Wall -Wextra -Werror`) with `0` warnings / `0` errors. |
| **4** | **Firmware uploaded** | Binary flashed to physical NUCLEO-G474RE Flash (`0x08000000`) and verified byte-for-byte via `STM32_Programmer_CLI -v -rst`. |
| **5** | **Target execution verified** | Executed on live `STM32G474RET6` silicon with structured register readbacks / telemetry captured over ST-LINK VCP (`COM8`, `115200 8N1`) or SWD SRAM readback. |
| **6** | **Physical behavior verified** | Real external physical stimulus or human interaction observed (e.g. physical `B1` switch press, visual `LD2` blink confirmation, external I2C/SPI slave ACK, real pot voltage sweep, or live physical power stage). |
| **7** | **Instrument measurement verified** | Calibrated external bench instrument measurement recorded (oscilloscope waveform/dead-time/break `< 50 ns`, logic analyzer capture, precision DMM, or `JP6` `IDD` microamp profiler). |

---

## 2. Cryptographic Binary Provenance Registry (Exact SHA-256 at Commit `543d21e`)

Every `.elf` and raw `.bin` binary produced and flashed onto the physical NUCLEO-G474RE board is recorded below with exact byte size and SHA-256 digest:

| # | Firmware Target (`<repo-root>/...`) | Build Profile | File Size (Bytes) | Exact SHA-256 Digest |
|---|---|---|---:|---|
| **1a** | `HARDWARE_VALIDATION/onboard_selftest/build/onboard_selftest.elf` | `debug` (`-Og -g3`) | `955,820` | `98a158383e27ee0c8b0ced3c660de3b504d752653d92149260e962eba31dd7d4` |
| **1b** | `HARDWARE_VALIDATION/onboard_selftest/build/onboard_selftest.bin` | `debug` (`-Og -g3`) | `23,448` | `244fe4e573c99bc7e6230a6fa85411a910b1c792628e73f83cb8365df8fedfd1` |
| **2a** | `23-code-library/freertos/build/nucleo_g474re_freertos.elf` | `debug` (`-Og -g3`) | `1,056,736` | `8265ac2d8827730ffd7fdb88a2ed7a98e90ccd4449a9e9689d140d0b740ef565` |
| **2b** | `23-code-library/freertos/build/nucleo_g474re_freertos.bin` | `debug` (`-Og -g3`) | `21,592` | `3edc1d72be86dd3bec04aca7a579292f864ff4f794fa525f47e7dc67b094213d` |
| **3a** | `19-projects/01-beginner-smart-instrument-panel/build/smart_instrument_panel.elf` | `debug` (`-Og -g3`) | `911,804` | `7110f82670256b0016c1a914ae3b8ddd3c617237b648cc816cc0f421f752eaab` |
| **3b** | `19-projects/01-beginner-smart-instrument-panel/build/smart_instrument_panel.bin` | `debug` (`-Og -g3`) | `12,724` | `16d397ea17e5c53381e32a24a3dafe62713381f10cf44adf1e49547e6c4f73c1` |
| **4a** | `19-projects/02-intermediate-dma-data-logger/build/dma_data_logger.elf` | `debug` (`-Og -g3`) | `924,000` | `34215653e6323ddab024bab5cd9960bb493a8527221182fd175a164f11cbb0dc` |
| **4b** | `19-projects/02-intermediate-dma-data-logger/build/dma_data_logger.bin` | `debug` (`-Og -g3`) | `11,784` | `400d17552bb3b7487e915a9421bb4410beabd5a5c64bdb65f601bbad46c699da` |
| **5a** | `19-projects/03-advanced-freertos-digital-power-controller/build/freertos_digital_power_ctrl.elf` | `debug` (`-Og -g3`) | `1,099,468` | `c30bf356d3c0c1f8060b6d9a71a47d5636cc5083a4901a2f6d1041f9960f69cc` |
| **5b** | `19-projects/03-advanced-freertos-digital-power-controller/build/freertos_digital_power_ctrl.bin` | `debug` (`-Og -g3`) | `21,424` | `0de80ac42c43d7a6aabc08356adca9e475803be6bb082ca00e08058158c74d36` |
| **6a** | `23-code-library/templates/build/firmware.elf` | `release` (`-O2 -g`) | `61,260` | `35e0dbe01af2a10dc4519460ecaa4ed83b8ccc4e8d5730e7d4fd454f7c6e3787` |
| **6b** | `23-code-library/templates/build/firmware.bin` | `release` (`-O2 -g`) | `2,632` | `4f7a01c0ba864f5cf256904ab719079d1727ac6cce51ee30d7465521c14c7d63` |

---

## 3. Complete 14-Field Traceability Matrix for Every Physical Hardware Test

### 3.1 Suite A — `onboard_selftest` (`9/9` Onboard Silicon Self-Tests)

| # | Traceability Field | Verified Audit Record |
|---|---|---|
| **1** | **Test ID / Target** | `HW-TEST-01`: `HARDWARE_VALIDATION/onboard_selftest` (Tests 1, 2, 3A, 3B, 4, 5A, 5B, 5C, 6A, 6B, 7, 8A/8B, 9) |
| **2** | **Git Commit & Profile** | Commit `543d21e`, `BUILD_TYPE=debug` (`-Og -g3`, `FLASH = 23,344 B [4.45%]`, `RAM = 10,752 B [10.94%]`) |
| **3** | **Artifact SHA-256** | `.elf`: `98a158383e27ee0c8b0ced3c660de3b504d752653d92149260e962eba31dd7d4` \| `.bin`: `244fe4e573c99bc7e6230a6fa85411a910b1c792628e73f83cb8365df8fedfd1` |
| **4** | **Toolchain & Flags** | `arm-none-eabi-gcc 14.3.1`, `-std=c11 -mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard -DSTM32G474xx -Wall -Wextra -Werror -Wpedantic -Wshadow -Wconversion -Wdouble-promotion -Wundef` (`0` warnings / `0` errors) |
| **5** | **Hardware & Probe** | ST NUCLEO-G474RE (`MB1367 Rev C`), `STM32G474RET6` (`DEV=0x469`, `REV=0x2003`), STLINK-V3E `<redacted-stlink-sn>`, FW `V3J17M11`, SWD `8000 KHz`, `VDD = 3.29 V` |
| **6** | **Flash Command** | `STM32_Programmer_CLI -c port=SWD mode=NORMAL -w onboard_selftest.elf -v -rst` (verified in `0.065 s`) |
| **7** | **Peripherals & Pins** | `DBGMCU`, `SCB`, `RCC`, `PWR`, `FLASH`, `DWT`, `GPIOA` (`PA5`), `GPIOC` (`PC13`), `USART2` (`PA2`/`PA3` AF7), `TIM2`, `SYSCFG`/`EXTI13`, `DMA1`/`DMAMUX1`, `ADC1` (`VREFINT`/`Temp`), `CORDIC`, `FMAC`, `CRC`, `FDCAN1` (internal loopback), `MPU`, `IWDG` |
| **8** | **Execution Mode** | **Onboard physical silicon execution** (zero external wiring). *Note:* `EXTI13` uses software `EXTI->SWIER1` trigger; `FDCAN1` uses internal silicon loopback (`CCCR.TEST | TEST.LBCK`). |
| **9** | **Expected Criteria** | Exact MCU ID `0x469`, `160`/`170 MHz` `DWT` within `±0.5%`, `VDDA` ~`3.28–3.30 V`, `CORDIC` err `< 1e-5`, `FMAC` `0 LSB` diff, `CRC-32 = 0xCBF43926`, `FDCAN1` 16 B BRS `ECR=0`, `.noinit` `DIV0` persistence across reset, `IWDG` `6/6` clean kicks. |
| **10** | **Observed Output & Registers** | `DBGMCU_IDCODE=0x20036469`; `160 MHz` `DWT_10ms=1,599,997` (`-0.0002%`, `PWR_CR5=0`, `FLASH_ACR=0x00040704`); `170 MHz` `DWT_10ms=1,699,989` (`-0.0006%`); `TIM2` `11` IRQs/`100 ms`; `EXTI13` `swier_cb=3/3` `EXTICR4=0x20`; `DMA1` `256/256` (`1,977` cyc); `ADC1` `VDDA=3288 mV`, `Die Temp=32.91 C`, `HT=20/TC=20/TE=0`; `CORDIC` `9.72×` (`2e-6` err); `FMAC` `2.32×` (`0 LSB`); `FDCAN1` `MATCH=1, ECR=0`; `DIV0` reboot `magic=0xFA017C0D, CFSR=0x02000000, SP=0x20017F48`; `IWDG` `PR=2, RLR=3999, kicks=6/6`. |
| **11** | **7-State Status** | `1.Static: PASS` \| `2.Host: PASS` \| `3.Cross-CC: PASS` \| `4.Uploaded: PASS` \| `5.Target Exec: PASS (9/9)` \| `6.Physical Behavior: PARTIAL` *(internal silicon verified; physical `B1`/`LD2` visual & external CAN bus pending)* \| `7.Instrument: PENDING` |
| **12** | **Defects Resolved** | Caught intra-object GNU `ld --wrap=HardFault_Handler_C` limitation vs weak symbol in `startup_stm32g474re.c`; fixed cleanly by overriding `UsageFault_Handler` in `onboard_selftest/Src/main.c` while keeping `fault_handler.c` untouched (SWD readback also proved `fault_handler.c`'s real `HardFault_Handler_C` captured `magic=0xFA017C0D` in SRAM). |
| **13** | **Scope Limitations** | `EXTI13` test used `SWIER1` (does NOT verify physical `PC13` edge or tactile `B1`); `FDCAN1` internal loopback does NOT verify external CAN-FD transceiver (`TJA1051`/`MCP2562FD`) physical differential bus. |
| **14** | **Pending User Actions** | Manual physical `B1` press & `LD2` observation (`PENDING — USER INTERACTION REQUIRED`); external CAN transceiver / scope checks (`PENDING — USER CONNECTION REQUIRED`). |

---

### 3.2 Suite B — FreeRTOS `V11.1.0` Reference Verification Firmware (`23-code-library/freertos/`)

| # | Traceability Field | Verified Audit Record |
|---|---|---|
| **1** | **Test ID / Target** | `HW-TEST-02`: `23-code-library/freertos` (`6/6` Self-Tests + `8.0 s` live telemetry stream) |
| **2** | **Git Commit & Profile** | Commit `543d21e`, `BUILD_TYPE=debug` (`-Og -g3`, `FLASH = 21,480 B [4.10%]`, `RAM = 35,728 B [36.34%]`) |
| **3** | **Artifact SHA-256** | `.elf`: `8265ac2d8827730ffd7fdb88a2ed7a98e90ccd4449a9e9689d140d0b740ef565` \| `.bin`: `3edc1d72be86dd3bec04aca7a579292f864ff4f794fa525f47e7dc67b094213d` |
| **4** | **Toolchain & Flags** | `arm-none-eabi-gcc 14.3.1`, `-Wall -Wextra -Werror` on user code (`0` warnings / `0` errors), FreeRTOS-Kernel `V11.1.0` (`ARM_CM4F`, `heap_4.c`) |
| **5** | **Hardware & Probe** | ST NUCLEO-G474RE (`MB1367 Rev C`), `STM32G474RET6`, STLINK-V3E `<redacted-stlink-sn>`, `VDD = 3.28 V`, VCP `COM8` `115200 8N1` |
| **6** | **Flash Command** | `STM32_Programmer_CLI -c port=SWD mode=NORMAL -w nucleo_g474re_freertos.elf -v -rst` (`21.16 KB`, verified in `0.060 s`) |
| **7** | **Peripherals & Pins** | `RCC`/`PWR`/`FLASH` (`170 MHz`), `SysTick` (`1 ms`), `PendSV`, `SVC`, `TIM2` (`10 Hz`, NVIC prio `6`), `EXTI13` (`PC13`, NVIC prio `7`), `USART2` (`PA2`/`PA3`), `LD2` (`PA5`), `IWDG` (`~3.0 s`) |
| **8** | **Execution Mode** | **Onboard physical silicon RTOS execution** (zero external wires; unattended `8.0 s` run). |
| **9** | **Expected Criteria** | `PRIGROUP=0`, `PendSV=0xF0`, `SysTick=0xF0`, `TIM2_PRIO=6`, Binary/Counting semaphores, Mutex PIP `1->5->1`, Deadlock timeout + `A->B`, `TIM2` `FromISR` + Queue + `fpu_err=0`, healthy HWM, zero heap leak, `IWDG` continuous feed. |
| **10** | **Observed Output & Registers** | `6/6 ALL_PASSED`: `NVIC_CONFIG`, `SEMAPHORES`, `PRIORITY_INHERITANCE (base=1 promoted=5 restored=1 high_acq=1)`, `DEADLOCK_GUARD`, `FROM_ISR_AND_QUEUE (tim2_isr=4 notify=4 q_tx=5 q_rx=4 sw_tmr=4 fpu_err=0)`, `STACK_HWM (Sup=339 Cons=309 Prod=186 Mon=459, heap=21104 B)`. Live `8.0 s` stream: `t=1010..8010 ms`, `q_tx=11..81`, `wdg=2..9`, `heap=21104/21104`, `HWM(S=339 C=309 P=186 M=324)`. |
| **11** | **7-State Status** | `1.Static: PASS` \| `2.Host: N/A (CM4F port)` \| `3.Cross-CC: PASS` \| `4.Uploaded: PASS` \| `5.Target Exec: PASS (6/6)` \| `6.Physical Behavior: PARTIAL` *(`btn=0` during unattended run; manual `B1` press not exercised)* \| `7.Instrument: PENDING` |
| **12** | **Defects Resolved** | None in `freertos/` (clean first-pass onboard bring-up). |
| **13** | **Scope Limitations** | **8 s run != formal WCET determinism proof** (see `AUDIT/FREERTOS_REVIEW.md`); `configUSE_NEWLIB_REENTRANT = 0`; `configUSE_TICKLESS_IDLE = 0`; manual `B1` (`EXTI13`) press not actuated (`btn=0`). |
| **14** | **Pending User Actions** | Manual physical `B1` press to verify `btn > 0` `xQueueSendFromISR` (`PENDING — USER INTERACTION REQUIRED`); logic analyzer context-switch / ISR jitter capture (`PENDING — USER CONNECTION REQUIRED`). |

---

### 3.3 Suite C — Capstone Project 01 (`smart_instrument_panel`)

| # | Traceability Field | Verified Audit Record |
|---|---|---|
| **1** | **Test ID / Target** | `HW-TEST-03`: `19-projects/01-beginner-smart-instrument-panel` (`MODE_INTERNAL_TELEM` + live UART RX `'s'`) |
| **2** | **Git Commit & Profile** | Commit `543d21e`, `BUILD_TYPE=debug` (`-Og -g3`, `12.34 KB` Flash) |
| **3** | **Artifact SHA-256** | `.elf`: `7110f82670256b0016c1a914ae3b8ddd3c617237b648cc816cc0f421f752eaab` \| `.bin`: `16d397ea17e5c53381e32a24a3dafe62713381f10cf44adf1e49547e6c4f73c1` |
| **4** | **Toolchain & Flags** | `arm-none-eabi-gcc 14.3.1`, `-Wall -Wextra -Werror` (`0` warnings / `0` errors) |
| **5** | **Hardware & Probe** | ST NUCLEO-G474RE (`MB1367 Rev C`), `STM32G474RET6`, STLINK-V3E `<redacted-stlink-sn>`, VCP `COM8` `115200 8N1` |
| **6** | **Flash Command** | `STM32_Programmer_CLI -c port=SWD mode=NORMAL -w smart_instrument_panel.elf -v -rst` |
| **7** | **Peripherals & Pins** | `clock_g4` (`170 MHz`), `GPIOA` (`PA5` LD2, `PA2`/`PA3` USART2, `PA1` TIM2_CH2 `20 kHz` PWM, `PA0` ADC1_IN1 ext pot), `GPIOC` (`PC13` B1), `adc_g4`, `tim_g4`, `usart_g4`, `ring_buffer`, `iwdg_g4`, `fault_handler` |
| **8** | **Execution Mode** | **Onboard physical silicon execution** (`MODE_INTERNAL_TELEM`) + live interactive host->target UART `'s'` command injection over `COM8`. |
| **9** | **Expected Criteria** | Exact `500 ms` JSON frames, calibrated `VDDA` & `Die Temp`, `20 kHz` PWM (`ARR=8499`), live `'s'` command parsed via `USART2_IRQHandler -> ring_buffer_t` incrementing `rx_cmds: 0 -> 1`. |
| **10** | **Observed Output & Registers** | `SYSCLK=170000000 Hz`, `VDDA=3286..3288 mV`, `Die Temp=32.5..33.2 C`, `PWM PA1 AF1 20000 Hz (ARR=8499)`, exact `500 ms` ticks (`527, 1027, 1527, 2027..4027 ms`), `'s'` command triggered `[PANEL STATUS]` with `RX Cmds: 1` and `rx_cmds: 1`. |
| **11** | **7-State Status** | `1.Static: PASS` \| `2.Host: PASS (drivers)` \| `3.Cross-CC: PASS` \| `4.Uploaded: PASS` \| `5.Target Exec: PASS` \| `6.Physical Behavior: PARTIAL` *(UART RX verified; ext `PA0` pot, `PA1` load, manual `B1` pending)* \| `7.Instrument: PENDING` |
| **12** | **Defects Resolved** | **Caught genuine silicon defect:** Missing `GPIO_EnableClock(GPIOA/GPIOC)` caused `0` bytes UART output on first flash despite `CFSR=0`; fixed cleanly and re-verified live on silicon. |
| **13** | **Scope Limitations** | `Ext Pot PA0` reads `0` (`[EXT PENDING]`) without an external `10 kΩ` potentiometer; `PA1` PWM waveform not yet probed on oscilloscope. |
| **14** | **Pending User Actions** | Wire `10 kΩ` pot to `PA0`, attach LED/scope to `PA1`, press `B1` (`PENDING — USER CONNECTION REQUIRED`). |

---

### 3.4 Suite D — Capstone Project 02 (`dma_data_logger`)

| # | Traceability Field | Verified Audit Record |
|---|---|---|
| **1** | **Test ID / Target** | `HW-TEST-04`: `19-projects/02-intermediate-dma-data-logger` (`ADC1 -> DMA1_Ch1` circular ping-pong + HW `CRC-32` + `I2C1` absent-slave probe) |
| **2** | **Git Commit & Profile** | Commit `543d21e`, `BUILD_TYPE=debug` (`-Og -g3`, `11.43 KB` Flash) |
| **3** | **Artifact SHA-256** | `.elf`: `34215653e6323ddab024bab5cd9960bb493a8527221182fd175a164f11cbb0dc` \| `.bin`: `400d17552bb3b7487e915a9421bb4410beabd5a5c64bdb65f601bbad46c699da` |
| **4** | **Toolchain & Flags** | `arm-none-eabi-gcc 14.3.1`, `-Wall -Wextra -Werror` (`0` warnings / `0` errors) |
| **5** | **Hardware & Probe** | ST NUCLEO-G474RE (`MB1367 Rev C`), `STM32G474RET6`, STLINK-V3E `<redacted-stlink-sn>`, VCP `COM8` `115200 8N1` |
| **6** | **Flash Command** | `STM32_Programmer_CLI -c port=SWD mode=NORMAL -w dma_data_logger.elf -v -rst` |
| **7** | **Peripherals & Pins** | `TIM3` TRGO, `ADC1` internal channels, `DMAMUX1` + `DMA1_Ch1` circular ping-pong, Hardware `CRC` (`0x04C11DB7`), `FLASH->OPTR.DBANK` (read-only dry-run), `I2C1` (`PA15`/`PB7` AF4), `USART2` (`PA2`/`PA3`) |
| **8** | **Execution Mode** | **Onboard physical silicon execution** (Bank 2 Flash staging in safe dry-run mode — zero Flash erases/option-byte writes; OLED absent -> tests `i2c_g4` `NACKF` timeout path). |
| **9** | **Expected Criteria** | `CRC32_TEST=0xCBF43926 (PASS)`, `FLASH_OPTR.DBANK=1`, non-hanging `I2C1` absent-slave detection, consecutive `160 ms` ping-pong `$LOG2` frames with alternating `half=0/1`, `ovr=0`, `te=0`, `ok=1`. |
| **10** | **Observed Output & Registers** | `CRC32_TEST=0xCBF43926 (PASS)`, `FLASH_OPTR.DBANK=1`, `I2C1 OLED (PA15/PB7 0x3C): ABSENT`, `25` consecutive `$LOG2` frames (`seq=0..24`, `t_ms=192..4032`, exact `160 ms` delta, `half=0/1`, `vdda_mv=3286..3288`, `temp=32.3..33.2 C`, `ovr=0, te=0, ok=1`). |
| **11** | **7-State Status** | `1.Static: PASS` \| `2.Host: PASS (drivers)` \| `3.Cross-CC: PASS` \| `4.Uploaded: PASS` \| `5.Target Exec: PASS` \| `6.Physical Behavior: PARTIAL` *(internal DMA/ADC/CRC/NACKF verified; external OLED pending)* \| `7.Instrument: PENDING` |
| **12** | **Defects Resolved** | Fixed missing `GPIO_EnableClock(GPIOA/GPIOB)` caught during physical bring-up. |
| **13** | **Scope Limitations** | `I2C1` verified absent-slave `NACKF` handling ONLY (does NOT verify physical slave ACK, Repeated START, or `i2c_recover_bus_9clks()`); Bank 2 Flash logging kept in read-only dry-run staging mode. |
| **14** | **Pending User Actions** | Connect `3.3 V` SSD1306 OLED (`0x3C`) + `4.7 kΩ` pull-ups on `PA15`/`PB7` (`PENDING — USER CONNECTION REQUIRED`). |

---

### 3.5 Suite E — Capstone Project 03 (`freertos_digital_power_ctrl`)

| # | Traceability Field | Verified Audit Record |
|---|---|---|
| **1** | **Test ID / Target** | `HW-TEST-05`: `19-projects/03-advanced-freertos-digital-power-controller` (`POWER_SIM_MODE=1` safe onboard simulation + live `TIM1`/`COMP1`/`DAC1` arming) |
| **2** | **Git Commit & Profile** | Commit `543d21e`, `BUILD_TYPE=debug` (`-Og -g3`, `20.89 KB` Flash, `POWER_SIM_MODE=1`) |
| **3** | **Artifact SHA-256** | `.elf`: `c30bf356d3c0c1f8060b6d9a71a47d5636cc5083a4901a2f6d1041f9960f69cc` \| `.bin`: `0de80ac42c43d7a6aabc08356adca9e475803be6bb082ca00e08058158c74d36` |
| **4** | **Toolchain & Flags** | `arm-none-eabi-gcc 14.3.1`, `-Wall -Wextra -Werror` (`0` warnings / `0` errors), FreeRTOS `V11.1.0` (`ARM_CM4F`, `heap_4.c`) |
| **5** | **Hardware & Probe** | ST NUCLEO-G474RE (`MB1367 Rev C`), `STM32G474RET6`, STLINK-V3E `<redacted-stlink-sn>`, VCP `COM8` `115200 8N1` |
| **6** | **Flash Command** | `STM32_Programmer_CLI -c port=SWD mode=NORMAL -w freertos_digital_power_ctrl.elf -v -rst` |
| **7** | **Peripherals & Pins** | `TIM1` complementary `50 kHz` center-aligned PWM (`PA8`/`PB13` AF6, `DTG=85` `500 ns`), `DAC1_CH1` (`PA4`, `HFSEL=10b`), `COMP1` (`PA1` pulled LOW in sim mode -> `TIM1->AF1.BKCMP1E`, `AOE=0`), `I2C1` (`PA15`/`PB7`), `USART2`, `IWDG` (`1500 ms`), `LD2` (`PA5`), `B1` (`PC13`) |
| **8** | **Execution Mode** | **HYBRID — Onboard FreeRTOS + peripheral register execution (`TIM1`/`COMP1`/`DAC1` live on silicon) WITH `POWER_SIM_MODE=1` software buck plant model** (`buck_sim_step()` in `power_ctrl.h`). **NOT a physical power-stage test!** |
| **9** | **Expected Criteria** | 4 static FreeRTOS tasks run cleanly at `170 MHz`, `1 kHz` Q15 PI regulator tracks `5000 mV` simulated plant, `TIM1->BDTR.MOE = 1`, `fault = 0`, `IWDG` fed continuously, healthy stack HWMs, absent OLED cleanly skipped. |
| **10** | **Observed Output & Registers** | `10` consecutive `500 ms` JSON frames (`t_ms=502..5002`, `sim:1`, `vref_mv:5000`, `vout_mv:4989..5021`, `duty_pm:209`, `ccr1:355`, `moe:1`, `fault:0`, `brk:0`, `wdg:1..19`, `hwm:[217,337,325,351]`). |
| **11** | **7-State Status** | `1.Static: PASS` \| `2.Host: PASS (drivers)` \| `3.Cross-CC: PASS` \| `4.Uploaded: PASS` \| `5.Target Exec: PASS (sim:1)` \| `6.Physical Behavior: PENDING` *(software plant only; no physical MOSFET power stage, no physical `B1` break test, no OLED)* \| `7.Instrument: PENDING` *(no scope capture of `500 ns` dead-time or `< 50 ns` `COMP1` trip)* |
| **12** | **Defects Resolved** | Fixed missing `GPIO_EnableClock(GPIOA/GPIOB/GPIOC)` caught during physical bring-up. |
| **13** | **Scope Limitations** | **CRITICAL DISTINCTION:** `sim:1` (`POWER_SIM_MODE=1`) validates the FreeRTOS task graph, Q15 PI math, `TIM1`/`COMP1`/`DAC1` register arming, and `IWDG` supervision on real silicon, **NOT** closed-loop physical power electronics regulation, `PA0` `ADC1_IN1` feedback (`POWER_SIM_MODE=0`), `500 ns` hardware dead-time integrity, or `< 50 ns` analog `COMP1` overcurrent trip latency! |
| **14** | **Pending User Actions** | Probe `PA8`/`PB13` on `>= 100 MHz` oscilloscope to verify `500 ns` dead-time BEFORE connecting any MOSFET stage; test `B1` long-press (`TIM1->EGR=BG`); test `COMP1` analog trip on `PA1`; optionally build `POWER_SIM_MODE=0` with current-limited bench PSU (`PENDING — USER CONNECTION REQUIRED`). |

---

### 3.6 Suite F — Clean Board Restoration (`23-code-library/templates/build/firmware.elf`)

| # | Traceability Field | Verified Audit Record |
|---|---|---|
| **1** | **Test ID / Target** | `HW-TEST-06`: `23-code-library/templates` (`170 MHz` Range 1 Boost blinky starter template) |
| **2** | **Git Commit & Profile** | Commit `543d21e`, `BUILD_TYPE=release` (`-O2 -g`, `2,616 B` Flash) |
| **3** | **Artifact SHA-256** | `.elf`: `35e0dbe01af2a10dc4519460ecaa4ed83b8ccc4e8d5730e7d4fd454f7c6e3787` \| `.bin`: `4f7a01c0ba864f5cf256904ab719079d1727ac6cce51ee30d7465521c14c7d63` |
| **4** | **Toolchain & Flags** | `arm-none-eabi-gcc 14.3.1`, `-O2 -g -Wall -Wextra -Werror` (`0` warnings / `0` errors) |
| **5** | **Hardware & Probe** | ST NUCLEO-G474RE (`MB1367 Rev C`), `STM32G474RET6`, STLINK-V3E `<redacted-stlink-sn>`, `VDD = 3.28 V`, SWD `8000 KHz` |
| **6** | **Flash Command** | `STM32_Programmer_CLI -c port=SWD mode=NORMAL -w firmware.elf -v -rst` |
| **7** | **Peripherals & Pins** | `clock_g4` (`170 MHz` Boost), `gpio_g4` (`PA5` `LD2`) |
| **8** | **Execution Mode** | Onboard post-validation safe restoration. |
| **9** | **Expected Criteria** | Clean Flash verification, SWD remains healthy and unlocked (`PA13`/`PA14` intact), `VDD = 3.28 V`, Device ID `0x469`, Rev X, `512 KB` NVM. |
| **10** | **Observed Output & Registers** | Post-flash SWD probe confirmed `Board: NUCLEO-G474RE`, `Voltage: 3.28V`, `SWD freq: 8000 KHz`, `Device ID: 0x469`, `Revision ID: Rev X`, `NVM size: 512 KBytes`. |
| **11** | **7-State Status** | `1.Static: PASS` \| `2.Host: N/A` \| `3.Cross-CC: PASS` \| `4.Uploaded: PASS` \| `5.Target Exec: PASS` \| `6.Physical Behavior: PENDING` *(visual `LD2` blink requires operator eye check)* \| `7.Instrument: N/A` |
| **12** | **Defects Resolved** | None (clean board health confirmed after all 5 test suites). |
| **13** | **Scope Limitations** | Zero Flash option-byte (`OPTR`/RDP/WRP/PCROP/BOOT0) modifications performed anywhere during testing. |
| **14** | **Pending User Actions** | None (board left in safe factory-equivalent blinky state). |

---

## 4. Critical Separations Audited (Zero Conflation Policy)

### 4.1 Lab 05 (`EXTI13`) — Software `SWIER1` vs Physical `PC13` / `B1` / `LD2`

| Layer | Hardware Path Exercised | Verified Status |
|---|---|---|
| **1. Software-Triggered EXTI (`EXTI->SWIER1`)** | `SYSCFGEN` + `SYSCFG->EXTICR[3]=0x20` + `SWIER1` -> `PR1` -> `IMR1` -> NVIC `EXTI15_10_IRQn` -> `(void)PR1; __DSB();` | **`TARGET EXECUTION VERIFIED` (`PASS`)** — `onboard_selftest` Test 4 (`swier_cb=3/3`, `last_line=13`, zero duplicate re-entry) |
| **2. Physical `PC13` Electrical Rising Edge** | External `0 V -> 3.3 V` Schmitt trigger -> `EXTICR[3]` input mux -> `RTSR1[13]` edge detector | **`PENDING — USER INTERACTION REQUIRED`** |
| **3. Actual `B1` Tactile Press / Release** | Mechanical switch press (rising triggers) vs release (falling ignored) & contact bounce | **`PENDING — USER INTERACTION REQUIRED`** |
| **4. Visual `LD2` (`PA5`) Illumination** | Human visual confirmation of `LD2` toggling per `B1` press | **`PENDING — USER INTERACTION REQUIRED`** |

### 4.2 Simulation Mode vs Physical Power Hardware (Capstone Project 03)

| Operating Mode | Compile Flag | What Is Verified on Silicon | What Remains Pending |
|---|---|---|---|
| **Onboard Safe Simulation** | `POWER_SIM_MODE=1` *(default)* | Real `STM32G474RET6` FreeRTOS `V11.1.0` 4-task scheduling, `1 kHz` Q15 PI math, `TIM1`/`COMP1`/`DAC1` register arming (`moe:1`), `IWDG` supervision, `USART2` JSON stream, software `buck_sim_step()` plant response. | Visual `LD2`, manual `B1` `TIM1->EGR=BG` break test, oscilloscope dead-time check on `PA8`/`PB13`. |
| **Physical Power Stage** | `POWER_SIM_MODE=0` | Cross-compiled & statically reviewed (`PA0` `ADC1_IN1` closed-loop sense + fail-safe `MOE=0` on ADC error). | **`PENDING — USER CONNECTION REQUIRED`** (requires `>= 100 MHz` scope `500 ns` dead-time verification BEFORE attaching isolated power stage + current-limited PSU + clamped `PA1`/`PA0` analog sensing). |

### 4.3 Summary of Deferred External Hardware & Bench Instruments

All **6 external-hardware labs** (`Labs 07, 09, 11, 12, 14, 16`), **2 driver external checks** (`spi_g4.c` full loopback/slave, `i2c_g4.c` live slave ACK/Repeated START/9-clock recovery), **3 capstone external extensions**, and **5 bench-instrument concepts** (`DIG-ELEC-006`, `G4-HRTIM-001`, `ANA-VREF-001`, `RT-PWR-002`, `HWSW-PWR-001`) are explicitly tracked with electrical safety rules (`<= 3.3 V`, `JP6` `IDD` only — NEVER `JP5`, `PA15`/`PB7` avoiding `PB8-BOOT0`) in [`HARDWARE_VALIDATION/PENDING_EXTERNAL_CONNECTIONS.md`](../HARDWARE_VALIDATION/PENDING_EXTERNAL_CONNECTIONS.md).
