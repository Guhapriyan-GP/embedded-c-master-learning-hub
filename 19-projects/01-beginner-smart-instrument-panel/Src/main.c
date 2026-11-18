/**
 * @file    main.c
 * @brief   Project 01 — Smart Instrument Panel & Telemetry Unit (Level 1-2 Capstone).
 *
 * Self-contained onboard mode (MODE_INTERNAL_TELEM, default at boot) requires
 * ZERO external wires on ST NUCLEO-G474RE:
 *   - System Clock: 170 MHz Boost Mode via Clock_Init_170MHz()
 *                   (or 160 MHz when compiled with -DUSE_CLOCK_160MHZ)
 *   - SysTick:      1 ms monotonic tick driving a 4-state non-blocking FSM debouncer
 *                   on Active-HIGH B1 (PC13, UM2505 external pull-down R31)
 *   - ADC1:         42.5 MHz (CKMODE=11b, <= 60 MHz), internal VREFINT + calibrated
 *                   internal die temperature sensor (onboard safe), plus optional
 *                   PA0 (ADC1_IN1) external potentiometer in MODE_EXT_POT_PWM
 *   - TIM2_CH2:     PA1 (AF1 — zero conflict with PA0!) 20.000 kHz hardware PWM
 *                   proportional to measured reading
 *   - USART2:       PA2/PA3 (AF7, 115200 8N1) ST-LINK VCP JSON telemetry +
 *                   non-blocking RX command parser ('m' mode toggle, 's' status dump)
 *                   backed by lock-free C11 SPSC ring_buffer_t
 *   - LD2 (PA5):    Mode status indicator LED
 *   - IWDG:         1.0 s hardware watchdog with DBG_IWDG_STOP debug freeze
 *
 * External hardware (PA0 potentiometer, PA1 PWM load) is strictly optional and
 * documented as EXTERNAL HARDWARE PENDING.
 */

#include "app_panel.h"
#include "stm32g474xx.h"

#include "adc_g4.h"
#include "clock_g4.h"
#include "fault_handler.h"
#include "gpio_g4.h"
#include "iwdg_g4.h"
#include "ring_buffer.h"
#include "tim_g4.h"
#include "usart_g4.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

/* --------------------------------------------------------------------------
 * Module-Private State
 * -------------------------------------------------------------------------- */
static volatile uint32_t g_systick_ms = 0U;

static uint8_t       g_rx_storage[PANEL_RX_RING_CAPACITY];
static ring_buffer_t g_rx_rb;

static btn_debouncer_t   g_btn_fsm;
static panel_telemetry_t g_telem;

/* --------------------------------------------------------------------------
 * Interrupt Service Routines (Minimal, Non-Blocking, Atomic)
 * -------------------------------------------------------------------------- */
void SysTick_Handler(void) {
    g_systick_ms++;
}

void USART2_IRQHandler(void) {
    usart_rx_irq_handler(USART2, &g_rx_rb);
}

/* --------------------------------------------------------------------------
 * Formatting & Telemetry Helpers (Integer-Only: Zero Double-Promotion Overhead)
 * -------------------------------------------------------------------------- */
static void format_temp_parts(int32_t mdegc, int32_t *out_whole, uint32_t *out_tenth) {
    int32_t abs_mdegc = (mdegc < 0) ? -mdegc : mdegc;
    *out_whole = mdegc / 1000;
    *out_tenth = (uint32_t)((abs_mdegc % 1000) / 100);
}

static void uart_send_str(const char *msg) {
    (void)usart_write_str(USART2, msg, PANEL_UART_TX_TIMEOUT);
}

static void panel_emit_json_telemetry(const panel_telemetry_t *t) {
    char     buf[224];
    int32_t  t_whole = 0;
    uint32_t t_tenth = 0U;
    format_temp_parts(t->temp_mdegc, &t_whole, &t_tenth);

    (void)snprintf(
        buf, sizeof(buf),
        "{\"tick_ms\":%lu,\"clk_mhz\":%lu,\"mode\":\"%s\",\"vdda_mv\":%lu,"
        "\"temp_c\":%ld.%01lu,\"pot_raw\":%u,\"pot_mv\":%lu,"
        "\"pwm_permille\":%u,\"rx_cmds\":%lu}\r\n",
        (unsigned long)t->tick_ms,
        (unsigned long)(t->sysclk_hz / 1000000UL),
        panel_mode_name(t->mode),
        (unsigned long)t->vdda_mv,
        (long)t_whole,
        (unsigned long)t_tenth,
        (unsigned int)t->pot_raw,
        (unsigned long)t->pot_mv,
        (unsigned int)t->pwm_permille,
        (unsigned long)t->rx_cmd_count);

    uart_send_str(buf);
}

