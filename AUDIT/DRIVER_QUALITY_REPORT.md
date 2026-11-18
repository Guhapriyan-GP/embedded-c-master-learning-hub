# STM32G474RE Reusable Driver Library — Deep Quality & Static/Dynamic Audit Report

> **Date:** 2026-11-18  
> **Repository:** `embedded-c-master-learning-hub` (`<repo-root>/23-code-library/Drivers/`, base commit `543d21e`)  
> **Target MCU / Board:** `STM32G474RET6` (ARM Cortex-M4F, `170 MHz` / `160 MHz` Boost, `512 KB` Flash, `96 KB` SRAM1+2, `32 KB` CCM-SRAM) on ST **NUCLEO-G474RE** (`MB1367 Rev C`, STLINK-V3E `<redacted-stlink-sn>`)  
> **Cross-Compiler:** `arm-none-eabi-gcc 14.3.1 20250623 (GNU Tools for STM32 14.3.rel1.20251027-0700)`  
> **Host Compiler / Static Analyzer / Coverage:** `gcc 15.2.0` (`-fanalyzer`, `gcov -b`), **873 / 873 host assertions passing (`0` failed)**  
> **Ground-Truth Specifications:** `stm32g474xx.h`, `core_cm4.h`, ST RM0440, DS12288, ES0430, PM0214, UM2505, ARM AN321, NXP UM10204

---

## 1. Executive Summary & Audit Verdict

All **12 reusable bare-metal drivers** in `23-code-library/Drivers/` were audited across **12 production firmware engineering attributes** (Initialization, Dependencies, Configuration, Error codes, Timeout behavior, Error recovery, Blocking/non-blocking, ISR safety, Thread safety, Static stack & RAM footprint via `-fstack-usage`, Resource ownership, and Verification/Tests).

| Audit Metric | Verified Result |
|---|---|
| **Drivers Audited** | `12 / 12` (`clock_g4`, `gpio_g4`, `ring_buffer`, `fault_handler`, `exti_g4`, `tim_g4`, `usart_g4`, `spi_g4`, `i2c_g4`, `adc_g4`, `dma_g4`, `iwdg_g4`) |
| **Cross-Compile (`-Wall -Wextra -Werror -Wpedantic -Wshadow -Wconversion -Wdouble-promotion -Wundef`)** | **PASS (`0` warnings, `0` errors)** across Debug (`-Og -g3`) and Release (`-O2 -g`) |
| **Host Unit Tests (`make -C tests clean test`)** | **`873 / 873` assertions PASS (`0` failed)** across 5 suites (`+279` new full register-mock assertions in `tests/unit/test_peripherals_mock.c`) |
| **Static Analysis (`make -C tests analyze`, `gcc -fanalyzer`)** | **PASS (`0` warnings, `0` errors)** across all 5 host suites and 10 host-compiled drivers |
| **`gcov -b` Line Coverage (Core + Pure Helpers, `build/cov`)** | **`96.04%` of `278` lines** (`ring_buffer.c` `100.00%`, `gpio_g4.c` `100.00%`, 7/8 pure helpers `100.00%`, `tim_g4.c` `70.27%`) |
| **`gcov -b` Line Coverage (Full Register-Mock 8 Drivers, `build/cov_full`)** | **`90.07%` of `1,017` lines** (`exti_g4` `100.00%`, `dma_g4` `95.73%`, `spi_g4` `94.05%`, `iwdg_g4` `93.88%`, `usart_g4` `91.41%`, `i2c_g4` `90.10%`, `tim_g4` `87.04%`, `adc_g4` `66.67%`) |
| **`ring_buffer.c` ARM Cortex-M4 `libatomic` Dependency** | **ZERO (`0` undefined symbols in `arm-none-eabi-nm -u`)** — native inline Thumb-2 `ldr`/`str` + `dmb ish` (`f3bf 8f5b`) |
| **Max Single-Function Static Stack (`-O2 -fstack-usage`)** | **`56 B`** (`dma_config_p2m_circular`, `iwdg_calc_prescaler_reload`, `iwdg_init_ms`) — **zero dynamic allocation (`malloc`)** in all 12 drivers |
| **Target Execution Verified Onboard (Zero External Wiring)** | **`10 / 12` full onboard target execution** (`clock_g4`, `gpio_g4`, `ring_buffer`, `fault_handler`, `exti_g4` [SWIER1], `tim_g4`, `usart_g4`, `adc_g4`, `dma_g4`, `iwdg_g4`); **`1 / 12` partial onboard** (`i2c_g4` absent-slave `NACKF`); **`1 / 12` external hardware pending** (`spi_g4`) |

---

## 2. Empirical ARM Cortex-M4 Assembly Proof: `ring_buffer.c` Zero-`libatomic` Verification

A common embedded C11 concern is whether `<stdatomic.h>` (`_Atomic uint32_t`) on ARM Cortex-M4 emits out-of-line helper calls to `libatomic` (`__atomic_load_4`, `__atomic_store_4`), which would fail to link under bare-metal `--specs=nano.specs --specs=nosys.specs` without `-latomic`.

### 2.1 Compilation & Undefined Symbol Check (`arm-none-eabi-nm -u`)

```text
$ arm-none-eabi-gcc -std=c11 -mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 \
    -mfloat-abi=hard -DSTM32G474xx -O2 -Wall -Wextra -Werror -Wpedantic \
    -Wshadow -Wconversion -Wdouble-promotion -Wundef -fstack-usage \
    -I23-code-library/Drivers -c 23-code-library/Drivers/ring_buffer.c -o ring_buffer.o

$ arm-none-eabi-nm -u ring_buffer.o
(empty output — 0 undefined symbols, 0 __atomic_* / libatomic references)
```

### 2.2 Verbatim Thumb-2 Disassembly (`arm-none-eabi-objdump -d ring_buffer.o`)

Excerpts of `rb_push` and `rb_pop` confirm native single-instruction 32-bit loads/stores paired with Inner Shareable Data Memory Barrier **`dmb ish` (`f3bf 8f5b`)**:

