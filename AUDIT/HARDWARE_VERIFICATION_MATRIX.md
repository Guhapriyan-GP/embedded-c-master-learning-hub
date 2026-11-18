# Physical NUCLEO-G474RE Hardware Verification Matrix

> **Date:** 2026-11-18 | **Branch:** `feature/complete-remediation`  
> **Board & Silicon:** ST **NUCLEO-G474RE** (`MB1367 Rev C`), `STM32G474RET6` (`Device ID = 0x469`, `Rev X` / `0x2003`, `512 KB` Flash, `96 KB` SRAM1+2, `32 KB` CCM-SRAM)  
> **Probe / Interface:** Onboard **STLINK-V3E** (`SN: 003700413335510133383531`, FW `V3J17M11`, SWD `8000 KHz`, VCP **`COM8`** `115200 8N1`, `VDD = 3.28–3.29 V`)  
> **Companion Reports:** [`AUDIT/HARDWARE_VALIDATION_SUMMARY.md`](HARDWARE_VALIDATION_SUMMARY.md), [`AUDIT/LAB_VERIFICATION_MATRIX.md`](LAB_VERIFICATION_MATRIX.md), [`HARDWARE_VALIDATION/PENDING_EXTERNAL_CONNECTIONS.md`](../HARDWARE_VALIDATION/PENDING_EXTERNAL_CONNECTIONS.md)

---

## 1. Onboard Silicon Tests Executed & Verified (Zero External Wiring)

### 1.1 Onboard Self-Test Suite (`HARDWARE_VALIDATION/onboard_selftest/build/onboard_selftest.elf` — 9/9 PASS)

| Test ID | Peripheral / Subsystem | Mapped Labs & Drivers | Observed Live Silicon Registers & Quantitative Evidence (`COM8` `115200 8N1`) | Status |
|---|---|---|---|---|
| **TEST 1** | MCU ID, CPUID, Flash Size, Reset Flags | Core Silicon (`stm32g474xx.h`) | `DBGMCU_IDCODE=0x20036469` (`DEV=0x469, REV=0x2003`), `SCB_CPUID=0x410FC241`, `FLASH_SIZE=512KB`, `RCC_CSR=0x14000000` | **`PASS`** |
| **TEST 2** | `GPIOA` (`PA5` `LD2`) & `GPIOC` (`PC13` `B1`) | Labs 01, 02; `gpio_g4.c` | `PA5(set=1 rst=0 tog=1)`, `GPIOA_MODER=0xABFFF7AF`, `PC13_MODER=0`, `PC13_IDR=0`, `GPIOC_MODER=0xF3FFFFFF` | **`PASS`** |
| **TEST 3A** | `160 MHz` Range 1 Boost (`HSI16` PLL `N=80`) + `DWT` | Labs 03, 04, 08; `clock_g4.c`, `usart_g4.c` | `SYSCLK=160000000Hz`, `PLLCFGR=0x31715032`, `CFGR=0x0000000F`, `PWR_CR1=0x00000200`, `PWR_CR5=0x00000000`, `PWR_SR2=0x00000100`, `FLASH_ACR=0x00040704`, `USART2_BRR=1389`, `DWT_10ms=1599997` (`-0.0002%`) | **`PASS`** |
| **TEST 3B** | `170 MHz` Range 1 Boost (`HSI16` PLL `N=85`) + `DWT` | Labs 03, 04, 08; `clock_g4.c`, `usart_g4.c` | `SYSCLK=170000000Hz`, `PLLCFGR=0x31715532`, `CFGR=0x0000000F`, `PWR_CR1=0x00000200`, `PWR_CR5=0x00000000`, `PWR_SR2=0x00000100`, `FLASH_ACR=0x00040704`, `USART2_BRR=1476`, `DWT_10ms=1699989` (`-0.0006%`) | **`PASS`** |
| **TEST 4** | `TIM2` Periodic IRQ (`rc_w0`) & `EXTI13` (`SWIER1`) | Labs 05, 06; `tim_g4.c`, `exti_g4.c` | `TIM2(PSC=25 ARR=65384 irqs_100ms=11)`, `EXTI13(swier_cb=3 last_line=13 SYSCFG_EXTICR4=0x00000020)` | **`PASS`** |
| **TEST 5A** | `DMA1` MEM2MEM Burst (`256 × 32-bit`) | Lab 10; `dma_g4.c` | `status=0`, `match=256/256`, `cycles=1977`, `DMA1_ISR=0x00000000` | **`PASS`** |
| **TEST 5B** | `ADC1` Internal `VREFINT` & Die Temperature | Labs 09*, 10; `adc_g4.c` | `ADC_CCR=0x00C30000`, `CALFACT=0x00000045`, `VREF_CAL=1657`, `VREF_RAW=1513`, **`VDDA=3288mV`**, `TS_CAL1=1037`, `TS_CAL2=1370`, `TEMP_RAW=956`, **`DIE_TEMP=32906mdegC`** (`32.91 °C`) | **`PASS`** |
| **TEST 5C** | `ADC1` + `DMAMUX1` + `DMA1_Ch1` Circular Ping-Pong | Lab 10; `adc_g4.c`, `dma_g4.c` | `DMAMUX0_CCR=0x00000005`, `HT=20`, `TC=20`, `TE=0`, `buf[0]_vref=1513`, `buf[1]_temp=955` | **`PASS`** |
| **TEST 6A** | `CORDIC` `q1.31` `cos`/`sin` vs Cortex-M4F FPU | Lab 17 | `pi/4 cos=0.707106 sin=0.707106`; 1×: `28` vs `584` cyc (`20.9×`); 256×: `8511` vs `82716` cyc (`9.72×`); `max_err=2E-6` | **`PASS`** |
| **TEST 6B** | `FMAC` 8-Tap `q1.15` FIR Filter vs Software | Lab 17 | `y[0]=2000`, `y[7]=16000`, `y[31]=16000`, `sw[31]=16000`, `max_diff=0LSB`, `FMAC=2055 cyc vs SW=4766 cyc` (`2.32×`), `FMAC_SR=0x00000001` | **`PASS`** |
| **TEST 7** | Hardware `CRC-32` & `FDCAN1` 16 B BRS Loopback | Lab 18 | `CRC_VEC=0xCBF43926`, `RX_ID=0x123`, `DLC=10(16B)`, `FDF=1`, `BRS=1`, `PAYLOAD_CRC=0x74818A7E`, `MATCH=1`, `NBTP=0x0A091A05`, `DBTP=0x00040B33`, `ECR=0x00000000` | **`PASS`** |
| **TEST 8A/8B** | 8-Region `MPU` Arming + `DIV0` `.noinit` Reboot | Lab 19; `fault_handler.c` | Phase 1: `MPU_CTRL=0x00000005`, `RBAR=0x200021A7`, `RASR=0x10000009`, `SHCSR=0x00070000`, `CCR=0x00000210`; Phase 2 post-reset: `magic=0xFA017C0D`, `CFSR=0x02000000`, `HFSR=0`, `PC=0x08001A60`, `LR=0x08002145`, `SP=0x20017F48`, `flags=0x00000084` | **`PASS`** |
| **TEST 9** | `IWDG` Safe Kick + `DBGMCU` Freeze | Lab 13; `iwdg_g4.c` | `status=0`, `PR=2`, `RLR=3999`, `DBGMCU_APB1FZR1=0x00001800`, `kicks=6/6 (300ms no reset)` | **`PASS`** |