static void panel_emit_status_dump(const panel_telemetry_t *t) {
    char     line[128];
    int32_t  t_whole = 0;
    uint32_t t_tenth = 0U;
    format_temp_parts(t->temp_mdegc, &t_whole, &t_tenth);

    uart_send_str("\r\n--- [PANEL STATUS] ---\r\n");

    (void)snprintf(line, sizeof(line), "  SYSCLK      : %lu Hz\r\n",
                   (unsigned long)t->sysclk_hz);
    uart_send_str(line);

    (void)snprintf(line, sizeof(line), "  Mode        : %s (B1 or UART 'm')\r\n",
                   panel_mode_name(t->mode));
    uart_send_str(line);

    (void)snprintf(line, sizeof(line), "  VDDA        : %lu mV (VREFINT)\r\n",
                   (unsigned long)t->vdda_mv);
    uart_send_str(line);

    (void)snprintf(line, sizeof(line), "  Die Temp    : %ld.%01lu C (TS_CAL1/2)\r\n",
                   (long)t_whole, (unsigned long)t_tenth);
    uart_send_str(line);

    (void)snprintf(line, sizeof(line), "  Ext Pot PA0 : raw=%u (%lu mV) [EXT PENDING]\r\n",
                   (unsigned int)t->pot_raw, (unsigned long)t->pot_mv);
    uart_send_str(line);

    (void)snprintf(line, sizeof(line), "  PWM PA1 AF1 : %u.%01u%% @ %u Hz (ARR=%lu)\r\n",
                   (unsigned int)(t->pwm_permille / 10U),
                   (unsigned int)(t->pwm_permille % 10U),
                   (unsigned int)PANEL_PWM_FREQ_HZ,
                   (unsigned long)TIM2->ARR);
    uart_send_str(line);

    (void)snprintf(line, sizeof(line), "  B1 Presses  : %lu | RX Cmds: %lu | ADC Err: %lu\r\n",
                   (unsigned long)g_btn_fsm.press_count,
                   (unsigned long)t->rx_cmd_count,
                   (unsigned long)t->adc_errors);
    uart_send_str(line);

    (void)snprintf(line, sizeof(line), "  Prev Reset  : IWDG=%u | HardFaultLog=%u\r\n",
                   t->iwdg_reset_prev ? 1U : 0U,
                   t->crash_log_prev ? 1U : 0U);
    uart_send_str(line);

    uart_send_str("----------------------\r\n");
}

/* --------------------------------------------------------------------------
 * Hardware Initialization
 * -------------------------------------------------------------------------- */
static bool board_init_clocks(void) {
#if defined(USE_CLOCK_160MHZ)
    return Clock_Init_160MHz();
#else
    return Clock_Init_170MHz();
#endif
}

static void board_init_gpio(void) {
    /* Enable AHB2 peripheral clocks before touching GPIOA/GPIOC registers */
    GPIO_EnableClock(GPIOA);
    GPIO_EnableClock(GPIOC);

    /* LD2 Green User LED: PA5 Push-Pull Output, initially OFF */
    (void)GPIO_ConfigOutput(GPIOA, 5U, GPIO_OTYPE_PUSHPULL,
                            GPIO_SPEED_LOW, GPIO_PULL_NONE);
    GPIO_ResetPin(GPIOA, 5U);

    /* B1 Blue User Button: PC13 Digital Input, No-Pull (external R31 pull-down, Active-HIGH) */
    (void)GPIO_ConfigInput(GPIOC, 13U, GPIO_PULL_NONE);

    /* ADC1_IN1 External Potentiometer Input: PA0 Analog (zero conflict with TIM2_CH2 on PA1!) */
    (void)GPIO_ConfigAnalog(GPIOA, 0U);

    /* TIM2_CH2 Hardware PWM Output: PA1 AF1 Push-Pull High-Speed */
    (void)GPIO_ConfigAF(GPIOA, 1U, 1U, GPIO_OTYPE_PUSHPULL,
                        GPIO_SPEED_HIGH, GPIO_PULL_NONE);

    /* USART2 ST-LINK VCP: PA2 (TX, AF7), PA3 (RX, AF7 with pull-up) */
    (void)GPIO_ConfigAF(GPIOA, 2U, 7U, GPIO_OTYPE_PUSHPULL,
                        GPIO_SPEED_MEDIUM, GPIO_PULL_NONE);
    (void)GPIO_ConfigAF(GPIOA, 3U, 7U, GPIO_OTYPE_PUSHPULL,
                        GPIO_SPEED_MEDIUM, GPIO_PULL_UP);
}

