/**
 * @file    i2c_g4.c
 * @brief   Bare-metal CMSIS STM32G4 v2 I2C1 / I2C2 / I2C3 Master driver implementation.
 */

#include "i2c_g4.h"

#if !defined(DRIVER_HOST_TEST)
#include "clock_g4.h"
#include "gpio_g4.h"
#endif

uint32_t i2c_select_timingr(uint32_t i2cclk_hz, i2c_speed_t speed) {
    if (speed == I2C_SPEED_100KHZ) {
        if (i2cclk_hz == 170000000UL) {
            return I2C_TIMINGR_170MHZ_100KHZ;
        }
        if (i2cclk_hz == 160000000UL) {
            return I2C_TIMINGR_160MHZ_100KHZ;
        }
        if (i2cclk_hz == 16000000UL) {
            return I2C_TIMINGR_16MHZ_100KHZ;
        }
    } else if (speed == I2C_SPEED_400KHZ) {
        if (i2cclk_hz == 170000000UL) {
            return I2C_TIMINGR_170MHZ_400KHZ;
        }
        if (i2cclk_hz == 160000000UL) {
            return I2C_TIMINGR_160MHZ_400KHZ;
        }
        if (i2cclk_hz == 16000000UL) {
            return I2C_TIMINGR_16MHZ_400KHZ;
        }
    }
    return 0U;
}

#if !defined(DRIVER_HOST_TEST)

static uint32_t i2c_get_pclk1_hz(void) {
    uint32_t ppre1 = (RCC->CFGR & RCC_CFGR_PPRE1) >> RCC_CFGR_PPRE1_Pos;
    return SystemCoreClock >> APBPrescTable[ppre1];
}

static bool i2c_enable_clock(const I2C_TypeDef *i2c) {
    uint32_t bit = 0U;
    if (i2c == I2C1) {
        bit = RCC_APB1ENR1_I2C1EN;
    } else if (i2c == I2C2) {
        bit = RCC_APB1ENR1_I2C2EN;
    } else if (i2c == I2C3) {
        bit = RCC_APB1ENR1_I2C3EN;
    } else {
        return false;
    }
    RCC->APB1ENR1 |= bit;
    (void)RCC->APB1ENR1;
    return true;
}

static i2c_status_t i2c_wait_not_busy(const I2C_TypeDef *i2c, uint32_t timeout_iters) {
    uint32_t remaining = timeout_iters;
    while ((i2c->ISR & I2C_ISR_BUSY) != 0U) {
        if (--remaining == 0U) {
            return I2C_ERR_BUSY;
        }
    }
    return I2C_OK;
}

/**
 * @brief Wait until @p target_flag asserts, while continuously monitoring NACKF, BERR, ARLO.
 */
static i2c_status_t i2c_wait_flag(I2C_TypeDef *i2c, uint32_t target_flag,
                                  uint32_t timeout_iters) {
    uint32_t remaining = timeout_iters;

    while (remaining > 0U) {
        uint32_t isr = i2c->ISR;

        if ((isr & I2C_ISR_NACKF) != 0U) {
            i2c->ICR = I2C_ICR_NACKCF;
            /* STM32G4 I2C master automatically generates STOP after NACK; wait bounded */
            uint32_t stop_wait = timeout_iters;
            while (((i2c->ISR & I2C_ISR_STOPF) == 0U) && (stop_wait > 0U)) {
                --stop_wait;
            }
            i2c->ICR = I2C_ICR_STOPCF;
            /* Flush any unshifted byte remaining in TXDR (RM0440 §39.7.7) */
            i2c->ISR = I2C_ISR_TXE;
            return I2C_ERR_NACK;
        }
        if ((isr & I2C_ISR_BERR) != 0U) {
            i2c->ICR = I2C_ICR_BERRCF;
            return I2C_ERR_BUS;
        }
        if ((isr & I2C_ISR_ARLO) != 0U) {
            i2c->ICR = I2C_ICR_ARLOCF;
            return I2C_ERR_ARLO;
        }
        if ((isr & target_flag) != 0U) {
            return I2C_OK;
        }
        --remaining;
    }

    return I2C_ERR_TIMEOUT;
}

static i2c_status_t i2c_wait_stop_and_clear(I2C_TypeDef *i2c, uint32_t timeout_iters) {
    i2c_status_t st = i2c_wait_flag(i2c, I2C_ISR_STOPF, timeout_iters);
    if (st == I2C_OK) {
        i2c->ICR = I2C_ICR_STOPCF;
    }
    return st;
}

