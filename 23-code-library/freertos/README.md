# NUCLEO-G474RE Standalone FreeRTOS V11.1.0 Verification Firmware

> **Verification Status:** Compile-Verified (`arm-none-eabi-gcc 14.3.1`, 0 errors / 0 warnings on user code) — **READY FOR ONBOARD NUCLEO-G474RE HARDWARE TEST**  
> **Upstream Kernel Pin:** Official `FreeRTOS/FreeRTOS-Kernel` **`V11.1.0`** (Commit `dbf70559b27d39c1fdb68dfb9a32140b6a6777a0`, MIT License)  
> **Target Port:** `portable/GCC/ARM_CM4F` (ARM Cortex-M4F with `FPv4-SP-D16` hard-float ABI) + `portable/MemMang/heap_4.c`

---

## 1. Overview & Onboard-Only Hardware Scope

This standalone firmware project compiles and links official **FreeRTOS-Kernel `V11.1.0`** with the repository's verified STM32G474RE linker script ([`../templates/stm32g474re.ld`](../templates/stm32g474re.ld)) and startup vector table ([`../templates/Src/startup_stm32g474re.c`](../templates/Src/startup_stm32g474re.c)). It requires **zero external wires** and exercises only onboard NUCLEO-G474RE peripherals:

| Onboard Peripheral | Pin / Resource | Role in Firmware |
|---|---|---|
| **ST-LINK VCP (`USART2`)** | `PA2` (TX) / `PA3` (RX), `AF7`, `115200 8N1` | Mutex-protected structured test results & 1 Hz telemetry |
| **User LED (`LD2`)** | `PA5` (Output Push-Pull) | Toggled by `SupervisorTask` whenever all workers pass watchdog check-in |
| **User Button (`B1`)** | `PC13` (Active-HIGH, `EXTI13`, NVIC Prio `7`) | Pushes button event packets via `xQueueSendFromISR()` + `portYIELD_FROM_ISR()` |
| **Hardware Timer (`TIM2`)** | `10 Hz` (`100 ms`), NVIC Prio `6` (`>= 5`) | Wakes `ConsumerTask` directly via `vTaskNotifyGiveFromISR()` + `portYIELD_FROM_ISR()` |
| **Watchdog (`IWDG`)** | `~3.0 s` (`LSI/32`, `RLR=3000`, `DBG_IWDG_STOP=1`) | Fed exclusively by `SupervisorTask` when all worker tasks report healthy Event Group bits |

---

## 2. Demonstrated & Self-Verified RTOS Features

At boot, `SupervisorTask` (Priority `4`, static allocation) executes a 6-part automated self-test suite and logs structured `[PASS]` / `[FAIL]` telemetry over `USART2`:

1. **`NVIC_CONFIG`:** Confirms `NVIC_SetPriorityGrouping(0U)`, `PendSV` & `SysTick` at lowest priority `0xF0`, and `TIM2_IRQn` at kernel-managed priority `6 >= configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY (5)`.
2. **`SEMAPHORES`:** Verifies Binary Semaphore initial empty state & acquire/release semantics, plus Counting Semaphore (`uxMaxCount = 3`) saturation & drain.
3. **`PRIORITY_INHERITANCE`:** Spawns `PI_Low` (prio `1`) holding a Mutex while `PI_High` (prio `5`) blocks on it, proving FreeRTOS promotes `PI_Low` to priority `5` and restores it to priority `1` on release.
4. **`DEADLOCK_GUARD`:** Demonstrates bounded `xSemaphoreTake` timeout recovery (`15 ms`), immediate release of Mutex A (breaking Hold-and-Wait), and clean strict lock-order (`A -> B`) acquisition.
5. **`FROM_ISR_AND_QUEUE`:** Verifies `TIM2_IRQHandler` -> `vTaskNotifyGiveFromISR()` wakeup, `ProducerTask` -> `xQueueSend()` -> `ConsumerTask` `xQueueReceive()` sequence monotonicity, Cortex-M4F hardware FPU preservation across context switches, and static Software Timer execution.
6. **`STACK_HWM`:** Verifies `uxTaskGetStackHighWaterMark()` margins across `Supervisor`, `Consumer`, `Producer`, and `Monitor` tasks alongside `configCHECK_FOR_STACK_OVERFLOW = 2` and `configUSE_MALLOC_FAILED_HOOK = 1`.

---

## 3. Build & Flash Commands

```bash
cd 23-code-library/freertos

# Clean reproducible debug build (-Og -g3)
make clean all

# Release build (-O2 -g)
make clean all BUILD_TYPE=release

# Inspect ELF sections and verify FreeRTOS vector bindings
make size
make nm-check

# Flash onto connected NUCLEO-G474RE (ST-LINK V3E)
make flash          # via STM32_Programmer_CLI
make flash-openocd  # via OpenOCD
```

Open a serial terminal at **`115200 8N1`** on the ST-LINK Virtual COM Port to view boot self-tests and live 1 Hz task/stack/heap telemetry.
