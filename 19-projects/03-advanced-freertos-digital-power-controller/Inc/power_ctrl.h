/**
 * @file    power_ctrl.h
 * @brief   Project 03 — Digital Power & Motor Controller Q15 fixed-point PI
 *          regulator (with anti-windup clamping), safe onboard buck plant model,
 *          and FreeRTOS task/IPC definitions.
 *
 * Target:  ST NUCLEO-G474RE (STM32G474RET6, Cortex-M4F @ 170 MHz, FreeRTOS V11.1.0)
 * Concepts: APP-PWR-001, RTOS-SCHED-001, RTOS-IPC-001, RTOS-SYNC-001,
 *           RTOS-ISR-001, RTOS-MEM-001, PERIPH-BDTR-002, G4-COMP-001,
 *           PERIPH-DAC-001, EMB-NUM-001, PERIPH-WDG-001, REL-SAFE-001
 */

#ifndef POWER_CTRL_H
#define POWER_CTRL_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* --------------------------------------------------------------------------
 * Operating Mode Switch:
 *   POWER_SIM_MODE = 1 (default): Safe onboard simulation mode on bare
 *     NUCLEO-G474RE (no external power stage required; PA1 pulled down to 0 V
 *     to prevent floating false trips while COMP1 -> TIM1->AF1.BKCMP1E stays
 *     armed and TIM1->EGR.BG exercises the real hardware break path on demand!)
 *   POWER_SIM_MODE = 0: External power stage mode (PA1 analog shunt sense +
 *     external buck/boost half-bridge — EXTERNAL HARDWARE PENDING)
 * -------------------------------------------------------------------------- */
#if !defined(POWER_SIM_MODE)
#define POWER_SIM_MODE              (1U)
#endif

/* External V_OUT voltage-sense scaling for POWER_SIM_MODE = 0 (PA0 = ADC1_IN1):
 *   V_out_mv = (V_adc_pin_mv * POWER_VSENSE_SCALE_NUM) / POWER_VSENSE_SCALE_DEN
 *   Default 10:1 divider (e.g. 90 kOhm / 10 kOhm): 24.0 V -> 2.40 V (< 3.3 V VDDA). */
#if !defined(POWER_VSENSE_SCALE_NUM)
#define POWER_VSENSE_SCALE_NUM      (10U)
#endif
#if !defined(POWER_VSENSE_SCALE_DEN)
#define POWER_VSENSE_SCALE_DEN      (1U)
#endif
#define PWR_VSENSE_ADC_CH           (1U)     /* PA0 = ADC1_IN1 (DS12288 Table 12) */
#define PWR_ADC_VREF_MV             (3300U)  /* Nominal VDDA / VREF+ (mV)         */
#define PWR_ADC_TIMEOUT             (50000U) /* Bounded ADC conversion budget     */

/* --------------------------------------------------------------------------
 * Hardware Timing & Safety Parameters (170 MHz SYSCLK)
 * -------------------------------------------------------------------------- */
#define PWR_TIM1_ARR                (1700U)  /* Center-aligned: 170M / (2*1700) = 50.000 kHz */
#define PWR_TIM1_DTG_500NS          (85U)    /* 85 * (1/170 MHz) = 500.0 ns dead-time        */
#define PWR_DUTY_MIN_PERMILLE       (50U)    /*  5.0% minimum closed-loop duty               */
#define PWR_DUTY_MAX_PERMILLE       (900U)   /* 90.0% maximum closed-loop duty               */

#define PWR_UART_BAUD               (115200U)
#define PWR_UART_TX_TIMEOUT         (200000U)
#define PWR_I2C_TIMEOUT             (50000U)
#define PWR_SSD1306_ADDR7           (0x3CU)
#define PWR_IWDG_TIMEOUT_MS         (1500U)

#define PWR_VIN_NOMINAL_MV          (24000U) /* Simulated 24.0 V DC input bus                */
#define PWR_DAC_THRESH_HW_CODE      (2048U)  /* 1.650 V @ 3.3 V VDDA for external shunt      */
#define PWR_DAC_THRESH_SIM_CODE     (3850U)  /* 3.102 V @ 3.3 V VDDA in safe sim mode        */

/* --------------------------------------------------------------------------
 * FreeRTOS Task Priorities & Stack Budgets (Strictly > 0 for all user tasks!)
 * -------------------------------------------------------------------------- */
#define PWR_PRIO_SAFETY             (4U)     /* CRITICAL: Break fault & IWDG supervisor      */
#define PWR_PRIO_CONTROL            (3U)     /* HIGH:     1 kHz Q15 PI control loop          */
#define PWR_PRIO_TELEMETRY          (2U)     /* MEDIUM:   Mutex-guarded USART2 stream        */
#define PWR_PRIO_UI                 (1U)     /* LOW:      B1 button & optional OLED (NEVER 0)*/

#define PWR_STACK_SAFETY            (256U)   /* 1024 bytes */
#define PWR_STACK_CONTROL           (384U)   /* 1536 bytes */
#define PWR_STACK_TELEMETRY         (512U)   /* 2048 bytes (safe for snprintf formatting)    */
#define PWR_STACK_UI                (384U)   /* 1536 bytes */

/* NVIC Priority for kernel-managed TIM1 break ISR (>= MAX_SYSCALL = 5) */
#define PWR_IRQ_PRIO_TIM1_BRK       (5U)

/* Direct Task Notification bits to SafetySupervisorTask */
#define PWR_NOTIFY_HW_BREAK         (1UL << 0U)
#define PWR_NOTIFY_CLEAR_FAULT_REQ  (1UL << 1U)