### 1.2 FreeRTOS `V11.1.0` Reference Firmware (`23-code-library/freertos/build/nucleo_g474re_freertos.elf` — 6/6 PASS)

| Test ID | RTOS Mechanism Verified | Observed Live Silicon Output (`COM8` `115200 8N1`, `8.0 s` Capture) | Status |
|---|---|---|---|
| **RTOS 1** | `NVIC_CONFIG` (`PRIGROUP=0`, `BASEPRI=0x50`) | `PRIGROUP=0 PendSV=0xF0 SysTick=0xF0 TIM2_PRIO=6` (`>= MAX_SYSCALL_PRIO=5`) | **`PASS`** |
| **RTOS 2** | Binary & Counting Semaphores | `binary=OK counting_max3=OK` | **`PASS`** |
| **RTOS 3** | Mutex Priority Inheritance Protocol (PIP) | `base_prio=1 promoted_prio=5 restored_prio=1 high_acq=1` | **`PASS`** |
| **RTOS 4** | Deadlock Avoidance (`A->B` Lock Order + Timeout) | `timeout_recovered=YES ordered_lock(A->B)=OK` | **`PASS`** |
| **RTOS 5** | `TIM2` `10 Hz` `FromISR` + Queue IPC + FPU + SwTimer | `tim2_isr=4 notify=4 q_tx=5 q_rx=4 sw_tmr=4 fpu_err=0` | **`PASS`** |
| **RTOS 6** | Stack High-Water Mark & `heap_4` Stability | `Sup=339 Cons=309 Prod=186 Mon=459 words_free (heap_free=21104 B)` + `8 s` `[TELEM]` `wdg=2..9`, `heap=21104/21104` | **`PASS`** |

