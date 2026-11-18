# Lab 15: FreeRTOS Preemptive Producer-Consumer (Queues, Mutexes, Static Allocation & ISR Safety)

> **Verification Status:** Statically Reviewed (Requires FreeRTOS Kernel `ARM_CM4F` Port Headers — Not Compiled Standalone Without FreeRTOS Tree / Not Tested on Physical NUCLEO-G474RE Hardware)

---

## 1. Objective & Concept IDs

Architect a production-safe **FreeRTOS** multi-task application on **STM32G474RE (`170 MHz`, Cortex-M4F)** featuring:
1. Proper **`NVIC_SetPriorityGrouping(0U)`** (4 preemption bits, 0 subpriority bits — mandatory for FreeRTOS on STM32!).
2. Complete, verified **`FreeRTOSConfig.h`** (`configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY = 5`, `configCHECK_FOR_STACK_OVERFLOW = 2`, `configSUPPORT_STATIC_ALLOCATION = 1`).
3. Adequate **`512-word` (`2048-byte`)** stack sizing for `vTelemetryTask` (fixing the guaranteed Cortex-M4F FPU + `printf` stack overflow!).
4. Strict `NULL` / `pdPASS` creation checks, drift-free `vTaskDelayUntil()`, **`xTaskCreateStatic()`** zero-heap deterministic task creation, and **`xQueueSendFromISR()` + `portYIELD_FROM_ISR()`** ISR-to-task handoff.

- **Target Concept IDs:** `RTOS-SCHED-001` (Preemptive Scheduling & `vTaskDelayUntil`), `RTOS-SYNC-001` (Queues & Priority-Inheritance Mutexes), `RTOS-ISR-001` (`MAX_SYSCALL_INTERRUPT_PRIORITY` & `FromISR`), `RTOS-MEM-001` (Static Allocation & Stack Overflow Hooks)
- **Scope Layer:** `[RTOS Generic]` / `[Cortex-M4F Specific]` / `[STM32G474RE Exact]`

---

## 2. Required Hardware

- **Development Board:** ST NUCLEO-G474RE (`MB1367` Rev C)
- **Host Interface:** USB cable carrying ST-LINK Virtual COM Port (`USART2` on `PA2`/`PA3`, `115200 8N1`) and onboard Blue Button `B1` (`PC13`, Active-HIGH) for ISR-to-Task event injection

---

## 3. Required Software

- `arm-none-eabi-gcc` (14.3.1+) with `-mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard`
- **FreeRTOS Kernel** (`v10.5+` or `v11.x`) using portable layer `portable/GCC/ARM_CM4F/port.c` + `portable/MemMang/heap_4.c`
- CMSIS headers (`stm32g474xx.h`, `core_cm4.h`)

---

## 4. Wiring & Pin Configuration

| MCU Pin | Function | Configuration | Role |
|---|---|---|---|
| `PA2` / `PA3` | `USART2_TX` / `RX` (`AF7`) | `115200 baud, 8N1` | Mutex-protected telemetry stream (`vTelemetryTask`) |
| `PC13` | `B1` Button (`EXTI13`) | Input `00b`, Rising Edge, NVIC Priority **`6`** (`>= 5`) | Demonstrates `xQueueSendFromISR` safe interrupt handoff |
| `PA5` | `LD2` LED | Output `01b` | Heartbeat toggled on every processed packet |

---

## 5. Register & Kernel Configuration (`FreeRTOSConfig.h`)

**Official Documentation:** **FreeRTOS Kernel Reference** (*Running the RTOS on an ARM Cortex-M Core*), **ARM PM0214 §4.3** (*NVIC Priority Grouping*), **RM0440**.

> [!IMPORTANT]
> **Critical STM32 + FreeRTOS NVIC Rules:**
> 1. **Call `NVIC_SetPriorityGrouping(0U);` before `vTaskStartScheduler()`!** On STM32G474RE (`__NVIC_PRIO_BITS = 4`), `PRIGROUP = 0` (identical to ST HAL `NVIC_PRIORITYGROUP_4` = `3`) allocates all 4 bits to **Preemption Priority** (`0..15`) and `0` bits to Subpriority. Any non-zero subpriority split breaks FreeRTOS's `BASEPRI` critical-section masking!
> 2. **Interrupt Priority Boundary:** Any ISR calling `...FromISR()` APIs (such as `EXTI15_10_IRQHandler`) **MUST** have a numerical NVIC priority **`>= configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY` (`5`)** and `<= 15`. Priorities `0..4` are never masked by FreeRTOS and **MUST NEVER** call any FreeRTOS API!
> 3. **Stack Sizing on Cortex-M4F:** `printf()` via newlib-nano + 34-word Cortex-M4F FPU extended exception frame + FreeRTOS context requires **$\ge 512$ words (`2048 bytes`)** — allocating only `128` words (`512 bytes`) guarantees silent stack corruption!