```asm
000000bc <rb_push>:
  bc: b1d0        cbz   r0, f4 <rb_push+0x38>
  be: 6803        ldr   r3, [r0, #0]
  c0: b1c3        cbz   r3, f4 <rb_push+0x38>
  c2: b410        push  {r4}
  c4: 68c2        ldr   r2, [r0, #12]        @ atomic_load_explicit(&rb->head, relaxed)
  c6: 6903        ldr   r3, [r0, #16]        @ atomic_load_explicit(&rb->tail, acquire)
  c8: f3bf 8f5b   dmb   ish                  @ acquire barrier
  cc: 6844        ldr   r4, [r0, #4]
  ce: 1ad3        subs  r3, r2, r3
  d0: 42a3        cmp   r3, r4
  d2: d303        bcc.n dc <rb_push+0x20>
  d4: 2000        movs  r0, #0
  d6: f85d 4b04   ldr.w r4, [sp], #4
  da: 4770        bx    lr
  dc: 6883        ldr   r3, [r0, #8]
  de: 6804        ldr   r4, [r0, #0]
  e0: 4013        ands  r3, r2
  e2: 3201        adds  r2, #1
  e4: 54e1        strb  r1, [r4, r3]         @ write payload byte into storage[head & mask]
  e6: f3bf 8f5b   dmb   ish                  @ release barrier (payload happens-before head++)
  ea: 60c2        str   r2, [r0, #12]        @ atomic_store_explicit(&rb->head, head+1, release)
  ec: f85d 4b04   ldr.w r4, [sp], #4
  f0: 2001        movs  r0, #1
  f2: 4770        bx    lr

000000f8 <rb_pop>:
  f8: b1b0        cbz   r0, 128 <rb_pop+0x30>
  fa: 6803        ldr   r3, [r0, #0]
  fc: b1a3        cbz   r3, 128 <rb_pop+0x30>
  fe: b199        cbz   r1, 128 <rb_pop+0x30>
 100: 6903        ldr   r3, [r0, #16]        @ atomic_load_explicit(&rb->tail, relaxed)
 102: 68c2        ldr   r2, [r0, #12]        @ atomic_load_explicit(&rb->head, acquire)
 104: f3bf 8f5b   dmb   ish                  @ acquire barrier
 108: 429a        cmp   r2, r3
 10a: d00d        beq.n 128 <rb_pop+0x30>
 10c: b410        push  {r4}
 10e: 6882        ldr   r2, [r0, #8]
 110: 6804        ldr   r4, [r0, #0]
 112: 401a        ands  r2, r3
 114: 3301        adds  r3, #1
 116: 5ca2        ldrb  r2, [r4, r2]         @ read payload byte from storage[tail & mask]
 118: 700a        strb  r2, [r1, #0]
 11a: f3bf 8f5b   dmb   ish                  @ release barrier (read completes before tail++)
 11e: 6103        str   r3, [r0, #16]        @ atomic_store_explicit(&rb->tail, tail+1, release)
 120: f85d 4b04   ldr.w r4, [sp], #4
 124: 2001        movs  r0, #1
 126: 4770        bx    lr
```

**Engineering Conclusion:** On ARMv7E-M (`-mcpu=cortex-m4`), `ring_buffer.c` compiles to **100% lock-free, wait-free, zero-`libatomic`, zero-branch-loop Thumb-2 instructions** using `4 B` static stack per `rb_push`/`rb_pop`.

---

## 3. Complete 12-Driver × 12-Attribute Deep Quality Matrix

All static stack figures below are verbatim outputs of `arm-none-eabi-gcc -O2 -fstack-usage` (`*.su`) across all 12 drivers.

### 3.1 `clock_g4.c` / `clock_g4.h`

| Attribute | Audited Assessment |
|---|---|
| **1. Initialization** | `Clock_Init_170MHz()`, `Clock_Init_160MHz()`, `Clock_Init_170MHz_HSE24()`, `Clock_DWT_Init()`. Executes full RM0440 §6.1.5 Boost sequence: `PWREN` -> `VOS=01b` -> wait `VOSF=0` -> `R1MODE=0` -> `4 WS` + ART caches -> safe fallback to `HSI16` if PLL active -> PLL lock -> two-step `HPRE=/2` -> `>= 1 us` delay -> `HPRE=/1` -> `SystemCoreClockUpdate()`. |
| **2. Dependencies** | `stm32g474xx.h`, CMSIS `SystemCoreClock` / `SystemCoreClockUpdate()`, `CoreDebug->DEMCR`, `DWT->CYCCNT`. Zero external driver dependencies. |
| **3. Configuration** | Fixed verified profiles (`170 MHz HSI16`, `160 MHz HSI16`, `170 MHz HSE24`); `PLLQ=/8` (`40.0`/`42.5 MHz`), `PLLP=/6` (`53.33`/`56.67 MHz <= 60 MHz` ADC limit). |
| **4. Error Codes** | `bool` (`true` on lock/switch, `false` on any bounded timeout). No granular error enum distinguishing `VOSF` vs `HSERDY` vs `PLLRDY` failure. |
| **5. Timeout Behavior** | Bounded `CLOCK_READY_TIMEOUT = 1,000,000` loop iterations on all 6 hardware readiness polls (`VOSF`, `FLASH_ACR`, `HSIRDY`, `HSERDY`, `PLLRDY`, `SWS`). `Clock_DelayUs()` auto-initializes `DWT` if uninitialized and chunks delays in `<= 1,000,000 us` slices to avoid 32-bit wrap. |
| **6. Error Recovery** | On `false` return, leaves MCU on whichever safe clock was active prior to failure (does not hard-lock); caller must inspect return value and decide fallback. |
| **7. Blocking / Non-Blocking** | **Blocking** (boot-time configuration & busy-wait `Clock_DelayUs()`/`Clock_DelayMs()`). `Clock_GetSysClkHz()`, `Clock_GetProfile()`, `Clock_GetCycles()` are non-blocking O(1). |
| **8. ISR Safety** | `Clock_GetSysClkHz()`, `Clock_GetProfile()`, `Clock_GetCycles()` are ISR-safe. `Clock_Init_*()` and `Clock_Delay*()` must NOT be called from ISRs. |
| **9. Thread Safety** | `Clock_Init_*()` is boot-time / single-threaded only (modifies global `RCC`, `PWR`, `FLASH`, `SystemCoreClock`, and `s_active_profile`). `Clock_DelayUs()` is reentrant once `DWT` is enabled. |
| **10. Memory Use (`.su` @ `-O2`)** | Static RAM: `4 B` (`s_active_profile`). Zero heap. Max stack: **`16 B`** (`Clock_DelayMs`), `12 B` (`Clock_DelayUs`), `8 B` (`Clock_Init_*`, `clock_configure_pll_boost`), `0 B` (`Clock_Get*`, `Clock_DWT_Init`). |
| **11. Resource Ownership** | Exclusively owns `RCC->CR/PLLCFGR/CFGR`, `PWR->CR1/CR5`, `FLASH->ACR`, `CoreDebug->DEMCR.TRCENA`, `DWT->CTRL/CYCCNT`. |
| **12. Tests** | Cross-compile `PASS` (`0` warnings); Onboard Target Execution `PASS` (`160 MHz` `1,599,997` cyc/10 ms, `170 MHz` `1,699,989` cyc/10 ms); Host mock: `N/A` (inline `nop` + `SystemCoreClockUpdate`); `Clock_Init_170MHz_HSE24()` + MCO scope: `PENDING — USER CONNECTION REQUIRED`. |

### 3.2 `gpio_g4.c` / `gpio_g4.h`

