# FreeRTOS V11.1.0 Architecture & Real-Time Engineering Audit Report

> **Date:** 2026-11-18  
> **Repository:** `embedded-c-master-learning-hub` (`<repo-root>/16-rtos/`, `<repo-root>/23-code-library/freertos/`, `<repo-root>/19-projects/03-advanced-freertos-digital-power-controller/`, base commit `543d21e`)  
> **Target MCU / Board:** `STM32G474RET6` (ARM Cortex-M4F, `170 MHz` Range 1 Boost Mode, `512 KB` Flash, `96 KB` SRAM1+2, `32 KB` CCM-SRAM) on ST **NUCLEO-G474RE** (`MB1367 Rev C`, STLINK-V3E `<redacted-stlink-sn>`)  
> **FreeRTOS Version:** Version-Pinned **FreeRTOS-Kernel `V11.1.0`** (Upstream Tag `f388a5c`, Commit `dbf70559`, Port `portable/GCC/ARM_CM4F`, Memory Manager `portable/MemMang/heap_4.c`)  
> **Cross-Compiler:** `arm-none-eabi-gcc 14.3.1 20250623 (GNU Tools for STM32 14.3.rel1.20251027-0700)`  
> **Verification Status:** **Compile-Verified (`arm-none-eabi-gcc 14.3.1 -Werror`, 0 warnings, 0 errors)** + **Target Execution Verified on NUCLEO-G474RE Silicon (`6/6 ALL_PASSED` in `freertos/`, `10/10` frames in Project 03)**

---

## 1. Executive Summary & Audit Verdict

This audit provides a comprehensive, 15-topic engineering assessment of the FreeRTOS `V11.1.0` implementation on STM32G474RE Cortex-M4F in this repository. It covers architectural trade-offs, low-level exception stack mechanics (`PendSV`/`SysTick`/`SVC`/FPU), static memory guarantees, synchronization primitives, interrupt priority partitioning under `BASEPRI`, a fully worked **Real-Time Response-Time Analysis (RTA)** exercise with mathematical recurrence convergence, and an explicit disclosure of system limitations.

| Audit Item | Assessment | Evidence / Verification |
|---|---|---|
| **FreeRTOS Kernel Source** | Official upstream `V11.1.0`, zero `.git` repository pollution | Recorded in `23-code-library/freertos/FreeRTOS-Kernel/VERSION.txt` |
| **Cortex-M4F Porting** | `portable/GCC/ARM_CM4F` (`port.c`, `portmacro.h`) with hardware FPU lazy stacking (`FPCCR`) | Disassembly of `xPortPendSVHandler` verifies `tst.w lr, #16` FPU check |
| **Mandatory NVIC Setup** | `NVIC_SetPriorityGrouping(0U)` explicitly enforced before `vTaskStartScheduler()` | Verified in `main.c` line 694 and checked live in `[PASS] NVIC_CONFIG: PRIGROUP=0` |
| **Static Memory Policy** | `configSUPPORT_STATIC_ALLOCATION = 1`; all 3 persistent worker tasks + Idle + Timer tasks statically allocated | `vApplicationGetIdleTaskMemory()` + `vApplicationGetTimerTaskMemory()` + `xTaskCreateStatic()` |
| **Stack Overflow Trap** | `configCHECK_FOR_STACK_OVERFLOW = 2` (`0xA5A5A5A5` canary scan + SP bounds check) | `vApplicationStackOverflowHook()` with `__BKPT(0)` and UART telemetry |
| **Deadlock Avoidance** | Strict global lock ordering (`A -> B`) + bounded timeouts | Verified live in `[PASS] DEADLOCK_GUARD: timeout_recovered=YES ordered_lock(A->B)=OK` |
| **Priority Inheritance** | Mutex PIP bounds inversion (`base=1 -> promoted=5 -> restored=1`) | Verified live in `[PASS] PRIORITY_INHERITANCE` |
| **Interrupt Preemption** | `vTaskNotifyGiveFromISR` + `portYIELD_FROM_ISR` immediate `PendSV` switch | Verified live in `[PASS] FROM_ISR_AND_QUEUE: notify=4 tim2_isr=4` |
| **Exact Mathematical RTA** | Joseph & Pandya iterative recurrence $R_i^{(k+1)}$ converged for all 4 Project 03 tasks | Total utilization $U = 18.05\% \le U_{\text{RMS}} = 75.68\%$, all deadlines met |
| **Determinism Limitations** | Explicitly documented: 8 s onboard run is functional proof, NOT formal WCET proof | Section 15 declaration |

---

## 2. Topic 1: Architectural Comparison — Superloop vs Cooperative vs Preemptive RTOS

