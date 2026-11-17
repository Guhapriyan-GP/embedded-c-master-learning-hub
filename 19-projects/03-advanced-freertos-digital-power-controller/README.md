# Project 03: FreeRTOS Digital Power & Motor Controller

## 1. Project Overview & Architecture
This production-grade capstone project implements a deterministic, multi-threaded digital controller under FreeRTOS. It coordinates high-frequency current sensing, closed-loop feedback calculation, user interface menus, and telemetry logging across preemptive real-time tasks.

```
+---------------------------------------------------------------------------------+
|                                FreeRTOS Kernel                                  |
|                                                                                 |
|  [ Task 1: Control Loop ] ──(Queue)──> [ Task 2: Telemetry Logger ]             |
|   - Priority: HIGH (3)                  - Priority: LOW (1)                     |
|   - Period: 1 kHz (1 ms)                - Streams via USART2 with Mutex         |
|   - High-Speed Current Feedback                                                 |
|                                                                                 |
|  [ Task 3: Fault Monitor ] ──────────> [ Task 4: UI & Display ]                 |
|   - Priority: CRITICAL (4)              - Priority: IDLE (0)                    |
|   - Checks Overcurrent Flags            - Updates OLED Screen                   |
+---------------------------------------------------------------------------------+
```

## 2. Real-Time Task Allocation & Deadlines
1. **Task 1: Current Control Loop (Priority 3 - High):**
   - Woken up deterministically every 1 ms using `vTaskDelayUntil()`.
   - Reads filtered ADC phase current from DMA buffer.
   - Computes Proportional-Integral (PI) control effort.
   - Updates TIM1 complementary PWM registers (`CCR1`, `CCR2`) with dead-time insertion.
   - Posts status packet to the telemetry queue without blocking (`xQueueSend(..., 0)`).
2. **Task 2: Telemetry Logger (Priority 1 - Low):**
   - Blocks on telemetry queue until new packets arrive (`portMAX_DELAY`).
   - Takes `uart_mutex` to ensure thread-safe access to USART2.
   - Formats and transmits serial diagnostic logs.
3. **Task 3: Safety & Hardware Watchdog (Priority 4 - Critical):**
   - Monitors analog comparator (COMP1) overcurrent trip flags.
   - Kicks hardware Independent Watchdog (IWDG) only when all tasks report healthy execution flags.
   - Enters emergency safe shutdown if current thresholds are exceeded.

## 3. Concurrency Protection Patterns
- **Priority Inversion Avoidance:** All shared serial output routines utilize FreeRTOS mutexes (`xSemaphoreCreateMutex`) supporting automatic priority inheritance.
- **Interrupt Interaction:** Any notifications sent from hardware ISRs utilize `xQueueSendFromISR` and trigger immediate context switching when a higher-priority task is unblocked.
