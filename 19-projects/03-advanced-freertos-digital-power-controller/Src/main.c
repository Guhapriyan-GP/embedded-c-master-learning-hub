/**
 * @file    main.c
 * @brief   Project 03 — FreeRTOS Digital Power & Motor Controller with
 *          Asynchronous Hardware Overcurrent Break (< 50 ns) (Level 3 Capstone).
 *
 * Links official pinned FreeRTOS-Kernel V11.1.0 (ARM_CM4F + heap_4.c).
 *
 * Safety-First STM32G474RE Architecture:
 *   1. Clock & NVIC: 170 MHz Boost Mode + mandatory NVIC_SetPriorityGrouping(0U).
 *   2. Complementary PWM: TIM1_CH1 (PA8, AF6) & TIM1_CH1N (PB13, AF6) @ 50.000 kHz
 *      center-aligned with 500.0 ns hardware dead-time (BDTR.DTG = 85).
 *   3. Asynchronous < 50 ns Overcurrent Break:
 *      DAC1_CH1 (PA4, HFSEL=10b) -> COMP1_INM (INMSEL=5), PA1 -> COMP1_INP,
 *      COMP1_OUT -> TIM1->AF1.BKCMP1E (BDTR.BKE=1, BKP=1, AOE=0 latched safe OFF!).
 *      Post-fault TIM1_BRK_TIM15_IRQHandler (NVIC Prio 5) notifies SafetySupervisorTask
 *      via xTaskNotifyFromISR() + __DSB() + portYIELD_FROM_ISR().
 *   4. 4 Properly Prioritized FreeRTOS Tasks (Static TCBs/Stacks, ZERO prio-0 user tasks):
 *      - SafetySupervisorTask (Prio 4, 256 words): Latched fault manager & IWDG feed
 *        (feeds IWDG ONLY when Ctrl + Telem + UI heartbeats are all healthy!)
 *      - ControlLoopTask      (Prio 3, 384 words): 1 kHz fixed-point Q15 PI regulator
 *        with anti-windup clamping
 *      - TelemetryTask        (Prio 2, 512 words): Mutex-guarded USART2 JSON telemetry
 *        including live stack high-water marks
 *      - UiTask               (Prio 1, 384 words): 10 Hz B1 button setpoint/fault-reset/
 *        hardware-break-test & optional I2C1 OLED (PA15/PB7, EXTERNAL HARDWARE PENDING)
 *
 * Default POWER_SIM_MODE=1 runs safely onboard NUCLEO-G474RE with ZERO external wires!
 */

#include "power_ctrl.h"
#include "stm32g474xx.h"

#include "FreeRTOS.h"
#include "event_groups.h"
#include "queue.h"
#include "semphr.h"
#include "task.h"

#include "adc_g4.h"
#include "clock_g4.h"
#include "fault_handler.h"
#include "gpio_g4.h"
#include "i2c_g4.h"
#include "iwdg_g4.h"
#include "usart_g4.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

/* --------------------------------------------------------------------------
 * Static FreeRTOS Kernel & Application Allocations
 * -------------------------------------------------------------------------- */
static StaticTask_t g_idle_tcb;
static StackType_t  g_idle_stack[configMINIMAL_STACK_SIZE];

static StaticTask_t g_timer_tcb;
static StackType_t  g_timer_stack[configTIMER_TASK_STACK_DEPTH];

static StaticTask_t g_safety_tcb;
static StackType_t  g_safety_stack[PWR_STACK_SAFETY];

static StaticTask_t g_ctrl_tcb;
static StackType_t  g_ctrl_stack[PWR_STACK_CONTROL];

static StaticTask_t g_telem_tcb;
static StackType_t  g_telem_stack[PWR_STACK_TELEMETRY];

static StaticTask_t g_ui_tcb;
static StackType_t  g_ui_stack[PWR_STACK_UI];

static StaticQueue_t g_telem_queue_cb;
static uint8_t       g_telem_queue_buf[4U * sizeof(power_telem_msg_t)];

static StaticSemaphore_t  g_uart_mutex_cb;
static StaticEventGroup_t g_wdg_events_cb;