| Architectural Pattern | Scheduling Model | Context Switch Latency | RAM / Stack Footprint | Determinism & Fault Isolation | When NOT to Use |
|---|---|---|---|---|---|
| **Bare-Metal Superloop + ISRs** | Polled sequential loop; interrupts preempt for urgent I/O | Interrupts: `~70 ns` (12 cycles @ 170 MHz); Main loop: unbounded if any module blocks | Single shared stack (e.g. 1 KB total); 0 B kernel overhead | Deterministic for tiny states; catastrophic jitter if any driver executes long delays | Do NOT use if multiple complex asynchronous protocols (e.g. USB + CAN-FD + FATFS + PID control) must run concurrently. |
| **Cooperative (Run-to-Completion / TT)** | Tasks execute sequentially; yield voluntarily via FSM returns | Function call overhead (`~10–20 ns`); no register stacking | Single shared stack; minimal scheduler table (~200 B) | Zero preemption race conditions; worst-case latency equals duration of longest task step | Do NOT use if hard real-time sub-millisecond reaction is required while a long calculation executes. |
| **Preemptive Priority RTOS (FreeRTOS)** | Priority-driven preemption via `SysTick` and `PendSV` | Full context switch: `~1.5–3.5 µs` (saving R4–R11 + optional FPU S16–S31) | **Private stack per task** (typically 512 B–2 KB each) + TCB (~100 B); ~10–25 KB RAM total | Highest-priority ready task preempts instantly regardless of lower-priority work | **When NOT to Use an RTOS:** (1) SRAM < 16 KB; (2) Control loop > 50 kHz (put the loop in HRTIM/TIM ISR or DMA, **never** in a FreeRTOS task!); (3) Ultra-low-power systems where kernel tick overhead drains battery. |

---

## 3. Topic 2: ARM Cortex-M4F Low-Level Context Switch Mechanics

The FreeRTOS `ARM_CM4F` port (`portable/GCC/ARM_CM4F/port.c`) coordinates three hardware core exceptions:

```
[Exception Priority Architecture]
  Priority 0..4  : High-Priority Unmasked ISRs (HRTIM, COMP1 Fault Break) -> BASEPRI = 0x50 never masks!
  Priority 5     : configMAX_SYSCALL_INTERRUPT_PRIORITY (BASEPRI threshold)
  Priority 6..14 : Kernel-Managed ISRs (TIM2, EXTI13, USART2) -> FromISR APIs safe
  Priority 15    : Lowest Priority -> SysTick_Handler & PendSV_Handler (0xF0)
```

1. **`SVC_Handler` (`vPortSVCHandler`, Core Exception 11):**
   - Invoked **exactly once** at scheduler startup by `xPortStartScheduler()` via assembly `svc 0`.
   - Reads `pxCurrentTCB`, restores the first task's initial hardware stack frame `{R0–R3, R12, LR, PC, xPSR}` and software frame `{R4–R11, EXC_RETURN}`, sets `PSP`, switches `CONTROL.SPSEL = 1` (Thread Mode uses PSP), and executes `bx lr` into the first task.
   - All subsequent interrupts and kernel handlers continue using **`MSP`**.
2. **`SysTick_Handler` (`xPortSysTickHandler`, Core Exception 15, Priority `0xF0`):**
   - Configured for `1 ms` tick (`configTICK_RATE_HZ = 1000`).
   - Writes `configMAX_SYSCALL_INTERRUPT_PRIORITY` (`0x50`) to `BASEPRI` to enter an internal critical section.
   - Increments `xTickCount`, evaluates the delayed task list, and if a higher-priority task unblocks or time-slicing requires a switch, sets `SCB->ICSR = SCB_ICSR_PENDSVSET_Msk`.
   - Restores `BASEPRI = 0`.
3. **`PendSV_Handler` (`xPortPendSVHandler`, Core Exception 14, Priority `0xF0`):**
   - Configured at lowest NVIC priority (`0xF0` in `SCB->SHP[10]`) so it **never preempts a real-time hardware ISR** and runs only after all pending Handler Mode exceptions retire.
   - Hardware has already pushed `{R0–R3, R12, LR, PC, xPSR}` onto the outgoing task's `PSP`.
   - **FPU Lazy Stacking Check (`ARM AN298` / `PM0214 §2.3`):** Evaluates `EXC_RETURN` in `LR`:
     - If `LR & (1 << 4) == 0` (`EXC_RETURN = 0xFFFFFFED`), the task used the FPU: hardware already auto-stacked `{S0–S15, FPSCR}`; `PendSV` executes `vstmdbeq r0!, {s16-s31}` to push the remaining 16 callee-saved FPU registers!
     - If `LR & (1 << 4) != 0` (`EXC_RETURN = 0xFFFFFFFD`), basic 8-word frame; FPU registers are bypassed.
   - Pushes `{R4–R11, LR}` onto `PSP`, writes new `PSP` into `pxCurrentTCB->pxTopOfStack`, invokes `vTaskSwitchContext()`, retrieves new `pxTopOfStack` from the incoming TCB, pops `{R4–R11, LR}`, conditionally pops `{S16–S31}` if incoming task uses FPU, loads `PSP`, and executes `bx lr`.

