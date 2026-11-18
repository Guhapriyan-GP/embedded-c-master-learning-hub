/**
 * @file    usart_g4.c
 * @brief   Bare-metal CMSIS USART1 / USART2 / LPUART1 driver implementation.
 */

#include "usart_g4.h"

uint32_t usart_calc_brr(uint32_t pclk_hz, uint32_t baud) {
    if ((pclk_hz == 0U) || (baud == 0U)) {
        return 0U;
    }

    uint64_t brr = ((uint64_t)pclk_hz + ((uint64_t)baud / 2ULL)) / (uint64_t)baud;
    if ((brr < 16ULL) || (brr > 0xFFFFULL)) {
        return 0U;
    }
    return (uint32_t)brr;
}

uint32_t lpuart_calc_brr(uint32_t pclk_hz, uint32_t baud) {
    if ((pclk_hz == 0U) || (baud == 0U)) {
        return 0U;
    }

    uint64_t num = (uint64_t)pclk_hz * 256ULL;
    uint64_t brr = (num + ((uint64_t)baud / 2ULL)) / (uint64_t)baud;
    if ((brr < 0x300ULL) || (brr > 0xFFFFFULL)) {
        return 0U;
    }
    return (uint32_t)brr;
}

#if !defined(DRIVER_HOST_TEST)

static uint32_t usart_get_pclk_hz(const USART_TypeDef *usart) {
    if (usart == USART1) {
        uint32_t ppre2 = (RCC->CFGR & RCC_CFGR_PPRE2) >> RCC_CFGR_PPRE2_Pos;
        return SystemCoreClock >> APBPrescTable[ppre2];
    }
    if ((usart == USART2) || (usart == LPUART1)) {
        uint32_t ppre1 = (RCC->CFGR & RCC_CFGR_PPRE1) >> RCC_CFGR_PPRE1_Pos;
        return SystemCoreClock >> APBPrescTable[ppre1];
    }
    return 0U;
}

static bool usart_enable_clock(const USART_TypeDef *usart, IRQn_Type *out_irqn) {
    if (usart == USART1) {
        RCC->APB2ENR |= RCC_APB2ENR_USART1EN;
        (void)RCC->APB2ENR;
        *out_irqn = USART1_IRQn;
        return true;
    }
    if (usart == USART2) {
        RCC->APB1ENR1 |= RCC_APB1ENR1_USART2EN;
        (void)RCC->APB1ENR1;
        *out_irqn = USART2_IRQn;
        return true;
    }
    if (usart == LPUART1) {
        RCC->APB1ENR2 |= RCC_APB1ENR2_LPUART1EN;
        (void)RCC->APB1ENR2;
        *out_irqn = LPUART1_IRQn;
        return true;
    }
    return false;
}

usart_status_t usart_init(USART_TypeDef *usart, uint32_t baud,
                          bool enable_rx_irq, uint32_t nvic_priority) {
    if ((usart == NULL) || (baud == 0U)) {
        return USART_ERR_INVALID_ARG;
    }

    IRQn_Type irqn = USART2_IRQn;
    if (!usart_enable_clock(usart, &irqn)) {
        return USART_ERR_INVALID_ARG;
    }

    uint32_t pclk = usart_get_pclk_hz(usart);
    uint32_t brr  = (usart == LPUART1) ? lpuart_calc_brr(pclk, baud)
                                       : usart_calc_brr(pclk, baud);
    if (brr == 0U) {
        return USART_ERR_BAUD_RANGE;
    }

    /* Disable USART before programming BRR / CR1 / CR2 / CR3 */
    usart->CR1 = 0U;
    usart->CR2 = 0U;
    usart->CR3 = 0U;
    usart->BRR = brr;

    /* Clear any stale status/error flags */
    usart->ICR = USART_ICR_PECF | USART_ICR_FECF | USART_ICR_NECF | USART_ICR_ORECF;

    uint32_t cr1 = USART_CR1_TE | USART_CR1_RE;
    if (enable_rx_irq) {
        cr1 |= USART_CR1_RXNEIE_RXFNEIE;
        NVIC_SetPriority(irqn, nvic_priority);
        NVIC_ClearPendingIRQ(irqn);
        NVIC_EnableIRQ(irqn);
    }

    usart->CR1 = cr1 | USART_CR1_UE;
    return USART_OK;
}