static TaskHandle_t       g_safety_handle = NULL;
static TaskHandle_t       g_ctrl_handle   = NULL;
static TaskHandle_t       g_telem_handle  = NULL;
static TaskHandle_t       g_ui_handle     = NULL;
static QueueHandle_t      g_telem_queue   = NULL;
static SemaphoreHandle_t  g_uart_mutex    = NULL;
static EventGroupHandle_t g_wdg_events    = NULL;

/* --------------------------------------------------------------------------
 * Shared Runtime State (Atomic / Volatile ISR-to-Task Boundary)
 * -------------------------------------------------------------------------- */
static volatile uint32_t g_hw_break_count = 0U;
static volatile uint32_t g_fault_latched  = 0U;
static volatile uint32_t g_wdg_feeds      = 0U;
static volatile int32_t  g_v_ref_mv       = 5000; /* Default 5.000 V target */
static volatile bool     g_oled_present   = false;

static const int32_t k_setpoints_mv[3] = { 3300, 5000, 12000 };

/* --------------------------------------------------------------------------
 * Thread-Safe UART Logging Helper (Priority-Inheritance Mutex)
 * -------------------------------------------------------------------------- */
static void pwr_uart_log(const char *msg) {
    if ((g_uart_mutex != NULL) && (xTaskGetSchedulerState() == taskSCHEDULER_RUNNING)) {
        if (xSemaphoreTake(g_uart_mutex, pdMS_TO_TICKS(100U)) == pdTRUE) {
            (void)usart_write_str(USART2, msg, PWR_UART_TX_TIMEOUT);
            (void)xSemaphoreGive(g_uart_mutex);
        }
    } else {
        (void)usart_write_str(USART2, msg, PWR_UART_TX_TIMEOUT);
    }
}

/* --------------------------------------------------------------------------
 * Mandatory FreeRTOS Static Allocation & Safety Hooks
 * -------------------------------------------------------------------------- */
void vApplicationGetIdleTaskMemory(StaticTask_t **ppxIdleTaskTCBBuffer,
                                   StackType_t **ppxIdleTaskStackBuffer,
                                   configSTACK_DEPTH_TYPE *puxIdleTaskStackSize) {
    *ppxIdleTaskTCBBuffer   = &g_idle_tcb;
    *ppxIdleTaskStackBuffer = g_idle_stack;
    *puxIdleTaskStackSize   = configMINIMAL_STACK_SIZE;
}

void vApplicationGetTimerTaskMemory(StaticTask_t **ppxTimerTaskTCBBuffer,
                                    StackType_t **ppxTimerTaskStackBuffer,
                                    configSTACK_DEPTH_TYPE *puxTimerTaskStackSize) {
    *ppxTimerTaskTCBBuffer   = &g_timer_tcb;
    *ppxTimerTaskStackBuffer = g_timer_stack;
    *puxTimerTaskStackSize   = configTIMER_TASK_STACK_DEPTH;
}

static void pwr_force_safe_off_and_halt(const char *reason) {
    __disable_irq();
    TIM1->BDTR &= ~TIM_BDTR_MOE; /* Immediately de-energize power stage */
    GPIO_SetPin(GPIOA, 5U);      /* Illuminate LD2 fault indicator */
    (void)usart_write_str(USART2, "\r\n[FATAL] ", PWR_UART_TX_TIMEOUT);
    (void)usart_write_str(USART2, reason, PWR_UART_TX_TIMEOUT);
    (void)usart_write_str(USART2, "\r\n", PWR_UART_TX_TIMEOUT);
    while (1) {
        __NOP();
    }
}

void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName) {
    (void)xTask;
    pwr_force_safe_off_and_halt((pcTaskName != NULL) ? pcTaskName : "STACK_OVERFLOW");
}

void vApplicationMallocFailedHook(void) {
    pwr_force_safe_off_and_halt("FREERTOS_MALLOC_FAILED");
}

void vApplicationAssertFailed(const char *file, int line) {
    (void)line;
    pwr_force_safe_off_and_halt((file != NULL) ? file : "CONFIG_ASSERT_FAILED");
}

/* --------------------------------------------------------------------------
 * Asynchronous Hardware Break Post-Fault ISR (NVIC Priority 5 >= MAX_SYSCALL)
 * NOTE: TIM1->BDTR.MOE was ALREADY cleared in silicon in < 50 ns before this
 *       ISR even entered! This ISR only latches state and notifies Task 3.
 * -------------------------------------------------------------------------- */