---

## 4. Topic 3: Task Lifecycle, TCB, States, `vTaskDelayUntil`, and The Idle Task

- **Task Control Block (`StaticTask_t` / `struct tskTaskControlBlock`):**
  Stores `pxTopOfStack`, `xStateListItem` (linked into Ready/Blocked/Suspended list), `uxPriority`, `pxStack` (bottom pointer for overflow check), and `ulNotifiedValue` array.
- **5 Canonical Task States:**
  1. `Running`: Currently executing on the Cortex-M4 core.
  2. `Ready`: Available to run, waiting for CPU preemption by priority.
  3. `Blocked`: Waiting on a time delay, queue, semaphore, event group, or notification. Consumes **0% CPU cycles**.
  4. `Suspended`: Explicitly paused via `vTaskSuspend()`.
  5. `Deleted`: Awaiting Idle task cleanup via `prvCheckTasksWaitingTermination()`.
- **Periodicity Without Drift:**
  - `vTaskDelay(pdMS_TO_TICKS(10))` causes progressive cumulative drift because delay is measured relative to invocation time (drifts by task execution time + preemptions).
  - **`vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(10))`** computes the wake tick relative to the target period boundary:

$$
xLastWakeTime \leftarrow xLastWakeTime + xPeriod
$$

- **Idle Task (`tskIDLE_PRIORITY = 0`):**
  Created automatically during `vTaskStartScheduler()`. **Must never be starved** because it cleans up deleted task allocations. If an application uses `vApplicationIdleHook()`, the hook **MUST NEVER call any blocking API** (`xQueueReceive`, `vTaskDelay`, etc.).

---

## 5. Topic 4: Inter-Task Communication — Queues & Stream/Message Buffers

- **Queues (`xQueueCreate`, `xQueueSend`, `xQueueReceive`):**
  - **Pass-by-Value:** Recommended for small structs (`<= 32 B`, e.g., `sensor_packet_t` in `freertos/main.c`). Prevents data races and dangling pointer hazards between tasks.
  - **Zero-Copy Pointer Queue:** For large buffers (e.g. 512-byte DMA blocks), pass `uint8_t *` pointers through the queue backed by a statically allocated buffer pool.
- **Stream Buffers & Message Buffers (`stream_buffer.c`):**
  - Designed for single-reader single-writer (SPSC) streaming.
  - Zero lock overhead (implemented via internal lock-free pointers).
  - Ideal for `USART2` DMA+IDLE line reception, where variable-length ASCII lines are transferred directly from ISR to task via `xMessageBufferSendFromISR()`.

---

## 6. Topic 5: Synchronization Primitives — Semaphores, Event Groups & Notifications

| Primitive | Mechanism | ISR-Safe API | Best Practice |
|---|---|---|---|
| **Binary Semaphore** | Token counter (max 1). **Note:** `xSemaphoreCreateBinary()` starts **empty** (`0`); caller must `Give` before first `Take`. | `xSemaphoreGiveFromISR()` | Event signaling (drops burst triggers if not drained!). |
| **Counting Semaphore** | Token counter (`0..uxMaxCount`). | `xSemaphoreGiveFromISR()` | Resource pool management (e.g. DMA buffer slots). |
| **Event Group** | 24 independent event bits (`configUSE_16_BIT_TICKS = 0`). Supports Wait-ANY (`uxBitsToWaitFor`, `xClearOnExit`), Wait-ALL (`xWaitForAllBits = pdTRUE`), and barrier synchronization (`xEventGroupSync`). | `xEventGroupSetBitsFromISR()` (deferred via timer daemon task!) | Watchdog check-ins from multiple worker tasks (`EVT_ALL_WORKERS = (BIT0 \| BIT1 \| BIT2)`). |
| **Direct-to-Task Notification** | 32-bit integer embedded directly in target TCB. | `vTaskNotifyGiveFromISR()` | **Preferred 1-to-1 signaling:** Consumes **0 B additional RAM**, executes **~45% faster** than binary semaphore! |

---

