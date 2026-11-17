# Stage 05 Mastery Exam: RTOS & Advanced Firmware Engineering

### Questions (100 Points Total)
1. **Preemptive Task Scheduling (20 Points):** Detail the exact role of the SysTick timer and the PendSV exception in a Cortex-M4 FreeRTOS context switch. Why is context switching deferred to PendSV rather than executed directly inside SysTick?
2. **Priority Inversion & Inheritance (25 Points):** Explain the classic priority inversion problem using three tasks (Low, Medium, High) and a shared resource. How does a FreeRTOS mutex with priority inheritance resolve this deadlock risk?
3. **Interrupt-Safe RTOS APIs (20 Points):** Why is calling `xQueueSend()` forbidden inside an ISR? What happens if `xQueueSendFromISR()` sets `*pxHigherPriorityTaskWoken` to `pdTRUE`?
4. **Independent vs Window Watchdogs (15 Points):** Contrast the IWDG and WWDG. Under what circumstances will refreshing a WWDG too early cause an immediate system reset?
5. **MISRA C Safety (20 Points):** Identify three distinct violations of MISRA C:2012 in the following snippet and rewrite it to be fully compliant:
   ```c
   int *p = 0x48000000;
   *p = *p | (1 << 5);
   ```