usart_status_t usart_write_byte(USART_TypeDef *usart, uint8_t byte,
                                uint32_t timeout_iters) {
    if ((usart == NULL) || (timeout_iters == 0U)) {
        return USART_ERR_INVALID_ARG;
    }

    uint32_t remaining = timeout_iters;
    while ((usart->ISR & USART_ISR_TXE_TXFNF) == 0U) {
        if (--remaining == 0U) {
            return USART_ERR_TIMEOUT;
        }
    }

    usart->TDR = byte;
    return USART_OK;
}

usart_status_t usart_write_buf(USART_TypeDef *usart, const uint8_t *data,
                               size_t len, uint32_t timeout_iters) {
    if ((usart == NULL) || ((data == NULL) && (len > 0U)) || (timeout_iters == 0U)) {
        return USART_ERR_INVALID_ARG;
    }

    for (size_t i = 0U; i < len; ++i) {
        usart_status_t st = usart_write_byte(usart, data[i], timeout_iters);
        if (st != USART_OK) {
            return st;
        }
    }
    return USART_OK;
}

usart_status_t usart_write_str(USART_TypeDef *usart, const char *str,
                               uint32_t timeout_iters) {
    if ((usart == NULL) || (str == NULL) || (timeout_iters == 0U)) {
        return USART_ERR_INVALID_ARG;
    }

    while (*str != '\0') {
        usart_status_t st = usart_write_byte(usart, (uint8_t)(*str), timeout_iters);
        if (st != USART_OK) {
            return st;
        }
        ++str;
    }
    return USART_OK;
}

usart_status_t usart_flush_tx(const USART_TypeDef *usart, uint32_t timeout_iters) {
    if ((usart == NULL) || (timeout_iters == 0U)) {
        return USART_ERR_INVALID_ARG;
    }

    uint32_t remaining = timeout_iters;
    while ((usart->ISR & USART_ISR_TC) == 0U) {
        if (--remaining == 0U) {
            return USART_ERR_TIMEOUT;
        }
    }
    return USART_OK;
}

uint32_t usart_clear_errors(USART_TypeDef *usart) {
    if (usart == NULL) {
        return USART_ERRFLAG_NONE;
    }

    uint32_t isr   = usart->ISR;
    uint32_t flags = USART_ERRFLAG_NONE;
    uint32_t icr   = 0U;

    if ((isr & USART_ISR_PE) != 0U) {
        flags |= USART_ERRFLAG_PE;
        icr   |= USART_ICR_PECF;
    }
    if ((isr & USART_ISR_FE) != 0U) {
        flags |= USART_ERRFLAG_FE;
        icr   |= USART_ICR_FECF;
    }
    if ((isr & USART_ISR_NE) != 0U) {
        flags |= USART_ERRFLAG_NE;
        icr   |= USART_ICR_NECF;
    }
    if ((isr & USART_ISR_ORE) != 0U) {
        flags |= USART_ERRFLAG_ORE;
        icr   |= USART_ICR_ORECF;
    }

    if (icr != 0U) {
        usart->ICR = icr;
    }
    return flags;
}

void usart_rx_irq_handler(USART_TypeDef *usart, ring_buffer_t *rx_rb) {
    if (usart == NULL) {
        return;
    }

    /* Clear ORE/FE/NE/PE first so an overrun error never locks up RXNE interrupts */
    (void)usart_clear_errors(usart);

    while ((usart->ISR & USART_ISR_RXNE_RXFNE) != 0U) {
        uint8_t rx_byte = (uint8_t)(usart->RDR & 0xFFU);
        if (rx_rb != NULL) {
            (void)rb_push(rx_rb, rx_byte);
        }
    }
    __DSB();
}

#endif /* !defined(DRIVER_HOST_TEST) */
