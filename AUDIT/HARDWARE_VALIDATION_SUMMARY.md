# Physical NUCLEO-G474RE Hardware Validation Summary

> **Date:** 2026-11-18  
> **Hardware Owner:** Agent 4 — Hardware Validation (Exclusive Sole Board Owner)  
> **Board & Silicon:** ST **NUCLEO-G474RE** (`MB1367 Rev C`), `STM32G474RET6` (ARM Cortex-M4F, `Device ID = 0x469`, `Rev X` / `0x2003`, `512 KB` Flash, `96 KB` SRAM1+2, `32 KB` CCM-SRAM)  
> **Debugger / VCP:** Onboard **STLINK-V3E** (`SN: 003700413335510133383531`, FW `V3J17M11`, SWD `8000 KHz`, **COM8** `115200 8N1`, Bootloader `0xD5`, `VDD = 3.28–3.29 V`)  
> **Safety Compliance:** Zero Flash option-byte (`OPTR`/RDP/WRP/PCROP/SEC/BOOT0) modifications, zero full mass erases, zero SWD (`PA13`/`PA14`) reconfiguration, zero external wires assumed. Board restored to clean 170 MHz blinky template (`23-code-library/templates/build/firmware.elf`) after all tests.

---

## 1. Executive Pass / Fail Summary Matrix

| # | Firmware Artifact | Build Profile | Real Hardware Result | Key Measured Silicon Evidence (`COM8`, `115200 8N1`) |
|---|---|---|---|---|
| 1 | `HARDWARE_VALIDATION/onboard_selftest/build/onboard_selftest.elf` | `debug` (`-Og -g3`, 0 warn, `23,344 B` Flash) | **`ONBOARD HARDWARE TESTED — 9/9 PASS`** | `160 MHz` (`1,599,997` cyc/10 ms) & `170 MHz` (`1,699,989` cyc/10 ms), `VDDA = 3288 mV`, `Die Temp = 32.91 °C`, `CORDIC` `9.72×` faster than FPU (`2e-6` max err), `FMAC` `0 LSB` diff, `CRC-32 = 0xCBF43926`, `FDCAN1` 16 B BRS loopback `ECR=0`, `MPU` + `DIV0` `.noinit` reboot (`CFSR=0x02000000`), `IWDG` `6/6` kicks |
| 2 | `23-code-library/freertos/build/nucleo_g474re_freertos.elf` | `debug` (`-Og -g3`, 0 warn, `21,480 B` Flash) | **`ONBOARD HARDWARE TESTED — 6/6 PASS`** | FreeRTOS `V11.1.0` (`ARM_CM4F`, `heap_4`): `NVIC_CONFIG`, `SEMAPHORES`, `PRIORITY_INHERITANCE`, `DEADLOCK_GUARD`, `FROM_ISR_AND_QUEUE` (`fpu_err=0`), `STACK_HWM` all PASS + 8 s live `[TELEM]` stream (`wdg=2..9`, `heap=21104/21104`) |
| 3 | `19-projects/01-beginner-smart-instrument-panel/build/smart_instrument_panel.elf` | `debug` (`-Og -g3`, 0 warn, `12.34 KB`) | **`ONBOARD HARDWARE TESTED — PASS`** *(after fixing real-silicon `GPIO_EnableClock()` defect!)* | `170 MHz`, `VDDA = 3286..3288 mV`, `Die Temp = 32.5..33.2 °C`, `20 kHz` PWM (`ARR=8499`), exact `500 ms` JSON telemetry, live UART `'s'` RX command parsed via SPSC `ring_buffer_t` (`rx_cmds: 0 -> 1`). Ext pot/load: `EXTERNAL HARDWARE PENDING` |
| 4 | `19-projects/02-intermediate-dma-data-logger/build/dma_data_logger.elf` | `debug` (`-Og -g3`, 0 warn, `11.43 KB`) | **`ONBOARD HARDWARE TESTED — PASS`** *(after `GPIO_EnableClock()` fix)* | `CRC32_TEST=0xCBF43926 (PASS)`, `FLASH_OPTR.DBANK=1` (read-only), `I2C1 OLED ABSENT` clean `NACKF` handling, 25 consecutive `$LOG2` frames at exact `160 ms` (`half=0/1`, `ovr=0`, `te=0`, `ok=1`). Ext OLED: `EXTERNAL HARDWARE PENDING` |
| 5 | `19-projects/03-advanced-freertos-digital-power-controller/build/freertos_digital_power_ctrl.elf` | `debug` (`-Og -g3`, 0 warn, `20.89 KB`) | **`ONBOARD HARDWARE TESTED — PASS`** *(after `GPIO_EnableClock()` fix)* | FreeRTOS `V11.1.0` 4-task execution at `170 MHz`, Q15 PI regulator tracking `5000 mV` (`vout_mv = 4989..5021`, `ccr1 = 355`), `TIM1`/`COMP1` armed (`moe:1`, `fault:0`), `wdg: 1..19`, healthy `hwm:[217,337,325,351]`. Ext power stage/OLED: `EXTERNAL HARDWARE PENDING` |
| 6 | `23-code-library/templates/build/firmware.elf` | `release` (`2.55 KB`) | **`RESTORED & VERIFIED HEALTHY`** | Clean 170 MHz blinky template flashed & verified; post-flash SWD probe confirmed `3.28 V`, `8000 KHz`, Device ID `0x469`, Rev X, 512 KB Flash |

