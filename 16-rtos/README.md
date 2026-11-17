# Stage 15: Real-Time Operating Systems (FreeRTOS) & Concurrency

## 1. Prerequisites
Stage 03 (ARM Cortex-M4 architecture), Stage 04 (Toolchain), and Stage 09 (Interrupts).

## 2. Core Technical Theory & Engineering Principles
When embedded systems grow complex, simple superloops become unmaintainable. A Real-Time Operating System (RTOS) provides deterministic, preemptive multitasking.

### 1. Superloop vs RTOS
- **Superloop:** Single infinite `while(1)` loop. Task execution time varies; high-priority events suffer jitter; blocking delays freeze the entire system.
- **Preemptive RTOS:** Priority-based preemptive scheduling. High-priority tasks execute immediately upon event arrival; lower-priority tasks are suspended.

### 2. FreeRTOS Kernel Architecture on Cortex-M4
- **Task Control Block (TCB):** Data structure storing task priority, stack pointers, and state (Running, Ready, Blocked, Suspended).
- **Context Switch Mechanism:**
  1. The **SysTick** timer fires periodically (typically 1000 Hz) and selects the highest-priority ready task.
  2. SysTick triggers the **PendSV** (Pended Service Call) software exception.
  3. The `PendSV_Handler` assembly routine saves CPU registers of the current task onto its stack, updates its TCB, loads the new task's stack pointer, restores registers, and resumes execution.

### 3. Synchronization Primitives & Hazards
- **Queues:** Thread-safe FIFO data structures for inter-task communication.
- **Semaphores:** Signaling mechanism for task-to-task or ISR-to-task synchronization.
- **Mutexes:** Mutual exclusion locks with **Priority Inheritance** to prevent priority inversion deadlocks.
- **Interrupt Interaction:** Any RTOS API called from an ISR must use the dedicated `FromISR` variant (e.g. `xQueueSendFromISR`).

## 3. Practical Coding Exercises & Challenges
1. Implement a 3-task FreeRTOS application with producer, consumer, and monitor tasks.
2. Demonstrate priority inversion and verify its resolution using priority inheritance mutexes.
3. Measure context switch time using a GPIO toggle inside the PendSV handler.

## 4. Expected Learning Outcomes & Mastery Criteria
Ability to architect robust, deterministic multi-threaded embedded firmware using FreeRTOS.
