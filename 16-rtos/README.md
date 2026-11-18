# Stage 15: Real-Time Operating Systems (FreeRTOS on ARM Cortex-M4F) & Concurrency

> **Sequential Navigation (Step 16 of 17 — Canonical Acyclic Order):**  
> **Home:** [`START_HERE.md`](../START_HERE.md) | **Roadmap:** [`learning-sequence.md`](../01-master-concept-map/learning-sequence.md) | **Prev:** [Stage 15 (13) — Watchdogs, RTC & Low-Power Modes (`15-power-management/`)](../15-power-management/README.md) | **Next:** [Stage 17 (15) — Advanced Firmware Engineering, MISRA & CI (`17-advanced-firmware-engineering/`)](../17-advanced-firmware-engineering/README.md)  
> **Prerequisites:** [`03-embedded-c/` (`EMB-ATOM-*`)](../03-embedded-c/README.md), [`04-arm-cortex-m/` (`ARM-CORE-*`)](../04-arm-cortex-m/README.md), [`10-interrupts/` (`RT-IRQ-*`)](../10-interrupts/README.md), [`15-power-management/`](../15-power-management/README.md)

> **Directory:** `16-rtos/` | **Target Platform:** STM32G474RET6 (ARM Cortex-M4F `ARM_CM4F` Port, `__NVIC_PRIO_BITS = 4`, 128 KB SRAM)  
> **Code Verification Notice:** The companion standalone FreeRTOS `V11.1.0` firmware project in [`23-code-library/freertos/`](../23-code-library/freertos/README.md) is **Compile-Verified (`arm-none-eabi-gcc 14.3.1`, 0 errors / 0 warnings on user code)** and **Target Execution Verified on physical NUCLEO-G474RE (`6/6 ALL_PASSED`, 8.0 s live UART capture; note that an 8 s functional demo does NOT prove formal WCET determinism — see [`AUDIT/FREERTOS_REVIEW.md`](../AUDIT/FREERTOS_REVIEW.md) & [`AUDIT/FREERTOS_VERIFICATION_REPORT.md`](../AUDIT/FREERTOS_VERIFICATION_REPORT.md))**. Inline illustrative snippets in this theory guide are **Statically Reviewed against `stm32g474xx.h`, `core_cm4.h`, and FreeRTOS `V11.1.0` `ARM_CM4F` Port Specifications**.

---

## 1. Learning Objectives & Concept Scope

| Concept ID | Concept Name | Architectural Scope | Mastery Tier |
|---|---|---|---|
| `RTOS-ARCH-001` / `RTOS-SCHED-001` | Superloop vs Cooperative vs Preemptive RTOS (**When NOT to Use an RTOS**), Time-Slicing & Tick Rate Trade-Offs | `[General Embedded]` / `[RTOS Generic]` | Level 1–2 (Essential) |
| `RTOS-CTX-001` / `RTOS-PORT-001` | Cortex-M4F **`PendSV`/`SysTick`/`SVC`** Context Switching (PSP vs MSP, `R4–R11` + **FPU `S16–S31`** Lazy Stacking) & **`NVIC_SetPriorityGrouping(0)`** | `[ARM Cortex-M4F]` / `[STM32G474RE Exact]` | Level 2 (Production) |
| `RTOS-TASK-001` / `002` | TCB, Task States (`Running/Ready/Blocked/Suspended/Deleted`), Priorities, `vTaskDelayUntil`, Idle Task & Idle Hook | `[RTOS Generic]` | Level 1–2 (Essential) |
| `RTOS-IPC-001` / `002` | Queues (Pass-by-Value vs Zero-Copy Pointer Pool) & **Stream / Message Buffers** | `[RTOS Generic]` | Level 2–3 (Production) |
| `RTOS-SYNC-001` / `002` / `003` / `004` | Binary & Counting Semaphores, **Mutexes & Priority Inheritance (Distinct from Deadlocks!)**, Event Groups (24-bit), & **Direct-to-Task Notifications** | `[RTOS Generic]` | Level 2–3 (Production) |
| `RTOS-ISR-001` / `002` | `FromISR` APIs (`pxHigherPriorityTaskWoken`, `portYIELD_FROM_ISR`), **`configMAX_SYSCALL_INTERRUPT_PRIORITY`**, & Unmasked High-Priority `BASEPRI` ISRs | `[ARM Cortex-M4F]` / `[STM32G474RE Exact]` | Level 2 (Production) |
| `RTOS-HAZ-001` / `002` | **Deadlocks (4 Coffman Conditions, Lock Ordering)** & Livelocks vs **Priority Inversion (PIP & PCP)** | `[RTOS Theory]` | Level 2 (Production) |
| `RTOS-MEM-001` / `002` | **`xTaskCreateStatic`**, `heap_1..5` (`heap_5` across SRAM1/2 + CCM), **`uxTaskGetStackHighWaterMark`**, & **`configCHECK_FOR_STACK_OVERFLOW = 2`** | `[RTOS Generic]` / `[STM32G474RE Exact]` | Level 2 (Production) |
| `RTOS-PWR-001` / `RTOS-THEORY-001` | **`LPTIM1` Tickless Idle (`Stop 0/1`)** & Real-Time Scheduling Theory (**RMS**, **EDF**, Liu-Layland $U \le n(2^{1/n}-1)$, Exact RTA) | `[STM32G474RE Exact]` / `[RTOS Theory]` | Level 3 (Specialist) |