/* --------------------------------------------------------------------------
 * Non-Blocking RX Command Parser (Consumes SPSC Ring Buffer)
 * -------------------------------------------------------------------------- */
static void panel_process_uart_rx(void) {
    uint8_t ch = 0U;
    while (rb_pop(&g_rx_rb, &ch)) {
        if ((ch == (uint8_t)'m') || (ch == (uint8_t)'M')) {
            g_telem.rx_cmd_count++;
            g_telem.mode = panel_mode_next(g_telem.mode);
            uart_send_str("[CMD] Mode -> ");
            uart_send_str(panel_mode_name(g_telem.mode));
            uart_send_str("\r\n");
        } else if ((ch == (uint8_t)'s') || (ch == (uint8_t)'S')) {
            g_telem.rx_cmd_count++;
            panel_emit_status_dump(&g_telem);
        } else if ((ch == (uint8_t)'h') || (ch == (uint8_t)'?')) {
            g_telem.rx_cmd_count++;
            uart_send_str("[HELP] Commands: 'm'=cycle mode, 's'=status dump, 'h'=help\r\n");
        } else {
            /* Ignore CR/LF or unknown characters cleanly */
        }
    }
}

/* --------------------------------------------------------------------------
 * Periodic Sensor Acquisition & PWM Update (Every 50 ms)
 * -------------------------------------------------------------------------- */
static bool panel_sample_and_update_pwm(void) {
    bool healthy = true;

    /* Always refresh onboard internal VDDA & die temperature (safe zero-wire path) */
    int32_t  temp_mdegc = 0;
    uint32_t vdda_mv    = 3300U;
    adc_status_t st = adc_read_temp_mdegc(&temp_mdegc, &vdda_mv, PANEL_ADC_TIMEOUT);
    if (st == ADC_OK) {
        g_telem.temp_mdegc = temp_mdegc;
        g_telem.vdda_mv    = vdda_mv;
    } else {
        g_telem.adc_errors++;
        healthy = false;
    }

    /* Sample external PA0 potentiometer when in MODE_EXT_POT_PWM */
    uint16_t duty_permille = 0U;
    if (g_telem.mode == MODE_IDLE) {
        duty_permille = 0U;
    } else if (g_telem.mode == MODE_INTERNAL_TELEM) {
        duty_permille = panel_temp_to_pwm_permille(g_telem.temp_mdegc);
    } else {
        uint16_t pot_raw = 0U;
        adc_status_t pot_st = adc_read_channel(ADC1, 1U, ADC_SMP_47_5_CYCLES,
                                               &pot_raw, PANEL_ADC_TIMEOUT);
        if (pot_st == ADC_OK) {
            g_telem.pot_raw = pot_raw;
            g_telem.pot_mv  = adc_raw_to_mv(pot_raw, g_telem.vdda_mv);
            duty_permille   = panel_raw_to_pwm_permille(pot_raw);
        } else {
            g_telem.adc_errors++;
            duty_permille = 0U;
            healthy = false;
        }
    }

    g_telem.pwm_permille = duty_permille;
    (void)TIM_SetPwmDutyPermille(TIM2, 2U, duty_permille);
    return healthy;
}

/* --------------------------------------------------------------------------
 * LED Mode Indicator (Non-Blocking)
 * -------------------------------------------------------------------------- */
static void panel_update_led(uint32_t now_ms, uint32_t *last_led_ms) {
    if (g_telem.mode == MODE_EXT_POT_PWM) {
        GPIO_SetPin(GPIOA, 5U);
        *last_led_ms = now_ms;
    } else {
        uint32_t period_ms = (g_telem.mode == MODE_INTERNAL_TELEM) ? 250U : 500U;
        if ((now_ms - *last_led_ms) >= period_ms) {
            *last_led_ms = now_ms;
            GPIO_TogglePin(GPIOA, 5U);
        }
    }
}

/* --------------------------------------------------------------------------
 * Application Entry Point
 * -------------------------------------------------------------------------- */
