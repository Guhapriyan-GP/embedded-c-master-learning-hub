/**
 * @file    dma_g4.h
 * @brief   Bare-metal CMSIS DMA1 / DMA2 + DMAMUX1 driver for STM32G474RE.
 *
 * Handles STM32G4-specific silicon details:
 *   - Always enables `RCC->AHB1ENR.DMAMUX1EN` alongside `DMA1EN`/`DMA2EN`
 *     (forgetting `DMAMUX1EN` silently stalls all peripheral DMA requests!).
 *   - Routes DMA1 Ch1..8 -> DMAMUX1 Ch0..7 and DMA2 Ch1..8 -> DMAMUX1 Ch8..15.
 *   - Uses explicit CMSIS channel pointer tables (avoiding the 0x14 vs 0x10
 *     reserved-word stride trap between adjacent `DMA_Channel_TypeDef` blocks).
 *   - **ES0430 §2.3.1 Compliance:** Clears ONLY individual `CTCIFx`/`CHTIFx`/`CTEIFx`
 *     bits in `DMAx->IFCR` — NEVER writes `CGIFx`!
 *   - Provides safe synchronous `dma_mem2mem_sync()` (ideal for self-tests) and
 *     circular peripheral-to-memory setup `dma_config_p2m_circular()`.
 *
 * @note  STM32G474RE CCM-SRAM Warning (RM0440 §2.3): DMA1/DMA2 CANNOT access
 *        CCM-SRAM at native `0x10000000`; use SRAM1/SRAM2 (`0x20000000..0x20017FFF`)
 *        or the S-bus CCM alias (`0x20018000..0x2001FFFF`).
 *
 * Reference: RM0440 Ch. 11 (DMA), Ch. 12 (DMAMUX, Table 91), ES0430 §2.3.1.
 * Concepts:  PERIPH-DMA-001, PERIPH-DMA-002, STM32-ERRATA-001
 */

#ifndef DMA_G4_H
#define DMA_G4_H

#include <stdbool.h>
#include <stdint.h>

#if !defined(DRIVER_HOST_TEST)
#include "stm32g474xx.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

/** @brief DMA driver status/error codes. */
typedef enum {
    DMA_OK              = 0,  /**< Operation succeeded                             */
    DMA_ERR_INVALID_ARG = -1, /**< Bad DMA instance, channel (not 1..8), width/ptr */
    DMA_ERR_TRANSFER    = -2, /**< Hardware transfer error flag (TEIF) asserted    */
    DMA_ERR_TIMEOUT     = -3  /**< Bounded polling wait expired before TCIF        */
} dma_status_t;

/**
 * @brief Verified STM32G474RE DMAMUX1 Request IDs (RM0440 Table 91).
 * @note  USART RX/TX order verified: RX is even, TX is odd!
 */
typedef enum {
    DMAMUX_REQ_MEM2MEM    = 0U,
    DMAMUX_REQ_ADC1       = 5U,
    DMAMUX_REQ_DAC1_CH1   = 6U,
    DMAMUX_REQ_DAC1_CH2   = 7U,
    DMAMUX_REQ_TIM6_UP    = 8U,
    DMAMUX_REQ_TIM7_UP    = 9U,
    DMAMUX_REQ_SPI1_RX    = 10U,
    DMAMUX_REQ_SPI1_TX    = 11U,
    DMAMUX_REQ_SPI2_RX    = 12U,
    DMAMUX_REQ_SPI2_TX    = 13U,
    DMAMUX_REQ_SPI3_RX    = 14U,
    DMAMUX_REQ_SPI3_TX    = 15U,
    DMAMUX_REQ_I2C1_RX    = 16U,
    DMAMUX_REQ_I2C1_TX    = 17U,
    DMAMUX_REQ_I2C2_RX    = 18U,
    DMAMUX_REQ_I2C2_TX    = 19U,
    DMAMUX_REQ_I2C3_RX    = 20U,
    DMAMUX_REQ_I2C3_TX    = 21U,
    DMAMUX_REQ_USART1_RX  = 24U,
    DMAMUX_REQ_USART1_TX  = 25U,
    DMAMUX_REQ_USART2_RX  = 26U,
    DMAMUX_REQ_USART2_TX  = 27U,
    DMAMUX_REQ_LPUART1_RX = 34U,
    DMAMUX_REQ_LPUART1_TX = 35U,
    DMAMUX_REQ_ADC2       = 36U
} dmamux_req_id_t;

