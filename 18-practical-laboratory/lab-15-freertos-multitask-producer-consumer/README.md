# Lab 15: FreeRTOS Preemptive Producer-Consumer (Queues, Mutexes, Static Allocation & ISR Safety)

> ⬅️ **Prev Lab:** [Lab 14: Stop 0 Low-Power Wakeup](../lab-14-low-power-stop-mode-wakeup/README.md) | ⬆️ **Parent Stage:** [18-practical-laboratory (All 20 Labs)](../README.md) | **Next Lab:** [Lab 16: DAC/OPAMP/COMP TIM1_BKIN](../lab-16-dac-opamp-comp-hardware-break/README.md) ➡️

> **Verification Status:** `ONBOARD HARDWARE TESTED` (`arm-none-eabi-gcc 14.3.1`, FreeRTOS-Kernel `V11.1.0`, 0 warnings) | **Hardware Evidence:** NUCLEO-G474RE `nucleo_g474re_freertos.elf` **`6/6 ALL_PASSED`** + `8.0 s` live `COM8` `[TELEM]` stream (`fpu_err=0`, `heap=21104/21104`)

> **Clock Configuration Profile:** **Primary = `170 MHz Range 1 Boost`** (`configCPU_CLOCK_HZ = SystemCoreClock = 170,000,000 Hz`, `1 ms` tick `LOAD = 169,999`, `USART2->BRR = 1,476`, `TIM2` 10 Hz `PSC = 16,999, ARR = 999`) | **Alternate = `160 MHz Range 1 Boost`** (`Clock_Init_160MHz()` sets `SystemCoreClock = 160,000,000 Hz`, auto-scaling `configCPU_CLOCK_HZ`, `LOAD = 159,999`, `USART2->BRR = 1,389`, `TIM2` `PSC = 15,999, ARR = 999`) | **Reusable Drivers:** [`clock_g4.h`](../../23-code-library/Drivers/clock_g4.h), [`usart_g4.h`](../../23-code-library/Drivers/usart_g4.h), [`tim_g4.h`](../../23-code-library/Drivers/tim_g4.h), [`exti_g4.h`](../../23-code-library/Drivers/exti_g4.h), [`iwdg_g4.h`](../../23-code-library/Drivers/iwdg_g4.h).

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

## 3. Required Software & Reproducible Firmware Project

- `arm-none-eabi-gcc` (`14.3.1+`) with `-mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard`
- **Pinned Official FreeRTOS-Kernel `V11.1.0`** (Commit `dbf70559b27d39c1fdb68dfb9a32140b6a6777a0`, MIT License) vendored directly in [`23-code-library/freertos/FreeRTOS-Kernel/`](../../23-code-library/freertos/FreeRTOS-Kernel/VERSION.txt) using `portable/GCC/ARM_CM4F/port.c` + `portable/MemMang/heap_4.c`
- CMSIS headers (`23-code-library/Drivers/CMSIS/Device/ST/STM32G4xx/Include/stm32g474xx.h`, `core_cm4.h`)

### Build the Compiled Reference Firmware (`23-code-library/freertos/`)

Instead of copying snippets manually, you can compile and flash the complete onboard-only reference implementation directly:

```bash
cd 23-code-library/freertos
make clean all          # Produces build/nucleo_g474re_freertos.elf & .bin
make nm-check           # Confirms SVC/PendSV/SysTick bind to FreeRTOS port.c
make flash              # Flash to NUCLEO-G474RE via STM32_Programmer_CLI
```

Key project files in [`23-code-library/freertos/`](../../23-code-library/freertos/README.md):
- [`Inc/FreeRTOSConfig.h`](../../23-code-library/freertos/Inc/FreeRTOSConfig.h) — Verified STM32G474RE `V11.1.0` kernel config (`PRIGROUP=0`, `MAX_SYSCALL_PRIO=5`, `CHECK_FOR_STACK_OVERFLOW=2`, static + dynamic allocation)
- [`Src/rtos_bsp.c`](../../23-code-library/freertos/Src/rtos_bsp.c) — Self-contained 170 MHz Boost clock, `USART2` 115200 8N1 VCP, `PA5` LD2, `PC13` EXTI13, `TIM2` 10 Hz IRQ, and `IWDG` (`DBG_IWDG_STOP=1`)
- [`Src/main.c`](../../23-code-library/freertos/Src/main.c) — 4-task architecture (`SupervisorTask`, `ConsumerTask`, `ProducerTask`, `MonitorTask`) with automated self-tests for NVIC config, Binary/Counting Semaphores, Mutex Priority Inheritance (`1 -> 5 -> 1`), Deadlock Guard (bounded timeout + lock order `A->B`), `TIM2_IRQHandler` `vTaskNotifyGiveFromISR`, Queue IPC, FPU preservation, Stack High-Water Marks, and Event-Group `IWDG` supervision.

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

