/**
 * @file    i2c_g4.h
 * @brief   Bare-metal CMSIS STM32G4 v2 I2C1 / I2C2 / I2C3 Master driver.
 *
 * Handles STM32G4-specific silicon details:
 *   - Verified UM10204-compliant `I2C_TIMINGR` profiles for BOTH 170 MHz and
 *     160 MHz (plus 16 MHz HSI16 fallback) at 100 kHz Standard & 400 kHz Fast Mode.
 *   - Hardware Repeated START (`AUTOEND=0 -> TC -> START|RD_WRN`) for `i2c_read_reg()`.
 *   - Transparent `RELOAD` + `TCR` 255-byte chunking for transfers > 255 bytes
 *     (e.g. 1024-byte SSD1306 framebuffers).
 *   - Checks `NACKF`, `BERR`, and `ARLO` on EVERY polling phase with bounded timeouts
 *     so missing slaves never deadlock the MCU.
 *   - Implements `i2c_recover_bus_9clks()` (NXP UM10204 §3.1.16) to unstick slaves
 *     holding SDA LOW across an MCU reset.
 *
 * @note  NUCLEO-G474RE Warning (UM2505): PB8 is `PB8-BOOT0` with an onboard 10 kOhm
 *        pull-down (R36). Attaching a permanent external 2.2 kOhm pull-up on PB8
 *        drives 2.705 V (> V_IH) at reset, booting the STM32G4 into ST System ROM
 *        Bootloader instead of User Flash unless Option Byte `nSWBOOT0 = 0` is
 *        programmed first! Always prefer `PA15` (I2C1_SCL, AF4) + `PB7` (I2C1_SDA,
 *        AF4) as the default NUCLEO-G474RE I2C1 wiring with external 2.2..4.7 kOhm
 *        pull-ups (internal ~40 kOhm pull-ups alone are insufficient for 400 kHz).
 *
 * Reference: RM0440 Chapter 39 (I2C), NXP UM10204 I2C-bus Specification.
 * Concepts:  PERIPH-I2C-001, PERIPH-I2C-002, PERIPH-I2C-003
 */

#ifndef I2C_G4_H
#define I2C_G4_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#if !defined(DRIVER_HOST_TEST)
#include "stm32g474xx.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

/** @brief I2C driver status/error codes. */
typedef enum {
    I2C_OK              = 0,  /**< Transaction completed cleanly                     */
    I2C_ERR_INVALID_ARG = -1, /**< NULL pointer, invalid 7-bit address (>0x7F), len=0 */
    I2C_ERR_NACK        = -2, /**< Slave NACKed address or data byte                 */
    I2C_ERR_BUS         = -3, /**< Hardware bus error (BERR: misplaced START/STOP)   */
    I2C_ERR_ARLO        = -4, /**< Arbitration lost (ARLO)                           */
    I2C_ERR_BUSY        = -5, /**< Bus stayed BUSY beyond timeout (run recovery!)    */
    I2C_ERR_TIMEOUT     = -6, /**< Bounded polling timeout expired                   */
    I2C_ERR_UNSUP_CLK   = -7  /**< No verified TIMINGR profile for current I2CCLK    */
} i2c_status_t;

/** @brief I2C bus speed class. */
typedef enum {
    I2C_SPEED_100KHZ = 0U, /**< Standard Mode (100 kHz, t_LOW >= 4.7 us, t_HIGH >= 4.0 us) */
    I2C_SPEED_400KHZ = 1U  /**< Fast Mode     (400 kHz, t_LOW >= 1.3 us, t_HIGH >= 0.6 us) */
} i2c_speed_t;

/**
 * @brief Verified STM32G4 v2 I2C_TIMINGR constants (ANFOFF=0, DNF=0):
 *        Format: [31:28] PRESC | [23:20] SCLDEL | [19:16] SDADEL | [15:8] SCLH | [7:0] SCLL
 *
 * Timing verification against NXP UM10204 & RM0440 §39.4.9 / Table 384:
 *   - 170 MHz / 100 kHz (0x70C45D6C): t_PRESC=47.06 ns, t_SCLL=5.13 us, t_SCLH=4.42 us
 *   - 160 MHz / 100 kHz (0x70C45866): t_PRESC=50.00 ns, t_SCLL=5.15 us, t_SCLH=4.45 us
 *   - 170 MHz / 400 kHz (0x70811021): t_PRESC=47.06 ns, t_SCLDEL=423.5 ns (>=400 ns),
 *                                     t_SCLL=1.60 us, t_SCLH=0.80 us (f_max=396.3 kHz)
 *   - 160 MHz / 400 kHz (0x70710F1F): t_PRESC=50.00 ns, t_SCLDEL=400.0 ns (>=400 ns),
 *                                     t_SCLL=1.60 us, t_SCLH=0.80 us (f_max=396.0 kHz)
 *   -  16 MHz / 100 kHz (0x30420F13): RM0440 Table 384 official value: t_PRESC=250 ns,
 *                                     t_SCLDEL=1250 ns, t_SCLL=5.00 us, t_SCLH=4.00 us (~98 kHz)
 *   -  16 MHz / 400 kHz (0x00310C16): t_PRESC=62.50 ns, t_SCLL=1.44 us, t_SCLH=0.81 us (<=385 kHz)
 */