| Attribute | Audited Assessment |
|---|---|
| **1. Initialization** | Caller invokes `GPIO_EnableClock(GPIOx)` then `GPIO_ConfigPin()` / `GPIO_ConfigInput()` / `GPIO_ConfigOutput()` / `GPIO_ConfigAF()` / `GPIO_ConfigAnalog()`. Writes `MODER` last to avoid transient output glitches when leaving STM32G4 `11b` Analog reset state. |
| **2. Dependencies** | `stm32g474xx.h` (`GPIO_TypeDef`, `RCC->AHB2ENR`). |
| **3. Configuration** | Per-pin `gpio_pin_config_t` (`pin 0..15`, `mode`, `otype`, `speed`, `pull`, `af 0..15`), plus RM0440 §9.4.8 4-step `GPIO_LockPin()`. |
| **4. Error Codes** | `bool` (`false` on `NULL`, `pin > 15`, `af > 15`, or attempting to lock a 2nd pin on an already-locked port). `GPIO_EnableClock()` silently ignores `NULL`/unknown ports. |
| **5. Timeout Behavior** | Zero polling loops — deterministic O(1) register writes with single `RCC->AHB2ENR` readback sync. |
| **6. Error Recovery** | Invalid arguments return `false` without mutating any GPIO register. `GPIO_LockPin()` is hardware-latched until MCU reset per RM0440 §9.4.8. |
| **7. Blocking / Non-Blocking** | **Non-blocking** (O(1) register access). |
| **8. ISR Safety** | `GPIO_SetPin()`, `GPIO_ResetPin()`, `GPIO_WritePin()`, `GPIO_ReadPin()` are fully ISR-safe (single-write `BSRR` / read `IDR`). `GPIO_TogglePin()` reads `ODR` then writes `BSRR` — **cross-pin atomic** on the port, **NOT same-pin atomic** under concurrent writers (caller must enforce single-writer per pin or critical section). `GPIO_Config*()` performs RMW on `MODER/OTYPER/OSPEEDR/PUPDR/AFR` — **not ISR-safe** if shared with tasks on the same port. |
| **9. Thread Safety** | `Set/Reset/Write/Read` are thread-safe across distinct pins; `Config*()` / `EnableClock()` require serialization if multiple tasks configure pins on the same port simultaneously. |
| **10. Memory Use (`.su` @ `-O2`)** | Static RAM: `0 B`. Zero heap. Max stack: **`16 B`** (`GPIO_ConfigPin`, `GPIO_ConfigAF`), `4 B` (`GPIO_ConfigInput/Output/Analog`), `0 B` (`GPIO_EnableClock`, `GPIO_LockPin`). Inline `Set/Reset/Write/Toggle/Read`: `0 B`. |
| **11. Resource Ownership** | Caller owns port/pin allocation; driver does not reserve pins globally (caller must not reconfigure SWD `PA13`/`PA14` or `PB8-BOOT0` carelessly). |
| **12. Tests** | Cross-compile `PASS`; Host `test_gpio_mock.c` (`82/82` assertions, **`100.00%` of 63 lines**, `96.88%` branches taken) + `test_peripherals_mock.c`; `gcc -fanalyzer` `0` warnings; Onboard Target Execution `PASS` (`PA5`, `PC13`, `PA2/PA3`, `PA15/PB7`). |

### 3.3 `ring_buffer.c` / `ring_buffer.h`

| Attribute | Audited Assessment |
|---|---|
| **1. Initialization** | `rb_init(rb, storage, capacity)` validates non-NULL pointers and strict power-of-2 `capacity >= 2` (`(n & (n-1)) == 0`). Zero heap allocation. |
| **2. Dependencies** | ISO C11 `<stdatomic.h>`, `<stdint.h>`, `<stdbool.h>`, `<stddef.h>` (with `__STDC_NO_ATOMICS__` compiler barrier fallback). Zero CMSIS/MCU dependency. |
| **3. Configuration** | Caller supplies backing `uint8_t[]` array and power-of-2 capacity (`2..2^31`). |
| **4. Error Codes** | `bool` (`false` on NULL/invalid capacity/full/empty) and `uint32_t` transferred byte count for `rb_write()`/`rb_read()`. |
| **5. Timeout Behavior** | Zero loops/waits — wait-free bounded O(1) for `push`/`pop`/`peek` and O(N) for `write`/`read`. |
| **6. Error Recovery** | Overflow on `rb_push()` drops new byte cleanly without corrupting existing queued bytes; `rb_count()` clamps `(head - tail)` to `capacity` to prevent unsigned underflow in `rb_free()`. |
| **7. Blocking / Non-Blocking** | **Strictly Non-Blocking** (lock-free, wait-free SPSC). |
| **8. ISR Safety** | **ISR-Safe for SPSC** (e.g., 1 producer ISR calling `rb_push`/`rb_write`, 1 consumer main/task calling `rb_pop`/`rb_read`). `rb_reset()` is NOT safe concurrently with active producer/consumer. |
| **9. Thread Safety** | **Strictly Single-Producer Single-Consumer (SPSC)** using C11 `memory_order_acquire`/`memory_order_release` (empirical `dmb ish` proof in Section 2). Multi-producer or multi-consumer requires external mutex or critical section. |
| **10. Memory Use (`.su` @ `-O2`)** | Control struct `ring_buffer_t`: `20 B` (`storage*`, `capacity`, `mask`, `_Atomic head`, `_Atomic tail`). Zero heap. Max stack: **`20 B`** (`rb_write`), `16 B` (`rb_read`), `4 B` (`rb_push`, `rb_pop`, `rb_peek`), `0 B` (`rb_init`, `rb_reset`, `rb_count`, `rb_free`, `rb_is_empty`, `rb_is_full`). |
| **11. Resource Ownership** | Caller owns both `ring_buffer_t` and backing `storage[]` lifetime. |
| **12. Tests** | Cross-compile `PASS` (`0` undefined `libatomic` symbols); Host `test_ring_buffer.c` (`104/104` assertions, **`100.00%` of 93 lines**, `98.53%` branches taken, including `UINT32_MAX` index wraparound); `gcc -fanalyzer` `0` warnings; Onboard Target Execution `PASS` (Project 01 live `USART2` RX `'s'` command). |

### 3.4 `fault_handler.c` / `fault_handler.h`

