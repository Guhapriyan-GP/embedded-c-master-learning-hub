/**
 * @file    main.c
 * @brief   Reproducible, Self-Contained FreeRTOS V11.1.0 Verification Firmware
 *          for ST NUCLEO-G474RE (STM32G474RET6, ARM Cortex-M4F @ 170 MHz).
 *
 * Requires ZERO external wiring — uses only onboard NUCLEO-G474RE hardware:
 *   - USART2 ST-LINK VCP (PA2 TX / PA3 RX, AF7, 115200 8N1) for structured logs
 *   - LD2 User LED (PA5) driven by SupervisorTask upon healthy watchdog check-in
 *   - B1 User Button (PC13, Active-HIGH, EXTI13 @ NVIC prio 7) -> xQueueSendFromISR
 *   - TIM2 10 Hz Hardware Timer (NVIC prio 6) -> vTaskNotifyGiveFromISR
 *   - IWDG (~3.0 s timeout, DBGMCU freeze enabled) supervised via Event Group
 *
 * Demonstrated & Self-Verified RTOS Concepts:
 *   1. Mandatory NVIC_SetPriorityGrouping(0U) + BASEPRI (MAX_SYSCALL_PRIO = 5)
 *   2. Preemptive Priority Scheduler (4 persistent tasks + static Idle & Timer)
 *   3. Producer -> Consumer Queue IPC (xQueueSend / xQueueReceive + sequence check)
 *   4. Interrupt-to-Task Direct Notification (TIM2_IRQHandler -> vTaskNotifyGiveFromISR)
 *   5. Binary & Counting Semaphores (creation, bounds, acquire/release)
 *   6. Mutex & Priority Inheritance Protocol (PIP: base prio 1 -> promoted 5 -> restored 1)
 *   7. Deadlock Avoidance (strict lock ordering A->B + bounded timeout recovery)
 *   8. Stack High-Water Mark Monitoring + Method 2 Stack Overflow Hook + IWDG Supervisor
 */

#include "stm32g474xx.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "event_groups.h"
#include "timers.h"
#include "rtos_bsp.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/*---------------------------------------------------------------------------
 * Task Priorities & Stack Sizing (in 4-byte StackType_t words)
 *---------------------------------------------------------------------------*/
#define PRIO_MONITOR        (1U)
#define PRIO_PRODUCER       (2U)
#define PRIO_CONSUMER       (3U)
#define PRIO_SUPERVISOR     (4U)
#define PRIO_SELFTEST_HIGH  (5U)

#define STACK_SUPERVISOR    (512U) /* 2048 B: runs self-tests + snprintf logging */
#define STACK_CONSUMER      (384U) /* 1536 B: FPU + queue drain + notification   */
#define STACK_PRODUCER      (256U) /* 1024 B: FPU + 10 Hz periodic producer      */
#define STACK_MONITOR       (512U) /* 2048 B: periodic telemetry formatting      */
#define STACK_HELPER        (192U) /*  768 B: minimal temporary self-test helper */

/* NVIC Interrupt Priorities (MUST be >= configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY = 5) */
#define IRQ_PRIO_TIM2       (6U)
#define IRQ_PRIO_EXTI13     (7U)

/* Watchdog Event Group Check-In Bits */
#define EVT_BIT_PRODUCER    (1UL << 0U)
#define EVT_BIT_CONSUMER    (1UL << 1U)
#define EVT_BIT_MONITOR     (1UL << 2U)
#define EVT_ALL_WORKERS     (EVT_BIT_PRODUCER | EVT_BIT_CONSUMER | EVT_BIT_MONITOR)

/*---------------------------------------------------------------------------
 * Shared Data Types & IPC Handles
 *---------------------------------------------------------------------------*/
typedef enum {
    PKT_SRC_PRODUCER = 0U,
    PKT_SRC_BTN_ISR  = 1U
} pkt_source_t;

typedef struct {
    uint32_t     seq;
    uint32_t     tick_ms;
    float        scaled_val;
    pkt_source_t source;
    uint8_t      btn_pressed;
} sensor_packet_t;

typedef struct {
    volatile UBaseType_t base_prio;
    volatile UBaseType_t promoted_prio;
    volatile UBaseType_t restored_prio;
    volatile bool        high_acquired;
} pip_test_result_t;

/* Core IPC objects */
static QueueHandle_t      g_data_queue      = NULL;
static SemaphoreHandle_t  g_uart_mutex      = NULL;
static SemaphoreHandle_t  g_bin_sem         = NULL;
static SemaphoreHandle_t  g_cnt_sem         = NULL;
static SemaphoreHandle_t  g_pip_mutex       = NULL;
static SemaphoreHandle_t  g_dl_mutex_a      = NULL;
static SemaphoreHandle_t  g_dl_mutex_b      = NULL;
static EventGroupHandle_t g_wdg_events      = NULL;
static TimerHandle_t      g_sw_timer        = NULL;