---

## 2. Genuine Real-Silicon Defects Caught & Resolved During Bring-Up

Physical hardware execution caught **two authentic issues** that passed static review and strict `-Wall -Wextra -Werror` compilation:

1. **Missing `GPIO_EnableClock()` Calls in Capstone Projects 01, 02, and 03 (CRITICAL SILICON DEFECT — FIXED):**
   - **Observed Symptom:** Initial flash of `19-projects/01-beginner-smart-instrument-panel/build/smart_instrument_panel.elf` succeeded cleanly (`Download verified successfully`), `SCB->CFSR` read `0x00000000` (no CPU fault), yet **`COM8` captured `0` bytes** over 4.2 seconds.
   - **Root Cause:** `23-code-library/Drivers/gpio_g4.c` requires callers to invoke `GPIO_EnableClock(GPIOx)` before `GPIO_Config*()`. All three capstones (`01`, `02`, `03`) called `GPIO_ConfigOutput()` / `GPIO_ConfigAF()` / `GPIO_ConfigInput()` without enabling `RCC->AHB2ENR` (`GPIOAEN`/`GPIOBEN`/`GPIOCEN`), so all GPIO register writes were silently ignored by the AHB2 bus and `PA2` (`USART2_TX`) remained in STM32G4's `0b11` Analog reset state.
   - **Clean Fix:** Added explicit `GPIO_EnableClock()` calls in `19-projects/01-beginner-smart-instrument-panel/Src/main.c` (`GPIOA`, `GPIOC`), `19-projects/02-intermediate-dma-data-logger/Src/main.c` (`GPIOA`, `GPIOB`), and `19-projects/03-advanced-freertos-digital-power-controller/Src/main.c` (`GPIOA`, `GPIOB`, `GPIOC`), rebuilt all 3 projects with 0 warnings / 0 errors, re-flashed, and verified live `COM8` output on all 3!

2. **Intra-Object Linker Resolution of `--wrap=HardFault_Handler_C` in Initial Self-Test Build (FIXED):**
   - **Observed Symptom:** On initial run of `onboard_selftest.elf`, Tests 1–8A passed and triggered `sdiv` divide-by-zero, which entered `23-code-library/Drivers/fault_handler.c`'s real `HardFault_Handler_C()` (verified live via `STM32_Programmer_CLI -r32 0x20002398 0x44`: `magic = 0xFA017C0D`, `r3 = 0x2A`, `pc = 0x08001A0C`, `cfsr = 0x02000000`, `fault_flags = 0x00000084`, `stacked_sp = 0x20017F48`), where `fault_handler.c` intentionally spins in `while (1)` for debugger inspection instead of rebooting.
   - **Root Cause:** GNU `ld --wrap=HardFault_Handler_C` only intercepts undefined symbol references, whereas `startup_stm32g474re.c` contains a weak intra-object `HardFault_Handler_C` definition.
   - **Clean Fix:** Overrode the weak `UsageFault_Handler` vector directly in `HARDWARE_VALIDATION/onboard_selftest/Src/main.c` with a naked MSP/PSP trampoline calling `Selftest_UsageFault_C()` (leaving `23-code-library/Drivers/fault_handler.c` completely unmodified!), which populates `fault_handler.c`'s `.noinit` `g_crash_log` and executes `NVIC_SystemReset()`, yielding full automated Phase 1 -> Reboot -> Phase 2 -> Test 9 verification on `COM8`.