| Attribute | Audited Assessment |
|---|---|
| **1. Initialization** | `FaultHandler_EnableDetailedFaults(trap_div_by_zero)` enables `SHCSR` (`MEMFAULTENA | BUSFAULTENA | USGFAULTENA`) and optional `CCR.DIV_0_TRP` + `__DSB(); __ISB();`. |
| **2. Dependencies** | `stm32g474xx.h`, `core_cm4.h` (`SCB`, `CoreDebug`), linker `.noinit` section in `stm32g474re.ld`, assembly/naked `HardFault_Handler` trampoline in `startup_stm32g474re.c`. |
| **3. Configuration** | `bool trap_div_by_zero`; persistent `g_crash_log` in `.noinit` (`CRASH_LOG_MAGIC = 0xFA017C0D`). |
| **4. Error Codes** | `bool FaultHandler_HasValidCrashLog()`; decoded bitmask `fault_flags_t` (`MEMMANAGE`, `BUSFAULT`, `USAGEFAULT`, `DIVBYZERO`, `UNALIGNED`, `UNDEFINSTR`, `INVSTATE`, `FORCED_HF`, `VECTTBL_HF`, `MMFAR_VALID`, `BFAR_VALID`). Invalid stacked SP writes sentinel `0xDEAD57AC`. |
| **5. Timeout Behavior** | `HardFault_Handler_C()` intentionally halts debugger (`bkpt #0` if `C_DEBUGEN=1`) then enters infinite spin loop for post-mortem inspection (or watchdog reset if IWDG active). |
| **6. Error Recovery** | Captures `SCB` registers BEFORE touching `stacked_frame`; validates `stacked_frame` lies inside 4-byte-aligned `SRAM1+SRAM2` (`0x20000000..0x20017FE0`) or `CCM-SRAM` (`0x10000000..0x10007FE0`) to prevent secondary BusFault -> Cortex-M4 Lockup! Next boot inspects `FaultHandler_HasValidCrashLog()` and calls `FaultHandler_ClearCrashLog()`. |
| **7. Blocking / Non-Blocking** | `Enable`/`HasValid`/`Clear` are non-blocking O(1); `HardFault_Handler_C()` is terminal (never returns). |
| **8. ISR Safety** | `HardFault_Handler_C()` executes in HardFault exception context (priority `-1`). |
| **9. Thread Safety** | `FaultHandler_EnableDetailedFaults()` is boot-time single-threaded; `g_crash_log` is `volatile` `.noinit`. |
| **10. Memory Use (`.su` @ `-O2`)** | Static `.noinit` RAM: `68 B` (`crash_log_t`). Zero heap. Max stack: **`8 B`** (`HardFault_Handler_C`), `0 B` (`Enable`/`HasValid`/`Clear`). |
| **11. Resource Ownership** | Owns `g_crash_log` (`.noinit`), `SCB->SHCSR` fault enables, `SCB->CCR.DIV_0_TRP`, and `HardFault_Handler_C` symbol. |
| **12. Tests** | Cross-compile `PASS`; Host `test_gpio_mock.c` verifies exact SRAM1/SRAM2/CCM bounds validator & SCB flag decoder; Onboard Target Execution `PASS` (live `DIV0` SWD `.noinit` capture `magic=0xFA017C0D, CFSR=0x02000000, SP=0x20017F48` + automated reboot persistence in `onboard_selftest` Test 8A/8B). |

### 3.5 `exti_g4.c` / `exti_g4.h`

| Attribute | Audited Assessment |
|---|---|
| **1. Initialization** | `EXTI_ConfigLine(port, pin, edge, nvic_priority)` enables `RCC->APB2ENR.SYSCFGEN` + readback, masks `IMR1`, routes `SYSCFG->EXTICR[0..3]` nibble, configures `RTSR1`/`FTSR1`, clears stale `PR1` (`rc_w1`), unmasks `IMR1`, and configures NVIC. |
| **2. Dependencies** | `stm32g474xx.h` (`RCC`, `SYSCFG`, `EXTI`, `GPIOA..GPIOG`, `NVIC`). Caller must configure GPIO pin to Digital Input (`MODER=00b`) separately. |
| **3. Configuration** | 16 lines (`0..15`), `EXTI_EDGE_NONE/RISING/FALLING/BOTH`, NVIC priority, per-line callback `(line, ctx)` via `EXTI_RegisterCallback()`. |
| **4. Error Codes** | `exti_status_t`: `EXTI_OK (0)`, `EXTI_ERR_INVALID_ARG (-1)`. |
| **5. Timeout Behavior** | Zero polling loops — deterministic O(1) setup and O(lines in range) dispatch. |
| **6. Error Recovery** | Invalid port/pin/range rejected safely without register corruption; stale pending bits cleared before unmasking. |
| **7. Blocking / Non-Blocking** | **Non-blocking**. |
| **8. ISR Safety** | `EXTI_ClearPending()` (`EXTI->PR1 = mask; (void)EXTI->PR1; __DSB();`), `EXTI_TriggerSoftware()`, and `EXTI_DispatchRange()` are **ISR-safe** (satisfies ARM AN321 §4.9 APB2 readback + ARM Erratum 838869 / ST ES0430 §2.1.3 tail `__DSB()`). `EXTI_ConfigLine()` / `EXTI_RegisterCallback()` should be called with the target line masked or before enabling IRQ. |
| **9. Thread Safety** | `s_exti_callbacks[16]` writes are non-atomic two-word struct writes (`cb` + `ctx`) — register callbacks before enabling the line or inside a critical section. |
| **10. Memory Use (`.su` @ `-O2`)** | Static RAM: `128 B` (`s_exti_callbacks[16]`). Zero heap. Max stack: **`32 B`** (`EXTI_DispatchRange`), `20 B` (`EXTI_ConfigLine`), `4 B` (`EXTI_RegisterCallback`), `0 B` (`exti_calc_exticr_index_shift`, `EXTI_DisableLine`, `EXTI_TriggerSoftware`). |
| **11. Resource Ownership** | Owns `SYSCFG->EXTICR[0..3]`, `EXTI->IMR1/RTSR1/FTSR1/SWIER1/PR1`, and callback table `s_exti_callbacks[16]`. |
| **12. Tests** | Cross-compile `PASS`; Host `test_drivers_math` (`100.00%` of 6 pure lines) + `test_peripherals_mock` (**`100.00%` of `88` full lines**, `100.00%` branches executed); `gcc -fanalyzer` `0` warnings; Onboard Target Execution `PASS` for `SWIER1` (`swier_cb=3/3`, `EXTICR4=0x20`); physical `PC13`/`B1` actuation: `PENDING — USER INTERACTION REQUIRED`. |

### 3.6 `tim_g4.c` / `tim_g4.h`

| Attribute | Audited Assessment |
|---|---|
| **1. Initialization** | `TIM_InitPeriodic(TIM2/3/6/7, freq_hz, enable_irq, prio)` and `TIM_InitPwm(TIM2/3, ch 1..4, freq_hz, duty_permille)`. Dynamically derives kernel clock from `SystemCoreClock` + `RCC->CFGR.PPRE1` (including STM32 `x2` timer clock rule when `PPRE1 > /1`), sets `ARPE` (+ `OCxPE` for PWM Mode 1), latches shadows via `EGR = UG`, clears `SR` (`rc_w0`), then sets `CEN`. |
| **2. Dependencies** | `stm32g474xx.h`, CMSIS `SystemCoreClock`, `APBPrescTable`. Caller configures PWM GPIO AF pins separately. |
| **3. Configuration** | Supports `TIM2` (32-bit, constrained to 16-bit ARR in `tim_calc_psc_arr` for portability), `TIM3` (16-bit), `TIM6`/`TIM7` (basic timers, periodic only); PWM channels `1..4`, duty `0..1000` permille. |
| **4. Error Codes** | `tim_status_t`: `TIM_OK (0)`, `TIM_ERR_INVALID_ARG (-1)`, `TIM_ERR_UNSUPPORTED_TIM (-2)`, `TIM_ERR_FREQ_OUT_OF_RANGE (-3)`. |
| **5. Timeout Behavior** | Zero polling loops — deterministic bounded execution. |
| **6. Error Recovery** | Stops counter (`CEN=0`) before reconfiguring; rejects unsupported timers (e.g., `TIM6` PWM) and unreachable frequencies cleanly. |
| **7. Blocking / Non-Blocking** | **Non-blocking**. |
| **8. ISR Safety** | `TIM_ClearUpdateFlag()` (`tim->SR = ~TIM_SR_UIF; (void)tim->SR; __DSB();`) and `TIM_SetPwmCompare()` / `TIM_SetPwmDutyPermille()` (single 32-bit `CCRx` store with `OCxPE` preload) are **ISR-safe**. `TIM_Init*()` is init-time only. |
| **9. Thread Safety** | `TIM_SetPwmCompare()` is atomic per channel; `TIM_Init*()` must be serialized per timer instance. |
| **10. Memory Use (`.su` @ `-O2`)** | Static RAM: `0 B`. Zero heap. Max stack: **`40 B`** (`tim_calc_psc_arr` due to 64-bit arithmetic), `32 B` (`TIM_InitPwm`), `24 B` (`TIM_InitPeriodic`, `TIM_SetPwmDutyPermille`), `16 B` (`tim_calc_pwm_ccr`), `0 B` (`TIM_GetKernelClockHz`, `TIM_SetPwmCompare`). |
| **11. Resource Ownership** | Caller owns timer instance (`TIM2`/`3`/`6`/`7`) and associated NVIC IRQ handler. Note: `TIM_InitPwm()` writes `ARR`/`PSC` for the whole timer, so multi-channel PWM on the same timer shares one carrier frequency. |
| **12. Tests** | Cross-compile `PASS`; Host `test_drivers_math` + `test_peripherals_mock` (**`87.04%` of `162` full lines**, `94.83%` branches executed); `gcc -fanalyzer` `0` warnings; Onboard Target Execution `PASS` (`TIM2` 100 Hz & 10 Hz IRQ, `TIM2_CH2` 20 kHz PWM init, `TIM3` TRGO); oscilloscope PWM capture: `PENDING — USER CONNECTION REQUIRED`. |