### 1.3 Capstone Projects (`19-projects/01..03`) — Onboard Silicon Modes

| Project | Firmware ELF | Onboard Silicon Observations (`COM8` `115200 8N1`) | Onboard Status | External Hardware Extensions |
|---|---|---|---|---|
| **Project 01** | `smart_instrument_panel.elf` | `170 MHz`, `VDDA=3286..3288 mV`, `Die Temp=32.5..33.2 °C`, `20 kHz` PWM (`ARR=8499`), exact `500 ms` JSON, live RX `'s'` cmd via SPSC `ring_buffer_t` (`rx_cmds: 0 -> 1`) | **`PASS`** | Ext `PA0` pot, `PA1` PWM load/scope, manual `B1`: `PENDING — USER CONNECTION REQUIRED` |
| **Project 02** | `dma_data_logger.elf` | `CRC32_TEST=0xCBF43926 (PASS)`, `FLASH_OPTR.DBANK=1`, clean `I2C1` absent-slave `NACKF`, `25×` consecutive `160 ms` `$LOG2` ping-pong DMA frames (`ovr=0, te=0, ok=1`) | **`PASS`** | Ext `PA15`/`PB7` SSD1306 OLED: `PENDING — USER CONNECTION REQUIRED` |
| **Project 03** | `freertos_digital_power_ctrl.elf` | FreeRTOS `V11.1.0` 4-task `170 MHz`, Q15 PI tracking `5000 mV` (`vout_mv=4989..5021`, `ccr1=355`), `TIM1`/`COMP1` armed (`moe:1, fault:0`), `wdg: 1..19`, `hwm:[217,337,325,351]` | **`PASS`** | Ext scope on `PA8`/`PB13`, manual `B1`, isolated power stage (`POWER_SIM_MODE=0`), OLED: `PENDING — USER CONNECTION REQUIRED` |

---

## 2. Reusable Driver Library Silicon Coverage (`23-code-library/Drivers/` — 12 Drivers)

| # | Driver | Host Tests | Cross-Compile (`-Werror`) | Onboard Silicon Verified | External Hardware Remaining |
|---|---|---|---|---|---|
| 1 | `clock_g4.c/.h` | `N/A` | `PASS` | `PASS` (`160` & `170 MHz` `HSI16` Boost + `DWT`) | Optional `24 MHz` HSE `X3` + `MCO` counter: `PENDING — USER CONNECTION REQUIRED` |
| 2 | `gpio_g4.c/.h` | `PASS` (`test_gpio_mock`) | `PASS` | `PASS` (`PA5`, `PC13`, `PA2`/`PA3`, `PA15`/`PB7`) | Optional `OSPEEDR` scope check: `PENDING — USER CONNECTION REQUIRED` |
| 3 | `exti_g4.c/.h` | `PASS` (`test_drivers_math`) | `PASS` | `PASS` (`EXTI13` `SWIER1`, `EXTICR4=0x20`, `(void)PR1; __DSB()`) | Optional manual `B1` actuation |
| 4 | `tim_g4.c/.h` | `PASS` (`test_drivers_math`) | `PASS` | `PASS` (`TIM2` `100 Hz` & `10 Hz`, `TIM3` TRGO, `TIM2_CH2` `20 kHz` PWM init) | External `PA6`/`PA7`/`PB0` RGB LED + scope: `PENDING — USER CONNECTION REQUIRED` |
| 5 | `usart_g4.c/.h` | `PASS` (`test_drivers_math`) | `PASS` | `PASS` (`USART2` `COM8` `160`/`170 MHz` TX + RX ISR) | None (fully verified onboard) |
| 6 | `ring_buffer.c/.h` | `PASS` (104/104) | `PASS` | `PASS` (Project 01 `USART2` RX `'s'` command) | None (fully verified onboard) |
| 7 | `adc_g4.c/.h` | `PASS` (`test_drivers_math`) | `PASS` | `PASS` (`VREFINT`, Die Temp, `CALFACT=0x45`, `VDDA=3288 mV`) | External `PA0` `10 kΩ` pot sweep: `PENDING — USER CONNECTION REQUIRED` |
| 8 | `dma_g4.c/.h` | `PASS` (`test_drivers_math`) | `PASS` | `PASS` (`MEM2MEM` `256/256` + `ADC1` circular `HT`/`TC`) | None (fully verified onboard) |
| 9 | `iwdg_g4.c/.h` | `PASS` (`test_drivers_math`) | `PASS` | `PASS` (`PR=2, RLR=3999`, `DBG_IWDG_STOP=1`, `6/6` kicks) | None (fully verified onboard) |
| 10 | `fault_handler.c/.h` | `PASS` (`test_gpio_mock`) | `PASS` | `PASS` (`DIV0` `.noinit` `magic=0xFA017C0D`, `CFSR=0x02000000`) | Optional MPU `MemManage` write-trap extension |
| 11 | `i2c_g4.c/.h` | `PASS` (`test_drivers_math`) | `PASS` | `PARTIAL` (bounded absent-slave `NACKF` verified in Project 02) | External `3.3 V` I2C slave + `4.7 kΩ` pull-ups + 9-clk recovery: `PENDING — USER CONNECTION REQUIRED` |
| 12 | `spi_g4.c/.h` | `PASS` (`test_drivers_math`) | `PASS` | `PENDING — USER CONNECTION REQUIRED` | External `PB4 <-> PB5` loopback jumper or `3.3 V` SPI slave: `PENDING — USER CONNECTION REQUIRED` |

