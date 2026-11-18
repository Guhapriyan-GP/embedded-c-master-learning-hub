/**
 * @file    logger_packet.h
 * @brief   Project 02 — High-Speed Multichannel DMA Data Logger packet layout,
 *          ping-pong buffer geometry, and Dual-Bank Flash Bank 2 64-bit staging.
 *
 * Target:  ST NUCLEO-G474RE (STM32G474RET6, Cortex-M4F @ 170 MHz)
 * Concepts: PERIPH-DMA-001, PERIPH-DMA-002, STM32-ERRATA-001, PERIPH-ADC-001,
 *           PERIPH-SYNC-001, PERIPH-CRC-001, PERIPH-I2C-001, STM32-FLASH-001,
 *           EMB-VOL-001, EMB-BAR-001, PERIPH-WDG-001
 */

#ifndef LOGGER_PACKET_H
#define LOGGER_PACKET_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

/* --------------------------------------------------------------------------
 * Acquisition & Peripheral Configuration Constants
 * -------------------------------------------------------------------------- */
#define LOGGER_SAMPLE_RATE_HZ       (100U)    /* TIM3 TRGO periodic scan rate (Hz) */
#define LOGGER_NUM_CHANNELS         (2U)      /* Rank 1: VREFINT (18), Rank 2: Temp (16) */
#define LOGGER_SCANS_PER_HALF       (16U)     /* 16 scans @ 100 Hz = 160 ms per half */
#define LOGGER_HALF_WORDS           (LOGGER_NUM_CHANNELS * LOGGER_SCANS_PER_HALF) /* 32 */
#define LOGGER_FULL_WORDS           (LOGGER_HALF_WORDS * 2U)                      /* 64 */

#define LOGGER_UART_BAUD            (115200U)
#define LOGGER_UART_TX_TIMEOUT      (200000U)
#define LOGGER_I2C_TIMEOUT          (50000U)
#define LOGGER_ADC_TIMEOUT          (200000U)
#define LOGGER_IWDG_TIMEOUT_MS      (1500U)

#define LOGGER_FRAME_MAGIC          (0x4C4F4732UL) /* ASCII "LOG2" */
#define LOGGER_CRC32_CHECK_EXPECTED (0xCBF43926UL) /* IEEE 802.3 CRC-32("123456789") */

/* Optional SSD1306 OLED 7-bit I2C1 address (PA15 SCL / PB7 SDA, AF4) */
#define LOGGER_SSD1306_ADDR7        (0x3CU)

/* STM32G474RE Dual-Bank Flash Bank 2 physical geometry (RM0440 §3.3.1) */
#define LOGGER_FLASH_BANK2_BASE     (0x08040000UL)
#define LOGGER_FLASH_BANK2_END      (0x08080000UL)
#define LOGGER_FLASH_PAGE_SIZE      (2048U)        /* 2 KB pages in Dual-Bank mode */
#define LOGGER_FLASH_DW_COUNT       (4U)           /* 32 B frame = 4 x 64-bit writes */

/* Frame status flag bitmask */
#define LOGGER_FLAG_CRC_SELFTEST_OK (1U << 0U)
#define LOGGER_FLAG_DBANK_ACTIVE    (1U << 1U)
#define LOGGER_FLAG_OLED_PRESENT    (1U << 2U)
#define LOGGER_FLAG_BANK2_DRYRUN_OK (1U << 3U)
#define LOGGER_FLAG_HALF1_SOURCE    (1U << 4U)

/* --------------------------------------------------------------------------
 * 64-Bit-Aligned Binary Telemetry & Flash Log Frame (Exactly 32 Bytes)
 * -------------------------------------------------------------------------- */