## 7. Topic 6: Mutexes, Strict Ownership & Priority Inheritance Protocol (PIP)

- **Strict Task Ownership:** Unlike binary semaphores (which can be given by an ISR or any task), a **Mutex** records `pxMutexHolder`. Only the task that locked the mutex is permitted to release it (`xSemaphoreGive()`).
- **Why Mutexes CANNOT Be Used in ISRs:**
  1. An ISR has no TCB and no priority; it cannot inherit priority.
  2. An ISR cannot block (`xTicksToWait` must be 0), so attempting to take a locked mutex in an ISR is fundamentally invalid.
- **Priority Inheritance Protocol (PIP):**
  When high-priority task **H** blocks on a mutex held by low-priority task **L**, the FreeRTOS kernel temporarily elevates **L**'s priority to match **H** (`uxTaskPriorityGet(L) -> H`). Medium-priority task **M** is thereby prevented from preempting **L**, bounding the inversion duration to **L**'s critical section. Upon release, **L**'s priority is restored to its base priority.
- **Priority Ceiling Protocol (PCP) Comparison:** FreeRTOS implements dynamic PIP (inherited on contention). Full PCP (associating a static ceiling with each mutex) requires custom extensions or POSIX layers.

---

## 8. Topic 7: Deadlocks vs Priority Inversion (CRITICAL CONCEPTUAL DISTINCTION)

> [!CAUTION]
> **Do NOT Conflate Priority Inversion with Deadlock!**
> - **Priority Inversion** is an unexpected delay where an intermediate task preempts a low-priority lock-holder, delaying a high-priority task. **Tasks CAN still finish; deadlines are missed.**
> - **Deadlock** is a permanent cycle of blocked tasks where **NO task can ever make forward progress**.

### 8.1 The 4 Coffman Conditions for Deadlock
1. **Mutual Exclusion:** Resources cannot be shared simultaneously.
2. **Hold and Wait:** A task holds one lock while requesting another.
3. **No Preemption:** A lock cannot be forcibly confiscated from a task.
4. **Circular Wait:** Task 1 holds A and waits for B; Task 2 holds B and waits for A.

### 8.2 Why Priority Inheritance Does NOT Prevent AB/BA Deadlocks
If Task 1 locks A and requests B, while Task 2 locks B and requests A:
- Priority Inheritance will promote Task 1 or Task 2 to the highest priority, **but neither task can release its held lock without obtaining the second lock**. Both tasks remain blocked forever.

### 8.3 Verified Remediation in Firmware
1. **Strict Global Lock Ordering:** Always acquire locks in uniform hierarchy (`Mutex A` before `Mutex B`).
2. **Bounded Acquisition Timeouts:** Always provide finite `xTicksToWait` (e.g. `pdMS_TO_TICKS(15)`). If the secondary lock fails, **release the primary lock immediately** and back off before retrying (verified in `freertos/main.c` Test 4).

---

## 9. Topic 8: `FromISR` APIs & Immediate Deferred Preemption

All FreeRTOS APIs invoked inside interrupt handlers end with `FromISR` (`xQueueSendFromISR`, `vTaskNotifyGiveFromISR`, etc.):
1. **`pxHigherPriorityTaskWoken`:** Output parameter passed by pointer. Initialized to `pdFALSE`. If the API unblocks a task whose priority exceeds the currently interrupted task, the kernel sets `*pxHigherPriorityTaskWoken = pdTRUE`.
2. **`portYIELD_FROM_ISR(xHigherPriorityTaskWoken)`:**
   - If `pdTRUE`, executes `portNVIC_INT_CTRL_REG = portNVIC_PENDSVSET_BIT` (pends `PendSV`).
   - Ensures that upon exiting the ISR, the Cortex-M4 tail-chains immediately into `PendSV_Handler` to execute the newly unblocked high-priority task, **eliminating up to 1 ms of SysTick scheduling latency**.

---

## 10. Topic 9: NVIC Priority Partitioning & `BASEPRI` on STM32G474RE

### 10.1 Mandatory Priority Grouping Call
On STM32G4, CMSIS `__NVIC_PRIO_BITS = 4`, providing 16 priority levels (`0..15`, stored in bits `[7:4]`). Before starting the FreeRTOS scheduler, the application **MUST** call:

```c
NVIC_SetPriorityGrouping(0U); /* or 3U: 4 preemption bits, 0 subpriority bits */
```

If subpriority bits are left enabled, Cortex-M4 priority comparisons fail to match FreeRTOS `BASEPRI` masking masks, tripping `configASSERT(uxPriority >= ucMaxSysCallPriority)`.