i2c_status_t i2c_init_master(I2C_TypeDef *i2c, i2c_speed_t speed) {
    if ((i2c == NULL) || ((speed != I2C_SPEED_100KHZ) && (speed != I2C_SPEED_400KHZ))) {
        return I2C_ERR_INVALID_ARG;
    }
    if (!i2c_enable_clock(i2c)) {
        return I2C_ERR_INVALID_ARG;
    }

    uint32_t pclk1   = i2c_get_pclk1_hz();
    uint32_t timingr = i2c_select_timingr(pclk1, speed);
    if (timingr == 0U) {
        return I2C_ERR_UNSUP_CLK;
    }

    /* Disable peripheral before updating TIMINGR (RM0440 §39.4.9 requirement) */
    i2c->CR1 &= ~I2C_CR1_PE;

    /* Keep Analog Noise Filter enabled (ANFOFF = 0), Digital Filter = 0 */
    i2c->CR1     = 0U;
    i2c->TIMINGR = timingr;
    i2c->ICR     = I2C_ICR_NACKCF | I2C_ICR_STOPCF | I2C_ICR_BERRCF | I2C_ICR_ARLOCF;

    /* Enable peripheral */
    i2c->CR1 |= I2C_CR1_PE;
    return I2C_OK;
}

static i2c_status_t i2c_write_stream(I2C_TypeDef *i2c, uint8_t addr7,
                                     bool has_prefix, uint8_t prefix_byte,
                                     const uint8_t *data, size_t len,
                                     uint32_t timeout_iters) {
    size_t total = len + (has_prefix ? 1U : 0U);
    if ((i2c == NULL) || (addr7 > 0x7FU) || (total == 0U) ||
        ((len > 0U) && (data == NULL)) || (timeout_iters == 0U)) {
        return I2C_ERR_INVALID_ARG;
    }

    i2c_status_t st = i2c_wait_not_busy(i2c, timeout_iters);
    if (st != I2C_OK) {
        return st;
    }

    i2c->ICR = I2C_ICR_NACKCF | I2C_ICR_STOPCF | I2C_ICR_BERRCF | I2C_ICR_ARLOCF;

    size_t remaining  = total;
    size_t data_idx   = 0U;
    bool   first      = true;
    bool   send_pref  = has_prefix;

    while (remaining > 0U) {
        uint32_t chunk = (remaining > 255U) ? 255U : (uint32_t)remaining;
        bool     more  = (remaining > 255U);

        uint32_t cr2 = (((uint32_t)addr7 << 1U) & I2C_CR2_SADD_Msk) |
                       ((chunk << I2C_CR2_NBYTES_Pos) & I2C_CR2_NBYTES_Msk);
        if (more) {
            cr2 |= I2C_CR2_RELOAD;
        } else {
            cr2 |= I2C_CR2_AUTOEND;
        }
        if (first) {
            cr2  |= I2C_CR2_START;
            first = false;
        }
        i2c->CR2 = cr2;

        for (uint32_t i = 0U; i < chunk; ++i) {
            st = i2c_wait_flag(i2c, I2C_ISR_TXIS, timeout_iters);
            if (st != I2C_OK) {
                return st;
            }
            if (send_pref) {
                i2c->TXDR = prefix_byte;
                send_pref = false;
            } else {
                i2c->TXDR = data[data_idx++];
            }
        }

        remaining -= chunk;
        if (more) {
            st = i2c_wait_flag(i2c, I2C_ISR_TCR, timeout_iters);
            if (st != I2C_OK) {
                return st;
            }
        }
    }

    return i2c_wait_stop_and_clear(i2c, timeout_iters);
}

i2c_status_t i2c_write(I2C_TypeDef *i2c, uint8_t addr7, const uint8_t *data,
                       size_t len, uint32_t timeout_iters) {
    return i2c_write_stream(i2c, addr7, false, 0U, data, len, timeout_iters);
}

i2c_status_t i2c_write_reg(I2C_TypeDef *i2c, uint8_t addr7, uint8_t reg,
                           const uint8_t *data, size_t len, uint32_t timeout_iters) {
    return i2c_write_stream(i2c, addr7, true, reg, data, len, timeout_iters);
}