### 3.7 `usart_g4.c` / `usart_g4.h`

| Attribute | Audited Assessment |
|---|---|
| **1. Initialization** | `usart_init(USART1/USART2/LPUART1, baud, enable_rx_irq, prio)` enables RCC clock + readback, derives `PCLK1`/`PCLK2` from `SystemCoreClock` + `PPRE1`/`PPRE2`, computes standard USART (`OVER8=0`) or 20-bit LPUART1 `BRR`, clears `ICR`, configures 8N1 `TE|RE|UE` (+ optional `RXNEIE` + NVIC). |
| **2. Dependencies** | `stm32g474xx.h`, `ring_buffer.h`, CMSIS `SystemCoreClock`, `APBPrescTable`. Caller configures TX/RX GPIO AF pins. |
| **3. Configuration** | Fixed `8N1`, `OVER8=0`; dynamic baud rate (`[0x10..0xFFFF]` for USART, `[0x300..0xFFFFF]` for LPUART1); optional RX interrupt. |
| **4. Error Codes** | `usart_status_t`: `USART_OK (0)`, `USART_ERR_INVALID_ARG (-1)`, `USART_ERR_BAUD_RANGE (-2)`, `USART_ERR_TIMEOUT (-3)`; `usart_errflags_t` bitmask (`PE`, `FE`, `NE`, `ORE`). |
| **5. Timeout Behavior** | Bounded `timeout_iters` (>0 validated) on `TXE` (`usart_write_byte/buf/str`) and `TC` (`usart_flush_tx`). **Audit Note:** `usart_rx_irq_handler()` drains `while ((usart->ISR & RXNE) != 0U)` relying on STM32G4 hardware clearing `RXNE` upon `RDR` read (fine on silicon with 8-byte FIFO; adding a bounded drain cap such as `<= 16` bytes is recommended hardening against stuck peripheral clock faults). |
| **6. Error Recovery** | `usart_clear_errors()` clears `ORE`/`FE`/`NE`/`PE` via `ICR` BEFORE draining `RDR` in `usart_rx_irq_handler()` so overrun never deadlocks RX interrupts. |
| **7. Blocking / Non-Blocking** | TX APIs (`write_byte/buf/str`, `flush_tx`) are **bounded blocking**; `usart_clear_errors()` and `usart_rx_irq_handler()` are **non-blocking**. |
| **8. ISR Safety** | `usart_rx_irq_handler()` and `usart_clear_errors()` are **ISR-safe** (pushing into SPSC `ring_buffer_t` + `__DSB()`). Blocking `usart_write_*()` must NOT be called from normal ISRs (only terminal fault handlers). |
| **9. Thread Safety** | Not internally locked — multi-task TX sharing a single `USARTx` requires an application-level FreeRTOS Mutex (as implemented in `23-code-library/freertos/` and Project 03). RX ISR -> single consumer task via `ring_buffer_t` is lock-free SPSC thread-safe. |
| **10. Memory Use (`.su` @ `-O2`)** | Static RAM: `0 B`. Zero heap. Max stack: **`24 B`** (`usart_init`), `16 B` (`usart_rx_irq_handler`), `8 B` (`usart_calc_brr`, `lpuart_calc_brr`, `usart_write_buf`, `usart_write_str`), `4 B` (`usart_clear_errors`), `0 B` (`usart_write_byte`, `usart_flush_tx`). |
| **11. Resource Ownership** | Caller owns `USARTx` instance, GPIO pins, and external `ring_buffer_t`. |
| **12. Tests** | Cross-compile `PASS`; Host `test_drivers_math` (`100.00%` of 15 pure lines) + `test_peripherals_mock` (**`91.41%` of `128` full lines**, `97.92%` branches executed); `gcc -fanalyzer` `0` warnings; Onboard Target Execution `PASS` (`160` & `170 MHz` `115200 8N1` TX + live RX `'s'` ISR). |

### 3.8 `spi_g4.c` / `spi_g4.h`

| Attribute | Audited Assessment |
|---|---|
| **1. Initialization** | `spi_init_master(SPI1/SPI2/SPI3, mode 0..3, max_sck_hz)` enables RCC clock + readback, disables `SPE`, computes exact non-overshooting `BR[2:0]`, sets `MSTR | SSM | SSI | CPOL | CPHA`, configures **`CR2 = (7 << DS_Pos) | SPI_CR2_FRXTH`** (8-bit frame + 8-bit RXNE threshold), and sets `SPE`. |
| **2. Dependencies** | `stm32g474xx.h`, CMSIS `SystemCoreClock`, `APBPrescTable`. Caller configures SCK/MISO/MOSI AF pins and software CS GPIO. |
| **3. Configuration** | 8-bit MSB-first full-duplex Master; Modes `0..3`; dynamic prescaler `/2../256`. |
| **4. Error Codes** | `spi_status_t`: `SPI_OK (0)`, `SPI_ERR_INVALID_ARG (-1)`, `SPI_ERR_TIMEOUT (-2)`. |
| **5. Timeout Behavior** | Bounded `timeout_iters` (>0 validated) on `TXE`, `RXNE`, and `spi_wait_idle()` (`FTLVL == 0 && BSY == 0`). |
| **6. Error Recovery** | Returns `SPI_ERR_TIMEOUT` cleanly without infinite hang; caller must deassert CS and re-init SPI on timeout. |
| **7. Blocking / Non-Blocking** | **Bounded blocking** polling transfers (`spi_transfer_byte`, `spi_transfer_buf`, `spi_wait_idle`). |
| **8. ISR Safety** | **Not ISR-safe** (blocking polling loops). |
| **9. Thread Safety** | Not internally locked — multi-task access to a shared SPI bus requires a FreeRTOS Mutex around the entire CS-LOW -> transfer -> `spi_wait_idle()` -> CS-HIGH transaction. |
| **10. Memory Use (`.su` @ `-O2`)** | Static RAM: `0 B`. Zero heap. Max stack: **`20 B`** (`spi_transfer_buf`), `12 B` (`spi_init_master`), `4 B` (`spi_calc_br_div`, `spi_transfer_byte`), `0 B` (`spi_wait_idle`). |
| **11. Resource Ownership** | Caller owns `SPIx` instance, GPIO pins (warning: `PA5` `SPI1_SCK` conflicts with NUCLEO `LD2`), and CS pin. |
| **12. Tests** | Cross-compile `PASS`; Host `test_drivers_math` (`100.00%` of 8 pure lines) + `test_peripherals_mock` (**`94.05%` of `84` full lines**, `100.00%` branches executed, `83.33%` taken); `gcc -fanalyzer` `0` warnings; Physical Hardware: **`PENDING — USER CONNECTION REQUIRED`** (`PB4 <-> PB5` loopback or 3.3 V SPI slave). |

