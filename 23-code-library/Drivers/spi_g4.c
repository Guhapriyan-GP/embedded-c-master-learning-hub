/**
 * @file    spi_g4.c
 * @brief   Bare-metal CMSIS SPI1 / SPI2 / SPI3 Master driver implementation.
 */

#include "spi_g4.h"

uint8_t spi_calc_br_div(uint32_t pclk_hz, uint32_t max_sck_hz) {
    if ((pclk_hz == 0U) || (max_sck_hz == 0U)) {
        return 7U;
    }

    for (uint8_t br = 0U; br < 7U; ++br) {
        uint64_t div = 2ULL << br;
        /* Exact ceiling check: guarantees pclk_hz / div <= max_sck_hz with zero overshoot */
        if ((uint64_t)pclk_hz <= ((uint64_t)max_sck_hz * div)) {
            return br;
        }
    }
    return 7U;
}

#if !defined(DRIVER_HOST_TEST)

static uint32_t spi_get_pclk_hz(const SPI_TypeDef *spi) {
    if (spi == SPI1) {
        uint32_t ppre2 = (RCC->CFGR & RCC_CFGR_PPRE2) >> RCC_CFGR_PPRE2_Pos;
        return SystemCoreClock >> APBPrescTable[ppre2];
    }
    if ((spi == SPI2) || (spi == SPI3)) {
        uint32_t ppre1 = (RCC->CFGR & RCC_CFGR_PPRE1) >> RCC_CFGR_PPRE1_Pos;
        return SystemCoreClock >> APBPrescTable[ppre1];
    }
    return 0U;
}

static bool spi_enable_clock(const SPI_TypeDef *spi) {
    if (spi == SPI1) {
        RCC->APB2ENR |= RCC_APB2ENR_SPI1EN;
        (void)RCC->APB2ENR;
        return true;
    }
    if (spi == SPI2) {
        RCC->APB1ENR1 |= RCC_APB1ENR1_SPI2EN;
        (void)RCC->APB1ENR1;
        return true;
    }
    if (spi == SPI3) {
        RCC->APB1ENR1 |= RCC_APB1ENR1_SPI3EN;
        (void)RCC->APB1ENR1;
        return true;
    }
    return false;
}

spi_status_t spi_init_master(SPI_TypeDef *spi, spi_mode_t mode, uint32_t max_sck_hz) {
    if ((spi == NULL) || ((uint32_t)mode > (uint32_t)SPI_MODE_3) || (max_sck_hz == 0U)) {
        return SPI_ERR_INVALID_ARG;
    }
    if (!spi_enable_clock(spi)) {
        return SPI_ERR_INVALID_ARG;
    }

    uint32_t pclk    = spi_get_pclk_hz(spi);
    uint8_t  br_code = spi_calc_br_div(pclk, max_sck_hz);

    /* 1. Disable SPI before reconfiguring CR1/CR2 */
    spi->CR1 &= ~SPI_CR1_SPE;

    /* 2. Configure Master mode, software NSS (SSM|SSI avoids MODF), BR, CPOL, CPHA */
    uint32_t cr1 = SPI_CR1_MSTR |
                   SPI_CR1_SSM |
                   SPI_CR1_SSI |
                   (((uint32_t)br_code & 0x7UL) << SPI_CR1_BR_Pos);

    if ((mode == SPI_MODE_2) || (mode == SPI_MODE_3)) {
        cr1 |= SPI_CR1_CPOL;
    }
    if ((mode == SPI_MODE_1) || (mode == SPI_MODE_3)) {
        cr1 |= SPI_CR1_CPHA;
    }
    spi->CR1 = cr1;

    /* 3. Configure 8-bit Data Size (DS = 0b0111) AND FRXTH = 1 (8-bit RXNE threshold!) */
    spi->CR2 = (7UL << SPI_CR2_DS_Pos) | SPI_CR2_FRXTH;

    /* 4. Enable SPI peripheral */
    spi->CR1 |= SPI_CR1_SPE;
    return SPI_OK;
}

spi_status_t spi_transfer_byte(SPI_TypeDef *spi, uint8_t tx_byte,
                               uint8_t *rx_byte, uint32_t timeout_iters) {
    if ((spi == NULL) || (timeout_iters == 0U)) {
        return SPI_ERR_INVALID_ARG;
    }

    /* 1. Wait for TX FIFO space (TXE) */
    uint32_t remaining = timeout_iters;
    while ((spi->SR & SPI_SR_TXE) == 0U) {
        if (--remaining == 0U) {
            return SPI_ERR_TIMEOUT;
        }
    }

    /* 2. Strictly 8-bit MMIO write to DR to prevent STM32G4 16-bit FIFO data packing */
    *(__IO uint8_t *)&spi->DR = tx_byte;

    /* 3. Wait for RX FIFO >= 1 byte (RXNE triggers at 8-bit because FRXTH=1) */
    remaining = timeout_iters;
    while ((spi->SR & SPI_SR_RXNE) == 0U) {
        if (--remaining == 0U) {
            return SPI_ERR_TIMEOUT;
        }
    }

    /* 4. Strictly 8-bit MMIO read from DR */
    uint8_t rx = *(__IO uint8_t *)&spi->DR;
    if (rx_byte != NULL) {
        *rx_byte = rx;
    }

    return SPI_OK;
}

spi_status_t spi_transfer_buf(SPI_TypeDef *spi, const uint8_t *tx_buf,
                              uint8_t *rx_buf, size_t len, uint32_t timeout_iters) {
    if ((spi == NULL) || (timeout_iters == 0U)) {
        return SPI_ERR_INVALID_ARG;
    }

    for (size_t i = 0U; i < len; ++i) {
        uint8_t tx = (tx_buf != NULL) ? tx_buf[i] : 0xFFU;
        uint8_t rx = 0U;
        spi_status_t st = spi_transfer_byte(spi, tx, &rx, timeout_iters);
        if (st != SPI_OK) {
            return st;
        }
        if (rx_buf != NULL) {
            rx_buf[i] = rx;
        }
    }
    return SPI_OK;
}

spi_status_t spi_wait_idle(const SPI_TypeDef *spi, uint32_t timeout_iters) {
    if ((spi == NULL) || (timeout_iters == 0U)) {
        return SPI_ERR_INVALID_ARG;
    }

    uint32_t remaining = timeout_iters;
    while (((spi->SR & SPI_SR_FTLVL) != 0U) || ((spi->SR & SPI_SR_BSY) != 0U)) {
        if (--remaining == 0U) {
            return SPI_ERR_TIMEOUT;
        }
    }
    return SPI_OK;
}

#endif /* !defined(DRIVER_HOST_TEST) */