i2c_status_t i2c_read_reg(I2C_TypeDef *i2c, uint8_t addr7, uint8_t reg,
                          uint8_t *out_data, size_t len, uint32_t timeout_iters) {
    if ((i2c == NULL) || (addr7 > 0x7FU) || (out_data == NULL) ||
        (len == 0U) || (timeout_iters == 0U)) {
        return I2C_ERR_INVALID_ARG;
    }

    i2c_status_t st = i2c_wait_not_busy(i2c, timeout_iters);
    if (st != I2C_OK) {
        return st;
    }

    i2c->ICR = I2C_ICR_NACKCF | I2C_ICR_STOPCF | I2C_ICR_BERRCF | I2C_ICR_ARLOCF;

    /* Phase 1: Write 1-byte register pointer with AUTOEND = 0 (enables Repeated START) */
    i2c->CR2 = (((uint32_t)addr7 << 1U) & I2C_CR2_SADD_Msk) |
               (1UL << I2C_CR2_NBYTES_Pos) |
               I2C_CR2_START;

    st = i2c_wait_flag(i2c, I2C_ISR_TXIS, timeout_iters);
    if (st != I2C_OK) {
        return st;
    }
    i2c->TXDR = reg;

    st = i2c_wait_flag(i2c, I2C_ISR_TC, timeout_iters);
    if (st != I2C_OK) {
        return st;
    }

    /* Phase 2: Repeated START in read direction (RD_WRN = 1), supporting >255B RELOAD */
    size_t remaining = len;
    size_t out_idx   = 0U;
    bool   first     = true;

    while (remaining > 0U) {
        uint32_t chunk = (remaining > 255U) ? 255U : (uint32_t)remaining;
        bool     more  = (remaining > 255U);

        uint32_t cr2 = (((uint32_t)addr7 << 1U) & I2C_CR2_SADD_Msk) |
                       I2C_CR2_RD_WRN |
                       ((chunk << I2C_CR2_NBYTES_Pos) & I2C_CR2_NBYTES_Msk);
        if (more) {
            cr2 |= I2C_CR2_RELOAD;
        } else {
            cr2 |= I2C_CR2_AUTOEND;
        }
        if (first) {
            cr2  |= I2C_CR2_START;
            first = false;
        }
        i2c->CR2 = cr2;

        for (uint32_t i = 0U; i < chunk; ++i) {
            st = i2c_wait_flag(i2c, I2C_ISR_RXNE, timeout_iters);
            if (st != I2C_OK) {
                return st;
            }
            out_data[out_idx++] = (uint8_t)(i2c->RXDR & 0xFFU);
        }

        remaining -= chunk;
        if (more) {
            st = i2c_wait_flag(i2c, I2C_ISR_TCR, timeout_iters);
            if (st != I2C_OK) {
                return st;
            }
        }
    }

    return i2c_wait_stop_and_clear(i2c, timeout_iters);
}

bool i2c_recover_bus_9clks(GPIO_TypeDef *scl_port, uint8_t scl_pin,
                           GPIO_TypeDef *sda_port, uint8_t sda_pin) {
    if ((scl_port == NULL) || (sda_port == NULL) || (scl_pin > 15U) || (sda_pin > 15U)) {
        return false;
    }

    GPIO_EnableClock(scl_port);
    GPIO_EnableClock(sda_port);

    /* Drive both lines open-drain HIGH (released) before toggling */
    GPIO_SetPin(scl_port, scl_pin);
    GPIO_SetPin(sda_port, sda_pin);
    (void)GPIO_ConfigOutput(scl_port, scl_pin, GPIO_OTYPE_OPENDRAIN,
                            GPIO_SPEED_MEDIUM, GPIO_PULL_UP);
    (void)GPIO_ConfigOutput(sda_port, sda_pin, GPIO_OTYPE_OPENDRAIN,
                            GPIO_SPEED_MEDIUM, GPIO_PULL_UP);
    Clock_DelayUs(5U);

    /* Clock up to 9 SCL pulses (~100 kHz) until slave releases SDA HIGH */
    for (uint8_t clk = 0U; clk < 9U; ++clk) {
        if (GPIO_ReadPin(sda_port, sda_pin)) {
            break;
        }
        GPIO_ResetPin(scl_port, scl_pin);
        Clock_DelayUs(5U);
        GPIO_SetPin(scl_port, scl_pin);
        Clock_DelayUs(5U);
    }

    /* Generate a clean STOP condition: SDA LOW -> SCL HIGH -> SDA HIGH */
    GPIO_ResetPin(scl_port, scl_pin);
    Clock_DelayUs(5U);
    GPIO_ResetPin(sda_port, sda_pin);
    Clock_DelayUs(5U);
    GPIO_SetPin(scl_port, scl_pin);
    Clock_DelayUs(5U);
    GPIO_SetPin(sda_port, sda_pin);
    Clock_DelayUs(5U);

    return GPIO_ReadPin(sda_port, sda_pin);
}

#endif /* !defined(DRIVER_HOST_TEST) */