void TIM1_BRK_TIM15_IRQHandler(void) {
    if ((TIM1->SR & TIM_SR_BIF) != 0U) {
        TIM1->SR = ~TIM_SR_BIF; /* Atomic rc_w0 clear (NEVER |= or &= ~) */
        g_hw_break_count++;
        g_fault_latched = 1U;
        GPIO_SetPin(GPIOA, 5U); /* Immediate visual fault latch on LD2 */

        BaseType_t woken = pdFALSE;
        if (g_safety_handle != NULL) {
            (void)xTaskNotifyFromISR(g_safety_handle,
                                     PWR_NOTIFY_HW_BREAK,
                                     eSetBits,
                                     &woken);
        }
        __DSB();
        portYIELD_FROM_ISR(woken);
    }
}

/* --------------------------------------------------------------------------
 * Mixed-Signal Hardware Break Chain: DAC1_CH1 + COMP1 -> TIM1 Complementary PWM
 * -------------------------------------------------------------------------- */
static void pwr_dac1_comp1_init(void) {
    /* 1. Enable DAC1 (AHB2) and SYSCFG/COMP1 (APB2) + mandatory readback barriers */
    RCC->AHB2ENR |= RCC_AHB2ENR_DAC1EN;
    (void)RCC->AHB2ENR;
    RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN;
    (void)RCC->APB2ENR;

    /* PA4 -> Analog (DAC1_OUT1 threshold reference) */
    (void)GPIO_ConfigAnalog(GPIOA, 4U);

#if (POWER_SIM_MODE != 0U)
    /* Safe onboard simulation mode: bias PA1 solidly LOW (0 V) via internal pull-down
     * so an unconnected NUCLEO pin cannot float above DAC1 threshold at boot. */
    (void)GPIO_ConfigInput(GPIOA, 1U, GPIO_PULL_DOWN);
#else
    /* External power stage mode: PA1 pure Analog input for shunt/OPAMP current sense */
    (void)GPIO_ConfigAnalog(GPIOA, 1U);
#endif

    /* 2. Configure DAC1_CH1 @ 170 MHz:
     *    CRITICAL: HFSEL = 10b (DAC_MCR_HFSEL_1) mandatory for f_HCLK > 160 MHz!
     *    MODE1 = 001b (DAC_MCR_MODE1_0): buffer ON, routed to PA4 + internal COMP1 */
    DAC1->CR &= ~DAC_CR_EN1;
    DAC1->MCR = (DAC1->MCR & ~(DAC_MCR_HFSEL | DAC_MCR_MODE1)) |
                DAC_MCR_HFSEL_1 |
                DAC_MCR_MODE1_0;
    DAC1->CR |= DAC_CR_EN1;
    Clock_DelayUs(20U); /* t_WAKEUP (~10 us typical) */

#if (POWER_SIM_MODE != 0U)
    DAC1->DHR12R1 = PWR_DAC_THRESH_SIM_CODE;
#else
    DAC1->DHR12R1 = PWR_DAC_THRESH_HW_CODE;
#endif
    Clock_DelayUs(10U);

    /* 3. Configure COMP1:
     *    INPSEL = 0 (PA1), INMSEL = 5 (DAC1_CH1 internal), HYST = 2 (20 mV), EN = 1 */
    COMP1->CSR = (0UL << COMP_CSR_INPSEL_Pos) |
                 (5UL << COMP_CSR_INMSEL_Pos) |
                 (2UL << COMP_CSR_HYST_Pos)   |
                 COMP_CSR_EN;
    Clock_DelayUs(5U);
}

