/**
 * @file    main.c
 * @brief   Project 02 — High-Speed Multichannel DMA Data Logger with Hardware
 *          IEEE 802.3 CRC-32 & Dual-Bank Flash Staging (Level 2 Capstone).
 *
 * Verified STM32G474RE (NUCLEO-G474RE) Onboard-Safe Pipeline:
 *   - System Clock : 170 MHz Range 1 Boost Mode via Clock_Init_170MHz()
 *   - Trigger      : TIM3 Update TRGO (MMS = 010b) @ exact 100.000 Hz
 *   - Acquisition  : ADC1 @ 42.5 MHz (CKMODE = 11b <= 60 MHz), hardware-triggered
 *                    by TIM3_TRGO (EXTSEL = 4, EXTEN = 01b rising edge), scanning
 *                    VREFINT (Ch18, SMP18 = 640.5 cyc) + Internal Die Temp Sensor
 *                    (Ch16, SMP16 = 640.5 cyc = 15.07 us >= 5 us DS12288 min!)
 *   - Ping-Pong DMA: DMA1_Channel1 via DMAMUX1_Channel0 Req ID 5 (ADC1), circular
 *                    mode with HTIE | TCIE | TEIE, clearing ONLY individual
 *                    CHTIF1 / CTCIF1 / CTEIF1 flags (never CGIF1 per ES0430 §2.3.1!)
 *   - Integrity    : Hardware IEEE 802.3 CRC-32 (CRC @ 0x40023000) verified at boot
 *                    on "123456789" -> 0xCBF43926 and computed over every 32-byte
 *                    64-bit-aligned logger_frame_t
 *   - Telemetry    : USART2 (PA2/PA3, 115200 8N1) framed ASCII + hex CRC-32 output
 *   - Optional OLED: I2C1 on PA15 (SCL, AF4) / PB7 (SDA, AF4) @ 400 kHz with bounded
 *                    NACKF/timeout probe (EXTERNAL HARDWARE PENDING — never hangs!)
 *   - Flash Bank 2 : Read-only FLASH_OPTR.DBANK inspection + 64-bit double-word
 *                    Bank 2 (0x08040000) dry-run RAM staging helper (NEVER modifies
 *                    Flash option bytes!)
 */

#include "logger_packet.h"
#include "stm32g474xx.h"

#include "adc_g4.h"
#include "clock_g4.h"
#include "dma_g4.h"
#include "fault_handler.h"
#include "gpio_g4.h"
#include "i2c_g4.h"
#include "iwdg_g4.h"
#include "tim_g4.h"
#include "usart_g4.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

/* --------------------------------------------------------------------------
 * DMA Ping-Pong Buffer & ISR Ownership State (DMA-Accessible SRAM1/2)
 * -------------------------------------------------------------------------- */
static volatile uint16_t g_adc_ping_pong[LOGGER_FULL_WORDS];

static volatile uint32_t g_half0_ready  = 0U;
static volatile uint32_t g_half1_ready  = 0U;
static volatile uint32_t g_dma_overruns = 0U;
static volatile uint32_t g_dma_te_count = 0U;
static volatile uint32_t g_systick_ms   = 0U;

static bool                 g_crc_selftest_ok = false;
static bool                 g_oled_present    = false;
static bool                 g_dbank_active    = false;
static uint32_t             g_seq_counter     = 0U;
static flash_bank2_staged_t g_last_staged;

/* --------------------------------------------------------------------------
 * Interrupt Service Routines
 * -------------------------------------------------------------------------- */
void SysTick_Handler(void) {
    g_systick_ms++;
}