---

## 2. Prerequisites & Curriculum Positioning

### Prerequisite Concept IDs (Corrected — Zero Numbering Drift!)
- **Stage 03 (`04-arm-cortex-m/`):** `ARM-EXC-001`, `ARM-FPU-001` (MSP vs PSP, `CONTROL.SPSEL`, `EXC_RETURN`, `FPU->FPCCR` lazy stacking, `SVC`/`PendSV`).
- **Stage 08 (`10-interrupts/`):** `RT-IRQ-001`, `RT-IRQ-002`, `RT-TIME-002` (`__NVIC_PRIO_BITS = 4`, `PRIGROUP`, `BASEPRI` selective masking, `SysTick`).
- **Stage 09 (`09-timers/`):** `PERIPH-SYSTICK-001`, `PERIPH-LPTIM-001` (`LPTIM1` for Stop-mode tickless idle).
- **Stage 14 (`15-power-management/`):** `REL-WDG-001`, `REL-PWR-001` (Watchdog supervision and Stop 0/1 wakeup).

---

## 3. Core Technical Theory & Engineering Principles

### 3.1 Superloop vs Cooperative vs Preemptive RTOS — And When NOT to Use an RTOS
| Architecture | Determinism & Latency | RAM / Flash Overhead | Best Fit |
|---|---|---|---|
| **Bare-Metal Superloop + ISRs** | Ultra-low ISR latency (`< 100 ns`), poor main-loop worst-case latency if any module blocks | Minimal (0 B kernel RAM) | Tiny MCUs, ultra-fast `< 10 µs` digital power loops, simple sensor nodes |
| **Cooperative (Run-to-Completion / Time-Triggered)** | Zero preemption race conditions between tasks; latency bounded by longest task slice | Minimal (~200 B) | Safety-certifiable state machines, deterministic motor/control state scheduling |
| **Preemptive Priority RTOS (FreeRTOS)** | Highest-priority Ready task runs within ~1–5 µs regardless of low-priority CPU work | ~4–10 KB Flash, **per-task private stacks** (512 B–4 KB each!) | Complex concurrent systems (CAN/USB/BLE stacks + UI + telemetry + periodic control) |

**When NOT to Use an RTOS:** (1) SRAM is severely constrained (each task needs its own worst-case stack + FPU/ISR margin); (2) hard real-time loop runs at $> 50\text{ kHz}$ (put the loop in a DMA/Timer/HRTIM ISR or `FMAC` above `configMAX_SYSCALL_INTERRUPT_PRIORITY` — NEVER in an RTOS task!); (3) safety certification cost favors a deterministic bare-metal FSM.

### 3.2 ARM Cortex-M4F Context Switching (`SVC`, `SysTick`, `PendSV` & FPU `S16–S31`) `[ARM Cortex-M4F]`
FreeRTOS `portable/GCC/ARM_CM4F/port.c` maps three Cortex-M4 core exceptions:
1. **`SVC_Handler` (`vPortSVCHandler`):** Executes **once** inside `vTaskStartScheduler()` to pop the first task's context (`R4–R11`, `EXC_RETURN`), load its `PSP`, and branch into Thread Mode (`CONTROL.SPSEL = 1`). All ISRs and kernel handlers continue using **`MSP`**.
2. **`SysTick_Handler` (`xPortSysTickHandler`, Priority 15 — Lowest!):** Increments `xTickCount`, moves expired Delayed tasks to Ready lists, and **pends `PendSV`** (`SCB->ICSR = SCB_ICSR_PENDSVSET_Msk`) if a higher-priority or time-sliced peer task must run.
3. **`PendSV_Handler` (`xPortPendSVHandler`, Priority 15 — Lowest!):** Runs only *after* all higher-priority peripheral ISRs complete!
   - Hardware already stacked `{R0–R3, R12, LR, PC, xPSR}` (plus 18-word FPU frame `{S0–S15, FPSCR}` if task used FPU, signaled by `EXC_RETURN` bit 4 == `0`: **`0xFFFFFFED`** vs non-FPU **`0xFFFFFFFD`**).
   - `PendSV` checks **`tst r14, #0x10`**: if FPU is active, executes `vstmdbeq r0!, {s16-s31}` to save callee-saved FPU registers!
   - Saves `R4–R11` + `LR` (`EXC_RETURN`) onto outgoing task's `PSP`, stores `PSP` into `pxCurrentTCB->pxTopOfStack`, calls `vTaskSwitchContext()`, loads incoming task's `PSP`, restores `R4–R11` + `LR` + conditional `vldmiaeq {s16-s31}`, and executes `bx r14`!

### 3.3 Task Lifecycle, Priorities, `vTaskDelayUntil` & The Idle Task
- **5 Task States:** `Running`, `Ready`, `Blocked` (waiting on time/queue/semaphore/notification — consumes **0% CPU**!), `Suspended` (`vTaskSuspend`), `Deleted` (`vTaskDelete`).
- **Drift-Free Periodicity:** Use **`vTaskDelayUntil(&last_wake, pdMS_TO_TICKS(10))`** (absolute wake tick) instead of relative `vTaskDelay()` (which drifts by execution time + preemption!).
- **Idle Task (`tskIDLE_PRIORITY = 0`):** Created automatically by the scheduler. **MUST NEVER BE STARVED** (do not place busy loops at priority $\ge 1$ or non-yielding UI tasks at priority `0`!), because Idle frees memory of deleted tasks (`prvCheckTasksWaitingTermination`) and executes `vApplicationIdleHook()` (which **MUST NEVER call any blocking API**!).