### Complete `FreeRTOSConfig.h` for STM32G474RE @ 170 MHz

```c
#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

#include "stm32g474xx.h"

#define configUSE_PREEMPTION                        1
#define configUSE_PORT_OPTIMISED_TASK_SELECTION     1
#define configUSE_TICKLESS_IDLE                     0
#define configCPU_CLOCK_HZ                          (170000000UL)
#define configTICK_RATE_HZ                          ((TickType_t)1000)
#define configMAX_PRIORITIES                        (7)
#define configMINIMAL_STACK_SIZE                    ((uint16_t)128)
#define configTOTAL_HEAP_SIZE                       ((size_t)(24 * 1024))
#define configMAX_TASK_NAME_LEN                     (16)
#define configUSE_16_BIT_TICKS                      0
#define configIDLE_SHOULD_YIELD                     1
#define configUSE_TASK_NOTIFICATIONS                1
#define configUSE_MUTEXES                           1
#define configUSE_RECURSIVE_MUTEXES                 0
#define configUSE_COUNTING_SEMAPHORES               1
#define configQUEUE_REGISTRY_SIZE                   8

/* Deterministic allocation & mandatory production safety checks */
#define configSUPPORT_STATIC_ALLOCATION             1
#define configSUPPORT_DYNAMIC_ALLOCATION            1
#define configCHECK_FOR_STACK_OVERFLOW              2   /* Pattern canary check */
#define configUSE_MALLOC_FAILED_HOOK                1
#define configUSE_IDLE_HOOK                         0
#define configUSE_TICK_HOOK                         0
#define INCLUDE_vTaskDelay                          1
#define INCLUDE_vTaskDelayUntil                     1
#define INCLUDE_uxTaskGetStackHighWaterMark         1

/* Cortex-M4 NVIC 4-bit priority configuration (__NVIC_PRIO_BITS == 4) */
#define configPRIO_BITS                             4
#define configLIBRARY_LOWEST_INTERRUPT_PRIORITY     15
#define configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY 5

#define configKERNEL_INTERRUPT_PRIORITY \
    (configLIBRARY_LOWEST_INTERRUPT_PRIORITY << (8 - configPRIO_BITS))
#define configMAX_SYSCALL_INTERRUPT_PRIORITY \
    (configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY << (8 - configPRIO_BITS))

/* Trap misconfigured ISR priorities / invalid API usage immediately in debug */
#define configASSERT(x) do { if ((x) == 0) { __disable_irq(); __BKPT(0); for (;;) {} } } while (0)

/* Map FreeRTOS handlers directly onto CMSIS startup vector names */
#define vPortSVCHandler     SVC_Handler
#define xPortPendSVHandler  PendSV_Handler
#define xPortSysTickHandler SysTick_Handler

#endif /* FREERTOS_CONFIG_H */
```

---

## 6. Code Implementation