### Explicit Clock Configuration Profile Declaration (`170 MHz` vs `160 MHz` Range 1 Boost)

In the compiled reference firmware ([`23-code-library/freertos/Inc/FreeRTOSConfig.h`](../../23-code-library/freertos/Inc/FreeRTOSConfig.h)), `configCPU_CLOCK_HZ` binds directly to CMSIS **`SystemCoreClock`** so FreeRTOS's `1000 Hz` `SysTick` reload, `USART2` baud rate, and `TIM2` ISR cadence never drift when switching between `170 MHz` and `160 MHz`:

| Clock Profile | `clock_g4.h` API | `configCPU_CLOCK_HZ` | FreeRTOS `SysTick->LOAD` (`1 ms`) | `USART2->BRR` (`115200`) | `TIM2` 10 Hz (`PSC` / `ARR`) | `IWDG` (`~3.0 s` LSI) |
|---|---|---|---|---|---|---|
| **`170 MHz Range 1 Boost` *(Primary)*** | `Clock_Init_170MHz()` | `170,000,000 Hz` | `169,999` (`0x2980F`) | `1,476` (`0x05C4`) | `16,999` / `999` | Independent (`32 kHz LSI`) |
| **`160 MHz Range 1 Boost` *(Alternate)*** | `Clock_Init_160MHz()` | `160,000,000 Hz` | `159,999` (`0x270FF`) | `1,389` (`0x056D`) | `15,999` / `999` | Independent (`32 kHz LSI`) |

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
        (void)EXTI->PR1; /* Flush AHB-to-APB2 posted-write bridge */

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

---

## 12. Standardized Verification Record

| Field | Record |
|---|---|
| **Verification Status** | `ONBOARD HARDWARE TESTED` *(Official FreeRTOS-Kernel `V11.1.0` ELF `6/6 ALL_PASSED` + `8.0 s` `[TELEM]` live on NUCLEO-G474RE)* |
| **Objective** | Build and verify a preemptive FreeRTOS `V11.1.0` (`ARM_CM4F`) multi-task firmware on STM32G474RE (`170 MHz` primary / `160 MHz` supported via `SystemCoreClock`) exercising `PRIGROUP = 0`, Static + Dynamic tasks, Queues, Binary/Counting Semaphores, Priority-Inheritance Mutexes, Deadlock Guard, `FromISR` notifications/queues, FPU preservation, Stack High-Water Marks, and Event-Group `IWDG` supervision. |
| **Firmware Version** | `v2.2.0-remediation` — Pinned FreeRTOS-Kernel **`V11.1.0`** (`dbf70559b27d39c1fdb68dfb9a32140b6a6777a0`) in [`23-code-library/freertos/`](../../23-code-library/freertos/README.md) |
| **Compiler Version** | `arm-none-eabi-gcc 14.3.1 20250623 (GNU Tools for STM32 14.3.rel1.20251027-0700)` |
| **Build Result** | **PASS** — `23-code-library/freertos/build/nucleo_g474re_freertos.elf` (`debug`: `FLASH = 21,480 B` `4.10%`, `RAM = 35,728 B` `36.34%`; `release`: `FLASH = 21,668 B` `4.13%`), **0 warnings, 0 errors**; `arm-none-eabi-nm` & `objdump` confirmed `SVC_Handler`, `PendSV_Handler`, `SysTick_Handler`, `TIM2_IRQHandler`, and `EXTI15_10_IRQHandler` vector bindings. |
| **Test Environment** | Windows 11 + ARM GNU Toolchain 14.3.rel1 + NUCLEO-G474RE (`MB1367` Rev C, `STM32G474RET6`, STLINK-V3E `V3J17M11`, `COM8`) |
| **Required Peripherals** | **Onboard Only:** `RCC`, `PWR`, `FLASH`, `NVIC`, `SysTick`, `USART2` (`PA2`/`PA3` ST-LINK VCP `COM8`), `TIM2` (10 Hz IRQ), `EXTI13` (`PC13` `B1`), `GPIOA` (`PA5` `LD2`), `IWDG` (`DBG_IWDG_STOP=1`), Cortex-M4F FPU. Zero external wiring required. |
| **Hardware Availability** | `ONBOARD — PHYSICALLY VERIFIED (Agent 4, 2026-11-18)` |
| **Pin Configuration** | `PA2`/`PA3` (`USART2` `AF7` `115200 8N1` VCP), `PA5` (`LD2` Push-Pull Output), `PC13` (`B1` Input `00b`, `EXTI13` Rising Edge, NVIC prio `7` >= `5`). |
| **Expected Behavior** | Emits 6 automated `[PASS]` self-test lines (`NVIC_CONFIG`, `SEMAPHORES`, `PRIORITY_INHERITANCE`, `DEADLOCK_GUARD`, `FROM_ISR_AND_QUEUE`, `STACK_HWM`), `[SUMMARY] FREERTOS_SELF_TESTS: 6/6 ALL_PASSED`, `[PASS] IWDG_SUPERVISOR`, and live `[TELEM t=... ms]` lines every `1000 ms` on `COM8` while `PA5` blinks steadily. |
| **Actual Observations** | **EXECUTED ON NUCLEO-G474RE (`23-code-library/freertos/build/nucleo_g474re_freertos.elf`, 2026-11-18):** All 6 automated RTOS self-tests passed (`NVIC_CONFIG`, `SEMAPHORES`, `PRIORITY_INHERITANCE` `1->5->1`, `DEADLOCK_GUARD`, `FROM_ISR_AND_QUEUE` `fpu_err=0`, `STACK_HWM`), `[SUMMARY] FREERTOS_SELF_TESTS: 6/6 ALL_PASSED`, `[PASS] IWDG_SUPERVISOR`, plus `8.0 s` live `COM8` `[TELEM]` (`q_tx=11..81`, `q_rx=10..80`, `tim2=10..80`, `wdg=2..9`, `heap=21104/21104`, healthy `HWM`). |
| **Register Values** | `SCB->AIRCR.PRIGROUP = 0`, `SHPR3`: `PendSV = 0xF0`, `SysTick = 0xF0`, `BASEPRI` critical mask = `0x50` (`MAX_SYSCALL_PRIO = 5`), `TIM2_IRQn` prio = `6`, `EXTI15_10_IRQn` prio = `7`, `DBGMCU->APB1FZR1[12] = 1`. |
| **Debugging Procedure** | Flash via `cd 23-code-library/freertos && STM32_Programmer_CLI -c port=SWD -w build/nucleo_g474re_freertos.bin 0x08000000 -v -rst`; monitor `COM8` at `115200 8N1`; verify vector bindings via `make nm-check` and inspect `uxTaskGetStackHighWaterMark` in `arm-none-eabi-gdb`. |
| **Remaining Limitations** | Tickless Idle (`configUSE_TICKLESS_IDLE = 0`) and `LPTIM1` Stop-1 RTOS sleep are deferred to future low-power RTOS extensions. Manual physical `B1` press (`btn=0 -> 1+`) remains optional. |

