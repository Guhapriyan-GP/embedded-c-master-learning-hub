# FreeRTOS Completion & Cross-Compilation Verification Report

> **Date:** 2026-11-18  
> **Branch:** `feature/complete-remediation`  
> **Firmware Project:** [`23-code-library/freertos/`](../23-code-library/freertos/README.md)  
> **Compile Status:** **Compile-Verified (`arm-none-eabi-gcc 14.3.1`, 0 errors / 0 warnings on user & kernel code)**  
> **Physical Hardware Status:** **`ONBOARD HARDWARE TESTED` (`6/6 ALL_PASSED` on physical NUCLEO-G474RE `MB1367 Rev C`, `COM8`, `115200 8N1` — see Section 5 below and [`AUDIT/HARDWARE_VALIDATION_SUMMARY.md`](HARDWARE_VALIDATION_SUMMARY.md))**

---

## 1. Official Upstream FreeRTOS-Kernel Version Pin & Provenance

The kernel was acquired directly from the official upstream repository and pruned to the minimal ARM Cortex-M4F subset (zero `.git` metadata inside the workspace). Provenance is recorded in [`23-code-library/freertos/FreeRTOS-Kernel/VERSION.txt`](../23-code-library/freertos/FreeRTOS-Kernel/VERSION.txt).

| Field | Verified Value |
|---|---|
| **Upstream Repository** | `https://github.com/FreeRTOS/FreeRTOS-Kernel` |
| **Release Tag** | **`V11.1.0`** (`tskKERNEL_VERSION_NUMBER = "V11.1.0"`) |
| **Annotated Tag Object SHA** | `f388a5c8078e152913e4eb3c5d75bf89561392df` |
| **Upstream Commit SHA** | **`dbf70559b27d39c1fdb68dfb9a32140b6a6777a0`** |
| **License** | MIT (`23-code-library/freertos/FreeRTOS-Kernel/LICENSE.md`) |
| **Architecture Port** | `portable/GCC/ARM_CM4F` (`port.c`, `portmacro.h` — `FPv4-SP-D16` hard-float ABI) |
| **Memory Manager** | `portable/MemMang/heap_4.c` (coalescing first-fit, `24 KB` heap) |
| **Core Kernel Files** | `tasks.c`, `queue.c`, `list.c`, `timers.c`, `event_groups.c`, `stream_buffer.c`, `include/*` |

---

## 2. Toolchain & Cross-Compilation Verification

- **Compiler:** `arm-none-eabi-gcc (GNU Tools for STM32 14.3.rel1.20251027-0700) 14.3.1 20250623`
- **Target Flags:** `-std=c11 -mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard -DSTM32G474xx`
- **User Code Warning Policy:** `-Wall -Wextra -Werror -Wpedantic -Wshadow -Wconversion -Wdouble-promotion -Wundef` (**0 errors, 0 warnings**)
- **Linker Script:** [`23-code-library/templates/stm32g474re.ld`](../23-code-library/templates/stm32g474re.ld) (512 KB Flash, 96 KB contiguous SRAM1+SRAM2, 32 KB CCMRAM)
- **Startup & CMSIS System:** [`23-code-library/templates/Src/startup_stm32g474re.c`](../23-code-library/templates/Src/startup_stm32g474re.c) & `system_stm32g4xx.c`
- **Built Artifacts (ready for flashing):**
  - `23-code-library/freertos/build/nucleo_g474re_freertos.elf`
  - `23-code-library/freertos/build/nucleo_g474re_freertos.bin`
  - `23-code-library/freertos/build/nucleo_g474re_freertos.hex`

### Build Profile Comparison

| Build Profile | Flags | `text` (B) | `data` (B) | `bss` (B) | FLASH Used | RAM Used (of 96 KB) | Compiler Warnings |
|---|---|---:|---:|---:|---:|---:|---|
| **`debug`** *(currently built)* | `-Og -g3` | `21,376` | `96` | `35,640` | `21,480 B` (`4.10%`) | `35,728 B` (`36.34%`) | **0** |
| **`release`** | `-O2 -g -DNDEBUG` | `21,564` | `96` | `35,640` | `21,668 B` (`4.13%`) | `35,728 B` (`36.34%`) | **0** |

### Detailed ELF Section Sizes (`arm-none-eabi-size -A`, `debug` build)

| Section | Size (Bytes) | Address (Hex) | Notes |
|---|---:|---|---|
| `.isr_vector` | `472` | `0x08000000` | Exact 118 vectors (16 core + 102 STM32G474RE IRQs), 512 B VTOR-aligned |
| `.text` | `19,232` | `0x080001E0` | FreeRTOS V11.1.0 kernel + `ARM_CM4F` port + BSP + verification tasks |
| `.rodata` | `1,664` | `0x08004D00` | Read-only strings & constants |
| `.ARM` / `.init_array` / `.fini_array` | `16` | `0x08005380` | Standard ARM EABI runtime tables |
| `.data` | `88` | `0x20000000` | Initialized globals (copied from Flash LMA) |
| `.bss` | `34,068` | `0x20000058` | `24 KB` `ucHeap` (`heap_4`) + static task stacks & TCBs |
| `.noinit` | `32` | `0x2000856C` | Persistent fault-frame capture buffer |
| `._user_heap_stack` | `1,540` | `0x2000858C` | Reserved MSP (`1 KB`) + min libc heap (`512 B`) |