#define I2C_TIMINGR_170MHZ_100KHZ 0x70C45D6CUL
#define I2C_TIMINGR_160MHZ_100KHZ 0x70C45866UL
#define I2C_TIMINGR_170MHZ_400KHZ 0x70811021UL
#define I2C_TIMINGR_160MHZ_400KHZ 0x70710F1FUL
#define I2C_TIMINGR_16MHZ_100KHZ  0x30420F13UL
#define I2C_TIMINGR_16MHZ_400KHZ  0x00310C16UL

/**
 * @brief Pure host-testable helper: select verified STM32G4 I2C_TIMINGR constant
 *        for @p i2cclk_hz (170 MHz, 160 MHz, or 16 MHz) and @p speed.
 *
 * @param i2cclk_hz  I2C peripheral kernel clock in Hz.
 * @param speed      I2C_SPEED_100KHZ or I2C_SPEED_400KHZ.
 * @return Non-zero TIMINGR register value, or 0 if clock/speed is unsupported.
 */
uint32_t i2c_select_timingr(uint32_t i2cclk_hz, i2c_speed_t speed);

#if !defined(DRIVER_HOST_TEST)

/**
 * @brief Initialize I2C1, I2C2, or I2C3 in 7-bit Master mode using the verified
 *        TIMINGR profile matching live SystemCoreClock / PCLK1.
 *
 * @note  Caller must configure SCL/SDA pins in Alternate Function Open-Drain mode
 *        with external pull-up resistors (2.2..4.7 kOhm).
 *
 * @param i2c    I2C1, I2C2, or I2C3.
 * @param speed  I2C_SPEED_100KHZ or I2C_SPEED_400KHZ.
 * @return I2C_OK or error status.
 */
i2c_status_t i2c_init_master(I2C_TypeDef *i2c, i2c_speed_t speed);

/**
 * @brief Master write of arbitrary length @p len (supports >255 bytes via RELOAD).
 * @param i2c            I2C1, I2C2, or I2C3.
 * @param addr7          7-bit unshifted slave address (0x00..0x7F).
 * @param data           Bytes to transmit.
 * @param len            Number of bytes (>= 1).
 * @param timeout_iters  Bounded polling iteration budget per flag wait.
 * @return I2C_OK or error status (I2C_ERR_NACK, I2C_ERR_TIMEOUT, etc.).
 */
i2c_status_t i2c_write(I2C_TypeDef *i2c, uint8_t addr7, const uint8_t *data,
                       size_t len, uint32_t timeout_iters);

/**
 * @brief Write @p reg followed by @p len bytes from @p data in a single I2C write
 *        transaction (supports arbitrary length via RELOAD chunking).
 */
i2c_status_t i2c_write_reg(I2C_TypeDef *i2c, uint8_t addr7, uint8_t reg,
                           const uint8_t *data, size_t len, uint32_t timeout_iters);

/**
 * @brief Read @p len bytes starting at device register @p reg using a hardware
 *        Repeated START (`START -> addr+W -> reg -> ReSTART -> addr+R -> data -> STOP`).
 *        Supports >255-byte reads via RELOAD chunking and checks NACKF on every phase.
 */
i2c_status_t i2c_read_reg(I2C_TypeDef *i2c, uint8_t addr7, uint8_t reg,
                          uint8_t *out_data, size_t len, uint32_t timeout_iters);

/**
 * @brief Bit-banged 9-clock SCL bus recovery + STOP sequence (NXP UM10204 §3.1.16)
 *        to release a slave holding SDA LOW after an interrupted transaction.
 *
 * @note  Temporarily reconfigures SCL/SDA as Open-Drain GPIO outputs. Caller MUST
 *        re-apply I2C Alternate Function pin configuration and call i2c_init_master()
 *        after recovery completes.
 *
 * @return true if SDA reads HIGH after recovery, false if line remains stuck LOW.
 */
bool i2c_recover_bus_9clks(GPIO_TypeDef *scl_port, uint8_t scl_pin,
                           GPIO_TypeDef *sda_port, uint8_t sda_pin);

#endif /* !defined(DRIVER_HOST_TEST) */

#ifdef __cplusplus
}
#endif

#endif /* I2C_G4_H */