### 3.9 `i2c_g4.c` / `i2c_g4.h`

| Attribute | Audited Assessment |
|---|---|
| **1. Initialization** | `i2c_init_master(I2C1/I2C2/I2C3, speed 100k/400k)` enables `APB1ENR1`, queries `PCLK1`, selects NXP UM10204-verified `TIMINGR` for `170`/`160`/`16 MHz`, disables `PE`, programs `TIMINGR`, clears `ICR`, enables `PE` (Analog Filter ON, `DNF=0`). |
| **2. Dependencies** | `stm32g474xx.h`, `clock_g4.h` (`Clock_DelayUs` for 9-clock recovery), `gpio_g4.h` (for `i2c_recover_bus_9clks`), CMSIS `SystemCoreClock`, `APBPrescTable`. |
| **3. Configuration** | 7-bit Master; `100 kHz` / `400 kHz`; transparent `RELOAD` + `TCR` chunking for `> 255 B` writes and reads; hardware Repeated START in `i2c_read_reg()`. |
| **4. Error Codes** | `i2c_status_t`: `I2C_OK (0)`, `I2C_ERR_INVALID_ARG (-1)`, `I2C_ERR_NACK (-2)`, `I2C_ERR_BUS (-3)`, `I2C_ERR_ARLO (-4)`, `I2C_ERR_BUSY (-5)`, `I2C_ERR_TIMEOUT (-6)`, `I2C_ERR_UNSUP_CLK (-7)`. |
| **5. Timeout Behavior** | Bounded `timeout_iters` (>0 validated) on `BUSY`, `TXIS`, `RXNE`, `TC`, `TCR`, `STOPF`, and post-`NACKF` `STOPF` wait. |
| **6. Error Recovery** | On `NACKF`: clears `NACKCF`, waits bounded for hardware `STOPF`, clears `STOPCF`, and **flushes stale `TXDR` via `i2c->ISR = I2C_ISR_TXE`** (RM0440 §39.7.7). On `BERR`/`ARLO`: clears respective `ICR` flag. Provides `i2c_recover_bus_9clks()` (NXP UM10204 §3.1.16) to unstick a slave holding SDA LOW. |
| **7. Blocking / Non-Blocking** | **Bounded blocking** polling transactions. |
| **8. ISR Safety** | **Not ISR-safe** (blocking polling + `Clock_DelayUs()` in recovery). |
| **9. Thread Safety** | Not internally locked — multi-task access to a shared I2C bus requires a FreeRTOS Mutex. |
| **10. Memory Use (`.su` @ `-O2`)** | Static RAM: `0 B`. Zero heap. Max stack: **`48 B`** (`i2c_write_stream`), `40 B` (`i2c_read_reg`, `i2c_recover_bus_9clks`), `24 B` (`i2c_write`, `i2c_write_reg`), `4 B` (`i2c_init_master`, `i2c_wait_flag`), `0 B` (`i2c_select_timingr`). |
| **11. Resource Ownership** | Caller owns `I2Cx` instance and SCL/SDA pins (default NUCLEO routing `PA15`/`PB7` avoids `PB8-BOOT0`). After `i2c_recover_bus_9clks()`, caller must restore AF4 pin config and call `i2c_init_master()`. |
| **12. Tests** | Cross-compile `PASS`; Host `test_drivers_math` (`100.00%` of 16 pure lines + UM10204 bounds) + `test_peripherals_mock` (**`90.10%` of `192` full lines**, `93.85%` branches executed, incl. `RELOAD`, `NACKF`+`TXE` flush, `BERR`, `ARLO`, `9clks`); `gcc -fanalyzer` `0` warnings; Onboard Target Execution `PARTIAL` (absent-slave `NACKF` in Projects 02/03); external slave ACK/Read/Recovery: **`PENDING — USER CONNECTION REQUIRED`**. |

### 3.10 `adc_g4.c` / `adc_g4.h`