### 3.4 Inter-Task Communication & Synchronization Primitives
| Primitive | Mechanism & Best Use Case | ISR Safe? | Key Engineering Rule |
|---|---|---|---|
| **Queue** | Fixed-size FIFO; pass-by-value for small structs (`<= 16 B`), or **zero-copy 2-queue pointer pool** for 256+ B DMA buffers | `xQueueSendFromISR` | Never copy 512 B DMA frames by value inside an ISR! |
| **Stream / Message Buffer** | Lock-free SPSC byte stream / variable-length framed buffer (`xTriggerLevelBytes`) | `xMessageBufferSendFromISR` | Ideal for `USART2` DMA+`IDLE` variable-length packets; single writer / single reader only! |
| **Binary / Counting Semaphore** | Event signaling (max 1 — drops bursts!) vs counting buffered events / resource pool slots (`uxMaxCount`) | `xSemaphoreGiveFromISR` | `xSemaphoreCreateBinary()` starts **empty** (`0`)! |
| **Direct-to-Task Notification** | 32-bit value/flag embedded directly inside target TCB (0 extra RAM, **~45% faster** than binary semaphore!) | `vTaskNotifyGiveFromISR` | Preferred 1-to-1 ISR-to-task wakeup (`ulTaskNotifyTake(pdTRUE, ...)`) |
| **Event Group** | 24 independent event bits (`configUSE_16_BIT_TICKS = 0`); Wait-ANY, Wait-ALL, & multi-task rendezvous (`xEventGroupSync`) | `xEventGroupSetBitsFromISR` (deferred via Timer task) | Ideal for **Watchdog Supervisor** checking in all monitored tasks! |
| **Mutex / Recursive Mutex** | Binary lock with **strict task ownership** + **Priority Inheritance** | **NEVER IN ISRs!** | Only the taking task can give it; ISRs have no TCB/priority to inherit and cannot block! |

### 3.5 Priority Inversion vs Deadlocks (CRITICAL CONCEPTUAL DISTINCTION!) `[RTOS Theory]`
Do **NOT** conflate Priority Inversion with Deadlocks!
1. **Priority Inversion (`RTOS-HAZ-002`):** Low-priority task **L** holds a lock; High-priority task **H** blocks waiting for it; Medium-priority task **M** preempts **L** indefinitely, causing **unbounded blocking / deadline miss** for **H**.
   - **Fix:** **Priority Inheritance Protocol (PIP)** inside FreeRTOS **Mutexes** temporarily elevates **L** to **H**'s priority so **M** cannot preempt **L** (or **Priority Ceiling Protocol [PCP]**). *Binary semaphores do NOT inherit priority!*
2. **Deadlock (`RTOS-HAZ-001`):** Permanent circular wait where **zero** tasks can make forward progress! Requires all **4 Coffman Conditions**: *(1) Mutual Exclusion, (2) Hold-and-Wait, (3) No Preemption, (4) Circular Wait* (e.g., Task 1 locks Mutex A then waits for B; Task 2 locks Mutex B then waits for A).
   - **CRITICAL TRUTH:** **Priority Inheritance does NOT prevent AB/BA deadlocks!** Break Circular Wait via **strict global lock ordering** (always lock A before B) and **bounded timeouts** (`xTicksToWait`).
3. **Livelock:** Tasks actively retry/yield without blocking yet make zero forward progress.