```c
#include "stm32g474xx.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include <stdint.h>
#include <stdio.h>

extern void Clock_Init_170MHz(void);   /* Reuse verified Lab 03 / Lab 08 implementation */
extern void USART2_Init_115200(void);  /* Reuse verified Lab 08 driver + _write() */

typedef struct {
    uint32_t seq;
    uint32_t tick_ms;
    uint8_t  source; /* 0 = Periodic Sensor Task, 1 = EXTI13 Button ISR */
} telemetry_msg_t;

static QueueHandle_t     g_sensor_queue = NULL;
static SemaphoreHandle_t g_uart_mutex   = NULL;

/* Static task buffers for zero-heap deterministic allocation (xTaskCreateStatic) */
#define SENSOR_STACK_WORDS      (256U)
#define TELEMETRY_STACK_WORDS   (512U) /* 2048 bytes: safe for Cortex-M4F FPU + printf! */

static StackType_t  g_sensor_stack[SENSOR_STACK_WORDS];
static StaticTask_t g_sensor_tcb;

/* Mandatory static Idle Task memory when configSUPPORT_STATIC_ALLOCATION == 1 */
static StackType_t  g_idle_stack[configMINIMAL_STACK_SIZE];
static StaticTask_t g_idle_tcb;

void vApplicationGetIdleTaskMemory(StaticTask_t **ppxIdleTaskTCBBuffer,
                                   StackType_t  **ppxIdleTaskStackBuffer,
                                   uint32_t      *pulIdleTaskStackSize) {
    *ppxIdleTaskTCBBuffer   = &g_idle_tcb;
    *ppxIdleTaskStackBuffer = g_idle_stack;
    *pulIdleTaskStackSize   = configMINIMAL_STACK_SIZE;
}

/* Mandatory safety hooks */
void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName) {
    (void)xTask;
    (void)pcTaskName;
    __disable_irq();
    __BKPT(1);
    for (;;) {}
}

void vApplicationMallocFailedHook(void) {
    __disable_irq();
    __BKPT(2);
    for (;;) {}
}

/* ISR-safe producer: NVIC priority MUST be >= configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY (5) */
void EXTI15_10_IRQHandler(void) {
    if ((EXTI->PR1 & EXTI_PR1_PIF13) != 0U) {
        EXTI->PR1 = EXTI_PR1_PIF13;

        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        telemetry_msg_t msg = {
            .seq     = 0xFFFFFFFFUL,
            .tick_ms = xTaskGetTickCountFromISR(),
            .source  = 1U
        };
        (void)xQueueSendFromISR(g_sensor_queue, &msg, &xHigherPriorityTaskWoken);

        __DSB();
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }
}

/* Producer Task (Static Allocation, Priority 2, exact 10 Hz via vTaskDelayUntil) */
static void vSensorTask(void *pvParameters) {
    (void)pvParameters;
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xPeriod = pdMS_TO_TICKS(100U);
    uint32_t seq = 0U;

    for (;;) {
        telemetry_msg_t msg = {
            .seq     = ++seq,
            .tick_ms = xTaskGetTickCount(),
            .source  = 0U
        };
        /* Non-blocking or bounded timeout prevents deadlock if consumer stalls */
        (void)xQueueSend(g_sensor_queue, &msg, pdMS_TO_TICKS(10U));

        /* Drift-free absolute periodic wake */
        vTaskDelayUntil(&xLastWakeTime, xPeriod);
    }
}

/* Consumer Task (512 words = 2048 B stack for M4F FPU + printf, Priority 1) */
static void vTelemetryTask(void *pvParameters) {
    (void)pvParameters;
    telemetry_msg_t rx;

    for (;;) {
        if (xQueueReceive(g_sensor_queue, &rx, portMAX_DELAY) == pdTRUE) {
            if (xSemaphoreTake(g_uart_mutex, pdMS_TO_TICKS(50U)) == pdTRUE) {
                UBaseType_t hwm = uxTaskGetStackHighWaterMark(NULL);
                printf("[RTOS t=%lu ms] src=%s seq=%lu (stack_free=%lu words)\r\n",
                       (unsigned long)rx.tick_ms,
                       (rx.source == 0U) ? "SENSOR" : "BTN_ISR",
                       (unsigned long)rx.seq,
                       (unsigned long)hwm);
                xSemaphoreGive(g_uart_mutex);
            }

            /* Atomic LD2 toggle */
            if ((GPIOA->ODR & GPIO_ODR_OD5) != 0U) {
                GPIOA->BSRR = GPIO_BSRR_BR5;
            } else {
                GPIOA->BSRR = GPIO_BSRR_BS5;
            }
        }
    }
}

static void EXTI13_Button_Init(void) {
    RCC->AHB2ENR |= RCC_AHB2ENR_GPIOCEN | RCC_AHB2ENR_GPIOAEN;
    RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN;
    volatile uint32_t tmpreg = RCC->APB2ENR;
    (void)tmpreg;

    GPIOA->MODER  = (GPIOA->MODER & ~(3U << 10U)) | (1U << 10U);
    GPIOC->MODER &= ~(3U << 26U);
    GPIOC->PUPDR &= ~(3U << 26U);

    SYSCFG->EXTICR[3] = (SYSCFG->EXTICR[3] & ~SYSCFG_EXTICR4_EXTI13_Msk) | SYSCFG_EXTICR4_EXTI13_PC;
    EXTI->RTSR1 |=  EXTI_RTSR1_RT13;
    EXTI->FTSR1 &= ~EXTI_FTSR1_FT13;
    EXTI->PR1    =  EXTI_PR1_PIF13;
    EXTI->IMR1  |=  EXTI_IMR1_IM13;

    /* CRITICAL: Priority 6 >= configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY (5)! */
    NVIC_SetPriority(EXTI15_10_IRQn, 6U);
    NVIC_ClearPendingIRQ(EXTI15_10_IRQn);
    NVIC_EnableIRQ(EXTI15_10_IRQn);
}

int main(void) {
    /* 1. CRITICAL: Set NVIC to 4 preemption / 0 subpriority bits BEFORE FreeRTOS! */
    NVIC_SetPriorityGrouping(0U);

    /* 2. Initialize 170 MHz PLL, USART2 VCP, and EXTI13 */
    Clock_Init_170MHz();
    USART2_Init_115200();
    EXTI13_Button_Init();

    /* 3. Create IPC primitives and strictly verify non-NULL handles */
    g_sensor_queue = xQueueCreate(16U, sizeof(telemetry_msg_t));
    g_uart_mutex   = xSemaphoreCreateMutex();
    configASSERT(g_sensor_queue != NULL);
    configASSERT(g_uart_mutex   != NULL);

    /* 4. Static task creation (zero heap) for Sensor Producer */
    TaskHandle_t hSensor = xTaskCreateStatic(
        vSensorTask, "Sensor", SENSOR_STACK_WORDS, NULL, 2U, g_sensor_stack, &g_sensor_tcb);
    configASSERT(hSensor != NULL);

    /* 5. Dynamic task creation with verified 512-word (2048 B) stack for printf Telemetry */
    BaseType_t ok = xTaskCreate(
        vTelemetryTask, "Telem", TELEMETRY_STACK_WORDS, NULL, 1U, NULL);
    configASSERT(ok == pdPASS);

    /* 6. Start FreeRTOS preemptive scheduler (never returns under normal operation) */
    vTaskStartScheduler();

    /* Should never reach here unless Idle task allocation failed */
    for (;;) {
        __BKPT(0);
    }
}
```