---

## 3. Verbatim Hardware Test Logs (`COM8`, `115200 8N1`)

### 3.1 Onboard Hardware Self-Test Suite (`HARDWARE_VALIDATION/onboard_selftest/build/onboard_selftest.elf`)

- **Build:** `arm-none-eabi-gcc 14.3.1`, `-std=c11 -Og -g3 -Wall -Wextra -Werror -Wpedantic -Wshadow -Wconversion -Wdouble-promotion -Wundef`, 0 warnings / 0 errors (`FLASH = 23,344 B [4.45%]`, `RAM = 10,752 B [10.94%]`).
- **Flash:** `STM32_Programmer_CLI -c port=SWD mode=NORMAL -w onboard_selftest.elf -v -rst` (`VDD = 3.29 V`, verified in `0.065 s`).

```text
=== NUCLEO-G474RE ONBOARD HARDWARE SELF-TEST (PHASE 1/2) ===
[PASS] TEST1_MCU_ID: DBGMCU_IDCODE=0x20036469 (DEV=0x469 REV=0x2003) SCB_CPUID=0x410FC241 FLASH_SIZE=512KB RCC_CSR=0x14000000
[PASS] TEST2_GPIO_LD2_B1: PA5(set=1 rst=0 tog=1) GPIOA_MODER=0xABFFF7AF PC13_MODER=0 PC13_IDR=0 GPIOC_MODER=0xF3FFFFFF
[PASS] TEST3A_CLOCK_160MHZ: SYSCLK=160000000Hz RCC_CR=0x03000500 PLLCFGR=0x31715032 CFGR=0x0000000F PWR_CR1=0x00000200 PWR_CR5=0x00000000 PWR_SR2=0x00000100 FLASH_ACR=0x00040704 USART2_BRR=1389 DWT_10ms=1599997
[PASS] TEST3B_CLOCK_170MHZ: SYSCLK=170000000Hz RCC_CR=0x03000500 PLLCFGR=0x31715532 CFGR=0x0000000F PWR_CR1=0x00000200 PWR_CR5=0x00000000 PWR_SR2=0x00000100 FLASH_ACR=0x00040704 USART2_BRR=1476 DWT_10ms=1699989
[PASS] TEST4_TIM2_AND_EXTI13: TIM2(PSC=25 ARR=65384 irqs_100ms=11) EXTI13(swier_cb=3 last_line=13 SYSCFG_EXTICR4=0x00000020)
[PASS] TEST5A_DMA1_MEM2MEM: status=0 match=256/256 cycles=1977 DMA1_ISR=0x00000000
[PASS] TEST5B_ADC1_VREF_TEMP: ADC_CCR=0x00C30000 CALFACT=0x00000045 VREF_CAL=1657 VREF_RAW=1513 VDDA=3288mV TS_CAL1=1037 TS_CAL2=1370 TEMP_RAW=956 DIE_TEMP=32906mdegC
[PASS] TEST5C_ADC1_DMA1_CIRC: DMAMUX0_CCR=0x00000005 HT=20 TC=20 TE=0 buf[0]_vref=1513 buf[1]_temp=955
[PASS] TEST6A_CORDIC_VS_FPU: pi/4 cos=0.707106 sin=0.707106 (1x: CORDIC=28 cyc vs FPU=584 cyc; 256x: CORDIC=8511 vs FPU=82716; max_err=2E-6)
[PASS] TEST6B_FMAC_FIR: y[0]=2000 y[7]=16000 y[31]=16000 sw[31]=16000 max_diff=0LSB FMAC=2055 cyc vs SW=4766 cyc FMAC_SR=0x00000001
[PASS] TEST7_FDCAN1_AND_CRC32: CRC_VEC=0xCBF43926 RX_ID=0x123 DLC=10(16B) FDF=1 BRS=1 PAYLOAD_CRC=0x74818A7E MATCH=1 NBTP=0x0A091A05 DBTP=0x00040B33 ECR=0x00000000
[PASS] TEST8A_MPU_ARMED: MPU_CTRL=0x00000005 RBAR=0x200021A7 RASR=0x10000009 SHCSR=0x00070000 CCR=0x00000210 benign_read=0x12345678 -> triggering DIV0 UsageFault...

=== NUCLEO-G474RE ONBOARD HARDWARE SELF-TEST (PHASE 2/2: POST-FAULT REBOOT) ===
[PASS] TEST8B_FAULT_PERSISTENCE_VERIFIED: magic=0xFA017C0D CFSR=0x02000000 HFSR=0x00000000 PC=0x08001A60 LR=0x08002145 SP=0x20017F48 flags=0x00000084 RCC_CSR=0x14000000
[PASS] TEST9_IWDG_SAFE_KICK: status=0 PR=2 RLR=3999 DBGMCU_APB1FZR1=0x00001800 kicks=6/6 (300ms no reset)
[SUMMARY] ONBOARD_SELFTEST_ALL_PASSED (9/9 TESTS, 0 FAILURES)
```

