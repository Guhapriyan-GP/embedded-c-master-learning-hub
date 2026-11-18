/**
 * @file    app_panel.h
 * @brief   Project 01 — Smart Instrument Panel & Telemetry Unit interface,
 *          mode definitions, and non-blocking 4-state SysTick FSM debouncer.
 *
 * Target:  ST NUCLEO-G474RE (STM32G474RET6, Cortex-M4F @ 170 MHz or 160 MHz)
 * Concepts: STM32-RCC-001, PERIPH-GPIO-001, ARM-SYST-001, FW-FSM-001,
 *           PERIPH-ADC-001, PERIPH-PWM-001, PERIPH-UART-001, EMB-CONC-001,
 *           PERIPH-WDG-001, REL-FAULT-001
 */

#ifndef APP_PANEL_H
#define APP_PANEL_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* --------------------------------------------------------------------------
 * Configuration Constants
 * -------------------------------------------------------------------------- */
#define PANEL_UART_BAUD             (115200U)
#define PANEL_PWM_FREQ_HZ           (20000U)  /* 20.000 kHz TIM2_CH2 on PA1 (AF1)     */
#define PANEL_BTN_DEBOUNCE_MS       (25U)     /* 25 ms mechanical contact settle time */
#define PANEL_ADC_PERIOD_MS         (50U)     /* 20 Hz sensor acquisition & PWM update */
#define PANEL_TELEM_PERIOD_MS       (500U)    /* 2 Hz structured USART2 JSON stream   */
#define PANEL_IWDG_TIMEOUT_MS       (1000U)   /* 1.0 s hardware watchdog supervision  */
#define PANEL_UART_TX_TIMEOUT       (200000U) /* Bounded polling loop budget          */
#define PANEL_ADC_TIMEOUT           (200000U) /* Bounded ADC polling loop budget      */
#define PANEL_RX_RING_CAPACITY      (64U)     /* Power-of-2 SPSC RX ring buffer       */

/* Temperature-to-PWM mapping bounds for self-contained onboard test mode */
#define PANEL_TEMP_MIN_MDEGC        (20000)   /* 20.0 C -> 10.0% PWM duty             */
#define PANEL_TEMP_MAX_MDEGC        (70000)   /* 70.0 C -> 100.0% PWM duty            */
#define PANEL_TEMP_MIN_PERMILLE     (100U)
#define PANEL_TEMP_MAX_PERMILLE     (1000U)

/* --------------------------------------------------------------------------
 * Operating Modes
 * -------------------------------------------------------------------------- */
typedef enum {
    MODE_IDLE           = 0U, /**< Safe idle: PWM = 0%, LD2 slow heartbeat    */
    MODE_INTERNAL_TELEM = 1U, /**< Self-contained onboard VREFINT + Temp mode */
    MODE_EXT_POT_PWM    = 2U  /**< External PA0 pot -> PA1 TIM2_CH2 PWM mode  */
} panel_mode_t;

/* --------------------------------------------------------------------------
 * Non-Blocking 4-State SysTick FSM Debouncer (Active-HIGH PC13 B1)
 * -------------------------------------------------------------------------- */
typedef enum {
    BTN_STATE_RELEASED         = 0U,
    BTN_STATE_DEBOUNCE_PRESS   = 1U,
    BTN_STATE_PRESSED          = 2U,
    BTN_STATE_DEBOUNCE_RELEASE = 3U
} btn_fsm_state_t;

typedef struct {
    btn_fsm_state_t state;
    uint32_t        state_enter_ms;
    uint32_t        debounce_ms;
    uint32_t        press_count;
} btn_debouncer_t;

/* --------------------------------------------------------------------------
 * Live Instrument Telemetry Snapshot
 * -------------------------------------------------------------------------- */
typedef struct {
    uint32_t     tick_ms;
    uint32_t     sysclk_hz;
    panel_mode_t mode;
    uint32_t     vdda_mv;
    int32_t      temp_mdegc;
    uint16_t     pot_raw;
    uint32_t     pot_mv;
    uint16_t     pwm_permille;
    uint32_t     adc_errors;
    uint32_t     rx_cmd_count;
    bool         iwdg_reset_prev;
    bool         crash_log_prev;
} panel_telemetry_t;

/* --------------------------------------------------------------------------
 * Inline Helpers
 * -------------------------------------------------------------------------- */

/**
 * @brief Return human-readable constant string for @p mode.
 */