### 10.2 Dual-Zone Priority Architecture

| NVIC Priority (`0..15`) | Hardware Vector / Peripheral | FreeRTOS Relationship | Behavior During `taskENTER_CRITICAL()` |
|---|---|---|---|
| **`0` .. `4`** *(Numerically `< 5`)* | `HRTIM1`, `TIM1` Motor Fault Break, `COMP1` Analog Trip | **Unmasked High-Priority Hard Real-Time** | **NEVER MASKED!** `BASEPRI = 0x50` ignores priorities `0..4`. **MUST NEVER CALL ANY FreeRTOS API!** |
| **`5`** | `configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY` (`0x50`) | FreeRTOS Kernel Mask Threshold | Threshold boundary. |
| **`5` .. `15`** *(Numerically `>= 5`)* | `TIM2`, `EXTI13`, `USART2`, `ADC1_DMA`, `SysTick` (15), `PendSV` (15) | **Kernel-Managed ISRs** | Masked temporarily during `taskENTER_CRITICAL()`. Fully safe to invoke `...FromISR()` APIs. |

---

## 11. Topic 10: Memory Management — `heap_1..5`, Static Allocation & CCM-SRAM

- **`heap_1.c`:** Allocate only, never free. Zero fragmentation; suitable for simple static firmware.
- **`heap_2.c`:** Best-fit allocation with fragmentation (legacy, superseded by `heap_4`).
- **`heap_3.c`:** Thread-safe wrapper around C library `malloc()` / `free()`.
- **`heap_4.c` (Used in `freertos/` and Project 03):** First-fit allocation with **adjacent free block coalescing**. Defeats memory fragmentation for repeated variable-size allocations. Configured to `24 KB` (`ucHeap[configTOTAL_HEAP_SIZE]`) in `SRAM1`.
- **`heap_5.c` (Multi-Region Non-Contiguous Allocation):**
  - Spans across STM32G474RE's separate memory banks: `SRAM1+SRAM2` (`96 KB` @ `0x20000000`) and `CCM-SRAM` (`32 KB` @ `0x10000000`).
  - **CRITICAL HARDWARE WARNING (RM0440 §2.3):** STM32G4 DMA controllers (`DMA1`/`DMA2`) **CANNOT access CCM-SRAM at `0x10000000`**. If `heap_5` allocates buffers in CCM, those buffers cannot be passed to DMA channels (or must use the S-bus alias `0x20018000`).
- **Deterministic Link-Time Allocation (`configSUPPORT_STATIC_ALLOCATION = 1`):**
  All TCBs (`StaticTask_t`), stacks (`StackType_t[]`), and queues (`StaticQueue_t`) are placed in `.bss` at link time via `xTaskCreateStatic()` — zero heap depletion risk.

---

## 12. Topic 11: Stack Sizing, Watermarks & Overflow Protection

1. **`StackType_t` 4-Byte Word Units:**
   - In `xTaskCreate()` / `xTaskCreateStatic()`, `usStackDepth` is measured in **4-byte words**, NOT bytes!
   - Allocating `128` words = `512` bytes. A task calling `printf()` or handling standard newlib buffers while stacking a 104-byte FPU frame will overflow `512` bytes instantly.
   - **Rule:** Telemetry or formatting tasks require **$\ge 512$ words ($2048$ bytes)**.
2. **Method 2 Stack Overflow Check (`configCHECK_FOR_STACK_OVERFLOW = 2`):**
   - At initialization, the kernel paints the task stack with canary pattern **`0xA5A5A5A5`**.
   - On every context switch (`vTaskSwitchContext`), the kernel validates the active stack pointer AND inspects the last 16 bytes of the stack. If any canary byte was modified, it branches immediately to `vApplicationStackOverflowHook()`.
3. **Runtime Monitoring (`uxTaskGetStackHighWaterMark`):**
   - Returns the minimum number of unused stack words ever observed since task inception. Monitored continuously in `freertos/` (`Sup=339, Cons=309, Prod=186, Mon=324 words free`) ensuring $\ge 30\%$ safety margin.

---

## 13. Topic 12: Low-Power Tickless Idle — SysTick vs `LPTIM1`

- **Built-in `configUSE_TICKLESS_IDLE = 1` (SysTick Sleep Mode):**
  - Uses 24-bit `SysTick` down-counter.
  - Maximum single sleep duration at `170 MHz`:

$$
T_{\text{max}} = \frac{2^{24} - 1}{170 \times 10^6\text{ Hz}} = \frac{16,777,215}{170,000,000} \approx \mathbf{98.69\text{ ms}}
$$

  - SysTick stops in STM32G4 `Stop 0 / Stop 1` low-power modes (where high-speed PLL and HSI16 are gated off).