#### Real Silicon Quantitative Highlights
- **Clock Accuracy (`SysTick` vs `DWT->CYCCNT`):** `160 MHz` measured `1,599,997` cycles / `10 ms` (`-0.0002%`); `170 MHz` measured `1,699,989` cycles / `10 ms` (`-0.0006%`); `PWR->CR5 = 0x00000000` (`R1MODE=0` Boost) and `FLASH->ACR = 0x00040704` (`4 WS`, ART Prefetch/ICache/DCache) verified on both profiles.
- **Internal Analog & Factory Calibration:** `VREFINT_CAL = 1657`, `VREF_RAW = 1513` $\implies$ **`VDDA = 3288 mV`** (matches ST-LINK `3.28–3.29 V` measurement within `2 mV`!); `TS_CAL1 = 1037`, `TS_CAL2 = 1370`, `TEMP_RAW = 956` $\implies$ **`Die Temp = 32.91 °C`**.
- **Hardware Math Coprocessors (`-Og`):** `CORDIC` achieved **`20.9×`** single-call speedup (`28` vs `584` cycles) and **`9.72×`** 256-angle throughput speedup (`8,511` vs `82,716` cycles, `max_err = 2e-6`); `FMAC` 8-tap `q1.15` FIR achieved **`0 LSB`** bit-exact match and **`2.32×`** speedup (`2,055` vs `4,766` cycles) even under CPU polling.
- **CAN-FD + IEEE 802.3 CRC-32:** Hardware `CRC-32("123456789") = 0xCBF43926` exact; `FDCAN1` 16-byte CAN-FD frame (`500 kbps` nominal / `2.000 Mbps` BRS) transmitted and received through Message RAM (`0x4000A400`) with `ECR = 0x00000000` and byte/CRC match.

---

### 3.2 FreeRTOS `V11.1.0` Reference Firmware (`23-code-library/freertos/build/nucleo_g474re_freertos.elf`)

- **Flash:** `STM32_Programmer_CLI -c port=SWD mode=NORMAL -w nucleo_g474re_freertos.elf -v -rst` (`21.16 KB`, `VDD = 3.28 V`, verified in `0.060 s`).
- **Capture Duration:** `8.0 s` live on `COM8` (`115200 8N1`).