int main(void) {
    FaultHandler_EnableDetailedFaults(true);
    NVIC_SetPriorityGrouping(0U);

    if (!board_init_clocks()) {
        while (1) {
            __NOP();
        }
    }
    Clock_DWT_Init();

    /* Capture reset diagnostics before clearing RCC->CSR flags */
    g_telem.iwdg_reset_prev = iwdg_was_reset_cause();
    iwdg_clear_reset_flags();
    g_telem.crash_log_prev  = FaultHandler_HasValidCrashLog();
    if (g_telem.crash_log_prev) {
        FaultHandler_ClearCrashLog();
    }

    g_telem.sysclk_hz = Clock_GetSysClkHz();
    g_telem.mode      = MODE_INTERNAL_TELEM; /* Self-contained zero-wire boot mode */
    g_telem.vdda_mv   = 3300U;

    board_init_gpio();
    btn_debouncer_init(&g_btn_fsm, PANEL_BTN_DEBOUNCE_MS);
    (void)rb_init(&g_rx_rb, g_rx_storage, PANEL_RX_RING_CAPACITY);

    /* Configure SysTick for exact 1 ms tick using live SystemCoreClock */
    (void)SysTick_Config(g_telem.sysclk_hz / 1000U);
    NVIC_SetPriority(SysTick_IRQn, 2U);

    /* Initialize USART2 (115200 8N1, RX interrupt enabled) */
    (void)usart_init(USART2, PANEL_UART_BAUD, true, 4U);

    /* Initialize ADC1 (CKMODE=11b -> 42.5 MHz @ 170 MHz or 40 MHz @ 160 MHz) */
    (void)adc_init_single_ended(ADC1, PANEL_ADC_TIMEOUT);

    /* Initialize TIM2_CH2 on PA1 (AF1) at 20.000 kHz, initial 0% duty */
    (void)TIM_InitPwm(TIM2, 2U, PANEL_PWM_FREQ_HZ, 0U);

    /* Start Independent Watchdog (1.0 s nominal, frozen while halted in GDB) */
    (void)iwdg_init_ms(PANEL_IWDG_TIMEOUT_MS, true, 500000U);

    uart_send_str("\r\n============================================================\r\n");
    uart_send_str("Project 01: Smart Instrument Panel & Telemetry Unit (G474RE)\r\n");
    uart_send_str("Default Mode: MODE_INTERNAL_TELEM (Zero external wires)\r\n");
    uart_send_str("Controls: B1 (PC13) or UART 'm'=mode, 's'=status, 'h'=help\r\n");
    uart_send_str("============================================================\r\n");

    uint32_t last_btn_ms   = g_systick_ms;
    uint32_t last_adc_ms   = g_systick_ms;
    uint32_t last_telem_ms = g_systick_ms;
    uint32_t last_led_ms   = g_systick_ms;
    bool     subsys_ok     = true;

    /* Initial sample so first status/JSON frame contains fresh data */
    subsys_ok = panel_sample_and_update_pwm();
    g_telem.tick_ms = g_systick_ms;
    panel_emit_status_dump(&g_telem);

    while (1) {
        uint32_t now_ms = g_systick_ms;
        g_telem.tick_ms = now_ms;

        /* 1. Non-blocking UART RX command processing */
        panel_process_uart_rx();

        /* 2. 1 ms non-blocking 4-state B1 (PC13) button debouncer FSM */
        if ((now_ms - last_btn_ms) >= 1U) {
            last_btn_ms = now_ms;
            bool b1_pressed = GPIO_ReadPin(GPIOC, 13U); /* Active-HIGH per UM2505 */
            if (btn_debouncer_step(&g_btn_fsm, b1_pressed, now_ms)) {
                g_telem.mode = panel_mode_next(g_telem.mode);
                uart_send_str("[BTN] B1 Press -> Mode: ");
                uart_send_str(panel_mode_name(g_telem.mode));
                uart_send_str("\r\n");
            }
        }

        /* 3. 50 ms ADC acquisition & TIM2_CH2 PWM update */
        if ((now_ms - last_adc_ms) >= PANEL_ADC_PERIOD_MS) {
            last_adc_ms = now_ms;
            subsys_ok = panel_sample_and_update_pwm();
        }

        /* 4. 500 ms structured JSON telemetry stream over USART2 */
        if ((now_ms - last_telem_ms) >= PANEL_TELEM_PERIOD_MS) {
            last_telem_ms = now_ms;
            panel_emit_json_telemetry(&g_telem);
        }

        /* 5. Mode-dependent LD2 (PA5) LED heartbeat */
        panel_update_led(now_ms, &last_led_ms);

        /* 6. Conditional hardware watchdog refresh */
        if (subsys_ok) {
            iwdg_kick();
        }
    }
}