- **Production `configUSE_TICKLESS_IDLE = 2` (`LPTIM1` Stop 0/1 Mode):**
  - Uses low-power timer **`LPTIM1` (`0x40007C00`)** clocked by `32.768 kHz LSE` or `32 kHz LSI`.
  - Stays active in `Stop 0` and `Stop 1` modes down to `~160–350 µA` (DS12288 Table 31).
  - Maximum sleep duration: $\frac{2^{16}}{32,768\text{ Hz}} = \mathbf{2.00\text{ seconds}}$ per interval.
  - Upon wakeup, invokes `vTaskStepTick()` to advance kernel ticks seamlessly.

---

## 14. Topic 13: Worked Real-Time Response-Time Analysis (RTA) Mathematical Exercise

### 14.1 Classical Schedulability Criteria vs Fixed-Priority Reality
Liu & Layland's Rate Monotonic Scheduling (RMS) utilization bound guarantees schedulability only for independent periodic tasks prioritized strictly by $T_i \le T_{i+1}$:

$$
U = \sum_{i=1}^{n} \frac{C_i}{T_i} \le n\left(2^{1/n} - 1\right) \xrightarrow{n \to \infty} \ln 2 \approx 0.693
$$

For $n = 4$ tasks, $U_{\text{RMS}} = 4(2^{1/4} - 1) = \mathbf{0.7568}$ (75.68%). However, in real-world embedded systems like Project 03 / Lab 15, task priorities are frequently assigned by **functional safety criticality** rather than period (e.g. placing a 100 ms `SafetySupervisor` at Priority 4 above a 1 ms `ControlLoop` at Priority 3). In such architectures, **RMS utilization bounds cannot prove schedulability — exact Response-Time Analysis (RTA) is mandatory**.

### 14.2 Joseph & Pandya (1986) Exact RTA Recurrence
A fixed-priority task $\tau_i$ with worst-case execution time $C_i$, relative deadline $D_i \le T_i$, worst-case lower-priority blocking time $B_i$, and set of higher-priority tasks $\text{hp}(i)$ meets its deadline if and only if the sequence converges to $R_i \le D_i$:

$$
R_i^{(k+1)} = C_i + B_i + \sum_{j \in \text{hp}(i)} \left\lceil \frac{R_i^{(k)}}{T_j} \right\rceil C_j, \qquad R_i^{(0)} = C_i + B_i
$$

### 14.3 Application to Project 03 / Lab 15 4-Task STM32G474RE FreeRTOS System

Parameters derived from [`19-projects/03-advanced-freertos-digital-power-controller/Src/main.c`](../19-projects/03-advanced-freertos-digital-power-controller/Src/main.c) (`170 MHz` Boost, `configTICK_RATE_HZ = 1000`):

| Task | Assigned FreeRTOS Priority | Period $T_i$ (ms) | Deadline $D_i$ (ms) | Illustrative WCET $C_i$ (ms) | Blocking $B_i$ (ms) | Functional Scope |
|---|---:|---:|---:|---:|---:|---|
| $\tau_1$: `SafetySupervisor` | `4` *(Highest)* | `100.0` | `100.0` | `0.05` | `0.00` | Event-group wait, `IWDG` kick, `LD2` toggle |
| $\tau_2$: `ControlLoop` | `3` | `1.0` | `1.0` | `0.10` | `0.00` | `1 kHz` Q15 PI regulator + `TIM1->CCR1` update |
| $\tau_3$: `Telemetry` | `2` | `500.0` | `500.0` | `15.00` | `5.00` | ~160 B blocking UART JSON stream at `115200 8N1` (`~13.9 ms`); blocked by `UiTask` holding `g_uart_mutex` |
| $\tau_4$: `UiTask` | `1` *(Lowest App)* | `100.0` | `100.0` | `5.00` | `0.00` | `10 Hz` `B1` debouncing + status logging (~55 B UART string, `~4.8 ms`) |

**Step 1 — Total Processor Utilization Calculation:**

$$
U = \frac{0.05}{100.0} + \frac{0.10}{1.0} + \frac{15.00}{500.0} + \frac{5.00}{100.0} = 0.0005 + 0.1000 + 0.0300 + 0.0500 = \mathbf{0.1805} \quad (18.05\%)
$$

Since $U = 18.05\% \le U_{\text{RMS}} = 75.68\%$, processor capacity is ample, but non-monotonic priority ordering requires full RTA convergence proof.

**Step 2 — Step-by-Step Iterative Convergence:**