void DMA1_Channel1_IRQHandler(void) {
    uint32_t isr = DMA1->ISR;
    bool     ht  = ((isr & DMA_ISR_HTIF1) != 0U);
    bool     tc  = ((isr & DMA_ISR_TCIF1) != 0U);
    bool     te  = ((isr & DMA_ISR_TEIF1) != 0U);

    /* ES0430 §2.3.1: Clear ONLY individual CTCIF1/CHTIF1/CTEIF1 — NEVER CGIF1! */
    dma_clear_flags(DMA1, 1U, tc, ht, te);

    if (ht) {
        if (g_half0_ready != 0U) {
            g_dma_overruns++;
        }
        g_half0_ready = 1U;
    }
    if (tc) {
        if (g_half1_ready != 0U) {
            g_dma_overruns++;
        }
        g_half1_ready = 1U;
    }
    if (te) {
        g_dma_te_count++;
    }
}

/* --------------------------------------------------------------------------
 * Hardware IEEE 802.3 CRC-32 Engine (CRC @ 0x40023000, RM0440 Ch. 16)
 * -------------------------------------------------------------------------- */
static void crc32_hw_init(void) {
    RCC->AHB1ENR |= RCC_AHB1ENR_CRCEN;
    (void)RCC->AHB1ENR; /* Mandatory AHB1 clock sync readback */

    CRC->POL  = 0x04C11DB7UL;
    CRC->INIT = 0xFFFFFFFFUL;
    /* 32-bit poly (POLYSIZE=00), byte-wise input reflect (REV_IN=01), output reflect */
    CRC->CR   = CRC_CR_REV_IN_0 | CRC_CR_REV_OUT | CRC_CR_RESET;
}

static uint32_t crc32_hw_compute(const uint8_t *data, size_t len) {
    CRC->POL  = 0x04C11DB7UL;
    CRC->INIT = 0xFFFFFFFFUL;
    CRC->CR   = CRC_CR_REV_IN_0 | CRC_CR_REV_OUT | CRC_CR_RESET;

    volatile uint8_t *dr8 = (volatile uint8_t *)(uintptr_t)&CRC->DR;
    for (size_t i = 0U; i < len; ++i) {
        *dr8 = data[i];
    }
    return CRC->DR ^ 0xFFFFFFFFUL;
}

/* --------------------------------------------------------------------------
 * UART Output Helper
 * -------------------------------------------------------------------------- */
static void uart_send_str(const char *msg) {
    (void)usart_write_str(USART2, msg, LOGGER_UART_TX_TIMEOUT);
}

/* --------------------------------------------------------------------------
 * Optional I2C1 SSD1306 OLED Hook (PA15 SCL / PB7 SDA, AF4, 400 kHz)
 * Bounded NACKF/timeout ensures firmware NEVER hangs when OLED is absent!
 * -------------------------------------------------------------------------- */
static bool oled_probe_and_init_optional(void) {
    /* PA15 (I2C1_SCL, AF4) & PB7 (I2C1_SDA, AF4): avoids PB8-BOOT0 bootloader trap! */
    (void)GPIO_ConfigAF(GPIOA, 15U, 4U, GPIO_OTYPE_OPENDRAIN,
                        GPIO_SPEED_HIGH, GPIO_PULL_UP);
    (void)GPIO_ConfigAF(GPIOB, 7U, 4U, GPIO_OTYPE_OPENDRAIN,
                        GPIO_SPEED_HIGH, GPIO_PULL_UP);

    if (i2c_init_master(I2C1, I2C_SPEED_400KHZ) != I2C_OK) {
        return false;
    }

    /* Minimal SSD1306 probe: send Display OFF command (0x00, 0xAE) to 0x3C */
    static const uint8_t k_probe_cmd[2] = { 0x00U, 0xAEU };
    if (i2c_write(I2C1, LOGGER_SSD1306_ADDR7, k_probe_cmd,
                  sizeof(k_probe_cmd), LOGGER_I2C_TIMEOUT) != I2C_OK) {
        return false;
    }

    /* Basic SSD1306 wake sequence when external display is actually attached */
    static const uint8_t k_init_cmds[] = {
        0x00U, /* Control byte: command stream */
        0x8DU, 0x14U, /* Enable charge pump */
        0xAFU         /* Display ON */
    };
    return (i2c_write(I2C1, LOGGER_SSD1306_ADDR7, k_init_cmds,
                      sizeof(k_init_cmds), LOGGER_I2C_TIMEOUT) == I2C_OK);
}