static void pwr_tim1_complementary_init(void) {
    RCC->APB2ENR |= RCC_APB2ENR_TIM1EN;
    (void)RCC->APB2ENR;

    /* PA8 = TIM1_CH1 (AF6), PB13 = TIM1_CH1N (AF6) per DS12288 Table 13 */
    (void)GPIO_ConfigAF(GPIOA, 8U, 6U, GPIO_OTYPE_PUSHPULL,
                        GPIO_SPEED_HIGH, GPIO_PULL_NONE);
    (void)GPIO_ConfigAF(GPIOB, 13U, 6U, GPIO_OTYPE_PUSHPULL,
                        GPIO_SPEED_HIGH, GPIO_PULL_NONE);

    TIM1->CR1  = 0U;
    TIM1->PSC  = 0U;
    TIM1->ARR  = PWR_TIM1_ARR; /* Center-aligned: 170 MHz / (2 * 1700) = 50.000 kHz */
    TIM1->CCR1 = 0U;           /* Soft-start from 0% duty */

    /* PWM Mode 1 (OC1M = 0110b) + Preload Enable (OC1PE = 1) */
    TIM1->CCMR1 = (6UL << TIM_CCMR1_OC1M_Pos) | TIM_CCMR1_OC1PE;
    TIM1->CCER  = TIM_CCER_CC1E | TIM_CCER_CC1NE;

    /* Safe Off Idle States: OIS1 = 0, OIS1N = 0 -> drive both MOSFET gates LOW on trip */
    TIM1->CR2  &= ~(TIM_CR2_OIS1 | TIM_CR2_OIS1N);

    /* CRITICAL: Direct silicon route COMP1_OUT -> TIM1_BKIN (< 50 ns async trip!).
     * Disable external BKINE bit so unconnected pin cannot false-trip. */
    TIM1->AF1   = TIM1_AF1_BKCMP1E;

    /* DTG = 85 (500.0 ns dead-time), OSSI|OSSR safe off-states, BKE=1, BKP=1 (active HIGH),
     * AOE = 0 (stay LATCHED OFF after overcurrent trip until software verifies clearance!) */
    TIM1->BDTR  = ((uint32_t)PWR_TIM1_DTG_500NS << TIM_BDTR_DTG_Pos) |
                  TIM_BDTR_OSSI |
                  TIM_BDTR_OSSR |
                  TIM_BDTR_BKE  |
                  TIM_BDTR_BKP;

    TIM1->EGR   = TIM_EGR_UG;
    TIM1->SR    = ~TIM_SR_BIF;
    TIM1->DIER  = TIM_DIER_BIE;

    /* NVIC Priority 5 (>= configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY) for xTaskNotifyFromISR */
    NVIC_SetPriority(TIM1_BRK_TIM15_IRQn, PWR_IRQ_PRIO_TIM1_BRK);
    NVIC_ClearPendingIRQ(TIM1_BRK_TIM15_IRQn);
    NVIC_EnableIRQ(TIM1_BRK_TIM15_IRQn);

    /* Enable Center-Aligned Mode 1 (CMS=01b) + ARPE + CEN, then assert MOE if COMP1 clear */
    TIM1->CR1   = TIM_CR1_CMS_0 | TIM_CR1_ARPE | TIM_CR1_CEN;
    if ((COMP1->CSR & COMP_CSR_VALUE) == 0U) {
        TIM1->BDTR |= TIM_BDTR_MOE;
    } else {
        g_fault_latched = 1U;
    }
}

/* --------------------------------------------------------------------------
 * Optional I2C1 OLED Probe (PA15/PB7 AF4, 400 kHz, Bounded NACKF Timeout)
 * -------------------------------------------------------------------------- */
static bool pwr_oled_probe_optional(void) {
    (void)GPIO_ConfigAF(GPIOA, 15U, 4U, GPIO_OTYPE_OPENDRAIN,
                        GPIO_SPEED_HIGH, GPIO_PULL_UP);
    (void)GPIO_ConfigAF(GPIOB, 7U, 4U, GPIO_OTYPE_OPENDRAIN,
                        GPIO_SPEED_HIGH, GPIO_PULL_UP);

    if (i2c_init_master(I2C1, I2C_SPEED_400KHZ) != I2C_OK) {
        return false;
    }
    static const uint8_t k_probe[2] = { 0x00U, 0xAEU };
    return (i2c_write(I2C1, PWR_SSD1306_ADDR7, k_probe,
                      sizeof(k_probe), PWR_I2C_TIMEOUT) == I2C_OK);
}

/* --------------------------------------------------------------------------
 * Task 1 (Priority 4): Safety & Watchdog Supervisor
 * -------------------------------------------------------------------------- */