| Attribute | Audited Assessment |
|---|---|
| **1. Initialization** | `adc_init_single_ended(ADC1/ADC2, timeout_iters)`: `RCC->AHB2ENR.ADC12EN` -> `ADDIS` (if active) -> shared `ADC12_COMMON->CCR.CKMODE = 11b` (`HCLK/4 <= 60 MHz`, guarded against peer ADC active with wrong `CKMODE`) -> `DEEPPWD=0` -> `ADVREGEN=1` -> `25 us` delay -> `ADCALDIF=0` + `ADCAL` -> `2 us` delay -> direct `rc_w1` `ISR = ADRDY` -> `ADEN=1` -> wait `ADRDY`. |
| **2. Dependencies** | `stm32g474xx.h`, `clock_g4.h` (`Clock_DelayUs` for `25 us` regulator, `2 us` post-cal, `20 us` VREFINT, `125 us` temp sensor), STM32G474RE factory ROM calibration words (`0x1FFF75A8`, `0x1FFF75AA`, `0x1FFF75CA`). |
| **3. Configuration** | 12-bit right-aligned single-ended polling; selectable sampling time `2.5..640.5` cycles; built-in `VREFINT` (`ADC1_IN18`) & die temp (`ADC1_IN16`) at `640.5` cycles (`15.1 us @ 42.5 MHz`). |
| **4. Error Codes** | `adc_status_t`: `ADC_OK (0)`, `ADC_ERR_INVALID_ARG (-1)`, `ADC_ERR_TIMEOUT (-2)`, `ADC_ERR_NOT_READY (-3)`; `adc_temp_mdegc()` returns `INT32_MIN` on invalid calibration inputs. |
| **5. Timeout Behavior** | Bounded `timeout_iters` (>0 validated) on `ADDIS`, `ADCAL`, `ADRDY`, and `EOC`. |
| **6. Error Recovery** | Refuses conversion (`ADC_ERR_NOT_READY`) if `ADEN == 0`; safely disables `DMAEN/DMACFG` before CPU polling conversions; clamps `raw > 4095` in `adc_raw_to_mv()` and guards divide-by-zero in `adc_calc_vdda_mv()`/`adc_temp_mdegc()`. |
| **7. Blocking / Non-Blocking** | **Bounded blocking** (init delays + conversion `EOC` wait). Pure conversion helpers (`adc_raw_to_mv`, `adc_calc_vdda_mv`, `adc_temp_mdegc`) are non-blocking O(1). |
| **8. ISR Safety** | **Not ISR-safe** for `adc_init_*` / `adc_read_*` (blocking waits and `Clock_DelayUs()`). Pure math helpers are ISR-safe. |
| **9. Thread Safety** | Not internally locked — `ADC1` and `ADC2` share `ADC12_COMMON->CCR`; initialize both before starting multi-task access and protect shared `ADCx` reads with a Mutex. |
| **10. Memory Use (`.su` @ `-O2`)** | Static RAM: `0 B`. Zero heap. Max stack: **`32 B`** (`adc_read_temp_mdegc`), `16 B` (`adc_init_single_ended`, `adc_read_vrefint_raw`, `adc_read_temp_raw`), `12 B` (`adc_read_channel`), `8 B` (`adc_temp_mdegc`), `0 B` (`adc_raw_to_mv`, `adc_calc_vdda_mv`). |
| **11. Resource Ownership** | Caller owns `ADC1`/`ADC2` and analog GPIO pins (`MODER=11b`, `PUPDR=00b`). Shared `ADC12_COMMON->CCR` checked before modifying `CKMODE`. |
| **12. Tests** | Cross-compile `PASS`; Host `test_drivers_math` (`100.00%` of 17 pure lines) + `test_peripherals_mock` (`66.67%` of `111` full lines; remaining post-`ADCAL` self-clear & ROM-deref `adc_read_temp_mdegc` lines verified on silicon); `gcc -fanalyzer` `0` warnings; Onboard Target Execution `PASS` (`CALFACT=0x45`, `VDDA=3288 mV`, `Die Temp=32.91 C`); external `PA0` pot sweep: `PENDING — USER CONNECTION REQUIRED`. |

### 3.11 `dma_g4.c` / `dma_g4.h`

| Attribute | Audited Assessment |
|---|---|
| **1. Initialization** | `dma_enable_clocks(DMA1/DMA2)` enables `DMAxEN | DMAMUX1EN` + readback; `dma_config_p2m_circular()` disables channel, clears individual flags (ES0430 §2.3.1), programs `DMAMUX1_Channel[0..15]`, `CPAR`, `CMAR`, `CNDTR`, `CCR`, optional `HTIE|TCIE|TEIE` + NVIC, then sets `EN`. |
| **2. Dependencies** | `stm32g474xx.h` (`RCC->AHB1ENR`, `DMA1/2`, `DMA1/2_Channel1..8`, `DMAMUX1_Channel0..15`, `NVIC`). |
| **3. Configuration** | `DMA1`/`DMA2` Ch `1..8` mapped to `DMAMUX1` Ch `0..15` via explicit CMSIS pointer tables (immune to `0x14`-byte stride trap); widths `1`, `2`, `4` bytes; verified RM0440 Table 91 `dmamux_req_id_t` enum. |
| **4. Error Codes** | `dma_status_t`: `DMA_OK (0)`, `DMA_ERR_INVALID_ARG (-1)`, `DMA_ERR_TRANSFER (-2)`, `DMA_ERR_TIMEOUT (-3)`; `dma_dmamux_channel_index()` returns `-1` on invalid input. |
| **5. Timeout Behavior** | `dma_config_p2m_circular()` has zero loops; `dma_mem2mem_sync()` polls `TCIF`/`TEIF` with bounded `timeout_iters` (>0 validated). |
| **6. Error Recovery** | `dma_mem2mem_sync()` validates pointer alignment (`width_bytes - 1`), disables channel (`CCR=0`), and clears individual flags (`CTCIF|CHTIF|CTEIF`, NEVER `CGIF` per ES0430 §2.3.1) on all exit paths (OK, error, or timeout). |
| **7. Blocking / Non-Blocking** | `dma_enable_clocks()`, `dma_clear_flags()`, `dma_config_p2m_circular()` are **non-blocking**; `dma_mem2mem_sync()` is **bounded blocking**. |
| **8. ISR Safety** | `dma_clear_flags()` (`IFCR = mask; (void)ISR; __DSB();`) is **ISR-safe** (ES0430 §2.3.1 + Erratum 838869 compliant). `dma_config_p2m_circular()` / `dma_mem2mem_sync()` are task/init-context only. |
| **9. Thread Safety** | Independent channels are thread-safe during data transfer; `dma_enable_clocks()` (RMW on `RCC->AHB1ENR`) should be called at init or under critical section. `dma->IFCR` writes are single atomic stores. |
| **10. Memory Use (`.su` @ `-O2`)** | Static RAM: `0 B` (`const` pointer lookup tables live in Flash `.rodata`). Zero heap. Max stack: **`56 B`** (`dma_config_p2m_circular`), `48 B` (`dma_mem2mem_sync`), `8 B` (`dma_clear_flags`), `0 B` (`dma_dmamux_channel_index`, `dma_enable_clocks`). |
| **11. Resource Ownership** | Caller owns DMA channel allocation, NVIC IRQ handlers, and DMA-accessible SRAM buffers (must use `SRAM1+SRAM2` `0x20000000..0x20017FFF` or CCM S-bus alias `0x20018000..0x2001FFFF`, NOT native CCM `0x10000000`!). |
| **12. Tests** | Cross-compile `PASS`; Host `test_drivers_math` (`100.00%` of 8 pure lines) + `test_peripherals_mock` (**`95.73%` of `117` full lines**, `100.00%` branches executed, ES0430 `CGIF==0` explicitly asserted); `gcc -fanalyzer` `0` warnings; Onboard Target Execution `PASS` (`MEM2MEM` `256/256` + `ADC1` circular `HT=20/TC=20/TE=0` + Project 02 25 ping-pong frames `ovr=0`). |

### 3.12 `iwdg_g4.c` / `iwdg_g4.h`