/* Watchdog Supervisor Heartbeat Event Group Bits */
#define PWR_EVT_CTRL_ALIVE          (1UL << 0U)
#define PWR_EVT_TELEM_ALIVE         (1UL << 1U)
#define PWR_EVT_UI_ALIVE            (1UL << 2U)
#define PWR_EVT_ALL_TASKS           (PWR_EVT_CTRL_ALIVE | PWR_EVT_TELEM_ALIVE | PWR_EVT_UI_ALIVE)

/* --------------------------------------------------------------------------
 * Fixed-Point Q15 PI Regulator with Anti-Windup Clamping
 *   Representation: Q15 = value * 32768 (1.0 == 32768 in int32_t accumulator)
 * -------------------------------------------------------------------------- */
#define Q15_SHIFT                   (15)
#define Q15_ONE                     (32768)

typedef struct {
    int32_t kp_q15;      /**< Proportional gain in Q15 (e.g. 0.45 -> 14746) */
    int32_t ki_q15;      /**< Integral gain * Ts in Q15 (e.g. 0.08 -> 2621) */
    int32_t integ_q15;   /**< Accumulated integral state (scaled by 2^15)   */
    int32_t out_min;     /**< Lower output saturation bound (permille)      */
    int32_t out_max;     /**< Upper output saturation bound (permille)      */
} q15_pi_ctrl_t;

static inline void q15_pi_init(q15_pi_ctrl_t *pi,
                               int32_t kp_q15,
                               int32_t ki_q15,
                               int32_t out_min,
                               int32_t out_max) {
    pi->kp_q15    = kp_q15;
    pi->ki_q15    = ki_q15;
    pi->integ_q15 = 0;
    pi->out_min   = out_min;
    pi->out_max   = out_max;
}

static inline void q15_pi_reset(q15_pi_ctrl_t *pi) {
    pi->integ_q15 = 0;
}

/**
 * @brief Execute one deterministic fixed-point Q15 PI iteration with conditional
 *        integration anti-windup clamping.
 * @param pi       Controller state.
 * @param err_mv   Signed voltage error (v_ref_mv - v_meas_mv) in millivolts.
 * @return Clamped control output in [out_min .. out_max] (PWM duty permille).
 */
static inline int32_t q15_pi_step(q15_pi_ctrl_t *pi, int32_t err_mv) {
    /* Normalize error to permille-scale units: err_mv / 24 -> ~1000 for 24 V full scale */
    int32_t err_scaled = err_mv / 24;

    /* Proportional term (Q15 -> integer permille via portable ISO C11 signed division) */
    int32_t p_term = (pi->kp_q15 * err_scaled) / Q15_ONE;

    /* Candidate integral update (Q15 accumulator) */
    int32_t cand_integ = pi->integ_q15 + (pi->ki_q15 * err_scaled);
    int32_t integ_min  = pi->out_min * Q15_ONE;
    int32_t integ_max  = pi->out_max * Q15_ONE;
    if (cand_integ > integ_max) {
        cand_integ = integ_max;
    } else if (cand_integ < integ_min) {
        cand_integ = integ_min;
    } else {
        /* Within integrator clamp bounds */
    }

    int32_t i_term = cand_integ / Q15_ONE;
    int32_t raw_u  = p_term + i_term;
    int32_t out_u  = raw_u;

    /* Conditional integration anti-windup: only commit integrator if unsaturated
     * or if error drives the output back inside the linear region */
    bool sat_high = (raw_u > pi->out_max);
    bool sat_low  = (raw_u < pi->out_min);

    if (sat_high) {
        out_u = pi->out_max;
        if (err_scaled < 0) {
            pi->integ_q15 = cand_integ;
        }
    } else if (sat_low) {
        out_u = pi->out_min;
        if (err_scaled > 0) {
            pi->integ_q15 = cand_integ;
        }
    } else {
        pi->integ_q15 = cand_integ;
    }

    return out_u;
}

/* --------------------------------------------------------------------------
 * First-Order Discrete Buck Converter Plant Model (Onboard Simulation Mode)
 * -------------------------------------------------------------------------- */
typedef struct {
    int32_t v_out_mv;
    uint32_t tick_count;
} buck_sim_plant_t;

static inline int32_t buck_sim_step(buck_sim_plant_t *plant,
                                    uint16_t duty_permille,
                                    bool moe_enabled) {
    plant->tick_count++;
    int32_t v_ideal_mv = moe_enabled
                         ? (int32_t)(((uint32_t)duty_permille * PWR_VIN_NOMINAL_MV) / 1000U)
                         : 0;
    /* Periodic small simulated load step (-250 mV every 2000 ms for 200 ms) */
    int32_t load_drop = (((plant->tick_count % 2000U) < 200U) && moe_enabled) ? 250 : 0;
    int32_t target_mv = v_ideal_mv - load_drop;
    if (target_mv < 0) {
        target_mv = 0;
    }
    /* First-order RC/LC filter with tau = 8 ms @ 1 kHz */
    plant->v_out_mv += (target_mv - plant->v_out_mv) / 8;
    return plant->v_out_mv;
}

/* --------------------------------------------------------------------------
 * Queue Telemetry Payload (ControlLoopTask -> TelemetryTask)
 * -------------------------------------------------------------------------- */
typedef struct {
    uint32_t tick_ms;
    int32_t  v_ref_mv;
    int32_t  v_out_mv;
    uint16_t duty_permille;
    uint16_t tim1_ccr1;
    bool     moe_active;
    bool     fault_latched;
    uint32_t break_trips;
    uint32_t wdg_feeds;
} power_telem_msg_t;

#ifdef __cplusplus
}
#endif

#endif /* POWER_CTRL_H */