---

## 7. Expected Output

At `115200 8N1` on the ST-LINK Virtual COM port:
```text
[RTOS t=100 ms] src=SENSOR seq=1 (stack_free=342 words)
[RTOS t=200 ms] src=SENSOR seq=2 (stack_free=342 words)
[RTOS t=247 ms] src=BTN_ISR seq=4294967295 (stack_free=342 words)
[RTOS t=300 ms] src=SENSOR seq=3 (stack_free=342 words)
```

---

## 8. Debugging Procedure

1. Monitor `stack_free` (`uxTaskGetStackHighWaterMark(NULL)`) printed by `vTelemetryTask` — confirm healthy positive headroom (`> 100` words free).
2. Deliberately test `configASSERT`: temporarily lower `NVIC_SetPriority(EXTI15_10_IRQn, 2U)` (numerically `< 5`), press `B1`, and confirm FreeRTOS immediately catches the illegal priority inside `vPortValidateInterruptPriority()`!
3. Verify `SCB->AIRCR` bits `[10:8]` (`PRIGROUP`) read `0` (`NVIC_SetPriorityGrouping(0U)`).

---

## 9. Common Errors & Troubleshooting

| Symptom | Root Cause | Verified Fix |
|---|---|---|
| HardFault or `vApplicationStackOverflowHook` inside `printf` | Allocated `128` words (`512 B`) for a task calling `printf` on Cortex-M4F | Allocate `>= 512` words (`2048 B`) or use lightweight `snprintf`/integer formatter |
| `configASSERT` fails inside `xQueueSendFromISR` | Forgot `NVIC_SetPriorityGrouping(0U)` OR set ISR priority `< 5` | Call `NVIC_SetPriorityGrouping(0U)` and set ISR priority `5..15` |
| `vTaskStartScheduler()` never ticks or crashes on `SVC 0` | Startup vector table did not map `SVC_Handler`, `PendSV_Handler`, `SysTick_Handler` | Define the 3 `#define vPort...` mappings in `FreeRTOSConfig.h` |
| Periodic task drifts over minutes | Used relative `vTaskDelay()` instead of absolute `vTaskDelayUntil()` | Use `vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(100))` |

---

## 10. Verification Procedure

- [ ] `NVIC_SetPriorityGrouping(0U)` executes prior to `vTaskStartScheduler()`.
- [ ] `FreeRTOSConfig.h` sets `configCHECK_FOR_STACK_OVERFLOW = 2` and `configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY = 5`.
- [ ] All `xQueueCreate`, `xSemaphoreCreateMutex`, `xTaskCreateStatic`, and `xTaskCreate` return values are checked.
- [ ] `vTelemetryTask` stack is `512` words (`2048` bytes) with verified positive high-water mark.

---

## 11. Extension Exercises

1. **Direct-to-Task Notifications:** Replace `g_sensor_queue` for the button event with `vTaskNotifyGiveFromISR()` / `ulTaskNotifyTake()` and benchmark latency reduction (~45% faster, zero queue RAM).
2. **Priority Inheritance Demonstration:** Add a medium-priority CPU-bound task (Priority `2`) competing while a low-priority task (Priority `1`) holds `g_uart_mutex` and a high-priority task (Priority `3`) waits on it — verify Mutex priority inheritance prevents unbounded inversion.
3. **100% Zero-Heap Build:** Convert `g_sensor_queue`, `g_uart_mutex`, and `vTelemetryTask` to `xQueueCreateStatic`, `xSemaphoreCreateMutexStatic`, and `xTaskCreateStatic`, then set `configSUPPORT_DYNAMIC_ALLOCATION = 0` and remove `heap_4.c` from the link!
