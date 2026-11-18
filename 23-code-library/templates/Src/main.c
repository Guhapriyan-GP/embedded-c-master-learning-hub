/**
 * @file    main.c
 * @brief   Bare-Metal Embedded C Starter Application for STM32G474RE (NUCLEO-G474RE).
 *
 * Verified Hardware Connections (UM2505 — MB1367 Rev C):
 *   - User LED (LD2, Green)  : PA5  (Active HIGH, Push-Pull)
 *   - User Button (B1, Blue) : PC13 (Active HIGH with external pull-down resistor!)
 *
 * Features Demonstrated:
 *   - Verified 170 MHz Range 1 Boost Mode clock initialization (RM0440 Section 6.1.5)
 *   - Cortex-M4 DWT->CYCCNT deterministic millisecond delays
 *   - Explicit STM32G4 GPIO configuration (clearing 0b11 Analog reset default)
 *   - Atomic BSRR LED control (ISR-safe)
 *   - Persistent .noinit HardFault crash-log readiness check on boot
 *   - Zero-wait-state .ccmram execution verification helper
 */

#include "clock_g4.h"
#include "fault_handler.h"
#include "gpio_g4.h"
#include "ring_buffer.h"
#include "stm32g474xx.h"

#define LD2_PIN     5U
#define B1_PIN      13U
#define BLINK_FAST  100U
#define BLINK_SLOW  500U

/* Example zero-wait-state function placed in CCM-SRAM (0x10000000) to verify
 * .ccmram Flash-to-CCMRAM copy loop in Reset_Handler and CCMRAM (xrw) flags. */
__attribute__((section(".ccmram"), noinline))
static uint32_t compute_blink_period_ms(bool button_pressed) {
    return button_pressed ? BLINK_FAST : BLINK_SLOW;
}

int main(void) {
    /* 1. Enable MemManage / BusFault / UsageFault + Divide-by-Zero trap */
    FaultHandler_EnableDetailedFaults(true);

    /* If recovering from a prior HardFault, keep g_crash_log inspectable in GDB
     * (or clear after reporting over UART/SWO in production telemetry). */
    volatile bool had_prior_crash = FaultHandler_HasValidCrashLog();
    (void)had_prior_crash;

    /* 2. Switch system clock to 170 MHz Range 1 Boost Mode (RM0440 Section 6.1.5) */
    (void)Clock_Init_170MHz();
    Clock_DWT_Init();

    /* 3. Enable GPIOA and GPIOC AHB2 peripheral clocks */
    GPIO_EnableClock(GPIOA);
    GPIO_EnableClock(GPIOC);

    /* 4. Configure PA5 (LD2 Green LED) as Push-Pull Output, Low Speed, No Pull */
    const gpio_pin_config_t led_cfg = {
        .pin   = LD2_PIN,
        .mode  = GPIO_MODE_OUTPUT,
        .otype = GPIO_OTYPE_PUSHPULL,
        .speed = GPIO_SPEED_LOW,
        .pull  = GPIO_PULL_NONE,
        .af    = 0U
    };
    (void)GPIO_ConfigPin(GPIOA, &led_cfg);

    /* 5. Configure PC13 (B1 User Button) explicitly as Digital Input (MODER = 00).
     *    CRITICAL: STM32G4 resets GPIOC->MODER to 0xFFFFFFFF (Analog 11).
     *    UM2505: NUCLEO-G474RE B1 has an external pull-down and is ACTIVE-HIGH! */
    const gpio_pin_config_t btn_cfg = {
        .pin   = B1_PIN,
        .mode  = GPIO_MODE_INPUT,
        .otype = GPIO_OTYPE_PUSHPULL,
        .speed = GPIO_SPEED_LOW,
        .pull  = GPIO_PULL_NONE,
        .af    = 0U
    };
    (void)GPIO_ConfigPin(GPIOC, &btn_cfg);

    /* Optional ring-buffer smoke initialization (demonstrates reusable driver linkage) */
    static uint8_t rb_storage[32];
    ring_buffer_t event_rb;
    (void)rb_init(&event_rb, rb_storage, (uint32_t)sizeof(rb_storage));

    /* 6. Super Loop: Read Active-HIGH B1 (PC13) and toggle LD2 (PA5) atomically via BSRR */
    while (1) {
        /* Active-HIGH per UM2505: pressed when IDR bit 13 == 1 */
        bool is_button_pressed = ((GPIOC->IDR & (1UL << B1_PIN)) != 0UL);
        uint32_t half_period_ms = compute_blink_period_ms(is_button_pressed);

        (void)rb_push(&event_rb, is_button_pressed ? 1U : 0U);
        if (rb_count(&event_rb) >= 16U) {
            uint8_t sample = 0U;
            (void)rb_pop(&event_rb, &sample);
        }

        /* Atomic LED ON via BSRR bit 5 */
        GPIO_SetPin(GPIOA, LD2_PIN);
        Clock_DelayMs(half_period_ms);

        /* Atomic LED OFF via BSRR bit 21 (5 + 16) */
        GPIO_ResetPin(GPIOA, LD2_PIN);
        Clock_DelayMs(half_period_ms);
    }

    return 0;
}