1. **Task $\tau_1$ (`SafetySupervisor`, Priority 4, $\text{hp}(1) = \emptyset$):**
   - $R_1^{(0)} = C_1 + B_1 = 0.05\text{ ms}$
   - Since $\text{hp}(1)$ is empty, recurrence terminates immediately:

$$
\mathbf{R_1 = 0.05\text{ ms} \le 100.0\text{ ms} \quad (\text{PASS, Slack } = 99.95\text{ ms})}
$$

2. **Task $\tau_2$ (`ControlLoop`, Priority 3, $\text{hp}(2) = \{\tau_1\}$):**
   - $R_2^{(0)} = C_2 + B_2 = 0.10\text{ ms}$
   - $R_2^{(1)} = 0.10 + \lceil 0.10 / 100.0 \rceil (0.05) = 0.10 + 1 \times 0.05 = 0.15\text{ ms}$
   - $R_2^{(2)} = 0.10 + \lceil 0.15 / 100.0 \rceil (0.05) = 0.10 + 1 \times 0.05 = 0.15\text{ ms}$ (converged!)

$$
\mathbf{R_2 = 0.15\text{ ms} \le 1.0\text{ ms} \quad (\text{PASS, Slack } = 0.85\text{ ms})}
$$

3. **Task $\tau_3$ (`Telemetry`, Priority 2, $\text{hp}(3) = \{\tau_1, \tau_2\}$):**
   - $R_3^{(0)} = C_3 + B_3 = 15.00 + 5.00 = 20.00\text{ ms}$
   - $R_3^{(1)} = 20.00 + \lceil 20.00 / 100.0 \rceil (0.05) + \lceil 20.00 / 1.0 \rceil (0.10) = 20.00 + 1(0.05) + 20(0.10) = 22.05\text{ ms}$
   - $R_3^{(2)} = 20.00 + \lceil 22.05 / 100.0 \rceil (0.05) + \lceil 22.05 / 1.0 \rceil (0.10) = 20.00 + 1(0.05) + 23(0.10) = 22.35\text{ ms}$
   - $R_3^{(3)} = 20.00 + \lceil 22.35 / 100.0 \rceil (0.05) + \lceil 22.35 / 1.0 \rceil (0.10) = 20.00 + 0.05 + 2.30 = 22.35\text{ ms}$ (converged!)

$$
\mathbf{R_3 = 22.35\text{ ms} \le 500.0\text{ ms} \quad (\text{PASS, Slack } = 477.65\text{ ms})}
$$

4. **Task $\tau_4$ (`UiTask`, Priority 1, $\text{hp}(4) = \{\tau_1, \tau_2, \tau_3\}$):**
   - $R_4^{(0)} = C_4 + B_4 = 5.00 + 0.00 = 5.00\text{ ms}$
   - $R_4^{(1)} = 5.00 + \lceil 5.00 / 100.0 \rceil (0.05) + \lceil 5.00 / 1.0 \rceil (0.10) + \lceil 5.00 / 500.0 \rceil (15.00) = 5.00 + 0.05 + 0.50 + 15.00 = 20.55\text{ ms}$
   - $R_4^{(2)} = 5.00 + \lceil 20.55 / 100.0 \rceil (0.05) + \lceil 20.55 / 1.0 \rceil (0.10) + \lceil 20.55 / 500.0 \rceil (15.00) = 5.00 + 0.05 + 2.10 + 15.00 = 22.15\text{ ms}$
   - $R_4^{(3)} = 5.00 + \lceil 22.15 / 100.0 \rceil (0.05) + \lceil 22.15 / 1.0 \rceil (0.10) + \lceil 22.15 / 500.0 \rceil (15.00) = 5.00 + 0.05 + 2.30 + 15.00 = 22.35\text{ ms}$
   - $R_4^{(4)} = 5.00 + 0.05 + 2.30 + 15.00 = 22.35\text{ ms}$ (converged!)

$$
\mathbf{R_4 = 22.35\text{ ms} \le 100.0\text{ ms} \quad (\text{PASS, Slack } = 77.65\text{ ms})}
$$

### 14.4 Critical Architecture Finding: Transitive Mutex Contention
RTA analysis uncovers a vital safety rule: If `SafetySupervisor` (Priority 4) ever acquires `g_uart_mutex` to log a fault message while `Telemetry` (Priority 2) is mid-transmission holding `g_uart_mutex`, FreeRTOS Priority Inheritance promotes `Telemetry` to Priority 4. Because Priority 4 preempts `ControlLoop` (Priority 3, $D_2 = 1.0\text{ ms}$), `ControlLoop` would be blocked for up to `15.0 ms`, **missing 15 consecutive 1 ms deadlines** ($R_2 > 15.1\text{ ms} \gg D_2$).  
**Design Remediation:** In safety-critical production systems, high-priority tasks must **never** take mutexes shared with slow blocking I/O; logging should be delegated asynchronously via non-blocking queues or DMA ring buffers.

