/**
 * @file    dma_g4.c
 * @brief   Bare-metal CMSIS DMA1 / DMA2 + DMAMUX1 driver implementation.
 */

#include "dma_g4.h"
#include <stddef.h>

int8_t dma_dmamux_channel_index(uint8_t dma_num, uint8_t ch_num) {
    if ((ch_num < 1U) || (ch_num > 8U)) {
        return -1;
    }
    if (dma_num == 1U) {
        return (int8_t)(ch_num - 1U);
    }
    if (dma_num == 2U) {
        return (int8_t)(8U + (ch_num - 1U));
    }
    return -1;
}

#if !defined(DRIVER_HOST_TEST)

static DMA_Channel_TypeDef *dma_get_channel(const DMA_TypeDef *dma, uint8_t ch_num) {
    static DMA_Channel_TypeDef *const k_dma1_ch[8] = {
        DMA1_Channel1, DMA1_Channel2, DMA1_Channel3, DMA1_Channel4,
        DMA1_Channel5, DMA1_Channel6, DMA1_Channel7, DMA1_Channel8
    };
    static DMA_Channel_TypeDef *const k_dma2_ch[8] = {
        DMA2_Channel1, DMA2_Channel2, DMA2_Channel3, DMA2_Channel4,
        DMA2_Channel5, DMA2_Channel6, DMA2_Channel7, DMA2_Channel8
    };

    if ((ch_num < 1U) || (ch_num > 8U)) {
        return NULL;
    }
    if (dma == DMA1) {
        return k_dma1_ch[ch_num - 1U];
    }
    if (dma == DMA2) {
        return k_dma2_ch[ch_num - 1U];
    }
    return NULL;
}

static DMAMUX_Channel_TypeDef *dma_get_dmamux_channel(const DMA_TypeDef *dma, uint8_t ch_num) {
    static DMAMUX_Channel_TypeDef *const k_mux_ch[16] = {
        DMAMUX1_Channel0,  DMAMUX1_Channel1,  DMAMUX1_Channel2,  DMAMUX1_Channel3,
        DMAMUX1_Channel4,  DMAMUX1_Channel5,  DMAMUX1_Channel6,  DMAMUX1_Channel7,
        DMAMUX1_Channel8,  DMAMUX1_Channel9,  DMAMUX1_Channel10, DMAMUX1_Channel11,
        DMAMUX1_Channel12, DMAMUX1_Channel13, DMAMUX1_Channel14, DMAMUX1_Channel15
    };

    uint8_t dma_num = (dma == DMA1) ? 1U : ((dma == DMA2) ? 2U : 0U);
    int8_t  idx     = dma_dmamux_channel_index(dma_num, ch_num);
    if ((idx < 0) || (idx > 15)) {
        return NULL;
    }
    return k_mux_ch[idx];
}

static IRQn_Type dma_get_irqn(const DMA_TypeDef *dma, uint8_t ch_num) {
    static const IRQn_Type k_dma1_irq[8] = {
        DMA1_Channel1_IRQn, DMA1_Channel2_IRQn, DMA1_Channel3_IRQn, DMA1_Channel4_IRQn,
        DMA1_Channel5_IRQn, DMA1_Channel6_IRQn, DMA1_Channel7_IRQn, DMA1_Channel8_IRQn
    };
    static const IRQn_Type k_dma2_irq[8] = {
        DMA2_Channel1_IRQn, DMA2_Channel2_IRQn, DMA2_Channel3_IRQn, DMA2_Channel4_IRQn,
        DMA2_Channel5_IRQn, DMA2_Channel6_IRQn, DMA2_Channel7_IRQn, DMA2_Channel8_IRQn
    };

    return (dma == DMA1) ? k_dma1_irq[ch_num - 1U] : k_dma2_irq[ch_num - 1U];
}

static bool dma_encode_width(uint8_t width_bytes, uint32_t *out_size_code) {
    switch (width_bytes) {
        case 1U: *out_size_code = 0U; return true; /* 8-bit  */
        case 2U: *out_size_code = 1U; return true; /* 16-bit */
        case 4U: *out_size_code = 2U; return true; /* 32-bit */
        default: return false;
    }
}

void dma_enable_clocks(const DMA_TypeDef *dma) {
    uint32_t mask = RCC_AHB1ENR_DMAMUX1EN;
    if (dma == DMA1) {
        mask |= RCC_AHB1ENR_DMA1EN;
    } else if (dma == DMA2) {
        mask |= RCC_AHB1ENR_DMA2EN;
    } else {
        return;
    }

    RCC->AHB1ENR |= mask;
    /* Readback barrier ensures AHB1 clock propagates before DMA/DMAMUX writes */
    (void)RCC->AHB1ENR;
}

void dma_clear_flags(DMA_TypeDef *dma, uint8_t ch_num,
                     bool clear_tc, bool clear_ht, bool clear_te) {
    if ((dma == NULL) || ((dma != DMA1) && (dma != DMA2)) || (ch_num < 1U) || (ch_num > 8U)) {
        return;
    }

    /* Each channel owns 4 bits in ISR/IFCR:
     *   bit +0 = GIF/CGIF (NEVER set CGIF per ES0430 §2.3.1!)
     *   bit +1 = TCIF/CTCIF
     *   bit +2 = HTIF/CHTIF
     *   bit +3 = TEIF/CTEIF */
    uint32_t shift = ((uint32_t)ch_num - 1U) * 4U;
    uint32_t mask  = 0U;

    if (clear_tc) {
        mask |= (1UL << (shift + 1U));
    }
    if (clear_ht) {
        mask |= (1UL << (shift + 2U));
    }
    if (clear_te) {
        mask |= (1UL << (shift + 3U));
    }

    if (mask != 0U) {
        dma->IFCR = mask;
        __DSB();
    }
}