/**
 * @brief Pure host-testable helper: map STM32G474RE DMA controller (1 or 2) and
 *        1-based channel number (1..8) to its 0-based DMAMUX1 channel index (0..15).
 *
 * Mapping (RM0440 §12.3.2):
 *   - DMA1 Ch1..8 -> DMAMUX1 Ch0..7
 *   - DMA2 Ch1..8 -> DMAMUX1 Ch8..15
 *
 * @param dma_num  1 for DMA1, 2 for DMA2.
 * @param ch_num   Channel number 1..8.
 * @return DMAMUX1 channel index 0..15, or -1 if arguments are out of range.
 */
int8_t dma_dmamux_channel_index(uint8_t dma_num, uint8_t ch_num);

#if !defined(DRIVER_HOST_TEST)

/**
 * @brief Enable AHB1 clocks for @p dma (DMA1 or DMA2) AND DMAMUX1 (+ readback barrier).
 */
void dma_enable_clocks(const DMA_TypeDef *dma);

/**
 * @brief Clear individual channel interrupt flags in `DMAx->IFCR` without ever
 *        touching `CGIFx` (strictly honoring STM32G4 Errata ES0430 §2.3.1).
 *
 * @param dma       DMA1 or DMA2.
 * @param ch_num    Channel number 1..8.
 * @param clear_tc  Clear Transfer Complete flag (CTCIFx).
 * @param clear_ht  Clear Half Transfer flag (CHTIFx).
 * @param clear_te  Clear Transfer Error flag (CTEIFx).
 */
void dma_clear_flags(DMA_TypeDef *dma, uint8_t ch_num,
                     bool clear_tc, bool clear_ht, bool clear_te);

/**
 * @brief Configure a circular Peripheral-to-Memory DMA channel with DMAMUX1 routing
 *        (ideal for continuous ADC ping-pong or USART circular RX).
 *
 * @param dma               DMA1 or DMA2.
 * @param ch_num            Channel number 1..8.
 * @param dmamux_req_id     Verified DMAMUX1 request ID (e.g. DMAMUX_REQ_ADC1).
 * @param periph_addr       Peripheral data register address (e.g. `&ADC1->DR`).
 * @param mem_addr          Destination SRAM buffer (must be DMA-accessible!).
 * @param count             Number of transfers (elements, 1..65535).
 * @param width_bytes       Element width in bytes: 1 (8-bit), 2 (16-bit), or 4 (32-bit).
 * @param enable_ht_tc_irq  If true, enables HTIE + TCIE + TEIE and NVIC IRQ.
 * @param nvic_priority     Encoded NVIC priority (ignored if enable_ht_tc_irq is false).
 * @return DMA_OK or DMA_ERR_INVALID_ARG.
 */
dma_status_t dma_config_p2m_circular(DMA_TypeDef *dma, uint8_t ch_num,
                                     uint8_t dmamux_req_id,
                                     volatile const void *periph_addr,
                                     volatile void *mem_addr,
                                     uint16_t count, uint8_t width_bytes,
                                     bool enable_ht_tc_irq, uint32_t nvic_priority);

/**
 * @brief Execute a synchronous Memory-to-Memory (`MEM2MEM`) DMA transfer with
 *        bounded polling timeout and Transfer Error (`TEIF`) verification.
 *
 * @param dma            DMA1 or DMA2.
 * @param ch_num         Channel number 1..8.
 * @param src            Source buffer address (Flash or SRAM1/2/CCM-alias).
 * @param dst            Destination buffer address (SRAM1/2/CCM-alias).
 * @param count          Number of elements to copy (1..65535).
 * @param width_bytes    Element width: 1, 2, or 4 bytes (src/dst must be aligned!).
 * @param timeout_iters  Bounded polling iteration budget for TCIF.
 * @return DMA_OK, DMA_ERR_INVALID_ARG, DMA_ERR_TRANSFER, or DMA_ERR_TIMEOUT.
 */
dma_status_t dma_mem2mem_sync(DMA_TypeDef *dma, uint8_t ch_num,
                              const void *src, void *dst,
                              uint16_t count, uint8_t width_bytes,
                              uint32_t timeout_iters);

#endif /* !defined(DRIVER_HOST_TEST) */

#ifdef __cplusplus
}
#endif

#endif /* DMA_G4_H */