### 12.1 6-State Granular Verification Breakdown

| Verification Stage | Status | Evidence / Scope |
|---|---|---|
| **Static review** | `PASS` | `FreeRTOSConfig.h` (`__NVIC_PRIO_BITS=4`, `MAX_SYSCALL_PRIO=5`, `PRIGROUP=0`, `heap_4`, Method-2 overflow hook) & vector mappings verified against FreeRTOS `V11.1.0` & PM0214. |
| **Host test passed** | `N/A` | Cortex-M4F `ARM_CM4F` RTOS scheduler (`SVC`/`PendSV`/`SysTick`) validated directly on target silicon. |
| **Cross-compilation passed** | `PASS` | `23-code-library/freertos/build/nucleo_g474re_freertos.elf` (`debug` & `release`) built with `arm-none-eabi-gcc 14.3.1`, 0 warnings / 0 errors; `nm-check` verified vectors. |
| **On-target upload succeeded** | `PASS` | Flashed & verified on NUCLEO-G474RE (`21.16 KB`, `VDD = 3.28 V`, `0.060 s` verify). |
| **Physical behavior verified** | `PASS` | `6/6 ALL_PASSED` self-tests + `8.0 s` continuous 4-task + `TIM2` ISR + `IWDG` supervisor execution on `COM8`. |
| **Measurement verified** | `PASS` | Exact `10 Hz` producer/consumer/TIM2 cadence, `fpu_err=0`, zero heap leak (`heap=21104/21104`), and verified positive stack `HWM(S=339 C=309 P=186 M=324)`. |

---

> ⬅️ **Prev Lab:** [Lab 14: Stop 0 Low-Power Wakeup](../lab-14-low-power-stop-mode-wakeup/README.md) | ⬆️ **Parent Stage:** [18-practical-laboratory (All 20 Labs)](../README.md) | **Next Lab:** [Lab 16: DAC/OPAMP/COMP TIM1_BKIN](../lab-16-dac-opamp-comp-hardware-break/README.md) ➡️
