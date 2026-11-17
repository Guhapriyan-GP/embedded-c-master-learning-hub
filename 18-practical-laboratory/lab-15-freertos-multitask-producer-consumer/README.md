# Lab 15: FreeRTOS Multitasking Producer-Consumer with Queues & Mutexes

## 1. Learning Objectives
Deploy FreeRTOS on the STM32G474RE, architecting a preemptive multi-threaded system with sensor acquisition, message queues, and thread-safe UART logging via mutexes.

## 2. Hardware Requirements & Wiring
- **Hardware:** NUCLEO-G474RE board, serial terminal.
- **Wiring:** Onboard USART2 ST-LINK VCP.

## 3. Peripheral Registers & Official Documentation
- SysTick / PendSV / SVC core exceptions
- FreeRTOS kernel memory management (`heap_4`)

**Reference Documentation:** FreeRTOS Kernel Reference Guide & ST PM0214.

## 4. Mathematical Calculations & Bitfield Settings
Task stack allocation: 128 words per task. Tick rate: 1000 Hz (1 ms).

## 5. Complete Bare-Metal C Source Code
```c
/* FreeRTOS Preemptive Architecture Template */
#include "stm32g474xx.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include <stdio.h>

QueueHandle_t sensor_queue;
SemaphoreHandle_t uart_mutex;

void vSensorTask(void *pvParameters) {
    uint32_t val = 0;
    while (1) {
        val++;
        xQueueSend(sensor_queue, &val, portMAX_DELAY);
        vTaskDelay(pdMS_TO_TICKS(100)); /* 10 Hz sampling */
    }
}

void vTelemetryTask(void *pvParameters) {
    uint32_t rx_val;
    while (1) {
        if (xQueueReceive(sensor_queue, &rx_val, portMAX_DELAY) == pdTRUE) {
            if (xSemaphoreTake(uart_mutex, portMAX_DELAY) == pdTRUE) {
                printf("[RTOS] Packet: %lu\r\n", rx_val);
                xSemaphoreGive(uart_mutex);
            }
        }
    }
}

int main(void) {
    /* Initialize clocks and USART2 */
    sensor_queue = xQueueCreate(10, sizeof(uint32_t));
    uart_mutex = xSemaphoreCreateMutex();

    xTaskCreate(vSensorTask, "Sensor", 128, NULL, 2, NULL);
    xTaskCreate(vTelemetryTask, "Telem", 128, NULL, 1, NULL);

    vTaskStartScheduler();
    while (1);
}
```

## 6. Debugging & Register Verification Procedure
Use FreeRTOS task runtime statistics to verify CPU utilization per task and inspect queue watermarks.

## 7. Common Beginner Mistakes & Diagnostic Guidance
1. Misconfiguring interrupt priorities: Peripheral interrupts interacting with FreeRTOS must have priority numerically >= `configMAX_SYSCALL_INTERRUPT_PRIORITY`.
2. Allocating too little stack space to tasks calling `printf`.

## 8. Expected Output & Verification
Both tasks execute deterministically with telemetry packets streaming smoothly over serial without dropped frames.