static void vSafetySupervisorTask(void *pvParameters) {
    (void)pvParameters;

    /* Start IWDG (1.5 s nominal, frozen in GDB via DBG_IWDG_STOP) once scheduler is live */
    (void)iwdg_init_ms(PWR_IWDG_TIMEOUT_MS, true, 500000U);

    EventBits_t seen_heartbeats = 0U;
    uint32_t    led_div         = 0U;

    for (;;) {
        uint32_t notify_bits = 0U;
        (void)xTaskNotifyWait(0U, 0xFFFFFFFFUL, &notify_bits, pdMS_TO_TICKS(100U));

        if ((notify_bits & PWR_NOTIFY_HW_BREAK) != 0U) {
            /* Hardware already cleared TIM1->BDTR.MOE in < 50 ns; confirm safe state */
            TIM1->BDTR &= ~TIM_BDTR_MOE;
            TIM1->CCR1  = 0U;
            GPIO_SetPin(GPIOA, 5U);
            pwr_uart_log("[SAFETY] HW BREAK LATCHED (MOE=0, AOE=0). Press B1 to clear.\r\n");
        }

        if ((notify_bits & PWR_NOTIFY_CLEAR_FAULT_REQ) != 0U) {
            if ((COMP1->CSR & COMP_CSR_VALUE) == 0U) {
                g_fault_latched = 0U;
                TIM1->SR        = ~TIM_SR_BIF;
                TIM1->BDTR     |= TIM_BDTR_MOE;
                GPIO_ResetPin(GPIOA, 5U);
                pwr_uart_log("[SAFETY] COMP1 clear -> Fault reset, TIM1 MOE re-enabled.\r\n");
            } else {
                pwr_uart_log("[SAFETY] Reset DENIED: COMP1 overcurrent still active!\r\n");
            }
        }

        /* Collect and verify worker task heartbeats before kicking IWDG */
        seen_heartbeats |= xEventGroupClearBits(g_wdg_events, PWR_EVT_ALL_TASKS);
        if ((seen_heartbeats & PWR_EVT_ALL_TASKS) == PWR_EVT_ALL_TASKS) {
            seen_heartbeats = 0U;
            iwdg_kick();
            g_wdg_feeds++;
        }

        /* Heartbeat LED toggle when healthy; solid ON when fault latched */
        if (g_fault_latched == 0U) {
            led_div++;
            if (led_div >= 5U) {
                led_div = 0U;
                GPIO_TogglePin(GPIOA, 5U);
            }
        }
    }
}

/* --------------------------------------------------------------------------
 * Task 2 (Priority 3): 1 kHz Fixed-Point Q15 PI Control Loop
 * -------------------------------------------------------------------------- */