/* Self-test synchronization semaphores */
static SemaphoreHandle_t  g_pi_locked_sem   = NULL;
static SemaphoreHandle_t  g_pi_low_done_sem = NULL;
static SemaphoreHandle_t  g_pi_hi_done_sem  = NULL;
static SemaphoreHandle_t  g_dl_locked_sem   = NULL;
static SemaphoreHandle_t  g_dl_done_sem     = NULL;

/* Task handles */
static TaskHandle_t g_supervisor_handle = NULL;
static TaskHandle_t g_consumer_handle   = NULL;
static TaskHandle_t g_producer_handle   = NULL;
static TaskHandle_t g_monitor_handle    = NULL;

/* Static task buffers (zero-heap deterministic allocation) */
static StaticTask_t g_idle_tcb;
static StackType_t  g_idle_stack[configMINIMAL_STACK_SIZE];

static StaticTask_t g_timer_tcb;
static StackType_t  g_timer_stack[configTIMER_TASK_STACK_DEPTH];

static StaticTask_t g_supervisor_tcb;
static StackType_t  g_supervisor_stack[STACK_SUPERVISOR];

static StaticTask_t g_consumer_tcb;
static StackType_t  g_consumer_stack[STACK_CONSUMER];

static StaticTask_t g_producer_tcb;
static StackType_t  g_producer_stack[STACK_PRODUCER];

static StaticTask_t g_helper1_tcb;
static StackType_t  g_helper1_stack[STACK_HELPER];

static StaticTask_t g_helper2_tcb;
static StackType_t  g_helper2_stack[STACK_HELPER];

/* Telemetry & verification counters */
static volatile uint32_t g_tim2_isr_total     = 0U;
static volatile uint32_t g_tim2_notify_total  = 0U;
static volatile uint32_t g_btn_isr_total      = 0U;
static volatile uint32_t g_queue_tx_total     = 0U;
static volatile uint32_t g_queue_rx_total     = 0U;
static volatile uint32_t g_seq_errors         = 0U;
static volatile uint32_t g_fpu_errors         = 0U;
static volatile uint32_t g_sw_timer_ticks     = 0U;
static volatile uint32_t g_wdg_feeds          = 0U;
static volatile bool     g_selftest_complete  = false;
static pip_test_result_t g_pip_result;

/*---------------------------------------------------------------------------
 * Thread-Safe UART Logging Helper
 *---------------------------------------------------------------------------*/
static void uart_log_str(const char *msg)
{
    if ((g_uart_mutex != NULL) && (xTaskGetSchedulerState() == taskSCHEDULER_RUNNING)) {
        if (xSemaphoreTake(g_uart_mutex, pdMS_TO_TICKS(100U)) == pdTRUE) {
            BSP_USART2_WriteStr(msg);
            (void)xSemaphoreGive(g_uart_mutex);
        }
    } else {
        BSP_USART2_WriteStr(msg);
    }
}

/*---------------------------------------------------------------------------
 * Mandatory FreeRTOS Static Allocation & Safety Hooks
 *---------------------------------------------------------------------------*/
void vApplicationGetIdleTaskMemory(StaticTask_t **ppxIdleTaskTCBBuffer,
                                   StackType_t **ppxIdleTaskStackBuffer,
                                   configSTACK_DEPTH_TYPE *puxIdleTaskStackSize)
{
    *ppxIdleTaskTCBBuffer   = &g_idle_tcb;
    *ppxIdleTaskStackBuffer = g_idle_stack;
    *puxIdleTaskStackSize   = configMINIMAL_STACK_SIZE;
}

void vApplicationGetTimerTaskMemory(StaticTask_t **ppxTimerTaskTCBBuffer,
                                    StackType_t **ppxTimerTaskStackBuffer,
                                    configSTACK_DEPTH_TYPE *puxTimerTaskStackSize)
{
    *ppxTimerTaskTCBBuffer   = &g_timer_tcb;
    *ppxTimerTaskStackBuffer = g_timer_stack;
    *puxTimerTaskStackSize   = configTIMER_TASK_STACK_DEPTH;
}

void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
    (void)xTask;
    __disable_irq();
    BSP_USART2_WriteStr("\r\n[FATAL] FreeRTOS Stack Overflow in task: ");
    BSP_USART2_WriteStr((pcTaskName != NULL) ? pcTaskName : "UNKNOWN");
    BSP_USART2_WriteStr("\r\n");
    BSP_DebugBreakIfAttached();
    for (;;) {
        __asm__ volatile("nop");
    }
}