static void oled_heartbeat_optional(uint32_t seq_id) {
    if (!g_oled_present) {
        return;
    }
    /* Toggle Normal (0xA6) vs Inverse (0xA7) on bit 3 as minimal live liveness pulse */
    uint8_t cmd[2] = { 0x00U, ((seq_id & 0x08U) != 0U) ? 0xA7U : 0xA6U };
    if (i2c_write(I2C1, LOGGER_SSD1306_ADDR7, cmd, sizeof(cmd), LOGGER_I2C_TIMEOUT) != I2C_OK) {
        /* If OLED is hot-unplugged, degrade gracefully without stalling DMA logging */
        g_oled_present = false;
    }
}

/* --------------------------------------------------------------------------
 * TIM3 TRGO (100.000 Hz) + ADC1 Multichannel Scan + DMA1_Ch1 Ping-Pong Setup
 * -------------------------------------------------------------------------- */
static bool logger_tim3_trgo_init_100hz(void) {
    RCC->APB1ENR1 |= RCC_APB1ENR1_TIM3EN;
    (void)RCC->APB1ENR1;

    uint32_t tim_clk = TIM_GetKernelClockHz(TIM3);
    if (tim_clk < 100000U) {
        return false;
    }

    TIM3->CR1  = 0U;
    TIM3->DIER = 0U;
    /* 170 MHz / 1700 = 100 kHz timer tick; ARR = 999 -> exact 100.000 Hz TRGO */
    TIM3->PSC  = (uint16_t)((tim_clk / 100000U) - 1U);
    TIM3->ARR  = (100000U / LOGGER_SAMPLE_RATE_HZ) - 1U; /* 999U */
    TIM3->CNT  = 0U;

    /* MMS = 010b: Update Event drives TIM3_TRGO (RM0440 §29.4.2) */
    TIM3->CR2  = TIM_CR2_MMS_1;
    TIM3->CR1  = TIM_CR1_ARPE;
    TIM3->EGR  = TIM_EGR_UG;
    TIM3->SR   = ~TIM_SR_UIF;
    return true;
}