---

## 3. Deferred External Experiments & Bench Measurements (`PENDING — USER CONNECTION REQUIRED`)

All deferred hardware tests are cataloged with wiring tables, electrical safety limits ($V_{\text{DD}} \le 3.3\text{ V}$, `JP6` `IDD` only — NEVER `JP5`, `PA15`/`PB7` avoiding `PB8-BOOT0`), and pass/fail criteria in [`HARDWARE_VALIDATION/PENDING_EXTERNAL_CONNECTIONS.md`](../HARDWARE_VALIDATION/PENDING_EXTERNAL_CONNECTIONS.md):

| Category | Item | Required External Hardware / Bench Instrument | Status |
|---|---|---|---|
| **Lab 07** | `TIM3` `25 kHz` RGB Hardware PWM | Common-cathode RGB LED + 3 × `330 Ω` resistors (`PA6`/`PA7`/`PB0`) + scope | `PENDING — USER CONNECTION REQUIRED` |
| **Lab 09** | `ADC1` Single-Channel Potentiometer | `10 kΩ` potentiometer on `PA0` (`0..3.3 V`) + DMM | `PENDING — USER CONNECTION REQUIRED` |
| **Lab 11** | `SPI1` Master Loopback / Sensor | `PB4 <-> PB5` jumper or `3.3 V` SPI Mode 3 sensor (`PB3..PB6`) + logic analyzer | `PENDING — USER CONNECTION REQUIRED` |
| **Lab 12** | `I2C1` `400 kHz` SSD1306 OLED & 9-Clk Recovery | `3.3 V` SSD1306 OLED (`0x3C`) + 2 × `4.7 kΩ` pull-ups (`PA15`/`PB7`) + scope | `PENDING — USER CONNECTION REQUIRED` |
| **Lab 14** | `Stop 0` Low-Power Wakeup & `JP6` `IDD` | Manual physical `B1` (`PC13`) press + precision µA DMM / PPK2 across **`JP6`** | `PENDING — USER CONNECTION REQUIRED` |
| **Lab 16** | `DAC1` + `OPAMP1` + `COMP1` -> `TIM1_BKIN` | `0..3.0 V` stimulus on `PA1` + `>= 100 MHz` 2-ch scope on `PA8`/`PA4` (`< 50 ns`) | `PENDING — USER CONNECTION REQUIRED` |
| **Concept `DIG-ELEC-006`** | `OSPEEDR` Slew-Rate Ringing & `33 Ω` Damping | `>= 200 MHz` oscilloscope with short spring ground clip (`PA8` MCO / `PB3`) | `PENDING — USER CONNECTION REQUIRED` |
| **Concept `G4-HRTIM-001`** | `HRTIM1` `184 ps` Fine Edge Resolution & DLL | `>= 500 MHz` oscilloscope on `PA8`/`PA9` (`AF13`) | `PENDING — USER CONNECTION REQUIRED` |
| **Concept `ANA-VREF-001`** | `VREFBUF` (`2.048`/`2.500 V`) Output Accuracy | `>= 5½`-digit DMM + UM2505 `SB11`/`SB12` bridge check BEFORE enabling `ENVR` | `PENDING — USER CONNECTION REQUIRED` |
| **Concept `RT-PWR-002`** | Stop 0 Leakage Delta (`MODER=00b` vs `11b`) | Precision µA DMM / PPK2 across **`JP6`** (SWD detached) | `PENDING — USER CONNECTION REQUIRED` |
| **Concept `HWSW-PWR-001`** | Full NUCLEO-G474RE Power Profile (5 States) | Precision µA/mA DMM / Joulescope across **`JP6`** (Run 170/160, Sleep, Stop 0/1) | `PENDING — USER CONNECTION REQUIRED` |