| Attribute | Audited Assessment |
|---|---|
| **1. Initialization** | `iwdg_init_ms(timeout_ms, freeze_in_debug, timeout_iters)`: optional `DBGMCU->APB1FZR1.DBG_IWDG_STOP` -> enable `LSI` + bounded wait `LSIRDY` -> `KR = 0xCCCC` (enable) -> `KR = 0x5555` (unlock) -> bounded wait `PVU|RVU == 0` -> program `PR` (`0..6`) & `RLR` (`0..4095`) -> bounded wait `PVU|RVU == 0` -> `KR = 0xAAAA` (reload). |
| **2. Dependencies** | `stm32g474xx.h` (`IWDG`, `RCC->CSR`, `DBGMCU->APB1FZR1`). |
| **3. Configuration** | Timeout `1..32,768 ms` at nominal `32 kHz` LSI (DS12288 `±5%` tolerance: `30.4..33.6 kHz`); optional debug freeze (`DBG_IWDG_STOP`). |
| **4. Error Codes** | `iwdg_status_t`: `IWDG_OK (0)`, `IWDG_ERR_INVALID_ARG (-1)`, `IWDG_ERR_TIMEOUT (-2)`. |
| **5. Timeout Behavior** | Bounded `timeout_iters` (>0 validated) on `LSIRDY` and both `IWDG->SR` (`PVU | RVU`) clock-domain sync waits. |
| **6. Error Recovery** | **Hardware Irreversibility Warning:** Once `IWDG_KEY_ENABLE (0xCCCC)` is written, STM32G4 silicon cannot stop IWDG until reset. `iwdg_was_reset_cause()` & `iwdg_clear_reset_flags()` enable post-reset watchdog fault diagnosis. |
| **7. Blocking / Non-Blocking** | `iwdg_init_ms()` is **bounded blocking** (LSI startup + APB-to-LSI sync); `iwdg_kick()`, `iwdg_was_reset_cause()`, `iwdg_clear_reset_flags()` are **non-blocking** O(1). |
| **8. ISR Safety** | `iwdg_kick()` (`IWDG->KR = 0xAAAA`) is hardware-atomic and ISR-safe, **though feeding watchdogs inside an ISR is an anti-pattern** (always feed from a dedicated supervisor task verifying all worker heartbeats). |
| **9. Thread Safety** | `iwdg_kick()` is single-write atomic; `iwdg_init_ms()` and `iwdg_clear_reset_flags()` are boot-time single-threaded. |
| **10. Memory Use (`.su` @ `-O2`)** | Static RAM: `0 B`. Zero heap. Max stack: **`56 B`** (`iwdg_calc_prescaler_reload`, `iwdg_init_ms`), `0 B` (`iwdg_was_reset_cause`, `iwdg_clear_reset_flags`, inline `iwdg_kick`). |
| **11. Resource Ownership** | Exclusively owns `IWDG`, `DBGMCU->APB1FZR1.DBG_IWDG_STOP`, and `RCC->CSR` (`LSION`/`RMVF`/`IWDGRSTF`). |
| **12. Tests** | Cross-compile `PASS`; Host `test_drivers_math` (`100.00%` of 15 pure lines + `±5%` LSI tolerance) + `test_peripherals_mock` (**`93.88%` of `49` full lines**, `93.75%` branches executed, key sequence `0x5555`/`0xAAAA` verified); `gcc -fanalyzer` `0` warnings; Onboard Target Execution `PASS` (`PR=2, RLR=3999, DBGMCU=0x1800, 6/6` kicks + FreeRTOS/Capstone supervision). |

---

## 4. Exact Host Unit Test & `gcov -b` Coverage Summary

Executed via `make -C tests clean test`, `make -C tests analyze`, and `make -C tests clean coverage`:

| Test Suite | Executable | Assertions Passed | Failed | `-fanalyzer` Warnings |
|---|---|---:|---:|---:|
| **Suite 1** | `test_c_foundations` | `116` | `0` | `0` |
| **Suite 2** | `test_ring_buffer` | `104` | `0` | `0` |
| **Suite 3** | `test_drivers_math` (`-DDRIVER_HOST_TEST`) | `292` | `0` | `0` |
| **Suite 4** | `test_gpio_mock` (`gpio_g4.c` + fault bounds) | `82` | `0` | `0` |
| **Suite 5 *(NEW)*** | `test_peripherals_mock` (all 8 peripheral drivers full register mock) | **`279`** | **`0`** | **`0`** |
| **TOTAL** | **All 5 Host Test Suites** | **`873`** | **`0`** | **`0`** |

| Driver File | `build/cov` (Core + Pure Helpers) | `build/cov_full` (Full Register-Mock) | Branches Executed (`cov_full`) |
|---|---:|---:|---:|
| `ring_buffer.c` | `100.00%` (`93/93`) | *(covered in `cov`)* | `100.00%` (`68/68` in `cov`) |
| `gpio_g4.c` | `100.00%` (`63/63`) | *(covered in `cov`)* | `100.00%` (`32/32` in `cov`) |
| `exti_g4.c` | `100.00%` (`6/6`) | **`100.00%`** (`88/88`) | `100.00%` (`60/60`) |
| `tim_g4.c` | `70.27%` (`26/37`) | **`87.04%`** (`141/162`) | `94.83%` (`110/116`) |
| `usart_g4.c` | `100.00%` (`15/15`) | **`91.41%`** (`117/128`) | `97.92%` (`94/96`) |
| `spi_g4.c` | `100.00%` (`8/8`) | **`94.05%`** (`79/84`) | `100.00%` (`72/72`) |
| `i2c_g4.c` | `100.00%` (`16/16`) | **`90.10%`** (`173/192`) | `93.85%` (`122/130`) |
| `adc_g4.c` | `100.00%` (`17/17`) | **`66.67%`** (`74/111`) | `76.92%` (`60/78`) |
| `dma_g4.c` | `100.00%` (`8/8`) | **`95.73%`** (`112/117`) | `100.00%` (`88/88`) |
| `iwdg_g4.c` | `100.00%` (`15/15`) | **`93.88%`** (`46/49`) | `93.75%` (`30/32`) |
| **Aggregate** | **`96.04%` (`267/278` lines)** | **`90.07%` (`916/1,017` lines)** | — |

---

## 5. Transparent Audit Observations & Future Hardening Backlog

1. **`usart_rx_irq_handler()` Bounded FIFO Drain:** Currently loops `while ((usart->ISR & USART_ISR_RXNE_RXFNE) != 0U)`. While safe on healthy STM32G4 silicon (hardware clears `RXNE` when `RDR` is read), capping iterations per ISR entry (e.g., `<= 16` bytes) guarantees bounded ISR WCET even under pathological bus/clock faults and enables direct host-mock loop testing.
2. **`clock_g4.c` Granular Failure Enum:** Returning a typed `clock_status_t` (`CLOCK_ERR_VOSF_TIMEOUT`, `CLOCK_ERR_HSE_TIMEOUT`, `CLOCK_ERR_PLL_TIMEOUT`, etc.) instead of `bool` would aid field telemetry diagnostics.
3. **`adc_read_temp_mdegc()` Factory ROM Abstraction:** Currently dereferences literal hardware addresses `ADC_VREFINT_CAL_ADDR` (`0x1FFF75AA`), `ADC_TS_CAL1_ADDR` (`0x1FFF75A8`), `ADC_TS_CAL2_ADDR` (`0x1FFF75CA`), preventing host-side execution of that wrapper without a thin ROM-read hook (underlying math functions `adc_calc_vdda_mv` and `adc_temp_mdegc` have `100.00%` host coverage and onboard silicon verification).
4. **External Bench Hardware Pending:** `spi_g4.c` (full physical loopback/slave) and `i2c_g4.c` (live slave ACK, Repeated START, and 9-clock bus recovery) remain `PENDING — USER CONNECTION REQUIRED` per `HARDWARE_VALIDATION/PENDING_EXTERNAL_CONNECTIONS.md`.
