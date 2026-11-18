/**
 * @file    exti_g4.c
 * @brief   Bare-metal CMSIS EXTI & SYSCFG GPIO interrupt driver implementation.
 */

#include "exti_g4.h"
#include <stddef.h>

bool exti_calc_exticr_index_shift(uint8_t pin, uint8_t *out_reg_idx, uint8_t *out_bit_shift) {
    if ((pin > 15U) || (out_reg_idx == NULL) || (out_bit_shift == NULL)) {
        return false;
    }
    *out_reg_idx   = (uint8_t)(pin / 4U);
    *out_bit_shift = (uint8_t)((pin % 4U) * 4U);
    return true;
}

#if !defined(DRIVER_HOST_TEST)

typedef struct {
    exti_callback_t cb;
    void           *ctx;
} exti_slot_t;

static exti_slot_t s_exti_callbacks[16];

static bool exti_port_to_code(const GPIO_TypeDef *port, uint32_t *out_code) {
    if (port == GPIOA) {
        *out_code = 0U;
    } else if (port == GPIOB) {
        *out_code = 1U;
    } else if (port == GPIOC) {
        *out_code = 2U;
    } else if (port == GPIOD) {
        *out_code = 3U;
    } else if (port == GPIOE) {
        *out_code = 4U;
    } else if (port == GPIOF) {
        *out_code = 5U;
    } else if (port == GPIOG) {
        *out_code = 6U;
    } else {
        return false;
    }
    return true;
}

static IRQn_Type exti_pin_to_irqn(uint8_t pin) {
    switch (pin) {
        case 0U:  return EXTI0_IRQn;
        case 1U:  return EXTI1_IRQn;
        case 2U:  return EXTI2_IRQn;
        case 3U:  return EXTI3_IRQn;
        case 4U:  return EXTI4_IRQn;
        default:  return (pin <= 9U) ? EXTI9_5_IRQn : EXTI15_10_IRQn;
    }
}

exti_status_t EXTI_ConfigLine(const GPIO_TypeDef *port, uint8_t pin,
                              exti_edge_t edge, uint32_t nvic_priority) {
    uint32_t port_code = 0U;
    uint8_t  reg_idx   = 0U;
    uint8_t  bit_shift = 0U;

    if ((port == NULL) ||
        !exti_port_to_code(port, &port_code) ||
        !exti_calc_exticr_index_shift(pin, &reg_idx, &bit_shift)) {
        return EXTI_ERR_INVALID_ARG;
    }

    /* 1. Enable SYSCFG clock on APB2 (+ readback barrier before EXTICR write) */
    RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN;
    (void)RCC->APB2ENR;

    /* 2. Mask line during reconfiguration to prevent spurious glitch interrupts */
    uint32_t line_mask = (1UL << pin);
    EXTI->IMR1 &= ~line_mask;

    /* 3. Route GPIO port to EXTI line in SYSCFG->EXTICR[0..3] (clear nibble first!) */
    SYSCFG->EXTICR[reg_idx] = (SYSCFG->EXTICR[reg_idx] & ~(0xFUL << bit_shift)) |
                              ((port_code & 0xFUL) << bit_shift);

    /* 4. Configure rising and/or falling hardware edge detectors */
    if ((edge == EXTI_EDGE_RISING) || (edge == EXTI_EDGE_BOTH)) {
        EXTI->RTSR1 |= line_mask;
    } else {
        EXTI->RTSR1 &= ~line_mask;
    }

    if ((edge == EXTI_EDGE_FALLING) || (edge == EXTI_EDGE_BOTH)) {
        EXTI->FTSR1 |= line_mask;
    } else {
        EXTI->FTSR1 &= ~line_mask;
    }

    /* 5. Clear any stale latched pending edge via direct rc_w1 write + DSB */
    EXTI_ClearPending(pin);

    /* 6. Unmask interrupt line and enable NVIC vector */
    if (edge != EXTI_EDGE_NONE) {
        EXTI->IMR1 |= line_mask;
        IRQn_Type irqn = exti_pin_to_irqn(pin);
        NVIC_SetPriority(irqn, nvic_priority);
        NVIC_ClearPendingIRQ(irqn);
        NVIC_EnableIRQ(irqn);
    }

    return EXTI_OK;
}

exti_status_t EXTI_DisableLine(uint8_t pin) {
    if (pin > 15U) {
        return EXTI_ERR_INVALID_ARG;
    }
    uint32_t line_mask = (1UL << pin);
    EXTI->IMR1  &= ~line_mask;
    EXTI->RTSR1 &= ~line_mask;
    EXTI->FTSR1 &= ~line_mask;
    EXTI_ClearPending(pin);
    return EXTI_OK;
}

exti_status_t EXTI_RegisterCallback(uint8_t pin, exti_callback_t cb, void *ctx) {
    if (pin > 15U) {
        return EXTI_ERR_INVALID_ARG;
    }
    s_exti_callbacks[pin].cb  = cb;
    s_exti_callbacks[pin].ctx = ctx;
    return EXTI_OK;
}

void EXTI_TriggerSoftware(uint8_t pin) {
    if (pin <= 15U) {
        EXTI->SWIER1 = (1UL << pin);
    }
}

void EXTI_DispatchRange(uint8_t min_line, uint8_t max_line) {
    if ((min_line > max_line) || (max_line > 15U)) {
        return;
    }

    uint32_t active = EXTI->PR1 & EXTI->IMR1;
    for (uint8_t line = min_line; line <= max_line; ++line) {
        if ((active & (1UL << line)) != 0U) {
            /* Clear pending flag BEFORE callback via rc_w1 + readback (ARM AN321 §4.9)
             * + __DSB() write-buffer drain (ARM Erratum 838869 / ES0430 §2.1.3) */
            EXTI_ClearPending(line);
            if (s_exti_callbacks[line].cb != NULL) {
                s_exti_callbacks[line].cb(line, s_exti_callbacks[line].ctx);
            }
        }
    }
    /* Guard any trailing buffered store in user callbacks before ISR BX LR exit (ARM Erratum 838869) */
    __DSB();
}

#endif /* !defined(DRIVER_HOST_TEST) */