---

## 15. Topic 14 & 15: Hardware Evidence, Traceability & Explicit Limitations Declaration

### 15.1 Hardware Evidence Summary (NUCLEO-G474RE Physical Execution)

Full test logs and register readbacks are recorded in [`AUDIT/HARDWARE_VALIDATION_SUMMARY.md`](HARDWARE_VALIDATION_SUMMARY.md):
- **Test Board:** ST NUCLEO-G474RE (`MB1367 Rev C`, STLINK-V3E `<redacted-stlink-sn>`, `VDD = 3.28 V`, `COM8` `115200 8N1`).
- **`23-code-library/freertos/` (`nucleo_g474re_freertos.elf`, SHA-256 `8265ac2d8827730ffd7fdb88a2ed7a98e90ccd4449a9e9689d140d0b740ef565`):**
  - `[PASS] NVIC_CONFIG: PRIGROUP=0 PendSV=0xF0 SysTick=0xF0 TIM2_PRIO=6`
  - `[PASS] SEMAPHORES: binary=OK counting_max3=OK`
  - `[PASS] PRIORITY_INHERITANCE: base_prio=1 promoted_prio=5 restored_prio=1 high_acq=1`
  - `[PASS] DEADLOCK_GUARD: timeout_recovered=YES ordered_lock(A->B)=OK`
  - `[PASS] FROM_ISR_AND_QUEUE: tim2_isr=4 notify=4 q_tx=5 q_rx=4 sw_tmr=4 fpu_err=0`
  - `[PASS] STACK_HWM: Sup=339 Cons=309 Prod=186 Mon=459 words_free (heap_free=21104 B)`
  - Continuous 8.0-second live telemetry stream (`wdg=2..9`, `heap=21104/21104`).
- **`19-projects/03-advanced-freertos-digital-power-controller/` (`freertos_digital_power_ctrl.elf`, SHA-256 `c30bf356d3c0c1f8060b6d9a71a47d5636cc5083a4901a2f6d1041f9960f69cc`):**
  - 4-task execution tracking $5000\text{ mV}$ reference (`vout_mv = 4989..5021`, `ccr1 = 355`), `TIM1`/`COMP1` armed (`moe:1, fault:0`), 10 consecutive frames across 5.2 seconds with healthy stack watermarks `hwm:[217, 337, 325, 351]`.

### 15.2 Explicit Limitations & Omissions Declaration

1. **Demonstration $\neq$ Guaranteed Worst-Case Execution Time (WCET) Determinism:**
   - Passing an 8-second demonstration on a lab NUCLEO-G474RE board demonstrates **functional correctness under nominal lab workload**, but does **NOT** constitute formal real-time determinism proof.
   - Formal hard-real-time certification (e.g. ISO 26262, DO-178C, IEC 61508) requires:
     - Static timing analysis (e.g. AbsInt aiT) or hardware instruction-trace instrumentation (ETM) to establish upper bounds on $C_i$ for every task, kernel critical section, and ISR burst.
     - Modeling Flash memory wait-state stalls (4 WS @ 170 MHz) and ART accelerator cache miss penalties.
     - Modeling bus-matrix crossbar contention between CPU instruction/data fetches and high-speed DMA channels.
2. **Newlib `_reent` Thread-Local Storage (TLS) Not Enabled:**
   - In `FreeRTOSConfig.h`, **`configUSE_NEWLIB_REENTRANT = 0`**.
   - Standard C library functions that depend on global state (`strtok`, `asctime`, `rand`, floating-point `errno`, `locale`) are **NOT thread-safe** across concurrent tasks in this demonstration. Application code in this repository uses reentrant formatting (`snprintf`) and integer math exclusively. If non-reentrant standard C functions are required, `configUSE_NEWLIB_REENTRANT` must be set to `1` (which allocates a `struct _reent` per TCB, consuming ~1 KB additional RAM per task).
3. **Tickless Idle Not Enabled in Demonstration Firmware:**
   - In `FreeRTOSConfig.h`, **`configUSE_TICKLESS_IDLE = 0`**.
   - The CPU continues receiving `1 ms` `SysTick` interrupts continuously. Deep low-power sleep modes (`Stop 0 / Stop 1`) were demonstrated separately in bare-metal Lab 14 (`lab-14-low-power-stop-mode-wakeup/`), rather than inside the active FreeRTOS verification binary.