void vApplicationMallocFailedHook(void)
{
    __disable_irq();
    BSP_USART2_WriteStr("\r\n[FATAL] FreeRTOS pvPortMalloc() failed!\r\n");
    BSP_DebugBreakIfAttached();
    for (;;) {
        __asm__ volatile("nop");
    }
}

void vApplicationAssertFailed(const char *file, int line)
{
    __disable_irq();
    BSP_USART2_Printf("\r\n[FATAL] configASSERT failed at %s:%d\r\n",
                      (file != NULL) ? file : "?", line);
    BSP_DebugBreakIfAttached();
    for (;;) {
        __asm__ volatile("nop");
    }
}

/*---------------------------------------------------------------------------
 * Software Timer Callback (exercises static Timer Service Task)
 *---------------------------------------------------------------------------*/
static void vHeartbeatTimerCallback(TimerHandle_t xTimer)
{
    (void)xTimer;
    g_sw_timer_ticks++;
}

/*---------------------------------------------------------------------------
 * Hardware Interrupt Handlers (Kernel-Managed Priorities >= 5)
 *---------------------------------------------------------------------------*/
void TIM2_IRQHandler(void)
{
    if ((TIM2->SR & TIM_SR_UIF) != 0U) {
        TIM2->SR = ~TIM_SR_UIF; /* Atomic rc_w0 clear (NEVER RMW &= ~ on rc_w0!) */
        g_tim2_isr_total++;

        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        if (g_consumer_handle != NULL) {
            vTaskNotifyGiveFromISR(g_consumer_handle, &xHigherPriorityTaskWoken);
        }
        __DSB();
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }
}

void EXTI15_10_IRQHandler(void)
{
    if ((EXTI->PR1 & EXTI_PR1_PIF13) != 0U) {
        EXTI->PR1 = EXTI_PR1_PIF13;
        g_btn_isr_total++;

        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        if (g_data_queue != NULL) {
            sensor_packet_t pkt = {
                .seq         = g_btn_isr_total,
                .tick_ms     = (uint32_t)xTaskGetTickCountFromISR(),
                .scaled_val  = 0.0f,
                .source      = PKT_SRC_BTN_ISR,
                .btn_pressed = 1U
            };
            (void)xQueueSendFromISR(g_data_queue, &pkt, &xHigherPriorityTaskWoken);
        }
        __DSB();
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }
}

/*---------------------------------------------------------------------------
 * Automated Self-Test Helper Tasks (Priority Inheritance & Deadlock Guard)
 *---------------------------------------------------------------------------*/
static void vPriorityInheritanceLowTask(void *pvParameters)
{
    (void)pvParameters;
    g_pip_result.base_prio = uxTaskPriorityGet(NULL);

    if (xSemaphoreTake(g_pip_mutex, pdMS_TO_TICKS(100U)) == pdTRUE) {
        /* Notify Supervisor that LowTask now owns the mutex */
        (void)xSemaphoreGive(g_pi_locked_sem);

        /* Wait until HighTask (prio 5) blocks on g_pip_mutex and promotes us */
        TickType_t start = xTaskGetTickCount();
        UBaseType_t observed = g_pip_result.base_prio;
        while ((xTaskGetTickCount() - start) < pdMS_TO_TICKS(200U)) {
            observed = uxTaskPriorityGet(NULL);
            if (observed >= PRIO_SELFTEST_HIGH) {
                break;
            }
        }
        g_pip_result.promoted_prio = observed;

        /* Releasing the mutex must disinherit us back to base_prio (1) */
        (void)xSemaphoreGive(g_pip_mutex);
        g_pip_result.restored_prio = uxTaskPriorityGet(NULL);
    }

    (void)xSemaphoreGive(g_pi_low_done_sem);
    vTaskDelete(NULL);
}

static void vPriorityInheritanceHighTask(void *pvParameters)
{
    (void)pvParameters;
    /* Blocking on g_pip_mutex held by LowTask triggers FreeRTOS Priority Inheritance */
    if (xSemaphoreTake(g_pip_mutex, pdMS_TO_TICKS(200U)) == pdTRUE) {
        g_pip_result.high_acquired = true;
        (void)xSemaphoreGive(g_pip_mutex);
    }
    (void)xSemaphoreGive(g_pi_hi_done_sem);
    vTaskDelete(NULL);
}

static void vDeadlockHolderTask(void *pvParameters)
{
    (void)pvParameters;
    /* Hold Mutex B temporarily while Supervisor attempts bounded acquisition */
    if (xSemaphoreTake(g_dl_mutex_b, pdMS_TO_TICKS(100U)) == pdTRUE) {
        (void)xSemaphoreGive(g_dl_locked_sem);
        vTaskDelay(pdMS_TO_TICKS(40U));
        (void)xSemaphoreGive(g_dl_mutex_b);
    }
    (void)xSemaphoreGive(g_dl_done_sem);
    vTaskDelete(NULL);
}