static void vControlLoopTask(void *pvParameters) {
    (void)pvParameters;

    q15_pi_ctrl_t pi;
#if (POWER_SIM_MODE != 0U)
    buck_sim_plant_t plant = { .v_out_mv = 0, .tick_count = 0U };
#endif

    /* Kp = 0.45 (14746 Q15), Ki*Ts = 0.08 (2621 Q15), clamped to [50..900] permille */
    q15_pi_init(&pi, 14746, 2621, (int32_t)PWR_DUTY_MIN_PERMILLE, (int32_t)PWR_DUTY_MAX_PERMILLE);

    TickType_t last_wake  = xTaskGetTickCount();
    uint32_t   loop_ticks = 0U;

    for (;;) {
        vTaskDelayUntil(&last_wake, pdMS_TO_TICKS(1U)); /* Strict 1.000 kHz cadence */
        loop_ticks++;

        bool     moe_active = ((TIM1->BDTR & TIM_BDTR_MOE) != 0U);
        bool     faulted    = (g_fault_latched != 0U) || !moe_active;
        int32_t  v_ref      = g_v_ref_mv;
        int32_t  v_meas     = 0;
        uint16_t duty_pm    = 0U;
        uint16_t ccr1_val   = 0U;

#if (POWER_SIM_MODE != 0U)
        v_meas = plant.v_out_mv;
#else
        /* External power stage mode: sample real V_OUT sense via ADC1_IN1 (PA0) */
        uint16_t adc_raw = 0U;
        if (adc_read_channel(ADC1, PWR_VSENSE_ADC_CH, ADC_SMP_47_5_CYCLES,
                             &adc_raw, PWR_ADC_TIMEOUT) == ADC_OK) {
            uint32_t pin_mv = adc_raw_to_mv(adc_raw, PWR_ADC_VREF_MV);
            v_meas = (int32_t)(((uint64_t)pin_mv * (uint64_t)POWER_VSENSE_SCALE_NUM) /
                               (uint64_t)POWER_VSENSE_SCALE_DEN);
        } else {
            /* Fail-safe: immediately de-energize power stage if ADC sense fails */
            TIM1->BDTR     &= ~TIM_BDTR_MOE;
            TIM1->CCR1      = 0U;
            g_fault_latched = 1U;
            faulted         = true;
            moe_active      = false;
        }
#endif

        if (faulted) {
            q15_pi_reset(&pi);
            duty_pm    = 0U;
            ccr1_val   = 0U;
            TIM1->CCR1 = 0U;
        } else {
            int32_t err_mv = v_ref - v_meas;
            int32_t u_pm   = q15_pi_step(&pi, err_mv);
            duty_pm        = (uint16_t)u_pm;
            ccr1_val       = (uint16_t)(((uint32_t)duty_pm * (uint32_t)PWR_TIM1_ARR) / 1000U);
            TIM1->CCR1     = ccr1_val;
        }

#if (POWER_SIM_MODE != 0U)
        /* Step discrete buck plant model (safe zero-wire closed-loop verification) */
        v_meas = buck_sim_step(&plant, duty_pm, !faulted);
#endif

        /* Check in with SafetySupervisorTask every 50 ms */
        if ((loop_ticks % 50U) == 0U) {
            (void)xEventGroupSetBits(g_wdg_events, PWR_EVT_CTRL_ALIVE);
        }

        /* Non-blocking telemetry enqueue every 500 ms */
        if ((loop_ticks % 500U) == 0U) {
            power_telem_msg_t msg = {
                .tick_ms       = (uint32_t)xTaskGetTickCount(),
                .v_ref_mv      = v_ref,
                .v_out_mv      = v_meas,
                .duty_permille = duty_pm,
                .tim1_ccr1     = ccr1_val,
                .moe_active    = moe_active,
                .fault_latched = faulted,
                .break_trips   = g_hw_break_count,
                .wdg_feeds     = g_wdg_feeds
            };
            (void)xQueueSend(g_telem_queue, &msg, 0U);
        }
    }
}

/* --------------------------------------------------------------------------
 * Task 3 (Priority 2, 512-word Stack): Mutex-Guarded USART2 Telemetry Logger
 * -------------------------------------------------------------------------- */
static void vTelemetryTask(void *pvParameters) {
    (void)pvParameters;

    power_telem_msg_t msg;
    char              line[224];

    for (;;) {
        if (xQueueReceive(g_telem_queue, &msg, pdMS_TO_TICKS(250U)) == pdTRUE) {
            UBaseType_t hwm_safe  = uxTaskGetStackHighWaterMark(g_safety_handle);
            UBaseType_t hwm_ctrl  = uxTaskGetStackHighWaterMark(g_ctrl_handle);
            UBaseType_t hwm_telem = uxTaskGetStackHighWaterMark(g_telem_handle);
            UBaseType_t hwm_ui    = uxTaskGetStackHighWaterMark(g_ui_handle);

            (void)snprintf(
                line, sizeof(line),
                "{\"t_ms\":%lu,\"sim\":%u,\"vref_mv\":%ld,\"vout_mv\":%ld,"
                "\"duty_pm\":%u,\"ccr1\":%u,\"moe\":%u,\"fault\":%u,"
                "\"brk\":%lu,\"wdg\":%lu,\"hwm\":[%lu,%lu,%lu,%lu]}\r\n",
                (unsigned long)msg.tick_ms,
                (unsigned int)POWER_SIM_MODE,
                (long)msg.v_ref_mv,
                (long)msg.v_out_mv,
                (unsigned int)msg.duty_permille,
                (unsigned int)msg.tim1_ccr1,
                msg.moe_active ? 1U : 0U,
                msg.fault_latched ? 1U : 0U,
                (unsigned long)msg.break_trips,
                (unsigned long)msg.wdg_feeds,
                (unsigned long)hwm_safe,
                (unsigned long)hwm_ctrl,
                (unsigned long)hwm_telem,
                (unsigned long)hwm_ui);
            pwr_uart_log(line);
        }

        (void)xEventGroupSetBits(g_wdg_events, PWR_EVT_TELEM_ALIVE);
    }
}

