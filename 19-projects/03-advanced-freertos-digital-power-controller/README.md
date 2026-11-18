# Project 03: FreeRTOS Digital Power & Motor Controller (with Hardware `< 50 ns` Break Protection)

> **Verification Status:** Statically Reviewed Safety-Critical RTOS Architecture (Integration Blueprint — Refer to Compile-Verified Labs 15, 16, 17, 19 for Underlying Driver Patterns)

---

## 1. Project Overview & Safety-First Architecture

This capstone project defines a deterministic, **safety-first** digital power / motor controller on **STM32G474RE (`170 MHz`, Cortex-M4F)** running **FreeRTOS**. Crucially, **catastrophic overcurrent protection is implemented 100% in asynchronous silicon hardware (`COMP1_OUT` $\to$ `TIM1_BKIN` / `HRTIM1_FLT1`)** clearing `MOE` in **`< 50 ns`** with zero CPU or RTOS latency — while FreeRTOS tasks handle closed-loop regulation, telemetry, watchdog supervision, and UI display safely above the Idle Task!

```text
+========================================================================================+
|                        ASYNCHRONOUS SILICON SAFETY LAYER (< 50 ns)                     |
|  [ Shunt + OPAMP1 ] ──> [ COMP1 vs DAC3_CH1 ] ──(Direct Silicon Route)──> [ TIM1_BKIN /|
|                                                                           HRTIM1_FLT1 ]|
|                           * Clears TIM1->BDTR.MOE in <50 ns *                          |
|                           * Zero CPU / Zero RTOS Latency!  *                           |
|                                       │ (Post-Fault IRQ, NVIC Prio 5)                  |
|                                       ▼ (xTaskNotifyFromISR)                           |
+=======================================│================================================+
|                                FreeRTOS Kernel (PRIGROUP = 0)                          |
|                                       │                                                |
|  [ Task 3: Safety & Watchdog ] <──────┘        [ Task 1: Control Loop ]                |
|   - Priority: 4 (CRITICAL)                      - Priority: 3 (HIGH)                   |
|   - Logs latched hardware fault                 - Period: 1 kHz (vTaskDelayUntil)      |
|   - Kicks IWDG iff Tasks 1,2,4 healthy          - Reads DMA ADC / FMAC PI -> TIM1 CCR  |
|   - Controls fault recovery interlock                      │ (xQueueSend, non-blocking)|
|                                                            ▼                           |
|  [ Task 4: UI & OLED Display ]                 [ Task 2: Telemetry Logger ]            |
|   - Priority: 1 (LOW — NEVER 0/IDLE!)           - Priority: 2 (MEDIUM)                 |
|   - Updates I2C1 OLED @ 10 Hz                   - Mutex-protected USART2 stream        |
|                                                                                        |
|  [ FreeRTOS Idle Task (Priority 0) ] — NEVER starved! Runs heap cleanup & WFI sleep    |
+========================================================================================+
```

> [!CAUTION]
> **Two Critical Safety & RTOS Bugs Eliminated in This Architecture:**
> 1. **Hardware Break Overcurrent Trip (NEVER Poll in an RTOS Task!):** Polling `COMP1` inside a FreeRTOS task adds `10–100 µs` of scheduler/context-switch latency — long enough to destroy power MOSFETs during a dead short! Routing `COMP1_OUT` directly to **`TIM1_BKIN` (`TIM1->AF1 |= TIM1_AF1_BKCMP1E`, `BDTR.AOE = 0`)** or **`HRTIM1_FLT1`** disables complementary gate drives in **`< 50 ns`** even if the CPU is mid-ISR or faulted.
> 2. **Task 4 Priority Raised from `0` to `1` (No Idle Task Starvation):** Running an application UI task at `tskIDLE_PRIORITY` (`0`) competes with and starves the FreeRTOS Idle Task, preventing deleted-task heap reclamation and low-power idle hooks. Every user task runs at Priority `>= 1`.

---

## 2. Complete Real-Time Task Allocation, Stack Budgets & Deadlines

All tasks are created statically (`xTaskCreateStatic`) after calling **`NVIC_SetPriorityGrouping(0U)`** (`4` preemption bits, `0` subpriority bits) with `configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY = 5`, `configCHECK_FOR_STACK_OVERFLOW = 2`, and `configUSE_MUTEXES = 1`.