static bool logger_adc_dma_start(void) {
    /* 1. Initialize ADC1 cleanly via shared driver (CKMODE=11b -> 42.5 MHz, ADCAL, ADEN) */
    if (adc_init_single_ended(ADC1, LOGGER_ADC_TIMEOUT) != ADC_OK) {
        return false;
    }

    /* 2. Enable internal VREFINT + Temp Sensor paths and wait >= 120 us t_START (DS12288) */
    ADC12_COMMON->CCR |= ADC_CCR_VREFEN | ADC_CCR_VSENSESEL;
    Clock_DelayUs(150U);

    /* 3. Configure SMPR2: SMP16 = 640.5 cycles (15.07 us >= 5 us min!) & SMP18 = 640.5 cycles */
    uint32_t smp16_shift = (ADC1_CH_TEMP_SENSOR - 10U) * 3U; /* bit 18 */
    uint32_t smp18_shift = (ADC1_CH_VREFINT     - 10U) * 3U; /* bit 24 */
    ADC1->SMPR2 = (ADC1->SMPR2 & ~((0x7UL << smp16_shift) | (0x7UL << smp18_shift))) |
                  ((uint32_t)ADC_SMP_640_5_CYCLES << smp16_shift)                    |
                  ((uint32_t)ADC_SMP_640_5_CYCLES << smp18_shift);

    /* 4. Regular scan sequence: L = 1 (2 channels), SQ1 = VREFINT (18), SQ2 = Temp (16) */
    ADC1->SQR1 = (((uint32_t)LOGGER_NUM_CHANNELS - 1U) << ADC_SQR1_L_Pos) |
                 ((uint32_t)ADC1_CH_VREFINT            << ADC_SQR1_SQ1_Pos) |
                 ((uint32_t)ADC1_CH_TEMP_SENSOR        << ADC_SQR1_SQ2_Pos);

    /* 5. Configure ADC1 CFGR:
     *    - DMAEN = 1, DMACFG = 1 (circular DMA request mode)
     *    - OVRMOD = 1 (overwrite on overrun so ADC never freezes)
     *    - EXTSEL = 4 (0b00100 = TIM3_TRGO, RM0440 Table 163 / LL_ADC_REG_TRIG_EXT_TIM3_TRGO)
     *    - EXTEN  = 1 (0b01 = rising edge hardware trigger)
     *    - CONT   = 0 (one 2-channel scan per TIM3_TRGO pulse @ 100 Hz) */
    ADC1->CFGR = (ADC1->CFGR & ~(ADC_CFGR_CONT | ADC_CFGR_EXTSEL | ADC_CFGR_EXTEN)) |
                 ADC_CFGR_DMAEN |
                 ADC_CFGR_DMACFG |
                 ADC_CFGR_OVRMOD |
                 (4UL << ADC_CFGR_EXTSEL_Pos) |
                 (1UL << ADC_CFGR_EXTEN_Pos);

    /* 6. Arm DMA1_Channel1 via DMAMUX1_Channel0 Req ID 5 (ADC1), circular 16-bit */
    if (dma_config_p2m_circular(DMA1, 1U, (uint8_t)DMAMUX_REQ_ADC1,
                                &ADC1->DR, g_adc_ping_pong,
                                LOGGER_FULL_WORDS, 2U, true, 3U) != DMA_OK) {
        return false;
    }

    /* 7. Clear stale ADC flags, start regular conversion wait-for-trigger, start TIM3 */
    ADC1->ISR = ADC_ISR_EOC | ADC_ISR_EOS | ADC_ISR_OVR;
    ADC1->CR |= ADC_CR_ADSTART;
    TIM3->CR1 |= TIM_CR1_CEN;
    return true;
}

/* --------------------------------------------------------------------------
 * Ping-Pong Half-Buffer DSP, Hardware CRC-32 Framing & Telemetry Streaming
 * -------------------------------------------------------------------------- */