typedef struct __attribute__((aligned(8))) {
    uint32_t magic;           /**< Offset  0: LOGGER_FRAME_MAGIC ("LOG2")         */
    uint32_t seq_id;          /**< Offset  4: Monotonic sequence counter          */
    uint32_t timestamp_ms;    /**< Offset  8: SysTick timestamp in milliseconds   */
    uint16_t vdda_mv;         /**< Offset 12: Actual VDDA via VREFINT_CAL (mV)    */
    uint16_t vref_raw_mean;   /**< Offset 14: Mean 12-bit ADC1_IN18 raw code      */
    uint16_t temp_raw_min;    /**< Offset 16: Min  12-bit ADC1_IN16 raw code      */
    uint16_t temp_raw_mean;   /**< Offset 18: Mean 12-bit ADC1_IN16 raw code      */
    uint16_t temp_raw_max;    /**< Offset 20: Max  12-bit ADC1_IN16 raw code      */
    int16_t  die_temp_deci_c; /**< Offset 22: Calibrated die temp (0.1 deg C)     */
    uint16_t dma_overruns;    /**< Offset 24: Unconsumed half-buffer overrun cnt  */
    uint16_t flags;           /**< Offset 26: LOGGER_FLAG_* bitmask               */
    uint32_t crc32_ieee;      /**< Offset 28: Hardware IEEE 802.3 CRC-32 [0..27]  */
} logger_frame_t;

_Static_assert(sizeof(logger_frame_t) == 32U,
               "logger_frame_t must be exactly 32 bytes");
_Static_assert((sizeof(logger_frame_t) % 8U) == 0U,
               "logger_frame_t size must be a multiple of 8 bytes for STM32G4 64-bit Flash writes");
_Static_assert(offsetof(logger_frame_t, crc32_ieee) == 28U,
               "logger_frame_t.crc32_ieee must sit at exact byte offset 28");

/* --------------------------------------------------------------------------
 * Safe Dual-Bank Flash Bank 2 Staging Record (Dry-Run RAM Buffer by Default)
 * -------------------------------------------------------------------------- */
typedef struct {
    uint32_t target_addr;                      /**< Next 8-byte-aligned Bank 2 address */
    uint64_t double_words[LOGGER_FLASH_DW_COUNT]; /**< 4 x 64-bit Flash programming words */
    bool     dbank_option_set;                 /**< Read-only snapshot of FLASH_OPTR.DBANK */
    bool     dry_run_verified;                 /**< Alignment, range, & pack verified  */
} flash_bank2_staged_t;

/**
 * @brief Compute next wear-leveled 32-byte slot address inside Flash Bank 2
 *        (0x08040000 .. 0x0807FFE0) without ever touching Flash option bytes.
 */
static inline uint32_t logger_bank2_slot_addr(uint32_t seq_id) {
    uint32_t slots_per_bank = (LOGGER_FLASH_BANK2_END - LOGGER_FLASH_BANK2_BASE) /
                              (uint32_t)sizeof(logger_frame_t);
    uint32_t slot_index     = seq_id % slots_per_bank;
    return LOGGER_FLASH_BANK2_BASE + (slot_index * (uint32_t)sizeof(logger_frame_t));
}

/**
 * @brief Pack a CRC-verified logger_frame_t into 4 x 64-bit double-words in RAM
 *        and verify STM32G4 Flash Bank 2 alignment and address bounds.
 * @note  Strict-aliasing safe via memcpy(). Does NOT erase/program Flash or
 *        modify FLASH->OPTR option bytes (dry-run RAM verification by default).
 */
static inline bool logger_stage_bank2_dryrun(const logger_frame_t *frame,
                                             bool dbank_active,
                                             flash_bank2_staged_t *out_staged) {
    if ((frame == NULL) || (out_staged == NULL)) {
        return false;
    }

    uint32_t addr = logger_bank2_slot_addr(frame->seq_id);
    bool aligned  = ((addr & 0x7UL) == 0UL);
    bool in_bank2 = (addr >= LOGGER_FLASH_BANK2_BASE) &&
                    ((addr + (uint32_t)sizeof(logger_frame_t)) <= LOGGER_FLASH_BANK2_END);

    out_staged->target_addr      = addr;
    out_staged->dbank_option_set = dbank_active;

    if (!aligned || !in_bank2) {
        out_staged->dry_run_verified = false;
        return false;
    }

    memcpy(out_staged->double_words, frame, sizeof(logger_frame_t));
    out_staged->dry_run_verified = true;
    return true;
}

#ifdef __cplusplus
}
#endif

#endif /* LOGGER_PACKET_H */