/* --------------------------------------------------------------------------
 * Task 4 (Priority 1 — NEVER Priority 0!): User Input (B1) & Optional OLED
 * -------------------------------------------------------------------------- */
static void vUiTask(void *pvParameters) {
    (void)pvParameters;

    TickType_t last_wake     = xTaskGetTickCount();
    uint32_t   hold_ticks    = 0U;
    uint32_t   setpoint_idx  = 1U; /* 5000 mV */
    bool       long_fired    = false;

    for (;;) {
        vTaskDelayUntil(&last_wake, pdMS_TO_TICKS(100U)); /* 10 Hz UI cadence */

        bool b1_high = GPIO_ReadPin(GPIOC, 13U); /* Active-HIGH per UM2505 */
        if (b1_high) {
            hold_ticks++;
            if ((hold_ticks >= 15U) && !long_fired) {
                /* Long press (>= 1.5 s): exercise real TIM1 hardware Break via EGR.BG! */
                long_fired = true;
                pwr_uart_log("[UI] Long B1 press -> Injecting TIM1->EGR=BG hardware break test!\r\n");
                TIM1->EGR = TIM_EGR_BG;
            }
        } else {
            if ((hold_ticks >= 1U) && !long_fired) {
                /* Short press: clear latched fault OR cycle target setpoint */
                if (g_fault_latched != 0U) {
                    (void)xTaskNotify(g_safety_handle, PWR_NOTIFY_CLEAR_FAULT_REQ, eSetBits);
                } else {
                    setpoint_idx = (setpoint_idx + 1U) % 3U;
                    g_v_ref_mv   = k_setpoints_mv[setpoint_idx];
                    pwr_uart_log("[UI] Short B1 press -> Setpoint cycled.\r\n");
                }
            }
            hold_ticks = 0U;
            long_fired = false;
        }

        /* Optional SSD1306 I2C1 liveness update (never hangs if absent!) */
        if (g_oled_present) {
            uint8_t cmd[2] = { 0x00U, (g_fault_latched != 0U) ? 0xA7U : 0xA6U };
            if (i2c_write(I2C1, PWR_SSD1306_ADDR7, cmd, sizeof(cmd), PWR_I2C_TIMEOUT) != I2C_OK) {
                g_oled_present = false;
            }
        }

        (void)xEventGroupSetBits(g_wdg_events, PWR_EVT_UI_ALIVE);
    }
}

/* --------------------------------------------------------------------------
 * Application Entry Point
 * -------------------------------------------------------------------------- */