| Task | Priority | Period / Trigger | Stack Size (Words / Bytes) | Core Responsibility |
|---|---|---|---|---|
| **Hardware Break (`COMP1 -> TIM1_BKIN` / `HRTIM1_FLT1`)** | **Hardware Async** *(Post-fault ISR: NVIC Prio `5`)* | `< 50 ns` analog trip | N/A (Hardware) | Immediately clears `TIM1->BDTR.MOE = 0` (`AOE = 0` latched safe OFF); `TIM1_BRK_TIM15_IRQHandler` sends `xTaskNotifyFromISR()` to Task 3 |
| **Task 3: Safety & Watchdog Supervisor** | **4 (CRITICAL)** | Event-driven (`ulTaskNotifyTake`, `100 ms` timeout) | `256` words (`1024 B`) | Handles post-fault safe shutdown sequencing, verifies heartbeat bitmask from Tasks 1, 2, 4, and refreshes `IWDG->KR = 0xAAAAU` ONLY when all tasks are alive |
| **Task 1: Current & Voltage Control Loop** | **3 (HIGH)** | `1 kHz` (`1 ms` via `vTaskDelayUntil`) *(inner current loop optionally in TIM1/ADC ISR or FMAC)* | `384` words (`1536 B`) | Reads DMA-sampled phase current (`ADC1`, `42.5 MHz`), computes PI/3p3z control effort (FPU or `FMAC`), updates `TIM1` complementary `CCR1`/`CCR2` with dead-time (`BDTR.DTG`), posts non-blocking telemetry (`xQueueSend(..., 0)`) |
| **Task 2: Telemetry Logger** | **2 (MEDIUM)** | Queue-driven (`xQueueReceive(..., portMAX_DELAY)`) | **`512` words (`2048 B`)** *(safe for M4F FPU + `printf`)* | Acquires `uart_mutex` (priority inheritance), formats and transmits framed status logs over `USART2` (`115200 8N1`) |
| **Task 4: UI & OLED Display** | **1 (LOW — strictly `> tskIDLE_PRIORITY`)** | `10 Hz` (`100 ms` via `vTaskDelayUntil`) | `384` words (`1536 B`) | Reads atomic snapshot of setpoints/fault state and refreshes `I2C1` SSD1306 OLED (`400 kHz`, bounded `NACKF` timeout so unplugged display never hangs Task 4!) |
| **FreeRTOS Idle Task** | **0 (`tskIDLE_PRIORITY`)** | Background | `128` words (`512 B`) | Unstarved kernel cleanup and `__WFI()` CPU idle savings |

---

## 3. Concurrency, Priority & Safety Protection Patterns

1. **Priority Inversion Avoidance:** Shared `USART2` and `I2C1` buses are guarded by FreeRTOS Mutexes (`xSemaphoreCreateMutexStatic`) with automatic priority inheritance. (Note: priority inheritance bounds inversion latency; deadlock avoidance still requires strict lock-ordering discipline!)
2. **Interrupt Boundary Enforcement:**
   - Any ISR invoking `xTaskNotifyFromISR()` / `xQueueSendFromISR()` (e.g., `TIM1_BRK_TIM15_IRQHandler`, `DMA1_Channel1_IRQHandler`) is configured at NVIC Priority **`5..15`** (`>= configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY`).
   - ISRs always end with `__DSB(); portYIELD_FROM_ISR(xHigherPriorityTaskWoken);`.
3. **MPU Stack Guard & Crash Persistence:** Enables Cortex-M4 MPU stack guard + `.noinit` crash log (Lab 19) so any fault safely latches `TIM1` outputs OFF (`OSSI/OSSR`), records `CFSR`/`PC`, and reboots cleanly.

---

## 4. Verification & Testing Checklist

- [ ] **Hardware Trip Latency Test:** Inject step overcurrent onto `COMP1_INP` (`PA1`) while FreeRTOS scheduler is running (and while CPU is halted in GDB!) — confirm `TIM1_CH1`/`CH1N` complementary outputs drop to safe idle state in **`< 50 ns`** with `TIM1->BDTR.MOE == 0` latched OFF.
- [ ] **Priority & Idle Starvation Check:** Verify `NVIC_SetPriorityGrouping(0U)` (`PRIGROUP = 0`), Task 4 runs at Priority `1` (not `0`), and Idle Task CPU share remains `> 20%` under normal load.
- [ ] **Stack High-Water Mark Audit:** Verify every task (`uxTaskGetStackHighWaterMark`) retains `> 25%` free stack headroom with `configCHECK_FOR_STACK_OVERFLOW == 2`.
- [ ] **Watchdog Fault Injection:** Deliberately stall Task 1 or Task 4 — confirm Task 3 stops kicking `IWDG` and triggers safe hardware reset within `1.0 s`.