static void logger_process_half(uint32_t word_offset, bool is_half1) {
    uint32_t vref_sum = 0U;
    uint32_t temp_sum = 0U;
    uint16_t temp_min = 0xFFFFU;
    uint16_t temp_max = 0U;

    /* Mandatory data memory barrier before reading DMA-populated ping-pong half */
    __DMB();
    for (uint32_t i = 0U; i < LOGGER_SCANS_PER_HALF; ++i) {
        uint16_t vref_raw = g_adc_ping_pong[word_offset + (i * LOGGER_NUM_CHANNELS) + 0U];
        uint16_t temp_raw = g_adc_ping_pong[word_offset + (i * LOGGER_NUM_CHANNELS) + 1U];
        vref_sum += vref_raw;
        temp_sum += temp_raw;
        if (temp_raw < temp_min) {
            temp_min = temp_raw;
        }
        if (temp_raw > temp_max) {
            temp_max = temp_raw;
        }
    }

    uint16_t vref_mean = (uint16_t)(vref_sum / LOGGER_SCANS_PER_HALF);
    uint16_t temp_mean = (uint16_t)(temp_sum / LOGGER_SCANS_PER_HALF);

    /* Factory calibration ROM words (DS12288 §3.14) */
    uint16_t vref_cal = *(const volatile uint16_t *)(uintptr_t)ADC_VREFINT_CAL_ADDR;
    uint16_t ts_cal1  = *(const volatile uint16_t *)(uintptr_t)ADC_TS_CAL1_ADDR;
    uint16_t ts_cal2  = *(const volatile uint16_t *)(uintptr_t)ADC_TS_CAL2_ADDR;

    uint32_t vdda_mv    = adc_calc_vdda_mv(vref_mean, vref_cal);
    int32_t  temp_mdegc = adc_temp_mdegc(temp_mean, ts_cal1, ts_cal2, vdda_mv);
    int16_t  temp_decic = (int16_t)(temp_mdegc / 100);

    uint16_t flags = 0U;
    if (g_crc_selftest_ok) {
        flags |= LOGGER_FLAG_CRC_SELFTEST_OK;
    }
    if (g_dbank_active) {
        flags |= LOGGER_FLAG_DBANK_ACTIVE;
    }
    if (g_oled_present) {
        flags |= LOGGER_FLAG_OLED_PRESENT;
    }
    if (is_half1) {
        flags |= LOGGER_FLAG_HALF1_SOURCE;
    }

    logger_frame_t frame;
    frame.magic           = LOGGER_FRAME_MAGIC;
    frame.seq_id          = g_seq_counter++;
    frame.timestamp_ms    = g_systick_ms;
    frame.vdda_mv         = (uint16_t)((vdda_mv > 65535U) ? 65535U : vdda_mv);
    frame.vref_raw_mean   = vref_mean;
    frame.temp_raw_min    = temp_min;
    frame.temp_raw_mean   = temp_mean;
    frame.temp_raw_max    = temp_max;
    frame.die_temp_deci_c = temp_decic;
    frame.dma_overruns    = (uint16_t)((g_dma_overruns > 65535U) ? 65535U : g_dma_overruns);
    frame.flags           = (uint16_t)(flags | LOGGER_FLAG_BANK2_DRYRUN_OK);

    /* Compute hardware IEEE 802.3 CRC-32 over header + payload bytes [0..27] */
    frame.crc32_ieee = crc32_hw_compute((const uint8_t *)&frame,
                                        offsetof(logger_frame_t, crc32_ieee));

    /* Stage 64-bit double-words in RAM for Flash Bank 2 (dry-run; never alters OPTR!) */
    bool staged_ok = logger_stage_bank2_dryrun(&frame, g_dbank_active, &g_last_staged);
    uint32_t verify_crc = crc32_hw_compute((const uint8_t *)g_last_staged.double_words,
                                           offsetof(logger_frame_t, crc32_ieee));
    bool crc_match = staged_ok && (verify_crc == frame.crc32_ieee);

    /* Emit structured framed telemetry line over USART2 VCP */
    char    line[192];
    int32_t abs_decic = (temp_decic < 0) ? -(int32_t)temp_decic : (int32_t)temp_decic;
    (void)snprintf(
        line, sizeof(line),
        "$LOG2,seq=%lu,t_ms=%lu,half=%u,vdda_mv=%u,temp=%d.%01ldC,"
        "raw=[%u,%u,%u],ovr=%u,te=%lu,b2=0x%08lX,crc32=0x%08lX,ok=%u\r\n",
        (unsigned long)frame.seq_id,
        (unsigned long)frame.timestamp_ms,
        is_half1 ? 1U : 0U,
        (unsigned int)frame.vdda_mv,
        (int)(temp_decic / 10),
        (long)(abs_decic % 10),
        (unsigned int)frame.temp_raw_min,
        (unsigned int)frame.temp_raw_mean,
        (unsigned int)frame.temp_raw_max,
        (unsigned int)frame.dma_overruns,
        (unsigned long)g_dma_te_count,
        (unsigned long)g_last_staged.target_addr,
        (unsigned long)frame.crc32_ieee,
        crc_match ? 1U : 0U);
    uart_send_str(line);

    oled_heartbeat_optional(frame.seq_id);
    GPIO_TogglePin(GPIOA, 5U);
}

/* --------------------------------------------------------------------------
 * Application Entry Point
 * -------------------------------------------------------------------------- */
