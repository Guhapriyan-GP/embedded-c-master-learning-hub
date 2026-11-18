/**
 * @file    spi_g4.h
 * @brief   Bare-metal CMSIS SPI1 / SPI2 / SPI3 Master driver for STM32G474RE.
 *
 * Handles STM32G4-specific silicon details:
 *   - Configures 8-bit frame size (`DS = 0b0111`) AND sets **`SPI_CR2_FRXTH`**
 *     so RXNE asserts after 1 byte (8-bit) instead of 2 bytes (16-bit FIFO threshold).
 *   - Accesses `SPIx->DR` strictly via 8-bit pointer `*(__IO uint8_t *)&spi->DR`
 *     to prevent 16-bit data-packing from clocking 2 bytes per write (RM0440 §42.5.9).
 *   - Dynamically computes BR[2:0] prescaler from live SystemCoreClock + APB clock.
 *   - Provides `spi_wait_idle()` checking `FTLVL == 0 && !BSY` before CS deassertion.
 *
 * Reference: RM0440 Chapter 42 (SPI/I2S), DS12288 Table 2 (SPI1/2/3 on LQFP64).
 * Concepts:  PERIPH-SPI-001
 */

#ifndef SPI_G4_H
#define SPI_G4_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#if !defined(DRIVER_HOST_TEST)
#include "stm32g474xx.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

/** @brief SPI driver status/error codes. */
typedef enum {
    SPI_OK              = 0,  /**< Operation succeeded                             */
    SPI_ERR_INVALID_ARG = -1, /**< NULL pointer, unsupported SPI, or bad argument  */
    SPI_ERR_TIMEOUT     = -2  /**< Bounded TXE / RXNE / BSY polling wait expired   */
} spi_status_t;

/** @brief Standard Motorola SPI clock polarity/phase modes (CPOL, CPHA). */
typedef enum {
    SPI_MODE_0 = 0U, /**< CPOL=0 (idle LOW),  CPHA=0 (sample 1st edge) */
    SPI_MODE_1 = 1U, /**< CPOL=0 (idle LOW),  CPHA=1 (sample 2nd edge) */
    SPI_MODE_2 = 2U, /**< CPOL=1 (idle HIGH), CPHA=0 (sample 1st edge) */
    SPI_MODE_3 = 3U  /**< CPOL=1 (idle HIGH), CPHA=1 (sample 2nd edge) */
} spi_mode_t;

/**
 * @brief Pure host-testable helper: compute smallest SPI_CR1.BR[2:0] code (0..7)
 *        such that `pclk_hz / (2 << br) <= max_sck_hz`.
 *
 * Encoding: BR=0 (/2), 1 (/4), 2 (/8), 3 (/16), 4 (/32), 5 (/64), 6 (/128), 7 (/256).
 *
 * @param pclk_hz     SPI peripheral APB clock frequency in Hz.
 * @param max_sck_hz  Maximum allowable SCK frequency in Hz.
 * @return BR[2:0] register field value (0..7). Returns 7 (/256) if max_sck_hz == 0
 *         or even /256 exceeds max_sck_hz.
 */
uint8_t spi_calc_br_div(uint32_t pclk_hz, uint32_t max_sck_hz);

#if !defined(DRIVER_HOST_TEST)

/**
 * @brief Initialize SPI1, SPI2, or SPI3 as 8-bit full-duplex MSB-first Master with
 *        software NSS (`SSM | SSI`), `DS = 0b0111`, and **`SPI_CR2_FRXTH = 1`**.
 *
 * @note  Caller must configure SCK/MISO/MOSI GPIO pins in Alternate Function mode
 *        with High/Very-High OSPEEDR and manage Chip Select (CS) via GPIO_WritePin().
 *        On NUCLEO-G474RE, PA5 (SPI1_SCK) is shared with Green LED LD2 (SB42).
 *
 * @param spi         SPI1, SPI2, or SPI3 (SPI4 is unbonded on LQFP64).
 * @param mode        SPI_MODE_0 .. SPI_MODE_3.
 * @param max_sck_hz  Maximum target SCK clock in Hz (baud prescaler auto-selected).
 * @return SPI_OK or SPI_ERR_INVALID_ARG.
 */
spi_status_t spi_init_master(SPI_TypeDef *spi, spi_mode_t mode, uint32_t max_sck_hz);

/**
 * @brief Full-duplex single-byte SPI transfer using strict 8-bit DR access
 *        (`*(__IO uint8_t *)&spi->DR`) with bounded timeouts.
 *
 * @param spi            SPI1, SPI2, or SPI3.
 * @param tx_byte        Byte to clock out on MOSI.
 * @param rx_byte        Optional pointer to store simultaneously clocked-in MISO byte (may be NULL).
 * @param timeout_iters  Maximum polling loop iterations per flag (must be > 0).
 * @return SPI_OK, SPI_ERR_INVALID_ARG, or SPI_ERR_TIMEOUT.
 */
spi_status_t spi_transfer_byte(SPI_TypeDef *spi, uint8_t tx_byte,
                               uint8_t *rx_byte, uint32_t timeout_iters);

/**
 * @brief Full-duplex multi-byte SPI buffer transfer with bounded timeouts.
 * @param spi            SPI1, SPI2, or SPI3.
 * @param tx_buf         Transmit buffer (if NULL, sends 0xFF dummy bytes).
 * @param rx_buf         Receive buffer (if NULL, discards received bytes).
 * @param len            Number of bytes to transfer.
 * @param timeout_iters  Maximum polling loop iterations per byte flag (must be > 0).
 * @return SPI_OK, SPI_ERR_INVALID_ARG, or SPI_ERR_TIMEOUT.
 */
spi_status_t spi_transfer_buf(SPI_TypeDef *spi, const uint8_t *tx_buf,
                              uint8_t *rx_buf, size_t len, uint32_t timeout_iters);

/**
 * @brief Wait (bounded) until TX FIFO is completely empty (`FTLVL == 0`) and
 *        shift register is idle (`BSY == 0`). Call BEFORE raising CS HIGH!
 */
spi_status_t spi_wait_idle(const SPI_TypeDef *spi, uint32_t timeout_iters);

#endif /* !defined(DRIVER_HOST_TEST) */

#ifdef __cplusplus
}
#endif

#endif /* SPI_G4_H */