dma_status_t dma_config_p2m_circular(DMA_TypeDef *dma, uint8_t ch_num,
                                     uint8_t dmamux_req_id,
                                     volatile const void *periph_addr,
                                     volatile void *mem_addr,
                                     uint16_t count, uint8_t width_bytes,
                                     bool enable_ht_tc_irq, uint32_t nvic_priority) {
    DMA_Channel_TypeDef    *ch  = dma_get_channel(dma, ch_num);
    DMAMUX_Channel_TypeDef *mux = dma_get_dmamux_channel(dma, ch_num);
    uint32_t size_code          = 0U;

    if ((ch == NULL) || (mux == NULL) || (periph_addr == NULL) || (mem_addr == NULL) ||
        (count == 0U) || !dma_encode_width(width_bytes, &size_code)) {
        return DMA_ERR_INVALID_ARG;
    }

    dma_enable_clocks(dma);

    /* 1. Disable channel cleanly before modifying configuration registers */
    ch->CCR = 0U;

    /* 2. Clear individual TC/HT/TE flags (never CGIF per ES0430 §2.3.1) */
    dma_clear_flags(dma, ch_num, true, true, true);

    /* 3. Route peripheral request through DMAMUX1 */
    mux->CCR = ((uint32_t)dmamux_req_id & DMAMUX_CxCR_DMAREQ_ID_Msk);

    /* 4. Configure addresses, element count, circular P2M mode */
    ch->CPAR  = (uint32_t)(uintptr_t)periph_addr;
    ch->CMAR  = (uint32_t)(uintptr_t)mem_addr;
    ch->CNDTR = (uint32_t)count;

    uint32_t ccr = DMA_CCR_CIRC |
                   DMA_CCR_MINC |
                   (size_code << DMA_CCR_PSIZE_Pos) |
                   (size_code << DMA_CCR_MSIZE_Pos);

    if (enable_ht_tc_irq) {
        ccr |= (DMA_CCR_HTIE | DMA_CCR_TCIE | DMA_CCR_TEIE);
        IRQn_Type irqn = dma_get_irqn(dma, ch_num);
        NVIC_SetPriority(irqn, nvic_priority);
        NVIC_ClearPendingIRQ(irqn);
        NVIC_EnableIRQ(irqn);
    }

    ch->CCR = ccr | DMA_CCR_EN;
    return DMA_OK;
}

dma_status_t dma_mem2mem_sync(DMA_TypeDef *dma, uint8_t ch_num,
                              const void *src, void *dst,
                              uint16_t count, uint8_t width_bytes,
                              uint32_t timeout_iters) {
    DMA_Channel_TypeDef *ch = dma_get_channel(dma, ch_num);
    uint32_t size_code      = 0U;

    if ((ch == NULL) || (src == NULL) || (dst == NULL) || (count == 0U) ||
        (timeout_iters == 0U) || !dma_encode_width(width_bytes, &size_code)) {
        return DMA_ERR_INVALID_ARG;
    }

    /* Validate pointer alignment for 16-bit / 32-bit bus transfers */
    uintptr_t align_mask = (uintptr_t)width_bytes - 1U;
    if ((((uintptr_t)src & align_mask) != 0U) || (((uintptr_t)dst & align_mask) != 0U)) {
        return DMA_ERR_INVALID_ARG;
    }

    dma_enable_clocks(dma);

    /* 1. Disable channel and clear individual TC/HT/TE flags */
    ch->CCR = 0U;
    dma_clear_flags(dma, ch_num, true, true, true);

    /* 2. In STM32 MEM2MEM with DIR=0: CPAR = source address, CMAR = destination address */
    ch->CPAR  = (uint32_t)(uintptr_t)src;
    ch->CMAR  = (uint32_t)(uintptr_t)dst;
    ch->CNDTR = (uint32_t)count;

    uint32_t ccr = DMA_CCR_MEM2MEM |
                   DMA_CCR_PINC |
                   DMA_CCR_MINC |
                   (size_code << DMA_CCR_PSIZE_Pos) |
                   (size_code << DMA_CCR_MSIZE_Pos);

    /* 3. Start transfer (data sync barrier ensures prior CPU writes to src are visible to DMA) */
    __DSB();
    ch->CCR = ccr | DMA_CCR_EN;

    uint32_t shift     = ((uint32_t)ch_num - 1U) * 4U;
    uint32_t tc_mask   = (1UL << (shift + 1U));
    uint32_t te_mask   = (1UL << (shift + 3U));
    uint32_t remaining = timeout_iters;
    dma_status_t st    = DMA_ERR_TIMEOUT;

    while (remaining > 0U) {
        uint32_t isr = dma->ISR;
        if ((isr & te_mask) != 0U) {
            st = DMA_ERR_TRANSFER;
            break;
        }
        if ((isr & tc_mask) != 0U) {
            st = DMA_OK;
            break;
        }
        --remaining;
    }

    /* 4. Disable channel and clear individual flags cleanly */
    ch->CCR = 0U;
    dma_clear_flags(dma, ch_num, true, true, true);
    __DSB();

    return st;
}

#endif /* !defined(DRIVER_HOST_TEST) */