int main(void) {
    FaultHandler_EnableDetailedFaults(true);
    NVIC_SetPriorityGrouping(0U);

    if (!Clock_Init_170MHz()) {
        while (1) {
            __NOP();
        }
    }
    Clock_DWT_Init();

    /* Enable AHB2 peripheral clocks before touching GPIOA/GPIOB registers */
    GPIO_EnableClock(GPIOA);
    GPIO_EnableClock(GPIOB);

    /* Configure LD2 (PA5) and USART2 (PA2/PA3 AF7) */
    (void)GPIO_ConfigOutput(GPIOA, 5U, GPIO_OTYPE_PUSHPULL,
                            GPIO_SPEED_LOW, GPIO_PULL_NONE);
    GPIO_ResetPin(GPIOA, 5U);

    (void)GPIO_ConfigAF(GPIOA, 2U, 7U, GPIO_OTYPE_PUSHPULL,
                        GPIO_SPEED_MEDIUM, GPIO_PULL_NONE);
    (void)GPIO_ConfigAF(GPIOA, 3U, 7U, GPIO_OTYPE_PUSHPULL,
                        GPIO_SPEED_MEDIUM, GPIO_PULL_UP);

    (void)SysTick_Config(Clock_GetSysClkHz() / 1000U);
    NVIC_SetPriority(SysTick_IRQn, 2U);

    (void)usart_init(USART2, LOGGER_UART_BAUD, false, 0U);

    /* Inspect Dual-Bank option bit READ-ONLY (never modifies Flash option bytes!) */
    g_dbank_active = ((FLASH->OPTR & FLASH_OPTR_DBANK) != 0U);

    /* Hardware CRC-32 initialization & IEEE 802.3 known-answer self-test */
    crc32_hw_init();
    static const uint8_t k_check_vec[9] = { '1', '2', '3', '4', '5', '6', '7', '8', '9' };
    uint32_t crc_check = crc32_hw_compute(k_check_vec, sizeof(k_check_vec));
    g_crc_selftest_ok  = (crc_check == LOGGER_CRC32_CHECK_EXPECTED);

    /* Probe optional I2C1 SSD1306 OLED on PA15/PB7 (bounded NACKF timeout) */
    g_oled_present = oled_probe_and_init_optional();

    uart_send_str("\r\n============================================================\r\n");
    uart_send_str("Project 02: High-Speed DMA Data Logger + CRC-32 (G474RE)\r\n");
    {
        char banner[128];
        (void)snprintf(banner, sizeof(banner),
                       "[BOOT] SYSCLK=%lu Hz | CRC32_TEST=0x%08lX (%s)\r\n",
                       (unsigned long)Clock_GetSysClkHz(),
                       (unsigned long)crc_check,
                       g_crc_selftest_ok ? "PASS" : "FAIL");
        uart_send_str(banner);

        (void)snprintf(banner, sizeof(banner),
                       "[BOOT] FLASH_OPTR.DBANK=%u (dry-run Bank 2 0x08040000 staging)\r\n",
                       g_dbank_active ? 1U : 0U);
        uart_send_str(banner);

        (void)snprintf(banner, sizeof(banner),
                       "[BOOT] I2C1 OLED (PA15/PB7 0x3C): %s\r\n",
                       g_oled_present ? "DETECTED" : "ABSENT (EXTERNAL HARDWARE PENDING)");
        uart_send_str(banner);
    }
    uart_send_str("============================================================\r\n");

    /* Start TIM3 TRGO (100 Hz) -> ADC1 Scan -> DMA1_Ch1 Ping-Pong pipeline */
    if (!logger_tim3_trgo_init_100hz() || !logger_adc_dma_start()) {
        uart_send_str("[FATAL] Failed to start TIM3/ADC1/DMA1 pipeline!\r\n");
        while (1) {
            __NOP();
        }
    }

    (void)iwdg_init_ms(LOGGER_IWDG_TIMEOUT_MS, true, 500000U);

    while (1) {
        if (g_half0_ready != 0U) {
            g_half0_ready = 0U;
            logger_process_half(0U, false);
        }
        if (g_half1_ready != 0U) {
            g_half1_ready = 0U;
            logger_process_half(LOGGER_HALF_WORDS, true);
        }

        if (g_dma_te_count == 0U) {
            iwdg_kick();
        }
    }
}