```text
============================================================
NUCLEO-G474RE FreeRTOS V11.1.0 Onboard Verification
SYSCLK=170000000 Hz (clk=OK) | PRIGROUP=0 | MAX_SYSCALL_PRIO=5
RCC_CSR=0x14000000 (IWDGRST=0) | Port=GCC/ARM_CM4F | Heap=heap_4
============================================================
[PASS] NVIC_CONFIG: PRIGROUP=0 PendSV=0xF0 SysTick=0xF0 TIM2_PRIO=6
[PASS] SEMAPHORES: binary=OK counting_max3=OK
[PASS] PRIORITY_INHERITANCE: base_prio=1 promoted_prio=5 restored_prio=1 high_acq=1
[PASS] DEADLOCK_GUARD: timeout_recovered=YES ordered_lock(A->B)=OK
[PASS] FROM_ISR_AND_QUEUE: tim2_isr=4 notify=4 q_tx=5 q_rx=4 sw_tmr=4 fpu_err=0
[PASS] STACK_HWM: Sup=339 Cons=309 Prod=186 Mon=459 words_free (heap_free=21104 B)
[SUMMARY] FREERTOS_SELF_TESTS: 6/6 ALL_PASSED
[PASS] IWDG_SUPERVISOR: enabled (~3000 ms, DBG_IWDG_STOP=1)
[TELEM t=1010 ms] q_tx=11 q_rx=10 tim2=10 btn=0 wdg=2 heap=21104/21104 HWM(S=339 C=309 P=186 M=443)
[TELEM t=2010 ms] q_tx=21 q_rx=20 tim2=20 btn=0 wdg=3 heap=21104/21104 HWM(S=339 C=309 P=186 M=324)
[TELEM t=3010 ms] q_tx=31 q_rx=30 tim2=30 btn=0 wdg=4 heap=21104/21104 HWM(S=339 C=309 P=186 M=324)
[TELEM t=4010 ms] q_tx=41 q_rx=40 tim2=40 btn=0 wdg=5 heap=21104/21104 HWM(S=339 C=309 P=186 M=324)
[TELEM t=5010 ms] q_tx=51 q_rx=50 tim2=50 btn=0 wdg=6 heap=21104/21104 HWM(S=339 C=309 P=186 M=324)
[TELEM t=6010 ms] q_tx=61 q_rx=60 tim2=60 btn=0 wdg=7 heap=21104/21104 HWM(S=339 C=309 P=186 M=324)
[TELEM t=7010 ms] q_tx=71 q_rx=70 tim2=70 btn=0 wdg=8 heap=21104/21104 HWM(S=339 C=309 P=186 M=324)
[TELEM t=8010 ms] q_tx=81 q_rx=80 tim2=80 btn=0 wdg=9 heap=21104/21104 HWM(S=339 C=309 P=186 M=324)
```

---

### 3.3 Capstone Project 01 — Smart Instrument Panel (`smart_instrument_panel.elf`)

- **Interactive RX Test:** Sent ASCII `'s'` over `COM8` at `t = 1.8 s` to verify `USART2_IRQHandler` -> lock-free C11 SPSC `ring_buffer_t` -> non-blocking command parser.

```text
============================================================
Project 01: Smart Instrument Panel & Telemetry Unit (G474RE)
Default Mode: MODE_INTERNAL_TELEM (Zero external wires)
Controls: B1 (PC13) or UART 'm'=mode, 's'=status, 'h'=help
============================================================

--- [PANEL STATUS] ---
  SYSCLK      : 170000000 Hz
  Mode        : INTERNAL_TELEM (B1 or UART 'm')
  VDDA        : 3288 mV (VREFINT)
  Die Temp    : 32.5 C (TS_CAL1/2)
  Ext Pot PA0 : raw=0 (0 mV) [EXT PENDING]
  PWM PA1 AF1 : 32.6% @ 20000 Hz (ARR=8499)
  B1 Presses  : 0 | RX Cmds: 0 | ADC Err: 0
  Prev Reset  : IWDG=0 | HardFaultLog=0
----------------------
{"tick_ms":527,"clk_mhz":170,"mode":"INTERNAL_TELEM","vdda_mv":3288,"temp_c":32.9,"pot_raw":0,"pot_mv":0,"pwm_permille":332,"rx_cmds":0}
{"tick_ms":1027,"clk_mhz":170,"mode":"INTERNAL_TELEM","vdda_mv":3286,"temp_c":33.0,"pot_raw":0,"pot_mv":0,"pwm_permille":334,"rx_cmds":0}
{"tick_ms":1527,"clk_mhz":170,"mode":"INTERNAL_TELEM","vdda_mv":3286,"temp_c":32.7,"pot_raw":0,"pot_mv":0,"pwm_permille":328,"rx_cmds":0}

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
{"tick_ms":2527,"clk_mhz":170,"mode":"INTERNAL_TELEM","vdda_mv":3286,"temp_c":33.0,"pot_raw":0,"pot_mv":0,"pwm_permille":334,"rx_cmds":1}
{"tick_ms":3027,"clk_mhz":170,"mode":"INTERNAL_TELEM","vdda_mv":3288,"temp_c":32.9,"pot_raw":0,"pot_mv":0,"pwm_permille":332,"rx_cmds":1}
{"tick_ms":3527,"clk_mhz":170,"mode":"INTERNAL_TELEM","vdda_mv":3288,"temp_c":32.9,"pot_raw":0,"pot_mv":0,"pwm_permille":332,"rx_cmds":1}
{"tick_ms":4027,"clk_mhz":170,"mode":"INTERNAL_TELEM","vdda_mv":3286,"temp_c":33.0,"pot_raw":0,"pot_mv":0,"pwm_permille":334,"rx_cmds":1}
```