/*---------------------------------------------------------------------------
 * Self-Test Suite (Executed Once by SupervisorTask Before Entering Watchdog Loop)
 *---------------------------------------------------------------------------*/
static uint32_t run_freertos_selftests(void)
{
    uint32_t passed = 0U;

    /* Test 1: Verify NVIC PRIGROUP, SHPR3 PendSV/SysTick lowest priority, TIM2 >= 5 */
    {
        uint32_t prigroup     = NVIC_GetPriorityGrouping();
        uint32_t shpr3        = SCB->SHP[10]; /* PendSV is SHP[10] (index 10 = PendSV, 11 = SysTick) */
        uint32_t systick_prio = SCB->SHP[11];
        uint32_t tim2_prio    = NVIC_GetPriority(TIM2_IRQn);
        bool ok = (prigroup == 0U) &&
                  (shpr3 == (uint32_t)configKERNEL_INTERRUPT_PRIORITY) &&
                  (systick_prio == (uint32_t)configKERNEL_INTERRUPT_PRIORITY) &&
                  (tim2_prio >= configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY);

        if (xSemaphoreTake(g_uart_mutex, pdMS_TO_TICKS(100U)) == pdTRUE) {
            BSP_USART2_Printf("[%s] NVIC_CONFIG: PRIGROUP=%lu PendSV=0x%02lX SysTick=0x%02lX TIM2_PRIO=%lu\r\n",
                              ok ? "PASS" : "FAIL",
                              (unsigned long)prigroup,
                              (unsigned long)shpr3,
                              (unsigned long)systick_prio,
                              (unsigned long)tim2_prio);
            (void)xSemaphoreGive(g_uart_mutex);
        }
        if (ok) {
            passed++;
        }
    }

    /* Test 2: Binary & Counting Semaphores */
    {
        bool bin_ok = (xSemaphoreTake(g_bin_sem, 0U) == pdFALSE) &&
                      (xSemaphoreGive(g_bin_sem) == pdTRUE) &&
                      (xSemaphoreTake(g_bin_sem, 0U) == pdTRUE) &&
                      (xSemaphoreTake(g_bin_sem, 0U) == pdFALSE);

        bool cnt_ok = (xSemaphoreGive(g_cnt_sem) == pdTRUE) &&
                      (xSemaphoreGive(g_cnt_sem) == pdTRUE) &&
                      (xSemaphoreGive(g_cnt_sem) == pdTRUE) &&
                      (xSemaphoreGive(g_cnt_sem) == pdFALSE) && /* max count 3 reached */
                      (uxSemaphoreGetCount(g_cnt_sem) == 3U) &&
                      (xSemaphoreTake(g_cnt_sem, 0U) == pdTRUE) &&
                      (xSemaphoreTake(g_cnt_sem, 0U) == pdTRUE) &&
                      (xSemaphoreTake(g_cnt_sem, 0U) == pdTRUE) &&
                      (uxSemaphoreGetCount(g_cnt_sem) == 0U);

        bool ok = bin_ok && cnt_ok;
        if (xSemaphoreTake(g_uart_mutex, pdMS_TO_TICKS(100U)) == pdTRUE) {
            BSP_USART2_Printf("[%s] SEMAPHORES: binary=%s counting_max3=%s\r\n",
                              ok ? "PASS" : "FAIL",
                              bin_ok ? "OK" : "ERR",
                              cnt_ok ? "OK" : "ERR");
            (void)xSemaphoreGive(g_uart_mutex);
        }
        if (ok) {
            passed++;
        }
    }

    /* Test 3: Mutex Priority Inheritance Protocol (PIP) */
    {
        g_pip_result.base_prio     = 0U;
        g_pip_result.promoted_prio = 0U;
        g_pip_result.restored_prio = 0U;
        g_pip_result.high_acquired = false;

        TaskHandle_t hLow = xTaskCreateStatic(
            vPriorityInheritanceLowTask, "PI_Low", STACK_HELPER, NULL,
            PRIO_MONITOR, g_helper1_stack, &g_helper1_tcb);
        configASSERT(hLow != NULL);

        /* Block so PI_Low (prio 1) runs and acquires g_pip_mutex */
        BaseType_t locked = xSemaphoreTake(g_pi_locked_sem, pdMS_TO_TICKS(200U));

        /* Spawn PI_High (prio 5 > Supervisor prio 4) to contend for g_pip_mutex */
        TaskHandle_t hHigh = xTaskCreateStatic(
            vPriorityInheritanceHighTask, "PI_High", STACK_HELPER, NULL,
            PRIO_SELFTEST_HIGH, g_helper2_stack, &g_helper2_tcb);
        configASSERT(hHigh != NULL);

        BaseType_t hi_done  = xSemaphoreTake(g_pi_hi_done_sem, pdMS_TO_TICKS(300U));
        BaseType_t low_done = xSemaphoreTake(g_pi_low_done_sem, pdMS_TO_TICKS(300U));

        /* Allow Idle task a tick to clean up deleted static task list items */
        vTaskDelay(pdMS_TO_TICKS(10U));

        bool ok = (locked == pdTRUE) && (hi_done == pdTRUE) && (low_done == pdTRUE) &&
                  (g_pip_result.base_prio == PRIO_MONITOR) &&
                  (g_pip_result.promoted_prio == PRIO_SELFTEST_HIGH) &&
                  (g_pip_result.restored_prio == PRIO_MONITOR) &&
                  g_pip_result.high_acquired;

        if (xSemaphoreTake(g_uart_mutex, pdMS_TO_TICKS(100U)) == pdTRUE) {
            BSP_USART2_Printf("[%s] PRIORITY_INHERITANCE: base_prio=%lu promoted_prio=%lu restored_prio=%lu high_acq=%u\r\n",
                              ok ? "PASS" : "FAIL",
                              (unsigned long)g_pip_result.base_prio,
                              (unsigned long)g_pip_result.promoted_prio,
                              (unsigned long)g_pip_result.restored_prio,
                              g_pip_result.high_acquired ? 1U : 0U);
            (void)xSemaphoreGive(g_uart_mutex);
        }
        if (ok) {
            passed++;
        }
    }

    /* Test 4: Deadlock Avoidance (Bounded Timeout Recovery + Strict Lock Order A -> B) */
    {
        TaskHandle_t hHolder = xTaskCreateStatic(
            vDeadlockHolderTask, "DL_Hold", STACK_HELPER, NULL,
            PRIO_SELFTEST_HIGH, g_helper1_stack, &g_helper1_tcb);
        configASSERT(hHolder != NULL);

        BaseType_t holder_ready = xSemaphoreTake(g_dl_locked_sem, pdMS_TO_TICKS(100U));

        bool timeout_recovered = false;
        if (xSemaphoreTake(g_dl_mutex_a, pdMS_TO_TICKS(50U)) == pdTRUE) {
            /* Attempt bounded lock of B while holder owns B -> must time out cleanly */
            if (xSemaphoreTake(g_dl_mutex_b, pdMS_TO_TICKS(15U)) == pdFALSE) {
                timeout_recovered = true;
            } else {
                (void)xSemaphoreGive(g_dl_mutex_b);
            }
            /* Release A immediately to break Hold-and-Wait */
            (void)xSemaphoreGive(g_dl_mutex_a);
        }

        BaseType_t holder_done = xSemaphoreTake(g_dl_done_sem, pdMS_TO_TICKS(200U));
        vTaskDelay(pdMS_TO_TICKS(10U));

        /* Now acquire both mutexes in strict global lock order (A then B) */
        bool ordered_ok = false;
        if (xSemaphoreTake(g_dl_mutex_a, pdMS_TO_TICKS(50U)) == pdTRUE) {
            if (xSemaphoreTake(g_dl_mutex_b, pdMS_TO_TICKS(50U)) == pdTRUE) {
                ordered_ok = true;
                (void)xSemaphoreGive(g_dl_mutex_b);
            }
            (void)xSemaphoreGive(g_dl_mutex_a);
        }

        bool ok = (holder_ready == pdTRUE) && (holder_done == pdTRUE) &&
                  timeout_recovered && ordered_ok;

        if (xSemaphoreTake(g_uart_mutex, pdMS_TO_TICKS(100U)) == pdTRUE) {
            BSP_USART2_Printf("[%s] DEADLOCK_GUARD: timeout_recovered=%s ordered_lock(A->B)=%s\r\n",
                              ok ? "PASS" : "FAIL",
                              timeout_recovered ? "YES" : "NO",
                              ordered_ok ? "OK" : "ERR");
            (void)xSemaphoreGive(g_uart_mutex);
        }
        if (ok) {
            passed++;
        }
    }

    /* Test 5: TIM2 10 Hz FromISR Direct Notification + Producer/Consumer Queue IPC + FPU */
    {
        BSP_TIM2_Start();
        vTaskDelay(pdMS_TO_TICKS(350U));

        uint32_t isr_cnt = g_tim2_isr_total;
        uint32_t ntf_cnt = g_tim2_notify_total;
        uint32_t tx_cnt  = g_queue_tx_total;
        uint32_t rx_cnt  = g_queue_rx_total;
        uint32_t sw_cnt  = g_sw_timer_ticks;

        bool ok = (isr_cnt >= 2U) && (ntf_cnt >= 2U) &&
                  (tx_cnt >= 2U) && (rx_cnt >= 2U) &&
                  (g_seq_errors == 0U) && (g_fpu_errors == 0U) &&
                  (sw_cnt >= 1U);

        if (xSemaphoreTake(g_uart_mutex, pdMS_TO_TICKS(100U)) == pdTRUE) {
            BSP_USART2_Printf("[%s] FROM_ISR_AND_QUEUE: tim2_isr=%lu notify=%lu q_tx=%lu q_rx=%lu sw_tmr=%lu fpu_err=%lu\r\n",
                              ok ? "PASS" : "FAIL",
                              (unsigned long)isr_cnt,
                              (unsigned long)ntf_cnt,
                              (unsigned long)tx_cnt,
                              (unsigned long)rx_cnt,
                              (unsigned long)sw_cnt,
                              (unsigned long)g_fpu_errors);
            (void)xSemaphoreGive(g_uart_mutex);
        }
        if (ok) {
            passed++;
        }
    }

    /* Test 6: Stack High-Water Mark Margin Across All Persistent Tasks */
    {
        UBaseType_t hwm_sup  = uxTaskGetStackHighWaterMark(g_supervisor_handle);
        UBaseType_t hwm_cons = uxTaskGetStackHighWaterMark(g_consumer_handle);
        UBaseType_t hwm_prod = uxTaskGetStackHighWaterMark(g_producer_handle);
        UBaseType_t hwm_mon  = uxTaskGetStackHighWaterMark(g_monitor_handle);

        bool ok = (hwm_sup >= 64U) && (hwm_cons >= 64U) &&
                  (hwm_prod >= 64U) && (hwm_mon >= 64U);

        if (xSemaphoreTake(g_uart_mutex, pdMS_TO_TICKS(100U)) == pdTRUE) {
            BSP_USART2_Printf("[%s] STACK_HWM: Sup=%lu Cons=%lu Prod=%lu Mon=%lu words_free (heap_free=%lu B)\r\n",
                              ok ? "PASS" : "FAIL",
                              (unsigned long)hwm_sup,
                              (unsigned long)hwm_cons,
                              (unsigned long)hwm_prod,
                              (unsigned long)hwm_mon,
                              (unsigned long)xPortGetFreeHeapSize());
            (void)xSemaphoreGive(g_uart_mutex);
        }
        if (ok) {
            passed++;
        }
    }

    return passed;
}