static inline const char *panel_mode_name(panel_mode_t mode) {
    switch (mode) {
        case MODE_IDLE:           return "IDLE";
        case MODE_INTERNAL_TELEM: return "INTERNAL_TELEM";
        case MODE_EXT_POT_PWM:    return "EXT_POT_PWM";
        default:                  return "UNKNOWN";
    }
}

/**
 * @brief Advance to next instrument panel operating mode (wraps around).
 */
static inline panel_mode_t panel_mode_next(panel_mode_t mode) {
    switch (mode) {
        case MODE_IDLE:           return MODE_INTERNAL_TELEM;
        case MODE_INTERNAL_TELEM: return MODE_EXT_POT_PWM;
        case MODE_EXT_POT_PWM:    return MODE_IDLE;
        default:                  return MODE_IDLE;
    }
}

/**
 * @brief Initialize a 4-state non-blocking debouncer FSM.
 */
static inline void btn_debouncer_init(btn_debouncer_t *fsm, uint32_t debounce_ms) {
    fsm->state          = BTN_STATE_RELEASED;
    fsm->state_enter_ms = 0U;
    fsm->debounce_ms    = debounce_ms;
    fsm->press_count    = 0U;
}

/**
 * @brief Step the 4-state debouncer FSM (wraparound-safe SysTick millisecond math).
 * @param fsm         Debouncer state structure.
 * @param raw_active  True when Active-HIGH B1 (PC13) reads logic HIGH.
 * @param now_ms      Monotonic 1 ms SysTick counter.
 * @return true exactly once per confirmed debounced button press.
 */
static inline bool btn_debouncer_step(btn_debouncer_t *fsm, bool raw_active, uint32_t now_ms) {
    bool confirmed_press = false;

    switch (fsm->state) {
        case BTN_STATE_RELEASED:
            if (raw_active) {
                fsm->state          = BTN_STATE_DEBOUNCE_PRESS;
                fsm->state_enter_ms = now_ms;
            }
            break;

        case BTN_STATE_DEBOUNCE_PRESS:
            if (!raw_active) {
                fsm->state = BTN_STATE_RELEASED;
            } else if ((now_ms - fsm->state_enter_ms) >= fsm->debounce_ms) {
                fsm->state = BTN_STATE_PRESSED;
                fsm->press_count++;
                confirmed_press = true;
            } else {
                /* Wait for debounce interval */
            }
            break;

        case BTN_STATE_PRESSED:
            if (!raw_active) {
                fsm->state          = BTN_STATE_DEBOUNCE_RELEASE;
                fsm->state_enter_ms = now_ms;
            }
            break;

        case BTN_STATE_DEBOUNCE_RELEASE:
            if (raw_active) {
                fsm->state = BTN_STATE_PRESSED;
            } else if ((now_ms - fsm->state_enter_ms) >= fsm->debounce_ms) {
                fsm->state = BTN_STATE_RELEASED;
            } else {
                /* Wait for release settle interval */
            }
            break;

        default:
            fsm->state = BTN_STATE_RELEASED;
            break;
    }

    return confirmed_press;
}

/**
 * @brief Map calibrated die temperature (mdegC) to PWM duty in permille [100..1000].
 */
static inline uint16_t panel_temp_to_pwm_permille(int32_t temp_mdegc) {
    if (temp_mdegc <= PANEL_TEMP_MIN_MDEGC) {
        return PANEL_TEMP_MIN_PERMILLE;
    }
    if (temp_mdegc >= PANEL_TEMP_MAX_MDEGC) {
        return PANEL_TEMP_MAX_PERMILLE;
    }

    uint32_t offset = (uint32_t)(temp_mdegc - PANEL_TEMP_MIN_MDEGC);
    uint32_t span   = (uint32_t)(PANEL_TEMP_MAX_MDEGC - PANEL_TEMP_MIN_MDEGC);
    uint32_t range  = (uint32_t)(PANEL_TEMP_MAX_PERMILLE - PANEL_TEMP_MIN_PERMILLE);
    uint32_t duty   = (uint32_t)PANEL_TEMP_MIN_PERMILLE + ((offset * range) / span);
    return (uint16_t)duty;
}

/**
 * @brief Map 12-bit ADC raw code [0..4095] to PWM duty in permille [0..1000].
 */
static inline uint16_t panel_raw_to_pwm_permille(uint16_t raw) {
    uint32_t clamped = (raw > 4095U) ? 4095U : (uint32_t)raw;
    return (uint16_t)((clamped * 1000UL + 2047UL) / 4095UL);
}

#ifdef __cplusplus
}
#endif

#endif /* APP_PANEL_H */