---

### 3.4 Capstone Project 02 — High-Speed DMA Data Logger (`dma_data_logger.elf`)

- **Capture Duration:** `4.0 s` live on `COM8` (`25` consecutive `160 ms` ping-pong frames, `0` overruns, `0` transfer errors).

```text
============================================================
Project 02: High-Speed DMA Data Logger + CRC-32 (G474RE)
[BOOT] SYSCLK=170000000 Hz | CRC32_TEST=0xCBF43926 (PASS)
[BOOT] FLASH_OPTR.DBANK=1 (dry-run Bank 2 0x08040000 staging)
[BOOT] I2C1 OLED (PA15/PB7 0x3C): ABSENT (EXTERNAL HARDWARE PENDING)
============================================================
$LOG2,seq=0,t_ms=192,half=0,vdda_mv=3286,temp=32.3C,raw=[954,954,956],ovr=0,te=0,b2=0x08040000,crc32=0xDBB3E973,ok=1
$LOG2,seq=1,t_ms=352,half=1,vdda_mv=3288,temp=32.5C,raw=[954,954,956],ovr=0,te=0,b2=0x08040020,crc32=0xDD6FFABA,ok=1
$LOG2,seq=2,t_ms=512,half=0,vdda_mv=3286,temp=32.3C,raw=[954,954,956],ovr=0,te=0,b2=0x08040040,crc32=0x2835C3B7,ok=1
$LOG2,seq=3,t_ms=672,half=1,vdda_mv=3286,temp=32.7C,raw=[955,955,955],ovr=0,te=0,b2=0x08040060,crc32=0x77ECC8B8,ok=1
... (21 more consecutive frames at exact 160 ms cadence, half=0/1 alternating, ovr=0, te=0, ok=1) ...
$LOG2,seq=23,t_ms=3872,half=1,vdda_mv=3288,temp=33.2C,raw=[955,956,957],ovr=0,te=0,b2=0x080402E0,crc32=0xCEA20C29,ok=1
$LOG2,seq=24,t_ms=4032,half=0,vdda_mv=3286,temp=33.0C,raw=[956,956,957],ovr=0,te=0,b2=0x08040300,crc32=0x3CCCB8CF,ok=1
```

---

### 3.5 Capstone Project 03 — FreeRTOS Digital Power Controller (`freertos_digital_power_ctrl.elf`)

- **Capture Duration:** `5.2 s` live on `COM8` (`POWER_SIM_MODE=1` safe onboard simulation; `10` consecutive `500 ms` frames).