/*---------------------------------------------------------------------------
 * Persistent RTOS Application Tasks
 *---------------------------------------------------------------------------*/

/* Priority 4 (Static): Runs boot self-test suite, then supervises IWDG & PA5 LED */
static void vSupervisorTask(void *pvParameters)
{
    (void)pvParameters;

    uint32_t passed = run_freertos_selftests();
    if (xSemaphoreTake(g_uart_mutex, pdMS_TO_TICKS(100U)) == pdTRUE) {
        BSP_USART2_Printf("[SUMMARY] FREERTOS_SELF_TESTS: %lu/6 %s\r\n",
                          (unsigned long)passed,
                          (passed == 6U) ? "ALL_PASSED" : "CHECK_FAILURES");
        (void)xSemaphoreGive(g_uart_mutex);
    }

    /* Start hardware IWDG (~3.0 s timeout, DBGMCU frozen on debug halt) */
    bool iwdg_ok = BSP_IWDG_Init_3000ms();
    if (xSemaphoreTake(g_uart_mutex, pdMS_TO_TICKS(100U)) == pdTRUE) {
        BSP_USART2_Printf("[%s] IWDG_SUPERVISOR: enabled (~3000 ms, DBG_IWDG_STOP=1)\r\n",
                          iwdg_ok ? "PASS" : "FAIL");
        (void)xSemaphoreGive(g_uart_mutex);
    }

    g_selftest_complete = true;

    for (;;) {
        /* Refresh IWDG ONLY when Producer, Consumer, and Monitor ALL check in! */
        EventBits_t bits = xEventGroupWaitBits(
            g_wdg_events,
            EVT_ALL_WORKERS,
            pdTRUE,  /* clear bits on exit */
            pdTRUE,  /* wait for ALL worker bits */
            pdMS_TO_TICKS(1500U));

        if ((bits & EVT_ALL_WORKERS) == EVT_ALL_WORKERS) {
            BSP_IWDG_Refresh();
            g_wdg_feeds++;
            BSP_LED_Toggle();
        } else {
            uart_log_str("[WARN] Watchdog supervisor missed worker check-in; IWDG NOT fed!\r\n");
        }
    }
}

