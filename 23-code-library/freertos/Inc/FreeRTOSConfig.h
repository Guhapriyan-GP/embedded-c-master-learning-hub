/**
 * @file    FreeRTOSConfig.h
 * @brief   Production FreeRTOS V11.1.0 configuration for STM32G474RET6
 *          (NUCLEO-G474RE, ARM Cortex-M4F, 170 MHz Boost Mode, 96 KB SRAM1+2).
 *
 * Verified against:
 *   - 23-code-library/Drivers/CMSIS/Device/ST/STM32G4xx/Include/stm32g474xx.h
 *     (__NVIC_PRIO_BITS == 4U)
 *   - FreeRTOS-Kernel V11.1.0 portable/GCC/ARM_CM4F/port.c
 *   - ARM PM0214 (Cortex-M4 NVIC Priority Grouping & BASEPRI)
 */

#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

#if defined(__ICCARM__) || defined(__CC_ARM) || defined(__GNUC__)
    #include <stdint.h>
    extern uint32_t SystemCoreClock;
    extern void vApplicationAssertFailed(const char *file, int line);
#endif

/*---------------------------------------------------------------------------
 * Core Scheduler & Timing Configuration
 *---------------------------------------------------------------------------*/
#define configUSE_PREEMPTION                        1
#define configUSE_PORT_OPTIMISED_TASK_SELECTION     1
#define configUSE_TICKLESS_IDLE                     0
#define configCPU_CLOCK_HZ                          (SystemCoreClock)
#define configTICK_RATE_HZ                          ((TickType_t)1000U)
#define configMAX_PRIORITIES                        (8U)
#define configMINIMAL_STACK_SIZE                    ((configSTACK_DEPTH_TYPE)128U)
#define configSTACK_DEPTH_TYPE                      uint32_t
#define configMAX_TASK_NAME_LEN                     (16U)
#define configTICK_TYPE_WIDTH_IN_BITS               TICK_TYPE_WIDTH_32_BITS
#define configIDLE_SHOULD_YIELD                     1

/*---------------------------------------------------------------------------
 * Memory Allocation & Safety Diagnostics
 *---------------------------------------------------------------------------*/
#define configSUPPORT_STATIC_ALLOCATION             1
#define configSUPPORT_DYNAMIC_ALLOCATION            1
#define configTOTAL_HEAP_SIZE                       ((size_t)(24U * 1024U))
#define configAPPLICATION_ALLOCATED_HEAP            0
#define configCHECK_FOR_STACK_OVERFLOW              2
#define configUSE_MALLOC_FAILED_HOOK                1
#define configUSE_IDLE_HOOK                         0
#define configUSE_TICK_HOOK                         0
#define configRECORD_STACK_HIGH_ADDRESS             1

/*---------------------------------------------------------------------------
 * IPC, Synchronization & Software Timers
 *---------------------------------------------------------------------------*/
#define configUSE_TASK_NOTIFICATIONS                1
#define configTASK_NOTIFICATION_ARRAY_ENTRIES       2
#define configUSE_MUTEXES                           1
#define configUSE_RECURSIVE_MUTEXES                 0
#define configUSE_COUNTING_SEMAPHORES               1
#define configQUEUE_REGISTRY_SIZE                   8
#define configUSE_QUEUE_SETS                        0
#define configUSE_TIME_SLICING                      1
#define configUSE_NEWLIB_REENTRANT                  0
#define configENABLE_BACKWARD_COMPATIBILITY         0
#define configNUM_THREAD_LOCAL_STORAGE_POINTERS     0

#define configUSE_TIMERS                            1
#define configTIMER_TASK_PRIORITY                   (3U)
#define configTIMER_QUEUE_LENGTH                    (8U)
#define configTIMER_TASK_STACK_DEPTH                (256U)

/*---------------------------------------------------------------------------
 * Optional API Function Inclusions
 *---------------------------------------------------------------------------*/
#define INCLUDE_vTaskPrioritySet                    1
#define INCLUDE_uxTaskPriorityGet                   1
#define INCLUDE_vTaskDelete                         1
#define INCLUDE_vTaskSuspend                        1
#define INCLUDE_xResumeFromISR                      0
#define INCLUDE_vTaskDelayUntil                     1
#define INCLUDE_vTaskDelay                          1
#define INCLUDE_xTaskGetSchedulerState              1
#define INCLUDE_xTaskGetCurrentTaskHandle           1
#define INCLUDE_uxTaskGetStackHighWaterMark         1
#define INCLUDE_xTaskGetIdleTaskHandle              1
#define INCLUDE_eTaskGetState                       1
#define INCLUDE_xTimerPendFunctionCall              1
#define INCLUDE_xTaskAbortDelay                     0
#define INCLUDE_xTaskGetHandle                      0

/*---------------------------------------------------------------------------
 * ARM Cortex-M4F NVIC Interrupt Priority Configuration (STM32G474RE)
 * __NVIC_PRIO_BITS = 4U -> 16 priority levels (0 = highest, 15 = lowest)
 *
 * MANDATORY: Application MUST call NVIC_SetPriorityGrouping(0U) before
 *            vTaskStartScheduler() (4 preemption bits, 0 subpriority bits).
 *
 * Priorities 0..4  : Zero-latency unmanaged ISRs (NEVER call FreeRTOS APIs!)
 * Priorities 5..15 : Kernel-managed ISRs (safe for ...FromISR() APIs)
 *---------------------------------------------------------------------------*/
#ifdef __NVIC_PRIO_BITS
    #define configPRIO_BITS                         __NVIC_PRIO_BITS
#else
    #define configPRIO_BITS                         4U
#endif

#define configLIBRARY_LOWEST_INTERRUPT_PRIORITY         15U
#define configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY    5U

#define configKERNEL_INTERRUPT_PRIORITY \
    (configLIBRARY_LOWEST_INTERRUPT_PRIORITY << (8U - configPRIO_BITS))

/* Must NEVER be zero! Value 5 << 4 = 0x50 written to BASEPRI. */
#define configMAX_SYSCALL_INTERRUPT_PRIORITY \
    (configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY << (8U - configPRIO_BITS))

/*---------------------------------------------------------------------------
 * Assertion Trap
 *---------------------------------------------------------------------------*/
#define configASSERT(x) \
    do { \
        if ((x) == 0) { \
            vApplicationAssertFailed(__FILE__, __LINE__); \
        } \
    } while (0)

/*---------------------------------------------------------------------------
 * Map FreeRTOS Port Exception Handlers Directly onto CMSIS Startup Vectors
 * (23-code-library/templates/Src/startup_stm32g474re.c)
 *---------------------------------------------------------------------------*/
#define vPortSVCHandler     SVC_Handler
#define xPortPendSVHandler  PendSV_Handler
#define xPortSysTickHandler SysTick_Handler

#endif /* FREERTOS_CONFIG_H */