```text
============================================================
Project 03: FreeRTOS Digital Power Controller (STM32G474RE)
  Clock : 170 MHz Boost | PRIGROUP=0 | FreeRTOS V11.1.0
  PWM   : TIM1 PA8/PB13 50 kHz Complementary, 500 ns dead-time
  Break : COMP1 -> TIM1->AF1.BKCMP1E (<50 ns MOE trip, AOE=0)
  Mode  : ONBOARD SAFE SIMULATION (Ext Power Stage PENDING)
  OLED  : ABSENT (EXTERNAL HARDWARE PENDING)
  B1    : Short press=setpoint/reset fault, Long press=BG trip
============================================================
{"t_ms":502,"sim":1,"vref_mv":5000,"vout_mv":5020,"duty_pm":209,"ccr1":355,"moe":1,"fault":0,"brk":0,"wdg":1,"hwm":[217,337,398,351]}
{"t_ms":1002,"sim":1,"vref_mv":5000,"vout_mv":5020,"duty_pm":209,"ccr1":355,"moe":1,"fault":0,"brk":0,"wdg":3,"hwm":[217,337,325,351]}
{"t_ms":1502,"sim":1,"vref_mv":5000,"vout_mv":5020,"duty_pm":209,"ccr1":355,"moe":1,"fault":0,"brk":0,"wdg":5,"hwm":[217,337,325,351]}
{"t_ms":2002,"sim":1,"vref_mv":5000,"vout_mv":4989,"duty_pm":209,"ccr1":355,"moe":1,"fault":0,"brk":0,"wdg":7,"hwm":[217,337,325,351]}
{"t_ms":2502,"sim":1,"vref_mv":5000,"vout_mv":5021,"duty_pm":209,"ccr1":355,"moe":1,"fault":0,"brk":0,"wdg":9,"hwm":[217,337,325,351]}
{"t_ms":3002,"sim":1,"vref_mv":5000,"vout_mv":5021,"duty_pm":209,"ccr1":355,"moe":1,"fault":0,"brk":0,"wdg":11,"hwm":[217,337,325,351]}
{"t_ms":3502,"sim":1,"vref_mv":5000,"vout_mv":5021,"duty_pm":209,"ccr1":355,"moe":1,"fault":0,"brk":0,"wdg":13,"hwm":[217,337,325,351]}
{"t_ms":4002,"sim":1,"vref_mv":5000,"vout_mv":4990,"duty_pm":209,"ccr1":355,"moe":1,"fault":0,"brk":0,"wdg":15,"hwm":[217,337,325,351]}
{"t_ms":4502,"sim":1,"vref_mv":5000,"vout_mv":5021,"duty_pm":209,"ccr1":355,"moe":1,"fault":0,"brk":0,"wdg":17,"hwm":[217,337,325,351]}
{"t_ms":5002,"sim":1,"vref_mv":5000,"vout_mv":5021,"duty_pm":209,"ccr1":355,"moe":1,"fault":0,"brk":0,"wdg":19,"hwm":[217,337,325,351]}
```

---

## 4. Final Board Restoration & Health Verification

After completing all 5 physical test suites, the clean starter template `23-code-library/templates/build/firmware.elf` (`170 MHz` Boost Mode + `PA5` LD2 blinky) was flashed and verified:

```text
ST-LINK SN  : 003700413335510133383531
ST-LINK FW  : V3J17M11
Board       : NUCLEO-G474RE
Voltage     : 3.28V
SWD freq    : 8000 KHz
Connect mode: Normal
Reset mode  : Software reset
Device ID   : 0x469
Revision ID : Rev X
Device name : STM32G47x/G48x/G414
NVM size    : 512 KBytes
```

---

## 5. Honest Verification Boundary (What Remains Pending External Hardware)

- **Physically Verified Onboard (`ONBOARD HARDWARE TESTED`):** All self-contained silicon peripherals and drivers exercised above (`RCC`/`PWR`/`FLASH` @ 160 MHz & 170 MHz, `GPIO` `PA5`/`PC13`, `SysTick`, `DWT->CYCCNT`, `TIM2`, `TIM3` TRGO, `TIM1`/`COMP1`/`DAC1` init, `EXTI13` SWIER, `USART2` TX/RX + `ring_buffer`, `DMA1` + `DMAMUX1`, `ADC1` internal `VREFINT`/`Temp`, `CORDIC`, `FMAC`, `CRC-32`, `FDCAN1` Internal Loopback, `MPU` + `.noinit` UsageFault persistence, `IWDG` + `DBGMCU`, `I2C1` absent-slave `NACKF`, and `FreeRTOS V11.1.0` kernel + all 3 capstones' onboard modes).
- **Deferred Pending User External Hardware / Bench Instruments (`EXTERNAL HARDWARE PENDING`):** Every experiment requiring external components, breadboard wires, oscilloscopes, logic analyzers, or microamp meters is documented with complete wiring, safety limits, and test procedures in [`HARDWARE_VALIDATION/PENDING_EXTERNAL_CONNECTIONS.md`](../HARDWARE_VALIDATION/PENDING_EXTERNAL_CONNECTIONS.md).