/* Priority 3 (Static): Woken directly by TIM2_IRQHandler FromISR notification; drains queue */
static void vConsumerTask(void *pvParameters)
{
    (void)pvParameters;
    uint32_t last_prod_seq = 0U;
    sensor_packet_t pkt;

    for (;;) {
        /* Direct-to-task notification from TIM2 10 Hz ISR (zero RAM, immediate PendSV) */
        uint32_t notified = ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(150U));
        if (notified > 0U) {
            g_tim2_notify_total += notified;
        }

        /* Drain all queued items from ProducerTask and EXTI13 Button ISR */
        while (xQueueReceive(g_data_queue, &pkt, 0U) == pdTRUE) {
            g_queue_rx_total++;
            if (pkt.source == PKT_SRC_PRODUCER) {
                if ((last_prod_seq != 0U) && (pkt.seq != (last_prod_seq + 1U))) {
                    g_seq_errors++;
                }
                last_prod_seq = pkt.seq;

                /* Verify hardware FPU register preservation across context switches */
                float expected = (float)(pkt.seq & 0xFFU) * 0.25f;
                float diff = pkt.scaled_val - expected;
                if ((diff > 0.001f) || (diff < -0.001f)) {
                    g_fpu_errors++;
                }
            }
        }

        (void)xEventGroupSetBits(g_wdg_events, EVT_BIT_CONSUMER);
    }
}