---

## 3. Vector Table & FreeRTOS Handler Binding Verification

Verified directly via `arm-none-eabi-nm` and `arm-none-eabi-objdump` on `build/nucleo_g474re_freertos.elf`:

| Symbol | Address | Vector Table Slot | Raw Vector Word | Disassembly Verification |
|---|---|---|---|---|
| `SVC_Handler` (`vPortSVCHandler`) | `0x08003C90` | Slot 11 (`0x0800002C`) | `0x08003C91` | `ldmia.w r0!, {r4-r11, lr}; msr PSP, r0` |
| `PendSV_Handler` (`xPortPendSVHandler`) | `0x08003CE0` | Slot 14 (`0x08000038`) | `0x08003CE1` | `mrs r0, PSP; isb; tst.w lr, #16` *(FPU lazy-stack check confirmed!)* |
| `SysTick_Handler` (`xPortSysTickHandler`) | `0x08003E34` | Slot 15 (`0x0800003C`) | `0x08003E35` | `mov.w r3, #0x50; msr BASEPRI, r3` *(`MAX_SYSCALL_PRIO = 5 << 4 = 0x50` confirmed!)* |
| `TIM2_IRQHandler` | `0x08000E68` | Slot 44 (`0x080000B0`) | `0x08000E69` | Calls `vTaskNotifyGiveFromISR()` + `portYIELD_FROM_ISR()` at NVIC prio `6` |
| `EXTI15_10_IRQHandler` | `0x08000EC8` | Slot 56 (`0x080000E0`) | `0x08000EC9` | Calls `xQueueSendFromISR()` + `portYIELD_FROM_ISR()` at NVIC prio `7` |
| `Default_Handler` | `0x08001768` | Unused slots | `0x08001769` | Confirmed NONE of the 5 handlers above alias to `Default_Handler` |

---

## 4. `FreeRTOSConfig.h` & Onboard Firmware Coverage Checklist

- [x] `configCPU_CLOCK_HZ = SystemCoreClock` (`170,000,000 Hz` via RM0440 §6.1.5 Boost Mode in `Src/rtos_bsp.c`)
- [x] `configTICK_RATE_HZ = 1000` (`1 ms` tick)
- [x] `configPRIO_BITS = 4`, `configLIBRARY_LOWEST_INTERRUPT_PRIORITY = 15` (`0xF0`), `configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY = 5` (`0x50`)
- [x] Mandatory `NVIC_SetPriorityGrouping(0U)` invoked at start of `main()` before `vTaskStartScheduler()`
- [x] `configCHECK_FOR_STACK_OVERFLOW = 2`, `configUSE_MALLOC_FAILED_HOOK = 1`, `INCLUDE_uxTaskGetStackHighWaterMark = 1`
- [x] Static + Dynamic allocation enabled (`vApplicationGetIdleTaskMemory` + `vApplicationGetTimerTaskMemory` + `xTaskCreateStatic` for `SupervisorTask`, `ConsumerTask`, `ProducerTask` + `xTaskCreate` for `MonitorTask`)
- [x] Self-contained onboard verification suite (zero external wiring required):
  1. `NVIC_CONFIG` (`PRIGROUP=0`, `PendSV=0xF0`, `SysTick=0xF0`, `TIM2_PRIO=6`)
  2. `SEMAPHORES` (Binary & Counting max=3)
  3. `PRIORITY_INHERITANCE` (`base_prio=1 -> promoted_prio=5 -> restored_prio=1`)
  4. `DEADLOCK_GUARD` (bounded `15 ms` timeout recovery + strict lock order `A -> B`)
  5. `FROM_ISR_AND_QUEUE` (`TIM2` 10 Hz `vTaskNotifyGiveFromISR` + Producer/Consumer `xQueueSend`/`xQueueReceive` + Cortex-M4F FPU preservation + Software Timer)
  6. `STACK_HWM` (`uxTaskGetStackHighWaterMark` for all 4 persistent tasks)
- [x] `IWDG` (~3.0 s, `DBGMCU_APB1FZR1_DBG_IWDG_STOP = 1`) supervised via Event Group + `PA5` (`LD2`) heartbeat LED

---

## 5. Physical NUCLEO-G474RE Onboard Hardware Verification (Agent 4)

**Status:** **`ONBOARD HARDWARE TESTED — 6/6 ALL_PASSED`**  
**Board:** ST NUCLEO-G474RE (`MB1367 Rev C`, ST-LINK SN `<redacted-stlink-sn>`, FW `V3J17M11`, `VDD = 3.28 V`, Device ID `0x469`, Rev X, 512 KB Flash)  
**Flash Command:** `STM32_Programmer_CLI -c port=SWD mode=NORMAL -w build/nucleo_g474re_freertos.elf -v -rst` (`21.16 KB`, verified in `0.060 s`).

### Verbatim Captured `COM8` Output (`115200 8N1`, 8.0-Second Live Run)

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

*(Note: Manual physical press of onboard `B1` (`PC13`, `btn=0` during automated unattended run) remains an optional manual interaction check; `EXTI13` ISR/callback dispatch was separately verified via `EXTI->SWIER1` in `HARDWARE_VALIDATION/onboard_selftest/`.)*