int main(void) {
    FaultHandler_EnableDetailedFaults(true);

    /* MANDATORY for FreeRTOS on ARM Cortex-M4: 4 preemption bits, 0 subpriority bits! */
    NVIC_SetPriorityGrouping(0U);

    if (!Clock_Init_170MHz()) {
        while (1) {
            __NOP();
        }
    }
    Clock_DWT_Init();

    /* Enable AHB2 peripheral clocks before touching GPIOA/GPIOB/GPIOC registers */
    GPIO_EnableClock(GPIOA);
    GPIO_EnableClock(GPIOB);
    GPIO_EnableClock(GPIOC);

    /* Basic onboard GPIO: LD2 (PA5), B1 (PC13 Active-HIGH), USART2 (PA2/PA3 AF7) */
    (void)GPIO_ConfigOutput(GPIOA, 5U, GPIO_OTYPE_PUSHPULL,
                            GPIO_SPEED_LOW, GPIO_PULL_NONE);
    GPIO_ResetPin(GPIOA, 5U);
    (void)GPIO_ConfigInput(GPIOC, 13U, GPIO_PULL_NONE);

    (void)GPIO_ConfigAF(GPIOA, 2U, 7U, GPIO_OTYPE_PUSHPULL,
                        GPIO_SPEED_MEDIUM, GPIO_PULL_NONE);
    (void)GPIO_ConfigAF(GPIOA, 3U, 7U, GPIO_OTYPE_PUSHPULL,
                        GPIO_SPEED_MEDIUM, GPIO_PULL_UP);
    (void)usart_init(USART2, PWR_UART_BAUD, false, 0U);

    /* Optional I2C1 OLED probe (never stalls if disconnected) */
    g_oled_present = pwr_oled_probe_optional();

    /* Initialize asynchronous hardware break chain & TIM1 50 kHz complementary PWM */
    pwr_dac1_comp1_init();
#if (POWER_SIM_MODE == 0U)
    /* External power stage mode: configure PA0 (ADC1_IN1) for closed-loop V_OUT sense */
    (void)GPIO_ConfigAnalog(GPIOA, 0U);
    if (adc_init_single_ended(ADC1, PWR_ADC_TIMEOUT) != ADC_OK) {
        g_fault_latched = 1U; /* Fail-safe: keep TIM1 MOE disabled if ADC1 fails to init */
    }
#endif
    pwr_tim1_complementary_init();

    pwr_uart_log("\r\n============================================================\r\n");
    pwr_uart_log("Project 03: FreeRTOS Digital Power Controller (STM32G474RE)\r\n");
    pwr_uart_log("  Clock : 170 MHz Boost | PRIGROUP=0 | FreeRTOS V11.1.0\r\n");
    pwr_uart_log("  PWM   : TIM1 PA8/PB13 50 kHz Complementary, 500 ns dead-time\r\n");
    pwr_uart_log("  Break : COMP1 -> TIM1->AF1.BKCMP1E (<50 ns MOE trip, AOE=0)\r\n");
#if (POWER_SIM_MODE != 0U)
    pwr_uart_log("  Mode  : ONBOARD SAFE SIMULATION (Ext Power Stage PENDING)\r\n");
#else
    pwr_uart_log("  Mode  : EXTERNAL HARDWARE POWER STAGE (ADC1_IN1 PA0 V_OUT sense)\r\n");
#endif
    pwr_uart_log("  OLED  : ");
    pwr_uart_log(g_oled_present ? "DETECTED on I2C1 (PA15/PB7)\r\n"
                                : "ABSENT (EXTERNAL HARDWARE PENDING)\r\n");
    pwr_uart_log("  B1    : Short press=setpoint/reset fault, Long press=BG trip\r\n");
    pwr_uart_log("============================================================\r\n");

    /* Create static IPC primitives */
    g_uart_mutex  = xSemaphoreCreateMutexStatic(&g_uart_mutex_cb);
    g_wdg_events  = xEventGroupCreateStatic(&g_wdg_events_cb);
    g_telem_queue = xQueueCreateStatic(4U,
                                       sizeof(power_telem_msg_t),
                                       g_telem_queue_buf,
                                       &g_telem_queue_cb);

    configASSERT(g_uart_mutex  != NULL);
    configASSERT(g_wdg_events  != NULL);
    configASSERT(g_telem_queue != NULL);

    /* Create 4 statically allocated FreeRTOS tasks (Priorities 4, 3, 2, 1 — NEVER 0!) */
    g_safety_handle = xTaskCreateStatic(vSafetySupervisorTask, "SafetySup",
                                        PWR_STACK_SAFETY, NULL, PWR_PRIO_SAFETY,
                                        g_safety_stack, &g_safety_tcb);

    g_ctrl_handle   = xTaskCreateStatic(vControlLoopTask, "CtrlQ15PI",
                                        PWR_STACK_CONTROL, NULL, PWR_PRIO_CONTROL,
                                        g_ctrl_stack, &g_ctrl_tcb);

    g_telem_handle  = xTaskCreateStatic(vTelemetryTask, "Telemetry",
                                        PWR_STACK_TELEMETRY, NULL, PWR_PRIO_TELEMETRY,
                                        g_telem_stack, &g_telem_tcb);

    g_ui_handle     = xTaskCreateStatic(vUiTask, "UiOledBtn",
                                        PWR_STACK_UI, NULL, PWR_PRIO_UI,
                                        g_ui_stack, &g_ui_tcb);

    configASSERT(g_safety_handle != NULL);
    configASSERT(g_ctrl_handle   != NULL);
    configASSERT(g_telem_handle  != NULL);
    configASSERT(g_ui_handle     != NULL);

    /* Transfer control to preemptive FreeRTOS scheduler (never returns) */
    vTaskStartScheduler();

    pwr_force_safe_off_and_halt("SCHEDULER_RETURNED");
    return 0;
}