/* Priority 2 (Static): Drift-free 10 Hz periodic producer using vTaskDelayUntil + FPU */
static void vProducerTask(void *pvParameters)
{
    (void)pvParameters;
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xPeriod = pdMS_TO_TICKS(100U);
    uint32_t seq = 0U;

    for (;;) {
        seq++;
        sensor_packet_t pkt = {
            .seq         = seq,
            .tick_ms     = (uint32_t)xTaskGetTickCount(),
            .scaled_val  = (float)(seq & 0xFFU) * 0.25f,
            .source      = PKT_SRC_PRODUCER,
            .btn_pressed = BSP_Button_Read() ? 1U : 0U
        };

        if (xQueueSend(g_data_queue, &pkt, pdMS_TO_TICKS(20U)) == pdTRUE) {
            g_queue_tx_total++;
        }

        (void)xEventGroupSetBits(g_wdg_events, EVT_BIT_PRODUCER);
        vTaskDelayUntil(&xLastWakeTime, xPeriod);
    }
}

/* Priority 1 (Dynamic): 1 Hz structured UART telemetry & stack/heap monitor */
static void vMonitorTask(void *pvParameters)
{
    (void)pvParameters;
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xPeriod = pdMS_TO_TICKS(1000U);

    for (;;) {
        (void)xEventGroupSetBits(g_wdg_events, EVT_BIT_MONITOR);

        if (g_selftest_complete) {
            UBaseType_t hwm_sup  = uxTaskGetStackHighWaterMark(g_supervisor_handle);
            UBaseType_t hwm_cons = uxTaskGetStackHighWaterMark(g_consumer_handle);
            UBaseType_t hwm_prod = uxTaskGetStackHighWaterMark(g_producer_handle);
            UBaseType_t hwm_mon  = uxTaskGetStackHighWaterMark(NULL);

            if (xSemaphoreTake(g_uart_mutex, pdMS_TO_TICKS(100U)) == pdTRUE) {
                BSP_USART2_Printf(
                    "[TELEM t=%lu ms] q_tx=%lu q_rx=%lu tim2=%lu btn=%lu wdg=%lu heap=%lu/%lu HWM(S=%lu C=%lu P=%lu M=%lu)\r\n",
                    (unsigned long)xTaskGetTickCount(),
                    (unsigned long)g_queue_tx_total,
                    (unsigned long)g_queue_rx_total,
                    (unsigned long)g_tim2_notify_total,
                    (unsigned long)g_btn_isr_total,
                    (unsigned long)g_wdg_feeds,
                    (unsigned long)xPortGetFreeHeapSize(),
                    (unsigned long)xPortGetMinimumEverFreeHeapSize(),
                    (unsigned long)hwm_sup,
                    (unsigned long)hwm_cons,
                    (unsigned long)hwm_prod,
                    (unsigned long)hwm_mon);
                (void)xSemaphoreGive(g_uart_mutex);
            }
        }

        vTaskDelayUntil(&xLastWakeTime, xPeriod);
    }
}

/*---------------------------------------------------------------------------
 * Application Entry Point
 *---------------------------------------------------------------------------*/