### 3.6 `FromISR` APIs, `BASEPRI` Partitioning & Mandatory `NVIC_SetPriorityGrouping(0)` `[STM32G474RE Exact]`
1. **Mandatory STM32 NVIC Grouping Call:** Before `vTaskStartScheduler()`, you **MUST** call **`NVIC_SetPriorityGrouping(0U)`** (or `3U` — both allocate **all 4 STM32G4 priority bits to Preemption Priority and 0 bits to Subpriority**!). Subpriority bits break FreeRTOS `BASEPRI` masking assumptions and trip `configASSERT`!
2. **`configMAX_SYSCALL_INTERRUPT_PRIORITY` (e.g., `5 << 4 = 0x50`):**
   - **Priorities `0..4` (Numerically `< 5`):** **Unmanaged Unmasked High-Priority ISRs** (e.g., `HRTIM1`, `TIM1` motor FOC, `COMP1` fault). NEVER masked by `taskENTER_CRITICAL()` (which writes `0x50` to `BASEPRI`), and **MUST NEVER call ANY FreeRTOS API**! *(Terminology note: `BASEPRI` eliminates RTOS critical-section masking delay for priorities `0..4`, **not** hardware interrupt latency — these ISRs still incur 12+ cycles of Cortex-M4 stacking, Flash/bus contention, or preemption by higher-priority exceptions; see [`10-interrupts/` §3.2](../10-interrupts/README.md#32-interrupt-masking-primask-vs-basepri-vs-faultmask-arm-cortex-m4f).)*
   - **Priorities `5..15` (Numerically `>= 5`):** **Kernel-Managed ISRs** (e.g., `USART2`, `ADC1_DMA`, `EXTI13`). Safe to call `...FromISR()` APIs; masked briefly during FreeRTOS critical sections.
   - **NEVER set `configMAX_SYSCALL_INTERRUPT_PRIORITY` to `0`** (writing `0` to `BASEPRI` disables masking completely!).
3. **Immediate Deferred Preemption:** Always pass `&xHigherPriorityTaskWoken` into `...FromISR()` and end the ISR with **`portYIELD_FROM_ISR(xHigherPriorityTaskWoken);`** (which pends `PendSV` so the woken high-priority task executes immediately upon ISR exit instead of waiting up to 1 ms for SysTick!).

### 3.7 Memory Management (`heap_1..5`), Static Allocation & Stack Overflow Protection
- **`StackType_t` Units Warning:** On Cortex-M4, `usStackDepth` in `xTaskCreate` is measured in **4-byte words**, NOT bytes (`128` words = `512` bytes — guaranteed overflow if calling newlib `printf` + FPU frame, which requires **$\ge 512$ words = 2048 bytes**!).
- **Stack Protection:** Enable **`configCHECK_FOR_STACK_OVERFLOW = 2`** (checks SP bounds AND scans the last 16 bytes for intact **`0xA5A5A5A5`** canary pattern on every context switch!) and poll **`uxTaskGetStackHighWaterMark(handle)`** in test builds ($\ge 30\%$ safety margin).
- **Zero-`malloc` Safety-Critical Allocation (`configSUPPORT_STATIC_ALLOCATION = 1`):** Allocate all TCBs (`StaticTask_t`), stacks (`StackType_t[]`), and queues (`StaticQueue_t`) statically at link time via **`xTaskCreateStatic()`** — 100% deterministic memory footprint!
- **`heap_1`..`heap_5` Comparison:**
  - `heap_1`: Allocate only, never free. `heap_2`: Best-fit, fragments (legacy). `heap_3`: Thread-safe libc `malloc` wrapper. **`heap_4`**: First-fit + adjacent block coalescing (default dynamic). **`heap_5`**: Same coalescing as `heap_4` across **non-contiguous SRAM regions** — spans STM32G474RE's 96 KB `SRAM1+SRAM2` (`0x20000000`) + 32 KB `CCM-SRAM` (`0x10000000`, remembering DMA cannot target `0x10000000`!).

### 3.8 `LPTIM1` Tickless Idle (`Stop 0/1`) & Real-Time Scheduling Theory
- **Tickless Idle:** Built-in `configUSE_TICKLESS_IDLE = 1` uses 24-bit `SysTick` in Sleep mode (max single sleep $\frac{2^{24}}{170\text{ MHz}} = \mathbf{98.69\text{ ms}}$!). For multi-second microamp Stop 0/1 sleep on STM32G474RE, implement custom **`configUSE_TICKLESS_IDLE = 2`** (`vPortSuppressTicksAndSleep`) driven by **`LPTIM1` (`0x40007C00`)** clocked from 32.768 kHz `LSE` / 32 kHz `LSI` and advance ticks via `vTaskStepTick()`.

---

## 4. Core Terminology & Acronyms

| Term | Engineering Definition |
|---|---|
| **TCB (`StaticTask_t`)** | Task Control Block storing `pxTopOfStack`, priority, state list item, and direct notification array. |
| **PSP / MSP** | Process Stack Pointer (per-task Thread Mode) vs Main Stack Pointer (ISRs & Handler Mode). |
| **Priority Inheritance (PIP)** | Temporarily raising a mutex holder's priority to match the highest blocked waiter (bounds inversion, NOT deadlocks!). |
| **`pxHigherPriorityTaskWoken`** | Output flag from `FromISR` APIs passed to `portYIELD_FROM_ISR()` to trigger immediate `PendSV` switch. |
| **High-Water Mark** | Minimum free stack words ever observed for a task (`uxTaskGetStackHighWaterMark()`). |
| **RMS / EDF / RTA** | Rate Monotonic Scheduling, Earliest Deadline First, and exact Response-Time Analysis. |

---

## 5. Hardware Register & `FreeRTOSConfig.h` Reference

Verified against `stm32g474xx.h` (`__NVIC_PRIO_BITS = 4U`) and FreeRTOS `ARM_CM4F`:

| Setting / Register | Exact STM32G474RE Value | Rationale |
|---|---|---|
| `NVIC_SetPriorityGrouping(0U)` | `SCB->AIRCR.PRIGROUP = 0` (4 pre / 0 sub) | Mandatory before `vTaskStartScheduler()` |
| `configPRIO_BITS` | `4` (`__NVIC_PRIO_BITS`) | STM32G474RE implements upper 4 bits `[7:4]` |
| `configKERNEL_INTERRUPT_PRIORITY` | `(15U << 4) = 0xF0` | Lowest NVIC priority for `SysTick` & `PendSV` |
| `configMAX_SYSCALL_INTERRUPT_PRIORITY` | `(5U << 4) = 0x50` | Priorities `0..4` unmasked high-priority unmanaged (unaffected by `BASEPRI`); `5..15` `FromISR`-safe |
| `configCHECK_FOR_STACK_OVERFLOW` | `2` | Method 2 `0xA5A5A5A5` watermark canary + SP check |
| `configSUPPORT_STATIC_ALLOCATION` | `1` | Enables `xTaskCreateStatic()` zero-heap production firmware |
| Vector Mapping (`#define`) | `vPortSVCHandler -> SVC_Handler`, `xPortPendSVHandler -> PendSV_Handler`, `xPortSysTickHandler -> SysTick_Handler` | Hooks FreeRTOS kernel directly into bare-metal startup table |

---

## 6. Engineering Mathematics & Worked Formulas

### 6.1 Rate Monotonic Scheduling (RMS — Liu & Layland, 1973) & EDF Bounds
For $n$ independent periodic tasks with execution time $C_i$ and period $T_i$ (prioritized strictly by shorter period = higher priority):

$$
U = \sum_{i=1}^{n} \frac{C_i}{T_i} \le n\left(2^{1/n} - 1\right) \xrightarrow{n \to \infty} \ln 2 \approx 0.693
$$

- For $n = 3$: $U_{\text{RMS}} = 3(2^{1/3} - 1) = \mathbf{0.7798}$ (77.98%).
- For $n = 4$: $U_{\text{RMS}} = 4(2^{1/4} - 1) = \mathbf{0.7568}$ (75.68%).
- For **Earliest Deadline First (EDF)** dynamic priority: $U \le \mathbf{1.000}$ (100% schedulable if deadlines equal periods!).

### 6.2 Exact Response-Time Analysis (RTA — Joseph & Pandya, 1986)
Even when $U > 0.693$ **or when priorities are not strictly Rate-Monotonic** (for example, placing a slower safety supervisor above a faster control loop), a fixed-priority task $i$ meets its relative deadline $D_i$ if and only if the iterative fixed-point recurrence converges to $R_i \le D_i$:

$$
R_i^{(k+1)} = C_i + B_i + \sum_{j \in \text{hp}(i)} \left\lceil \frac{R_i^{(k)}}{T_j} \right\rceil C_j, \qquad R_i^{(0)} = C_i + B_i
$$

where $C_i$ is the task's worst-case execution time (WCET), $B_i$ is the worst-case lower-priority mutex blocking time under Priority Inheritance / Priority Ceiling, and $\text{hp}(i)$ is the set of all tasks with strictly higher priority than task $i$.

### 6.3 Worked Fixed-Priority RTA Exercise: STM32G474RE 4-Task Power Controller (Project 03 / Lab 15 Pattern)

Consider the 4-task FreeRTOS architecture implemented in [`19-projects/03-advanced-freertos-digital-power-controller/Src/main.c`](../19-projects/03-advanced-freertos-digital-power-controller/Src/main.c) (`170 MHz` Boost, `configTICK_RATE_HZ = 1000`, blocking `USART2` at `115200 8N1` guarded by Priority-Inheritance mutex `g_uart_mutex`). Notice that priorities are assigned by **safety criticality** rather than pure Rate Monotonic order (`SafetySupervisor` has $T_1 = 100\text{ ms}$ at Priority 4 above `ControlLoop` $T_2 = 1.0\text{ ms}$ at Priority 3), so Liu-Layland's RMS theorem does **not** directly guarantee schedulability — **exact RTA is mandatory**:

| Task | FreeRTOS Priority | Period $T_i$ (ms) | Deadline $D_i$ (ms) | Illustrative WCET Budget $C_i$ (ms) | Blocking $B_i$ (ms) | Role & Shared Mutex Scope |
|---|---:|---:|---:|---:|---:|---|
| $\tau_1$: `SafetySupervisor` | `4` *(Highest)* | `100.0` | `100.0` | `0.05` | `0.00` *(steady state)* | Event-group heartbeat check, `IWDG` kick, `LD2` toggle |
| $\tau_2$: `ControlLoop` | `3` | `1.0` | `1.0` | `0.10` | `0.00` | `1 kHz` Q15 PI regulator + `TIM1->CCR1` update (never takes `g_uart_mutex`) |
| $\tau_3$: `Telemetry` | `2` | `500.0` | `500.0` | `15.00` | `5.00` | `snprintf` + ~160 B blocking `115200` JSON TX (`~13.9 ms`); blocked by `UiTask` holding `g_uart_mutex` |
| $\tau_4$: `UiTask` | `1` *(Lowest App)* | `100.0` | `100.0` | `5.00` | `0.00` | `10 Hz` `B1` poll + optional OLED / ~55 B log string (`~4.8 ms`) |

**Step 1 — Total Processor Utilization:**

$$
U = \frac{0.05}{100.0} + \frac{0.10}{1.0} + \frac{15.00}{500.0} + \frac{5.00}{100.0} = 0.0005 + 0.1000 + 0.0300 + 0.0500 = 0.1805 \quad (18.05\%)
$$

**Step 2 — Iterative Response-Time Recurrence (Steady-State Operation):**

1. **$\tau_1$ (`SafetySupervisor`, Prio 4, $\text{hp}(1) = \emptyset$):**
   - $R_1^{(0)} = C_1 + B_1 = 0.05\text{ ms} \implies \mathbf{R_1 = 0.05\text{ ms} \le 100.0\text{ ms}}$ (`PASS`, margin `99.95 ms`).
2. **$\tau_2$ (`ControlLoop`, Prio 3, $\text{hp}(2) = \{\tau_1\}$):**
   - $R_2^{(0)} = 0.10\text{ ms}$
   - $R_2^{(1)} = 0.10 + \lceil 0.10 / 100.0 \rceil (0.05) = 0.10 + 0.05 = 0.15\text{ ms}$
   - $R_2^{(2)} = 0.10 + \lceil 0.15 / 100.0 \rceil (0.05) = 0.15\text{ ms}$ (converged) $\implies \mathbf{R_2 = 0.15\text{ ms} \le 1.0\text{ ms}}$ (`PASS`, margin `0.85 ms`).
3. **$\tau_3$ (`Telemetry`, Prio 2, $\text{hp}(3) = \{\tau_1, \tau_2\}$):**
   - $R_3^{(0)} = C_3 + B_3 = 15.00 + 5.00 = 20.00\text{ ms}$
   - $R_3^{(1)} = 20.00 + \lceil 20.00 / 100.0 \rceil (0.05) + \lceil 20.00 / 1.0 \rceil (0.10) = 20.00 + 0.05 + 2.00 = 22.05\text{ ms}$
   - $R_3^{(2)} = 20.00 + \lceil 22.05 / 100.0 \rceil (0.05) + \lceil 22.05 / 1.0 \rceil (0.10) = 20.00 + 0.05 + 2.30 = 22.35\text{ ms}$
   - $R_3^{(3)} = 20.00 + \lceil 22.35 / 100.0 \rceil (0.05) + \lceil 22.35 / 1.0 \rceil (0.10) = 22.35\text{ ms}$ (converged) $\implies \mathbf{R_3 = 22.35\text{ ms} \le 500.0\text{ ms}}$ (`PASS`).
4. **$\tau_4$ (`UiTask`, Prio 1, $\text{hp}(4) = \{\tau_1, \tau_2, \tau_3\}$):**
   - $R_4^{(0)} = 5.00\text{ ms}$
   - $R_4^{(1)} = 5.00 + \lceil 5.00 / 100.0 \rceil (0.05) + \lceil 5.00 / 1.0 \rceil (0.10) + \lceil 5.00 / 500.0 \rceil (15.00) = 20.55\text{ ms}$
   - $R_4^{(2)} = 5.00 + \lceil 20.55 / 100.0 \rceil (0.05) + \lceil 20.55 / 1.0 \rceil (0.10) + \lceil 20.55 / 500.0 \rceil (15.00) = 22.15\text{ ms}$
   - $R_4^{(3)} = 5.00 + \lceil 22.15 / 100.0 \rceil (0.05) + \lceil 22.15 / 1.0 \rceil (0.10) + \lceil 22.15 / 500.0 \rceil (15.00) = 22.35\text{ ms}$
   - $R_4^{(4)} = 22.35\text{ ms}$ (converged) $\implies \mathbf{R_4 = 22.35\text{ ms} \le 100.0\text{ ms}}$ (`PASS`).

**Step 3 — Transitive Priority-Inheritance Hazard Uncovered by RTA (Production Lesson!):**
If `SafetySupervisor` (Prio 4) ever calls `pwr_uart_log()` (`xSemaphoreTake(g_uart_mutex)`) during a fault/reset event while `Telemetry` (Prio 2) is mid-transmission holding `g_uart_mutex` for `15.0 ms`, FreeRTOS Priority Inheritance promotes `Telemetry` to **Priority 4** — preempting `ControlLoop` (Prio 3, $D_2 = 1.0\text{ ms}$) for up to `15.0 ms` ($R_2 > 15.1\text{ ms} \gg D_2 = 1.0\text{ ms}$ $\implies$ **15 consecutive 1 ms control-loop deadline misses**!). In Project 03, hardware overcurrent protection stays safe in `< 50 ns` because `COMP1 -> TIM1_BKIN` operates purely in silicon, **but** in safety-certified production systems you MUST decouple Prio 4 logging into a non-blocking queue/DMA TX ring buffer so the highest-priority task **never** contends on a mutex held across slow blocking UART I/O.

> [!WARNING]
> **Explicit Real-Time Determinism & WCET Caveat:** Passing an 8-second onboard FreeRTOS demonstration (`23-code-library/freertos/` `6/6 ALL_PASSED` or Project 03 `5.2 s` live telemetry on NUCLEO-G474RE) proves **functional correctness under nominal lab load**, and does **NOT** constitute a formal Worst-Case Execution Time (WCET) or hard-real-time determinism proof! True hard-real-time certification additionally requires: (1) static WCET bounding or exhaustive `DWT->CYCCNT` / ETM trace worst-case profiling of every task, kernel critical section (`vTaskSwitchContext`, `SysTick`, `PendSV`), and unmanaged/managed ISR burst; (2) explicit accounting for Flash wait-state / ART cache misses and DMA bus contention; and (3) verification of blocking terms $B_i$ across all mutexes and interrupt bursts (see [`AUDIT/FREERTOS_REVIEW.md`](../AUDIT/FREERTOS_REVIEW.md)).

---

## 7. Official Documentation & Standards References

| Document | Exact Sections |
|---|---|
| **FreeRTOS Kernel Documentation** | *Mastering the FreeRTOS Real Time Kernel* (Ch. 1–8), *Running FreeRTOS on ARM Cortex-M* (`BASEPRI`, `PRIGROUP`), *Tickless Idle* |
| **ST PM0214 / ARM AN298** | PM0214 §2.3 & §4.6 (`PendSV`, `SVC`, `PSP`, `FPU->FPCCR`); ARM AN298 (*Cortex-M4F Lazy Stacking*) |
| **Liu & Layland (1973) / Buttazzo** | *Scheduling Algorithms for Multiprogramming in a Hard-Real-Time Environment*; *Hard Real-Time Computing Systems* |

---

## 8. Production-Grade Bare-Metal / FreeRTOS C Code Examples

> **Verification Status:** Statically Reviewed against FreeRTOS `ARM_CM4F` port API & `stm32g474xx.h` (Not Tested on Physical Hardware).

### 8.1 Static Task Creation (`xTaskCreateStatic`) + Zero-RAM Direct Notification From ISR
```c
#include "stm32g474xx.h"
#include "FreeRTOS.h"
#include "task.h"

#define SENSOR_TASK_STACK_WORDS 512U /* 2048 bytes (safe for FPU + formatting!) */

static StaticTask_t g_sensor_tcb;
static StackType_t  g_sensor_stack[SENSOR_TASK_STACK_WORDS];
static TaskHandle_t g_sensor_handle = NULL;

void vSensorTask(void *pvParameters)
{
    (void)pvParameters;
    for (;;) {
        /* Block indefinitely waiting for direct-to-task notification from DMA/EXTI ISR */
        uint32_t events = ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        if (events > 0U) {
            /* Process fresh hardware buffer outside ISR context */
            GPIOA->BSRR = ((GPIOA->ODR & GPIO_ODR_OD5) != 0U) ? GPIO_BSRR_BR5 : GPIO_BSRR_BS5;
        }
    }
}

void EXTI15_10_IRQHandler(void)
{
    if ((EXTI->PR1 & EXTI_PR1_PIF13) != 0U) {
        EXTI->PR1 = EXTI_PR1_PIF13; /* Direct rc_w1 clear (NEVER '|='!) */
        (void)EXTI->PR1;            /* APB2 posted-write commit readback (ARM AN321 §4.9) */
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        if (g_sensor_handle != NULL) {
            vTaskNotifyGiveFromISR(g_sensor_handle, &xHigherPriorityTaskWoken);
        }
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken); /* Immediate PendSV switch! */
        __DSB();                    /* Write-buffer drain & ARM Erratum 838869 (ES0430 §2.1.3) */
    }
}

void RTOS_App_Start(void)
{
    /* MANDATORY on STM32G474RE: 4 preemption bits, 0 subpriority bits! */
    NVIC_SetPriorityGrouping(0U);

    /* Priority 6 >= configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY (5) -> FromISR-safe! */
    NVIC_SetPriority(EXTI15_10_IRQn, 6U);
    NVIC_EnableIRQ(EXTI15_10_IRQn);

    g_sensor_handle = xTaskCreateStatic(
        vSensorTask, "Sensor", SENSOR_TASK_STACK_WORDS, NULL,
        tskIDLE_PRIORITY + 3U, g_sensor_stack, &g_sensor_tcb);

    configASSERT(g_sensor_handle != NULL);
    vTaskStartScheduler();
}

void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
    (void)xTask; (void)pcTaskName;
    __disable_irq();
    __BKPT(0);
    for (;;) {}
}
```

---

## 9. Practical Laboratory Experiments & Debugging Checkpoints

- **Runnable FreeRTOS V11.1.0 Firmware Project (Compile-Verified):** [`23-code-library/freertos/`](../23-code-library/freertos/README.md) — Official version-pinned **FreeRTOS-Kernel `V11.1.0`** (`GCC/ARM_CM4F` + `heap_4.c`) cross-compiled for NUCLEO-G474RE (`make clean all` -> `build/nucleo_g474re_freertos.elf`). Exercises `NVIC_SetPriorityGrouping(0U)`, 4-task preemptive scheduling + static Idle/Timer allocation, Queue IPC, `TIM2_IRQHandler` `vTaskNotifyGiveFromISR()`, Binary/Counting Semaphores, automated Mutex Priority Inheritance (`base_prio=1 -> promoted_prio=5 -> restored_prio=1`), bounded-timeout Deadlock Guard, Stack High-Water Mark monitoring, and Event-Group `IWDG` supervision with zero external wiring. See [`AUDIT/FREERTOS_VERIFICATION_REPORT.md`](../AUDIT/FREERTOS_VERIFICATION_REPORT.md).
- **Lab 15:** [`lab-15-freertos-multitask-producer-consumer`](../18-practical-laboratory/lab-15-freertos-multitask-producer-consumer/README.md) — Producer-consumer queue with proper $\ge 512$-word telemetry stack, `NVIC_SetPriorityGrouping(0)`, `vTaskDelayUntil`, and step-by-step build/flash guide referencing `23-code-library/freertos/`.
- **GDB Verification Checkpoint (Cortex-M4F Context Switch):**
  ```text
  (gdb) p/x $control      # Inside task: verify bit 1 (SPSEL) == 1 (using PSP!)
  (gdb) p/x $psp          # Inspect active task PSP vs $msp (ISR stack)
  (gdb) p/x *(uint32_t*)0xE000ED20  # Verify SHPR3: PendSV (b23:16) == 0xF0, SysTick (b31:24) == 0xF0
  (gdb) p uxTaskGetStackHighWaterMark(g_sensor_handle)
  ```

---

## 10. Common Engineering Mistakes, Hardware Traps & Silicon Errata (ES0430)

1. **Allocating `128` Words to a Task Calling `printf()`:** On Cortex-M4F (newlib + 104-byte FPU frame), `512` bytes overflows immediately and corrupts adjacent RAM! Always allocate $\ge 512$ words (2 KB) or use a lightweight integer formatter + `configCHECK_FOR_STACK_OVERFLOW = 2`.
2. **Forgetting `NVIC_SetPriorityGrouping(0U)` on STM32:** Leaves subpriority bits enabled and fails `vPortValidateInterruptPriority()`.
3. **Setting a `FromISR` Peripheral to NVIC Priority `0..4`:** Violates `configMAX_SYSCALL_INTERRUPT_PRIORITY` (`5`), corrupts kernel ready lists inside critical sections, and trips `configASSERT`!
4. **Calling `xSemaphoreTake()` on a Mutex Inside an ISR:** Forbidden! Mutexes require an owning TCB for priority inheritance.
5. **Conflating Priority Inheritance with Deadlock Prevention:** Two tasks locking Mutex A/B in opposite order STILL deadlock even with Priority Inheritance active!
6. **Putting Power-Stage Overcurrent Trip Inside an RTOS Task:** Software task scheduling latency (`10–100 µs`) destroys power MOSFETs; always wire `COMP1` $\to$ `TIM1_BKIN` hardware break and use the RTOS task only for post-fault logging.

---

## 11. Progressive Coding Exercises & Design Challenges

1. **[Level 2]** Implement a 3-task Priority Inversion demo (`L=1, M=2, H=3`): prove on a logic analyzer that a Binary Semaphore lets `M` starve `H`, then swap to `xSemaphoreCreateMutex()` and observe `uxTaskPriorityGet(L)` elevate to `3`!
2. **[Level 2]** Induce an AB/BA 2-mutex deadlock, inspect both `eBlocked` tasks in GDB, then eliminate the deadlock via strict lock ordering and bounded timeouts.
3. **[Level 2]** Run an unmanaged Priority-2 `TIM2` ISR alongside a kernel-managed Priority-6 `USART2` ISR; prove `TIM2` has **0 added jitter** during `taskENTER_CRITICAL()`.
4. **[Level 3]** Implement an Event-Group Watchdog Supervisor (`xEventGroupWaitBits(..., pdTRUE, pdTRUE, timeout)`) that refreshes `IWDG` ONLY when all 3 worker tasks check in within deadline; stall 1 worker and verify clean IWDG recovery.

---

## 12. Self-Assessment Mastery Questions

1. **Q:** Why do `SysTick` and `PendSV` run at the **lowest** NVIC priority (`15` / `0xF0`) on Cortex-M4F?  
   *Answer Check:* So `PendSV` never preempts a real-time hardware ISR (which would cause a HardFault / UsageFault trying to switch thread stacks during Handler Mode) and waits cleanly until all pending ISRs finish!
2. **Q:** What is the difference between `EXC_RETURN = 0xFFFFFFFD` and `0xFFFFFFED` during `PendSV_Handler`?  
   *Answer Check:* Bit 4 (`0x10`) = `1` (`0xFFFFFFFD`) means standard 8-word basic frame (no FPU); Bit 4 = `0` (`0xFFFFFFED`) means extended 26-word FPU frame (`S0–S15` auto-stacked, so `PendSV` must also save/restore `S16–S31`!).
3. **Q:** Why can a Cortex-M4F task using `SysTick` tickless idle only sleep for `98.69 ms` at 170 MHz, and how does STM32G474RE solve this?  
   *Answer Check:* 24-bit `SysTick->LOAD` maxes out at $2^{24}/170\text{ MHz} = 98.69\text{ ms}$ (and stops in Stop mode). Use **`LPTIM1`** (`configUSE_TICKLESS_IDLE = 2`) clocked by 32.768 kHz `LSE` in Stop 0/1!

---

## 13. Measurable Mastery & Completion Criteria

| Concept ID | Pass / Fail Verification Criterion | Measurement Tool |
|---|---|---|
| `RTOS-CTX-001` / `PORT-001` | Step `PendSV_Handler` in GDB confirming `PSP != MSP`, `PRIGROUP == 0/3`, `SHP` `0xF0`, and `EXC_RETURN` `0xFFFFFFFD` vs `0xFFFFFFED` | GDB |
| `RTOS-ISR-001` / `002` | `vTaskNotifyGiveFromISR` + `portYIELD_FROM_ISR` wakes task in `< 5 µs`; Priority-2 unmanaged ISR shows `0` jitter during `taskENTER_CRITICAL()` | Logic Analyzer |
| `RTOS-HAZ-001` / `002` | Capture L/M/H inversion fixed by Mutex PIP; break AB/BA deadlock via lock ordering | Logic Analyzer + GDB |
| `RTOS-MEM-001` / `002` | Zero-heap `xTaskCreateStatic` app with `configCHECK_FOR_STACK_OVERFLOW = 2` catching intentional recursion and $\ge 30\%$ high-water margin | GDB |
| `RTOS-THEORY-001` | Measure $C_i$ via `DWT->CYCCNT` and prove RMS utilization & exact RTA $R_i \le D_i$ with 0 missed `xTaskDelayUntil()` deadlines | `DWT->CYCCNT` + Calculation |

---

> **Sequential Navigation (Step 16 of 17):**  
> **Prev:** [Stage 15 (13) — Watchdogs, RTC & Low-Power Modes (`15-power-management/`)](../15-power-management/README.md) | **Home:** [`START_HERE.md`](../START_HERE.md) | **Progress:** [`LEARNER_PROGRESS_DASHBOARD.md`](../LEARNERS/LEARNER_PROGRESS_DASHBOARD.md) | **Next:** [Stage 17 (15) — Advanced Firmware Engineering, MISRA & CI (`17-advanced-firmware-engineering/`)](../17-advanced-firmware-engineering/README.md)