int main(void)
{
    /* 1. MANDATORY BEFORE FREERTOS: 4 preemption bits, 0 subpriority bits */
    NVIC_SetPriorityGrouping(0U);

    /* 2. Bring up 170 MHz Boost Mode clock and onboard NUCLEO-G474RE peripherals */
    bool clk_ok = BSP_Clock_Init_170MHz();
    uint32_t reset_flags = BSP_GetAndClearResetFlags();

    BSP_LED_Init();
    BSP_USART2_Init_115200();

    BSP_USART2_WriteStr("\r\n============================================================\r\n");
    BSP_USART2_Printf("NUCLEO-G474RE FreeRTOS %s Onboard Verification\r\n", tskKERNEL_VERSION_NUMBER);
    BSP_USART2_Printf("SYSCLK=%lu Hz (clk=%s) | PRIGROUP=%lu | MAX_SYSCALL_PRIO=%u\r\n",
                      (unsigned long)SystemCoreClock,
                      clk_ok ? "OK" : "FALLBACK",
                      (unsigned long)NVIC_GetPriorityGrouping(),
                      (unsigned)configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY);
    BSP_USART2_Printf("RCC_CSR=0x%08lX (IWDGRST=%u) | Port=GCC/ARM_CM4F | Heap=heap_4\r\n",
                      (unsigned long)reset_flags,
                      ((reset_flags & RCC_CSR_IWDGRSTF) != 0U) ? 1U : 0U);
    BSP_USART2_WriteStr("============================================================\r\n");

    /* 3. Create IPC primitives with strict verification */
    g_data_queue      = xQueueCreate(16U, sizeof(sensor_packet_t));
    g_uart_mutex      = xSemaphoreCreateMutex();
    g_bin_sem         = xSemaphoreCreateBinary();
    g_cnt_sem         = xSemaphoreCreateCounting(3U, 0U);
    g_pip_mutex       = xSemaphoreCreateMutex();
    g_dl_mutex_a      = xSemaphoreCreateMutex();
    g_dl_mutex_b      = xSemaphoreCreateMutex();
    g_wdg_events      = xEventGroupCreate();
    g_pi_locked_sem   = xSemaphoreCreateBinary();
    g_pi_low_done_sem = xSemaphoreCreateBinary();
    g_pi_hi_done_sem  = xSemaphoreCreateBinary();
    g_dl_locked_sem   = xSemaphoreCreateBinary();
    g_dl_done_sem     = xSemaphoreCreateBinary();
    g_sw_timer        = xTimerCreate("SwHB", pdMS_TO_TICKS(100U), pdTRUE, NULL, vHeartbeatTimerCallback);

    configASSERT(g_data_queue      != NULL);
    configASSERT(g_uart_mutex      != NULL);
    configASSERT(g_bin_sem         != NULL);
    configASSERT(g_cnt_sem         != NULL);
    configASSERT(g_pip_mutex       != NULL);
    configASSERT(g_dl_mutex_a      != NULL);
    configASSERT(g_dl_mutex_b      != NULL);
    configASSERT(g_wdg_events      != NULL);
    configASSERT(g_pi_locked_sem   != NULL);
    configASSERT(g_pi_low_done_sem != NULL);
    configASSERT(g_pi_hi_done_sem  != NULL);
    configASSERT(g_dl_locked_sem   != NULL);
    configASSERT(g_dl_done_sem     != NULL);
    configASSERT(g_sw_timer        != NULL);

    configASSERT(xTimerStart(g_sw_timer, 0U) == pdPASS);

    /* 4. Create the 4 primary tasks (3 static zero-heap + 1 dynamic heap_4) */
    g_supervisor_handle = xTaskCreateStatic(
        vSupervisorTask, "Supervisor", STACK_SUPERVISOR, NULL,
        PRIO_SUPERVISOR, g_supervisor_stack, &g_supervisor_tcb);
    configASSERT(g_supervisor_handle != NULL);

    g_consumer_handle = xTaskCreateStatic(
        vConsumerTask, "Consumer", STACK_CONSUMER, NULL,
        PRIO_CONSUMER, g_consumer_stack, &g_consumer_tcb);
    configASSERT(g_consumer_handle != NULL);

    g_producer_handle = xTaskCreateStatic(
        vProducerTask, "Producer", STACK_PRODUCER, NULL,
        PRIO_PRODUCER, g_producer_stack, &g_producer_tcb);
    configASSERT(g_producer_handle != NULL);

    BaseType_t mon_ok = xTaskCreate(
        vMonitorTask, "Monitor", STACK_MONITOR, NULL,
        PRIO_MONITOR, &g_monitor_handle);
    configASSERT((mon_ok == pdPASS) && (g_monitor_handle != NULL));

    /* 5. Configure kernel-managed peripheral interrupts (priority >= 5) */
    BSP_TIM2_Init_10Hz(IRQ_PRIO_TIM2);
    BSP_Button_Init(IRQ_PRIO_EXTI13);

    /* 6. Start FreeRTOS preemptive scheduler (invokes SVC_Handler -> vPortSVCHandler) */
    vTaskStartScheduler();

    /* Unreachable unless scheduler failed to start */
    for (;;) {
        BSP_DebugBreakIfAttached();
    }
}
